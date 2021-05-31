/* Size Groups
 *
 * CtkSizeGroup provides a mechanism for grouping a number of
 * widgets together so they all request the same amount of space.
 * This is typically useful when you want a column of widgets to
 * have the same size, but you can't use a CtkTable widget.
 *
 * Note that size groups only affect the amount of space requested,
 * not the size that the widgets finally receive. If you want the
 * widgets in a CtkSizeGroup to actually be the same size, you need
 * to pack them in such a way that they get the size they request
 * and not more. For example, if you are packing your widgets
 * into a table, you would not include the CTK_FILL flag.
 */

#include <glib/gi18n.h>
#include <ctk/ctk.h>

/* Convenience function to create a combo box holding a number of strings
 */
CtkWidget *
create_combo_box (const char **strings)
{
  CtkWidget *combo_box;
  const char **str;

  combo_box = ctk_combo_box_text_new ();

  for (str = strings; *str; str++)
    ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo_box), *str);

  ctk_combo_box_set_active (CTK_COMBO_BOX (combo_box), 0);

  return combo_box;
}

static void
add_row (CtkGrid      *table,
         int           row,
         CtkSizeGroup *size_group,
         const char   *label_text,
         const char  **options)
{
  CtkWidget *combo_box;
  CtkWidget *label;

  label = ctk_label_new_with_mnemonic (label_text);
  ctk_widget_set_halign (label, CTK_ALIGN_START);
  ctk_widget_set_valign (label, CTK_ALIGN_BASELINE);
  ctk_widget_set_hexpand (label, TRUE);
  ctk_grid_attach (table, label, 0, row, 1, 1);

  combo_box = create_combo_box (options);
  ctk_label_set_mnemonic_widget (CTK_LABEL (label), combo_box);
  ctk_widget_set_halign (combo_box, CTK_ALIGN_END);
  ctk_widget_set_valign (combo_box, CTK_ALIGN_BASELINE);
  ctk_size_group_add_widget (size_group, combo_box);
  ctk_grid_attach (table, combo_box, 1, row, 1, 1);
}

static void
toggle_grouping (CtkToggleButton *check_button,
                 CtkSizeGroup    *size_group)
{
  CtkSizeGroupMode new_mode;

  /* CTK_SIZE_GROUP_NONE is not generally useful, but is useful
   * here to show the effect of CTK_SIZE_GROUP_HORIZONTAL by
   * contrast.
   */
  if (ctk_toggle_button_get_active (check_button))
    new_mode = CTK_SIZE_GROUP_HORIZONTAL;
  else
    new_mode = CTK_SIZE_GROUP_NONE;

  ctk_size_group_set_mode (size_group, new_mode);
}

CtkWidget *
do_sizegroup (CtkWidget *do_widget)
{
  static CtkWidget *window = NULL;
  CtkWidget *table;
  CtkWidget *frame;
  CtkWidget *vbox;
  CtkWidget *check_button;
  CtkSizeGroup *size_group;

  static const char *color_options[] = {
    "Red", "Green", "Blue", NULL
  };

  static const char *dash_options[] = {
    "Solid", "Dashed", "Dotted", NULL
  };

  static const char *end_options[] = {
    "Square", "Round", "Double Arrow", NULL
  };

  if (!window)
    {
      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_screen (CTK_WINDOW (window), ctk_widget_get_screen (do_widget));
      ctk_window_set_title (CTK_WINDOW (window), "Size Groups");
      ctk_window_set_resizable (CTK_WINDOW (window), FALSE);
      g_signal_connect (window, "destroy",
                        G_CALLBACK (ctk_widget_destroyed), &window);

      vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 5);
      ctk_container_add (CTK_CONTAINER (window), vbox);
      ctk_container_set_border_width (CTK_CONTAINER (vbox), 5);

      size_group = ctk_size_group_new (CTK_SIZE_GROUP_HORIZONTAL);
      g_object_set_data_full (G_OBJECT (window), "size-group", size_group, g_object_unref);

      /* Create one frame holding color options */
      frame = ctk_frame_new ("Color Options");
      ctk_box_pack_start (CTK_BOX (vbox), frame, TRUE, TRUE, 0);

      table = ctk_grid_new ();
      ctk_container_set_border_width (CTK_CONTAINER (table), 5);
      ctk_grid_set_row_spacing (CTK_GRID (table), 5);
      ctk_grid_set_column_spacing (CTK_GRID (table), 10);
      ctk_container_add (CTK_CONTAINER (frame), table);

      add_row (CTK_GRID (table), 0, size_group, "_Foreground", color_options);
      add_row (CTK_GRID (table), 1, size_group, "_Background", color_options);

      /* And another frame holding line style options */
      frame = ctk_frame_new ("Line Options");
      ctk_box_pack_start (CTK_BOX (vbox), frame, FALSE, FALSE, 0);

      table = ctk_grid_new ();
      ctk_container_set_border_width (CTK_CONTAINER (table), 5);
      ctk_grid_set_row_spacing (CTK_GRID (table), 5);
      ctk_grid_set_column_spacing (CTK_GRID (table), 10);
      ctk_container_add (CTK_CONTAINER (frame), table);

      add_row (CTK_GRID (table), 0, size_group, "_Dashing", dash_options);
      add_row (CTK_GRID (table), 1, size_group, "_Line ends", end_options);

      /* And a check button to turn grouping on and off */
      check_button = ctk_check_button_new_with_mnemonic ("_Enable grouping");
      ctk_box_pack_start (CTK_BOX (vbox), check_button, FALSE, FALSE, 0);

      ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (check_button), TRUE);
      g_signal_connect (check_button, "toggled",
                        G_CALLBACK (toggle_grouping), size_group);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);

  return window;
}
