/* Overlay/Interactive Overlay
 *
 * Shows widgets in static positions over a main widget.
 *
 * The overlayed widgets can be interactive controls such
 * as the entry in this example, or just decorative, like
 * the big blue label.
 */

#include <ctk/ctk.h>

static void
do_number (CtkButton *button, CtkEntry *entry)
{
  ctk_entry_set_text (entry, ctk_button_get_label (button));
}

CtkWidget *
do_overlay (CtkWidget *do_widget G_GNUC_UNUSED)
{
  static CtkWidget *window = NULL;

  if (!window)
    {
      CtkWidget *overlay;
      CtkWidget *grid;
      CtkWidget *button;
      CtkWidget *vbox;
      CtkWidget *label;
      CtkWidget *entry;
      int i, j;
      char *text;

      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_default_size (CTK_WINDOW (window), 500, 510);
      ctk_window_set_title (CTK_WINDOW (window), "Interactive Overlay");

      overlay = ctk_overlay_new ();
      grid = ctk_grid_new ();
      ctk_container_add (CTK_CONTAINER (overlay), grid);

      entry = ctk_entry_new ();

      for (j = 0; j < 5; j++)
        {
          for (i = 0; i < 5; i++)
            {
              text = g_strdup_printf ("%d", 5*j + i);
              button = ctk_button_new_with_label (text);
              g_free (text);
              ctk_widget_set_hexpand (button, TRUE);
              ctk_widget_set_vexpand (button, TRUE);
              g_signal_connect (button, "clicked", G_CALLBACK (do_number), entry);
              ctk_grid_attach (CTK_GRID (grid), button, i, j, 1, 1);
            }
        }

      vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 10);
      ctk_overlay_add_overlay (CTK_OVERLAY (overlay), vbox);
      ctk_overlay_set_overlay_pass_through (CTK_OVERLAY (overlay), vbox, TRUE);
      ctk_widget_set_halign (vbox, CTK_ALIGN_CENTER);
      ctk_widget_set_valign (vbox, CTK_ALIGN_CENTER);

      label = ctk_label_new ("<span foreground='blue' weight='ultrabold' font='40'>Numbers</span>");
      ctk_label_set_use_markup (CTK_LABEL (label), TRUE);
      ctk_box_pack_start (CTK_BOX (vbox), label, FALSE, FALSE, 8);

      ctk_entry_set_placeholder_text (CTK_ENTRY (entry), "Your Lucky Number");
      ctk_box_pack_start (CTK_BOX (vbox), entry, FALSE, FALSE, 8);

      ctk_container_add (CTK_CONTAINER (window), overlay);

      g_signal_connect (window, "destroy",
                        G_CALLBACK (ctk_widget_destroyed), &window);

      ctk_widget_show_all (overlay);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show (window);
  else
    ctk_widget_destroy (window);

  return window;
}
