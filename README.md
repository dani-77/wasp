<p align="center"><img src="assets/logo-icon.png" width="128" alt="wasp logo"></p>

<h1 align="center">wasp</h1>

<p align="center"><em>Fast · Light · Focused</em></p>

---

**wasp** is a fork of [dwl] (dwm for Wayland) aiming for one thing dwl
deliberately doesn't do: a config you can change without recompiling.
Everything lives in `~/.config/wasp/config.lua`, and reloading it takes a
save and a keypress, not a rebuild — see Hot-reload below. Beyond that,
wasp keeps dwl's own goals — small, hackable, few dependencies, suckless
in spirit — and pulls in a handful of [dwl-patches] adapted to fit this
model rather than `config.h` + recompile.

## Status (2026-08-15)

Most of the day-to-day compositor is Lua-driven now. What's actually
working:

- **Forked from dwl `main`**, full upstream git history kept.
- **Bar**: the [dwl-patches] `bar` + `barconfig` patches applied and
  verified live — a real dwm-style status bar (text via fcft/pixman/tllist),
  fed status text over `stdin` the classic dwl way (`your-script | wasp`).
  `scripts/statusbar.sh` is a ready-to-use one (CPU/RAM/volume/battery/
  clock) — without something feeding it, the bar just shows its startup
  placeholder text forever, it has no built-in widgets of its own.
- **Lua config, appearance**: wasp embeds Lua 5.4 (`luaconfig.c`/`.h`) and
  loads `~/.config/wasp/config.lua` at startup. Border width/colors, the
  bar's enable/position/layout, and the background color are live-driven
  from it — see `examples/config.lua`. Verified by eye in a nested session.
- **Keybindings**: fully Lua-driven — `wasp.keys` in `config.lua` builds the
  keymap at startup, `wasp.modkey` picks the primary modifier, and
  `wasp.terminal`/`wasp.menu` make the launched terminal/menu agnostic
  (no more hardcoded `foot`). Suggested bindings for workspace switching,
  window/monitor navigation, resize (master-area and keyboard
  move/resize), and layout cycling (tile/floating/monocle/dwindle) ship in
  `examples/config.lua`; a small built-in fallback keymap keeps you from
  ever being locked out if `config.lua` is missing or broken.
- **Layouts**: tile, floating, monocle (upstream dwl), plus `dwindle`
  (fibonacci/spiral tiling, adapted from [dwl-patches]).
- **Gaps**: `wasp.gaps = { inner, outer, smart }` — inner/outer spacing
  around tiled windows, `smart` drops the outer gap for a single window.
- **Keyboard**: `wasp.keyboard = { layout, variant, model, options, rules,
  repeat_rate, repeat_delay }` — xkb layout switching and repeat speed,
  live from `config.lua` instead of `config.h` constants.
- **Session file**: `make install` installs the `wasp` binary and a
  `wasp.desktop` under `wayland-sessions`, so greetd (or any greeter that
  reads that directory) can list and select it.
- **Autostart**: `wasp.autostart = { {"swaybg", "-i", "wall.png"}, ... }` —
  an array of argv arrays, fork+exec'd once at startup (no shell), killed
  cleanly on exit.
- **Hot-reload**: a bound key (`mod+shift+r` by default) re-reads
  `config.lua` and re-applies gaps, `wasp.animations` (including
  re-baking its easing curves), the bar (visibility/position/colors),
  every window's border color, the background, keyboard layout/repeat
  speed, and keybindings themselves — all live, no restart. Border
  *width* on already-open windows and `wasp.autostart` still need one
  (autostart only ever runs once on purpose, so reload doesn't relaunch
  everything in it).
- **Named scratchpads**: `wasp.scratchpad = { { name=, cmd=, app_id=, w=,
  h= }, ... }` — a hidden, toggleable floating window per slot, spawned
  the first time it's toggled (bound via the `toggle-scratchpad` action)
  and hidden/shown (not killed/respawned) on every toggle after that.
- **Window rules**: `wasp.rules = { { app_id=, title=, tags=, floating=,
  monitor=, center=, shield_when_capture= }, ... }` — send specific apps
  to a workspace, make them open floating, pin them to a monitor, or
  re-center them at their own requested size instead of wherever they
  happened to want to place themselves (often the top-left corner).
  `shield_when_capture` refuses that window's own single-window capture
  requests and blanks it for the duration of any real whole-output
  capture (recording/screen-share) — it's the same on-screen content, so
  you see the blank too for as long as capture is live, a visible cue
  that it's protected, not a hidden swap; reverts to normal the instant
  capture ends, so it's never a permanent placeholder.
- **Workspaces exposed over `ext-workspace-v1`**: one workspace group per
  monitor, one workspace per tag inside it — so any bar/shell that speaks
  the protocol (not a dwl/wasp-specific thing to build against) sees and
  can switch tags, something dwl had no story for at all before this.
  Verified end-to-end with a standalone protocol client, not just by
  inspection.
- **Per-output rules and live scale**: `wasp.monitors = { { name=,
  mfact=, nmaster=, scale=, layout=, transform=, x=, y= }, ... }` — same
  idea as `wasp.rules`, but for outputs instead of clients (a monitor
  uses the first matching rule, matching upstream dwl's own `monrules[]`
  semantics). `scale` is live — a bound key (`mod+shift+p`/`m`, same
  convention as spitfire) nudges it up/down at runtime, and it's the one
  field `reload` re-applies; the rest stays a startup-time setting, like
  it always was.

- **D-Bus session + xdg-desktop-portal**: `wasp-session` (what `wasp.desktop`
  actually launches) wraps startup in `dbus-run-session` and exports
  `XDG_CURRENT_DESKTOP=wasp`, so every client — the bar's status pipe,
  `wasp.autostart`, anything you launch by hand — gets a real D-Bus
  session bus from the first process on, instead of falling back to an
  ad-hoc one per app (or none at all). `packaging/wasp-portals.conf`
  (installed to `/etc/xdg-desktop-portal/`) picks `gtk` for
  FileChooser/Settings/Notification and `wlr` for ScreenCast/Screenshot —
  see Building below for the packages that back those.
- **Animations**: `wasp.animations = { enable, duration_move, duration_open,
  duration_close, duration_tag, type_open, type_close, zoom_ratio,
  fade_from_opacity, tag_direction, curve_move, curve_open, curve_close,
  curve_tag }` — MangoWC-style open/close/move/tag-switch tweening
  (`"fade"`/`"zoom"`/`"none"`, cubic-bezier easing per action). `enable =
  false` (the default) reproduces the original instant behavior
  bit-for-bit; live on hot-reload, same as gaps. Not ported: layer-shell
  (bar/panel) animation, per-window-rule overrides, `slide`-from-edge for
  OPEN — see `NOTES.md` item 3 for the full scope note.
- **Touchpad gestures**: `wasp.gestures = { { fingers=, direction=,
  action=, ... }, ... }` — swipe gestures dispatched through the exact same
  action/arg table as `wasp.keys` (`focusmon`, `view`, `spawn`, ... all work
  here too), matched by finger count (0/omitted = any) and direction
  (`"left"`/`"right"`/`"up"`/`"down"`, classified from the swipe's dominant
  axis once it ends). Swipe only — no pinch/hold yet.
- **Modern screen-capture protocol**: `ext-image-copy-capture-v1` +
  `ext-image-capture-source-v1` + `ext-foreign-toplevel-list-v1`, so a
  client can request to capture one specific window, not just the whole
  output — the legacy, wlroots-only, whole-output-only
  `wlr-screencopy`/`wlr_export_dmabuf` pair (what `grim` still uses)
  keeps working exactly as before, unreplaced, running alongside it. See
  `wasp.rules`' `shield_when_capture` above for the privacy half.

Not done yet: mouse-drag resize variety and more border styles. The full
running list, plus which [dwl-patches] are earmarked for which feature and
why, lives in [`NOTES.md`](NOTES.md) — that's the file to check for current
plans, not this README.

## Building

Same dependencies as upstream dwl, plus Lua 5.4:

- libinput, wayland, **wlroots 0.20** (libinput backend), xkbcommon
- wayland-protocols, pkg-config (compile-time only)
- fcft, pixman, tllist (for the bar)
- **lua5.4** (development headers — e.g. `lua54-devel` on Void Linux)
- libxcb, libxcb-wm (XWayland — see below)

XWayland is built by default (needs a wlroots built with X11 support,
which is the common case); disable it by commenting out the relevant
lines in `config.mk` if you'd rather not pull in libxcb at all.

```sh
make
sudo make install
```

`make install` also drops `packaging/wasp-portals.conf` under
`/etc/xdg-desktop-portal/`, but it only has anything to configure once
the backends it names are actually installed: `dbus`, `dbus-run-session`
(usually the same package), `xdg-desktop-portal`,
`xdg-desktop-portal-gtk`, and `xdg-desktop-portal-wlr`. None of these are
wasp-specific — same portal backends most wlroots compositors use — so
they're not pulled in by `make install` itself, just expected to already
be on the system (or installed alongside it) the way libinput or wayland
are.

## Configuring

Copy the example to get started:

```sh
mkdir -p ~/.config/wasp
cp examples/config.lua ~/.config/wasp/config.lua
```

`make install` also drops a copy at `/usr/local/share/wasp/config.lua`
(`$PREFIX/share/wasp/`) — if you installed from a release tarball rather
than a source checkout, that's where to find it instead of hunting for
`examples/` in source you may not have on disk:

```sh
cp /usr/local/share/wasp/config.lua ~/.config/wasp/config.lua
```

`examples/config.lua` is both the default and the reference: appearance,
gaps, keyboard, monitors/output scale, terminal/menu, autostart, named
scratchpads, window rules, and a full suggested keymap (with a comment
block explaining every action and its fields). It'll keep growing as more
of `config.h` moves over to Lua — see [`NOTES.md`](NOTES.md) for what's
coming.

For the bar's right-side status text, pipe something into wasp's `stdin`
(it has no built-in widgets of its own, same as upstream dwl) —
`scripts/statusbar.sh` is a ready-to-use one. Since stdin is fixed at
launch, this has to happen at the point wasp itself is started, not
after — `wasp.desktop`'s `Exec=` already points at a wrapper
(`scripts/wasp-session`, installed as `wasp-session`) that does exactly
that, so if you launch wasp through `make install` + a greeter (greetd,
...), it's wired up automatically and there's nothing else to do.

For local/dev testing, without installing, do it by hand instead:

```sh
scripts/statusbar.sh | ./wasp
```

## Credit

wasp is a fork — nearly everything here is [dwl]'s work, not wasp's own.
The original dwl README (build details, community links, project
philosophy, acknowledgements to Devin J. Pohly and the rest of the dwl
contributors) is preserved at [`doc/NOTES.md`](doc/NOTES.md) rather than
overwritten.

## License

Two licenses, because this is a fork, not a from-scratch project:

- **`LICENSE` (GPLv3)** — dwl's own license. `wasp.c`, `config.def.h`,
  `config.mk`, `Makefile`, `client.h`, `util.c`, `util.h`, `wasp.1`, and
  `protocols/*` are all modifications of dwl's original files, so they (and
  the compiled `wasp` binary as a whole, since it links GPLv3 code) stay
  under GPLv3 — that's simply a requirement of the license they were under
  before wasp touched them, not a choice wasp gets to make.
- **[`LICENSE.wasp`](LICENSE.wasp) (MIT)** — files wasp added that have no
  upstream dwl equivalent and aren't derived from it: this README,
  `NOTES.md`, `luaconfig.c`/`.h`, `examples/config.lua`, `scripts/*`,
  `packaging/*`, `wasp.desktop`, and `assets/*`. Copyright Daniel Azevedo.

[dwl]: https://codeberg.org/dwl/dwl
[dwl-patches]: https://codeberg.org/dwl/dwl-patches
