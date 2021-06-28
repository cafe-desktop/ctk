#include <ctk/ctk.h>

static gboolean
clicked_icon (CtkTreeView  *tv,
              gint          x,
              gint          y,
              CtkTreePath **path)
{
  CtkTreeViewColumn *col;
  gint cell_x, cell_y;
  gint cell_pos, cell_width;
  GList *cells, *l;
  gint depth;
  gint level_indentation;
  gint expander_size;
  gint indent;

  if (ctk_tree_view_get_path_at_pos (tv, x, y, path, &col, &cell_x, &cell_y))
    {
      cells = ctk_cell_layout_get_cells (CTK_CELL_LAYOUT (col));

#if 1
      /* ugly workaround to fix the problem:
       * manually calculate the indent for the row
       */
      depth = ctk_tree_path_get_depth (*path);
      level_indentation = ctk_tree_view_get_level_indentation (tv);
      ctk_widget_style_get (CTK_WIDGET (tv), "expander-size", &expander_size, NULL);
      expander_size += 4;
      indent = (depth - 1) * level_indentation + depth * expander_size;
#else
      indent = 0;
#endif

      for (l = cells; l; l = l->next)
        {
          ctk_tree_view_column_cell_get_position (col, l->data, &cell_pos, &cell_width);
          if (cell_pos + indent <= cell_x && cell_x <= cell_pos + indent + cell_width)
            {
              g_print ("clicked in %s\n", g_type_name_from_instance (l->data));
              if (CTK_IS_CELL_RENDERER_PIXBUF (l->data))
                {
                  g_list_free (cells);
                  return TRUE;
                }
            }
        }

      g_list_free (cells);
    }

  return FALSE;
}

static gboolean
release_event (CtkTreeView    *tv,
               CdkEventButton *event)
{
  CtkTreePath *path;

  if (event->type != CDK_BUTTON_RELEASE)
    return TRUE;

  if (clicked_icon (tv, event->x, event->y, &path))
    {
      CtkTreeModel *model;
      CtkTreeIter iter;
      gchar *text;

      model = ctk_tree_view_get_model (tv);
      ctk_tree_model_get_iter (model, &iter, path);
      ctk_tree_model_get (model, &iter, 0, &text, -1);

      g_print ("text was: %s\n", text);
      g_free (text);
      ctk_tree_path_free (path);

      return TRUE;
    }

  return FALSE;
}

int main (int argc, char *argv[])
{
  CtkWidget *window;
  CtkWidget *sw;
  CtkWidget *tv;
  CtkTreeViewColumn *col;
  CtkCellRenderer *cell;
  CtkTreeStore *store;
  CtkTreeIter iter;

  ctk_init (&argc, &argv);

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  sw = ctk_scrolled_window_new (NULL, NULL);
  ctk_container_add (CTK_CONTAINER (window), sw);
  tv = ctk_tree_view_new ();
  ctk_container_add (CTK_CONTAINER (sw), tv);

  col = ctk_tree_view_column_new ();
  cell = ctk_cell_renderer_text_new ();
  ctk_tree_view_column_pack_start (col, cell, TRUE);
  ctk_tree_view_column_add_attribute (col, cell, "text", 0);

  cell = ctk_cell_renderer_toggle_new ();
  ctk_tree_view_column_pack_start (col, cell, FALSE);
  ctk_tree_view_column_add_attribute (col, cell, "active", 1);

  cell = ctk_cell_renderer_text_new ();
  ctk_tree_view_column_pack_start (col, cell, TRUE);
  ctk_tree_view_column_add_attribute (col, cell, "text", 0);

  cell = ctk_cell_renderer_pixbuf_new ();
  ctk_tree_view_column_pack_start (col, cell, FALSE);
  ctk_tree_view_column_add_attribute (col, cell, "icon-name", 2);

  cell = ctk_cell_renderer_toggle_new ();
  ctk_tree_view_column_pack_start (col, cell, FALSE);
  ctk_tree_view_column_add_attribute (col, cell, "active", 1);

  ctk_tree_view_append_column (CTK_TREE_VIEW (tv), col);

  store = ctk_tree_store_new (3, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_STRING);
  ctk_tree_store_insert_with_values (store, NULL, NULL, 0, 0, "One row", 1, FALSE, 2, "document-open", -1);
  ctk_tree_store_insert_with_values (store, &iter, NULL, 1, 0, "Two row", 1, FALSE, 2, "dialog-warning", -1);
  ctk_tree_store_insert_with_values (store, NULL, &iter, 0, 0, "Three row", 1, FALSE, 2, "dialog-error", -1);

  ctk_tree_view_set_model (CTK_TREE_VIEW (tv), CTK_TREE_MODEL (store));

  g_signal_connect (tv, "button-release-event",
                    G_CALLBACK (release_event), NULL);

  ctk_widget_show_all (window);

  ctk_main ();

  return 0;
}
