# wasp — working notes

Running list of things we want wasp to be able to do, and which dwl-patches
(from https://codeberg.org/dwl/dwl-patches) are reference material for each —
not drop-in patches, since almost everything here needs adapting to fit
wasp's own model: config driven live from `config.lua`, not `config.h` +
recompile. Add to this as things come to mind; nothing here needs to be
decided all at once.

## Core (not patch-derived)
- Lua config, `~/.config/wasp/config.lua`, live-reloadable — see the design
  discussion in project memory (embed `lua_State*`, parse into config
  structs, reload on file-watch or a bound key). Load-once-at-startup is
  done; the actual reload trigger (file-watch or bound key re-running
  `waspconfig_load()`) is still open.
- Built-in toggleable status bar (`wasp.bar = { enable, height }` or
  similar). **Decided (2026-08-12)**: base it on dwl-patches' `bar` +
  `barconfig` rather than porting spitfire's bitmap-font approach — it's
  mature, pre-implemented, real text rendering (fcft/pixman/tllist), and a
  more solid starting point than reinventing font rendering from scratch.
  Done: `barlayout`/colors/enable-toggle are `config.lua`-driven.
- **Keybindings (done, 2026-08-12)**: `wasp.keys` in `config.lua` builds
  dwl.c's `Key keys[]` at runtime (`luaconfig.c`'s `load_keys()`), instead
  of the old compile-time `config.h` array. `wasp.modkey` sets what "mod"
  means; `wasp.terminal`/`wasp.menu` make the launched terminal/menu
  agnostic (this laptop uses `alacritty`, but it's just Lua data now, not
  hardcoded like upstream dwl's `foot`). `luaconfig.c`'s `set_defaults()`
  builds a small emergency-fallback keymap (terminal, menu, window/
  workspace nav, kill, quit, VT switch) so a missing/broken `config.lua`
  never locks you out; the full suggested keymap (layouts, monitor nav,
  resize, `moveresizekb`, ...) lives in `examples/config.lua` as data —
  see that file's action-reference comment for the full list of actions
  and fields. `dwl.c`'s `buttons[]` (mouse bindings) is still the old
  static `config.h` array, not Lua-driven yet — worth revisiting.

## Reference patches to adapt (not apply as-is)
- **hot-reload** — reference for what *not* to copy: its `dlopen`'d `.so` +
  `HOT`/cold-part macro split is solving "don't recompile" via a mechanism
  we won't need once config is Lua-driven and grabs a wide keybind superset.
  Skip the patch itself; the goal is already covered by the Lua design.
- **bar** / **barconfig** / **barborder** / **bar-notitle** / etc. — bar
  family, see "Decided" above.
- **gaps** / **vanitygaps** / **genericgaps** — inner/outer gaps, adjustable
  live from `config.lua`. **Done (2026-08-12)**: adapted (not applied
  as-is) from the `gaps` patch — `gappedarea()`/`insetgap()` helpers in
  `dwl.c`, consumed by `tile()`/`monocle()`/`dwindle()`. Unlike the
  reference patch's single `gappx`, wasp splits it into `wasp.gaps = {
  inner, outer, smart }` (spitfire-style: `inner` between windows, `outer`
  against the monitor edge, `smart` drops `outer` for a lone window) —
  see `examples/config.lua`. No live increment/toggle keybind yet (the
  reference patch has `togglegaps`) — just static `config.lua` values for
  now, worth adding later if wanted.
- **dragresize** / **better-resize** — mouse-drag resize behavior.
  **moveresizekb is done** (2026-08-12): adapted into `dwl.c`, bound via
  `wasp.keys`' `moveresizekb` action (dx/dy/dw/dh, see
  `examples/config.lua`) — floating-window keyboard move/resize.
- **dwindle** — fibonacci/spiral tiling layout. **Done (2026-08-12)**:
  adapted into `dwl.c` as `dwindle()`, reachable via `setlayout`'s
  `layout = "dwindle"` (or `"fibonacci"`) in `config.lua`.
- **borders** / **smartborders** / **simpleborders** — window border
  rendering/behavior (colors should come from `config.lua`, same as
  spitfire's `spitfire.border`).
- **autostart** — but embedded as a `wasp.autostart({...})`-style call in
  `config.lua` (like spitfire), not a static C array in `config.h`.

## Keyboard
- **Layout switching + repeat speed — done (2026-08-12)**: `wasp.keyboard =
  { layout, variant, model, options, rules, repeat_rate, repeat_delay }` in
  `config.lua`, same table shape as spitfire's `spitfire.keyboard`.
  `luaconfig.c`'s `load_keyboard()` fills dwl.c's `xkb_rules` (feeds
  `xkb_keymap_new_from_names()` directly) and `repeat_rate`/`repeat_delay`
  globals. Applied once at startup, same as everything else pending the
  general reload story above.

## Licensing
- **Done (2026-08-12)**: split license, since wasp now has original code of
  its own, not just modified dwl files. `LICENSE` (GPLv3, dwl's own) stays
  as-is and still covers everything derived from upstream dwl (`dwl.c`,
  `config.def.h`, `config.mk`, `Makefile`, `client.h`, `util.c`, `util.h`,
  `dwl.1`, `protocols/*`) plus the compiled `wasp` binary as a whole, since
  it links GPLv3 code — that's a requirement of GPLv3, not a choice.
  **`LICENSE.wasp` (new, MIT)** covers wasp's own from-scratch files with
  no upstream equivalent (`luaconfig.c`/`.h`, `README.md`, this file,
  `examples/config.lua`, `scripts/*`, `wasp.desktop`, `assets/*`) —
  Copyright Daniel Azevedo. See README.md's License section for the short
  version of why it's split this way rather than all-MIT or all-GPL.

## Session/greeter integration
- **Done (2026-08-12)**: build output renamed `dwl` → `wasp` (Makefile's
  `wasp:` target, `make install` installs `$(PREFIX)/bin/wasp`).
  `wasp.desktop` (was `dwl.desktop`) installs to
  `$(DATADIR)/wayland-sessions/wasp.desktop` so greetd (or any
  wayland-sessions-reading greeter) can list/select it. `dwl.1`'s man page
  content itself is still the unmodified upstream dwl one — not renamed,
  out of scope for now.

## Not yet decided / just flagged
- **Status text source — decided (2026-08-12), option (a)**: keep dwl's
  classic stdin-pipe model for the bar's right-side status text (`stext`)
  rather than porting `pwm`/`spitfire`'s native in-process widgets into C.
  **`scripts/statusbar.sh` added (2026-08-12)**: the earlier throwaway test
  script, now a real, persisted, repo-tracked file (it kept vanishing
  between sessions since it only ever lived in a Bash session, not on
  disk — that's why the bar kept coming back to showing its
  `dwl-*-dirty` startup placeholder each time). Run
  `scripts/statusbar.sh | wasp` to feed it. Widget set/order: CPU, RAM,
  Audio (volume), Battery, Time/Date — `top`+`free` for CPU/RAM (forced
  `LC_ALL=C`, `top`'s decimal point is locale-dependent and breaks the
  `awk` parsing otherwise), `pactl get-sink-volume` for volume, `date` for
  the clock. Battery path is detected, not hardcoded — scans
  `/sys/class/power_supply/` for the first `BAT*` entry (`BAT1` on this
  laptop, `BAT0` on others), same pattern as `pwm`'s
  `battery_file_search()` and `spitfire`'s `read_battery_status()`. Once
  `autostart` lands in `config.lua` (see below), this should be spawned
  from there instead of piped by hand.
