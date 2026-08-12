-- wasp — example config, and the default until you copy it:
--   mkdir -p ~/.config/wasp && cp examples/config.lua ~/.config/wasp/config.lua
--
-- This is an early skeleton: only appearance (border/bar/background) is
-- wired up so far. Keybindings, autostart, gaps, etc. land on top of this
-- same `wasp` table as luaconfig.c grows — see NOTES.md for what's next.

wasp = {}

wasp.border = {
  width = 2,
  focus = "#7aa2f7",  -- focused window border
  normal = "#414868", -- unfocused window border
}

wasp.bar = {
  enable = true,
  top = true,
  layout = "tln|s", -- t=tags l=layout-symbol n=window-name s=status, | splits left/right
}

wasp.background = "#11111bff"
