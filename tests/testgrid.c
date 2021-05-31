#include <gtk/gtk.h>

static GtkWidget *
oriented_test_widget (const gchar *label, const gchar *color, gdouble angle)
{
  GtkWidget *box;
  GtkWidget *widget;
  GtkCssProvider *provider;
  gchar *data;

  widget = ctk_label_new (label);
  ctk_label_set_angle (CTK_LABEL (widget), angle);
  box = ctk_event_box_new ();
  provider = ctk_css_provider_new ();
  data = g_strdup_printf ("GtkEventBox { background-color: %s; }", color);
  ctk_css_provider_load_from_data (provider, data, -1, NULL);
  ctk_style_context_add_provider (ctk_widget_get_style_context (box),
                                  CTK_STYLE_PROVIDER (provider),
                                  CTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_free (data);
  g_object_unref (provider);
  ctk_container_add (CTK_CONTAINER (box), widget);

  return box;
}

static GtkWidget *
test_widget (const gchar *label, const gchar *color)
{
  return oriented_test_widget (label, color, 0.0);
}

static GtkOrientation o;

static gboolean
toggle_orientation (GtkWidget *window, GdkEventButton *event, GtkGrid *grid)
{
  o = 1 - o;

  ctk_orientable_set_orientation (CTK_ORIENTABLE (grid), o);

  return FALSE;
}

static void
simple_grid (void)
{
  GtkWidget *window;
  GtkWidget *grid;
  GtkWidget *test1, *test2, *test3, *test4, *test5, *test6;

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_title (CTK_WINDOW (window), "Orientation");
  grid = ctk_grid_new ();
  ctk_container_add (CTK_CONTAINER (window), grid);
  g_signal_connect (window, "button-press-event", G_CALLBACK (toggle_orientation), grid);

  ctk_grid_set_column_spacing (CTK_GRID (grid), 5);
  ctk_grid_set_row_spacing (CTK_GRID (grid), 5);
  test1 = test_widget ("1", "red");
  ctk_container_add (CTK_CONTAINER (grid), test1);
  test2 = test_widget ("2", "green");
  ctk_container_add (CTK_CONTAINER (grid), test2);
  test3 = test_widget ("3", "blue");
  ctk_container_add (CTK_CONTAINER (grid), test3);
  test4 = test_widget ("4", "green");
  ctk_grid_attach (CTK_GRID (grid), test4, 0, 1, 1, 1);
  ctk_widget_set_vexpand (test4, TRUE);
  test5 = test_widget ("5", "blue");
  ctk_grid_attach_next_to (CTK_GRID (grid), test5, test4, CTK_POS_RIGHT, 2, 1);
  test6 = test_widget ("6", "yellow");
  ctk_grid_attach (CTK_GRID (grid), test6, -1, 0, 1, 2);
  ctk_widget_set_hexpand (test6, TRUE);

  g_assert (ctk_grid_get_child_at (CTK_GRID (grid), 0, -1) == NULL);
  g_assert (ctk_grid_get_child_at (CTK_GRID (grid), 0, 0) == test1);
  g_assert (ctk_grid_get_child_at (CTK_GRID (grid), 1, 0) == test2);
  g_assert (ctk_grid_get_child_at (CTK_GRID (grid), 0, 1) == test4);
  g_assert (ctk_grid_get_child_at (CTK_GRID (grid), -1, 0) == test6);
  g_assert (ctk_grid_get_child_at (CTK_GRID (grid), -1, 1) == test6);
  g_assert (ctk_grid_get_child_at (CTK_GRID (grid), -1, 2) == NULL);
  ctk_widget_show_all (window);
}

static void
text_grid (void)
{
  GtkWidget *window;
  GtkWidget *grid;
  GtkWidget *paned1;
  GtkWidget *box;
  GtkWidget *label;

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_title (CTK_WINDOW (window), "Height-for-Width");
  paned1 = ctk_paned_new (CTK_ORIENTATION_HORIZONTAL);
  ctk_container_add (CTK_CONTAINER (window), paned1);

  box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  ctk_paned_pack1 (CTK_PANED (paned1), box, TRUE, FALSE);
  ctk_paned_pack2 (CTK_PANED (paned1), ctk_label_new ("Space"), TRUE, FALSE);

  grid = ctk_grid_new ();
  ctk_orientable_set_orientation (CTK_ORIENTABLE (grid), CTK_ORIENTATION_VERTICAL);
  ctk_container_add (CTK_CONTAINER (box), ctk_label_new ("Above"));
  ctk_container_add (CTK_CONTAINER (box), ctk_separator_new (CTK_ORIENTATION_HORIZONTAL));
  ctk_container_add (CTK_CONTAINER (box), grid);
  ctk_container_add (CTK_CONTAINER (box), ctk_separator_new (CTK_ORIENTATION_HORIZONTAL));
  ctk_container_add (CTK_CONTAINER (box), ctk_label_new ("Below"));

  label = ctk_label_new ("Some text that may wrap if it has to");
  ctk_label_set_width_chars (CTK_LABEL (label), 10);
  ctk_label_set_line_wrap (CTK_LABEL (label), TRUE);
  ctk_grid_attach (CTK_GRID (grid), label, 0, 0, 1, 1);

  ctk_grid_attach (CTK_GRID (grid), test_widget ("1", "red"), 1, 0, 1, 1);
  ctk_grid_attach (CTK_GRID (grid), test_widget ("2", "blue"), 0, 1, 1, 1);

  label = ctk_label_new ("Some text that may wrap if it has to");
  ctk_label_set_ellipsize (CTK_LABEL (label), PANGO_ELLIPSIZE_END);
  ctk_label_set_width_chars (CTK_LABEL (label), 10);
  ctk_grid_attach (CTK_GRID (grid), label, 1, 1, 1, 1);

  ctk_widget_show_all (window);
}

static void
box_comparison (void)
{
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *box;
  GtkWidget *label;
  GtkWidget *grid;

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_title (CTK_WINDOW (window), "Grid vs. Box");
  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 5);
  ctk_container_add (CTK_CONTAINER (window), vbox);

  ctk_container_add (CTK_CONTAINER (vbox), ctk_label_new ("Above"));
  ctk_container_add (CTK_CONTAINER (vbox), ctk_separator_new (CTK_ORIENTATION_HORIZONTAL));

  box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
  ctk_container_add (CTK_CONTAINER (vbox), box);

  ctk_box_pack_start (CTK_BOX (box), test_widget ("1", "white"), FALSE, FALSE, 0);

  label = ctk_label_new ("Some ellipsizing text");
  ctk_label_set_ellipsize (CTK_LABEL (label), PANGO_ELLIPSIZE_END);
  ctk_label_set_width_chars (CTK_LABEL (label), 10);
  ctk_box_pack_start (CTK_BOX (box), label, TRUE, FALSE, 0);

  ctk_box_pack_start (CTK_BOX (box), test_widget ("2", "green"), FALSE, FALSE, 0);

  label = ctk_label_new ("Some text that may wrap if needed");
  ctk_label_set_line_wrap (CTK_LABEL (label), TRUE);
  ctk_label_set_width_chars (CTK_LABEL (label), 10);
  ctk_box_pack_start (CTK_BOX (box), label, TRUE, FALSE, 0);

  ctk_box_pack_start (CTK_BOX (box), test_widget ("3", "red"), FALSE, FALSE, 0);

  grid = ctk_grid_new ();
  ctk_orientable_set_orientation (CTK_ORIENTABLE (grid), CTK_ORIENTATION_VERTICAL);
  ctk_container_add (CTK_CONTAINER (vbox), ctk_separator_new (CTK_ORIENTATION_HORIZONTAL));
  ctk_container_add (CTK_CONTAINER (vbox), grid);

  ctk_grid_attach (CTK_GRID (grid), test_widget ("1", "white"), 0, 0, 1, 1);

  label = ctk_label_new ("Some ellipsizing text");
  ctk_label_set_ellipsize (CTK_LABEL (label), PANGO_ELLIPSIZE_END);
  ctk_label_set_width_chars (CTK_LABEL (label), 10);
  ctk_grid_attach (CTK_GRID (grid), label, 1, 0, 1, 1);
  ctk_widget_set_hexpand (label, TRUE);

  ctk_grid_attach (CTK_GRID (grid), test_widget ("2", "green"), 2, 0, 1, 1);

  label = ctk_label_new ("Some text that may wrap if needed");
  ctk_label_set_line_wrap (CTK_LABEL (label), TRUE);
  ctk_label_set_width_chars (CTK_LABEL (label), 10);
  ctk_grid_attach (CTK_GRID (grid), label, 3, 0, 1, 1);
  ctk_widget_set_hexpand (label, TRUE);

  ctk_grid_attach (CTK_GRID (grid), test_widget ("3", "red"), 4, 0, 1, 1);

  ctk_container_add (CTK_CONTAINER (vbox), ctk_separator_new (CTK_ORIENTATION_HORIZONTAL));
  ctk_container_add (CTK_CONTAINER (vbox), ctk_label_new ("Below"));

  ctk_widget_show_all (window);
}

static void
empty_line (void)
{
  GtkWidget *window;
  GtkWidget *grid;
  GtkWidget *child;

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_title (CTK_WINDOW (window), "Empty row");
  grid = ctk_grid_new ();
  ctk_container_add (CTK_CONTAINER (window), grid);

  ctk_grid_set_row_spacing (CTK_GRID (grid), 10);
  ctk_grid_set_column_spacing (CTK_GRID (grid), 10);

  child = test_widget ("(0, 0)", "red");
  ctk_grid_attach (CTK_GRID (grid), child, 0, 0, 1, 1);
  ctk_widget_set_hexpand (child, TRUE);
  ctk_widget_set_vexpand (child, TRUE);

  ctk_grid_attach (CTK_GRID (grid), test_widget ("(0, 1)", "blue"), 0, 1, 1, 1);

  ctk_grid_attach (CTK_GRID (grid), test_widget ("(10, 0)", "green"), 10, 0, 1, 1);
  ctk_grid_attach (CTK_GRID (grid), test_widget ("(10, 1)", "magenta"), 10, 1, 1, 1);

  ctk_widget_show_all (window);
}

static void
empty_grid (void)
{
  GtkWidget *window;
  GtkWidget *grid;
  GtkWidget *child;

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_title (CTK_WINDOW (window), "Empty grid");
  grid = ctk_grid_new ();
  ctk_container_add (CTK_CONTAINER (window), grid);

  ctk_grid_set_row_spacing (CTK_GRID (grid), 10);
  ctk_grid_set_column_spacing (CTK_GRID (grid), 10);
  ctk_grid_set_row_homogeneous (CTK_GRID (grid), TRUE);

  child = test_widget ("(0, 0)", "red");
  ctk_grid_attach (CTK_GRID (grid), child, 0, 0, 1, 1);
  ctk_widget_set_hexpand (child, TRUE);
  ctk_widget_set_vexpand (child, TRUE);

  ctk_widget_show_all (window);
  ctk_widget_hide (child);
}

static void
scrolling (void)
{
  GtkWidget *window;
  GtkWidget *sw;
  GtkWidget *viewport;
  GtkWidget *grid;
  GtkWidget *child;
  gint i;

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_title (CTK_WINDOW (window), "Scrolling");
  sw = ctk_scrolled_window_new (NULL, NULL);
  viewport = ctk_viewport_new (NULL, NULL);
  grid = ctk_grid_new ();

  ctk_container_add (CTK_CONTAINER (window), sw);
  ctk_container_add (CTK_CONTAINER (sw), viewport);
  ctk_container_add (CTK_CONTAINER (viewport), grid);

  child = oriented_test_widget ("#800080", "#800080", -45.0);
  ctk_grid_attach (CTK_GRID (grid), child, 0, 0, 1, 1);
  ctk_widget_set_hexpand (child, TRUE);
  ctk_widget_set_vexpand (child, TRUE);

  for (i = 1; i < 16; i++)
    {
      gchar *color;
      color = g_strdup_printf ("#%02x00%02x", 128 + 8*i, 128 - 8*i);
      child = test_widget (color, color);
      ctk_grid_attach (CTK_GRID (grid), child, 0, i, i + 1, 1);
      ctk_widget_set_hexpand (child, TRUE);
      g_free (color);
    }

  for (i = 1; i < 16; i++)
    {
      gchar *color;
      color = g_strdup_printf ("#%02x00%02x", 128 - 8*i, 128 + 8*i);
      child = oriented_test_widget (color, color, -90.0);
      ctk_grid_attach (CTK_GRID (grid), child, i, 0, 1, i);
      ctk_widget_set_vexpand (child, TRUE);
      g_free (color);
    }

  ctk_widget_show_all (window);
}

static void
insert_cb (GtkButton *button, GtkWidget *window)
{
  GtkGrid *g, *g1, *g2, *g3, *g4;
  GtkWidget *child;
  gboolean inserted;

  g = CTK_GRID (ctk_bin_get_child (CTK_BIN (window)));
  g1 = CTK_GRID (ctk_grid_get_child_at (g, 0, 0));
  g2 = CTK_GRID (ctk_grid_get_child_at (g, 1, 0));
  g3 = CTK_GRID (ctk_grid_get_child_at (g, 0, 1));
  g4 = CTK_GRID (ctk_grid_get_child_at (g, 1, 1));

  inserted = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button), "inserted"));

  if (inserted)
    {
      ctk_grid_remove_row (g1, 1);
      ctk_grid_remove_column (g2, 1);
      ctk_grid_remove_row (g3, 1);
      ctk_grid_remove_column (g4, 1);
    }
  else
    {
      ctk_grid_insert_row (g1, 1);
      ctk_grid_attach (g1, test_widget ("(0, 1)", "red"), 0, 1, 1, 1);
      ctk_grid_attach (g1, test_widget ("(2, 1)", "red"), 2, 1, 1, 1);

      ctk_grid_insert_column (g2, 1);
      ctk_grid_attach (g2, test_widget ("(1, 0)", "red"), 1, 0, 1, 1);
      ctk_grid_attach (g2, test_widget ("(1, 2)", "red"), 1, 2, 1, 1);

      child = ctk_grid_get_child_at (g3, 0, 0);
      ctk_grid_insert_next_to (g3, child, CTK_POS_BOTTOM);
      ctk_grid_attach (g3, test_widget ("(0, 1)", "red"), 0, 1, 1, 1);
      ctk_grid_attach (g3, test_widget ("(2, 1)", "red"), 2, 1, 1, 1);

      child = ctk_grid_get_child_at (g4, 0, 0);
      ctk_grid_insert_next_to (g4, child, CTK_POS_RIGHT);
      ctk_grid_attach (g4, test_widget ("(1, 0)", "red"), 1, 0, 1, 1);
      ctk_grid_attach (g4, test_widget ("(1, 2)", "red"), 1, 2, 1, 1);

      ctk_widget_show_all (CTK_WIDGET (g));
    }

  ctk_button_set_label (button, inserted ? "Insert" : "Remove");
  g_object_set_data (G_OBJECT (button), "inserted", GINT_TO_POINTER (!inserted));
}

static void
insert (void)
{
  GtkWidget *window;
  GtkWidget *g;
  GtkWidget *grid;
  GtkWidget *child;
  GtkWidget *button;

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_title (CTK_WINDOW (window), "Insertion / Removal");

  g = ctk_grid_new ();
  ctk_grid_set_row_spacing (CTK_GRID (g), 10);
  ctk_grid_set_column_spacing (CTK_GRID (g), 10);
  ctk_container_add (CTK_CONTAINER (window), g);

  grid = ctk_grid_new ();
  ctk_grid_attach (CTK_GRID (g), grid, 0, 0, 1, 1);

  ctk_grid_attach (CTK_GRID (grid), test_widget ("(0, 0)", "blue"), 0, 0, 1, 1);
  ctk_grid_attach (CTK_GRID (grid), test_widget ("(0, 1)", "blue"), 0, 1, 1, 1);
  ctk_grid_attach (CTK_GRID (grid), test_widget ("(1, 0)", "green"), 1, 0, 1, 2);
  ctk_grid_attach (CTK_GRID (grid), test_widget ("(2, 0)", "yellow"), 2, 0, 1, 1);
  ctk_grid_attach (CTK_GRID (grid), test_widget ("(2, 1)", "yellow"), 2, 1, 1, 1);

  grid = ctk_grid_new ();
  ctk_grid_attach (CTK_GRID (g), grid, 1, 0, 1, 1);

  ctk_grid_attach (CTK_GRID (grid), test_widget ("(0, 0)", "blue"), 0, 0, 1, 1);
  ctk_grid_attach (CTK_GRID (grid), test_widget ("(1, 0)", "blue"), 1, 0, 1, 1);
  ctk_grid_attach (CTK_GRID (grid), test_widget ("(0, 1)", "green"), 0, 1, 2, 1);
  ctk_grid_attach (CTK_GRID (grid), test_widget ("(0, 2)", "yellow"), 0, 2, 1, 1);
  ctk_grid_attach (CTK_GRID (grid), test_widget ("(1, 2)", "yellow"), 1, 2, 1, 1);

  grid = ctk_grid_new ();
  ctk_grid_attach (CTK_GRID (g), grid, 0, 1, 1, 1);

  child = test_widget ("(0, 0)", "blue");
  ctk_grid_attach (CTK_GRID (grid), child, 0, 0, 1, 1);
  ctk_grid_attach (CTK_GRID (grid), test_widget ("(0, 1)", "blue"), 0, 1, 1, 1);
  ctk_grid_attach (CTK_GRID (grid), test_widget ("(1, 0)", "green"), 1, 0, 1, 2);
  ctk_grid_attach (CTK_GRID (grid), test_widget ("(2, 0)", "yellow"), 2, 0, 1, 1);
  ctk_grid_attach (CTK_GRID (grid), test_widget ("(2, 1)", "yellow"), 2, 1, 1, 1);

  grid = ctk_grid_new ();
  ctk_grid_attach (CTK_GRID (g), grid, 1, 1, 1, 1);

  child = test_widget ("(0, 0)", "blue");
  ctk_grid_attach (CTK_GRID (grid), child, 0, 0, 1, 1);
  ctk_grid_attach (CTK_GRID (grid), test_widget ("(1, 0)", "blue"), 1, 0, 1, 1);
  ctk_grid_attach (CTK_GRID (grid), test_widget ("(0, 1)", "green"), 0, 1, 2, 1);
  ctk_grid_attach (CTK_GRID (grid), test_widget ("(0, 2)", "yellow"), 0, 2, 1, 1);
  ctk_grid_attach (CTK_GRID (grid), test_widget ("(1, 2)", "yellow"), 1, 2, 1, 1);

  button = ctk_button_new_with_label ("Insert");
  g_signal_connect (button, "clicked", G_CALLBACK (insert_cb), window);
  ctk_grid_attach (CTK_GRID (g), button, 0, 2, 2, 1);

  ctk_widget_show_all (window);
}

static void
spanning_grid (void)
{
  GtkWidget *window;
  GtkWidget *g;
  GtkWidget *c;

  /* inspired by bug 698660
   * the row/column that are empty except for the spanning
   * child need to stay collapsed
   */

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_title (CTK_WINDOW (window), "Spanning");

  g = ctk_grid_new ();
  ctk_container_add (CTK_CONTAINER (window), g);

  c = test_widget ("0", "blue");
  ctk_widget_set_hexpand (c, TRUE);
  ctk_grid_attach (CTK_GRID (g), c, 0, 4, 4, 1);

  c = test_widget ("1", "green");
  ctk_widget_set_vexpand (c, TRUE);
  ctk_grid_attach (CTK_GRID (g), c, 4, 0, 1, 4);

  c = test_widget ("2", "red");
  ctk_widget_set_hexpand (c, TRUE);
  ctk_widget_set_vexpand (c, TRUE);
  ctk_grid_attach (CTK_GRID (g), c, 3, 3, 1, 1);

  c = test_widget ("3", "yellow");
  ctk_grid_attach (CTK_GRID (g), c, 0, 3, 2, 1);

  c = test_widget ("4", "orange");
  ctk_grid_attach (CTK_GRID (g), c, 3, 0, 1, 2);

  c = test_widget ("5", "purple");
  ctk_grid_attach (CTK_GRID (g), c, 1, 1, 1, 1);

  c = test_widget ("6", "white");
  ctk_grid_attach (CTK_GRID (g), c, 0, 1, 1, 1);

  c = test_widget ("7", "cyan");
  ctk_grid_attach (CTK_GRID (g), c, 1, 0, 1, 1);

  ctk_widget_show_all (window);
}

int
main (int argc, char *argv[])
{
  ctk_init (NULL, NULL);

  if (g_getenv ("RTL"))
    ctk_widget_set_default_direction (CTK_TEXT_DIR_RTL);

  simple_grid ();
  text_grid ();
  box_comparison ();
  empty_line ();
  scrolling ();
  insert ();
  empty_grid ();
  spanning_grid ();

  ctk_main ();

  return 0;
}
