/* Lua-driven configuration.
 *
 * Appearance globals below are the same ones wasp.c has always read from
 * config.h -- same names, same types (minus `static const`), just no
 * longer baked in at compile time. Keybindings are handled differently:
 * wasp.c's `Key` array used to be a compile-time `static const` table in
 * config.h; it's now built at runtime by luaconfig.c from
 * `wasp.keys` in config.lua (falling back to a small built-in emergency
 * set from set_defaults() -- see luaconfig.c -- if the file is missing,
 * broken, or just doesn't set `wasp.keys`, so you're never locked out).
 *
 * waspconfig_load() populates all of this from ~/.config/wasp/config.lua
 * (or $XDG_CONFIG_HOME/wasp/config.lua) at startup.
 *
 * Arg/Key live here (moved out of wasp.c) because both wasp.c and
 * luaconfig.c need the exact same definition: wasp.c to keep using them
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
 * wasp.c's createkeyboardgroup(); empty/NULL fields mean "let xkbcommon
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

/* wasp.animations = { enable=, duration_move=, duration_open=,
 * duration_close=, duration_tag=, type_open=, type_close=, zoom_ratio=,
 * fade_from_opacity=, tag_direction=, curve_move=, curve_open=,
 * curve_close=, curve_tag= } -- MangoWC-style open/close/move/tag-switch
 * window tweening on top of wlr_scene, see NOTES.md item 3. `enable =
 * false` (the default) reproduces wasp.c's original instant behavior
 * bit-for-bit. Durations are milliseconds; `type_open`/`type_close` are
 * "fade" (opacity only), "zoom" (shrink/grow around center by
 * `zoom_ratio`, also fades) or "none". `tag_direction` is which monitor
 * edge tag-switches slide to/from: "left"/"right"/"top"/"bottom". The
 * curve_* fields are CSS cubic-bezier()-style control points
 * {x1,y1,x2,y2} (endpoints pinned at (0,0)/(1,1)) baked into a lookup
 * table by init_anim_curves() (wasp.c) every time this table is
 * (re)loaded -- see ease() in wasp.c for why a LUT instead of solving the
 * Bezier analytically. */
struct wasp_bezier { float x1, y1, x2, y2; };

extern int animations_enable;
extern unsigned int animdur_move, animdur_open, animdur_close, animdur_tag;
extern const char *animtype_open, *animtype_close;
extern float animzoom_ratio, animfade_from_opacity;
extern const char *animtag_direction;
extern struct wasp_bezier animbz_move, animbz_open, animbz_close, animbz_tag;

/* Bakes animbz_* into wasp.c's per-action easing lookup tables. Defined in
 * wasp.c (not luaconfig.c) since it's wasp.c's own ease()/animate_client()
 * that consume the tables; called at the end of load_animations() so a
 * hot-reload picks up new curves immediately, same as gaps. */
void init_anim_curves(void);

/* keybindings, rebuilt from scratch on every waspconfig_load() call. */
extern Key *keys;
extern size_t nkeys;

/* wasp.autostart = { {"swaybg","-i","~/wallpaper.png"}, {"foot"} } -- an
 * array of argv arrays (NULL-terminated overall, each argv itself
 * NULL-terminated -- same shape as termcmd/menucmd, one level up). Only
 * ever *read* here -- actually spawning them (wasp.c's autostartexec(),
 * fork+execvp once at real startup) is deliberately not part of
 * waspconfig_load() itself, or every hot-reload (see reload() in wasp.c)
 * would respawn everything all over again. */
extern const char ***autostart;
extern size_t nautostart;

/* wasp.scratchpad = { { name=, cmd={...}, app_id=, w=, h= }, ... } -- named
 * scratchpad slots, toggled by the "toggle-scratchpad" action (its arg is
 * the slot's `name`). `cmd` is spawned (wasp.c's spawn(), fork+execvp, same
 * as termcmd/menucmd) the first time a slot is toggled with nothing
 * running yet; `app_id` is what wasp.c's mapnotify() matches against
 * client_get_appid() to claim the freshly-spawned client for this slot
 * (defaults to `name` itself if omitted -- the common case is spawning
 * with a matching --app-id/--class). `w`/`h` are the fraction (0, 1] of
 * the monitor's usable area the client is centered and sized to when
 * shown; default 0.6 each if omitted or out of range.
 *
 * This struct is *config data only* -- which live Client, if any, belongs
 * to a given slot is tracked on the Client itself (wasp.c's `scratchpad`
 * field, the slot's name) rather than here, because `scratchpads` is
 * rebuilt from scratch on every waspconfig_load() call (same as `keys`)
 * and would otherwise lose track of an already-spawned client across a
 * hot-reload. See wasp.c's togglescratchpad(). */
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
 * here (out of wasp.c/config.def.h, where it used to be a `static const
 * Rule rules[]` array) for the same reason Key/Arg live here: wasp.c's
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
 * placed (wasp.c's centeredgeom() -- the same formula wasp.scratchpad
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

/* wasp.monitors = { { name=, mfact=, nmaster=, scale=, layout=,
 * transform=, x=, y= }, ... } -- dwl's classic per-output monitor rule
 * table (upstream's `monrules[]`, `config.def.h`), moved here the same
 * way Rule/Scratchpad were: wasp.c's createmon()/reload() and
 * luaconfig.c's load_monitors() both need the exact same struct layout.
 * `name` is a substring match against the output's own name (NULL/""
 * matches any output); a monitor uses the *first* matching rule, not
 * every one that matches (unlike Rule's tags, which OR-accumulate --
 * matches upstream dwl's own monrules[] semantics, preserved here on
 * purpose, not an oversight). `x`/`y` are layout position (-1 = let
 * wlroots auto-place it); `lt` is `const void *` here (really `const
 * Layout *`) for the same reason Arg.v is when it holds a layout --
 * this header can't see wasp.c's `Layout` type at all, see
 * wasp_layout_by_name(). `rr` is `uint32_t` here (really `enum
 * wl_output_transform`) so this header doesn't need a wlroots/wayland
 * include just for one enum.
 *
 * Only `scale` is live -- reload() re-applies it (by re-matching each
 * already-connected monitor's name against this array) to whichever
 * monitor(s) it applies to; `mfact`/`nmaster`/`layout`/`transform`/`x`/`y`
 * are createmon()-time-only, same as they always were, so reload()
 * doesn't stomp on a live `setmfact`/`incnmaster`/`setlayout` tweak the
 * way blindly re-applying all of monrules[] every reload would. */
typedef struct {
	const char *name;
	float mfact;
	int nmaster;
	float scale;
	const void *lt;
	uint32_t rr;
	int x, y;
} MonitorRule;

extern MonitorRule *monrules;
extern size_t nmonrules;

/* wasp.gestures = { { fingers=, direction=, action=, ... }, ... } -- touchpad
 * swipe gestures (pinch/hold aren't wired up -- libinput/wlroots deliver
 * them separately and nothing here listens for either yet, see NOTES.md
 * item 8). Dispatched through the exact same action/arg machinery as
 * wasp.keys, not a separate gesture-only action set: `action` is any name
 * wasp_lookup_action() (wasp.c) knows, and per-action extra fields (`tag`,
 * `dir`, `cmd`, ...) are read by luaconfig.c's build_key_arg() -- the very
 * same function wasp.keys entries already use. `fingers` is the exact
 * finger count to match (0 = any); `direction` is "left"/"right"/"up"/
 * "down", classified by wasp.c's swipeend() from the gesture's summed delta
 * once it ends (dominant axis, then sign). A missing/empty wasp.gestures is
 * a safe default, same as wasp.rules -- no gestures configured just means
 * swipes do nothing. */
typedef struct {
	int fingers;
	const char *direction;
	void (*func)(const Arg *);
	Arg arg;
} Gesture;

extern Gesture *gestures;
extern size_t ngestures;

/* Loads (or reloads) the config, overwriting the globals above in place.
 * Safe to call again later for a hot-reload once callers redraw/rearrange
 * afterwards -- nothing here restarts the compositor, and it never
 * touches autostart_pids/spawns anything itself (see wasp.c's
 * autostartexec() vs. reload()). */
void waspconfig_load(void);

/* Bridge into wasp.c: luaconfig.c is a separate translation unit and can't
 * see wasp.c's `static` action functions or its `layouts[]` table (defined
 * by config.h, included only into wasp.c) directly. wasp.c defines both of
 * these, right after `#include "config.h"`, and hands values over by name
 * instead. wasp_layout_by_name() returns `const void *` (really `const
 * Layout *`) so this header doesn't need the Layout type at all. */
ActionFn wasp_lookup_action(const char *name);
const void *wasp_layout_by_name(const char *name);

#endif
