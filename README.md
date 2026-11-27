<p align="center"><img src="assets/logo-icon.png" width="128" alt="wasp logo"></p>

<h1 align="center">wasp</h1>

<p align="center"><em>Fast · Light · Focused</em></p>

---

**wasp** is a [dwl]-based Wayland compositor with dwm-style tiling,
Lua configuration and live reload. It uses [wlroots] and [SceneFX] for
window management and rendering. Configure appearance, keybindings,
touchpad gestures, autostart, scratchpads and window and output rules
in `~/.config/wasp/config.lua`.

## Features

- **Live Lua config** — reload keybindings, gestures, gaps, animations,
  bar settings, keyboard layout and output scale.
- **Tiling layouts** — tile, monocle, dwindle (fibonacci/spiral) and
  floating, with layout selection per monitor.
- **Workspace overview** — browse scaled previews of neighbouring tags
  and drag windows between them.
- **Animations** — open/close/move/tag-switch tweening, fade or zoom,
  cubic-bezier easing, off by default.
- **Rounded corners + blur** — via SceneFX: window corner radius,
  per-window blur-behind, and background/wallpaper blur.
- **Touchpad gestures** — swipe actions (finger count + direction)
  wired through the same action table as keybindings.
- **Named scratchpads** — hidden, toggleable floating windows per slot
  (a drop-down terminal, a notes app, ...).
- **Per-window capture privacy** — flag a window to refuse individual
  capture and hide its content during whole-output capture through
  `ext-image-copy-capture-v1`. The overview also shields its previews.
- **Window & output rules** — send an app to a workspace, float it,
  center it, pin it to a monitor; per-output scale/layout/rotation.
- **Workspaces over `ext-workspace-v1`** — any compatible bar or shell
  can see and switch your workspaces, no wasp-specific integration
  needed.
- **A real status bar** — dwm-style, fed by any script over `stdin`.

## Install

**Arch Linux**: in the AUR as
[`wasp-d77`](https://aur.archlinux.org/packages/wasp-d77)
(`yay -S wasp-d77`; the plain `wasp` AUR name is an unrelated MQTT
broker). The same `PKGBUILD` is in
[`packaging/arch`](packaging/arch/PKGBUILD).

**Void Linux**: a package template is included
([`packaging/void/template`](packaging/void/template), a copy of the
one in [`d77void/srcpkgs-d77`](https://github.com/d77void/srcpkgs-d77))
for building with `xbps-src`.

See the [build instructions](doc/DEVELOPMENT.md#building) for dependencies
and source installation. The workspace overview is available in this
source tree. Packages built from v0.12.1 predate it.

## Configure

For a new config, copy the example from your installation or checkout.

| Installation | Example config |
| --- | --- |
| Arch or Void package | `/usr/share/wasp/config.lua` |
| Default source install | `/usr/local/share/wasp/config.lua` |
| Source checkout | [`examples/config.lua`](examples/config.lua) |

From a source checkout, copy the file with the following commands.

```sh
config_dir="${XDG_CONFIG_HOME:-$HOME/.config}/wasp"
mkdir -p "$config_dir"
cp -i examples/config.lua "$config_dir/config.lua"
```

The example sets `Mod` to Alt. Press **Mod+Shift+R** to apply edits, or run
`pkill -HUP -x wasp` from a script. A Lua syntax or runtime error during
reload keeps the previous config. See [live reload](doc/DEVELOPMENT.md#live-reload)
for when each setting takes effect.

## Workspace overview

Press **Mod+O** or swipe up with four fingers to open the overview on the
selected monitor. The example config sets these in `wasp.keys` and
`wasp.gestures`. The selected tag appears in the centre, with its immediate
neighbours above and below. Use Up/Down or scroll to browse, Enter or click
to switch, and Escape to cancel.

Drag a window onto a neighbouring card to move it there. A thumbnail
follows the pointer and the destination card lights up. The overview stays
open after the move so you can keep browsing or press Enter to follow the
window. Fullscreen windows stay on their current tag and reappear in
fullscreen when you return to it.

Existing configs can add the key and gesture to their respective tables.

```lua
-- In wasp.keys
{ mods = { "mod" }, key = "o", action = "overview" },
-- In wasp.gestures
{ fingers = 4, direction = "up", action = "overview" },
```

Each monitor has its own tags. If several tags are visible on the selected
monitor, the overview starts on the lowest-numbered tag. Dragging a window
replaces its source tag and preserves any other tags assigned to it.
Windows with `shield_when_capture` use solid thumbnails.

## Docs

- [`doc/DEVELOPMENT.md`](doc/DEVELOPMENT.md) — build instructions,
  full feature status, and how everything's been verified.
- [`CHANGELOG.md`](CHANGELOG.md) — changes in released versions.
- [`NOTES.md`](NOTES.md) — roadmap and implementation notes.
- [`doc/NOTES.md`](doc/NOTES.md) — historical upstream dwl README.

## Credit

wasp started as a fork of [dwl] (dwm for Wayland) and has grown well
past what dwl provides on its own — but the tiling core, the wlroots
integration, and plenty of the original design still trace back to dwl
and its contributors, led by Devin J. Pohly. The original dwl README is
preserved at [`doc/NOTES.md`](doc/NOTES.md) rather than overwritten.

## License

Two licenses, because this started as a fork, not a from-scratch
project:

- **`LICENSE` (GPLv3)** — dwl's own license, covering the files that
  are modifications of dwl's originals (the core compositor).
- **[`LICENSE.wasp`](LICENSE.wasp) (MIT)** — files wasp added that have
  no upstream dwl equivalent (this README, the Lua config layer,
  scripts, packaging, assets). Copyright Daniel Azevedo.

See [`doc/DEVELOPMENT.md`](doc/DEVELOPMENT.md#license) for the exact
file-by-file breakdown.

[dwl]: https://codeberg.org/dwl/dwl
[wlroots]: https://gitlab.freedesktop.org/wlroots/wlroots
[SceneFX]: https://github.com/wlrfx/scenefx
