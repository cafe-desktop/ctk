/* Pickers
 *
 * These widgets are mainly intended for use in preference dialogs.
 * They allow to select colors, fonts, files, directories and applications.
 */

#include <ctk/ctk.h>

CtkWidget *
do_pickers (CtkWidget *do_widget)
{
  static CtkWidget *window = NULL;
  CtkWidget *table, *label, *picker;

  if (!window)
  {
    window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
    ctk_window_set_screen (CTK_WINDOW (window),
                           ctk_widget_get_screen (do_widget));
    ctk_window_set_title (CTK_WINDOW (window), "Pickers");

    g_signal_connect (window, "destroy",
                      G_CALLBACK (ctk_widget_destroyed), &window);

    ctk_container_set_border_width (CTK_CONTAINER (window), 10);

    table = ctk_grid_new ();
    ctk_grid_set_row_spacing (CTK_GRID (table), 3);
    ctk_grid_set_column_spacing (CTK_GRID (table), 10);
    ctk_container_add (CTK_CONTAINER (window), table);

    ctk_container_set_border_width (CTK_CONTAINER (table), 10);

    label = ctk_label_new ("Color:");
    ctk_widget_set_halign (label, CTK_ALIGN_START);
    ctk_widget_set_valign (label, CTK_ALIGN_CENTER);
    ctk_widget_set_hexpand (label, TRUE);
    picker = ctk_color_button_new ();
    ctk_grid_attach (CTK_GRID (table), label, 0, 0, 1, 1);
    ctk_grid_attach (CTK_GRID (table), picker, 1, 0, 1, 1);

    label = ctk_label_new ("Font:");
    ctk_widget_set_halign (label, CTK_ALIGN_START);
    ctk_widget_set_valign (label, CTK_ALIGN_CENTER);
    ctk_widget_set_hexpand (label, TRUE);
    picker = ctk_font_button_new ();
    ctk_grid_attach (CTK_GRID (table), label, 0, 1, 1, 1);
    ctk_grid_attach (CTK_GRID (table), picker, 1, 1, 1, 1);

    label = ctk_label_new ("File:");
    ctk_widget_set_halign (label, CTK_ALIGN_START);
    ctk_widget_set_valign (label, CTK_ALIGN_CENTER);
    ctk_widget_set_hexpand (label, TRUE);
    picker = ctk_file_chooser_button_new ("Pick a File",
                                          CTK_FILE_CHOOSER_ACTION_OPEN);
    ctk_file_chooser_set_local_only (CTK_FILE_CHOOSER (picker), FALSE);
    ctk_grid_attach (CTK_GRID (table), label, 0, 2, 1, 1);
    ctk_grid_attach (CTK_GRID (table), picker, 1, 2, 1, 1);

    label = ctk_label_new ("Folder:");
    ctk_widget_set_halign (label, CTK_ALIGN_START);
    ctk_widget_set_valign (label, CTK_ALIGN_CENTER);
    picker = ctk_file_chooser_button_new ("Pick a Folder",
                                          CTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
    ctk_grid_attach (CTK_GRID (table), label, 0, 3, 1, 1);
    ctk_grid_attach (CTK_GRID (table), picker, 1, 3, 1, 1);

    label = ctk_label_new ("Mail:");
    ctk_widget_set_halign (label, CTK_ALIGN_START);
    ctk_widget_set_valign (label, CTK_ALIGN_CENTER);
    ctk_widget_set_hexpand (label, TRUE);
    picker = ctk_app_chooser_button_new ("x-scheme-handler/mailto");
    ctk_app_chooser_button_set_show_dialog_item (CTK_APP_CHOOSER_BUTTON (picker), TRUE);
    ctk_grid_attach (CTK_GRID (table), label, 0, 4, 1, 1);
    ctk_grid_attach (CTK_GRID (table), picker, 1, 4, 1, 1);
  }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);

  return window;
}
