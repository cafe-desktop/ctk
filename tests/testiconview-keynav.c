/* testiconview-keynav.c
 * Copyright (C) 2010  Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Matthias Clasen
 */

/*
 * This example demonstrates how to use the keynav-failed signal to
 * extend arrow keynav over adjacent icon views. This can be used when
 * grouping items.
 */

#include <gtk/gtk.h>

static GtkTreeModel *
get_model (void)
{
  static GtkListStore *store;
  GtkTreeIter iter;

  if (store)
    return (GtkTreeModel *) g_object_ref (store);

  store = ctk_list_store_new (1, G_TYPE_STRING);

  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 0, "One", -1);
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 0, "Two", -1);
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 0, "Three", -1);
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 0, "Four", -1);
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 0, "Five", -1);
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 0, "Six", -1);
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 0, "Seven", -1);
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 0, "Eight", -1);

  return (GtkTreeModel *) store;
}

static gboolean
visible_func (GtkTreeModel *model,
              GtkTreeIter  *iter,
              gpointer      data)
{
  gboolean first = GPOINTER_TO_INT (data);
  gboolean visible;
  GtkTreePath *path;

  path = ctk_tree_model_get_path (model, iter);

  if (ctk_tree_path_get_indices (path)[0] < 4)
    visible = first;
  else
    visible = !first;

  ctk_tree_path_free (path);

  return visible;
}

GtkTreeModel *
get_filter_model (gboolean first)
{
  GtkTreeModelFilter *model;

  model = (GtkTreeModelFilter *)ctk_tree_model_filter_new (get_model (), NULL);

  ctk_tree_model_filter_set_visible_func (model, visible_func, GINT_TO_POINTER (first), NULL);

  return (GtkTreeModel *) model;
}

static GtkWidget *
get_view (gboolean first)
{
  GtkWidget *view;

  view = ctk_icon_view_new_with_model (get_filter_model (first));
  ctk_icon_view_set_text_column (CTK_ICON_VIEW (view), 0);
  ctk_widget_set_size_request (view, 0, -1);

  return view;
}

typedef struct
{
  GtkWidget *header1;
  GtkWidget *view1;
  GtkWidget *header2;
  GtkWidget *view2;
} Views;

static gboolean
keynav_failed (GtkWidget        *view,
               GtkDirectionType  direction,
               Views            *views)
{
  GtkTreePath *path;
  GtkTreeModel *model;
  GtkTreeIter iter;
  gint col;
  GtkTreePath *sel;

  if (view == views->view1 && direction == CTK_DIR_DOWN)
    {
      if (ctk_icon_view_get_cursor (CTK_ICON_VIEW (views->view1), &path, NULL))
        {
          col = ctk_icon_view_get_item_column (CTK_ICON_VIEW (views->view1), path);
          ctk_tree_path_free (path);

          sel = NULL;
          model = ctk_icon_view_get_model (CTK_ICON_VIEW (views->view2));
          ctk_tree_model_get_iter_first (model, &iter);
          do {
            path = ctk_tree_model_get_path (model, &iter);
            if (ctk_icon_view_get_item_column (CTK_ICON_VIEW (views->view2), path) == col)
              {
                sel = path;
                break;
              }
          } while (ctk_tree_model_iter_next (model, &iter));

          ctk_icon_view_set_cursor (CTK_ICON_VIEW (views->view2), sel, NULL, FALSE);
          ctk_tree_path_free (sel);
        }
      ctk_widget_grab_focus (views->view2);
      return TRUE;
    }

  if (view == views->view2 && direction == CTK_DIR_UP)
    {
      if (ctk_icon_view_get_cursor (CTK_ICON_VIEW (views->view2), &path, NULL))
        {
          col = ctk_icon_view_get_item_column (CTK_ICON_VIEW (views->view2), path);
          ctk_tree_path_free (path);

          sel = NULL;
          model = ctk_icon_view_get_model (CTK_ICON_VIEW (views->view1));
          ctk_tree_model_get_iter_first (model, &iter);
          do {
            path = ctk_tree_model_get_path (model, &iter);
            if (ctk_icon_view_get_item_column (CTK_ICON_VIEW (views->view1), path) == col)
              {
                if (sel)
                  ctk_tree_path_free (sel);
                sel = path;
              }
            else
              ctk_tree_path_free (path);
          } while (ctk_tree_model_iter_next (model, &iter));

          ctk_icon_view_set_cursor (CTK_ICON_VIEW (views->view1), sel, NULL, FALSE);
          ctk_tree_path_free (sel);
        }
      ctk_widget_grab_focus (views->view1);
      return TRUE;
    }

  return FALSE;
}

static gboolean
focus_out (GtkWidget     *view,
           GdkEventFocus *event,
           gpointer       data)
{
  ctk_icon_view_unselect_all (CTK_ICON_VIEW (view));

  return FALSE;
}

static gboolean
focus_in (GtkWidget     *view,
          GdkEventFocus *event,
          gpointer       data)
{
  GtkTreePath *path;

  if (!ctk_icon_view_get_cursor (CTK_ICON_VIEW (view), &path, NULL))
    {
      path = ctk_tree_path_new_from_indices (0, -1);
      ctk_icon_view_set_cursor (CTK_ICON_VIEW (view), path, NULL, FALSE);
    }

  ctk_icon_view_select_path (CTK_ICON_VIEW (view), path);
  ctk_tree_path_free (path);

  return FALSE;
}

#define CSS \
  "GtkWindow {\n" \
  "  background-color: @base_color;\n" \
  "}\n"

static void
set_styles (void)
{
  GtkCssProvider *provider;
  GdkScreen *screen;

  provider = ctk_css_provider_new ();

  if (!ctk_css_provider_load_from_data (provider, CSS, -1, NULL))
    {
      g_assert_not_reached ();
    }

  screen = gdk_display_get_default_screen (gdk_display_get_default ());

  ctk_style_context_add_provider_for_screen (screen, CTK_STYLE_PROVIDER (provider),
                                             CTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

int
main (int argc, char *argv[])
{
  GtkWidget *window;
  GtkWidget *vbox;
  Views views;

  ctk_init (&argc, &argv);

  set_styles ();

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  ctk_container_add (CTK_CONTAINER (window), vbox);

  views.header1 = g_object_new (CTK_TYPE_LABEL,
                                "label", "<b>Group 1</b>",
                                "use-markup", TRUE,
                                "xalign", 0.0,
                                NULL);
  views.view1 = get_view (TRUE);
  views.header2 = g_object_new (CTK_TYPE_LABEL,
                                "label", "<b>Group 2</b>",
                                "use-markup", TRUE,
                                "xalign", 0.0,
                                NULL);
  views.view2 = get_view (FALSE);

  g_signal_connect (views.view1, "keynav-failed",
                    G_CALLBACK (keynav_failed), &views);
  g_signal_connect (views.view2, "keynav-failed",
                    G_CALLBACK (keynav_failed), &views);
  g_signal_connect (views.view1, "focus-in-event",
                    G_CALLBACK (focus_in), NULL);
  g_signal_connect (views.view1, "focus-out-event",
                    G_CALLBACK (focus_out), NULL);
  g_signal_connect (views.view2, "focus-in-event",
                    G_CALLBACK (focus_in), NULL);
  g_signal_connect (views.view2, "focus-out-event",
                    G_CALLBACK (focus_out), NULL);

  ctk_container_add (CTK_CONTAINER (vbox), views.header1);
  ctk_container_add (CTK_CONTAINER (vbox), views.view1);
  ctk_container_add (CTK_CONTAINER (vbox), views.header2);
  ctk_container_add (CTK_CONTAINER (vbox), views.view2);

  ctk_widget_show_all (window);

  ctk_main ();

  return 0;
}

