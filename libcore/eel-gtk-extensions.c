/* eel-gtk-extensions.c - implementation of new functions that operate on
 * gtk classes. Perhaps some of these should be
 * rolled into gtk someday.
 *
 * Copyright (C) 1999, 2000, 2001 Eazel, Inc.
 *
 * The Gnome Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Gnome Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Gnome Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Authors: John Sullivan <sullivan@eazel.com>
 *          Ramiro Estrugo <ramiro@eazel.com>
 *          Darin Adler <darin@eazel.com>
 */

#include <glib.h>
#include "eel-gtk-extensions.h"
#include <gdk/gdk.h>

/* Used for window position & size sanity-checking. The sizes are big enough to prevent
 * at least normal-sized gnome panels from obscuring the window at the screen edges.
 */
#define MINIMUM_ON_SCREEN_WIDTH     100
#define MINIMUM_ON_SCREEN_HEIGHT    100

/**
 * eel_gtk_window_get_geometry_string:
 * @window: a #GtkWindow
 *
 * Obtains the geometry string for this window, suitable for
 * set_geometry_string(); assumes the window has NorthWest gravity
 *
 * Return value: geometry string, must be freed
**/
char*
eel_gtk_window_get_geometry_string (GtkWindow *window)
{
    char *str;
    int w, h, x, y;

    g_return_val_if_fail (GTK_IS_WINDOW (window), NULL);
    g_return_val_if_fail (gtk_window_get_gravity (window) ==
                          GDK_GRAVITY_NORTH_WEST, NULL);

    gtk_window_get_position (window, &x, &y);
    gtk_window_get_size (window, &w, &h);

    str = g_strdup_printf ("%dx%d+%d+%d", w, h, x, y);

    return str;
}

static void
sanity_check_window_position (int *left, int *top)
{
    g_assert (left != NULL);
    g_assert (top != NULL);

    /* Make sure the top of the window is on screen, for
     * draggability (might not be necessary with all window managers,
     * but seems reasonable anyway). Make sure the top of the window
     * isn't off the bottom of the screen, or so close to the bottom
     * that it might be obscured by the panel.
     */
    *top = CLAMP (*top, 0, gdk_screen_height() - MINIMUM_ON_SCREEN_HEIGHT);

    /* FIXME bugzilla.eazel.com 669:
     * If window has negative left coordinate, set_uposition sends it
     * somewhere else entirely. Not sure what level contains this bug (XWindows?).
     * Hacked around by pinning the left edge to zero, which just means you
     * can't set a window to be partly off the left of the screen using
     * this routine.
     */
    /* Make sure the left edge of the window isn't off the right edge of
     * the screen, or so close to the right edge that it might be
     * obscured by the panel.
     */
    *left = CLAMP (*left, 0, gdk_screen_width() - MINIMUM_ON_SCREEN_WIDTH);
}

static void
sanity_check_window_dimensions (guint *width, guint *height)
{
    g_assert (width != NULL);
    g_assert (height != NULL);

    /* Pin the size of the window to the screen, so we don't end up in
     * a state where the window is so big essential parts of it can't
     * be reached (might not be necessary with all window managers,
     * but seems reasonable anyway).
     */
    *width = MIN (*width, gdk_screen_width());
    *height = MIN (*height, gdk_screen_height());
}

/**
 * eel_gtk_window_set_initial_geometry:
 *
 * Sets the position and size of a GtkWindow before the
 * GtkWindow is shown. It is an error to call this on a window that
 * is already on-screen. Takes into account screen size, and does
 * some sanity-checking on the passed-in values.
 * If no originals coordinates are given the window would be centered
 *
 * @window: A non-visible GtkWindow
 * @geometry_flags: A EelGdkGeometryFlags value defining which of
 * the following parameters have defined values
 * @left: pixel coordinate for left of window
 * @top: pixel coordinate for top of window
 * @width: width of window in pixels
 * @height: height of window in pixels
 */
void
eel_gtk_window_set_initial_geometry (GtkWindow *window,
                                     EelGdkGeometryFlags geometry_flags,
                                     int left,
                                     int top,
                                     guint width,
                                     guint height)
{
    GdkScreen *screen;
    int real_left, real_top;
    int screen_width, screen_height;

    g_return_if_fail (GTK_IS_WINDOW (window));

    /* Setting the default size doesn't work when the window is already showing.
     * Someday we could make this move an already-showing window, but we don't
     * need that functionality yet.
     */
    g_return_if_fail (!gtk_widget_get_visible (GTK_WIDGET (window)));

    if ((geometry_flags & EEL_GDK_X_VALUE) && (geometry_flags & EEL_GDK_Y_VALUE)) {
        real_left = left;
        real_top = top;

        screen = gtk_window_get_screen (window);
        screen_width  = gdk_screen_get_width  (screen);
        screen_height = gdk_screen_get_height (screen);

        /* This is sub-optimal. GDK doesn't allow us to set win_gravity
         * to South/East types, which should be done if using negative
         * positions (so that the right or bottom edge of the window
         * appears at the specified position, not the left or top).
         * However it does seem to be consistent with other GNOME apps.
         */
        if (geometry_flags & EEL_GDK_X_NEGATIVE) {
            real_left = screen_width - real_left;
        }
        if (geometry_flags & EEL_GDK_Y_NEGATIVE) {
            real_top = screen_height - real_top;
        }

        sanity_check_window_position (&real_left, &real_top);
        gtk_window_move (window, real_left, real_top);
    } else {
        if (left == -1 && top == -1) {
            /* no position coordinates found, let's center the window */
            gtk_window_set_position (window, GTK_WIN_POS_CENTER);
        }
    }

    sanity_check_window_dimensions (&width, &height);
    gtk_window_set_default_size (GTK_WINDOW (window), (int)width, (int)height);
}

/**
 * eel_gtk_window_set_initial_geometry_from_string:
 *
 * Sets the position and size of a GtkWindow before the
 * GtkWindow is shown. The geometry is passed in as a string.
 * It is an error to call this on a window that
 * is already on-screen. Takes into account screen size, and does
 * some sanity-checking on the passed-in values.
 *
 * @window: A non-visible GtkWindow
 * @geometry_string: A string suitable for use with eel_gdk_parse_geometry
 * @minimum_width: If the width from the string is smaller than this,
 * use this for the width.
 * @minimum_height: If the height from the string is smaller than this,
 * use this for the height.
 * @ignore_position: If true position data from string will be ignored.
 */
void
eel_gtk_window_set_initial_geometry_from_string (GtkWindow *window,
                                                 const char *geometry_string,
                                                 guint minimum_width,
                                                 guint minimum_height,
                                                 gboolean ignore_position)
{
    int left, top;
    guint width, height;
    EelGdkGeometryFlags geometry_flags;

    g_return_if_fail (GTK_IS_WINDOW (window));
    g_return_if_fail (geometry_string != NULL);

    /* Setting the default size doesn't work when the window is already showing.
     * Someday we could make this move an already-showing window, but we don't
     * need that functionality yet.
     */
    g_return_if_fail (!gtk_widget_get_visible (GTK_WIDGET (window)));

    width = height = 0;
    left = top = -1;
    geometry_flags = eel_gdk_parse_geometry (geometry_string, &left, &top, &width, &height);

    /* Make sure the window isn't smaller than makes sense for this window.
     * Other sanity checks are performed in set_initial_geometry.
     */
    width = MAX (width, minimum_width);
    height = MAX (height, minimum_height);

    /* Ignore saved window position if requested. */
    if (ignore_position) {
        geometry_flags &= ~(EEL_GDK_X_VALUE | EEL_GDK_Y_VALUE);
    }

    eel_gtk_window_set_initial_geometry (window, geometry_flags, left, top, width, height);
}

GtkMenuItem *
eel_gtk_menu_insert_separator (GtkMenu *menu, int index)
{
    GtkWidget *menu_item;

    menu_item = gtk_separator_menu_item_new ();
    gtk_widget_show (menu_item);
    gtk_menu_shell_insert (GTK_MENU_SHELL (menu), menu_item, index);

    return GTK_MENU_ITEM (menu_item);
}

GtkMenuItem *
eel_gtk_menu_append_separator (GtkMenu *menu)
{
    return eel_gtk_menu_insert_separator (menu, -1);
}

/**
 * eel_pop_up_context_menu:
 *
 * Pop up a context menu under the mouse.
 * The menu is sunk after use, so it will be destroyed unless the
 * caller first ref'ed it.
 *
 * This function is more of a helper function than a gtk extension,
 * so perhaps it belongs in a different file.
 *
 * @menu: The menu to pop up under the mouse.
 * @offset_x: Number of pixels to displace the popup menu vertically
 * @offset_y: Number of pixels to displace the popup menu horizontally
 * @event: The event that invoked this popup menu.
**/
void
eel_pop_up_context_menu (GtkMenu	     *menu,
                         gint16	      offset_x,
                         gint16	      offset_y,
                         GdkEventButton *event)
{
    GdkPoint offset;
    int button;

    g_return_if_fail (GTK_IS_MENU (menu));

    offset.x = offset_x;
    offset.y = offset_y;

    /* The event button needs to be 0 if we're popping up this menu from
     * a button release, else a 2nd click outside the menu with any button
     * other than the one that invoked the menu will be ignored (instead
     * of dismissing the menu). This is a subtle fragility of the GTK menu code.
     */

    if (event) {
        button = event->type == GDK_BUTTON_RELEASE
            ? 0
            : event->button;
    } else {
        button = 0;
    }

    gtk_menu_popup (menu,					/* menu */
                    NULL,					/* parent_menu_shell */
                    NULL,					/* parent_menu_item */
                    NULL,
                    &offset,			        /* data */
                    button,					/* button */
                    event ? event->time : GDK_CURRENT_TIME); /* activate_time */

    g_object_ref_sink (menu);
    g_object_unref (menu);
}

/**
 * eel_gtk_widget_set_shown
 *
 * Show or hide a widget.
 * @widget: The widget.
 * @shown: Boolean value indicating whether the widget should be shown or hidden.
**/
void
eel_gtk_widget_set_shown (GtkWidget *widget, gboolean shown)
{
    g_return_if_fail (GTK_IS_WIDGET (widget));

    if (shown) {
        gtk_widget_show (widget);
    } else {
        gtk_widget_hide (widget);
    }
}

static gboolean
tree_view_button_press_callback (GtkWidget *tree_view,
                                 GdkEventButton *event,
                                 gpointer data)
{
    GtkTreePath *path;
    GtkTreeViewColumn *column;

    if (event->button == 1 && event->type == GDK_BUTTON_PRESS) {
        if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (tree_view),
                                           event->x, event->y,
                                           &path,
                                           &column,
                                           NULL,
                                           NULL)) {
            gtk_tree_view_row_activated
                (GTK_TREE_VIEW (tree_view), path, column);
        }
    }

    return FALSE;
}

void
eel_gtk_tree_view_set_activate_on_single_click (GtkTreeView *tree_view,
                                                gboolean should_activate)
{
    guint button_press_id;

    button_press_id = GPOINTER_TO_UINT
        (g_object_get_data (G_OBJECT (tree_view),
                            "eel-tree-view-activate"));

    if (button_press_id && !should_activate) {
        g_signal_handler_disconnect (tree_view, button_press_id);
        g_object_set_data (G_OBJECT (tree_view),
                           "eel-tree-view-activate",
                           NULL);
    } else if (!button_press_id && should_activate) {
        button_press_id = g_signal_connect
            (tree_view,
             "button_press_event",
             G_CALLBACK  (tree_view_button_press_callback),
             NULL);
        g_object_set_data (G_OBJECT (tree_view),
                           "eel-tree-view-activate",
                           GUINT_TO_POINTER (button_press_id));
    }
}

GdkScreen *
eel_gtk_widget_get_screen (GtkWidget *widget)
{
    GdkScreen *screen = NULL;

    if (G_UNLIKELY (widget == NULL))
        screen = gdk_screen_get_default ();
    else if (GTK_IS_WIDGET (widget))
        screen = gtk_widget_get_screen (widget);

    return screen;
}