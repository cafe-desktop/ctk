/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
/*
 * CTK - The GIMP Toolkit
 * Copyright (C) 2006  Carlos Garnacho Parro <carlosg@gnome.org>
 *
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */
#include <ctk/ctk.h>

static gpointer GROUP_A = "GROUP_A";
static gpointer GROUP_B = "GROUP_B";

gchar *tabs1 [] = {
  "aaaaaaaaaa",
  "bbbbbbbbbb",
  "cccccccccc",
  "dddddddddd",
  NULL
};

gchar *tabs2 [] = {
  "1",
  "2",
  "3",
  "4",
  "55555",
  NULL
};

gchar *tabs3 [] = {
  "foo",
  "bar",
  NULL
};

gchar *tabs4 [] = {
  "beer",
  "water",
  "lemonade",
  "coffee",
  "tea",
  NULL
};

static const CtkTargetEntry button_targets[] = {
  { "CTK_NOTEBOOK_TAB", CTK_TARGET_SAME_APP, 0 },
};

static CtkNotebook*
window_creation_function (CtkNotebook *source_notebook,
                          CtkWidget   *child,
                          gint         x,
                          gint         y,
                          gpointer     data)
{
  CtkWidget *window, *notebook;

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  notebook = ctk_notebook_new ();
  g_signal_connect (notebook, "create-window",
                    G_CALLBACK (window_creation_function), NULL);

  ctk_notebook_set_group_name (CTK_NOTEBOOK (notebook),
                               ctk_notebook_get_group_name (source_notebook));

  ctk_container_add (CTK_CONTAINER (window), notebook);

  ctk_window_set_default_size (CTK_WINDOW (window), 300, 300);
  ctk_window_move (CTK_WINDOW (window), x, y);
  ctk_widget_show_all (window);

  return CTK_NOTEBOOK (notebook);
}

static void
on_page_reordered (CtkNotebook *notebook, CtkWidget *child, guint page_num, gpointer data)
{
  g_print ("page %d reordered\n", page_num);
}

static void
on_notebook_drag_begin (CtkWidget      *widget,
                        CdkDragContext *context,
                        gpointer        data)
{
  CdkPixbuf *pixbuf;
  guint page_num;

  page_num = ctk_notebook_get_current_page (CTK_NOTEBOOK (widget));

  if (page_num > 2)
    {
      CtkIconTheme *icon_theme;
      int width;

      icon_theme = ctk_icon_theme_get_for_screen (ctk_widget_get_screen (widget));
      ctk_icon_size_lookup (CTK_ICON_SIZE_DND, &width, NULL);
      pixbuf = ctk_icon_theme_load_icon (icon_theme,
                                         (page_num % 2) ? "help-browser" : "process-stop",
                                         width,
                                         CTK_ICON_LOOKUP_GENERIC_FALLBACK,
                                         NULL);

      ctk_drag_set_icon_pixbuf (context, pixbuf, 0, 0);
      g_object_unref (pixbuf);
    }
}

static gboolean
remove_in_idle (gpointer data)
{
  CtkWidget *child = data;
  CtkWidget *parent = ctk_widget_get_parent (child);
  CtkWidget *tab_label;

  tab_label = ctk_notebook_get_tab_label (CTK_NOTEBOOK (parent), child);
  g_print ("Removing tab: %s\n", ctk_label_get_text (CTK_LABEL (tab_label)));
  ctk_container_remove (CTK_CONTAINER (parent), child);

  return G_SOURCE_REMOVE;
}

static void
on_button_drag_data_received (CtkWidget        *widget,
                              CdkDragContext   *context,
                              gint              x,
                              gint              y,
                              CtkSelectionData *data,
                              guint             info,
                              guint             time,
                              gpointer          user_data)
{
  CtkWidget **child;

  child = (void*) ctk_selection_data_get_data (data);

  g_idle_add (remove_in_idle, *child);
}

static void
action_clicked_cb (CtkWidget *button,
                   CtkWidget *notebook)
{
  CtkWidget *page, *title;

  page = ctk_entry_new ();
  ctk_entry_set_text (CTK_ENTRY (page), "Addition");
  ctk_widget_show (page);

  title = ctk_label_new ("Addition");

  ctk_notebook_append_page (CTK_NOTEBOOK (notebook), page, title);
  ctk_notebook_set_tab_reorderable (CTK_NOTEBOOK (notebook), page, TRUE);
  ctk_notebook_set_tab_detachable (CTK_NOTEBOOK (notebook), page, TRUE);
}

static CtkWidget*
create_notebook (gchar           **labels,
                 const gchar      *group,
                 CtkPositionType   pos)
{
  CtkWidget *notebook, *title, *page, *action_widget;

  notebook = ctk_notebook_new ();
  ctk_widget_set_vexpand (notebook, TRUE);
  ctk_widget_set_hexpand (notebook, TRUE);

  action_widget = ctk_button_new_from_icon_name ("list-add-symbolic", CTK_ICON_SIZE_BUTTON);
  g_signal_connect (action_widget, "clicked", G_CALLBACK (action_clicked_cb), notebook);
  ctk_widget_show (action_widget);
  ctk_notebook_set_action_widget (CTK_NOTEBOOK (notebook), action_widget, CTK_PACK_END);

  g_signal_connect (notebook, "create-window",
                    G_CALLBACK (window_creation_function), NULL);

  ctk_notebook_set_tab_pos (CTK_NOTEBOOK (notebook), pos);
  ctk_notebook_set_scrollable (CTK_NOTEBOOK (notebook), TRUE);
  ctk_container_set_border_width (CTK_CONTAINER (notebook), 6);
  ctk_notebook_set_group_name (CTK_NOTEBOOK (notebook), group);

  while (*labels)
    {
      page = ctk_entry_new ();
      ctk_entry_set_text (CTK_ENTRY (page), *labels);

      title = ctk_label_new (*labels);

      ctk_notebook_append_page (CTK_NOTEBOOK (notebook), page, title);
      ctk_notebook_set_tab_reorderable (CTK_NOTEBOOK (notebook), page, TRUE);
      ctk_notebook_set_tab_detachable (CTK_NOTEBOOK (notebook), page, TRUE);

      labels++;
    }

  g_signal_connect (CTK_NOTEBOOK (notebook), "page-reordered",
                    G_CALLBACK (on_page_reordered), NULL);
  g_signal_connect_after (G_OBJECT (notebook), "drag-begin",
                          G_CALLBACK (on_notebook_drag_begin), NULL);
  return notebook;
}

static CtkWidget*
create_notebook_non_dragable_content (gchar           **labels,
                                      const gchar      *group,
                                      CtkPositionType   pos)
{
  CtkWidget *notebook, *title, *page, *action_widget;

  notebook = ctk_notebook_new ();
  ctk_widget_set_vexpand (notebook, TRUE);
  ctk_widget_set_hexpand (notebook, TRUE);

  action_widget = ctk_button_new_from_icon_name ("list-add-symbolic", CTK_ICON_SIZE_BUTTON);
  g_signal_connect (action_widget, "clicked", G_CALLBACK (action_clicked_cb), notebook);
  ctk_widget_show (action_widget);
  ctk_notebook_set_action_widget (CTK_NOTEBOOK (notebook), action_widget, CTK_PACK_END);

  g_signal_connect (notebook, "create-window",
                    G_CALLBACK (window_creation_function), NULL);

  ctk_notebook_set_tab_pos (CTK_NOTEBOOK (notebook), pos);
  ctk_notebook_set_scrollable (CTK_NOTEBOOK (notebook), TRUE);
  ctk_container_set_border_width (CTK_CONTAINER (notebook), 6);
  ctk_notebook_set_group_name (CTK_NOTEBOOK (notebook), group);

  while (*labels)
    {
      CtkWidget *button;
      button = ctk_button_new_with_label (*labels);
      /* Use CtkListBox since it bubbles up motion notify event, which can
       * experience more issues than CtkBox. */
      page = ctk_list_box_new ();
      ctk_container_add (CTK_CONTAINER (page), button);

      title = ctk_label_new (*labels);

      ctk_notebook_append_page (CTK_NOTEBOOK (notebook), page, title);
      ctk_notebook_set_tab_reorderable (CTK_NOTEBOOK (notebook), page, TRUE);
      ctk_notebook_set_tab_detachable (CTK_NOTEBOOK (notebook), page, TRUE);

      labels++;
    }

  g_signal_connect (CTK_NOTEBOOK (notebook), "page-reordered",
                    G_CALLBACK (on_page_reordered), NULL);
  g_signal_connect_after (G_OBJECT (notebook), "drag-begin",
                          G_CALLBACK (on_notebook_drag_begin), NULL);
  return notebook;
}

static CtkWidget*
create_notebook_with_notebooks (gchar           **labels,
                                const gchar      *group,
                                CtkPositionType   pos)
{
  CtkWidget *notebook, *title, *page;
  gint count = 0;

  notebook = ctk_notebook_new ();
  g_signal_connect (notebook, "create-window",
                    G_CALLBACK (window_creation_function), NULL);

  ctk_notebook_set_tab_pos (CTK_NOTEBOOK (notebook), pos);
  ctk_notebook_set_scrollable (CTK_NOTEBOOK (notebook), TRUE);
  ctk_container_set_border_width (CTK_CONTAINER (notebook), 6);
  ctk_notebook_set_group_name (CTK_NOTEBOOK (notebook), group);

  while (*labels)
    {
      page = create_notebook (labels, group, pos);
      ctk_notebook_popup_enable (CTK_NOTEBOOK (page));

      title = ctk_label_new (*labels);

      ctk_notebook_append_page (CTK_NOTEBOOK (notebook), page, title);
      ctk_notebook_set_tab_reorderable (CTK_NOTEBOOK (notebook), page, TRUE);
      ctk_notebook_set_tab_detachable (CTK_NOTEBOOK (notebook), page, TRUE);

      count++;
      labels++;
    }

  g_signal_connect (CTK_NOTEBOOK (notebook), "page-reordered",
                    G_CALLBACK (on_page_reordered), NULL);
  g_signal_connect_after (G_OBJECT (notebook), "drag-begin",
                          G_CALLBACK (on_notebook_drag_begin), NULL);
  return notebook;
}

static CtkWidget*
create_trash_button (void)
{
  CtkWidget *button;

  button = ctk_button_new_with_mnemonic ("_Delete");

  ctk_drag_dest_set (button,
                     CTK_DEST_DEFAULT_MOTION | CTK_DEST_DEFAULT_DROP,
                     button_targets,
                     G_N_ELEMENTS (button_targets),
                     CDK_ACTION_MOVE);

  g_signal_connect_after (G_OBJECT (button), "drag-data-received",
                          G_CALLBACK (on_button_drag_data_received), NULL);
  return button;
}

gint
main (gint argc, gchar *argv[])
{
  CtkWidget *window, *grid;

  ctk_init (&argc, &argv);

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  grid = ctk_grid_new ();

  ctk_grid_attach (CTK_GRID (grid),
                   create_notebook_non_dragable_content (tabs1, GROUP_A, CTK_POS_TOP),
                   0, 0, 1, 1);

  ctk_grid_attach (CTK_GRID (grid),
                   create_notebook (tabs2, GROUP_B, CTK_POS_BOTTOM),
                   0, 1, 1, 1);

  ctk_grid_attach (CTK_GRID (grid),
                   create_notebook (tabs3, GROUP_B, CTK_POS_LEFT),
                   1, 0, 1, 1);

  ctk_grid_attach (CTK_GRID (grid),
                   create_notebook_with_notebooks (tabs4, GROUP_A, CTK_POS_RIGHT),
                   1, 1, 1, 1);

  ctk_grid_attach (CTK_GRID (grid),
                   create_trash_button (),
                   1, 2, 1, 1);

  ctk_container_add (CTK_CONTAINER (window), grid);
  ctk_window_set_default_size (CTK_WINDOW (window), 400, 400);
  ctk_widget_show_all (window);

  ctk_main ();

  return 0;
}
