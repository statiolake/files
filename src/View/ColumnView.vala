/*
 Copyright (C) 2014 ELementary Developers

 This program is free software: you can redistribute it and/or modify it
 under the terms of the GNU Lesser General Public License version 3, as published
 by the Free Software Foundation.

 This program is distributed in the hope that it will be useful, but
 WITHOUT ANY WARRANTY; without even the implied warranties of
 MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 PURPOSE. See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along
 with this program. If not, see <http://www.gnu.org/licenses/>.

 Authors : Jeremy Wootten <jeremy@elementary.org>
*/


namespace FM {
    /* View for use within the Miller View only */
    public class ColumnView : AbstractTreeView {
    /** Miller View support */
        bool awaiting_double_click = false;
        uint double_click_timeout_id = 0;
        private unowned GOF.File? selected_folder = null;

        public ColumnView (Marlin.View.Slot _slot) {
//message ("New column view");
            base (_slot);
            /* We do not need to load the directory - this is done by Miller */
        }

        construct {
//message ("Column view construct");
        }

        /* Was fm_columns_view_finalize */
        ~ColumnView () {
        }

/** Override parents virtual methods as required*/
        protected override Marlin.ZoomLevel get_set_up_zoom_level () {
//message ("CV setup zoom_level");
            Preferences.marlin_column_view_settings.bind ("zoom-level", this, "zoom-level", GLib.SettingsBindFlags.SET);
            return (Marlin.ZoomLevel)(Preferences.marlin_column_view_settings.get_enum ("zoom-level"));
        }

        public override Marlin.ZoomLevel get_normal_zoom_level () {
            var zoom = Preferences.marlin_column_view_settings.get_enum ("default-zoom-level");
            Preferences.marlin_column_view_settings.set_enum ("zoom-level", zoom);
            return (Marlin.ZoomLevel)zoom;
        }

/** Modified Signal handlers */
        protected new void on_view_selection_changed () {
//message ("on tree selection changed");
            set_active_slot ();
            base.on_view_selection_changed ();
        }

/** Implement Abstract TreeView abstract methods*/
        protected override Gtk.Widget? create_view () {
//message ("CV create view");
            model.set_property ("has-child", false);
            base.create_view ();
            return tree as Gtk.Widget;
        }

        protected override bool on_view_button_release_event (Gdk.EventButton event) {
//message ("Column view button release");
            //bool result =  (Preferences.settings.get_boolean ("single-click") && awaiting_double_click);
            //return result;
            return true;
        }

        protected override bool handle_primary_button_single_click_mode (Gdk.EventButton event, Gtk.TreeSelection? selection, Gtk.TreePath? path, Gtk.TreeViewColumn? col, bool no_mods, bool on_blank) {
//message ("CV handle left button");
            bool result = false;
            if (event.type == Gdk.EventType.BUTTON_PRESS) {
                /* Ignore second GDK_BUTTON_PRESS event of double-click */
                if (awaiting_double_click) {
                    result = true;
                } else if (path != null) {
                    /*Determine where user clicked - this will be the sole selection */
//message ("CV unselect all");
                    selection.unselect_all ();
                    selection.select_path (path);

                    if (!on_blank) {
                        /* Determine if folder selected ... */
                        selected_folder = null;
                        unowned GOF.File file = selected_files.data;

                        if (file.is_folder ()) {
//message ("on folder");
                            /*  ... store clicked folder and start double-click timeout */
                            selected_folder = file;
                            awaiting_double_click = true;
//message ("freeze updates and start timeout");
                            freeze_updates ();
                            /* use short timeout to maintain responsiveness */
                            double_click_timeout_id = GLib.Timeout.add (100, not_double_click);
                        }
                    } else
                        /* Do not activate row if click on blank part */
                        result = true;
                } else
                    /* pass on event to activate the row and slot clicked on unless on blank part*/
                    result = on_blank;
            } else if (event.type == Gdk.EventType.@2BUTTON_PRESS) {
                cancel_await_double_click ();
                if (selected_folder != null) {
                    load_root_location (selected_folder.location);
                }
                result = true;
            }
            return result;
        }

        protected override bool handle_middle_button_click (Gdk.EventButton event, Gtk.TreeSelection? selection, Gtk.TreePath? path, Gtk.TreeViewColumn? col, bool no_mods, bool on_blank) {
                /* opens folder(s) in new tab */
                cancel_await_double_click ();
                if (path != null) {
                    //message (" (Marlin.OpenFlag.NEW_TAB);
                    return true;
                } else
                    return false;
        }

        protected override bool handle_default_button_click () {
            cancel_await_double_click ();
            return false;
        }

/** Private methods */
        private void cancel_await_double_click () {
//message ("MCV cancel await double click");
            if (awaiting_double_click) {
                GLib.Source.remove (double_click_timeout_id);
                double_click_timeout_id = 0;
                awaiting_double_click = false;
                unfreeze_updates ();
            }
        }


        private bool not_double_click () {
//message ("MCV not double click");
            if (double_click_timeout_id != 0) {
//message ("timeout not cancelled");
                double_click_timeout_id = 0;
                awaiting_double_click = false;
//message ("unfreeze updated");
                unfreeze_updates ();
//message ("activate if drag not pending");
                if (!is_drag_pending ()) {
//message ("activation");
                    activate_selected_items ();
                }
            }
            return false;
        }


    }
}
