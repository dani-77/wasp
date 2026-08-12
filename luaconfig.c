/* See luaconfig.h for the overall shape of this. This first pass only
 * covers appearance (border color/width, root background, the built-in
 * bar's enable/position/layout) -- enough to prove the whole pipeline
 * (find the file, run it, read values back into the globals dwl.c already
 * uses) end to end. Keybindings/autostart/rules etc. build on top of this
 * same waspconfig_load() later; see NOTES.md.
 */
#include "luaconfig.h"

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Storage for the globals luaconfig.h declares `extern`. Same values
 * config.def.h used to hardcode -- see set_defaults() below, which (re)sets
 * all of them before we ever consult Lua, so a missing/broken config.lua
 * still yields a fully-usable compositor. */
unsigned int borderpx;
int showbar;
int topbar;
const char *barlayout;
float rootcolor[4];
uint32_t colors[3][3]; /* [SchemeNorm|SchemeSel|SchemeUrg][fg|bg|border] */

static void
set_defaults(void)
{
	borderpx = 1;
	showbar = 1;
	topbar = 1;
	barlayout = "tln|s";
	rootcolor[0] = rootcolor[1] = rootcolor[2] = 0.0f;
	rootcolor[3] = 1.0f;
	colors[0][0] = 0xbbbbbbff; colors[0][1] = 0x222222ff; colors[0][2] = 0x444444ff; /* SchemeNorm */
	colors[1][0] = 0xeeeeeeff; colors[1][1] = 0x005577ff; colors[1][2] = 0x005577ff; /* SchemeSel */
	colors[2][0] = 0;          colors[2][1] = 0;          colors[2][2] = 0x770000ff; /* SchemeUrg */
}

/* ~/.config/wasp/config.lua, or $XDG_CONFIG_HOME/wasp/config.lua if set.
 * Returned buffer is static -- copy it if it needs to outlive the next
 * call. */
static const char *
config_path(void)
{
	static char path[1024];
	const char *xdg = getenv("XDG_CONFIG_HOME");
	const char *home;
	struct passwd *pw;

	if (xdg && *xdg) {
		snprintf(path, sizeof(path), "%s/wasp/config.lua", xdg);
		return path;
	}

	home = getenv("HOME");
	if (!home || !*home) {
		pw = getpwuid(getuid());
		home = pw ? pw->pw_dir : "/";
	}
	snprintf(path, sizeof(path), "%s/.config/wasp/config.lua", home);
	return path;
}

/* "#rrggbb" or "#rrggbbaa" (alpha defaults to ff) -> 0xRRGGBBAA, the same
 * encoding config.def.h's COLOR() macro and the `colors[][3]` table use.
 * Returns 1 and writes *out on success, 0 (leaving *out untouched) on any
 * malformed input. */
static int
parse_hex_color(const char *s, uint32_t *out)
{
	size_t len;
	char buf[9] = "ffffffff";
	char *end;
	unsigned long v;

	if (!s || s[0] != '#')
		return 0;

	len = strlen(s + 1);
	if (len != 6 && len != 8)
		return 0;

	memcpy(buf, s + 1, len);

	v = strtoul(buf, &end, 16);
	if (*end)
		return 0;

	*out = (uint32_t)v;
	return 1;
}

static void
read_string_field(lua_State *L, int tblidx, const char *key, const char **out)
{
	lua_getfield(L, tblidx, key);
	if (lua_isstring(L, -1))
		*out = strdup(lua_tostring(L, -1)); /* leaked on purpose for now -- see NOTES.md on reload */
	lua_pop(L, 1);
}

static void
read_color_field(lua_State *L, int tblidx, const char *key, uint32_t *out)
{
	lua_getfield(L, tblidx, key);
	if (lua_isstring(L, -1)) {
		uint32_t v;
		if (parse_hex_color(lua_tostring(L, -1), &v))
			*out = v;
	}
	lua_pop(L, 1);
}

static void
read_bool_field(lua_State *L, int tblidx, const char *key, int *out)
{
	lua_getfield(L, tblidx, key);
	if (lua_isboolean(L, -1))
		*out = lua_toboolean(L, -1);
	lua_pop(L, 1);
}

static void
read_int_field(lua_State *L, int tblidx, const char *key, unsigned int *out)
{
	lua_getfield(L, tblidx, key);
	if (lua_isnumber(L, -1))
		*out = (unsigned int)lua_tointeger(L, -1);
	lua_pop(L, 1);
}

static void
load_border(lua_State *L, int wasptbl)
{
	int t;

	lua_getfield(L, wasptbl, "border");
	if (!lua_istable(L, -1)) {
		lua_pop(L, 1);
		return;
	}
	t = lua_gettop(L);
	read_int_field(L, t, "width", &borderpx);
	read_color_field(L, t, "focus", &colors[1][2]);  /* SchemeSel  border */
	read_color_field(L, t, "normal", &colors[0][2]); /* SchemeNorm border */
	lua_pop(L, 1);
}

static void
load_bar(lua_State *L, int wasptbl)
{
	int t;

	lua_getfield(L, wasptbl, "bar");
	if (!lua_istable(L, -1)) {
		lua_pop(L, 1);
		return;
	}
	t = lua_gettop(L);
	read_bool_field(L, t, "enable", &showbar);
	read_bool_field(L, t, "top", &topbar);
	read_string_field(L, t, "layout", &barlayout);
	lua_pop(L, 1);
}

static void
load_background(lua_State *L, int wasptbl)
{
	lua_getfield(L, wasptbl, "background");
	if (lua_isstring(L, -1)) {
		uint32_t v;
		if (parse_hex_color(lua_tostring(L, -1), &v)) {
			rootcolor[0] = ((v >> 24) & 0xFF) / 255.0f;
			rootcolor[1] = ((v >> 16) & 0xFF) / 255.0f;
			rootcolor[2] = ((v >> 8)  & 0xFF) / 255.0f;
			rootcolor[3] = (v & 0xFF) / 255.0f;
		}
	}
	lua_pop(L, 1);
}

void
waspconfig_load(void)
{
	lua_State *L;
	const char *path;

	set_defaults();

	L = luaL_newstate();
	luaL_openlibs(L);

	path = config_path();
	if (luaL_dofile(L, path) != LUA_OK) {
		fprintf(stderr, "wasp: %s: %s\n", path, lua_tostring(L, -1));
		lua_close(L);
		return; /* keep the defaults set_defaults() already put in place */
	}

	lua_getglobal(L, "wasp");
	if (lua_istable(L, -1)) {
		int wasptbl = lua_gettop(L);
		load_border(L, wasptbl);
		load_bar(L, wasptbl);
		load_background(L, wasptbl);
	}
	lua_pop(L, 1); /* the wasp global (or whatever lua_getglobal pushed) */

	lua_close(L);
}
