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
  structs, reload on file-watch or a bound key).
- Built-in toggleable status bar (`wasp.bar = { enable, height }` or
  similar). **Decided (2026-08-12)**: base it on dwl-patches' `bar` +
  `barconfig` rather than porting spitfire's bitmap-font approach — it's
  mature, pre-implemented, real text rendering (fcft/pixman/tllist), and a
  more solid starting point than reinventing font rendering from scratch.
  Still needs adapting: `barlayout`/colors/enable-toggle need to move from
  static `config.h` values to the live `config.lua`-driven config.

## Reference patches to adapt (not apply as-is)
- **hot-reload** — reference for what *not* to copy: its `dlopen`'d `.so` +
  `HOT`/cold-part macro split is solving "don't recompile" via a mechanism
  we won't need once config is Lua-driven and grabs a wide keybind superset.
  Skip the patch itself; the goal is already covered by the Lua design.
- **bar** / **barconfig** / **barborder** / **bar-notitle** / etc. — bar
  family, see "Decided" above.
- **gaps** / **vanitygaps** / **genericgaps** — inner/outer gaps, adjustable
  live from `config.lua`.
- **dragresize** / **moveresizekb** / **better-resize** — resize behavior
  (mouse-drag and keyboard).
- **borders** / **smartborders** / **simpleborders** — window border
  rendering/behavior (colors should come from `config.lua`, same as
  spitfire's `spitfire.border`).
- **autostart** — but embedded as a `wasp.autostart({...})`-style call in
  `config.lua` (like spitfire), not a static C array in `config.h`.

## Not yet decided / just flagged
- **Status text source — decided (2026-08-12), option (a)**: keep dwl's
  classic stdin-pipe model for the bar's right-side status text (`stext`,
  `dwl.c:3225`/`:2965`) rather than porting `pwm`/`spitfire`'s native
  in-process widgets into C. `config.lua`'s `autostart` spawns whatever
  status-generator script pipes lines into dwl's `stdin` (à la
  `slstatus -s | dwl`). Daniel's usual widget set/order, confirmed live: CPU,
  RAM, Audio (volume), Battery, Time/Date — see the throwaway test script at
  the bottom of a Bash session on 2026-08-12 for a working shape (top+free
  for CPU/RAM, `pactl get-sink-volume` for volume, `/sys/class/power_supply/
  BAT1` for battery, `date` for clock) as a starting point for the real
  status script. Battery path must be detected, not hardcoded (`BAT1` on
  this laptop, but `BAT0` on others) — iterate `/sys/class/power_supply/`
  for the first entry starting with `BAT`, same pattern already proven in
  both `pwm` (`battery_file_search()`/`bar.rs`) and `spitfire`
  (`read_battery_status()` in `bar.rs`) — just reuse that logic.
