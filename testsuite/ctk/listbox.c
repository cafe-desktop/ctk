#include <ctk/ctk.h>

static gint
sort_list (CtkListBoxRow *row1,
           CtkListBoxRow *row2,
           gpointer       data)
{
  CtkWidget *label1, *label2;
  gint n1, n2;
  gint *count = data;

  (*count)++;

  label1 = ctk_bin_get_child (CTK_BIN (row1));
  n1 = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (label1), "data"));

  label2 = ctk_bin_get_child (CTK_BIN (row2));
  n2 = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (label2), "data"));

  return (n1 - n2);
}

static void
check_sorted (CtkListBox *list)
{
  GList *children;
  CtkWidget *row, *label;
  gint n1, n2;
  GList *l;

  n2 = n1 = 0;
  children = ctk_container_get_children (CTK_CONTAINER (list));
  for (l = children; l; l = l->next)
    {
      row = l->data;
      n1 = n2;
      label = ctk_bin_get_child (CTK_BIN (row));
      n2 = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (label), "data"));
      g_assert_cmpint (n1, <=, n2);
    }
  g_list_free (children);
}

static void
test_sort (void)
{
  CtkListBox *list;
  CtkListBoxRow *row;
  CtkWidget *label;
  gint i, r;
  gchar *s;
  gint count;

  list = CTK_LIST_BOX (ctk_list_box_new ());
  g_object_ref_sink (list);
  ctk_widget_show (CTK_WIDGET (list));

  for (i = 0; i < 100; i++)
    {
      r = g_random_int_range (0, 1000);
      s = g_strdup_printf ("%d: %d", i, r);
      label = ctk_label_new (s);
      g_object_set_data (G_OBJECT (label), "data", GINT_TO_POINTER (r));
      g_free (s);
      ctk_container_add (CTK_CONTAINER (list), label);
    }

  count = 0;
  ctk_list_box_set_sort_func (list, sort_list, &count, NULL);
  g_assert_cmpint (count, >, 0);

  check_sorted (list);

  count = 0;
  ctk_list_box_invalidate_sort (list);
  g_assert_cmpint (count, >, 0);

  count = 0;
  row = ctk_list_box_get_row_at_index (list, 0);
  ctk_list_box_row_changed (row);
  g_assert_cmpint (count, >, 0);

  g_object_unref (list);
}

static CtkListBoxRow *callback_row;

static void
on_row_selected (CtkListBox    *list_box G_GNUC_UNUSED,
                 CtkListBoxRow *row,
                 gpointer       data)
{
  gint *i = data;

  (*i)++;

  callback_row = row;
}

static void
test_selection (void)
{
  CtkListBox *list;
  CtkListBoxRow *row, *row2;
  CtkWidget *label;
  gint i;
  gchar *s;
  gint count;
  gint index;

  list = CTK_LIST_BOX (ctk_list_box_new ());
  g_object_ref_sink (list);
  ctk_widget_show (CTK_WIDGET (list));

  g_assert_cmpint (ctk_list_box_get_selection_mode (list), ==, CTK_SELECTION_SINGLE);
  g_assert (ctk_list_box_get_selected_row (list) == NULL);

  for (i = 0; i < 100; i++)
    {
      s = g_strdup_printf ("%d", i);
      label = ctk_label_new (s);
      g_object_set_data (G_OBJECT (label), "data", GINT_TO_POINTER (i));
      g_free (s);
      ctk_container_add (CTK_CONTAINER (list), label);
    }

  count = 0;
  g_signal_connect (list, "row-selected",
                    G_CALLBACK (on_row_selected),
                    &count);

  row = ctk_list_box_get_row_at_index (list, 20);
  g_assert (!ctk_list_box_row_is_selected (row));
  ctk_list_box_select_row (list, row);
  g_assert (ctk_list_box_row_is_selected (row));
  g_assert (callback_row == row);
  g_assert_cmpint (count, ==, 1);
  row2 = ctk_list_box_get_selected_row (list);
  g_assert (row2 == row);
  ctk_list_box_unselect_all (list);
  row2 = ctk_list_box_get_selected_row (list);
  g_assert (row2 == NULL);
  ctk_list_box_select_row (list, row);
  row2 = ctk_list_box_get_selected_row (list);
  g_assert (row2 == row);

  ctk_list_box_set_selection_mode (list, CTK_SELECTION_BROWSE);
  ctk_container_remove (CTK_CONTAINER (list), CTK_WIDGET (row));
  g_assert (callback_row == NULL);
  g_assert_cmpint (count, ==, 4);
  row2 = ctk_list_box_get_selected_row (list);
  g_assert (row2 == NULL);

  row = ctk_list_box_get_row_at_index (list, 20);
  ctk_list_box_select_row (list, row);
  g_assert (ctk_list_box_row_is_selected (row));
  g_assert (callback_row == row);
  g_assert_cmpint (count, ==, 5);

  ctk_list_box_set_selection_mode (list, CTK_SELECTION_NONE);
  g_assert (!ctk_list_box_row_is_selected (row));
  g_assert (callback_row == NULL);
  g_assert_cmpint (count, ==, 6);
  row2 = ctk_list_box_get_selected_row (list);
  g_assert (row2 == NULL);

  row = ctk_list_box_get_row_at_index (list, 20);
  index = ctk_list_box_row_get_index (row);
  g_assert_cmpint (index, ==, 20);

  row = CTK_LIST_BOX_ROW (ctk_list_box_row_new ());
  g_object_ref_sink (row);
  index = ctk_list_box_row_get_index (row);
  g_assert_cmpint (index, ==, -1);
  g_object_unref (row);

  g_object_unref (list);
}

static void
on_selected_rows_changed (CtkListBox *box G_GNUC_UNUSED,
			  gpointer    data)
{
  gint *i = data;

  (*i)++;
}

static void
test_multi_selection (void)
{
  CtkListBox *list;
  GList *l;
  CtkListBoxRow *row, *row2;
  CtkWidget *label;
  gint i;
  gchar *s;
  gint count;

  list = CTK_LIST_BOX (ctk_list_box_new ());
  g_object_ref_sink (list);
  ctk_widget_show (CTK_WIDGET (list));

  g_assert_cmpint (ctk_list_box_get_selection_mode (list), ==, CTK_SELECTION_SINGLE);
  g_assert (ctk_list_box_get_selected_rows (list) == NULL);

  ctk_list_box_set_selection_mode (list, CTK_SELECTION_MULTIPLE);

  for (i = 0; i < 100; i++)
    {
      s = g_strdup_printf ("%d", i);
      label = ctk_label_new (s);
      g_object_set_data (G_OBJECT (label), "data", GINT_TO_POINTER (i));
      g_free (s);
      ctk_container_add (CTK_CONTAINER (list), label);
    }

  count = 0;
  g_signal_connect (list, "selected-rows-changed",
                    G_CALLBACK (on_selected_rows_changed),
                    &count);

  row = ctk_list_box_get_row_at_index (list, 20);

  ctk_list_box_select_all (list);
  g_assert_cmpint (count, ==, 1);
  l = ctk_list_box_get_selected_rows (list);
  g_assert_cmpint (g_list_length (l), ==, 100);
  g_list_free (l);
  g_assert (ctk_list_box_row_is_selected (row));

  ctk_list_box_unselect_all (list);
  g_assert_cmpint (count, ==, 2);
  l = ctk_list_box_get_selected_rows (list);
  g_assert (l == NULL);
  g_assert (!ctk_list_box_row_is_selected (row));

  ctk_list_box_select_row (list, row);
  g_assert (ctk_list_box_row_is_selected (row));
  g_assert_cmpint (count, ==, 3);
  l = ctk_list_box_get_selected_rows (list);
  g_assert_cmpint (g_list_length (l), ==, 1);
  g_assert (l->data == row);
  g_list_free (l);

  row2 = ctk_list_box_get_row_at_index (list, 40);
  g_assert (!ctk_list_box_row_is_selected (row2));
  ctk_list_box_select_row (list, row2);
  g_assert (ctk_list_box_row_is_selected (row2));
  g_assert_cmpint (count, ==, 4);
  l = ctk_list_box_get_selected_rows (list);
  g_assert_cmpint (g_list_length (l), ==, 2);
  g_assert (l->data == row);
  g_assert (l->next->data == row2);
  g_list_free (l);

  ctk_list_box_unselect_row (list, row);
  g_assert (!ctk_list_box_row_is_selected (row));
  g_assert_cmpint (count, ==, 5);
  l = ctk_list_box_get_selected_rows (list);
  g_assert_cmpint (g_list_length (l), ==, 1);
  g_assert (l->data == row2);
  g_list_free (l);

  g_object_unref (list);
}

static gboolean
filter_func (CtkListBoxRow *row,
             gpointer       data)
{
  gint *count = data;
  CtkWidget *child;
  gint i;

  (*count)++;

  child = ctk_bin_get_child (CTK_BIN (row));
  i = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (child), "data"));

  return (i % 2) == 0;
}

static void
check_filtered (CtkListBox *list)
{
  GList *children, *l;
  gint count;
  CtkWidget *row;

  count = 0;
  children = ctk_container_get_children (CTK_CONTAINER (list));
  for (l = children; l; l = l->next)
    {
      row = l->data;
      if (ctk_widget_get_child_visible (row))
        count++;
    }
  g_list_free (children);
  g_assert_cmpint (count, ==, 50);
}

static void
test_filter (void)
{
  CtkListBox *list;
  CtkListBoxRow *row;
  gint i;
  gchar *s;
  CtkWidget *label;
  gint count;

  list = CTK_LIST_BOX (ctk_list_box_new ());
  g_object_ref_sink (list);
  ctk_widget_show (CTK_WIDGET (list));

  g_assert_cmpint (ctk_list_box_get_selection_mode (list), ==, CTK_SELECTION_SINGLE);
  g_assert (ctk_list_box_get_selected_row (list) == NULL);

  for (i = 0; i < 100; i++)
    {
      s = g_strdup_printf ("%d", i);
      label = ctk_label_new (s);
      g_object_set_data (G_OBJECT (label), "data", GINT_TO_POINTER (i));
      g_free (s);
      ctk_container_add (CTK_CONTAINER (list), label);
    }

  count = 0;
  ctk_list_box_set_filter_func (list, filter_func, &count, NULL);
  g_assert_cmpint (count, >, 0);

  check_filtered (list);

  count = 0;
  ctk_list_box_invalidate_filter (list);
  g_assert_cmpint (count, >, 0);

  count = 0;
  row = ctk_list_box_get_row_at_index (list, 0);
  ctk_list_box_row_changed (row);
  g_assert_cmpint (count, >, 0);

  g_object_unref (list);
}

static void
header_func (CtkListBoxRow *row,
             CtkListBoxRow *before G_GNUC_UNUSED,
             gpointer       data)
{
  CtkWidget *child;
  gint i;
  gint *count = data;
  CtkWidget *header;
  gchar *s;

  (*count)++;

  child = ctk_bin_get_child (CTK_BIN (row));
  i = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (child), "data"));

  if (i % 2 == 0)
    {
      s = g_strdup_printf ("Header %d", i);
      header = ctk_label_new (s);
      g_free (s);
    }
  else
    header = NULL;

  ctk_list_box_row_set_header (row, header);
}

static void
check_headers (CtkListBox *list)
{
  GList *children, *l;
  gint count;
  CtkListBoxRow *row;

  count = 0;
  children = ctk_container_get_children (CTK_CONTAINER (list));
  for (l = children; l; l = l->next)
    {
      row = l->data;
      if (ctk_list_box_row_get_header (row) != NULL)
        count++;
    }
  g_list_free (children);
  g_assert_cmpint (count, ==, 50);
}

static void
test_header (void)
{
  CtkListBox *list;
  CtkListBoxRow *row;
  gint i;
  gchar *s;
  CtkWidget *label;
  gint count;

  list = CTK_LIST_BOX (ctk_list_box_new ());
  g_object_ref_sink (list);
  ctk_widget_show (CTK_WIDGET (list));

  g_assert_cmpint (ctk_list_box_get_selection_mode (list), ==, CTK_SELECTION_SINGLE);
  g_assert (ctk_list_box_get_selected_row (list) == NULL);

  for (i = 0; i < 100; i++)
    {
      s = g_strdup_printf ("%d", i);
      label = ctk_label_new (s);
      g_object_set_data (G_OBJECT (label), "data", GINT_TO_POINTER (i));
      g_free (s);
      ctk_container_add (CTK_CONTAINER (list), label);
    }

  count = 0;
  ctk_list_box_set_header_func (list, header_func, &count, NULL);
  g_assert_cmpint (count, >, 0);

  check_headers (list);

  count = 0;
  ctk_list_box_invalidate_headers (list);
  g_assert_cmpint (count, >, 0);

  count = 0;
  row = ctk_list_box_get_row_at_index (list, 0);
  ctk_list_box_row_changed (row);
  g_assert_cmpint (count, >, 0);

  g_object_unref (list);
}

int
main (int argc, char *argv[])
{
  ctk_test_init (&argc, &argv);

  g_test_add_func ("/listbox/sort", test_sort);
  g_test_add_func ("/listbox/selection", test_selection);
  g_test_add_func ("/listbox/multi-selection", test_multi_selection);
  g_test_add_func ("/listbox/filter", test_filter);
  g_test_add_func ("/listbox/header", test_header);

  return g_test_run ();
}
