<p align="center"><img src="assets/logo-icon.png" width="128" alt="wasp logo"></p>

<h1 align="center">wasp</h1>

<p align="center"><em>Fast · Light · Focused</em></p>

---

**wasp** is a fork of [dwl] (dwm for Wayland) aiming for one thing dwl
deliberately doesn't do: a config you can change without recompiling.
Everything lives in `~/.config/wasp/config.lua`, and (once reload lands —
see Status below) reloading it takes a save and a keypress, not a rebuild.
Beyond that, wasp keeps dwl's own goals — small, hackable, few dependencies,
suckless in spirit — and pulls in a handful of [dwl-patches] adapted to fit
this model rather than `config.h` + recompile.

## Status (2026-08-12)

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
  `config.lua` and re-applies gaps, the bar (visibility/position/colors),
  every window's border color, the background, keyboard layout/repeat
  speed, and keybindings themselves — all live, no restart. Border
  *width* on already-open windows and `wasp.autostart` still need one
  (autostart only ever runs once on purpose, so reload doesn't relaunch
  everything in it).

Not done yet: mouse-drag resize variety and more border styles. The full
running list, plus which [dwl-patches] are earmarked for which feature and
why, lives in [`NOTES.md`](NOTES.md) — that's the file to check for current
plans, not this README.

## Building

Same dependencies as upstream dwl, plus Lua 5.4:

- libinput, wayland, wlroots (libinput backend), xkbcommon
- wayland-protocols, pkg-config (compile-time only)
- fcft, pixman, tllist (for the bar)
- **lua5.4** (development headers — e.g. `lua54-devel` on Void Linux)

XWayland needs libxcb, libxcb-wm, and a wlroots built with X11 support;
enable it by uncommenting the relevant lines in `config.mk`.

```sh
make
sudo make install
```

## Configuring

Copy the example to get started:

```sh
mkdir -p ~/.config/wasp
cp examples/config.lua ~/.config/wasp/config.lua
```

`examples/config.lua` is both the default and the reference: appearance,
gaps, keyboard, terminal/menu, and a full suggested keymap (with a comment
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

- **`LICENSE` (GPLv3)** — dwl's own license. `dwl.c`, `config.def.h`,
  `config.mk`, `Makefile`, `client.h`, `util.c`, `util.h`, `dwl.1`, and
  `protocols/*` are all modifications of dwl's original files, so they (and
  the compiled `wasp` binary as a whole, since it links GPLv3 code) stay
  under GPLv3 — that's simply a requirement of the license they were under
  before wasp touched them, not a choice wasp gets to make.
- **[`LICENSE.wasp`](LICENSE.wasp) (MIT)** — files wasp added that have no
  upstream dwl equivalent and aren't derived from it: this README,
  `NOTES.md`, `luaconfig.c`/`.h`, `examples/config.lua`, `scripts/*`,
  `wasp.desktop`, and `assets/*`. Copyright Daniel Azevedo.

[dwl]: https://codeberg.org/dwl/dwl
[dwl-patches]: https://codeberg.org/dwl/dwl-patches
