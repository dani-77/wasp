# wasp — working notes

Running list of things we want wasp to be able to do, and which dwl-patches
(from https://codeberg.org/dwl/dwl-patches) are reference material for each —
not drop-in patches, since almost everything here needs adapting to fit
wasp's own model: config driven live from `config.lua`, not `config.h` +
recompile. Add to this as things come to mind; nothing here needs to be
decided all at once.

## Up next (2026-08-12, Daniel's stated order)
Three things, in this order (2026-08-14: #2 and #3 done, #1 likely already
covered in substance -- see its own note -- so this list is effectively
closed out, modulo confirming #1 with Daniel):

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
3. ~~**Animations, MangoWC-style.**~~ **Done (2026-08-14)** -- MangoWC's
   own source (`~/d77void-pkgs/hostdir/sources/mangowc-0.15.4`,
   `src/animation/{client,common,tag}.h`) was actually read this time, as
   flagged below as still-needed -- design mirrors its mechanism closely
   (per-`wlr_output` `frame`-listener tick, no separate timer; cubic-
   bezier easing baked to a 256-point lookup table since the curve isn't
   solvable for y(x) directly; a detached scene-graph snapshot for CLOSE,
   since the real `Client` is freed synchronously on unmap and nothing
   else survives to animate), but re-scoped down for a first version --
   deliberately NOT ported: layer-shell animation (wasp's bar isn't
   `Client`-based, no ask for it), per-`wasp.rules`-entry overrides
   (`isnoanimation`/per-rule open-close type -- one global on/off +
   global durations/curves for now), and `slide`-from-edge for OPEN
   (ships `fade`/`zoom` only; TAG already has its own edge-slide, `slide`
   for OPEN would reuse the same helper later if wanted).

   New `Client.anim` (`struct wasp_animation`) plus a separate
   `ClosingClient`/`closing_clients` list for CLOSE. Touches `resize()`
   (split into the real bounds/configure head + a new `start_animation()`
   that arms/updates `c->anim` instead of writing the scene graph
   directly), `arrange()` (its old binary
   `wlr_scene_node_set_enabled()` tag-visibility toggle now calls
   `tagin()`/`tagout()` when animations are on), `mapnotify()` (one-shot
   `pending_action = AnimOpen` flag), `unmapnotify()` (snapshots via
   `init_closing_client()` *before* `setmon(c, NULL, 0)` wipes `c->mon` --
   an early draft got this ordering wrong and would have silently never
   animated any close), and `rendermon()` (ticks every animating
   client/closing snapshot before each commit, re-arms via
   `wlr_output_schedule_frame()` only while something's actually
   animating). `wasp.animations` config (`enable`, per-action
   `duration_*`, `type_open`/`type_close`, `zoom_ratio`,
   `fade_from_opacity`, `tag_direction`, per-action `curve_*` cubic-bezier
   control points) follows the `wasp.gaps` load pattern exactly; two new
   `luaconfig.c` field-readers (`read_float_field`/`read_bezier_field`)
   were genuinely needed (fractional fields), durations stayed plain
   integer-ms via the existing `read_int_field`. `enable = false` by
   default reproduces the original instant behavior bit-for-bit.

   A real correctness wrinkle worth remembering: a brand-new client's
   very first `arrange()` pass looks, from `arrange()`'s own perspective,
   identical to a tag-hidden client becoming visible again (both start
   with their scene node disabled) -- without care, `tile()`/`dwindle()`/
   `monocle()`'s own relayout pass right after `mapnotify()` armed the
   OPEN tween would immediately reclassify it as a plain MOVE or TAG and
   silently kill the fade/zoom. Fixed by having `start_animation()`
   inherit the currently-running action (and resume position/opacity from
   wherever the tween currently is, not restart) whenever an *unlabeled*
   resize() call arrives mid-OPEN/TAG, and `tagin()` skips relabeling
   `pending_action` at all when a client is already mid-OPEN. MOVE/TAG
   tweens are position/clip-only, not a true buffer rescale (a deliberate
   v1 simplification, unlike OPEN/CLOSE's "zoom", which does rescale via
   `wlr_scene_buffer_set_dest_size` off the buffer's own immutable pixel
   size) -- only matters if a relayout also changes a tiled client's size
   mid-tween (e.g. `setmfact`), where it'll crop rather than scale.

   Verified: clean build (`-Wpedantic -Wall -Wextra
   -Wdeclaration-after-statement -Wshadow -Wfloat-conversion`, no new
   warnings).

   **Two real bugs found and fixed in the real running session (2026-08-14),
   not just by inspection** -- the "not yet verified: a live visual pass"
   line above turned out to matter:

   - **A window that redraws often (live output, this very terminal) got
     permanently stuck mid-fade/mid-move, not just briefly.** Root cause:
     `commitnotify()` (pre-existing upstream dwl code, unrelated to
     animations) calls `resize(c, c->geom, ...)` on *every* surface
     commit, not just real relayouts. Each one reached `start_animation()`
     and reset `time_started`, even when the target hadn't actually
     changed -- for a client committing faster than its own animation's
     duration (200ms), the tween's progress could never reach `p >= 1.0`,
     leaving it visibly stuck indefinitely. Fixed: `start_animation()` now
     only (re)arms when the target genuinely differs from where the
     client already is (idle) or is already animating toward (running);
     an *unlabeled* call (no explicit OPEN/TAG `pending_action`) with an
     unchanged target is now a no-op, letting any in-progress tween finish
     undisturbed.
   - **The same stuck-fade symptom, but *only* at a fractional output
     scale (confirmed live: broken at 1.25, clean at 1.00)** -- a second,
     independent bug, not a symptom of the first. `scale_buffer_iter()`'s
     "zoom" rescale computed `wlr_scene_buffer_set_dest_size()`'s target
     from `buffer->buffer->width/height` (the surface's *physical* pixel
     size -- `buffer_scale` times logical size on any HiDPI/fractional
     output) but `set_dest_size()` takes *logical* scene-graph units, same
     as everything else in this file. At scale 1.0 physical and logical
     coincide, so the bug was invisible; at 1.25 the buffer got rendered
     far too large for its clipped region, showing as garbled/overlapping
     text. Fixed: reads `wlr_surface_state.current.width/height` (via
     `wlr_scene_surface_try_from_buffer()`) instead -- wlroots' own
     already-scale-correct logical size -- falling back to
     `buffer->buffer->width/height` only for a non-surface-backed buffer
     (e.g. a single-pixel buffer), where the distinction doesn't apply.

   Both confirmed via a nested test session reproducing the exact
   conditions (a continuously-redrawing client, and separately `wasp.
   monitors`' `scale = 1.25`) after the live session itself first
   surfaced them -- screenshots before/after each fix, not just re-reading
   the code.

   **Two more real bugs, same day, same root pattern** -- an unlabeled
   (`commitnotify()`-driven) `resize()` call redirecting an in-progress
   animation that `start_animation()` didn't know how to leave alone:

   - **A window that was supposed to zoom in from a small box instead
     rendered corrupted/stretched** when a *third* window arrived while
     it was still mid-OPEN and got immediately re-tiled into a
     disproportionately different slot (e.g. going from a tall column to
     a short quarter-tile). Root cause: `scale_buffer_iter()` used one
     shared scale factor for both width and height, which is only correct
     when the tween's box keeps a constant aspect ratio -- true for a
     symmetric "zoom" OPEN/CLOSE targeting its own final box, false once
     that target itself changes shape mid-tween via the "resuming" path
     above. Fixed: separate `scale_x`/`scale_y` in `wasp_buf_ctx`, each
     axis computed against its own target dimension. Confirmed against
     MangoWC's own `BufferData` (`width_scale`/`height_scale` as separate
     fields, and also reading `surface->current.width/height` the same
     way) -- this wasn't a guess, its reference implementation does
     exactly this for exactly this reason.
   - **Windows visually stayed on screen across a tag switch they were
     never tagged onto -- not the client's actual tags changing, its
     on-screen presence following the user regardless** (reported live,
     2026-08-14, initially described as "windows follow you between
     workspaces," clarified to "their *visualization* follows," which is
     the precise symptom). Root cause: `tagout()` (arrange(), see its own
     comment) deliberately never touches `c->geom` -- it stays the real,
     restorable position on purpose, driving the off-screen slide via a
     separate `c->anim.target` instead. But `start_animation()` (called
     from `resize()`, including `commitnotify()`'s frequent unlabeled
     calls) unconditionally sets `c->anim.target = c->geom` whenever it
     (re)arms -- so a stray commit mid-tag-out redirected the animation
     *back onto the visible on-screen box*, aborting the slide-out
     entirely and leaving the window incorrectly visible (`node.enabled`
     never got a chance to be disabled, since the tween never reached its
     off-screen target to trigger that). Fixed: `start_animation()` now
     returns immediately, before touching anything, for any unlabeled
     call arriving while `c->anim.tag_hide_after` is set -- the tag-out
     tween is left completely undisturbed until it either finishes
     (hiding the node) or a real `tagin()` reverses it.

   Confirmed live (2026-08-14) after installing `wlrctl` (virtual-keyboard
   protocol, lets a script send real keypresses into a nested test
   session -- no more guessing from code review alone): tag switching
   itself was fixed by the above. But two more real bugs surfaced right
   after, same day, both specific to *floating* clients (a named
   scratchpad, `wasp.scratchpad`) -- confirmed live first (typed `htop`
   into a scratchpad that was supposedly hidden, watched it actually
   run), then reproduced and fixed in the same nested-plus-`wlrctl`
   harness before ever touching the real session again:

   - **A named scratchpad, once hidden, stayed fully visible *and
     interactive* forever** (not just visually stuck -- accepted
     keyboard input, ran `htop` in it while "hidden"). Root cause: the
     `tag_hide_after` guard added above was itself gated on the wrong
     signal. `resize()`'s own `interact` parameter is *also* set by
     `commitnotify()` for every floating client (`c->isfloating &&
     !c->isfullscreen`), for an unrelated reason (which bounding box to
     clamp against) -- gating the guard on `!interact` meant it silently
     never applied to *any* floating client, since every one of its
     commits looked "interactive." A tiled window's commits pass
     `interact=0`, which is why plain tag-switching (tiled windows) had
     already tested clean. Fixed: gate on `c == grabc` (the client
     actually mid-interactive-drag) instead -- `moveresizekb()` (a
     discrete keypress, no continuous grab of its own) now briefly
     stands in as `grabc` around its own `resize()` call so it keeps
     behaving the same (instant, no animation) as before.
   - **A scratchpad's own OPEN animation rendered as a tiny, wrongly-
     proportioned blob of content inside an already-full-size border**
     (worse at output scale 1.25 than 1.0, but present at both --
     confirmed by testing with `wasp.animations.enable = false`, which
     showed neither). Root cause, found via one more debug-logged nested
     run: for the brief window between `applyrules()`/`setmon()` setting
     a freshly-spawned scratchpad's tags and `mapnotify()`'s own
     scratchpad-claim block (further down) actually calling
     `setfloating(c, 1)`, the client's `isfloating` flag was still
     false -- so `arrange()`'s `tile()` pass caught it as an ordinary
     tiled client and resized it to fill the *entire monitor*, a third,
     wildly wrong target sandwiched between the client's own natural
     size and the real scratchpad box. Always instant/invisible without
     animations; with them, that wrong target corrupted the OPEN tween's
     start box via the "resuming" retarget path. Fixed: mark `isfloating
     = 1` as soon as the pending-scratchpad match is detected, *before*
     `applyrules()`/`setmon()` ever run, so `tile()` never gets the
     chance to claim it. A small residual (content very slightly
     overflowing the border for the first couple of frames, since the
     "resuming" path still reuses the OPEN tween's original start box
     rather than recomputing one for the corrected target) was left as-is
     -- clearly minor next to the fixed bug, not chased further this
     session.

Two more added 2026-08-12 (not yet ordered relative to the three above):

4. ~~**Output scale**, matching spitfire/niri/MangoWC.~~ **Done
   (2026-08-13)** -- see "Core" below for the implementation. Went with
   the full `wasp.monitors`/`MonitorRule` port (mfact, nmaster, scale,
   layout, transform, x, y) rather than a lone global scale number, per
   this item's own earlier scoping note -- barely more work than doing
   `scale` alone once `Rule`/`Scratchpad`/`Key` had already established
   the "move a config.def.h struct to luaconfig.h + a `load_*()` in
   luaconfig.c" pattern three times over. `mod+shift+p`/`m` matches
   spitfire's own live-rescale keybind exactly, muscle memory carries
   over between the two projects.
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
- **Per-output rules and live scale (done, 2026-08-13)**: `wasp.monitors
  = { { name=, mfact=, nmaster=, scale=, layout=, transform=, x=, y= },
  ... }` -- `MonitorRule`/`monrules[]` (upstream dwl's own compile-time
  `config.def.h` array, matched first-wins in `createmon()`) moved the
  same way `Rule`/`Scratchpad`/`Key` did: `MonitorRule` now lives in
  `luaconfig.h` (`lt`/`rr` typed as `const void *`/`uint32_t` there,
  same reasoning as `Arg.v` holding a layout pointer -- that header can't
  see dwl.c's `Layout` type or pull in a wayland/wlroots include just for
  one enum), `monrules`/`nmonrules` are runtime globals `luaconfig.c`'s
  `load_monitors()` rebuilds on every `waspconfig_load()` call,
  `config.def.h`/`config.h` only keep an explanatory comment where the
  static array used to be. Unlike `wasp.rules`, an *empty* monitor-rule
  set is a real footgun (a monitor with mfact=0/nmaster=0/no layout) --
  so `set_defaults()` in `luaconfig.c` does get a one-entry emergency
  fallback here, the exact same default `config.def.h` used to hardcode
  (any output, mfact 0.55, nmaster 1, scale 1, tile, no rotation,
  autoconfigured position), built via `wasp_layout_by_name("tile")`
  rather than statically, since that's a plain lookup into dwl.c's
  `layouts[]` with no Lua/config.lua dependency, safe to call this
  early. First-match-wins per output (not OR-accumulated like `Rule`'s
  `tags`) -- deliberately preserved upstream dwl's own `monrules[]`
  semantics rather than switching to `Rule`-style accumulation, since
  "which single set of mfact/nmaster/layout applies to this monitor"
  doesn't have an OR-able reading the way multiple tags does.

  New vs. upstream dwl: only `scale` is live. A new `setscale` action
  (`dwl.c`, next to `setmfact()`) nudges `selmon`'s output scale by a
  relative `delta` via the same minimal `struct wlr_output_state state =
  {0}; wlr_output_state_set_scale(...); wlr_output_commit_state(...);
  updatemons(NULL, NULL);` idiom `powermgrsetmode()` already used for a
  single-field output-state change, bound to `mod+shift+p`/`m` -- the
  exact same keys spitfire already uses for the same thing, so muscle
  memory carries over between the two projects (spitfire's own
  `examples/config.lua` comment for `spitfire.output` was the direct
  source for "starting value at startup + live reload + dedicated
  inc/dec keybind" as the shape to aim for). `reload()` re-applies
  `scale` specifically (re-matching every already-connected monitor's
  name against `monrules`, first match, only committing if the value
  actually changed) but deliberately *not*
  `mfact`/`nmaster`/`layout`/`transform`/`x`/`y` -- those stay
  createmon()-time-only, matching how `mfact`/`nmaster` already don't
  get reset by `reload()` today (both are meant to be freely
  keybind-adjustable at runtime via `setmfact`/`incnmaster` without a
  later `reload()` silently reverting the live tweak back to whatever
  `config.lua` says; unlike `scale`, which realistically only gets
  changed rarely/deliberately, so reapplying it on reload is far less
  likely to surprise anyone). Verified with a nested test instance:
  `wasp.monitors = { { scale = 1.5 } }` produced `wl_output.scale: 2`
  over the wire (the legacy integer-only `wl_output.scale` event rounds
  a fractional value up -- expected wlroots behavior for any fractional-
  scale-capable compositor, not a bug -- the real `1.5` float lives in
  `wlr_output->scale` and reaches fractional-scale-aware clients via
  `wp-fractional-scale-v1` instead).

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
- **D-Bus session bus + xdg-desktop-portal (done, 2026-08-14)**: confirmed
  live (`DBUS_SESSION_BUS_ADDRESS` empty in a real wasp session, only the
  distro's system bus running) that nothing upstream of `wasp-session` —
  not wasp itself, not greetd — ever starts a D-Bus *session* bus. This is
  the same root cause already written up above as d77run's app_id
  instability (GApplication falling back to its raw id when D-Bus
  single-instance registration has nothing to register against), just
  caught at the source this time instead of downstream. `scripts/
  wasp-session` now wraps the whole pipeline in `dbus-run-session --`
  (same pattern as spitfire's `packaging/spitfire-session`) and exports
  `XDG_CURRENT_DESKTOP=wasp` before handing off, so every client — the
  bar's stdin pipe, `wasp.autostart`, anything a user spawns — inherits a
  real session bus from the very first process. `packaging/
  wasp-portals.conf` (new dir, installed to
  `/etc/xdg-desktop-portal/wasp-portals.conf` by `make install`) pins
  `default=gtk` (FileChooser/Settings/Notification/Print, no DE required)
  plus explicit `Secret=gnome-keyring` (already running here) and
  `ScreenCast=wlr`/`Screenshot=wlr` — wasp already creates both
  `wlr_screencopy_manager_v1` and `wlr_export_dmabuf_manager_v1` in
  `setup()`, which is what `xdg-desktop-portal-wlr` needs on the
  compositor side, so those two are real, not aspirational. Deliberately
  not `xdg-desktop-portal-hyprland`, even though it's the one already
  installed on this machine — it talks to Hyprland's own IPC socket for
  window listing and doesn't work against a plain wlr-screencopy
  compositor like wasp. Needs `xdg-desktop-portal`,
  `xdg-desktop-portal-gtk`, and `xdg-desktop-portal-wlr` installed (none
  were, as of 2026-08-14 — only `-hyprland` was) for the portals to
  actually resolve; the `.conf` file alone just picks which backend
  *would* handle each interface once something provides it.

  **Tested end-to-end (2026-08-14), not just by inspection**: installed
  `xdg-desktop-portal-gtk`/`-wlr`, `make install`ed the new
  `wasp-session`/`wasp-portals.conf`, then ran a nested wasp
  (client of the real session, own isolated D-Bus/XDG_RUNTIME_DIR) with
  `xdg-desktop-portal -v` inside the same bus. Log confirms `wasp-portals.
  conf` is actually picked up (`XDP: Using portal configuration file
  '/etc/xdg-desktop-portal/wasp-portals.conf' for desktop 'wasp'`), `gtk`
  resolves and stays up for FileChooser/Notification/Settings/etc., and
  `wlr` is correctly *selected* for Screenshot/ScreenCast — but
  `xdg-desktop-portal-wlr` itself then failed (`couldn't connect to
  context` / `failed to initialize screencast`): **PipeWire wasn't
  running** — installed (`pipewire`, `wireplumber`) but nothing started
  it, same shape of problem as the D-Bus one above, one layer up. Not a
  wasp bug — confirmed wasp's own protocol support is fine by running
  `xdg-desktop-portal-wlr` directly against the nested wasp and watching
  it connect (`wlroots: wl_display connected`, negotiates dmabuf feedback
  and `zwlr_output_manager_v1` cleanly) before failing purely on the
  PipeWire step. Fixed by adding `{"pipewire"}` to `wasp.autostart` (live
  `config.lua`; `examples/config.lua` gets the same line commented out,
  with a note not to add `pipewire-pulse` since it would contend with the
  separately-running PulseAudio daemon already handling audio here) —
  re-ran the same nested test with that autostarted and
  `xdg-desktop-portal-wlr` got past the PipeWire step cleanly this time
  (`Using render node /dev/dri/renderD128`, dmabuf/xdg_output
  negotiation, no errors). Initially added `wireplumber` alongside it too
  (thinking it needed starting separately as the session/policy manager)
  -- Daniel corrected that on Void's `pipewire` package it comes up on
  its own; re-tested with *only* `{"pipewire"}` in autostart and
  confirmed `wireplumber` still shows up in `ps` by itself, portal test
  still clean either way. `wasp.autostart` only runs once at startup (see
  Hot-reload notes above), so this needs an actual restart of wasp to
  take effect, not just a reload.

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
