#include <gtk/gtk.h>

static void
row_unrevealed (GObject *revealer, GParamSpec *pspec, gpointer data)
{
  GtkWidget *row, *list;

  row = ctk_widget_get_parent (GTK_WIDGET (revealer));
  list = ctk_widget_get_parent (row);

  ctk_container_remove (GTK_CONTAINER (list), row);
}

static void
remove_this_row (GtkButton *button, GtkWidget *child)
{
  GtkWidget *row, *revealer;

  row = ctk_widget_get_parent (child);
  revealer = ctk_revealer_new ();
  ctk_revealer_set_reveal_child (GTK_REVEALER (revealer), TRUE);
  ctk_widget_show (revealer);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  ctk_widget_reparent (child, revealer);
G_GNUC_END_IGNORE_DEPRECATIONS
  ctk_container_add (GTK_CONTAINER (row), revealer);
  g_signal_connect (revealer, "notify::child-revealed",
                    G_CALLBACK (row_unrevealed), NULL);
  ctk_revealer_set_reveal_child (GTK_REVEALER (revealer), FALSE);
}

static GtkWidget *create_row (const gchar *label);

static void
row_revealed (GObject *revealer, GParamSpec *pspec, gpointer data)
{
  GtkWidget *row, *child;

  row = ctk_widget_get_parent (GTK_WIDGET (revealer));
  child = ctk_bin_get_child (GTK_BIN (revealer));
  g_object_ref (child);
  ctk_container_remove (GTK_CONTAINER (revealer), child);
  ctk_widget_destroy (GTK_WIDGET (revealer));
  ctk_container_add (GTK_CONTAINER (row), child);
  g_object_unref (child);
}

static void
add_row_below (GtkButton *button, GtkWidget *child)
{
  GtkWidget *revealer, *row, *list;
  gint index;

  row = ctk_widget_get_parent (child);
  index = ctk_list_box_row_get_index (GTK_LIST_BOX_ROW (row));
  list = ctk_widget_get_parent (row);
  row = create_row ("Extra row");
  revealer = ctk_revealer_new ();
  ctk_container_add (GTK_CONTAINER (revealer), row);
  ctk_widget_show_all (revealer);
  g_signal_connect (revealer, "notify::child-revealed",
                    G_CALLBACK (row_revealed), NULL);
  ctk_list_box_insert (GTK_LIST_BOX (list), revealer, index + 1);
  ctk_revealer_set_reveal_child (GTK_REVEALER (revealer), TRUE);
}

static void
add_separator (GtkListBoxRow *row, GtkListBoxRow *before, gpointer data)
{
  if (!before)
    return;

  ctk_list_box_row_set_header (row, ctk_separator_new (GTK_ORIENTATION_HORIZONTAL));
}

static GtkWidget *
create_row (const gchar *text)
{
  GtkWidget *row, *label, *button;

  row = ctk_box_new (GTK_ORIENTATION_HORIZONTAL, 10);
  label = ctk_label_new (text);
  ctk_container_add (GTK_CONTAINER (row), label);
  button = ctk_button_new_with_label ("x");
  ctk_widget_set_hexpand (button, TRUE);
  ctk_widget_set_halign (button, GTK_ALIGN_END);
  ctk_widget_set_valign (button, GTK_ALIGN_CENTER);
  ctk_container_add (GTK_CONTAINER (row), button);
  g_signal_connect (button, "clicked", G_CALLBACK (remove_this_row), row);
  button = ctk_button_new_with_label ("+");
  ctk_widget_set_valign (button, GTK_ALIGN_CENTER);
  ctk_container_add (GTK_CONTAINER (row), button);
  g_signal_connect (button, "clicked", G_CALLBACK (add_row_below), row);

  return row;
}

int main (int argc, char *argv[])
{
  GtkWidget *window, *list, *sw, *row;
  gint i;
  gchar *text;

  ctk_init (NULL, NULL);

  window = ctk_window_new (GTK_WINDOW_TOPLEVEL);
  ctk_window_set_default_size (GTK_WINDOW (window), 300, 300);

  list = ctk_list_box_new ();
  ctk_list_box_set_selection_mode (GTK_LIST_BOX (list), GTK_SELECTION_NONE);
  ctk_list_box_set_header_func (GTK_LIST_BOX (list), add_separator, NULL, NULL);
  sw = ctk_scrolled_window_new (NULL, NULL);
  ctk_container_add (GTK_CONTAINER (window), sw);
  ctk_container_add (GTK_CONTAINER (sw), list);

  for (i = 0; i < 20; i++)
    {
      text = g_strdup_printf ("Row %d", i);
      row = create_row (text);
      ctk_list_box_insert (GTK_LIST_BOX (list), row, -1);
    }

  ctk_widget_show_all (window);

  ctk_main ();

  return 0;
}
