/* Entry/Entry Completion
 *
 * GtkEntryCompletion provides a mechanism for adding support for
 * completion in GtkEntry.
 *
 */

#include <glib/gi18n.h>
#include <ctk/ctk.h>

/* Creates a tree model containing the completions */
GtkTreeModel *
create_completion_model (void)
{
  GtkListStore *store;
  GtkTreeIter iter;

  store = ctk_list_store_new (1, G_TYPE_STRING);

  /* Append one word */
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 0, "GNOME", -1);

  /* Append another word */
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 0, "total", -1);

  /* And another word */
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 0, "totally", -1);

  return CTK_TREE_MODEL (store);
}


GtkWidget *
do_entry_completion (GtkWidget *do_widget)
{
  static GtkWidget *window = NULL;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *entry;
  GtkEntryCompletion *completion;
  GtkTreeModel *completion_model;

  if (!window)
    {
      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_screen (CTK_WINDOW (window),
                             ctk_widget_get_screen (do_widget));
      ctk_window_set_title (CTK_WINDOW (window), "Entry Completion");
      ctk_window_set_resizable (CTK_WINDOW (window), FALSE);

      g_signal_connect (window, "destroy",
                        G_CALLBACK (ctk_widget_destroyed), &window);

      vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 5);
      ctk_container_add (CTK_CONTAINER (window), vbox);
      ctk_container_set_border_width (CTK_CONTAINER (vbox), 5);

      label = ctk_label_new (NULL);
      ctk_label_set_markup (CTK_LABEL (label), "Completion demo, try writing <b>total</b> or <b>gnome</b> for example.");
      ctk_box_pack_start (CTK_BOX (vbox), label, FALSE, FALSE, 0);

      /* Create our entry */
      entry = ctk_entry_new ();
      ctk_box_pack_start (CTK_BOX (vbox), entry, FALSE, FALSE, 0);

      /* Create the completion object */
      completion = ctk_entry_completion_new ();

      /* Assign the completion to the entry */
      ctk_entry_set_completion (CTK_ENTRY (entry), completion);
      g_object_unref (completion);

      /* Create a tree model and use it as the completion model */
      completion_model = create_completion_model ();
      ctk_entry_completion_set_model (completion, completion_model);
      g_object_unref (completion_model);

      /* Use model column 0 as the text column */
      ctk_entry_completion_set_text_column (completion, 0);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);

  return window;
}
