/* Entry/Entry Buffer
 *
 * CtkEntryBuffer provides the text content in a CtkEntry.
 * Applications can provide their own buffer implementation,
 * e.g. to provide secure handling for passwords in memory.
 */

#include <glib/gi18n.h>
#include <ctk/ctk.h>

CtkWidget *
do_entry_buffer (CtkWidget *do_widget)
{
  static CtkWidget *window = NULL;
  CtkWidget *vbox;
  CtkWidget *label;
  CtkWidget *entry;
  CtkEntryBuffer *buffer;

  if (!window)
    {
      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_screen (CTK_WINDOW (window),
                             ctk_widget_get_screen (do_widget));
      ctk_window_set_title (CTK_WINDOW (window), "Entry Buffer");
      ctk_window_set_resizable (CTK_WINDOW (window), FALSE);
      g_signal_connect (window, "destroy",
                        G_CALLBACK (ctk_widget_destroyed), &window);

      vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 5);
      ctk_container_add (CTK_CONTAINER (window), vbox);
      ctk_container_set_border_width (CTK_CONTAINER (vbox), 5);

      label = ctk_label_new (NULL);
      ctk_label_set_markup (CTK_LABEL (label),
                            "Entries share a buffer. Typing in one is reflected in the other.");
      ctk_box_pack_start (CTK_BOX (vbox), label, FALSE, FALSE, 0);

      /* Create a buffer */
      buffer = ctk_entry_buffer_new (NULL, 0);

      /* Create our first entry */
      entry = ctk_entry_new_with_buffer (buffer);
      ctk_box_pack_start (CTK_BOX (vbox), entry, FALSE, FALSE, 0);

      /* Create the second entry */
      entry = ctk_entry_new_with_buffer (buffer);
      ctk_entry_set_visibility (CTK_ENTRY (entry), FALSE);
      ctk_box_pack_start (CTK_BOX (vbox), entry, FALSE, FALSE, 0);

      g_object_unref (buffer);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);

  return window;
}
