#include <ctk/ctk.h>

#include <string.h>

typedef struct _Row Row;
typedef struct _RowClass RowClass;

struct _Row
{
  GtkListBoxRow parent_instance;
  GtkWidget *label;
  gint sort_id;
};

struct _RowClass
{
  GtkListBoxRowClass parent_class;
};

const char *css =
  "GtkListBoxRow {"
  " border-width: 1px;"
  " border-style: solid;"
  " border-color: blue;"
  "}"
  "GtkListBoxRow:prelight {"
  "background-color: green;"
  "}"
  "GtkListBoxRow:active {"
  "background-color: red;"
  "}";

G_DEFINE_TYPE (Row, row, CTK_TYPE_LIST_BOX_ROW)

static void
row_init (Row *row)
{
}

static void
row_class_init (RowClass *class)
{
}

GtkWidget *
row_new (const gchar* text, gint sort_id) {
  Row *row;

  row = g_object_new (row_get_type (), NULL);
  if (text != NULL)
    {
      row->label = ctk_label_new (text);
      ctk_container_add (CTK_CONTAINER (row), row->label);
      ctk_widget_show (row->label);
    }
  row->sort_id = sort_id;

  return CTK_WIDGET (row);
}


static void
update_header_cb (Row *row, Row *before, gpointer data)
{
  GtkWidget *hbox, *l, *b;
  GList *children;

  if (before == NULL ||
      (row->label != NULL &&
       strcmp (ctk_label_get_text (CTK_LABEL (row->label)), "blah3") == 0))
    {
      /* Create header if needed */
      if (ctk_list_box_row_get_header (CTK_LIST_BOX_ROW (row)) == NULL)
        {
          hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
          l = ctk_label_new ("Header");
          ctk_container_add (CTK_CONTAINER (hbox), l);
          b = ctk_button_new_with_label ("button");
          ctk_container_add (CTK_CONTAINER (hbox), b);
          ctk_widget_show (l);
          ctk_widget_show (b);
          ctk_list_box_row_set_header (CTK_LIST_BOX_ROW (row), hbox);
      }

      hbox = ctk_list_box_row_get_header(CTK_LIST_BOX_ROW (row));

      children = ctk_container_get_children (CTK_CONTAINER (hbox));
      l = children->data;
      g_list_free (children);
      ctk_label_set_text (CTK_LABEL (l), g_strdup_printf ("Header %d", row->sort_id));
    }
  else
    {
      ctk_list_box_row_set_header(CTK_LIST_BOX_ROW (row), NULL);
    }
}

static int
sort_cb (Row *a, Row *b, gpointer data)
{
  return a->sort_id - b->sort_id;
}

static int
reverse_sort_cb (Row *a, Row *b, gpointer data)
{
  return b->sort_id - a->sort_id;
}

static gboolean
filter_cb (Row *row, gpointer data)
{
  const char *text;

  if (row->label != NULL)
    {
      text = ctk_label_get_text (CTK_LABEL (row->label));
      return strcmp (text, "blah3") != 0;
    }

  return TRUE;
}

static void
row_activated_cb (GtkListBox *list_box,
                  GtkListBoxRow *row)
{
  g_print ("activated row %p\n", row);
}

static void
row_selected_cb (GtkListBox *list_box,
                 GtkListBoxRow *row)
{
  g_print ("selected row %p\n", row);
}

static void
sort_clicked_cb (GtkButton *button,
                 gpointer data)
{
  GtkListBox *list = data;

  ctk_list_box_set_sort_func (list, (GtkListBoxSortFunc)sort_cb, NULL, NULL);
}

static void
reverse_sort_clicked_cb (GtkButton *button,
                         gpointer data)
{
  GtkListBox *list = data;

  ctk_list_box_set_sort_func (list, (GtkListBoxSortFunc)reverse_sort_cb, NULL, NULL);
}

static void
filter_clicked_cb (GtkButton *button,
                   gpointer data)
{
  GtkListBox *list = data;

  ctk_list_box_set_filter_func (list, (GtkListBoxFilterFunc)filter_cb, NULL, NULL);
}

static void
unfilter_clicked_cb (GtkButton *button,
                   gpointer data)
{
  GtkListBox *list = data;

  ctk_list_box_set_filter_func (list, NULL, NULL, NULL);
}

static void
change_clicked_cb (GtkButton *button,
                   gpointer data)
{
  Row *row = data;

  if (strcmp (ctk_label_get_text (CTK_LABEL (row->label)), "blah3") == 0)
    {
      ctk_label_set_text (CTK_LABEL (row->label), "blah5");
      row->sort_id = 5;
    }
  else
    {
      ctk_label_set_text (CTK_LABEL (row->label), "blah3");
      row->sort_id = 3;
    }
  ctk_list_box_row_changed (CTK_LIST_BOX_ROW (row));
}

static void
add_clicked_cb (GtkButton *button,
                gpointer data)
{
  GtkListBox *list = data;
  GtkWidget *new_row;
  static int new_button_nr = 1;

  new_row = row_new( g_strdup_printf ("blah2 new %d", new_button_nr), new_button_nr);
  ctk_widget_show_all (new_row);
  ctk_container_add (CTK_CONTAINER (list), new_row);
  new_button_nr++;
}

static void
separate_clicked_cb (GtkButton *button,
                     gpointer data)
{
  GtkListBox *list = data;

  ctk_list_box_set_header_func (list, (GtkListBoxUpdateHeaderFunc)update_header_cb, NULL, NULL);
}

static void
unseparate_clicked_cb (GtkButton *button,
                       gpointer data)
{
  GtkListBox *list = data;

  ctk_list_box_set_header_func (list, NULL, NULL, NULL);
}

static void
visibility_clicked_cb (GtkButton *button,
                       gpointer data)
{
  GtkWidget *row = data;

  ctk_widget_set_visible (row, !ctk_widget_get_visible (row));
}

static void
selection_mode_changed (GtkComboBox *combo, gpointer data)
{
  GtkListBox *list = data;

  ctk_list_box_set_selection_mode (list, ctk_combo_box_get_active (combo));
}

static void
single_click_clicked (GtkButton *check, gpointer data)
{
  GtkListBox *list = data;

  g_print ("single: %d\n", ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (check)));
  ctk_list_box_set_activate_on_single_click (list, ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (check)));
}

int
main (int argc, char *argv[])
{
  GtkCssProvider *provider;
  GtkWidget *window, *hbox, *vbox, *list, *row, *row3, *row_vbox, *row_hbox, *l;
  GtkWidget *check, *button, *combo, *scrolled;

  ctk_init (NULL, NULL);

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
  ctk_container_add (CTK_CONTAINER (window), hbox);

  provider = ctk_css_provider_new ();
  ctk_css_provider_load_from_data (provider, css, -1, NULL);
  ctk_style_context_add_provider_for_screen (ctk_widget_get_screen (window),
                                             CTK_STYLE_PROVIDER (provider),
                                             CTK_STYLE_PROVIDER_PRIORITY_USER);


  list = ctk_list_box_new ();

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  ctk_container_add (CTK_CONTAINER (hbox), vbox);

  combo = ctk_combo_box_text_new ();
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo),
                                  "CTK_SELECTION_NONE");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo),
                                  "CTK_SELECTION_SINGLE");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo),
                                  "CTK_SELECTION_BROWSE");
  g_signal_connect (combo, "changed", G_CALLBACK (selection_mode_changed), list);
  ctk_container_add (CTK_CONTAINER (vbox), combo);
  ctk_combo_box_set_active (CTK_COMBO_BOX (combo), ctk_list_box_get_selection_mode (CTK_LIST_BOX (list)));
  check = ctk_check_button_new_with_label ("single click mode");
  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (check), ctk_list_box_get_activate_on_single_click (CTK_LIST_BOX (list)));
  g_signal_connect (check, "toggled", G_CALLBACK (single_click_clicked), list);
  ctk_container_add (CTK_CONTAINER (vbox), check);

  scrolled = ctk_scrolled_window_new (NULL, NULL);
  ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (scrolled), CTK_POLICY_NEVER, CTK_POLICY_AUTOMATIC);
  ctk_container_add (CTK_CONTAINER (scrolled), list);
  ctk_container_add (CTK_CONTAINER (hbox), scrolled);

  g_signal_connect (list, "row-activated", G_CALLBACK (row_activated_cb), NULL);
  g_signal_connect (list, "row-selected", G_CALLBACK (row_selected_cb), NULL);

  row = row_new ("blah4", 4);
  ctk_container_add (CTK_CONTAINER (list), row);
  row3 = row = row_new ("blah3", 3);
  ctk_container_add (CTK_CONTAINER (list), row);
  row = row_new ("blah1", 1);
  ctk_container_add (CTK_CONTAINER (list), row);
  row = row_new ("blah2", 2);
  ctk_container_add (CTK_CONTAINER (list), row);

  row = row_new (NULL, 0);
  row_vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  row_hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
  l = ctk_label_new ("da box for da man");
  ctk_container_add (CTK_CONTAINER (row_hbox), l);
  check = ctk_check_button_new ();
  ctk_container_add (CTK_CONTAINER (row_hbox), check);
  button = ctk_button_new_with_label ("ya!");
  ctk_container_add (CTK_CONTAINER (row_hbox), button);
  ctk_container_add (CTK_CONTAINER (row_vbox), row_hbox);
  check = ctk_check_button_new ();
  ctk_container_add (CTK_CONTAINER (row_vbox), check);
  ctk_container_add (CTK_CONTAINER (row), row_vbox);
  ctk_container_add (CTK_CONTAINER (list), row);

  row = row_new (NULL, 0);
  button = ctk_button_new_with_label ("focusable row");
  ctk_widget_set_hexpand (button, FALSE);
  ctk_widget_set_halign (button, CTK_ALIGN_START);
  ctk_container_add (CTK_CONTAINER (row), button);
  ctk_container_add (CTK_CONTAINER (list), row);

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  ctk_container_add (CTK_CONTAINER (hbox), vbox);

  button = ctk_button_new_with_label ("sort");
  ctk_container_add (CTK_CONTAINER (vbox), button);
  g_signal_connect (button, "clicked", G_CALLBACK (sort_clicked_cb), list);

  button = ctk_button_new_with_label ("reverse");
  ctk_container_add (CTK_CONTAINER (vbox), button);
  g_signal_connect (button, "clicked", G_CALLBACK (reverse_sort_clicked_cb), list);

  button = ctk_button_new_with_label ("change");
  ctk_container_add (CTK_CONTAINER (vbox), button);
  g_signal_connect (button, "clicked", G_CALLBACK (change_clicked_cb), row3);

  button = ctk_button_new_with_label ("filter");
  ctk_container_add (CTK_CONTAINER (vbox), button);
  g_signal_connect (button, "clicked", G_CALLBACK (filter_clicked_cb), list);

  button = ctk_button_new_with_label ("unfilter");
  ctk_container_add (CTK_CONTAINER (vbox), button);
  g_signal_connect (button, "clicked", G_CALLBACK (unfilter_clicked_cb), list);

  button = ctk_button_new_with_label ("add");
  ctk_container_add (CTK_CONTAINER (vbox), button);
  g_signal_connect (button, "clicked", G_CALLBACK (add_clicked_cb), list);

  button = ctk_button_new_with_label ("separate");
  ctk_container_add (CTK_CONTAINER (vbox), button);
  g_signal_connect (button, "clicked", G_CALLBACK (separate_clicked_cb), list);

  button = ctk_button_new_with_label ("unseparate");
  ctk_container_add (CTK_CONTAINER (vbox), button);
  g_signal_connect (button, "clicked", G_CALLBACK (unseparate_clicked_cb), list);

  button = ctk_button_new_with_label ("visibility");
  ctk_container_add (CTK_CONTAINER (vbox), button);
  g_signal_connect (button, "clicked", G_CALLBACK (visibility_clicked_cb), row3);

  ctk_widget_show_all (window);

  ctk_main ();

  return 0;
}
