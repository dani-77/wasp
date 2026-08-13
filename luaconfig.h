/* Lua-driven configuration.
 *
 * Appearance globals below are the same ones dwl.c has always read from
 * config.h -- same names, same types (minus `static const`), just no
 * longer baked in at compile time. Keybindings are handled differently:
 * dwl.c's `Key` array used to be a compile-time `static const` table in
 * config.h; it's now built at runtime by luaconfig.c from
 * `wasp.keys` in config.lua (falling back to a small built-in emergency
 * set from set_defaults() -- see luaconfig.c -- if the file is missing,
 * broken, or just doesn't set `wasp.keys`, so you're never locked out).
 *
 * waspconfig_load() populates all of this from ~/.config/wasp/config.lua
 * (or $XDG_CONFIG_HOME/wasp/config.lua) at startup.
 *
 * Arg/Key live here (moved out of dwl.c) because both dwl.c and
 * luaconfig.c need the exact same definition: dwl.c to keep using them
 * exactly as config.h always did (action functions, keybinding()'s
 * dispatch loop), luaconfig.c to build Key values out of parsed Lua data.
 */
#ifndef WASP_LUACONFIG_H
#define WASP_LUACONFIG_H

#include <stdint.h>
#include <xkbcommon/xkbcommon.h>

typedef union {
	int i;
	uint32_t ui;
	float f;
	const void *v;
} Arg;

typedef struct {
	uint32_t mod;
	xkb_keysym_t keysym;
	void (*func)(const Arg *);
	Arg arg; /* not const -- luaconfig.c builds these at runtime */
} Key;

typedef void (*ActionFn)(const Arg *);

/* appearance -- see luaconfig.c set_defaults() for values */
extern unsigned int borderpx;
extern int showbar;
extern int topbar;
extern const char *barlayout;
extern float rootcolor[4];
extern uint32_t colors[3][3];

/* agnostic launcher commands -- NULL-terminated argv, heap-allocated.
 * config.def.h's buttons[] (mouse bindings, still static for now) reads
 * these too, via the extern declarations here. */
extern const char **termcmd;
extern const char **menucmd;

/* keyboard -- xkb_rules feeds xkb_keymap_new_from_names() directly in
 * dwl.c's createkeyboardgroup(); empty/NULL fields mean "let xkbcommon
 * pick its own default" (in practice "us"), same as upstream dwl. */
extern struct xkb_rule_names xkb_rules;
extern int repeat_rate;
extern int repeat_delay;

/* gaps -- gapsouter insets a monitor's usable area on all 4 sides before
 * tile()/monocle()/dwindle() lay clients out in it; gapsinner insets each
 * client's own rect by half its value (so two adjacent windows end up
 * gapsinner pixels apart). gapsmart, if set, drops the outer gap when
 * there's only one tiled window on screen. Pixels, all three. */
extern unsigned int gapsinner;
extern unsigned int gapsouter;
extern int gapsmart;

/* keybindings, rebuilt from scratch on every waspconfig_load() call. */
extern Key *keys;
extern size_t nkeys;

/* wasp.autostart = { {"swaybg","-i","~/wallpaper.png"}, {"foot"} } -- an
 * array of argv arrays (NULL-terminated overall, each argv itself
 * NULL-terminated -- same shape as termcmd/menucmd, one level up). Only
 * ever *read* here -- actually spawning them (dwl.c's autostartexec(),
 * fork+execvp once at real startup) is deliberately not part of
 * waspconfig_load() itself, or every hot-reload (see reload() in dwl.c)
 * would respawn everything all over again. */
extern const char ***autostart;
extern size_t nautostart;

/* wasp.scratchpad = { { name=, cmd={...}, app_id=, w=, h= }, ... } -- named
 * scratchpad slots, toggled by the "toggle-scratchpad" action (its arg is
 * the slot's `name`). `cmd` is spawned (dwl.c's spawn(), fork+execvp, same
 * as termcmd/menucmd) the first time a slot is toggled with nothing
 * running yet; `app_id` is what dwl.c's mapnotify() matches against
 * client_get_appid() to claim the freshly-spawned client for this slot
 * (defaults to `name` itself if omitted -- the common case is spawning
 * with a matching --app-id/--class). `w`/`h` are the fraction (0, 1] of
 * the monitor's usable area the client is centered and sized to when
 * shown; default 0.6 each if omitted or out of range.
 *
 * This struct is *config data only* -- which live Client, if any, belongs
 * to a given slot is tracked on the Client itself (dwl.c's `scratchpad`
 * field, the slot's name) rather than here, because `scratchpads` is
 * rebuilt from scratch on every waspconfig_load() call (same as `keys`)
 * and would otherwise lose track of an already-spawned client across a
 * hot-reload. See dwl.c's togglescratchpad(). */
typedef struct {
	const char *name;
	const char **cmd;
	const char *app_id;
	float w, h;
} Scratchpad;

extern Scratchpad *scratchpads;
extern size_t nscratchpads;

/* wasp.rules = { { app_id=, title=, tags=, floating=, monitor=, center= },
 * ... } -- dwl's classic per-app-id/title placement rule table, moved
 * here (out of dwl.c/config.def.h, where it used to be a `static const
 * Rule rules[]` array) for the same reason Key/Arg live here: dwl.c's
 * applyrules() and luaconfig.c's load_rules() both need the exact same
 * struct layout. `app_id`/`title` are substring-matched against the
 * client's own (NULL = matches anything); `tags` is a 1..9 workspace
 * number (0 = don't force one, i.e. leave whatever the target monitor's
 * own active tagset already is); `monitor` is a 0-based output index (-1
 * = don't force one); a client matching more than one rule gets every
 * matched rule's `tags` OR'd together, but only the *last* match's
 * `isfloating`/`monitor`/`center` (same last-match-wins semantics
 * upstream dwl's applyrules() already had for those two fields).
 * `center` re-centers a floating client at its own requested size once
 * placed (dwl.c's centeredgeom() -- the same formula wasp.scratchpad
 * already uses to center a shown scratchpad); no effect on a tiled
 * client. */
typedef struct {
	const char *id;
	const char *title;
	uint32_t tags;
	int isfloating;
	int monitor;
	int center;
} Rule;

extern Rule *rules;
extern size_t nrules;

/* Loads (or reloads) the config, overwriting the globals above in place.
 * Safe to call again later for a hot-reload once callers redraw/rearrange
 * afterwards -- nothing here restarts the compositor, and it never
 * touches autostart_pids/spawns anything itself (see dwl.c's
 * autostartexec() vs. reload()). */
void waspconfig_load(void);

/* Bridge into dwl.c: luaconfig.c is a separate translation unit and can't
 * see dwl.c's `static` action functions or its `layouts[]` table (defined
 * by config.h, included only into dwl.c) directly. dwl.c defines both of
 * these, right after `#include "config.h"`, and hands values over by name
 * instead. wasp_layout_by_name() returns `const void *` (really `const
 * Layout *`) so this header doesn't need the Layout type at all. */
ActionFn wasp_lookup_action(const char *name);
const void *wasp_layout_by_name(const char *name);

#endif
