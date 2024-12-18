/* Combo Boxes
 *
 * The CtkComboBox widget allows to select one option out of a list.
 * The CtkComboBoxEntry additionally allows the user to enter a value
 * that is not in the list of options.
 *
 * How the options are displayed is controlled by cell renderers.
 */

#include <glib/gi18n.h>
#include <ctk/ctk.h>

enum
{
  ICON_NAME_COL,
  TEXT_COL
};

static CtkTreeModel *
create_icon_store (void)
{
  const gchar *icon_names[6] = {
    "dialog-warning",
    "process-stop",
    "document-new",
    "edit-clear",
    NULL,
    "document-open"
  };
  const gchar *labels[6] = {
    N_("Warning"),
    N_("Stop"),
    N_("New"),
    N_("Clear"),
    NULL,
    N_("Open")
  };

  CtkTreeIter iter;
  CtkListStore *store;
  gint i;

  store = ctk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);

  for (i = 0; i < G_N_ELEMENTS (icon_names); i++)
    {
      if (icon_names[i])
        {
          ctk_list_store_append (store, &iter);
          ctk_list_store_set (store, &iter,
                              ICON_NAME_COL, icon_names[i],
                              TEXT_COL, _(labels[i]),
                              -1);
        }
      else
        {
          ctk_list_store_append (store, &iter);
          ctk_list_store_set (store, &iter,
                              ICON_NAME_COL, NULL,
                              TEXT_COL, "separator",
                              -1);
        }
    }

  return CTK_TREE_MODEL (store);
}

/* A CtkCellLayoutDataFunc that demonstrates how one can control
 * sensitivity of rows. This particular function does nothing
 * useful and just makes the second row insensitive.
 */
static void
set_sensitive (CtkCellLayout   *cell_layout G_GNUC_UNUSED,
               CtkCellRenderer *cell,
               CtkTreeModel    *tree_model,
               CtkTreeIter     *iter,
               gpointer         data G_GNUC_UNUSED)
{
  CtkTreePath *path;
  gint *indices;
  gboolean sensitive;

  path = ctk_tree_model_get_path (tree_model, iter);
  indices = ctk_tree_path_get_indices (path);
  sensitive = indices[0] != 1;
  ctk_tree_path_free (path);

  g_object_set (cell, "sensitive", sensitive, NULL);
}

/* A CtkTreeViewRowSeparatorFunc that demonstrates how rows can be
 * rendered as separators. This particular function does nothing
 * useful and just turns the fourth row into a separator.
 */
static gboolean
is_separator (CtkTreeModel *model,
              CtkTreeIter  *iter,
              gpointer      data G_GNUC_UNUSED)
{
  CtkTreePath *path;
  gboolean result;

  path = ctk_tree_model_get_path (model, iter);
  result = ctk_tree_path_get_indices (path)[0] == 4;
  ctk_tree_path_free (path);

  return result;
}

static CtkTreeModel *
create_capital_store (void)
{
  struct {
    gchar *group;
    gchar *capital;
  } capitals[] = {
    { "A - B", NULL },
    { NULL, "Albany" },
    { NULL, "Annapolis" },
    { NULL, "Atlanta" },
    { NULL, "Augusta" },
    { NULL, "Austin" },
    { NULL, "Baton Rouge" },
    { NULL, "Bismarck" },
    { NULL, "Boise" },
    { NULL, "Boston" },
    { "C - D", NULL },
    { NULL, "Carson City" },
    { NULL, "Charleston" },
    { NULL, "Cheyenne" },
    { NULL, "Columbia" },
    { NULL, "Columbus" },
    { NULL, "Concord" },
    { NULL, "Denver" },
    { NULL, "Des Moines" },
    { NULL, "Dover" },
    { "E - J", NULL },
    { NULL, "Frankfort" },
    { NULL, "Harrisburg" },
    { NULL, "Hartford" },
    { NULL, "Helena" },
    { NULL, "Honolulu" },
    { NULL, "Indianapolis" },
    { NULL, "Jackson" },
    { NULL, "Jefferson City" },
    { NULL, "Juneau" },
    { "K - O", NULL },
    { NULL, "Lansing" },
    { NULL, "Lincoln" },
    { NULL, "Little Rock" },
    { NULL, "Madison" },
    { NULL, "Montgomery" },
    { NULL, "Montpelier" },
    { NULL, "Nashville" },
    { NULL, "Oklahoma City" },
    { NULL, "Olympia" },
    { "P - S", NULL },
    { NULL, "Phoenix" },
    { NULL, "Pierre" },
    { NULL, "Providence" },
    { NULL, "Raleigh" },
    { NULL, "Richmond" },
    { NULL, "Sacramento" },
    { NULL, "Salem" },
    { NULL, "Salt Lake City" },
    { NULL, "Santa Fe" },
    { NULL, "Springfield" },
    { NULL, "St. Paul" },
    { "T - Z", NULL },
    { NULL, "Tallahassee" },
    { NULL, "Topeka" },
    { NULL, "Trenton" },
    { NULL, NULL }
  };

  CtkTreeIter iter, iter2;
  CtkTreeStore *store;
  gint i;

  store = ctk_tree_store_new (1, G_TYPE_STRING);

  for (i = 0; capitals[i].group || capitals[i].capital; i++)
    {
      if (capitals[i].group)
        {
          ctk_tree_store_append (store, &iter, NULL);
          ctk_tree_store_set (store, &iter, 0, capitals[i].group, -1);
        }
      else if (capitals[i].capital)
        {
          ctk_tree_store_append (store, &iter2, &iter);
          ctk_tree_store_set (store, &iter2, 0, capitals[i].capital, -1);
        }
    }

  return CTK_TREE_MODEL (store);
}

static void
is_capital_sensitive (CtkCellLayout   *cell_layout G_GNUC_UNUSED,
                      CtkCellRenderer *cell,
                      CtkTreeModel    *tree_model,
                      CtkTreeIter     *iter,
                      gpointer         data G_GNUC_UNUSED)
{
  gboolean sensitive;

  sensitive = !ctk_tree_model_iter_has_child (tree_model, iter);

  g_object_set (cell, "sensitive", sensitive, NULL);
}

static void
fill_combo_entry (CtkWidget *combo)
{
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo), "One");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo), "Two");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo), "2\302\275");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo), "Three");
}


/* A simple validating entry */

#define TYPE_MASK_ENTRY             (mask_entry_get_type ())
#define MASK_ENTRY(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_MASK_ENTRY, MaskEntry))
#define MASK_ENTRY_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TYPE_MASK_ENTRY, MaskEntryClass))
#define IS_MASK_ENTRY(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_MASK_ENTRY))
#define IS_MASK_ENTRY_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TYPE_MASK_ENTRY))
#define MASK_ENTRY_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TYPE_MASK_ENTRY, MaskEntryClass))


typedef struct _MaskEntry MaskEntry;
struct _MaskEntry
{
  CtkEntry entry;
  gchar *mask;
};

typedef struct _MaskEntryClass MaskEntryClass;
struct _MaskEntryClass
{
  CtkEntryClass parent_class;
};


static void mask_entry_editable_init (CtkEditableInterface *iface);

G_DEFINE_TYPE_WITH_CODE (MaskEntry, mask_entry, CTK_TYPE_ENTRY,
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_EDITABLE,
                                                mask_entry_editable_init));


static void
mask_entry_set_background (MaskEntry *entry)
{
  if (entry->mask)
    {
      if (!g_regex_match_simple (entry->mask, ctk_entry_get_text (CTK_ENTRY (entry)), 0, 0))
        {
          PangoAttrList *attrs;

          attrs = pango_attr_list_new ();
          pango_attr_list_insert (attrs, pango_attr_foreground_new (65535, 32767, 32767));
          ctk_entry_set_attributes (CTK_ENTRY (entry), attrs);
          pango_attr_list_unref (attrs);
          return;
        }
    }

  ctk_entry_set_attributes (CTK_ENTRY (entry), NULL);
}


static void
mask_entry_changed (CtkEditable *editable)
{
  mask_entry_set_background (MASK_ENTRY (editable));
}


static void
mask_entry_init (MaskEntry *entry)
{
  entry->mask = NULL;
}


static void
mask_entry_class_init (MaskEntryClass *klass G_GNUC_UNUSED)
{ }


static void
mask_entry_editable_init (CtkEditableInterface *iface)
{
  iface->changed = mask_entry_changed;
}


CtkWidget *
do_combobox (CtkWidget *do_widget)
{
  static CtkWidget *window = NULL;

  if (!window)
  {
    CtkWidget *vbox, *frame, *box, *combo, *entry;
    CtkTreeModel *model;
    CtkCellRenderer *renderer;
    CtkTreePath *path;
    CtkTreeIter iter;

    window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
    ctk_window_set_screen (CTK_WINDOW (window),
                           ctk_widget_get_screen (do_widget));
    ctk_window_set_title (CTK_WINDOW (window), "Combo Boxes");

    g_signal_connect (window, "destroy",
                      G_CALLBACK (ctk_widget_destroyed), &window);

    ctk_container_set_border_width (CTK_CONTAINER (window), 10);

    vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 2);
    ctk_container_add (CTK_CONTAINER (window), vbox);

    /* A combobox demonstrating cell renderers, separators and
     *  insensitive rows
     */
    frame = ctk_frame_new ("Items with icons");
    ctk_box_pack_start (CTK_BOX (vbox), frame, FALSE, FALSE, 0);

    box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
    ctk_container_set_border_width (CTK_CONTAINER (box), 5);
    ctk_container_add (CTK_CONTAINER (frame), box);

    model = create_icon_store ();
    combo = ctk_combo_box_new_with_model (model);
    g_object_unref (model);
    ctk_container_add (CTK_CONTAINER (box), combo);

    renderer = ctk_cell_renderer_pixbuf_new ();
    ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (combo), renderer, FALSE);
    ctk_cell_layout_set_attributes (CTK_CELL_LAYOUT (combo), renderer,
                                    "icon-name", ICON_NAME_COL,
                                    NULL);

    ctk_cell_layout_set_cell_data_func (CTK_CELL_LAYOUT (combo),
                                        renderer,
                                        set_sensitive,
                                        NULL, NULL);

    renderer = ctk_cell_renderer_text_new ();
    ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (combo), renderer, TRUE);
    ctk_cell_layout_set_attributes (CTK_CELL_LAYOUT (combo), renderer,
                                    "text", TEXT_COL,
                                    NULL);

    ctk_cell_layout_set_cell_data_func (CTK_CELL_LAYOUT (combo),
                                        renderer,
                                        set_sensitive,
                                        NULL, NULL);

    ctk_combo_box_set_row_separator_func (CTK_COMBO_BOX (combo),
                                          is_separator, NULL, NULL);

    ctk_combo_box_set_active (CTK_COMBO_BOX (combo), 0);

    /* A combobox demonstrating trees.
     */
    frame = ctk_frame_new ("Where are we ?");
    ctk_box_pack_start (CTK_BOX (vbox), frame, FALSE, FALSE, 0);

    box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
    ctk_container_set_border_width (CTK_CONTAINER (box), 5);
    ctk_container_add (CTK_CONTAINER (frame), box);

    model = create_capital_store ();
    combo = ctk_combo_box_new_with_model (model);
    g_object_unref (model);
    ctk_container_add (CTK_CONTAINER (box), combo);

    renderer = ctk_cell_renderer_text_new ();
    ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (combo), renderer, TRUE);
    ctk_cell_layout_set_attributes (CTK_CELL_LAYOUT (combo), renderer,
                                    "text", 0,
                                    NULL);
    ctk_cell_layout_set_cell_data_func (CTK_CELL_LAYOUT (combo),
                                        renderer,
                                        is_capital_sensitive,
                                        NULL, NULL);

    path = ctk_tree_path_new_from_indices (0, 8, -1);
    ctk_tree_model_get_iter (model, &iter, path);
    ctk_tree_path_free (path);
    ctk_combo_box_set_active_iter (CTK_COMBO_BOX (combo), &iter);

    /* A CtkComboBoxEntry with validation */
    frame = ctk_frame_new ("Editable");
    ctk_box_pack_start (CTK_BOX (vbox), frame, FALSE, FALSE, 0);

    box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
    ctk_container_set_border_width (CTK_CONTAINER (box), 5);
    ctk_container_add (CTK_CONTAINER (frame), box);

    combo = ctk_combo_box_text_new_with_entry ();
    fill_combo_entry (combo);
    ctk_container_add (CTK_CONTAINER (box), combo);

    entry = g_object_new (TYPE_MASK_ENTRY, NULL);
    MASK_ENTRY (entry)->mask = "^([0-9]*|One|Two|2\302\275|Three)$";

    ctk_container_remove (CTK_CONTAINER (combo), ctk_bin_get_child (CTK_BIN (combo)));
    ctk_container_add (CTK_CONTAINER (combo), entry);

    /* A combobox with string IDs */
    frame = ctk_frame_new ("String IDs");
    ctk_box_pack_start (CTK_BOX (vbox), frame, FALSE, FALSE, 0);

    box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
    ctk_container_set_border_width (CTK_CONTAINER (box), 5);
    ctk_container_add (CTK_CONTAINER (frame), box);

    combo = ctk_combo_box_text_new ();
    ctk_combo_box_text_append (CTK_COMBO_BOX_TEXT (combo), "never", "Not visible");
    ctk_combo_box_text_append (CTK_COMBO_BOX_TEXT (combo), "when-active", "Visible when active");
    ctk_combo_box_text_append (CTK_COMBO_BOX_TEXT (combo), "always", "Always visible");
    ctk_container_add (CTK_CONTAINER (box), combo);

    entry = ctk_entry_new ();
    g_object_bind_property (combo, "active-id",
                            entry, "text",
                            G_BINDING_BIDIRECTIONAL);
    ctk_container_add (CTK_CONTAINER (box), entry);
  }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);

  return window;
}
