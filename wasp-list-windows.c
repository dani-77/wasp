/* wasp-list-windows -- lists every open window's app_id/title/identifier
 * over ext-foreign-toplevel-list-v1 (wasp.c's setup(), NOTES.md item 9).
 * Not compositor-specific -- a plain Wayland client, no wlroots headers
 * needed -- but exists here because grim's own `-T <identifier>` flag
 * (single-window capture) has no way to discover that identifier on its
 * own: it's an opaque, per-toplevel string the compositor assigns, not
 * an app_id or title. This is the missing "list windows so I can pick
 * one" step.
 *
 * Usage:
 *   wasp-list-windows                 -- print a table of every window
 *   wasp-list-windows -a <substring>  -- print just the matching
 *                                         identifiers, one per line --
 *                                         e.g. grim -T "$(wasp-list-windows -a firefox)"
 *
 * A window flagged shield_when_capture in wasp.rules still shows up
 * here (this only lists windows, it doesn't request to capture any of
 * them) -- but grim -T against its identifier will be refused by wasp,
 * same as any other single-window capture request for it. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client.h>
#include "ext-foreign-toplevel-list-v1-client-protocol.h"

struct win {
	struct win *next;
	char *title, *app_id, *identifier;
};

static struct win *wins;
static struct ext_foreign_toplevel_list_v1 *toplevel_list;

static void
handle_title(void *data, struct ext_foreign_toplevel_handle_v1 *handle, const char *title)
{
	struct win *w = data;
	(void)handle;
	free(w->title);
	w->title = strdup(title);
}

static void
handle_app_id(void *data, struct ext_foreign_toplevel_handle_v1 *handle, const char *app_id)
{
	struct win *w = data;
	(void)handle;
	free(w->app_id);
	w->app_id = strdup(app_id);
}

static void
handle_identifier(void *data, struct ext_foreign_toplevel_handle_v1 *handle, const char *identifier)
{
	struct win *w = data;
	(void)handle;
	free(w->identifier);
	w->identifier = strdup(identifier);
}

static void
handle_done(void *data, struct ext_foreign_toplevel_handle_v1 *handle)
{
	/* Nothing to do here -- we just print whatever we've collected once
	 * the initial roundtrips are done, see main(). */
	(void)data;
	(void)handle;
}

static void
handle_closed(void *data, struct ext_foreign_toplevel_handle_v1 *handle)
{
	(void)data;
	ext_foreign_toplevel_handle_v1_destroy(handle);
}

static const struct ext_foreign_toplevel_handle_v1_listener handle_listener = {
	.closed = handle_closed,
	.done = handle_done,
	.title = handle_title,
	.app_id = handle_app_id,
	.identifier = handle_identifier,
};

static void
handle_toplevel(void *data, struct ext_foreign_toplevel_list_v1 *list,
		struct ext_foreign_toplevel_handle_v1 *handle)
{
	struct win *w = calloc(1, sizeof(*w));
	(void)data;
	(void)list;
	w->next = wins;
	wins = w;
	ext_foreign_toplevel_handle_v1_add_listener(handle, &handle_listener, w);
}

static void
handle_finished(void *data, struct ext_foreign_toplevel_list_v1 *list)
{
	(void)data;
	(void)list;
}

static const struct ext_foreign_toplevel_list_v1_listener list_listener = {
	.toplevel = handle_toplevel,
	.finished = handle_finished,
};

static void
handle_global(void *data, struct wl_registry *registry, uint32_t name,
		const char *interface, uint32_t version)
{
	(void)data;
	(void)version;
	if (!strcmp(interface, ext_foreign_toplevel_list_v1_interface.name))
		toplevel_list = wl_registry_bind(registry, name,
				&ext_foreign_toplevel_list_v1_interface, 1);
}

static void
handle_global_remove(void *data, struct wl_registry *registry, uint32_t name)
{
	(void)data;
	(void)registry;
	(void)name;
}

static const struct wl_registry_listener registry_listener = {
	.global = handle_global,
	.global_remove = handle_global_remove,
};

int
main(int argc, char *argv[])
{
	struct wl_display *display;
	struct wl_registry *registry;
	struct win *w;
	const char *filter = NULL;

	if (argc == 3 && !strcmp(argv[1], "-a")) {
		filter = argv[2];
	} else if (argc != 1) {
		fprintf(stderr, "usage: %s [-a <app_id-substring>]\n", argv[0]);
		return 1;
	}

	display = wl_display_connect(NULL);
	if (!display) {
		fprintf(stderr, "%s: couldn't connect to a Wayland display\n", argv[0]);
		return 1;
	}

	registry = wl_display_get_registry(display);
	wl_registry_add_listener(registry, &registry_listener, NULL);
	wl_display_roundtrip(display);

	if (!toplevel_list) {
		fprintf(stderr,
				"%s: compositor doesn't support ext-foreign-toplevel-list-v1\n",
				argv[0]);
		wl_display_disconnect(display);
		return 1;
	}
	ext_foreign_toplevel_list_v1_add_listener(toplevel_list, &list_listener, NULL);

	/* One roundtrip to receive the `toplevel` event for every
	 * already-open window (each one creates a new handle + registers
	 * handle_listener on it), a second to receive those handles' own
	 * title/app_id/identifier/done events -- in practice the compositor
	 * sends both bursts close enough together that two roundtrips is
	 * always enough, matching the same pattern other minimal Wayland
	 * CLI tools (wlr-randr and friends) use rather than something more
	 * elaborate like waiting on each handle's own `done`. */
	wl_display_roundtrip(display);
	wl_display_roundtrip(display);

	for (w = wins; w; w = w->next) {
		if (filter && (!w->app_id || !strstr(w->app_id, filter)))
			continue;
		if (filter)
			printf("%s\n", w->identifier ? w->identifier : "");
		else
			printf("%-36s %-24s %s\n",
					w->identifier ? w->identifier : "",
					w->app_id ? w->app_id : "",
					w->title ? w->title : "");
	}

	ext_foreign_toplevel_list_v1_destroy(toplevel_list);
	wl_display_disconnect(display);
	return 0;
}
