/* Icon View/Editing and Drag-and-Drop
 *
 * The CtkIconView widget supports Editing and Drag-and-Drop.
 * This example also demonstrates using the generic CtkCellLayout
 * interface to set up cell renderers in an icon view.
 */

#include <ctk/ctk.h>
#include <string.h>

enum
{
  COL_TEXT,
  NUM_COLS
};


static void
fill_store (CtkListStore *store)
{
  CtkTreeIter iter;
  const gchar *text[] = { "Red", "Green", "Blue", "Yellow" };
  gint i;

  /* First clear the store */
  ctk_list_store_clear (store);

  for (i = 0; i < 4; i++)
    {
      ctk_list_store_append (store, &iter);
      ctk_list_store_set (store, &iter, COL_TEXT, text[i], -1);
    }
}

static CtkListStore *
create_store (void)
{
  CtkListStore *store;

  store = ctk_list_store_new (NUM_COLS, G_TYPE_STRING);

  return store;
}

static void
set_cell_color (CtkCellLayout   *cell_layout,
                CtkCellRenderer *cell,
                CtkTreeModel    *tree_model,
                CtkTreeIter     *iter,
                gpointer         data)
{
  gchar *text;
  CdkRGBA color;
  guint32 pixel = 0;
  GdkPixbuf *pixbuf;

  ctk_tree_model_get (tree_model, iter, COL_TEXT, &text, -1);
  if (!text)
    return;

  if (cdk_rgba_parse (&color, text))
    pixel =
      ((gint)(color.red * 255)) << 24 |
      ((gint)(color.green * 255)) << 16 |
      ((gint)(color.blue  * 255)) << 8 |
      ((gint)(color.alpha * 255));

  g_free (text);

  pixbuf = gdk_pixbuf_new (CDK_COLORSPACE_RGB, TRUE, 8, 24, 24);
  gdk_pixbuf_fill (pixbuf, pixel);

  g_object_set (cell, "pixbuf", pixbuf, NULL);

  g_object_unref (pixbuf);
}

static void
edited (CtkCellRendererText *cell,
        gchar               *path_string,
        gchar               *text,
        gpointer             data)
{
  CtkTreeModel *model;
  CtkTreeIter iter;
  CtkTreePath *path;

  model = ctk_icon_view_get_model (CTK_ICON_VIEW (data));
  path = ctk_tree_path_new_from_string (path_string);

  ctk_tree_model_get_iter (model, &iter, path);
  ctk_list_store_set (CTK_LIST_STORE (model), &iter,
                      COL_TEXT, text, -1);

  ctk_tree_path_free (path);
}

CtkWidget *
do_iconview_edit (CtkWidget *do_widget)
{
  static CtkWidget *window = NULL;

  if (!window)
    {
      CtkWidget *icon_view;
      CtkListStore *store;
      CtkCellRenderer *renderer;

      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);

      ctk_window_set_screen (CTK_WINDOW (window),
                             ctk_widget_get_screen (do_widget));
      ctk_window_set_title (CTK_WINDOW (window), "Editing and Drag-and-Drop");

      g_signal_connect (window, "destroy",
                        G_CALLBACK (ctk_widget_destroyed), &window);

      store = create_store ();
      fill_store (store);

      icon_view = ctk_icon_view_new_with_model (CTK_TREE_MODEL (store));
      g_object_unref (store);

      ctk_icon_view_set_selection_mode (CTK_ICON_VIEW (icon_view),
                                        CTK_SELECTION_SINGLE);
      ctk_icon_view_set_item_orientation (CTK_ICON_VIEW (icon_view),
                                          CTK_ORIENTATION_HORIZONTAL);
      ctk_icon_view_set_columns (CTK_ICON_VIEW (icon_view), 2);
      ctk_icon_view_set_reorderable (CTK_ICON_VIEW (icon_view), TRUE);

      renderer = ctk_cell_renderer_pixbuf_new ();
      ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (icon_view),
                                  renderer, TRUE);
      ctk_cell_layout_set_cell_data_func (CTK_CELL_LAYOUT (icon_view),
                                          renderer,
                                          set_cell_color,
                                          NULL, NULL);

      renderer = ctk_cell_renderer_text_new ();
      ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (icon_view),
                                  renderer, TRUE);
      g_object_set (renderer, "editable", TRUE, NULL);
      g_signal_connect (renderer, "edited", G_CALLBACK (edited), icon_view);
      ctk_cell_layout_set_attributes (CTK_CELL_LAYOUT (icon_view),
                                      renderer,
                                      "text", COL_TEXT,
                                      NULL);

      ctk_container_add (CTK_CONTAINER (window), icon_view);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);

  return window;
}
