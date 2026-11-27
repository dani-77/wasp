# wasp — development docs

Wasp is a [dwl] fork with Lua configuration and live reload. See the
[README](../README.md) for installation and day-to-day use and the
[working notes](../NOTES.md) for the roadmap and implementation history.

## Features

- **Forked from dwl `main`**, full upstream git history kept.
- **Bar**: the [dwl-patches] `bar` + `barconfig` patches applied and
  verified live — a real dwm-style status bar (text via fcft/pixman/tllist),
  fed status text over `stdin` the classic dwl way (`your-script | wasp`).
  `scripts/statusbar.sh` is a ready-to-use one (CPU/RAM/volume/battery/
  clock) — without something feeding it, the bar just shows its startup
  placeholder text forever, it has no built-in widgets of its own.
- **Lua config and appearance** — wasp embeds Lua 5.4 and loads
  `$XDG_CONFIG_HOME/wasp/config.lua` or `~/.config/wasp/config.lua` at
  startup. See the [example](../examples/config.lua) for borders, the
  background, and bar visibility, positioning and layout.
- **Keybindings**: fully Lua-driven — `wasp.keys` in `config.lua` builds the
  keymap at startup, `wasp.modkey` picks the primary modifier, and
  `wasp.terminal`/`wasp.menu` make the launched terminal/menu agnostic
  (no more hardcoded `foot`). Suggested bindings for workspace switching,
  window/monitor navigation, resize (master-area and keyboard
  move/resize), and layout cycling (tile/floating/monocle/dwindle) ship in
  `examples/config.lua`; a small built-in fallback keymap keeps you from
  ever being locked out if `config.lua` is missing or broken.
  Besides the stack-order `focusstack`/`movestack`, `focusdir`/`swapdir`
  (`dir = "left"|"right"|"up"|"down"`) move focus to / swap with the
  neighbouring tiled window spatially (centre past the focused one's, and
  overlapping on the other axis), independent of layout. An entry in
  `wasp.keys`/`wasp.gestures`/`wasp.wheel` with an unknown key, action or
  direction is skipped with a warning on stderr. A unit harness verified
  spatial actions with a 2×2 grid and floating and hidden clients.
- **Layouts**: tile, floating, monocle (upstream dwl), plus `dwindle`
  (fibonacci/spiral tiling, adapted from [dwl-patches]). The tile, monocle
  and dwindle layout functions take a tag mask and report window boxes
  through a placement callback. The regular arrange path applies those
  boxes and updates the monocle symbol and stacking order.
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
- **Hot reload** — the `reload` action or SIGHUP reloads the config.
  See [live reload](#live-reload) for how settings take effect.
- **Named scratchpads**: `wasp.scratchpad = { { name=, cmd=, app_id=, w=,
  h= }, ... }` — a hidden, toggleable floating window per slot, spawned
  the first time it's toggled (bound via the `toggle-scratchpad` action)
  and hidden/shown (not killed/respawned) on every toggle after that.
- **Window rules**: `wasp.rules = { { app_id=, title=, tags=, floating=,
  monitor=, center=, shield_when_capture= }, ... }` — send specific apps
  to a workspace, make them open floating, pin them to a monitor, or
  re-center them at their own requested size instead of wherever they
  happened to want to place themselves (often the top-left corner).
  `shield_when_capture` refuses individual capture requests through
  `ext-image-copy-capture-v1`. An output capture session using that
  protocol blanks visible protected windows in the compositor until the
  last such session ends. The overview shields those windows in cards
  and drag thumbnails.
- **Workspaces exposed over `ext-workspace-v1`**: one workspace group per
  monitor, one workspace per tag inside it — so any bar/shell that speaks
  the protocol (not a dwl/wasp-specific thing to build against) sees and
  can switch tags, something dwl had no story for at all before this.
  Verified end-to-end with a standalone protocol client, not just by
  inspection.
- **Occupied tags for the d77 shells**: ext-workspace-v1 has no
  "occupied" state, so wasp (like whiplash and spitfire) keeps
  `$XDG_RUNTIME_DIR/d77-workspaces-$WAYLAND_DISPLAY`, one line per
  workspace, `<ext-workspace id> <window count>`, with the ids
  `createmon()` gives the handles (`<output>-<tag>`). Utumno,
  quickshell-d77 (archived; superseded by Utumno), fabric-d77 and helium-d77 read it to dim empty tags.
  `publishworkspaces()` rewrites it from `drawbars()` only when it
  changes (written beside it and renamed); the scratchpad tag is not
  counted; `cleanup()` removes it and stops further writes, since
  destroying the clients there redraws the bars.
- **Per-output rules and live scale**: `wasp.monitors = { { name=,
  mfact=, nmaster=, scale=, layout=, transform=, x=, y= }, ... }` — same
  idea as `wasp.rules`, but for outputs instead of clients (a monitor
  uses the first matching rule, matching upstream dwl's own `monrules[]`
  semantics). `scale` is live — a bound key (`mod+shift+p`/`m`, same
  convention as spitfire) nudges it up/down at runtime, and it's the one
  field `reload` re-applies. Other output rules apply when the output is
  created.
- **D-Bus session + xdg-desktop-portal**: `wasp-session` (what `wasp.desktop`
  actually launches) wraps startup in `dbus-run-session` and exports
  `XDG_CURRENT_DESKTOP=wasp`, so every client — the bar's status pipe,
  `wasp.autostart`, anything you launch by hand — gets a real D-Bus
  session bus from the first process on, instead of falling back to an
  ad-hoc one per app (or none at all). `packaging/wasp-portals.conf`
  (installed to `/etc/xdg-desktop-portal/`) picks `gtk` for
  FileChooser/Settings/Notification/Print, `wlr` for ScreenCast/Screenshot,
  and `gnome-keyring` for Secret. See [Building](#building) for the
  runtime packages.
- **Animations**: `wasp.animations = { enable, duration_move, duration_open,
  duration_close, duration_tag, type_open, type_close, zoom_ratio,
  fade_from_opacity, tag_direction, curve_move, curve_open, curve_close,
  curve_tag }` — MangoWC-style open/close/move/tag-switch tweening
  (`"fade"`/`"zoom"`/`"none"`, cubic-bezier easing per action). `enable =
  false` (the default) applies geometry immediately. Tag animations
  slide windows in and out, with immediate visibility changes for the
  fullscreen layer. See [the working notes](../NOTES.md) for the
  animation design.
- **Touchpad gestures**: `wasp.gestures = { { fingers=, direction=,
  action=, ... }, ... }` — swipe gestures dispatched through the exact same
  action/arg table as `wasp.keys` (`focusmon`, `view`, `spawn`, ... all work
  here too), matched by finger count (0/omitted = any) and direction
  (`"left"`/`"right"`/`"up"`/`"down"`, classified from the swipe's dominant
  axis once it ends).
- **Mouse wheel bindings** — `wasp.wheel` maps physical wheel notches and
  modifiers to the same actions as `wasp.keys`.
- **Screen capture** — `ext-image-copy-capture-v1`,
  `ext-image-capture-source-v1` and `ext-foreign-toplevel-list-v1` support
  output and individual window capture. Legacy `wlr-screencopy` and
  `wlr-export-dmabuf` remain available for whole-output capture. Wasp's
  window shielding during output capture tracks sessions opened through
  `ext-image-copy-capture-v1`.
- **`wasp-list-windows`** — lists the capture identifier, app ID and title
  for managed toplevels. `-a <app_id-substring>` prints matching
  identifiers. Choose a single identifier when using a `grim` build
  that supports `-T`.

  ```sh
  wasp-list-windows
  grim -T "<identifier>" window.png
  ```

  Protected windows appear in the list. Wasp refuses individual capture
  requests for their identifiers.
- **Rounded corners + blur**: via [SceneFX](https://github.com/wlrfx/scenefx),
  a drop-in replacement for wlroots' own scene API — `wasp.border.radius`
  (same table as `width`/`focus`/`normal`, matching spitfire's own
  `spitfire.border.radius`) rounds both the border and window content
  (fullscreen always forces square corners); `wasp.blur = { enable,
  radius, passes, noise, brightness, contrast, saturation }` adds both
  background/wallpaper blur and per-window blur-behind (through a
  translucent window's own transparency). Both default off, reproducing
  today's exact rendering bit-for-bit. Verified against XWayland clients
  too, not just XDG — same rounding, no gap.

Not done yet: mouse-drag resize variety and drop shadows. The full
running list, plus which [dwl-patches] are earmarked for which feature and
why, lives in [`NOTES.md`](../NOTES.md) — that's the file to check for
current plans, not this doc.

## Workspace overview

The example config binds `overview` to Mod+O and an upward four-finger
swipe through `wasp.keys` and `wasp.gestures`, respectively. The action
tracks a preview tag on the selected output. Enter and click call the
normal tag activation path. Dropping a window replaces its source tag
bit and keeps any other tags assigned to that window. Hidden scratchpads
stay off the cards, and fullscreen windows keep their original tag.
Switching tags changes fullscreen-layer visibility immediately, including
dialogs on that layer. Fullscreen and floating geometry stay with the
client when its tag is hidden.

SceneFX draws the cards within the output's logical usable area, leaving
reserved panel space available. A preview whose tag mask matches the
active mask uses current window placements. Other previews use the
monitor's layout callback with their own tag mask. Floating geometry and fullscreen state come from
the existing clients. The scene layer takes the latest surface buffers,
including subsurfaces, and clips each thumbnail to its card and output.
It preserves buffer transforms, source cropping, rounded corners and
surface opacity. Explicitly synchronized buffers carry their acquire
fences into the preview, whose render samples register release fences.
The previews contain managed toplevels and their subsurfaces. Once a drag
starts, a separate thumbnail follows the pointer within the output's usable
area. It uses the same buffer and shielding rules as the cards. Pointer
motion moves its SceneFX subtree and recolors the destination outline. Drop
hit testing still uses the cards beneath the thumbnail.

Rendering and hit testing share the same scene nodes. Browsing leaves
tag visibility and workspace protocol state alone. The overview consumes
window input while open and releases its preview buffers on dismissal.
Session lock and removal or power-off of the selected output dismiss it.
Protected previews use solid rectangles and never reference the clients'
buffers, including during output capture.

Headless SceneFX checks covered Wayland and XWayland client previews.
Keyboard and pointer tests covered cancellation, activation, scrolling,
cross-tag drops, fullscreen and floating geometry, capture shielding, and
session lock. Screenshots taken mid-drag verified that the thumbnail moved
with the pointer and the target card remained highlighted. A two-output
headless run verified overview and fullscreen output isolation. The
overview also passed a hardware test in a separate Wasp session. The
compositor builds without XWayland.

## Building

Build dependencies include a C compiler, make, pkg-config and the
following libraries and protocol definitions.

- libinput, Wayland (server, client and scanner), wayland-protocols,
  xkbcommon and **wlroots 0.20** (`wlroots-0.20`).
- [SceneFX](https://github.com/wlrfx/scenefx) **0.5** (`scenefx-0.5`)
  built against wlroots 0.20. Wasp uses it for rendering, including
  thumbnails, rounded corners and blur.
- fcft, pixman and the tllist headers for the bar.
- Lua 5.4 (`lua5.4`, or `lua54-devel` on Void Linux).
- libxcb and xcb-icccm, plus wlroots with XWayland support, for the
  default build. Xwayland is also a runtime dependency for X11 clients.

`make` builds the compositor and the `wasp-list-windows` Wayland client.
For a build without XWayland, set `XWAYLAND=` and `XLIBS=` in `config.mk`
or pass both assignments to `make` and `make install`. Use a clean build
when changing those flags.

```sh
make
sudo make install
```

`make install` uses `/usr/local` as its default prefix and installs the
session entry, programs and example config. `PREFIX` sets the installation
prefix and `DESTDIR` stages the files under a separate root. The portal
preferences are installed to `/etc/xdg-desktop-portal/wasp-portals.conf`
inside that root.

The `wasp-session` wrapper uses `dbus-run-session`. Install the GTK and
wlr backends alongside `xdg-desktop-portal` to enable the configured
portals. The Secret portal preference uses GNOME Keyring. Void and Arch
build recipes are available under [`packaging/`](../packaging/).

## Configuring

Copy [`examples/config.lua`](../examples/config.lua) to
`${XDG_CONFIG_HOME:-$HOME/.config}/wasp/config.lua`. Installed copies
live under `$PREFIX/share/wasp/config.lua`, typically
`/usr/local/share/wasp/config.lua` for a source build and
`/usr/share/wasp/config.lua` for a package. The compositor uses its
compiled defaults when the user config is missing.

### Live reload

The example binds **Mod+Shift+R** to `reload`. SIGHUP triggers the same
action. A syntax or runtime error keeps the last successfully loaded config
and reports the error on stderr. The session wrapper writes stderr to
`$XDG_STATE_HOME/wasp/wasp.log` or `~/.local/state/wasp/wasp.log`. When
`notify-send` and a notification daemon are available, the error also
appears as a desktop notification.

- **On reload** — keys, gestures, wheel bindings, gaps, snap distance,
  animation settings, blur parameters for existing blur nodes, bar
  settings, border colours, background, keyboard settings and output
  scale.
- **On window or output creation** — window rules and border width apply
  to newly mapped windows. Corner radius updates on a geometry update.
  The blur toggle controls blur nodes on newly mapped windows and newly
  created outputs. Output rules other than scale apply when the output
  is created.
- **On session startup** — `wasp.autostart` launches its commands once.

### Status bar input

The built-in bar reads right-side status text from standard input. The
`wasp.desktop` entry runs `wasp-session`, which pipes `wasp-statusbar`
into the compositor. To use the same status script from a checkout, run
the compositor with the script connected at startup.

```sh
scripts/statusbar.sh | ./wasp
```

## Credit

wasp is a fork — nearly everything here is [dwl]'s work, not wasp's own.
The original dwl README (build details, community links, project
philosophy, acknowledgements to Devin J. Pohly and the rest of the dwl
contributors) is preserved at [`NOTES.md`](NOTES.md) (this directory's
own, not the root one) rather than overwritten.

## License

Two licenses, because this is a fork, not a from-scratch project:

- **`LICENSE` (GPLv3)** — dwl's own license. `wasp.c`, `config.def.h`,
  `config.mk`, `Makefile`, `client.h`, `util.c`, `util.h`, `wasp.1`, and
  `protocols/*` are all modifications of dwl's original files, so they (and
  the compiled `wasp` binary as a whole, since it links GPLv3 code) stay
  under GPLv3 — that's simply a requirement of the license they were under
  before wasp touched them, not a choice wasp gets to make.
- **[`LICENSE.wasp`](../LICENSE.wasp) (MIT)** — files wasp added that have
  no upstream dwl equivalent and aren't derived from it: `README.md`,
  `doc/DEVELOPMENT.md`, `NOTES.md`, `luaconfig.c`/`.h`,
  `examples/config.lua`, `scripts/*`, `packaging/*`, `wasp.desktop`, and
  `assets/*`. Copyright Daniel Azevedo.

[dwl]: https://codeberg.org/dwl/dwl
[dwl-patches]: https://codeberg.org/dwl/dwl-patches
