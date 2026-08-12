/* See luaconfig.h for the overall shape of this. Covers appearance
 * (border color/width, root background, the built-in bar's
 * enable/position/layout), the agnostic terminal/menu launcher commands,
 * and keybindings -- config.lua's `wasp.keys` array is turned into the
 * `Key keys[]` dwl.c's keybinding() dispatch loop walks. Autostart/rules
 * etc. build on top of this same waspconfig_load() later; see NOTES.md.
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

#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_output_layout.h>

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
const char **termcmd;
const char **menucmd;
Key *keys;
size_t nkeys;
struct xkb_rule_names xkb_rules;
int repeat_rate;
int repeat_delay;
unsigned int gapsinner;
unsigned int gapsouter;
int gapsmart;
const char ***autostart;
size_t nautostart;

/* Primary modifier for config.lua keybinds that say mods = {"mod", ...} --
 * set via wasp.modkey ("alt"/"ctrl"/"super"/"shift"), defaults to Alt (what
 * config.def.h used to hardcode as MODKEY). Only used while building
 * keys[] below; dwl.c's static buttons[] (mouse bindings, not Lua-driven
 * yet) still uses its own compile-time MODKEY macro. */
static uint32_t modkey;

static const char *default_termcmd[] = { "alacritty", NULL };
static const char *default_menucmd[] = { "wmenu-run", NULL };

/* Emergency-fallback keymap: enough to never be locked out (terminal, menu,
 * window/workspace nav, kill, quit, VT switch) if config.lua is missing,
 * broken, or just doesn't set wasp.keys. The real, fuller suggested keymap
 * (layouts, monitor nav, resize, moveresizekb, ...) lives in
 * examples/config.lua as data, not duplicated here in C. */
#define NDEFAULTKEYS 38
static Key default_keys[NDEFAULTKEYS];

static void
build_default_keys(void)
{
	static const xkb_keysym_t tagsyms[9] = {
		XKB_KEY_1, XKB_KEY_2, XKB_KEY_3, XKB_KEY_4, XKB_KEY_5,
		XKB_KEY_6, XKB_KEY_7, XKB_KEY_8, XKB_KEY_9,
	};
	static const xkb_keysym_t tagshiftsyms[9] = {
		XKB_KEY_exclam, XKB_KEY_at, XKB_KEY_numbersign, XKB_KEY_dollar,
		XKB_KEY_percent, XKB_KEY_asciicircum, XKB_KEY_ampersand,
		XKB_KEY_asterisk, XKB_KEY_parenleft,
	};
	static const xkb_keysym_t vtsyms[12] = {
		XKB_KEY_XF86Switch_VT_1, XKB_KEY_XF86Switch_VT_2, XKB_KEY_XF86Switch_VT_3,
		XKB_KEY_XF86Switch_VT_4, XKB_KEY_XF86Switch_VT_5, XKB_KEY_XF86Switch_VT_6,
		XKB_KEY_XF86Switch_VT_7, XKB_KEY_XF86Switch_VT_8, XKB_KEY_XF86Switch_VT_9,
		XKB_KEY_XF86Switch_VT_10, XKB_KEY_XF86Switch_VT_11, XKB_KEY_XF86Switch_VT_12,
	};
	ActionFn spawnfn = wasp_lookup_action("spawn");
	ActionFn focusstackfn = wasp_lookup_action("focusstack");
	ActionFn viewfn = wasp_lookup_action("view");
	ActionFn tagfn = wasp_lookup_action("tag");
	ActionFn killclientfn = wasp_lookup_action("killclient");
	ActionFn quitfn = wasp_lookup_action("quit");
	ActionFn togglebarfn = wasp_lookup_action("togglebar");
	ActionFn chvtfn = wasp_lookup_action("chvt");
	int n = 0;
	int i;

	default_keys[n++] = (Key){ modkey | WLR_MODIFIER_SHIFT, XKB_KEY_Return, spawnfn, (Arg){.v = termcmd} };
	default_keys[n++] = (Key){ modkey, XKB_KEY_p, spawnfn, (Arg){.v = menucmd} };
	default_keys[n++] = (Key){ modkey, XKB_KEY_j, focusstackfn, (Arg){.i = +1} };
	default_keys[n++] = (Key){ modkey, XKB_KEY_k, focusstackfn, (Arg){.i = -1} };
	default_keys[n++] = (Key){ modkey, XKB_KEY_b, togglebarfn, (Arg){0} };
	default_keys[n++] = (Key){ modkey | WLR_MODIFIER_SHIFT, XKB_KEY_c, killclientfn, (Arg){0} };
	default_keys[n++] = (Key){ modkey | WLR_MODIFIER_SHIFT, XKB_KEY_q, quitfn, (Arg){0} };
	default_keys[n++] = (Key){ WLR_MODIFIER_CTRL | WLR_MODIFIER_ALT, XKB_KEY_Terminate_Server, quitfn, (Arg){0} };

	for (i = 0; i < 9; i++) {
		default_keys[n++] = (Key){ modkey, tagsyms[i], viewfn, (Arg){.ui = 1u << i} };
		default_keys[n++] = (Key){ modkey | WLR_MODIFIER_SHIFT, tagshiftsyms[i], tagfn, (Arg){.ui = 1u << i} };
	}

	for (i = 0; i < 12; i++)
		default_keys[n++] = (Key){ WLR_MODIFIER_CTRL | WLR_MODIFIER_ALT, vtsyms[i], chvtfn, (Arg){.ui = (unsigned int)(i + 1)} };

	keys = default_keys;
	nkeys = (size_t)n;
}

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
	modkey = WLR_MODIFIER_ALT;
	termcmd = default_termcmd;
	menucmd = default_menucmd;
	build_default_keys(); /* needs termcmd/menucmd already set, above */
	memset(&xkb_rules, 0, sizeof xkb_rules); /* all NULL -> xkbcommon's own default ("us") */
	repeat_rate = 25;
	repeat_delay = 600;
	gapsinner = 0;
	gapsouter = 0;
	gapsmart = 0;
	autostart = NULL; /* re-parsed data only -- doesn't touch already-spawned
	                    * processes, see autostartexec()/reload() in dwl.c */
	nautostart = 0;
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

/* wasp.gaps = { inner = <px>, outer = <px>, smart = <bool> } -- see
 * gapsinner/gapsouter/gapsmart in luaconfig.h for what each does; consumed
 * by tile()/monocle()/dwindle() in dwl.c. */
static void
load_gaps(lua_State *L, int wasptbl)
{
	int t;

	lua_getfield(L, wasptbl, "gaps");
	if (!lua_istable(L, -1)) {
		lua_pop(L, 1);
		return;
	}
	t = lua_gettop(L);
	read_int_field(L, t, "inner", &gapsinner);
	read_int_field(L, t, "outer", &gapsouter);
	read_bool_field(L, t, "smart", &gapsmart);
	lua_pop(L, 1);
}

/* wasp.keyboard = { layout=, variant=, model=, options=, rules=,
 * repeat_rate=, repeat_delay= } -- layout/variant/model/options/rules are
 * xkbcommon RMLVO fields (empty/omitted -> xkbcommon's own default, "us"
 * in practice); repeat_rate is repeats/second, repeat_delay is ms before
 * the first repeat. Same table shape as spitfire's wasp.keyboard. */
static void
load_keyboard(lua_State *L, int wasptbl)
{
	int t;

	lua_getfield(L, wasptbl, "keyboard");
	if (!lua_istable(L, -1)) {
		lua_pop(L, 1);
		return;
	}
	t = lua_gettop(L);
	read_string_field(L, t, "layout", &xkb_rules.layout);
	read_string_field(L, t, "variant", &xkb_rules.variant);
	read_string_field(L, t, "model", &xkb_rules.model);
	read_string_field(L, t, "options", &xkb_rules.options);
	read_string_field(L, t, "rules", &xkb_rules.rules);

	lua_getfield(L, t, "repeat_rate");
	if (lua_isnumber(L, -1))
		repeat_rate = (int)lua_tointeger(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, t, "repeat_delay");
	if (lua_isnumber(L, -1))
		repeat_delay = (int)lua_tointeger(L, -1);
	lua_pop(L, 1);

	lua_pop(L, 1); /* keyboard table */
}

/* wasp.modkey = "alt"|"ctrl"|"super"|"shift" -- what "mod" means inside a
 * wasp.keys entry's mods array. Doesn't touch dwl.c's static buttons[]
 * (mouse bindings still use MODKEY from config.def.h). */
static uint32_t
basemod_from_name(const char *name)
{
	if (!strcmp(name, "alt"))
		return WLR_MODIFIER_ALT;
	if (!strcmp(name, "ctrl") || !strcmp(name, "control"))
		return WLR_MODIFIER_CTRL;
	if (!strcmp(name, "super") || !strcmp(name, "logo") || !strcmp(name, "win"))
		return WLR_MODIFIER_LOGO;
	if (!strcmp(name, "shift"))
		return WLR_MODIFIER_SHIFT;
	return 0;
}

static void
load_modkey(lua_State *L, int wasptbl)
{
	lua_getfield(L, wasptbl, "modkey");
	if (lua_isstring(L, -1)) {
		uint32_t m = basemod_from_name(lua_tostring(L, -1));
		if (m)
			modkey = m;
	}
	lua_pop(L, 1);
}

/* Per-keybind modifier name, same as basemod_from_name() plus "mod" ->
 * whatever wasp.modkey resolved to. */
static uint32_t
keybit_from_name(const char *name)
{
	if (!strcmp(name, "mod"))
		return modkey;
	return basemod_from_name(name);
}

/* Reads a Lua array of strings at stack index tblidx into a freshly
 * malloc'd NULL-terminated argv. Returns NULL for an empty/non-array
 * table. Leaked on purpose -- see NOTES.md on reload. */
static const char **
build_argv(lua_State *L, int tblidx)
{
	int n = (int)lua_rawlen(L, tblidx);
	const char **argv;
	int i;

	if (n <= 0)
		return NULL;
	argv = malloc((size_t)(n + 1) * sizeof(char *));
	if (!argv)
		return NULL;
	for (i = 1; i <= n; i++) {
		lua_rawgeti(L, tblidx, i);
		argv[i - 1] = lua_isstring(L, -1) ? strdup(lua_tostring(L, -1)) : "";
		lua_pop(L, 1);
	}
	argv[n] = NULL;
	return argv;
}

static void
load_terminal_menu(lua_State *L, int wasptbl)
{
	lua_getfield(L, wasptbl, "terminal");
	if (lua_istable(L, -1)) {
		const char **argv = build_argv(L, lua_gettop(L));
		if (argv)
			termcmd = argv;
	}
	lua_pop(L, 1);

	lua_getfield(L, wasptbl, "menu");
	if (lua_istable(L, -1)) {
		const char **argv = build_argv(L, lua_gettop(L));
		if (argv)
			menucmd = argv;
	}
	lua_pop(L, 1);
}

/* wasp.autostart = { {"swaybg","-i","~/wallpaper.png"}, {"foot"} } -- an
 * array of argv arrays, same shape as termcmd/menucmd one level up. Only
 * parses/stores the data here (dwl.c's autostartexec() is what actually
 * spawns it, once, at real startup -- see luaconfig.h). Entries that
 * aren't tables, or are empty tables, are skipped rather than aborting
 * the whole list. */
static void
load_autostart(lua_State *L, int wasptbl)
{
	int t, n, i;
	const char ***list;
	size_t count = 0;

	lua_getfield(L, wasptbl, "autostart");
	if (!lua_istable(L, -1)) {
		lua_pop(L, 1);
		return;
	}
	t = lua_gettop(L);
	n = (int)lua_rawlen(L, t);
	if (n <= 0) {
		lua_pop(L, 1);
		return;
	}

	list = malloc((size_t)(n + 1) * sizeof(const char **));
	if (!list) {
		lua_pop(L, 1);
		return;
	}

	for (i = 1; i <= n; i++) {
		const char **argv;
		lua_rawgeti(L, t, i);
		argv = lua_istable(L, -1) ? build_argv(L, lua_gettop(L)) : NULL;
		lua_pop(L, 1);
		if (argv)
			list[count++] = argv;
	}
	list[count] = NULL;

	if (count > 0) {
		autostart = list; /* leaked on reload -- see NOTES.md */
		nautostart = count;
	} else {
		free(list);
	}
	lua_pop(L, 1); /* autostart table */
}

/* Builds the Arg for one wasp.keys[i] entry, based on its action name.
 * Every action wasp_lookup_action() (dwl.c) knows about needs a case here
 * -- see the comment above dwl.c's actiontable for the full list. */
static Arg
build_key_arg(lua_State *L, int e, const char *action)
{
	Arg arg;
	memset(&arg, 0, sizeof arg);

	if (!strcmp(action, "spawn")) {
		lua_getfield(L, e, "cmd");
		if (lua_istable(L, -1))
			arg.v = build_argv(L, lua_gettop(L));
		lua_pop(L, 1);
	} else if (!strcmp(action, "spawn-terminal")) {
		arg.v = termcmd;
	} else if (!strcmp(action, "spawn-menu")) {
		arg.v = menucmd;
	} else if (!strcmp(action, "focusstack") || !strcmp(action, "incnmaster")) {
		arg.i = 1;
		lua_getfield(L, e, "dir");
		if (lua_isnumber(L, -1))
			arg.i = (int)lua_tointeger(L, -1);
		lua_pop(L, 1);
	} else if (!strcmp(action, "setmfact")) {
		lua_getfield(L, e, "delta");
		if (lua_isnumber(L, -1))
			arg.f = (float)lua_tonumber(L, -1);
		lua_pop(L, 1);
	} else if (!strcmp(action, "view") || !strcmp(action, "toggleview")
			|| !strcmp(action, "tag") || !strcmp(action, "toggletag")) {
		/* tag = 1..9 -> that workspace's bit; tag = "all" -> every tag
		 * (arg.ui = ~0, e.g. MODKEY+0's "view everything"); omitted ->
		 * arg.ui = 0, which for view()/tag() means "toggle back to the
		 * previously selected tags" (e.g. MODKEY+Tab) rather than "all". */
		lua_getfield(L, e, "tag");
		if (lua_isnumber(L, -1)) {
			int tag = (int)lua_tointeger(L, -1);
			arg.ui = tag <= 0 ? 0u : (1u << (tag - 1));
		} else if (lua_isstring(L, -1) && !strcmp(lua_tostring(L, -1), "all")) {
			arg.ui = ~0u;
		} else {
			arg.ui = 0;
		}
		lua_pop(L, 1);
	} else if (!strcmp(action, "focusmon") || !strcmp(action, "tagmon")) {
		lua_getfield(L, e, "dir");
		arg.i = (lua_isstring(L, -1) && !strcmp(lua_tostring(L, -1), "right"))
			? WLR_DIRECTION_RIGHT : WLR_DIRECTION_LEFT;
		lua_pop(L, 1);
	} else if (!strcmp(action, "setlayout")) {
		lua_getfield(L, e, "layout");
		if (lua_isstring(L, -1))
			arg.v = wasp_layout_by_name(lua_tostring(L, -1));
		lua_pop(L, 1); /* no layout field -> {0}, same as MODKEY+space always meant: cycle */
	} else if (!strcmp(action, "chvt")) {
		lua_getfield(L, e, "vt");
		if (lua_isnumber(L, -1))
			arg.ui = (unsigned int)lua_tointeger(L, -1);
		lua_pop(L, 1);
	} else if (!strcmp(action, "moveresizekb")) {
		static const char *fields[4] = { "dx", "dy", "dw", "dh" };
		int *delta = malloc(4 * sizeof(int));
		int i;
		if (delta) {
			for (i = 0; i < 4; i++) {
				delta[i] = 0;
				lua_getfield(L, e, fields[i]);
				if (lua_isnumber(L, -1))
					delta[i] = (int)lua_tointeger(L, -1);
				lua_pop(L, 1);
			}
			arg.v = delta;
		}
	}
	/* togglefloating/togglefullscreen/togglebar/zoom/killclient/quit take
	 * no arg -- the zeroed Arg from memset above is exactly right. */

	return arg;
}

/* wasp.keys = { { mods = {"mod","shift"}, key = "Return", action = "spawn",
 *                 cmd = {"alacritty"} }, ... }
 * `key` is resolved via xkb_keysym_from_name() -- anything libxkbcommon
 * recognises by name works (letters, "Return", "F1", "XF86AudioMute", ...).
 * Entries that don't parse (bad key name, unknown action) are silently
 * skipped rather than aborting the whole table -- see NOTES.md. */
static void
load_keys(lua_State *L, int wasptbl)
{
	int t, n, i;
	Key *buf;
	size_t count = 0;

	lua_getfield(L, wasptbl, "keys");
	if (!lua_istable(L, -1)) {
		lua_pop(L, 1);
		return;
	}
	t = lua_gettop(L);
	n = (int)lua_rawlen(L, t);
	if (n <= 0) {
		lua_pop(L, 1);
		return;
	}

	buf = calloc((size_t)n, sizeof(Key));
	if (!buf) {
		lua_pop(L, 1);
		return;
	}

	for (i = 1; i <= n; i++) {
		int e;
		uint32_t mod = 0;
		const char *action;
		ActionFn fn;
		xkb_keysym_t sym;

		lua_rawgeti(L, t, i);
		if (!lua_istable(L, -1)) {
			lua_pop(L, 1);
			continue;
		}
		e = lua_gettop(L);

		lua_getfield(L, e, "mods");
		if (lua_istable(L, -1)) {
			int mt = lua_gettop(L);
			int mn = (int)lua_rawlen(L, mt), mi;
			for (mi = 1; mi <= mn; mi++) {
				lua_rawgeti(L, mt, mi);
				if (lua_isstring(L, -1))
					mod |= keybit_from_name(lua_tostring(L, -1));
				lua_pop(L, 1);
			}
		}
		lua_pop(L, 1); /* mods */

		lua_getfield(L, e, "key");
		sym = lua_isstring(L, -1)
			? xkb_keysym_from_name(lua_tostring(L, -1), XKB_KEYSYM_CASE_INSENSITIVE)
			: XKB_KEY_NoSymbol;
		lua_pop(L, 1); /* key */
		if (sym == XKB_KEY_NoSymbol) {
			lua_pop(L, 1); /* entry table */
			continue;
		}

		lua_getfield(L, e, "action");
		action = lua_isstring(L, -1) ? lua_tostring(L, -1) : NULL;
		fn = action ? wasp_lookup_action(action) : NULL;
		lua_pop(L, 1); /* action */
		if (!fn) {
			lua_pop(L, 1); /* entry table */
			continue;
		}

		buf[count].mod = mod;
		buf[count].keysym = sym;
		buf[count].func = fn;
		buf[count].arg = build_key_arg(L, e, action);
		count++;

		lua_pop(L, 1); /* entry table */
	}
	lua_pop(L, 1); /* keys table */

	if (count > 0) {
		keys = buf; /* leaked on reload -- see NOTES.md */
		nkeys = count;
	} else {
		free(buf);
	}
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
		load_gaps(L, wasptbl);
		load_keyboard(L, wasptbl);
		load_modkey(L, wasptbl);       /* before load_keys(): "mod" resolution */
		load_terminal_menu(L, wasptbl); /* before load_keys(): spawn-terminal/-menu */
		load_keys(L, wasptbl);
		load_autostart(L, wasptbl);
	}
	lua_pop(L, 1); /* the wasp global (or whatever lua_getglobal pushed) */

	lua_close(L);
}
