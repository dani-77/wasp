.POSIX:
.SUFFIXES:

include config.mk

# flags for compiling
DWLCPPFLAGS = -I. -DWLR_USE_UNSTABLE -D_POSIX_C_SOURCE=200809L \
	-DVERSION=\"$(VERSION)\" $(XWAYLAND)
DWLDEVCFLAGS = -g -Wpedantic -Wall -Wextra -Wdeclaration-after-statement \
	-Wno-unused-parameter -Wshadow -Wunused-macros -Werror=strict-prototypes \
	-Werror=implicit -Werror=return-type -Werror=incompatible-pointer-types \
	-Wfloat-conversion

# CFLAGS / LDFLAGS
PKGS      = wayland-server xkbcommon libinput pixman-1 fcft lua5.4 $(XLIBS)
DWLCFLAGS = `$(PKG_CONFIG) --cflags $(PKGS)` $(SCENEFX_INCS) $(WLR_INCS) $(DWLCPPFLAGS) $(DWLDEVCFLAGS) $(CFLAGS)
# SCENEFX_LIBS *before* WLR_LIBS -- both export a `wlr_scene_*` symbol set
# (scenefx is a drop-in replacement, not a separate namespace), so link
# order decides which implementation actually gets called for the
# colliding names. Getting this backwards is a real, silent crash, not a
# cosmetic issue: confirmed via gdb (`wlr_scene_node_set_enabled` resolving
# into libwlroots-0.20.so's own copy, built against the *unextended*
# wlr_scene_rect layout, walking a rect scenefx actually allocated with
# its own larger struct -- a real ABI mismatch, not a wasp logic bug).
# MangoWC's own meson.build confirms the same ordering
# (libscenefx_dep listed before wlroots_dep in its dependencies list).
LDLIBS    = `$(PKG_CONFIG) --libs $(PKGS)` $(SCENEFX_LIBS) $(WLR_LIBS) -lm $(LIBS)

all: wasp wasp-list-windows
wasp: wasp.o util.o luaconfig.o
	$(CC) wasp.o util.o luaconfig.o $(DWLCFLAGS) $(LDFLAGS) $(LDLIBS) -o $@
wasp.o: wasp.c client.h config.h config.mk cursor-shape-v1-protocol.h \
	pointer-constraints-unstable-v1-protocol.h wlr-layer-shell-unstable-v1-protocol.h \
	wlr-output-power-management-unstable-v1-protocol.h xdg-shell-protocol.h luaconfig.h
util.o: util.c util.h
luaconfig.o: luaconfig.c luaconfig.h

# wasp-list-windows -- a small standalone Wayland *client*, no wlroots or
# wayland-server involved, so it gets its own PKGS/CFLAGS rather than
# reusing $(DWLCFLAGS)/the generic .c.o suffix rule above (that rule's
# flags are wasp-the-compositor-specific and would drag in
# wayland-server/wlroots headers this doesn't need or want). See its own
# top-of-file comment for what it's for -- NOTES.md item 9.
WLWPKGS   = wayland-client
WLWCFLAGS = `$(PKG_CONFIG) --cflags $(WLWPKGS)` -I. -g -Wall -Wextra $(CFLAGS)
WLWLIBS   = `$(PKG_CONFIG) --libs $(WLWPKGS)`

wasp-list-windows: wasp-list-windows.o ext-foreign-toplevel-list-v1-protocol.o
	$(CC) wasp-list-windows.o ext-foreign-toplevel-list-v1-protocol.o $(WLWCFLAGS) $(LDFLAGS) $(WLWLIBS) -o $@
wasp-list-windows.o: wasp-list-windows.c ext-foreign-toplevel-list-v1-client-protocol.h
	$(CC) $(CPPFLAGS) $(WLWCFLAGS) -c wasp-list-windows.c -o $@
ext-foreign-toplevel-list-v1-protocol.o: ext-foreign-toplevel-list-v1-protocol.c
	$(CC) $(CPPFLAGS) $(WLWCFLAGS) -c ext-foreign-toplevel-list-v1-protocol.c -o $@

# wayland-scanner is a tool which generates C headers and rigging for Wayland
# protocols, which are specified in XML. wlroots requires you to rig these up
# to your build system yourself and provide them in the include path.
WAYLAND_SCANNER   = `$(PKG_CONFIG) --variable=wayland_scanner wayland-scanner`
WAYLAND_PROTOCOLS = `$(PKG_CONFIG) --variable=pkgdatadir wayland-protocols`

cursor-shape-v1-protocol.h:
	$(WAYLAND_SCANNER) enum-header \
		$(WAYLAND_PROTOCOLS)/staging/cursor-shape/cursor-shape-v1.xml $@
pointer-constraints-unstable-v1-protocol.h:
	$(WAYLAND_SCANNER) enum-header \
		$(WAYLAND_PROTOCOLS)/unstable/pointer-constraints/pointer-constraints-unstable-v1.xml $@
wlr-layer-shell-unstable-v1-protocol.h:
	$(WAYLAND_SCANNER) enum-header \
		protocols/wlr-layer-shell-unstable-v1.xml $@
wlr-output-power-management-unstable-v1-protocol.h:
	$(WAYLAND_SCANNER) server-header \
		protocols/wlr-output-power-management-unstable-v1.xml $@
xdg-shell-protocol.h:
	$(WAYLAND_SCANNER) server-header \
		$(WAYLAND_PROTOCOLS)/stable/xdg-shell/xdg-shell.xml $@
ext-foreign-toplevel-list-v1-client-protocol.h:
	$(WAYLAND_SCANNER) client-header \
		$(WAYLAND_PROTOCOLS)/staging/ext-foreign-toplevel-list/ext-foreign-toplevel-list-v1.xml $@
ext-foreign-toplevel-list-v1-protocol.c:
	$(WAYLAND_SCANNER) private-code \
		$(WAYLAND_PROTOCOLS)/staging/ext-foreign-toplevel-list/ext-foreign-toplevel-list-v1.xml $@

config.h:
	cp config.def.h $@
clean:
	rm -f wasp wasp-list-windows *.o *-protocol.h *-protocol.c

dist: clean
	mkdir -p wasp-$(VERSION)
	cp -R LICENSE* Makefile CHANGELOG.md README.md NOTES.md client.h config.def.h \
		config.mk protocols wasp.1 wasp.c util.c util.h luaconfig.c luaconfig.h \
		wasp-list-windows.c examples scripts packaging wasp.desktop wasp-$(VERSION)
	tar -caf wasp-$(VERSION).tar.gz wasp-$(VERSION)
	rm -rf wasp-$(VERSION)

install: wasp wasp-list-windows
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	rm -f $(DESTDIR)$(PREFIX)/bin/wasp
	cp -f wasp $(DESTDIR)$(PREFIX)/bin
	chmod 755 $(DESTDIR)$(PREFIX)/bin/wasp
	cp -f wasp-list-windows $(DESTDIR)$(PREFIX)/bin
	chmod 755 $(DESTDIR)$(PREFIX)/bin/wasp-list-windows
	cp -f scripts/statusbar.sh $(DESTDIR)$(PREFIX)/bin/wasp-statusbar
	chmod 755 $(DESTDIR)$(PREFIX)/bin/wasp-statusbar
	cp -f scripts/wasp-session $(DESTDIR)$(PREFIX)/bin/wasp-session
	chmod 755 $(DESTDIR)$(PREFIX)/bin/wasp-session
	mkdir -p $(DESTDIR)$(MANDIR)/man1
	cp -f wasp.1 $(DESTDIR)$(MANDIR)/man1
	chmod 644 $(DESTDIR)$(MANDIR)/man1/wasp.1
	mkdir -p $(DESTDIR)$(DATADIR)/wayland-sessions
	cp -f wasp.desktop $(DESTDIR)$(DATADIR)/wayland-sessions/wasp.desktop
	chmod 644 $(DESTDIR)$(DATADIR)/wayland-sessions/wasp.desktop
	mkdir -p $(DESTDIR)$(DATADIR)/wasp
	cp -f examples/config.lua $(DESTDIR)$(DATADIR)/wasp/config.lua
	chmod 644 $(DESTDIR)$(DATADIR)/wasp/config.lua
	mkdir -p $(DESTDIR)/etc/xdg-desktop-portal
	cp -f packaging/wasp-portals.conf $(DESTDIR)/etc/xdg-desktop-portal/wasp-portals.conf
	chmod 644 $(DESTDIR)/etc/xdg-desktop-portal/wasp-portals.conf
uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/wasp $(DESTDIR)$(PREFIX)/bin/wasp-list-windows \
		$(DESTDIR)$(PREFIX)/bin/wasp-statusbar \
		$(DESTDIR)$(PREFIX)/bin/wasp-session $(DESTDIR)$(MANDIR)/man1/wasp.1 \
		$(DESTDIR)$(DATADIR)/wayland-sessions/wasp.desktop \
		$(DESTDIR)$(DATADIR)/wasp/config.lua \
		$(DESTDIR)/etc/xdg-desktop-portal/wasp-portals.conf

.SUFFIXES: .c .o
.c.o:
	$(CC) $(CPPFLAGS) $(DWLCFLAGS) -o $@ -c $<
