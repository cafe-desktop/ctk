#include <ctk/ctk.h>

enum
{
  PROP_LABEL = 1,
  PROP_ID,
  LAST_PROPERTY
};

static GParamSpec *properties[LAST_PROPERTY] = { NULL, };

typedef struct
{
  GObject parent;

  gchar *label;
  gint id;
} MyObject;

typedef struct
{
  GObjectClass parent_class;
} MyObjectClass;

G_DEFINE_TYPE (MyObject, my_object, G_TYPE_OBJECT)

static void
my_object_init (MyObject *obj G_GNUC_UNUSED)
{
}

static void
my_object_get_property (GObject    *object,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  MyObject *obj = (MyObject *)object;

  switch (property_id)
    {
    case PROP_LABEL:
      g_value_set_string (value, obj->label);
      break;
    case PROP_ID:
      g_value_set_int (value, obj->id);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
my_object_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  MyObject *obj = (MyObject *)object;

  switch (property_id)
    {
    case PROP_LABEL:
      g_free (obj->label);
      obj->label = g_value_dup_string (value);
      break;
    case PROP_ID:
      obj->id = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
my_object_finalize (GObject *obj)
{
  MyObject *object = (MyObject *)obj;

  g_free (object->label);

  G_OBJECT_CLASS (my_object_parent_class)->finalize (obj);
}

static void
my_object_class_init (MyObjectClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->get_property = my_object_get_property;
  object_class->set_property = my_object_set_property;
  object_class->finalize = my_object_finalize;

  properties[PROP_LABEL] = g_param_spec_string ("label", "label", "label",
                                                NULL, G_PARAM_READWRITE);
  properties[PROP_ID] = g_param_spec_int ("id", "id", "id",
                                          0, G_MAXINT, 0, G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, LAST_PROPERTY, properties);
}

static CtkWidget *
create_widget (gpointer item,
               gpointer user_data G_GNUC_UNUSED)
{
  MyObject *obj = (MyObject *)item;
  CtkWidget *label;

  label = ctk_label_new ("");
  g_object_bind_property (obj, "label", label, "label", G_BINDING_SYNC_CREATE);

  return label;
}

static gint
compare_items (gconstpointer a,
	       gconstpointer b,
	       gpointer      data G_GNUC_UNUSED)
{
  gint id_a, id_b;

  g_object_get ((gpointer)a, "id", &id_a, NULL);
  g_object_get ((gpointer)b, "id", &id_b, NULL);

  return id_a - id_b;
}

static void
add_some (CtkButton  *button G_GNUC_UNUSED,
	  GListStore *store)
{
  gint n, i;
  guint n_items;

  for (n = 0; n < 50; n++)
    {
      GObject *obj;
      gchar *label;

      n_items = g_list_model_get_n_items (G_LIST_MODEL (store));
      i = g_random_int_range (0, MAX (2 * n_items, 1));
      label = g_strdup_printf ("Added %d", i);
      obj = g_object_new (my_object_get_type (),
                          "id", i,
                          "label", label,
                          NULL);
      g_list_store_insert_sorted (store, obj, compare_items, NULL);
      g_object_unref (obj);
      g_free (label);
    }
}

static void
remove_some (CtkButton  *button G_GNUC_UNUSED,
	     GListStore *store)
{
  gint n, i;
  guint n_items;

  for (n = 0; n < 50; n++)
    {
      n_items = g_list_model_get_n_items (G_LIST_MODEL (store));
      if (n_items == 0)
        return;
      i = g_random_int_range (0, n_items);
      g_list_store_remove (store, i);
    }
}

int
main (int   argc G_GNUC_UNUSED,
      char *argv[] G_GNUC_UNUSED)
{
  CtkWidget *window, *grid, *sw, *box, *button;
  GListStore *store;
  gint i;

  ctk_init (NULL, NULL);

  store = g_list_store_new (my_object_get_type ());
  for (i = 0; i < 100; i++)
    {
      MyObject *obj;
      gchar *label;

      label = g_strdup_printf ("item %d", i);
      obj = g_object_new (my_object_get_type (),
                          "id", i,
                          "label", label,
                          NULL);
      g_list_store_append (store, obj);
      g_free (label);
      g_object_unref (obj);
    }

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  grid = ctk_grid_new ();
  ctk_container_add (CTK_CONTAINER (window), grid);
  sw = ctk_scrolled_window_new (NULL, NULL);
  ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (sw),
                                  CTK_POLICY_AUTOMATIC,
                                  CTK_POLICY_AUTOMATIC);
  ctk_widget_set_hexpand (sw, TRUE);
  ctk_widget_set_vexpand (sw, TRUE);
  ctk_grid_attach (CTK_GRID (grid), sw, 0, 0, 1, 1);

  box = ctk_list_box_new ();
  ctk_list_box_bind_model (CTK_LIST_BOX (box), G_LIST_MODEL (store), create_widget, NULL, NULL);
  ctk_container_add (CTK_CONTAINER (sw), box);

  sw = ctk_scrolled_window_new (NULL, NULL);
  ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (sw),
                                  CTK_POLICY_AUTOMATIC,
                                  CTK_POLICY_AUTOMATIC);
  ctk_widget_set_hexpand (sw, TRUE);
  ctk_widget_set_vexpand (sw, TRUE);
  ctk_grid_attach (CTK_GRID (grid), sw, 1, 0, 1, 1);

  box = ctk_flow_box_new ();
  ctk_flow_box_bind_model (CTK_FLOW_BOX (box), G_LIST_MODEL (store), create_widget, NULL, NULL);
  ctk_container_add (CTK_CONTAINER (sw), box);

  button = ctk_button_new_with_label ("Add some");
  g_signal_connect (button, "clicked", G_CALLBACK (add_some), store);
  ctk_grid_attach (CTK_GRID (grid), button, 0, 1, 1, 1);

  button = ctk_button_new_with_label ("Remove some");
  g_signal_connect (button, "clicked", G_CALLBACK (remove_some), store);
  ctk_grid_attach (CTK_GRID (grid), button, 0, 2, 1, 1);

  ctk_widget_show_all (window);

  ctk_main ();

  return 0;
}
