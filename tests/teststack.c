#include <gtk/gtk.h>

GtkWidget *stack;
GtkWidget *switcher;
GtkWidget *sidebar;
GtkWidget *w1;

static void
set_visible_child (GtkWidget *button, gpointer data)
{
  ctk_stack_set_visible_child (CTK_STACK (stack), CTK_WIDGET (data));
}

static void
set_visible_child_name (GtkWidget *button, gpointer data)
{
  ctk_stack_set_visible_child_name (CTK_STACK (stack), (const char *)data);
}

static void
toggle_hhomogeneous (GtkWidget *button, gpointer data)
{
  gboolean active = ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (button));
  ctk_stack_set_hhomogeneous (CTK_STACK (stack), active);
}

static void
toggle_vhomogeneous (GtkWidget *button, gpointer data)
{
  gboolean active = ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (button));
  ctk_stack_set_vhomogeneous (CTK_STACK (stack), active);
}

static void
toggle_icon_name (GtkWidget *button, gpointer data)
{
  gboolean active = ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (button));
  ctk_container_child_set (CTK_CONTAINER (stack), w1,
			   "icon-name", active ? "edit-find-symbolic" : NULL,
			   NULL);
}

static void
toggle_transitions (GtkWidget *combo, gpointer data)
{
  int id = ctk_combo_box_get_active (CTK_COMBO_BOX (combo));
  ctk_stack_set_transition_type (CTK_STACK (stack), id);
}

static void
on_back_button_clicked (GtkButton *button, GtkStack *stack)
{
  const gchar *seq[] = { "1", "2", "3" };
  const gchar *vis;
  gint i;

  vis = ctk_stack_get_visible_child_name (stack);

  for (i = 1; i < G_N_ELEMENTS (seq); i++)
    {
      if (g_strcmp0 (vis, seq[i]) == 0)
        {
          ctk_stack_set_visible_child_full (stack, seq[i - 1], CTK_STACK_TRANSITION_TYPE_SLIDE_RIGHT);
          break;
        }
    }
}

static void
on_forward_button_clicked (GtkButton *button, GtkStack *stack)
{
  const gchar *seq[] = { "1", "2", "3" };
  const gchar *vis;
  gint i;

  vis = ctk_stack_get_visible_child_name (stack);

  for (i = 0; i < G_N_ELEMENTS (seq) - 1; i++)
    {
      if (g_strcmp0 (vis, seq[i]) == 0)
        {
          ctk_stack_set_visible_child_full (stack, seq[i + 1], CTK_STACK_TRANSITION_TYPE_SLIDE_LEFT);
          break;
        }
    }
}

static void
update_back_button_sensitivity (GtkStack *stack, GParamSpec *pspec, GtkWidget *button)
{
  const gchar *vis;

  vis = ctk_stack_get_visible_child_name (stack);
  ctk_widget_set_sensitive (button, g_strcmp0 (vis, "1") != 0);
}

static void
update_forward_button_sensitivity (GtkStack *stack, GParamSpec *pspec, GtkWidget *button)
{
  const gchar *vis;

  vis = ctk_stack_get_visible_child_name (stack);
  ctk_widget_set_sensitive (button, g_strcmp0 (vis, "3") != 0);
}

gint
main (gint argc,
      gchar ** argv)
{
  GtkWidget *window, *box, *button, *hbox, *combo, *layout;
  GtkWidget *w2, *w3;
  GtkListStore* store;
  GtkWidget *tree_view;
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  GtkWidget *scrolled_win;
  int i;
  GtkTreeIter iter;
  GEnumClass *class;

  ctk_init (&argc, &argv);

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_widget_set_size_request (window, 300, 300);

  box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  ctk_container_add (CTK_CONTAINER (window), box);

  switcher = ctk_stack_switcher_new ();
  ctk_box_pack_start (CTK_BOX (box), switcher, FALSE, FALSE, 0);

  stack = ctk_stack_new ();

  /* Make transitions longer so we can see that they work */
  ctk_stack_set_transition_duration (CTK_STACK (stack), 1500);

  ctk_widget_set_halign (stack, CTK_ALIGN_START);
  ctk_widget_set_vexpand (stack, TRUE);

  /* Add sidebar before stack */
  sidebar = ctk_stack_sidebar_new ();
  ctk_stack_sidebar_set_stack (CTK_STACK_SIDEBAR (sidebar), CTK_STACK (stack));
  layout = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
  ctk_box_pack_start (CTK_BOX (layout), sidebar, FALSE, FALSE, 0);
  ctk_box_pack_start (CTK_BOX (layout), stack, TRUE, TRUE, 0);

  ctk_container_add (CTK_CONTAINER (box), layout);

  ctk_stack_switcher_set_stack (CTK_STACK_SWITCHER (switcher), CTK_STACK (stack));

  w1 = ctk_text_view_new ();
  ctk_text_buffer_set_text (ctk_text_view_get_buffer (CTK_TEXT_VIEW (w1)),
			    "This is a\nTest\nBalh!", -1);

  ctk_container_add_with_properties (CTK_CONTAINER (stack), w1,
				     "name", "1",
				     "title", "1",
				     NULL);

  w2 = ctk_button_new_with_label ("Gazoooooooooooooooonk");
  ctk_container_add (CTK_CONTAINER (stack), w2);
  ctk_container_child_set (CTK_CONTAINER (stack), w2,
			   "name", "2",
			   "title", "2",
                           "needs-attention", TRUE,
			   NULL);


  scrolled_win = ctk_scrolled_window_new (NULL, NULL);
  ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (scrolled_win),
				  CTK_POLICY_AUTOMATIC,
				  CTK_POLICY_AUTOMATIC);
  ctk_widget_set_size_request (scrolled_win, 100, 200);


  store = ctk_list_store_new (1, G_TYPE_STRING);

  for (i = 0; i < 40; i++)
    ctk_list_store_insert_with_values (store, &iter, i, 0,  "Testvalule", -1);

  tree_view = ctk_tree_view_new_with_model (CTK_TREE_MODEL (store));

  ctk_container_add (CTK_CONTAINER (scrolled_win), tree_view);
  w3 = scrolled_win;

  renderer = ctk_cell_renderer_text_new ();
  column = ctk_tree_view_column_new_with_attributes ("Target", renderer,
						     "text", 0, NULL);
  ctk_tree_view_append_column (CTK_TREE_VIEW (tree_view), column);

  ctk_stack_add_titled (CTK_STACK (stack), w3, "3", "3");

  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
  ctk_container_add (CTK_CONTAINER (box), hbox);

  button = ctk_button_new_with_label ("1");
  ctk_container_add (CTK_CONTAINER (hbox), button);
  g_signal_connect (button, "clicked", (GCallback) set_visible_child, w1);

  button = ctk_button_new_with_label ("2");
  ctk_container_add (CTK_CONTAINER (hbox), button);
  g_signal_connect (button, "clicked", (GCallback) set_visible_child, w2);

  button = ctk_button_new_with_label ("3");
  ctk_container_add (CTK_CONTAINER (hbox), button);
  g_signal_connect (button, "clicked", (GCallback) set_visible_child, w3);

  button = ctk_button_new_with_label ("1");
  ctk_container_add (CTK_CONTAINER (hbox), button);
  g_signal_connect (button, "clicked", (GCallback) set_visible_child_name, (gpointer) "1");

  button = ctk_button_new_with_label ("2");
  ctk_container_add (CTK_CONTAINER (hbox), button);
  g_signal_connect (button, "clicked", (GCallback) set_visible_child_name, (gpointer) "2");

  button = ctk_button_new_with_label ("3");
  ctk_container_add (CTK_CONTAINER (hbox), button);
  g_signal_connect (button, "clicked", (GCallback) set_visible_child_name, (gpointer) "3");

  button = ctk_check_button_new ();
  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (button),
				ctk_stack_get_hhomogeneous (CTK_STACK (stack)));
  ctk_container_add (CTK_CONTAINER (hbox), button);
  g_signal_connect (button, "clicked", (GCallback) toggle_hhomogeneous, NULL);

  button = ctk_check_button_new_with_label ("homogeneous");
  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (button),
				ctk_stack_get_vhomogeneous (CTK_STACK (stack)));
  ctk_container_add (CTK_CONTAINER (hbox), button);
  g_signal_connect (button, "clicked", (GCallback) toggle_vhomogeneous, NULL);

  button = ctk_toggle_button_new_with_label ("Add icon");
  g_signal_connect (button, "toggled", (GCallback) toggle_icon_name, NULL);
  ctk_container_add (CTK_CONTAINER (hbox), button);

  combo = ctk_combo_box_text_new ();
  class = g_type_class_ref (CTK_TYPE_STACK_TRANSITION_TYPE);
  for (i = 0; i < class->n_values; i++)
    ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo), class->values[i].value_nick);
  g_type_class_unref (class);

  ctk_container_add (CTK_CONTAINER (hbox), combo);
  g_signal_connect (combo, "changed", (GCallback) toggle_transitions, NULL);
  ctk_combo_box_set_active (CTK_COMBO_BOX (combo), 0);

  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
  ctk_container_add (CTK_CONTAINER (box), hbox);

  button = ctk_button_new_with_label ("<");
  g_signal_connect (button, "clicked", (GCallback) on_back_button_clicked, stack);
  g_signal_connect (stack, "notify::visible-child-name",
                    (GCallback)update_back_button_sensitivity, button);
  ctk_container_add (CTK_CONTAINER (hbox), button);

  button = ctk_button_new_with_label (">");
  ctk_container_add (CTK_CONTAINER (hbox), button);
  g_signal_connect (button, "clicked", (GCallback) on_forward_button_clicked, stack);
  g_signal_connect (stack, "notify::visible-child-name",
                    (GCallback)update_forward_button_sensitivity, button);


  ctk_widget_show_all (window);
  ctk_main ();

  ctk_widget_destroy (window);

  return 0;
}
