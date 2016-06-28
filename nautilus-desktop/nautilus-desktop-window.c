
/*
 * Nautilus
 *
 * Copyright (C) 2000 Eazel, Inc.
 *
 * Nautilus is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * Nautilus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Darin Adler <darin@bentspoon.com>
 */

#include <config.h>

#include "nautilus-desktop-window.h"
#include "nautilus-desktop-window-slot.h"
#include "nautilus-desktop-link-monitor.h"

#include <X11/Xatom.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <gio/gio.h>
#include <glib/gi18n.h>

#include <eel/eel-vfs-extensions.h>
#include <src/nautilus-file-utilities.h>
#include <src/nautilus-icon-names.h>
#include <src/nautilus-global-preferences.h>
#include <src/nautilus-window.h>
#include <src/nautilus-application.h>

struct NautilusDesktopWindowDetails {
	gulong size_changed_id;

	gboolean loaded;

	GtkWidget *desktop_selection;
  GdkWindow *root_window;
};

G_DEFINE_TYPE (NautilusDesktopWindow, nautilus_desktop_window, 
	       NAUTILUS_TYPE_WINDOW);

static void
nautilus_desktop_window_update_directory (NautilusDesktopWindow *window)
{
	GFile *location;

	g_assert (NAUTILUS_IS_DESKTOP_WINDOW (window));

	window->details->loaded = FALSE;
	location = g_file_new_for_uri (EEL_DESKTOP_URI);
        /* We use NAUTILUS_WINDOW_OPEN_FLAG_DONT_MAKE_ACTIVE so the nautilus-window
         * doesn't call gtk_window_present () which raises the window on the stack
         * and actually hides it from the background */
        nautilus_application_open_location_full (NAUTILUS_APPLICATION (g_application_get_default ()),
                                                 location,
                                                 NAUTILUS_WINDOW_OPEN_FLAG_DONT_MAKE_ACTIVE,
                                                 NULL, NAUTILUS_WINDOW (window), NULL);
	window->details->loaded = TRUE;

	g_object_unref (location);
}

static void
nautilus_desktop_window_finalize (GObject *obj)
{
	nautilus_desktop_link_monitor_shutdown ();

	G_OBJECT_CLASS (nautilus_desktop_window_parent_class)->finalize (obj);
}

static void
nautilus_desktop_window_init_actions (NautilusDesktopWindow *window)
{
	GAction *action;

	/* Don't allow close action on desktop */
	action = g_action_map_lookup_action (G_ACTION_MAP (window),
					     "close-current-view");
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action), FALSE);

	/* Don't allow new tab on desktop */
	action = g_action_map_lookup_action (G_ACTION_MAP (window),
					     "new-tab");
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action), FALSE);

	/* Don't allow switching to home dir on desktop */
	action = g_action_map_lookup_action (G_ACTION_MAP (window),
					     "go-home");
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action), FALSE);
}

static void
selection_get_cb (GtkWidget          *widget,
		  GtkSelectionData   *selection_data,
		  guint               info,
		  guint               time)
{
	/* No extra targets atm */
}

static gboolean
selection_clear_event_cb (GtkWidget	        *widget,
			  GdkEventSelection     *event,
			  NautilusDesktopWindow *window)
{
	gtk_widget_destroy (GTK_WIDGET (window));

	return TRUE;
}

static void
nautilus_desktop_window_init_selection (NautilusDesktopWindow *window)
{
	char selection_name[32];
	GdkAtom selection_atom;
	Window selection_owner;
	GdkDisplay *display;
	GtkWidget *selection_widget;
	GdkScreen *screen;

	screen = gdk_screen_get_default ();

	g_snprintf (selection_name, sizeof (selection_name),
		    "_NET_DESKTOP_MANAGER_S%d", gdk_screen_get_number (screen));
	selection_atom = gdk_atom_intern (selection_name, FALSE);
	display = gdk_screen_get_display (screen);

	selection_owner = XGetSelectionOwner (GDK_DISPLAY_XDISPLAY (display),
					      gdk_x11_atom_to_xatom_for_display (display,
										 selection_atom));
	if (selection_owner != None) {
		g_critical ("Another desktop manager in use; desktop window won't be created");
		return;
	}

	selection_widget = gtk_invisible_new_for_screen (screen);
	/* We need this for gdk_x11_get_server_time() */
	gtk_widget_add_events (selection_widget, GDK_PROPERTY_CHANGE_MASK);

	if (!gtk_selection_owner_set_for_display (display,
						  selection_widget,
						  selection_atom,
						  gdk_x11_get_server_time (gtk_widget_get_window (selection_widget)))) {
		gtk_widget_destroy (selection_widget);
		g_critical ("Can't set ourselves as selection owner for desktop manager; "
			    "desktop window won't be created");
		return;
	}

	g_signal_connect (selection_widget, "selection-get",
			  G_CALLBACK (selection_get_cb), window);
	g_signal_connect (selection_widget, "selection-clear-event",
			  G_CALLBACK (selection_clear_event_cb), window);

	window->details->desktop_selection = selection_widget;
}

static GdkRectangle*
intersect_primary_monitor_with_workareas (NautilusDesktopWindow *window,
                                          long                  *workareas,
                                          int                    n_items)
{
	int i;
	GdkRectangle geometry;
	GdkRectangle *intersected_geometry;

        intersected_geometry = g_new (GdkRectangle, 1);
	gdk_screen_get_monitor_geometry (gdk_screen_get_default (),
                                         gdk_screen_get_primary_monitor (gdk_screen_get_default ()),
                                         &geometry);
        intersected_geometry->x = geometry.x;
        intersected_geometry->y = geometry.y;
        intersected_geometry->width = geometry.width;
        intersected_geometry->height = geometry.height;

	for (i = 0; i < n_items; i += 4) {
		GdkRectangle workarea;

		workarea.x = workareas[i];
		workarea.y = workareas[i + 1];
		workarea.width = workareas[i + 2];
		workarea.height = workareas[i + 3];

                g_print ("owrkarea %d %d %d %d, intersected %d %d %d %d\n", workarea.x, workarea.y, workarea.width, workarea.height, intersected_geometry->x, intersected_geometry->y,
                         intersected_geometry->width, intersected_geometry->height);
		if (!gdk_rectangle_intersect (&geometry, &workarea, &workarea))
			continue;

		intersected_geometry->x = MAX (intersected_geometry->x, workarea.x);
		intersected_geometry->y = MAX (intersected_geometry->y, workarea.y);
		intersected_geometry->width = MIN (intersected_geometry->width,
                                                   workarea.x + workarea.width - intersected_geometry->x);
		intersected_geometry->height = MIN (intersected_geometry->height,
                                                    workarea.y + workarea.height - intersected_geometry->y);
	}

  return intersected_geometry;
}

static GdkRectangle*
calculate_size_desktop_geometry (NautilusDesktopWindow *window)
{
	long *nworkareas = NULL;
	long *workareas = NULL;
	GdkAtom type_returned;
	int format_returned;
	int length_returned;
        GdkWindow *root_window;
        GdkRectangle *intersected_geometry = NULL;

        g_print ("calculate size desktop\n");
        root_window = gdk_screen_get_root_window (gdk_screen_get_default ());
	/* Find the number of desktops so we know how long the
	 * workareas array is going to be (each desktop will have four
	 * elements in the workareas array describing
	 * x,y,width,height) */
	gdk_error_trap_push ();
	if (!gdk_property_get (root_window,
			       gdk_atom_intern ("_NET_NUMBER_OF_DESKTOPS", FALSE),
			       gdk_x11_xatom_to_atom (XA_CARDINAL),
			       0, 4, FALSE,
			       &type_returned,
			       &format_returned,
			       &length_returned,
			       (guchar **) &nworkareas)) {
		g_warning("Can not calculate _NET_NUMBER_OF_DESKTOPS");
	}
	if (gdk_error_trap_pop()
	    || nworkareas == NULL
	    || type_returned != gdk_x11_xatom_to_atom (XA_CARDINAL)
	    || format_returned != 32)
		g_warning("Can not calculate _NET_NUMBER_OF_DESKTOPS");

	/* Note : gdk_property_get() is broken (API documents admit
	 * this).  As a length argument, it expects the number of
	 * _bytes_ of data you require.  Internally, gdk_property_get
	 * converts that value to a count of 32 bit (4 byte) elements.
	 * However, the length returned is in bytes, but is calculated
	 * via the count of returned elements * sizeof(long).  This
	 * means on a 64 bit system, the number of bytes you have to
	 * request does not correspond to the number of bytes you get
	 * back, and is the reason for the workaround below.
	 */
	gdk_error_trap_push ();
	if (nworkareas == NULL || (*nworkareas < 1)
	    || !gdk_property_get (root_window,
				  gdk_atom_intern ("_NET_WORKAREA", FALSE),
				  gdk_x11_xatom_to_atom (XA_CARDINAL),
				  0, ((*nworkareas) * 4 * 4), FALSE,
				  &type_returned,
				  &format_returned,
				  &length_returned,
				  (guchar **) &workareas)) {
		g_warning("Can not get _NET_WORKAREA");
		workareas = NULL;
	}

	if (gdk_error_trap_pop ()
	    || workareas == NULL
	    || type_returned != gdk_x11_xatom_to_atom (XA_CARDINAL)
	    || ((*nworkareas) * 4 * sizeof(long)) != length_returned
	    || format_returned != 32) {
              g_critical ("NET_WORKAREA canot be peeked");
	} else {
        g_print ("good good \n");
                intersected_geometry = intersect_primary_monitor_with_workareas (window, workareas, *nworkareas);
        }

	if (nworkareas != NULL)
		g_free (nworkareas);

	if (workareas != NULL)
		g_free (workareas);

  return intersected_geometry;
}

static void
net_workarea_changed (NautilusDesktopWindow *window)
{
  GdkRectangle *geometry;

  geometry = calculate_size_desktop_geometry (window);
  g_object_set (window,
                "width_request", geometry->width,
                "height_request", geometry->height,
                NULL);

  gtk_window_move (GTK_WINDOW (window), geometry->x, geometry->y);

  g_free (geometry);
}

static void
nautilus_desktop_window_constructed (GObject *obj)
{
	AtkObject *accessible;
	NautilusDesktopWindow *window = NAUTILUS_DESKTOP_WINDOW (obj);

	G_OBJECT_CLASS (nautilus_desktop_window_parent_class)->constructed (obj);

	/* Initialize the desktop link monitor singleton */
	nautilus_desktop_link_monitor_get ();

	/* shouldn't really be needed given our semantic type
	 * of _NET_WM_TYPE_DESKTOP, but why not
	 */
	gtk_window_set_resizable (GTK_WINDOW (window),
				  FALSE);
	gtk_window_set_decorated (GTK_WINDOW (window),
				  FALSE);

	g_object_set_data (G_OBJECT (window), "is_desktop_window",
			   GINT_TO_POINTER (1));

	net_workarea_changed (window);

	nautilus_desktop_window_init_selection (window);
	nautilus_desktop_window_init_actions (window);

	if (window->details->desktop_selection != NULL) {
		/* Set the accessible name so that it doesn't inherit the cryptic desktop URI. */
		accessible = gtk_widget_get_accessible (GTK_WIDGET (window));

		if (accessible) {
			atk_object_set_name (accessible, _("Desktop"));
		}

		/* Special sawmill setting */
		gtk_window_set_wmclass (GTK_WINDOW (window), "desktop_window", "Nautilus");

		/* Point window at the desktop folder.
		 * Note that nautilus_desktop_window_init is too early to do this.
		 */
		nautilus_desktop_window_update_directory (window);

		/* We realize it immediately so that the NAUTILUS_DESKTOP_WINDOW_ID
		 * property is set so gnome-settings-daemon doesn't try to set
		 * background. And we do a gdk_flush() to be sure X gets it.
		 */
		gtk_widget_realize (GTK_WIDGET (window));
		gdk_flush ();
	}
}

static GdkFilterReturn
root_window_property_filter (GdkXEvent *gdk_xevent,
				   GdkEvent *event,
				   gpointer data)
{
	XEvent *xevent = gdk_xevent;
	NautilusDesktopWindow *window;

	window = NAUTILUS_DESKTOP_WINDOW (data);

	switch (xevent->type) {
	case PropertyNotify:
		if (xevent->xproperty.atom == gdk_x11_get_xatom_by_name ("_NET_WORKAREA"))
			net_workarea_changed (window);
		break;
	default:
		break;
	}

	return GDK_FILTER_CONTINUE;
}


static void
unrealized_callback (GtkWidget *widget,
                     NautilusDesktopWindow *window)
{
	/* Remove the property filter */
	gdk_window_remove_filter (gdk_screen_get_root_window (gdk_screen_get_default ()),
				  root_window_property_filter,
				  window);
}

static void
realized_callback (GtkWidget *widget, NautilusDesktopWindow *window)
{
	GdkWindow *root_window;
	GdkScreen *screen;

	screen = gtk_widget_get_screen (widget);
	root_window = gdk_screen_get_root_window (screen);

	/* Read out the workarea geometry and update the icon container accordingly */
	net_workarea_changed (window);

	/* Setup the property filter */
	gdk_window_set_events (root_window, GDK_PROPERTY_CHANGE_MASK);
	gdk_window_add_filter (root_window,
			       root_window_property_filter,
			       window);
}

static void
nautilus_desktop_window_init (NautilusDesktopWindow *window)
{
	window->details = G_TYPE_INSTANCE_GET_PRIVATE (window, NAUTILUS_TYPE_DESKTOP_WINDOW,
						       NautilusDesktopWindowDetails);
	gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET (window)),
				     "nautilus-desktop-window");

	g_signal_connect_object (window, "realize",
				 G_CALLBACK (realized_callback), window, 0);
	g_signal_connect_object (window, "unrealize",
				 G_CALLBACK (unrealized_callback), window, 0);

}

static NautilusDesktopWindow *
nautilus_desktop_window_new (void)
{
	GApplication *application;
	NautilusDesktopWindow *window;

	application = g_application_get_default ();
	window = g_object_new (NAUTILUS_TYPE_DESKTOP_WINDOW,
			       "application", application,
			       "disable-chrome", TRUE,
			       NULL);

        net_workarea_changed (window);

	return window;
}

static NautilusDesktopWindow *the_desktop_window = NULL;

void
nautilus_desktop_window_ensure (void)
{
	NautilusDesktopWindow *window;

	if (!the_desktop_window) {
		window = nautilus_desktop_window_new ();
		g_object_add_weak_pointer (G_OBJECT (window), (gpointer *) &the_desktop_window);
		the_desktop_window = window;
	}
}

GtkWidget *
nautilus_desktop_window_get (void)
{
	return GTK_WIDGET (the_desktop_window);
}

static gboolean
nautilus_desktop_window_delete_event (GtkWidget *widget,
				      GdkEventAny *event)
{
	/* Returning true tells GTK+ not to delete the window. */
	return TRUE;
}

static void
map (GtkWidget *widget)
{
	/* Chain up to realize our children */
	GTK_WIDGET_CLASS (nautilus_desktop_window_parent_class)->map (widget);
	gdk_window_lower (gtk_widget_get_window (widget));
}

static void
unrealize (GtkWidget *widget)
{
	NautilusDesktopWindow *window;
	NautilusDesktopWindowDetails *details;

	window = NAUTILUS_DESKTOP_WINDOW (widget);
	details = window->details;

	if (details->size_changed_id != 0) {
		g_signal_handler_disconnect (gtk_window_get_screen (GTK_WINDOW (window)),
					     details->size_changed_id);
		details->size_changed_id = 0;
	}

	gtk_widget_destroy (details->desktop_selection);

	GTK_WIDGET_CLASS (nautilus_desktop_window_parent_class)->unrealize (widget);
}

static void
set_wmspec_desktop_hint (GdkWindow *window)
{
	GdkAtom atom;

	atom = gdk_atom_intern ("_NET_WM_WINDOW_TYPE_DESKTOP", FALSE);
        
	gdk_property_change (window,
			     gdk_atom_intern ("_NET_WM_WINDOW_TYPE", FALSE),
			     gdk_x11_xatom_to_atom (XA_ATOM), 32,
			     GDK_PROP_MODE_REPLACE, (guchar *) &atom, 1);
}

static void
realize (GtkWidget *widget)
{
	NautilusDesktopWindow *window;
	NautilusDesktopWindowDetails *details;
	GdkVisual *visual;

	window = NAUTILUS_DESKTOP_WINDOW (widget);
	details = window->details;

	/* Make sure we get keyboard events */
	gtk_widget_set_events (widget, gtk_widget_get_events (widget) 
			      | GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);
			      
	visual = gdk_screen_get_rgba_visual (gtk_widget_get_screen (widget));
	if (visual) {
		gtk_widget_set_visual (widget, visual);
	}

	/* Do the work of realizing. */
	GTK_WIDGET_CLASS (nautilus_desktop_window_parent_class)->realize (widget);

	/* This is the new way to set up the desktop window */
	set_wmspec_desktop_hint (gtk_widget_get_window (widget));

	details->size_changed_id =
		g_signal_connect_swapped (gtk_window_get_screen (GTK_WINDOW (window)), "size-changed",
				  G_CALLBACK (net_workarea_changed), window);
}

static void
real_sync_title (NautilusWindow *window,
		 NautilusWindowSlot *slot)
{
	/* hardcode "Desktop" */
	gtk_window_set_title (GTK_WINDOW (window), _("Desktop"));
}

static void
real_window_close (NautilusWindow *window)
{
	/* stub, does nothing */
	return;
}

static NautilusWindowSlot *
real_create_slot (NautilusWindow *window,
                  GFile          *location)
{
	return NAUTILUS_WINDOW_SLOT (nautilus_desktop_window_slot_new (window));
}

static void
nautilus_desktop_window_class_init (NautilusDesktopWindowClass *klass)
{
	GtkWidgetClass *wclass = GTK_WIDGET_CLASS (klass);
	NautilusWindowClass *nclass = NAUTILUS_WINDOW_CLASS (klass);
	GObjectClass *oclass = G_OBJECT_CLASS (klass);

	oclass->constructed = nautilus_desktop_window_constructed;
	oclass->finalize = nautilus_desktop_window_finalize;

	wclass->realize = realize;
	wclass->unrealize = unrealize;
	wclass->map = map;
	wclass->delete_event = nautilus_desktop_window_delete_event;

	nclass->sync_title = real_sync_title;
	nclass->close = real_window_close;
	nclass->create_slot = real_create_slot;

	g_type_class_add_private (klass, sizeof (NautilusDesktopWindowDetails));
}

gboolean
nautilus_desktop_window_loaded (NautilusDesktopWindow *window)
{
	return window->details->loaded;
}
