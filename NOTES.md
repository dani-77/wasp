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
2. ~~**Scratchpad.**~~ **Done (2026-08-13)** -- see "Core" below for the
   implementation, and [[wasp-project]] memory for the summary. Only the
   named/spawn-on-first-use flavor, matching spitfire's
   `spitfire.scratchpad.toggle(name, spawncmd, app_id, w_frac, h_frac)`
   (see `~/Projectos/spitfire/examples/config.lua`) -- spitfire's *other*
   flavor, a single anonymous slot
   (`spitfire.window.toggle_scratchpad()`), wasn't ported; not clearly
   more useful than just adding a second named slot, revisit only if it
   turns out to actually be missed.
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
5. ~~**Expose wasp's workspaces to external shells (Utumno,
   quickshell-d77, helium-d77, fabric-d77).**~~ **Done (2026-08-13)** --
   `ext-workspace-v1`, see "Core" below for the implementation.
   **Prerequisite that turned out to be needed first: bumped wlroots
   0.19 -> 0.20.** Confirmed via `/usr/include/wlroots-0.19` that the
   `wlr_ext_workspace_v1` helper simply doesn't exist there -- it's
   0.20+ only. Low-risk in practice: upstream dwl's own `main` (the
   mirror kept in this repo) already made this exact jump in commit
   `2c9cb2a`, an 11-line diff (`config.mk`'s two `wlroots-0.19` ->
   `wlroots-0.20`, plus an XWayland cursor-buffer API change) --
   `wlroots0.20-devel` was already packaged in Void's repos too, so
   nothing exotic to install. One further wrinkle past that commit:
   this machine's actual wlroots (0.20.2) had *already* simplified
   `wlr_xwayland_set_cursor()` further than upstream's commit assumed
   (dropped the stride/width/height args entirely, not just the
   buffer-getter change) -- fixed against the real installed header,
   not the older commit's exact snippet. Confirmed via `nm`/`.pc` that
   the packaged wlroots0.20 was itself built `have_xwayland=true`.
   **Also enabled XWayland while at it** (Daniel's call, unprompted by
   this item specifically -- "caso contrário não há feh nem steam para
   ninguém") -- `config.mk`'s `XWAYLAND`/`XLIBS` were commented out
   (dwl's own upstream default); libxcb-devel/xcb-util-wm-devel were
   already installed on this machine so it was just uncommenting.
6. ~~**Window rules, `wasp.rules` in config.lua.**~~ **Done (2026-08-13)**
   -- see "Core" below for the implementation. Research trail kept: the
   `center` field's design came directly from reading **mwc**
   (`dqrk0jeste/mwc`, `src/toplevel.c`'s `toplevel_handle_map()`) and
   **MangoWC** (`mangowm/mango`, `src/fetch/client.h`'s
   `setclient_coordinate_center()`) -- both do the exact one-line formula
   wasp already had in `scratchpadgeom()` (`x = usable.x + (usable.width -
   client.width) / 2`), just with different defaults: mwc centers *every*
   floating toplevel unconditionally, MangoWC centers by default with a
   per-rule opt-out (`no_force_center`) plus `offsetx`/`offsety`/
   `width`/`height` rule fields wasp didn't need for its own actual want
   here and didn't port. **Not done**: dwl-patches'
   `alwayscenter`/`centeredmaster`/`center-terminal`/`movecenter` and
   Hyprland's own rule model were flagged as reference material earlier
   but weren't actually consulted in the end -- mwc/MangoWC's C was closer
   to hand and sufficient on their own.
7. **Rounded border corners, and blur too.** Reference for the *feel*:
   Daniel's own **spitfire** -- `spitfire.border = { width, active,
   inactive, radius }` (see its `examples/config.lua`), border drawn *on
   top of* the window, radius masks the window's own square corners along
   with rounding the border itself. wasp's border today is 4 separate flat
   `wlr_scene_rect` rects per client (`client_set_border_color()` /
   `resize()` in `dwl.c`) -- plain rects can't be rounded (or blurred) on
   their own, needs an actual rendering change, not just a config knob.
   **Real path forward found (2026-08-13): [SceneFX]
   (https://github.com/wlrfx/scenefx)** -- "a drop-in replacement for the
   wlroots scene API" adding rounded corners (separate inner/outer
   radius), drop shadows, opacity, and background blur, while keeping the
   scene-graph model wasp/dwl already builds on. Not original work from
   scratch after all. Three things to check out together, all wlroots+
   scenefx-based real compositors:
   - **[mwc](https://github.com/dqrk0jeste/mwc)** -- tiling compositor,
     wlroots 0.18 + scenefx 0.2. Probably the closest in spirit to
     dwl/wasp of the three.
   - **[maomaowm](https://github.com/Gugu7264/maomaowm)** -- "dwl but no
     suckless", wlroots+scenefx.
   - **MangoWC** (already flagged above for animations) -- worth checking
     whether it *also* uses scenefx for its own eye-candy, given it's the
     confirmed dwl-fork precedent already.
   - **dwl itself has a real, if unmaintained, integration to start
     from**: `stale-patches/scenefx/` in the official dwl-patches repo
     (`https://codeberg.org/dwl/dwl-patches/raw/branch/main/stale-patches/scenefx/scenefx.patch`,
     moved out of the active `patches/` directory, last touched
     2025-12-18). Its own README (worth reading in full before starting)
     flags real caveats: blur doesn't work together with opacity on the
     same window; `scenefx-0.2` must come *before* `wlroots-0.18` in the
     Makefile's dependency order; Xwayland clients don't get rounded
     borders/shadows (shadows might still work independently); several
     patch variants exist for different scenefx commits, some missing
     blur support (only the "0.8-dev" variant has rounded borders + blur
     + shadows all together) -- read carefully and pick the matching
     scenefx version, don't just grab the newest-looking one blind.

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
- **Named scratchpads (done, 2026-08-13)**: `wasp.scratchpad = { { name=,
  cmd=, app_id=, w=, h= }, ... }` in `config.lua`, toggled via the
  `toggle-scratchpad` action (arg: `name`). Design, deliberately not a
  straight port of dwl-patches' `namedscratchpads`/`simple_scratchpad`
  (which hide via `wlr_scene_node_set_enabled` directly): reuses dwl's
  existing `VISIBLEON()`/`arrange()` tag-visibility machinery instead —
  hiding a scratchpad just sets its `tags` to `SPTAG` (one dedicated bit
  above `TAGMASK`, shared by every named slot, never part of any
  monitor's normal tagset), so the usual per-client visibility walk in
  `arrange()` hides it for free, same code path as any other tag switch.
  Showing it sets `tags` back to the current monitor's active tagset,
  floating, centered/sized per `w`/`h` (fraction of the monitor's usable
  area). `togglescratchpad()`'s hide/show decision is
  `VISIBLEON(client, selmon)` (not just "does it have SPTAG"), so
  toggling a scratchpad that's floating on a different tag/monitor always
  brings it to the current one rather than hiding it further — matches
  drop-down-terminal expectations. Which live `Client` (if any) belongs
  to a slot is tracked via a `scratchpad` field (the slot's name) added
  to `Client` itself, not inside the `Scratchpad` config array — that
  array is rebuilt from scratch on every `waspconfig_load()` call (same
  as `keys`/`autostart`), so anything caching state *inside* it wouldn't
  survive a hot-reload; the `Client` field does, so a reload never
  orphans an already-spawned scratchpad. Known gap, same shape as the
  `autostart` one already documented above: if the spawned process exits
  before ever mapping a surface, dwl.c's "pending spawn" bookkeeping
  (`pending_scratchpad_name`/`pending_scratchpad_appid`, cleared once
  claimed) is left dangling until the *next* toggle of any slot
  overwrites it — harmless in practice (worst case, a later differently
  named slot's freshly-spawned client gets mis-claimed if `app_id`s
  happen to collide), not chased further in this pass. Only the named/
  spawn-on-first-use flavor exists; no anonymous single-slot scratchpad
  (see "Up next" above for why that was skipped for now).
- **Window rules (done, 2026-08-13)**: `wasp.rules = { { app_id=, title=,
  tags=, floating=, monitor=, center= }, ... }` in `config.lua` -- moved
  the old `Rule`/`static const Rule rules[]` (upstream dwl's own
  compile-time `config.def.h` array, matched in `applyrules()`) the same
  way `keys` moved: `Rule` itself now lives in `luaconfig.h` (shared
  layout, same reasoning as `Arg`/`Key`), `rules`/`nrules` are runtime
  globals `luaconfig.c`'s `load_rules()` rebuilds from `wasp.rules` on
  every `waspconfig_load()` call, `config.def.h` only keeps an explanatory
  comment where the static array used to be (same treatment `keys[]`
  already got). Empty rule set is a safe, supported default -- unlike
  `keys`, nothing about a missing/empty `wasp.rules` can lock you out, so
  there's no emergency-fallback table to maintain here. `tags` takes a
  1..9 workspace number (matching `wasp.keys`' own `tag` field
  convention) rather than a raw bitmask, converted internally; a client
  matching more than one rule gets every match's `tags` OR'd together but
  only the *last* match's `floating`/`monitor`/`center` (same last-match-
  wins semantics upstream dwl's `applyrules()` already had for those
  before this change). New vs. upstream dwl: the `center` field --
  `applyrules()` calls a new shared `centeredgeom()` helper (`dwl.c`,
  right before `applyrules()`) after `setmon()`, at the client's own
  requested size (`c->geom`, already populated by `mapnotify()` by the
  time `applyrules()` runs) -- see the "Researched" note under item 6
  above for where that formula came from. `scratchpadgeom()` (named
  scratchpads) now calls `centeredgeom()` too instead of duplicating the
  math, so there's exactly one centering implementation, not two.

  **Debugged (2026-08-13): d77run's `center` rule "stopped working" --
  not a wasp bug.** After the wlroots 0.20/XWayland changes, Daniel's
  `d77run` rule (see live `config.lua`) stopped centering. Added
  temporary `fprintf()`s to `applyrules()` (removed again once diagnosed
  -- not left in the tree) and watched `client_get_appid(c)`'s actual
  return value: it's **not stable across runs**. Sometimes `"d77run"`
  (prgname), sometimes `"dev.d77.gmrun-rejuvenated"` (its internal
  GApplication id) -- same binary, same machine, no wasp/wlroots change
  in between two back-to-back test runs. Best working theory: GApplication
  tries to register itself as a D-Bus singleton on startup, and falls
  back to reporting its raw app id instead of prgname when that
  registration doesn't succeed (e.g. `DBUS_SESSION_BUS_ADDRESS` unset in
  that particular run) -- not confirmed by reading d77run's own source,
  just consistent with every run observed. Not something wasp can or
  should paper over -- `applyrules()`'s matching logic was never wrong,
  the *input* it was matching against just genuinely changes. Fixed
  pragmatically in `config.lua`, not in `dwl.c`: list the same app twice
  in `wasp.rules`, once per app_id it's been seen reporting, identical
  fields both times (dwl's rule matching already tolerates multiple
  matches fine). `examples/config.lua` got a general-purpose comment
  about this pattern (check with `WAYLAND_DEBUG=1 <app> 2>&1 | grep
  set_app_id` if a rule that used to work stops matching, don't assume
  wasp regressed first) rather than repeating d77run's specific two
  values, since they're meaningless outside this machine.
- **Workspaces over `ext-workspace-v1` (done, 2026-08-13)**: needs
  wlroots 0.20's `wlr_ext_workspace_v1` helper -- see item 5 above for
  the version-bump story. One `wlr_ext_workspace_group_handle_v1` per
  `Monitor` (tags are genuinely per-monitor in dwl, not global -- a
  client's `tags` bitmask only means anything relative to `c->mon`'s own
  `tagset`, see `VISIBLEON()`), `LENGTH(tags)` (9) workspace handles
  inside each group, all created in `createmon()` right before its
  existing `drawbars()` call (so the first `updateextworkspaces()` --
  see below -- sets correct initial active/urgent state for free) and
  torn down in `cleanupmon()`. Deliberately does *not* advertise the
  `CREATE_WORKSPACE`/`ASSIGN`/`REMOVE` capabilities -- wasp's 9 tags are
  fixed, dwl has no notion of dynamic workspaces, so only
  `ACTIVATE`/`DEACTIVATE` are offered per workspace and `commit`-event
  requests of the other types are ignored if a client sends one anyway.
  Design reference: **MangoWC**'s own `src/ext-protocol/ext-workspace.h`
  (cloned locally to read) -- confirmed its actual `create()` call shape
  no longer matches this installed wlroots version's real header (their
  code takes `name` as the 2nd arg + no separate `set_name()`; this
  wlroots takes `id` as the 2nd arg and `set_name()` is its own call) --
  written against *this machine's* actual
  `/usr/include/wlroots-0.20/wlr/types/wlr_ext_workspace_v1.h`, not
  copied from theirs, but the overall shape (one group per output, batch-
  processed `commit` event with a request list, not one callback per
  request type) carried over. Two update paths:
  - **State out** (dwl -> client): `updateextworkspaces(Monitor *m)`,
    called from `drawbars()` (now `updateextworkspaces(m)` +
    `drawbar(m)` per monitor, not just `drawbar(m)`) -- deliberately
    *not* piggybacked on `drawbar()`'s own existing `occ`/`urg`
    computation in its `t` (tags) bar-layout case, because that whole
    function returns immediately if the bar's scene buffer is disabled
    (`wasp.bar.enable = false`, which is this machine's own live
    config) and would've silently stopped updating workspace state the
    moment the bar was turned off. `drawbars()` already ran at every
    "something about tags/clients changed" point in the file (tag
    switches, urgency via `urgent()`, reload, ...), so hooking it there
    means every one of those triggers this for free, bar or no bar.
  - **Requests in** (client -> dwl): `extworkspacecommit()`, listening
    on `ext_workspace_mgr->events.commit`. `ACTIVATE`/`DEACTIVATE`
    reuse `view()`/`toggleview()` verbatim rather than duplicating their
    logic, via the same `selmon = <target>;` -then-call-the-ordinary-
    selmon-relative-action trick `buttonpress()` already uses for a
    click landing on a non-focused monitor's own bar (see its
    `selmon = xytomon(...)` before dispatching a `Button`).
  Which `(Monitor*, tag index)` a `wlr_ext_workspace_handle_v1*` means
  is recovered via a small heap-allocated `ExtWorkspaceTag` stashed in
  the handle's own `data` field at creation (freed alongside the handle
  in `cleanupmon()`) -- the reverse direction of `m->ext_workspaces[i]`.
  **Verified end-to-end**, not just by inspection: wrote a throwaway
  standalone Wayland client (raw `wayland-scanner`-generated bindings,
  no wlroots) that binds `ext_workspace_manager_v1` against a nested
  test instance and dumps every group/workspace/state event it
  receives -- got exactly 1 group (`caps=0`), 9 workspaces named "1"..
  "9" with `caps=3` (ACTIVATE|DEACTIVATE) each, and workspace "1"
  correctly `state=1` (active) while the rest read `state=0`, matching
  a freshly created monitor's default `tagset`.

  **Found for real against Utumno (2026-08-13), not just theorized:**
  `wlr_ext_workspace_handle_v1_set_coordinates()` also needs calling --
  one `uint32_t` per workspace, its 0-based tag index -- or every
  workspace's `coordinates` comes through empty. This isn't cosmetic:
  Utumno's own generic-protocol bar widget
  (`~/Projectos/utumno/modules/Workspaces.qml`) *already* sorts its
  workspace list by `coordinates` before rendering (correct, defensive
  code on their side, comparing element-by-element, falling back to `0`
  for missing entries) -- but comparing two empty arrays makes that
  comparator a no-op every time, so the sort silently did nothing and
  the bar showed whatever order Quickshell's own internal list happened
  to be in (observed live: `9,6,8,7,5,4,1,3,2`), not tag order. Confirmed
  the fix with the same standalone test client, extended to also decode
  the `coordinates` event: `coords=[0,] name=1` .. `coords=[8,] name=9`,
  in order, after adding the `set_coordinates()` call right after
  `set_name()` in `createmon()`'s workspace-creation loop. No change
  needed in Utumno at all in the end -- the bug was entirely on wasp's
  side not providing what Utumno's own code already correctly expected.

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
