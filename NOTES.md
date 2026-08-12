# wasp — working notes

Running list of things we want wasp to be able to do, and which dwl-patches
(from https://codeberg.org/dwl/dwl-patches) are reference material for each —
not drop-in patches, since almost everything here needs adapting to fit
wasp's own model: config driven live from `config.lua`, not `config.h` +
recompile. Add to this as things come to mind; nothing here needs to be
decided all at once.

## Up next (2026-08-12, Daniel's stated order)
Three things, in this order — not started yet, logging the order/scoping
notes before picking any of them up:

1. **Keyboard-only window resize/move.** **Scoped down (2026-08-12)**:
   not a sway/i3 modal resize-mode after all — Daniel's call was to keep
   the *floating*-window part to "the usual dwl/dwm scheme" (keyboard
   increments/nudges), which is already exactly what `moveresizekb` does
   (done, see below — arrow keys move, shift+arrows resize, fixed pixel
   deltas per press). For *tiled* windows, the dwm/dwl-usual story is
   already there too: `setmfact` (master/stack boundary, `mod+h`/`mod+l`)
   and `incnmaster` (window count in master, `mod+i`/`mod+d`) — no
   per-stack-client ratio exists in dwl's simpler fixed master-stack
   model (unlike sway/i3's binary-tree layout), and that's fine, not the
   goal here. No modal/transient keybinding-state mechanism needed. So:
   this item may already be *done* in substance — check with Daniel
   whether anything concrete is still missing before starting more work
   here, rather than assuming a gap.
2. **Scratchpad.** Reference: spitfire already has this working twice --
   a single anonymous slot (`spitfire.window.toggle_scratchpad()`) and a
   named/spawn-on-first-use one keyed by `app_id`
   (`spitfire.scratchpad.toggle(name, spawncmd, app_id, w_frac, h_frac)`),
   see `~/Projectos/spitfire/examples/config.lua`. dwl-patches has
   `namedscratchpads` and `simple_scratchpad` as reference material for
   the C side (window hide/show via `wlr_scene_node_set_enabled` +
   tag/floating juggling, not full unmap/remap).
3. **Animations, MangoWC-style.** MangoWC (`mangowm/mango`) is the
   confirmed real dwl fork precedent for this (see project memory) --
   need to actually read its source for how it drives open/close/move/
   tag-switch tweening on top of `wlr_scene` before designing wasp's own;
   haven't done that read yet. Biggest lift of the three -- needs a
   frame-timer-driven interpolation system touching `resize()`/`arrange()`/
   `mapnotify()`/`unmapnotify()`, none of which exist yet.

Two more added 2026-08-12 (not yet ordered relative to the three above):

4. **Output scale**, matching spitfire/niri/MangoWC. dwl already has the
   raw plumbing -- `MonitorRule.scale` (per-monitor, in `monrules[]`) and
   `Monitor.b.scale`/`wlr_output_set_scale()` already exist -- it's just
   entirely static `config.def.h` today, not `config.lua`-driven, and
   there's no live rescale keybind (spitfire's `Mod+Shift+P`/`M`). Spitfire's
   `spitfire.output = { scale = 1.0 }` (see its `examples/config.lua`) is
   the reference shape -- a starting value applied at startup, re-applied
   live on reload, with dedicated inc/dec keybinds on top. Probably wants
   its own `wasp.monitors`/`wasp.output`-style config section eventually
   (name-matched per-output rules, mirroring dwl's own `monrules[]`
   fields: mfact, nmaster, scale, layout, rotate/reflect, x/y) rather than
   a single global scale -- scope that properly when picked up, don't
   just bolt on a lone global number.
5. **Expose wasp's workspaces to external shells (Utumno,
   quickshell-d77, helium-d77, fabric-d77).** All four are Daniel's own
   Quickshell/Fabric/Rust desktop-shell projects (`~/Projectos/{utumno,
   quickshell-d77,helium-d77,fabric-d77}`) -- bars/launchers/etc. that
   currently target niri/Hyprland/sway, none of them wasp/dwl yet. Without
   something exposing workspace state, their bars just won't show any
   workspace list at all when run against wasp. dwl's own model is tags
   (dwm-style bitmask, several active at once, per-window multi-tag) which
   doesn't map 1:1 onto niri-style single-active workspaces -- needs actual
   design thought, not just a mechanical port. Best bet is almost
   certainly the standardized **`ext-workspace-v1`** Wayland protocol
   (what niri implements, and what spitfire's own bar comment already
   flagged: "Advertised over ext-workspace-v1, so any bar that knows that
   protocol sees them too") rather than bespoke per-shell integration --
   one protocol implementation in wasp should cover all four clients (and
   any other ext-workspace-v1-aware bar) at once, versus Hyprland's
   approach (its own custom IPC socket, not a standard protocol, not
   worth mimicking). dwl has no `ext-workspace-v1` support today at all;
   this is new server-side protocol work, not a patch to adapt.
6. **Window rules, `wasp.rules` in config.lua.** e.g. Firefox always opens
   on workspace 9, `d77run` opens floating and centered. Small, well-scoped
   compared to the rest of this list -- dwl already has exactly this as a
   static C array, just not Lua-driven yet:
   ```c
   typedef struct {
       const char *id;    /* app_id */
       const char *title;
       uint32_t tags;
       int isfloating;
       int monitor;
   } Rule;
   static const Rule rules[] = { ... };  /* config.def.h, applied in applyrules() */
   ```
   Same pattern as everything else this session: make `rules`/`nrules`
   extern globals (`luaconfig.h`), `load_rules()` in `luaconfig.c` builds
   them from a `wasp.rules = { { app_id=, title=, tags=, floating=,
   monitor= }, ... }` array, `dwl.c`'s `applyrules()` switches from the
   static array to the runtime one. `config.def.h` currently requires at
   least one rule to exist (a hardcoded comment says so) -- check whether
   that constraint still needs to hold once rules are optional/Lua-driven,
   probably doesn't. No centering support in the existing `Rule` struct
   (`isfloating` only, not position) -- `d77run` floating *and centered*
   needs a new field or a separate `alwayscenter`-style behavior; dwl-patches
   has `alwayscenter`/`centeredmaster`/`center-terminal`/`movecenter` as
   reference material for that part specifically.

## Core (not patch-derived)
- **Lua config, live-reloadable — done (2026-08-12), bound-key trigger**:
  `dwl.c`'s `reload()` action (default bind: `mod+shift+r`, see
  `examples/config.lua`) calls `waspconfig_load()` again and re-applies
  whatever of it can take effect without a restart, live: **gaps** (free —
  `arrange()` already reads `gapsinner`/`gapsouter`/`gapsmart` fresh every
  call, no extra plumbing needed), the **bar**'s visibility/position/
  colors (`updatebar()` + `arrangelayers()` + `drawbars()`), every
  existing client's **border color** (walks `clients`, re-applies
  `colors[Scheme*][ColBorder]` per client's current
  focused/urgent/normal state via `client_set_border_color()`), the root
  **background**, and **keyboard** repeat speed + xkb layout (rebuilds
  and reassigns the keymap on the live `kb_group`, same calls
  `createkeyboardgroup()` makes at startup). **Keybindings themselves**
  are live for free — `keybinding()`'s dispatch loop already walks the
  current `keys[]`/`nkeys` globals on every keypress, no cache to
  invalidate. Not yet live: border *width* on already-mapped clients
  (would need a per-client `resize()`/geometry recompute, not just a
  color swap) and `wasp.autostart` (deliberately -- see below). File-watch
  based reload (as opposed to a bound key) was the other option
  considered; not implemented, bound key covers the actual want (know
  when you've just edited the file) more directly and more simply.
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
- **autostart** — **done (2026-08-12)**: `wasp.autostart = { {argv...},
  ... }` in `config.lua` (a plain array of argv arrays, wasp's own
  existing convention — same shape as `wasp.terminal`/`wasp.menu` one
  level up — rather than spitfire's `wasp.autostart({...})` function-call
  syntax or the dwl-patches reference's flat NULL-separated C array).
  `luaconfig.c`'s `load_autostart()` only *parses* it into the
  `autostart`/`nautostart` globals; `dwl.c`'s `autostartexec()` (adapted
  from dwl-patches' `autostart` patch: fork+execvp each one directly, no
  shell, `setsid()` so each becomes its own process group) is what
  actually spawns them, called once from `run()` right after the backend
  starts. Deliberately **not** part of `waspconfig_load()`/`reload()`
  itself — if it were, every hot-reload would respawn everything in the
  list all over again. `cleanup()` kills each one's whole process group
  (`kill(-pid, ...)`, not just the direct child -- verified live: a
  `{"sh","-c","sleep 100"}` entry's grandchild `sleep` died too, a plain
  positive-pid kill would've orphaned it) and reaps it; `handlesig()`'s
  `SIGCHLD` handler nulls out any `autostart_pids[]` entry that already
  exited on its own first, so `cleanup()` never risks signaling a pid the
  kernel has since handed to an unrelated process.

## Keyboard
- **Layout switching + repeat speed — done (2026-08-12)**: `wasp.keyboard =
  { layout, variant, model, options, rules, repeat_rate, repeat_delay }` in
  `config.lua`, same table shape as spitfire's `spitfire.keyboard`.
  `luaconfig.c`'s `load_keyboard()` fills dwl.c's `xkb_rules` (feeds
  `xkb_keymap_new_from_names()` directly) and `repeat_rate`/`repeat_delay`
  globals. Applied once at startup, same as everything else pending the
  general reload story above.
- **Resolved, 2026-08-12 — was stale post-reload state, not a real bug**:
  AltGr+7 was switching to workspace 7 instead of typing `{` on Daniel's
  PT layout. Root cause turned out to be that a `reload()` (bound key,
  not a full restart) hadn't fully applied an earlier `layout = "us"` →
  `"pt"` change -- a full session logout/login fixed it immediately,
  confirming `wasp.keyboard.layout = "pt"` itself was already correct,
  not an actual Alt-vs-AltGr keysym/modifier collision as first
  suspected. So: **`reload()`'s keyboard keymap swap has a known
  limitation** -- rebuilding and reassigning the keymap on the live
  `kb_group` (see `reload()` in `dwl.c`) isn't always enough to fully
  pick up a *layout* change; a real restart is more reliable for that
  specific case. Checked wlroots' public `wlr_keyboard_group` API for an
  obvious fix (e.g. resetting each individual member keyboard, not just
  the group's own virtual one) -- nothing exposed publicly beyond what
  `reload()` already does (`struct wlr_keyboard_group`'s member-device
  list is private/internal, not reachable from `dwl.c`), so not chasing
  this further without a clearer lead. In practice: gaps/bar/colors/
  repeat-speed reload live reliably; a keyboard *layout* change might
  need a restart to fully take -- worth keeping in mind, not urgent to
  fix blind.

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
  `battery_file_search()` and `spitfire`'s `read_battery_status()`.
  **Correction (2026-08-12)**: the earlier "spawn it from `wasp.autostart`"
  plan above doesn't actually work mechanically -- stdin is fixed at the
  moment a process is launched, so nothing wasp does *after* it's already
  running (autostart included, whenever that lands) can retroactively
  rewire its own stdin to a spawned child's stdout. The pipe has to exist
  before/at exec time. Fixed properly instead: **`scripts/wasp-session`**,
  a wrapper (`wasp-statusbar | exec wasp`) that `wasp.desktop`'s `Exec=`
  now points at instead of `wasp` directly, installed alongside it by
  `make install`. So through a greeter it's automatic; for local/dev
  testing without installing, `scripts/statusbar.sh | ./wasp` by hand is
  still the way (README's Configuring section has both).
  **Bug fixed (2026-08-12)**: quitting wasp (`mod+shift+q`) left
  `wasp-statusbar` (the pipe's write side) running -- it's a separate
  process the shell set up, wasp has no handle on it -- so it wouldn't
  notice for up to its 5s sleep, then died noisily (SIGPIPE/EPIPE on its
  next `printf`, surfaced as `printf: printf: I/O error` plus, apparently,
  a Rust-flavored `Io error: broken pipe (os error 32)` / `Error:
  ExitFailure(1)` from whatever supervises the session -- looks like
  greetd's own reporting of the session command's messy exit). Fixed:
  `scripts/statusbar.sh` now traps `TERM`/`INT`/`PIPE` and backgrounds its
  `sleep` (`sleep 5 & wait $!`, so a trapped signal interrupts the wait
  immediately instead of only being noticed once the sleep completes) --
  verified live, a `SIGTERM` now kills it instantly (`time kill -TERM
  $pid` => 0,000s) instead of the printf-failure path. If a session
  teardown signal reaches it (the realistic path -- it shares the
  `wasp-session` script's process group), exit is immediate; a pipe that
  merely goes silent with no signal (e.g. testing `wasp` standalone by
  hand) is still bounded by the 5s sleep, but now exits clean and quiet
  instead of erroring. Didn't chase tightening that remaining ~5s tail
  further (would mean polling in ~1s slices) -- not worth the extra
  wakeups for a cosmetic tail latency on an already-declining
  responsiveness ask (see the volume-instant-refresh discussion above).
