<p align="center"><img src="assets/logo-icon.png" width="128" alt="wasp logo"></p>

<h1 align="center">wasp</h1>

<p align="center"><em>Fast · Light · Focused</em></p>

---

**wasp** is a Wayland compositor with a config you edit and reload live —
no recompiling, no restarting. Everything lives in
`~/.config/wasp/config.lua`: appearance, keybindings, gaps, animations,
rounded corners and blur, touchpad gestures, autostart, named
scratchpads, per-app window rules, and per-output settings. Save the
file, press a key, and it's live. dwm-style tiling (tile, monocle,
dwindle, floating) underneath, built on [wlroots] and [SceneFX], with
[dwl] as its original foundation.

## Features

- **Live Lua config, hot-reload** — most of what you'd want to tweak
  (appearance, gaps, animations, blur, keybindings, keyboard layout,
  output scale, the bar) applies the instant you reload, no restart.
- **Tiling layouts** — tile, monocle, dwindle (fibonacci/spiral), and
  floating, switchable per-monitor and per-tag.
- **Animations** — open/close/move/tag-switch tweening, fade or zoom,
  cubic-bezier easing, off by default.
- **Rounded corners + blur** — via SceneFX: window corner radius,
  per-window blur-behind, and background/wallpaper blur.
- **Touchpad gestures** — swipe actions (finger count + direction)
  wired through the same action table as keybindings.
- **Named scratchpads** — hidden, toggleable floating windows per slot
  (a drop-down terminal, a notes app, ...).
- **Per-window capture privacy** — flag a window to refuse being
  screen-shared/recorded on its own, and to blank out during any
  whole-screen recording.
- **Window & output rules** — send an app to a workspace, float it,
  center it, pin it to a monitor; per-output scale/layout/rotation.
- **Workspaces over `ext-workspace-v1`** — any compatible bar or shell
  can see and switch your workspaces, no wasp-specific integration
  needed.
- **A real status bar** — dwm-style, fed by any script over `stdin`.

## Install

**Void Linux**: a package template is included
([`packaging/void/template`](packaging/void/template)) for building
with `xbps-src`.

Other distros: build from source — see
[`doc/DEVELOPMENT.md`](doc/DEVELOPMENT.md) for dependencies and build
steps.

## Configure

```sh
mkdir -p ~/.config/wasp
cp examples/config.lua ~/.config/wasp/config.lua
```

[`examples/config.lua`](examples/config.lua) is both the default and the
full reference — every option, commented, ready to uncomment and tweak.
Edit it, reload (`mod+shift+r` by default), and most changes apply
immediately.

## Docs

- [`doc/DEVELOPMENT.md`](doc/DEVELOPMENT.md) — build instructions,
  full feature status, and how everything's been verified.
- [`NOTES.md`](NOTES.md) — the roadmap: what's done, what's next, and
  why things were built the way they were. Read alongside
  `doc/DEVELOPMENT.md` if you're building or contributing.

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
