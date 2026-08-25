-- wasp — example config, and the default until you copy it:
--   mkdir -p ~/.config/wasp && cp examples/config.lua ~/.config/wasp/config.lua
--
-- Workspaces show up in any ext-workspace-v1-aware bar/shell for free, no
-- config needed for that part. See NOTES.md for what's still coming.

wasp = {}

-- Appearance ---------------------------------------------------------
wasp.border = {
  width = 2,
  focus = "#7aa2f7",
  normal = "#414868",
  radius = 0, -- fullscreen windows always get square corners regardless
}

-- Background/wallpaper blur and per-window blur-behind (scenefx). enable
-- = false reproduces today's exact rendering, no blur node ever created.
-- Global only — one look for the whole session, no per-window override.
-- Picked up live on reload for a blur node that already exists; enable
-- itself flipping on/off only affects windows mapped after the reload.
wasp.blur = {
  enable = false,
  radius = 5,
  passes = 3,
  noise = 0.02,
  brightness = 0.9,
  contrast = 0.9,
  saturation = 1.1,
}

-- inner: between adjacent tiled windows. outer: margin against the
-- monitor's usable edges. smart: drop the outer gap with one tiled window.
wasp.gaps = {
  inner = 0,
  outer = 0,
  smart = false,
}

-- Open/close/move/tag-switch tweening on top of the wlr_scene graph.
-- enable = false reproduces the original instant behavior bit-for-bit.
-- type_open/type_close: "fade" | "zoom" | "none". tag_direction is which
-- monitor edge tag-switches slide to/from. curve_* are CSS
-- cubic-bezier()-style control points {x1, y1, x2, y2} (endpoints pinned
-- at (0,0)/(1,1)) — see e.g. https://cubic-bezier.com to pick one.
wasp.animations = {
  enable = false,
  duration_move = 200,
  duration_open = 200,
  duration_close = 150,
  duration_tag = 200,
  type_open = "zoom",
  type_close = "zoom",
  zoom_ratio = 0.8,
  fade_from_opacity = 0.0,
  tag_direction = "right",
  curve_move  = { 0.25, 0.1, 0.25, 1.0 },
  curve_open  = { 0.25, 0.1, 0.25, 1.0 },
  curve_close = { 0.25, 0.1, 0.25, 1.0 },
  curve_tag   = { 0.25, 0.1, 0.25, 1.0 },
}

-- Inputs -------------------------------------------------------------
-- Per-output rules: mfact/nmaster/layout starting values, scale, rotation,
-- position. name matched as a substring against the output's own name
-- (wlr-randr); omit for a fallback rule. A monitor uses the *first* rule
-- that matches, not every one that does. transform: "normal" | "90" |
-- "180" | "270" | "flipped" | "flipped-90" | "flipped-180" | "flipped-270".
-- x/y: layout position in pixels, -1 = auto-place.
--
-- scale applies right at startup (set it here instead of pressing
-- mod+shift+p every session) and is also the one field the "setscale"
-- action and reload keep live.
wasp.monitors = {
  -- { name = "eDP-1", scale = 1.25 }, -- example: a HiDPI laptop panel
  { mfact = 0.55, nmaster = 1, scale = 1, layout = "tile", x = -1, y = -1 },
}

-- xkbcommon RMLVO fields, same shape as spitfire.keyboard. Empty/omitted
-- fields mean "let xkbcommon pick its own default" (in practice "us").
-- repeat_rate: repeats/second. repeat_delay: ms held before the first
-- repeat — raise it if typing ever feels like it drops in doubled letters.
wasp.keyboard = {
  layout = "us",
  variant = "",
  model = "",
  options = "", -- e.g. "ctrl:nocaps"
  repeat_rate = 25,
  repeat_delay = 600,
}

-- Bar & background -----------------------------------------------------
wasp.bar = {
  enable = true,
  top = true,
  layout = "tln|s", -- t=tags l=layout-symbol n=window-name s=status, | splits left/right
}
wasp.background = "#11111bff"

-- Terminal / menu ------------------------------------------------------
-- Agnostic launchers: whatever's actually installed. Any argv works.
wasp.terminal = { "alacritty" }
wasp.menu = { "wmenu-run" }

-- What "mod" means below. One of "alt" | "ctrl" | "super" | "shift".
wasp.modkey = "alt"

-- Autostart --------------------------------------------------------------
-- One argv array per program, run once (fork+execvp, no shell) at
-- startup, killed on exit. wasp has no opinion on wallpaper/idle/notifier/
-- polkit daemons, same as upstream dwl. Screen-sharing/screenshot via
-- xdg-desktop-portal-wlr needs PipeWire running to transport frames.
wasp.autostart = {
  -- { "swaybg", "-i", "/path/to/wallpaper.png" },
  -- { "mako" }, -- notifications
  -- { "pipewire" },
  -- A polkit agent — without one, anything asking for privilege
  -- escalation (NetworkManager's GUI, mounting removable drives, ...)
  -- just fails silently on Wayland. Pick whichever your DE already ships
  -- (polkit-gnome/polkit-kde/lxqt-policykit/xfce-polkit/polkit-mate/...):
  -- { "/usr/libexec/polkit-mate-authentication-agent-1" },
}

-- Scratchpad -------------------------------------------------------------
-- A hidden, toggleable floating window per slot. Toggling an empty slot
-- spawns cmd; toggling again hides/shows that same window rather than
-- killing and respawning it. app_id is what wasp matches the freshly-
-- spawned window against (defaults to name — set explicitly if cmd
-- doesn't open with a matching --class/--app-id). w/h: fraction of the
-- monitor's usable area when shown (default 0.6 each). Bound via the
-- "toggle-scratchpad" action below.
local function terminal_with(...)
  local argv = {}
  for i, a in ipairs(wasp.terminal) do argv[i] = a end
  for _, a in ipairs({ ... }) do argv[#argv + 1] = a end
  return argv
end

wasp.scratchpad = {
  -- { name = "term", cmd = terminal_with("--class", "scratch-term"),
  --   app_id = "scratch-term", w = 0.6, h = 0.6 },
}

-- Window rules -----------------------------------------------------------
-- Matched by app_id/title (substring against the client's own, either
-- field omittable). tags (1-9) sends it to a workspace; floating takes it
-- out of tiling; monitor (0-based) forces which output it opens on;
-- center re-centers a floating window at its own requested size. A client
-- matching more than one rule gets every match's tags combined, but only
-- the *last* match's floating/monitor/center/shield_when_capture. Applied
-- once, at creation — reloading affects the next window that app opens,
-- not ones already on screen.
--
-- shield_when_capture = true refuses this window's own single-window
-- capture requests outright, and swaps its content for a solid rect for
-- the duration of any whole-output capture (recording/share/grim) — you
-- see the blank too the whole time, a visible cue rather than a hidden
-- swap. Doesn't affect other windows' own capture. To capture a single
-- window from the CLI (grim -T <id>), see wasp-list-windows.
--
-- Some GTK/GLib apps' app_id isn't fixed — they can fall back to their raw
-- GApplication id if D-Bus single-instance registration fails (e.g. no
-- DBUS_SESSION_BUS_ADDRESS). If a rule stops matching, check with
-- WAYLAND_DEBUG=1 <app> 2>&1 | grep set_app_id before assuming wasp broke.
wasp.rules = {
  -- { app_id = "org.gimp.GIMP", floating = true },
  -- { app_id = "firefox", tags = 9 },
  -- { app_id = "some-launcher", floating = true, center = true },
  -- { app_id = "org.keepassxc.KeePassXC", shield_when_capture = true },
}

-- Keybindings ----------------------------------------------------------
-- Each entry: { mods = {...}, key = "<xkb keysym name>", action = "...",
--               <action-specific fields> }
--
-- key accepts anything libxkbcommon knows the name of: letters/digits
-- ("j", "1"), symbols ("comma", "exclam"), named keys ("Return", "space"),
-- function/media keys ("F1", "XF86AudioRaiseVolume"), arrows, etc.
--
-- Actions and their fields:
--   spawn            cmd = {argv...}       run an arbitrary command
--   spawn-terminal    (none)               run wasp.terminal
--   spawn-menu        (none)               run wasp.menu
--   focusstack        dir = 1 | -1          next/prev window in stack
--   movestack         dir = 1 | -1          swap focused window with next/prev
--   zoom              (none)                swap focused window into/out of master
--   incnmaster        dir = 1 | -1          grow/shrink master area's window count
--   setmfact          delta = <float>       grow/shrink master area's size
--   view              tag = 1..9 | "all" (omit = toggle back)   switch workspace
--   viewshift         dir = 1 | -1          next/previous workspace (wraps)
--   toggleview        tag = 1..9                                 also show workspace
--   tag               tag = 1..9 | "all" (omit = toggle back)   move focused window
--   toggletag         tag = 1..9                                 also tag focused window
--   focusmon/tagmon    dir = "left" | "right"           focus / move window to other monitor
--   setlayout         layout = "tile"|"floating"|"monocle"|"dwindle" (omit = cycle)
--   setscale          delta = <float>       grow/shrink focused monitor's scale, clamped 0.25-4.0
--   togglefloating / togglefullscreen / togglebar / killclient / quit  (none)
--   chvt              vt = <number>         switch virtual terminal
--   moveresizekb      dx = dy = dw = dh = <pixels>   nudge/resize focused floating window
--   reload            (none)                re-read config.lua live
--   toggle-scratchpad name = "<slot name>"   spawn/show/hide a wasp.scratchpad slot
wasp.keys = {}
local keys = wasp.keys

local function bind(mods, key, action, fields)
  fields = fields or {}
  fields.mods, fields.key, fields.action = mods, key, action
  keys[#keys + 1] = fields
end

-- Launchers
bind({ "mod", "shift" }, "Return", "spawn-terminal")
bind({ "mod" },          "p",      "spawn-menu")
-- A second launcher on a different modifier so it doesn't compete with mod+<key>:
-- bind({ "super" }, "r", "spawn", { cmd = { "d77run" } })

-- Media keys (bare, no modifier — dedicated hardware keys). Via ALSA
-- (amixer); swap for wpctl (PipeWire) or pactl (PulseAudio) if that's not
-- what your system mixes through.
bind({}, "XF86AudioRaiseVolume", "spawn", { cmd = { "amixer", "-q", "set", "Master", "5%+" } })
bind({}, "XF86AudioLowerVolume", "spawn", { cmd = { "amixer", "-q", "set", "Master", "5%-" } })
bind({}, "XF86AudioMute",        "spawn", { cmd = { "amixer", "-q", "set", "Master", "toggle" } })

-- Screenshot (grim) — whole output, saved to ~/screenshot-<timestamp>.png.
-- Wrapped in sh -c since spawn() execvp()s directly (no shell expansion).
-- Region-select needs slurp too:
-- { "sh", "-c", 'grim -g "$(slurp)" ~/screenshot-$(date +%Y%m%d-%H%M%S).png' }
bind({}, "Print", "spawn", { cmd = { "sh", "-c", "grim ~/screenshot-$(date +%Y%m%d-%H%M%S).png" } })

-- Window navigation
bind({ "mod" }, "j",    "focusstack", { dir = 1 })
bind({ "mod" }, "k",    "focusstack", { dir = -1 })
bind({ "mod" }, "Tab",  "view") -- go back to the previously selected tags
bind({ "mod" }, "Return", "zoom")
bind({ "mod", "shift" }, "j", "movestack", { dir = 1 })
bind({ "mod", "shift" }, "k", "movestack", { dir = -1 })

-- Scratchpad — pairs with the wasp.scratchpad slot above; uncomment both
-- together. Backtick/grave is the common drop-down-terminal convention.
-- bind({ "mod" }, "grave", "toggle-scratchpad", { name = "term" })

-- Resize
bind({ "mod" }, "i", "incnmaster", { dir = 1 })
bind({ "mod" }, "d", "incnmaster", { dir = -1 })
bind({ "mod" }, "h", "setmfact",   { delta = -0.05 })
bind({ "mod" }, "l", "setmfact",   { delta = 0.05 })
bind({ "mod" },          "Left",  "moveresizekb", { dx = -40 })
bind({ "mod" },          "Right", "moveresizekb", { dx = 40 })
bind({ "mod" },          "Up",    "moveresizekb", { dy = -40 })
bind({ "mod" },          "Down",  "moveresizekb", { dy = 40 })
bind({ "mod", "shift" }, "Left",  "moveresizekb", { dw = -40 })
bind({ "mod", "shift" }, "Right", "moveresizekb", { dw = 40 })
bind({ "mod", "shift" }, "Up",    "moveresizekb", { dh = -40 })
bind({ "mod", "shift" }, "Down",  "moveresizekb", { dh = 40 })

-- Layouts
bind({ "mod" }, "t", "setlayout", { layout = "tile" })
bind({ "mod" }, "f", "setlayout", { layout = "floating" })
bind({ "mod" }, "m", "setlayout", { layout = "monocle" })
bind({ "mod" }, "r", "setlayout", { layout = "dwindle" })
bind({ "mod" }, "space", "setlayout") -- cycle
bind({ "mod", "shift" }, "space", "togglefloating")
bind({ "mod" }, "e", "togglefullscreen")
bind({ "mod" }, "b", "togglebar")

-- Monitor navigation
bind({ "mod" },          "comma",  "focusmon", { dir = "left" })
bind({ "mod" },          "period", "focusmon", { dir = "right" })
bind({ "mod", "shift" }, "less",    "tagmon",   { dir = "left" })
bind({ "mod", "shift" }, "greater", "tagmon",   { dir = "right" })

-- Live output-scale inc/dec — same Mod+Shift+P/M as spitfire, so muscle
-- memory carries over between the two.
bind({ "mod", "shift" }, "p", "setscale", { delta = 0.25 })
bind({ "mod", "shift" }, "m", "setscale", { delta = -0.25 })

-- Workspaces (tags) 1-9: switch/also-show/move-window/also-tag-window
local tagkeys = { "1", "2", "3", "4", "5", "6", "7", "8", "9" }
local tagshiftkeys = { "exclam", "at", "numbersign", "dollar", "percent",
                        "asciicircum", "ampersand", "asterisk", "parenleft" }
for i, key in ipairs(tagkeys) do
  bind({ "mod" },                     key,              "view",       { tag = i })
  bind({ "mod", "ctrl" },             key,              "toggleview", { tag = i })
  bind({ "mod", "shift" },            tagshiftkeys[i],  "tag",        { tag = i })
  bind({ "mod", "ctrl", "shift" },    tagshiftkeys[i],  "toggletag",  { tag = i })
end
bind({ "mod" },          "0",           "view", { tag = "all" })
bind({ "mod", "shift" }, "parenright",  "tag",  { tag = "all" })

-- Window/session control
bind({ "mod", "shift" }, "c", "killclient")
bind({ "mod", "shift" }, "q", "quit")

-- Hot-reload — re-reads this file and re-applies gaps, animations
-- (re-baking curve_* easing tables), blur's look params on any blur node
-- that already exists, the bar, every window's border color, background,
-- keyboard layout/repeat speed, monitors' scale (only that field —
-- mfact/nmaster/layout/transform/x/y stay startup-time only), and
-- keybindings, all live. Border width/radius on already-open windows,
-- blur.enable flipping on/off, and autostart still need a restart (or a
-- window's next real geometry change, for border width/radius).
bind({ "mod", "shift" }, "r", "reload")

-- VT switching (Ctrl-Alt-Fx) and Ctrl-Alt-Backspace, same as upstream dwl
for vt = 1, 12 do
  bind({ "ctrl", "alt" }, "XF86Switch_VT_" .. vt, "chvt", { vt = vt })
end
bind({ "ctrl", "alt" }, "Terminate_Server", "quit")

-- Touchpad gestures ------------------------------------------------------
-- { fingers = <count, omit/0 = any>, direction = "left"|"right"|"up"|"down",
--   action = "...", <action-specific fields> } — same action table as
-- wasp.keys above. direction is classified from the swipe's dominant axis
-- once it ends. Swipe only for now (no pinch/hold) — see NOTES.md item 8.
wasp.gestures = {
  -- 3-finger left/right cycles focus between windows (same as Mod+j/k).
  -- { fingers = 3, direction = "left",  action = "focusstack", dir = -1 },
  -- { fingers = 3, direction = "right", action = "focusstack", dir = 1 },
  -- 4-finger up/down for fullscreen toggle.
  -- { fingers = 4, direction = "up",    action = "togglefullscreen" },
  -- 4-finger left/right — next/previous workspace, wrapping (9 -> 1, 1 -> 9).
  -- { fingers = 4, direction = "left",  action = "viewshift", dir = -1 },
  -- { fingers = 4, direction = "right", action = "viewshift", dir = 1 },
  -- Alternative to the 3-finger set above: focus between monitors instead
  -- (useful with 2+ monitors — pick one or the other, not both, since
  -- they'd otherwise fight over the same fingers/direction combo):
  -- { fingers = 3, direction = "left",  action = "focusmon", dir = "left" },
  -- { fingers = 3, direction = "right", action = "focusmon", dir = "right" },
}
