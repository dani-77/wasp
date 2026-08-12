/* Lua-driven configuration.
 *
 * These are the same globals dwl.c has always read from config.h — same
 * names, same types (minus `static const`) — just no longer baked in at
 * compile time. waspconfig_load() populates them from
 * ~/.config/wasp/config.lua (or $XDG_CONFIG_HOME/wasp/config.lua) at
 * startup, falling back to the exact values config.def.h used to hardcode
 * for anything the file doesn't set or if it's missing/fails to parse.
 *
 * Keeping the names/types identical to what config.def.h declared is
 * deliberate: every existing dwl.c call site (colors[SchemeSel], borderpx,
 * rootcolor, ...) keeps compiling completely unchanged. Only config.def.h
 * (which no longer declares these) and dwl.c's `main` (which now calls
 * waspconfig_load() before setup()) needed to change.
 */
#ifndef WASP_LUACONFIG_H
#define WASP_LUACONFIG_H

#include <stdint.h>

extern unsigned int borderpx;
extern int showbar;
extern int topbar;
extern const char *barlayout;
extern float rootcolor[4];
extern uint32_t colors[3][3];

/* Loads (or reloads) the config, overwriting the globals above in place.
 * Safe to call again later for a hot-reload once callers redraw/rearrange
 * afterwards -- nothing here restarts the compositor. */
void waspconfig_load(void);

#endif
