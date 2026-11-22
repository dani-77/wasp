# Changelog

wasp is a fork of [dwl](https://codeberg.org/dwl/dwl), branched from its
`main` on 2026-02-25 (after dwl 0.8). Entries from dwl are tagged with
the upstream commit they come from.

* [0.12.1](#0121)
* [0.12.0](#0120)
* [0.11.0](#0110)
* [0.10.1](#0101)
* [0.10.0](#0100)
* [0.9.1](#091)
* [0.9.0](#090)
* [dwl history](#dwl-history)


## 0.12.1

### Fixed

* A layer surface that switches its keyboard interactivity to none while
  mapped gives the keyboard back to the focused client, as unmapping one
  does. helium-d77 keeps its launcher and session menu mapped and hides
  them that way, so keys kept going to the hidden panel.


## 0.12.0

### Added

* Occupied tags for the d77 shells (Utumno, quickshell-d77, fabric-d77,
  helium-d77): ext-workspace-v1 has no "occupied" state, so wasp, like
  whiplash and spitfire, keeps
  `$XDG_RUNTIME_DIR/d77-workspaces-$WAYLAND_DISPLAY` with each tag's
  window count, which the shells read to dim empty tags.

### Fixed

* Reloading a `config.lua` with a syntax or runtime error (the `reload`
  action or SIGHUP) no longer drops the session back to the built-in
  defaults, keybindings included: the config already in effect stays,
  and the error is logged and sent as a notification ending in
  "Keeping the previous config." A missing config file still means the
  defaults, as does a broken file at startup.


## 0.11.0

### Added

* `focusdir` and `swapdir` actions (`dir = "left"|"right"|"up"|"down"`):
  move focus to / swap places with the tiled window next to the focused
  one in that direction, regardless of layout or stacking order. Idea
  from vindle.
* Config warnings: an entry in `wasp.keys`, `wasp.gestures` or
  `wasp.wheel` with an unknown key, action or direction is still skipped
  but now says so on stderr (so in `wasp.log`) instead of silently
  vanishing, e.g. after a SIGHUP reload. The warnings of one load are also
  sent as a single `notify-send` desktop notification (best effort: needs
  `notify-send` and a notification daemon).


## 0.10.1

### Added

* SIGHUP reloads config.lua, same as the `reload` action, so scripts and
  settings tools (Kantele) can apply changes: `pkill -HUP -x wasp`.
* `none` action: does nothing and lets the key through to the focused
  window. Bind it earlier in `wasp.keys` to switch off a later binding.
* `wasp-session` writes the session's output to
  `$XDG_STATE_HOME/wasp/wasp.log` (the previous session's log is kept as
  `wasp.log.old`) and enables core dumps, so a crash can be looked into
  after logging back in.


## 0.10.0

Selected fixes and features ported from dwl 0.9 and dwl `main` (up to
a6e2c15, 2026-11-01), plus wasp fixes found along the way.

### Added

* `wasp.wheel`: mouse wheel bindings, same entry shape and actions as
  `wasp.keys` and `wasp.gestures`. Only a real wheel runs them, once per
  notch; nothing is bound by default (dwl e8bf70d, fa400c5, 40b0ae7).
* `wasp.snap`: floating windows dragged with the mouse snap to the
  monitor's edges within this many pixels; default 32, 0 disables
  (dwl 8f52e96).
* `focusmon`/`tagmon` accept `dir = "up"` and `"down"` (dwl 5fd19fe).
* layer-shell v4: on-demand keyboard interactivity (dwl 0a6dc6d).
* Seat capabilities are set for virtual keyboards and pointers, so they
  work with no physical input device (dwl 99cdc9a).
* The output swapchain manager is used when applying output
  configurations (dwl dc9a860).

### Changed

* Keybindings match a key's unshifted or shifted symbol on the active
  layout, so `{ "mod", "shift" }` + `"2"` works on any layout. The
  effective symbol is still tried, so existing configs naming shifted
  symbols, `XF86Switch_VT_n` and `Terminate_Server` keep working
  (dwl f9c644f, bab3fda). The default keymap and `examples/config.lua`
  now use `1`-`9`, `0`, `comma` and `period` with Shift.
* Clients' min/max size hints are respected (dwl 4de32d2, 1dae5ba,
  45054a3).
* Unmanaged X11 clients get their own scene layer, above fullscreen
  clients (dwl 1fe1b4e).
* Focus leaves an unmanaged surface (a menu) properly: its parent is
  deactivated so it closes the menu; focusing a menu or layer surface
  keeps the parent's focused border color (dwl f6e3a28, 6013835,
  669b26d).
* Outputs turned off by a client stay off when they reconnect
  (dwl 4953b7e).
* Focus goes to the visible fullscreen client after a tag switch
  (dwl a6e2c15).

### Removed

* The rendermon() frame skip for pending resizes, which made resizing
  choppy (dwl 70c33c0, 90e7c72).

### Fixed

* Use-after-free when destroying a client with a decoration
  (dwl f4dfdab, 04279f2).
* Use-after-free in the idle inhibitor check after a client unmaps
  (dwl 4847f97).
* Use-after-free of the foreign-toplevel handle (title change while
  unmapped) and of the window capture source (capture after a remap).
* Crash on exit when XWayland failed to start (dwl f08d8b3).
* Crash with more outputs than the GPU supports (dwl afbafd5, 9dfaea2).
* While locked: keybindings no longer run on key repeat, clients no
  longer get pointer events, and `wasp.gestures` swipes no longer run
  their actions (dwl f22c223, b83881f).
* The release of a key used by a binding is no longer sent to the
  client that gets the focus (dwl f6800e9).
* Asleep disabled outputs are kept in the output configuration
  (dwl babccfe).
* Cursor image after an interactive move/resize (dwl 8ad2c92).
* Floating windows mapped offset by their client-side decorations, and
  popup positioning (dwl 2a56719, e203845).
* Firefox opening with rounded corners when tiled (dwl d1ebf3c,
  d9ddaba).
* Unmanaged X11 windows not following their own geometry changes, and
  X11 clients shifting by the border width on map (dwl e451862,
  4fd683c).

### Contributors

Thanks to the dwl contributors whose work is ported here: Alex Denes,
Andrea Chiavazza, brinkpw, DanD, DreamMaoMao, Guido Cella, guyuming,
julmajustus, klim, Leonardo Hernández Hernández, MayOrMayNotBeACat,
Peter Hofmann, save196, sewn.


## 0.9.1

* Statusbar pipe no longer leaks into autostart children.
* Capture-manager listeners are removed on cleanup.
* Packaging: missing fcft/pixman/tllist/wayland-protocols dependencies.
* Leftover "dwl" strings from the fork rename (`-v`, execvp failure,
  fallback bar text).


## 0.9.0

First wasp release: live Lua config with hot-reload, named scratchpads,
animations, per-output rules and live scale, ext-workspace-v1, window
rules, rounded corners and blur (scenefx), touchpad gestures, the
modern screen-capture protocols with per-window capture privacy. See
NOTES.md and doc/DEVELOPMENT.md.


## dwl history

Changes from dwl itself up to the fork, kept as dwl wrote them.

## dwl, after 0.7
### Added

* Support for the linux-drm-syncobj-v1 protocol ([wlroots!4715][wlroots!4715], [#685][685])
* Allow the use of non-system wlroots library ([#646][646])

[wlroots!4715]: https://gitlab.freedesktop.org/wlroots/wlroots/-/merge_requests/4715
[685]: https://codeberg.org/dwl/dwl/pulls/685
[646]: https://codeberg.org/dwl/dwl/pulls/646


### Changed
### Deprecated
### Removed
### Fixed

* Crash when a client is created while all outputs are disabled.

### Security
### Contributors


## dwl 0.7

This version is just 0.6 with wlroots 0.18 compatibility.

### Added

* Add support for the alpha-modifier-v1 protocol ([wlroots!4616][wlroots!4616]).
* dwl now will survive GPU resets ([#601][601]).

[wlroots!4616]: https://gitlab.freedesktop.org/wlroots/wlroots/-/merge_requests/4616
[601]: https://codeberg.org/dwl/dwl/issues/601


### Contributors

Guido Cella


## dwl 0.6

### Added

* Add `rootcolor` to change the default background color ([#544][544]).
* Implement the wlr-virtual-pointer-unstable-v1 protocol ([#574][574]).
* Implement the pointer-constraints and relative-pointer protocols ([#317][317])
* Implement the wlr-output-power-management protocol ([#599][599])

[544]: https://codeberg.org/dwl/dwl/pulls/544
[574]: https://codeberg.org/dwl/dwl/pulls/574
[317]: https://codeberg.org/dwl/dwl/issues/317
[599]: https://codeberg.org/dwl/dwl/issues/559


### Changed

* Keyboards are now managed through keyboard groups ([#549][549]).
* Only the first matched keybinding is executed.
* Allow toggling the layout before selecting a different one ([#570][570]).
* Fullscreen clients are now rendered above wlr_layer_surfaces in the top layer
  ([#609][609]).
* The default menu was changed from `bemenu-run` to `wmenu-run` ([#553][553]).
* The option `sloppyfocus` now replicates the dwm behavior ([#599][599]).
* Allow configure position of monitors with negative values. (-1, -1) is
  used to auto-configure them ([#635][635]).
* dwl now kills the entire process group of `startup_cmd`
* The O_NONBLOCK flag is set for stdout.

[549]: https://codeberg.org/dwl/dwl/pulls/549
[570]: https://codeberg.org/dwl/dwl/pulls/570
[609]: https://codeberg.org/dwl/dwl/pulls/609
[553]: https://codeberg.org/dwl/dwl/issues/553
[599]: https://codeberg.org/dwl/dwl/pulls/599
[635]: https://codeberg.org/dwl/dwl/pulls/635


### Removed

* The SLOC limit is now removed ([#497][497]).

[497]: https://codeberg.org/dwl/dwl/pulls/497


### Fixed

* Clients not having the correct border color when mapping.
* Compliance with the xdg-decoration-unstable-v1 ([#546][546]).
* dwl no longer sends negative values in xdg_toplevel.configure events.
* Crashes with disabled monitors ([#472][472]).

[546]: https://codeberg.org/dwl/dwl/pulls/546
[472]: https://codeberg.org/dwl/dwl/issues/472


### Contributors

Ben Jargowsky
Benjamin Chausse
David Donahue
Devin J. Pohly
Dima Krasner
Emil Miler
Forrest Bushstone
Guido Cella
Peter Hofmann
Rutherther
Squibid
choc
fictitiousexistence
korei999
sewn
thanatos


## dwl 0.5

### Added

* Allow configure x and y position of outputs ([#301][301])
* Implement repeatable keybindings ([#368][368])
* Print app id in printstatus() output ([#381][381])
* Display client count in monocle symbol ([#387][387])
* Export XCURSOR_SIZE to fix apps using an older version of Qt ([#425][425])
* Support for wp-fractional-scale-v1 (through wlr_scene: [wlroots!3511][wlroots!3511])
* dwl now sends `wl_surface.preferred_buffer_scale` (through wlr_scene: [wlroots!4269][wlroots!4269])
* Add support for xdg-shell v6 ([#465][465])
* Add support for wp-cursor-shape-v1 ([#444][444])
* Add desktop file ([#484][484])
* Add macro to easily configure colors ([#466][466])
* Color of urgent clients are now red ([#494][494])
* New flag `-d` and option `log_level` to change the wlroots debug level
* Add CHANGELOG.md ([#501][501])

[301]: https://github.com/djpohly/dwl/pull/301
[368]: https://github.com/djpohly/dwl/pull/368
[381]: https://github.com/djpohly/dwl/pull/381
[387]: https://github.com/djpohly/dwl/issues/387
[425]: https://github.com/djpohly/dwl/pull/425
[wlroots!4269]: https://gitlab.freedesktop.org/wlroots/wlroots/-/merge_requests/4269
[wlroots!3511]: https://gitlab.freedesktop.org/wlroots/wlroots/-/merge_requests/3511
[465]: https://github.com/djpohly/dwl/pull/465
[444]: https://github.com/djpohly/dwl/pull/444
[484]: https://github.com/djpohly/dwl/pull/484
[466]: https://github.com/djpohly/dwl/issues/466
[494]: https://github.com/djpohly/dwl/pull/494
[501]: https://github.com/djpohly/dwl/pull/501


### Changed

* Replace `tags` with `TAGCOUNT` in config.def.h ([#403][403])
* Pop ups are now destroyed when focusing another client ([#408][408])
* dwl does not longer respect size hints, instead clip windows if they are
  larger than they should be ([#455][455])
* The version of wlr-layer-shell-unstable-v1 was lowered to 3 (from 4)
* Use the same border color as dwm ([#494][494])

[403]: https://github.com/djpohly/dwl/pull/403
[408]: https://github.com/djpohly/dwl/pull/409
[455]: https://github.com/djpohly/dwl/pull/455
[494]: https://github.com/djpohly/dwl/pull/494


### Removed

* Remove unused `rootcolor` option ([#401][401])
* Remove support for wlr-input-inhibitor-unstable-v1 ([#430][430])
* Remove support for KDE idle protocol ([#431][431])

[401]: https://github.com/djpohly/dwl/pull/401
[430]: https://github.com/djpohly/dwl/pull/430
[431]: https://github.com/djpohly/dwl/pull/431


### Fixed

* Fix crash when creating a layer surface with all outputs disabled
  ([#421][421])
* Fix other clients being shown as focused if the focused client have pop ups
  open ([#408][408])
* Resize fullscreen clients when updating monitor mode
* dwl no longer crash at exit like sometimes did
* Fullscreen background appearing above clients ([#487][487])
* Fix a segfault when user provides invalid xkb_rules ([#518][518])

[421]: https://github.com/djpohly/dwl/pull/421
[408]: https://github.com/djpohly/dwl/issues/408
[487]: https://github.com/djpohly/dwl/issues/487
[518]: https://github.com/djpohly/dwl/pull/518


### Contributors

* A Frederick Christensen
* Angelo Antony
* Ben Collerson
* Devin J. Pohly
* Forrest Bushstone
* gan-of-culture
* godalming123
* Job79
* link2xt
* Micah Gorrell
* Nikita Ivanov
* Palanix
* pino-desktop
* Weiseguy
* Yves Zoundi
