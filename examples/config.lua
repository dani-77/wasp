-- wasp — example config, and the default until you copy it:
--   mkdir -p ~/.config/wasp && cp examples/config.lua ~/.config/wasp/config.lua
--
-- Appearance, terminal/menu, keybindings, keyboard, and gaps are all wired
-- up. Autostart, rules etc. land on top of this same `wasp` table later --
-- see NOTES.md for what's next.

wasp = {}

wasp.border = {
  width = 2,
  focus = "#7aa2f7",  -- focused window border
  normal = "#414868", -- unfocused window border
}

-- Gaps between/around tiled windows (tile/monocle/dwindle; floating is
-- untouched either way) — pixels. `inner` splits between two adjacent
-- windows, `outer` is the margin against the monitor's usable edges.
-- `smart`, if true, drops the outer gap when there's only one tiled window
-- on screen (same idea as spitfire's -- there it's implicit; here it's
-- opt-in since dwl's classic look has always kept edge gaps at 0).
wasp.gaps = {
  inner = 0,
  outer = 0,
  smart = false,
}

-- Keyboard layout (xkbcommon RMLVO fields) and repeat speed — same table
-- shape as spitfire.keyboard. Empty/omitted layout/variant/model/
-- options/rules mean "let xkbcommon pick its own default" (in practice
-- "us"). repeat_rate is repeats/second, repeat_delay is ms held before the
-- first repeat -- if typing ever feels like it drops in doubled letters,
-- raise repeat_delay rather than assuming a bug.
wasp.keyboard = {
  layout = "us",
  variant = "",
  model = "",
  options = "", -- e.g. "ctrl:nocaps"
  repeat_rate = 25,
  repeat_delay = 600,
}

wasp.bar = {
  enable = true,
  top = true,
  layout = "tln|s", -- t=tags l=layout-symbol n=window-name s=status, | splits left/right
}

wasp.background = "#11111bff"

-- Agnostic launchers: whatever's actually installed. Any argv works, e.g.
-- {"alacritty", "-e", "tmux"} or {"foot"} if you switch terminals later.
wasp.terminal = { "alacritty" }
wasp.menu = { "wmenu-run" }

-- What "mod" means below. One of "alt" | "ctrl" | "super" | "shift".
wasp.modkey = "alt"

-- Autostart -- one argv array per program, run once (fork+execvp, no
-- shell) right after startup, killed on exit. Uncomment/add whatever you
-- actually want running -- wasp has no opinion on wallpaper/idle/notifier
-- daemons etc., same as upstream dwl.
wasp.autostart = {
  -- { "swaybg", "-i", "/path/to/wallpaper.png" },
  -- { "mako" }, -- notifications
}

-- Keybindings ----------------------------------------------------------
-- Each entry: { mods = {...}, key = "<xkb keysym name>", action = "...",
--               <action-specific fields> }
--
-- `key` accepts anything libxkbcommon knows the name of: letters/digits
-- ("j", "1"), symbols ("comma", "exclam"), named keys ("Return", "Tab",
-- "space"), function/media keys ("F1", "XF86AudioRaiseVolume"), arrows
-- ("Left"/"Right"/"Up"/"Down"), and so on.
--
-- Actions and their fields:
--   spawn            cmd = {argv...}       run an arbitrary command
--   spawn-terminal    (none)               run wasp.terminal
--   spawn-menu        (none)               run wasp.menu
--   focusstack        dir = 1 | -1          next/prev window in stack
--   incnmaster        dir = 1 | -1          grow/shrink the master area's window count
--   setmfact          delta = <float>       grow/shrink the master area's size (+/-)
--   view              tag = 1..9 | "all" (omit = toggle back)   switch workspace
--   toggleview        tag = 1..9                                 also show workspace
--   tag               tag = 1..9 | "all" (omit = toggle back)   move focused window to workspace
--   toggletag         tag = 1..9                                 also tag focused window with workspace
--   focusmon          dir = "left" | "right"            focus other monitor
--   tagmon            dir = "left" | "right"            move focused window to other monitor
--   setlayout         layout = "tile"|"floating"|"monocle"|"dwindle" (omit to cycle)
--   togglefloating    (none)
--   togglefullscreen  (none)
--   togglebar         (none)
--   killclient        (none)
--   quit              (none)
--   chvt              vt = <number>         switch to a different virtual terminal
--   moveresizekb      dx = dy = dw = dh = <pixels>   nudge/resize the focused floating window
--   reload            (none)                re-read config.lua live -- see the note above wasp.autostart below for what this does/doesn't cover
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

-- Media keys — dedicated hardware keys, so no modifier needed (they can't
-- collide with anything text-related; `mods = {}` means "bare key").
-- Via ALSA (amixer); swap "Master" for whatever `amixer scontrols` lists
-- on your system if it differs, or for
-- "wpctl set-volume @DEFAULT_AUDIO_SINK@ 5%+-/mute-toggle" (PipeWire) /
-- "pactl set-sink-volume/set-sink-mute @DEFAULT_SINK@ ..." (PulseAudio via
-- pactl) if ALSA isn't what your system actually mixes through.
bind({}, "XF86AudioRaiseVolume", "spawn", { cmd = { "amixer", "-q", "set", "Master", "5%+" } })
bind({}, "XF86AudioLowerVolume", "spawn", { cmd = { "amixer", "-q", "set", "Master", "5%-" } })
bind({}, "XF86AudioMute",        "spawn", { cmd = { "amixer", "-q", "set", "Master", "toggle" } })

-- Window navigation
bind({ "mod" }, "j",    "focusstack", { dir = 1 })
bind({ "mod" }, "k",    "focusstack", { dir = -1 })
bind({ "mod" }, "Tab",  "view") -- go back to the previously selected tags
bind({ "mod" }, "Return", "zoom")  -- swap focused window into/out of master

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

-- Hot-reload -- re-reads this file and re-applies gaps, the bar's
-- visibility/position/colors, every window's border color, the
-- background, keyboard layout/repeat speed, and keybindings themselves,
-- all live, no restart. Border *width* on already-open windows and
-- wasp.autostart are the two things that still need a restart to pick up
-- (autostart deliberately only ever runs once, at real startup -- see
-- NOTES.md; otherwise every reload would relaunch everything in it).
bind({ "mod", "shift" }, "r", "reload")

-- VT switching (Ctrl-Alt-Fx) and Ctrl-Alt-Backspace, same as upstream dwl
for vt = 1, 12 do
  bind({ "ctrl", "alt" }, "XF86Switch_VT_" .. vt, "chvt", { vt = vt })
end
bind({ "ctrl", "alt" }, "Terminate_Server", "quit")
