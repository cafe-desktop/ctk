#include <ctk/ctk.h>

static void
row_unrevealed (GObject *revealer, GParamSpec *pspec, gpointer data)
{
  CtkWidget *row, *list;

  row = ctk_widget_get_parent (CTK_WIDGET (revealer));
  list = ctk_widget_get_parent (row);

  ctk_container_remove (CTK_CONTAINER (list), row);
}

static void
remove_this_row (CtkButton *button, CtkWidget *child)
{
  CtkWidget *row, *revealer;

  row = ctk_widget_get_parent (child);
  revealer = ctk_revealer_new ();
  ctk_revealer_set_reveal_child (CTK_REVEALER (revealer), TRUE);
  ctk_widget_show (revealer);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  ctk_widget_reparent (child, revealer);
G_GNUC_END_IGNORE_DEPRECATIONS
  ctk_container_add (CTK_CONTAINER (row), revealer);
  g_signal_connect (revealer, "notify::child-revealed",
                    G_CALLBACK (row_unrevealed), NULL);
  ctk_revealer_set_reveal_child (CTK_REVEALER (revealer), FALSE);
}

static CtkWidget *create_row (const gchar *label);

static void
row_revealed (GObject *revealer, GParamSpec *pspec, gpointer data)
{
  CtkWidget *row, *child;

  row = ctk_widget_get_parent (CTK_WIDGET (revealer));
  child = ctk_bin_get_child (CTK_BIN (revealer));
  g_object_ref (child);
  ctk_container_remove (CTK_CONTAINER (revealer), child);
  ctk_widget_destroy (CTK_WIDGET (revealer));
  ctk_container_add (CTK_CONTAINER (row), child);
  g_object_unref (child);
}

static void
add_row_below (CtkButton *button, CtkWidget *child)
{
  CtkWidget *revealer, *row, *list;
  gint index;

  row = ctk_widget_get_parent (child);
  index = ctk_list_box_row_get_index (CTK_LIST_BOX_ROW (row));
  list = ctk_widget_get_parent (row);
  row = create_row ("Extra row");
  revealer = ctk_revealer_new ();
  ctk_container_add (CTK_CONTAINER (revealer), row);
  ctk_widget_show_all (revealer);
  g_signal_connect (revealer, "notify::child-revealed",
                    G_CALLBACK (row_revealed), NULL);
  ctk_list_box_insert (CTK_LIST_BOX (list), revealer, index + 1);
  ctk_revealer_set_reveal_child (CTK_REVEALER (revealer), TRUE);
}

static void
add_separator (CtkListBoxRow *row, CtkListBoxRow *before, gpointer data)
{
  if (!before)
    return;

  ctk_list_box_row_set_header (row, ctk_separator_new (CTK_ORIENTATION_HORIZONTAL));
}

static CtkWidget *
create_row (const gchar *text)
{
  CtkWidget *row, *label, *button;

  row = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 10);
  label = ctk_label_new (text);
  ctk_container_add (CTK_CONTAINER (row), label);
  button = ctk_button_new_with_label ("x");
  ctk_widget_set_hexpand (button, TRUE);
  ctk_widget_set_halign (button, CTK_ALIGN_END);
  ctk_widget_set_valign (button, CTK_ALIGN_CENTER);
  ctk_container_add (CTK_CONTAINER (row), button);
  g_signal_connect (button, "clicked", G_CALLBACK (remove_this_row), row);
  button = ctk_button_new_with_label ("+");
  ctk_widget_set_valign (button, CTK_ALIGN_CENTER);
  ctk_container_add (CTK_CONTAINER (row), button);
  g_signal_connect (button, "clicked", G_CALLBACK (add_row_below), row);

  return row;
}

int main (int argc, char *argv[])
{
  CtkWidget *window, *list, *sw;
  gint i;
 
  ctk_init (NULL, NULL);

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_default_size (CTK_WINDOW (window), 300, 300);

  list = ctk_list_box_new ();
  ctk_list_box_set_selection_mode (CTK_LIST_BOX (list), CTK_SELECTION_NONE);
  ctk_list_box_set_header_func (CTK_LIST_BOX (list), add_separator, NULL, NULL);
  sw = ctk_scrolled_window_new (NULL, NULL);
  ctk_container_add (CTK_CONTAINER (window), sw);
  ctk_container_add (CTK_CONTAINER (sw), list);

  for (i = 0; i < 20; i++)
    {
      CtkWidget *row;
      gchar *text;

      text = g_strdup_printf ("Row %d", i);
      row = create_row (text);
      ctk_list_box_insert (CTK_LIST_BOX (list), row, -1);
    }

  ctk_widget_show_all (window);

  ctk_main ();

  return 0;
}
