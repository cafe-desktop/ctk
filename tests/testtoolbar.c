/* testtoolbar.c
 *
 * Copyright (C) 2002 Anders Carlsson <andersca@codefactory.se>
 * Copyright (C) 2002 James Henstridge <james@daa.com.au>
 * Copyright (C) 2003 Soeren Sandmann <sandmann@daimi.au.dk>
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
 */
#include "config.h"
#include <ctk/ctk.h>

static void
change_orientation (CtkWidget *button, CtkWidget *toolbar)
{
  CtkWidget *grid;
  CtkOrientation orientation;

  grid = ctk_widget_get_parent (toolbar);
  if (ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (button)))
    orientation = CTK_ORIENTATION_VERTICAL;
  else
    orientation = CTK_ORIENTATION_HORIZONTAL;

  g_object_ref (toolbar);
  ctk_container_remove (CTK_CONTAINER (grid), toolbar);
  ctk_orientable_set_orientation (CTK_ORIENTABLE (toolbar), orientation);
  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    {
      ctk_widget_set_hexpand (toolbar, TRUE);
      ctk_widget_set_vexpand (toolbar, FALSE);
      ctk_grid_attach (CTK_GRID (grid), toolbar, 0, 0, 2, 1);
    }
  else
    {
      ctk_widget_set_hexpand (toolbar, FALSE);
      ctk_widget_set_vexpand (toolbar, TRUE);
      ctk_grid_attach (CTK_GRID (grid), toolbar, 0, 0, 1, 5);
    }
  g_object_unref (toolbar);
}

static void
change_show_arrow (CtkWidget *button, CtkWidget *toolbar)
{
  ctk_toolbar_set_show_arrow (CTK_TOOLBAR (toolbar),
		ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (button)));
}

static void
set_toolbar_style_toggled (CtkCheckButton *button, CtkToolbar *toolbar)
{
  CtkWidget *option_menu;
  
  option_menu = g_object_get_data (G_OBJECT (button), "option-menu");

  if (ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (button)))
    {
      int style;

      style = ctk_combo_box_get_active (CTK_COMBO_BOX (option_menu));

      ctk_toolbar_set_style (toolbar, style);
      ctk_widget_set_sensitive (option_menu, TRUE);
    }
  else
    {
      ctk_toolbar_unset_style (toolbar);
      ctk_widget_set_sensitive (option_menu, FALSE);
    }
}

static void
change_toolbar_style (CtkWidget *option_menu, CtkWidget *toolbar)
{
  CtkToolbarStyle style;

  style = ctk_combo_box_get_active (CTK_COMBO_BOX (option_menu));
  ctk_toolbar_set_style (CTK_TOOLBAR (toolbar), style);
}

static void
set_visible_func(CtkTreeViewColumn *tree_column, CtkCellRenderer *cell,
		 CtkTreeModel *model, CtkTreeIter *iter, gpointer data)
{
  CtkToolItem *tool_item;
  gboolean visible;

  ctk_tree_model_get (model, iter, 0, &tool_item, -1);

  g_object_get (tool_item, "visible", &visible, NULL);
  g_object_set (cell, "active", visible, NULL);
  g_object_unref (tool_item);
}

static void
visibile_toggled(CtkCellRendererToggle *cell, const gchar *path_str,
		 CtkTreeModel *model)
{
  CtkTreePath *path;
  CtkTreeIter iter;
  CtkToolItem *tool_item;
  gboolean visible;

  path = ctk_tree_path_new_from_string (path_str);
  ctk_tree_model_get_iter (model, &iter, path);

  ctk_tree_model_get (model, &iter, 0, &tool_item, -1);
  g_object_get (tool_item, "visible", &visible, NULL);
  g_object_set (tool_item, "visible", !visible, NULL);
  g_object_unref (tool_item);

  ctk_tree_model_row_changed (model, path, &iter);
  ctk_tree_path_free (path);
}

static void
set_expand_func(CtkTreeViewColumn *tree_column, CtkCellRenderer *cell,
		CtkTreeModel *model, CtkTreeIter *iter, gpointer data)
{
  CtkToolItem *tool_item;

  ctk_tree_model_get (model, iter, 0, &tool_item, -1);

  g_object_set (cell, "active", ctk_tool_item_get_expand (tool_item), NULL);
  g_object_unref (tool_item);
}

static void
expand_toggled(CtkCellRendererToggle *cell, const gchar *path_str,
	       CtkTreeModel *model)
{
  CtkTreePath *path;
  CtkTreeIter iter;
  CtkToolItem *tool_item;

  path = ctk_tree_path_new_from_string (path_str);
  ctk_tree_model_get_iter (model, &iter, path);

  ctk_tree_model_get (model, &iter, 0, &tool_item, -1);
  ctk_tool_item_set_expand (tool_item, !ctk_tool_item_get_expand (tool_item));
  g_object_unref (tool_item);

  ctk_tree_model_row_changed (model, path, &iter);
  ctk_tree_path_free (path);
}

static void
set_homogeneous_func(CtkTreeViewColumn *tree_column, CtkCellRenderer *cell,
		     CtkTreeModel *model, CtkTreeIter *iter, gpointer data)
{
  CtkToolItem *tool_item;

  ctk_tree_model_get (model, iter, 0, &tool_item, -1);

  g_object_set (cell, "active", ctk_tool_item_get_homogeneous (tool_item), NULL);
  g_object_unref (tool_item);
}

static void
homogeneous_toggled(CtkCellRendererToggle *cell, const gchar *path_str,
		    CtkTreeModel *model)
{
  CtkTreePath *path;
  CtkTreeIter iter;
  CtkToolItem *tool_item;

  path = ctk_tree_path_new_from_string (path_str);
  ctk_tree_model_get_iter (model, &iter, path);

  ctk_tree_model_get (model, &iter, 0, &tool_item, -1);
  ctk_tool_item_set_homogeneous (tool_item, !ctk_tool_item_get_homogeneous (tool_item));
  g_object_unref (tool_item);

  ctk_tree_model_row_changed (model, path, &iter);
  ctk_tree_path_free (path);
}


static void
set_important_func(CtkTreeViewColumn *tree_column, CtkCellRenderer *cell,
		   CtkTreeModel *model, CtkTreeIter *iter, gpointer data)
{
  CtkToolItem *tool_item;

  ctk_tree_model_get (model, iter, 0, &tool_item, -1);

  g_object_set (cell, "active", ctk_tool_item_get_is_important (tool_item), NULL);
  g_object_unref (tool_item);
}

static void
important_toggled(CtkCellRendererToggle *cell, const gchar *path_str,
		  CtkTreeModel *model)
{
  CtkTreePath *path;
  CtkTreeIter iter;
  CtkToolItem *tool_item;

  path = ctk_tree_path_new_from_string (path_str);
  ctk_tree_model_get_iter (model, &iter, path);

  ctk_tree_model_get (model, &iter, 0, &tool_item, -1);
  ctk_tool_item_set_is_important (tool_item, !ctk_tool_item_get_is_important (tool_item));
  g_object_unref (tool_item);

  ctk_tree_model_row_changed (model, path, &iter);
  ctk_tree_path_free (path);
}

static CtkListStore *
create_items_list (CtkWidget **tree_view_p)
{
  CtkWidget *tree_view;
  CtkListStore *list_store;
  CtkCellRenderer *cell;
  
  list_store = ctk_list_store_new (2, CTK_TYPE_TOOL_ITEM, G_TYPE_STRING);
  
  tree_view = ctk_tree_view_new_with_model (CTK_TREE_MODEL (list_store));

  ctk_tree_view_insert_column_with_attributes (CTK_TREE_VIEW (tree_view),
					       -1, "Tool Item",
					       ctk_cell_renderer_text_new (),
					       "text", 1, NULL);

  cell = ctk_cell_renderer_toggle_new ();
  g_signal_connect (cell, "toggled", G_CALLBACK (visibile_toggled),
		    list_store);
  ctk_tree_view_insert_column_with_data_func (CTK_TREE_VIEW (tree_view),
					      -1, "Visible",
					      cell,
					      set_visible_func, NULL, NULL);

  cell = ctk_cell_renderer_toggle_new ();
  g_signal_connect (cell, "toggled", G_CALLBACK (expand_toggled),
		    list_store);
  ctk_tree_view_insert_column_with_data_func (CTK_TREE_VIEW (tree_view),
					      -1, "Expand",
					      cell,
					      set_expand_func, NULL, NULL);

  cell = ctk_cell_renderer_toggle_new ();
  g_signal_connect (cell, "toggled", G_CALLBACK (homogeneous_toggled),
		    list_store);
  ctk_tree_view_insert_column_with_data_func (CTK_TREE_VIEW (tree_view),
					      -1, "Homogeneous",
					      cell,
					      set_homogeneous_func, NULL,NULL);

  cell = ctk_cell_renderer_toggle_new ();
  g_signal_connect (cell, "toggled", G_CALLBACK (important_toggled),
		    list_store);
  ctk_tree_view_insert_column_with_data_func (CTK_TREE_VIEW (tree_view),
					      -1, "Important",
					      cell,
					      set_important_func, NULL,NULL);

  g_object_unref (list_store);

  *tree_view_p = tree_view;

  return list_store;
}

static void
add_item_to_list (CtkListStore *store, CtkToolItem *item, const gchar *text)
{
  CtkTreeIter iter;

  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter,
		      0, item,
		      1, text,
		      -1);
  
}

static void
bold_toggled (CtkToggleToolButton *button)
{
  g_message ("Bold toggled (active=%d)",
	     ctk_toggle_tool_button_get_active (button));
}

static void
set_icon_size_toggled (CtkCheckButton *button, CtkToolbar *toolbar)
{
  CtkWidget *option_menu;
  
  option_menu = g_object_get_data (G_OBJECT (button), "option-menu");

  if (ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (button)))
    {
      int icon_size;

      if (ctk_combo_box_get_active (CTK_COMBO_BOX (option_menu)) == 0)
        icon_size = CTK_ICON_SIZE_SMALL_TOOLBAR;
      else
        icon_size = CTK_ICON_SIZE_LARGE_TOOLBAR;

      ctk_toolbar_set_icon_size (toolbar, icon_size);
      ctk_widget_set_sensitive (option_menu, TRUE);
    }
  else
    {
      ctk_toolbar_unset_icon_size (toolbar);
      ctk_widget_set_sensitive (option_menu, FALSE);
    }
}

static void
icon_size_history_changed (CtkComboBox *menu, CtkToolbar *toolbar)
{
  int icon_size;

  if (ctk_combo_box_get_active (menu) == 0)
    icon_size = CTK_ICON_SIZE_SMALL_TOOLBAR;
  else
    icon_size = CTK_ICON_SIZE_LARGE_TOOLBAR;

  ctk_toolbar_set_icon_size (toolbar, icon_size);
}

static gboolean
toolbar_drag_drop (CtkWidget *widget, CdkDragContext *context,
		   gint x, gint y, guint time, CtkWidget *label)
{
  gchar buf[32];

  g_snprintf(buf, sizeof(buf), "%d",
	     ctk_toolbar_get_drop_index (CTK_TOOLBAR (widget), x, y));
  ctk_label_set_label (CTK_LABEL (label), buf);

  return TRUE;
}

static CtkTargetEntry target_table[] = {
  { "application/x-toolbar-item", 0, 0 }
};

static void
rtl_toggled (CtkCheckButton *check)
{
  if (ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (check)))
    ctk_widget_set_default_direction (CTK_TEXT_DIR_RTL);
  else
    ctk_widget_set_default_direction (CTK_TEXT_DIR_LTR);
}

typedef struct
{
  int x;
  int y;
} MenuPositionData;

static void
position_function (CtkMenu *menu, gint *x, gint *y, gboolean *push_in, gpointer user_data)
{
  /* Do not do this in your own code */

  MenuPositionData *position_data = user_data;

  if (x)
    *x = position_data->x;

  if (y)
    *y = position_data->y;

  if (push_in)
    *push_in = FALSE;
}

static gboolean
popup_context_menu (CtkToolbar *toolbar, gint x, gint y, gint button_number)
{
  MenuPositionData position_data;
  
  CtkMenu *menu = CTK_MENU (ctk_menu_new ());
  int i;

  for (i = 0; i < 5; i++)
    {
      CtkWidget *item;
      gchar *label = g_strdup_printf ("Item _%d", i);
      item = ctk_menu_item_new_with_mnemonic (label);
      ctk_menu_shell_append (CTK_MENU_SHELL (menu), item);
    }
  ctk_widget_show_all (CTK_WIDGET (menu));

  if (button_number != -1)
    {
      position_data.x = x;
      position_data.y = y;
      
      ctk_menu_popup (menu, NULL, NULL, position_function,
		      &position_data, button_number, ctk_get_current_event_time());
    }
  else
    ctk_menu_popup (menu, NULL, NULL, NULL, NULL, 0, ctk_get_current_event_time());

  return TRUE;
}

static CtkToolItem *drag_item = NULL;

static gboolean
toolbar_drag_motion (CtkToolbar     *toolbar,
		     CdkDragContext *context,
		     gint            x,
		     gint            y,
		     guint           time,
		     gpointer        null)
{
  gint index;
  
  if (!drag_item)
    {
      drag_item = ctk_tool_button_new (NULL, "A quite long button");
      g_object_ref_sink (g_object_ref (drag_item));
    }
  
  cdk_drag_status (context, CDK_ACTION_MOVE, time);

  index = ctk_toolbar_get_drop_index (toolbar, x, y);
  
  ctk_toolbar_set_drop_highlight_item (toolbar, drag_item, index);
  
  return TRUE;
}

static void
toolbar_drag_leave (CtkToolbar     *toolbar,
		    CdkDragContext *context,
		    guint           time,
		    gpointer	    null)
{
  if (drag_item)
    {
      g_object_unref (drag_item);
      drag_item = NULL;
    }
  
  ctk_toolbar_set_drop_highlight_item (toolbar, NULL, 0);
}

static gboolean
timeout_cb (CtkWidget *widget)
{
  static gboolean sensitive = TRUE;
  
  sensitive = !sensitive;
  
  ctk_widget_set_sensitive (widget, sensitive);
  
  return TRUE;
}

static gboolean
timeout_cb1 (CtkWidget *widget)
{
	static gboolean sensitive = TRUE;
	sensitive = !sensitive;
	ctk_widget_set_sensitive (widget, sensitive);
	return TRUE;
}

gint
main (gint argc, gchar **argv)
{
  CtkWidget *window, *toolbar, *grid, *treeview, *scrolled_window;
  CtkWidget *hbox, *hbox1, *hbox2, *checkbox, *option_menu, *menu;
  gint i;
  static const gchar *toolbar_styles[] = { "icons", "text", "both (vertical)",
					   "both (horizontal)" };
  CtkToolItem *item;
  CtkListStore *store;
  CtkWidget *image;
  CtkWidget *menuitem;
  CtkWidget *button;
  CtkWidget *label;
  GIcon *gicon;
  GSList *group;
  
  ctk_init (&argc, &argv);

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);

  g_signal_connect (window, "destroy", G_CALLBACK(ctk_main_quit), NULL);

  grid = ctk_grid_new ();
  ctk_container_add (CTK_CONTAINER (window), grid);

  toolbar = ctk_toolbar_new ();
  ctk_grid_attach (CTK_GRID (grid), toolbar, 0, 0, 2, 1);

  hbox1 = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 3);
  ctk_container_set_border_width (CTK_CONTAINER (hbox1), 5);
  ctk_grid_attach (CTK_GRID (grid), hbox1, 1, 1, 1, 1);

  hbox2 = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 2);
  ctk_container_set_border_width (CTK_CONTAINER (hbox2), 5);
  ctk_grid_attach (CTK_GRID (grid), hbox2, 1, 2, 1, 1);

  checkbox = ctk_check_button_new_with_mnemonic("_Vertical");
  ctk_box_pack_start (CTK_BOX (hbox1), checkbox, FALSE, FALSE, 0);
  g_signal_connect (checkbox, "toggled",
		    G_CALLBACK (change_orientation), toolbar);

  checkbox = ctk_check_button_new_with_mnemonic("_Show Arrow");
  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (checkbox), TRUE);
  ctk_box_pack_start (CTK_BOX (hbox1), checkbox, FALSE, FALSE, 0);
  g_signal_connect (checkbox, "toggled",
		    G_CALLBACK (change_show_arrow), toolbar);

  checkbox = ctk_check_button_new_with_mnemonic("_Set Toolbar Style:");
  g_signal_connect (checkbox, "toggled", G_CALLBACK (set_toolbar_style_toggled), toolbar);
  ctk_box_pack_start (CTK_BOX (hbox1), checkbox, FALSE, FALSE, 0);

  option_menu = ctk_combo_box_text_new ();
  ctk_widget_set_sensitive (option_menu, FALSE);  
  g_object_set_data (G_OBJECT (checkbox), "option-menu", option_menu);
  
  for (i = 0; i < G_N_ELEMENTS (toolbar_styles); i++)
    ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (option_menu), toolbar_styles[i]);
  ctk_combo_box_set_active (CTK_COMBO_BOX (option_menu),
                            ctk_toolbar_get_style (CTK_TOOLBAR (toolbar)));
  ctk_box_pack_start (CTK_BOX (hbox2), option_menu, FALSE, FALSE, 0);
  g_signal_connect (option_menu, "changed",
		    G_CALLBACK (change_toolbar_style), toolbar);

  checkbox = ctk_check_button_new_with_mnemonic("_Set Icon Size:"); 
  g_signal_connect (checkbox, "toggled", G_CALLBACK (set_icon_size_toggled), toolbar);
  ctk_box_pack_start (CTK_BOX (hbox2), checkbox, FALSE, FALSE, 0);

  option_menu = ctk_combo_box_text_new ();
  g_object_set_data (G_OBJECT (checkbox), "option-menu", option_menu);
  ctk_widget_set_sensitive (option_menu, FALSE);
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (option_menu), "small toolbar");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (option_menu), "large toolbar");

  ctk_box_pack_start (CTK_BOX (hbox2), option_menu, FALSE, FALSE, 0);
  g_signal_connect (option_menu, "changed",
		    G_CALLBACK (icon_size_history_changed), toolbar);
  
  scrolled_window = ctk_scrolled_window_new (NULL, NULL);
  ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (scrolled_window),
				  CTK_POLICY_AUTOMATIC, CTK_POLICY_AUTOMATIC);
  ctk_widget_set_hexpand (scrolled_window, TRUE);
  ctk_widget_set_vexpand (scrolled_window, TRUE);
  ctk_grid_attach (CTK_GRID (grid), scrolled_window, 1, 3, 1, 1);

  store = create_items_list (&treeview);
  ctk_container_add (CTK_CONTAINER (scrolled_window), treeview);
  
  item = ctk_tool_button_new (NULL, NULL);
  ctk_tool_button_set_icon_name (CTK_TOOL_BUTTON (item), "document-new");
  ctk_tool_button_set_label (CTK_TOOL_BUTTON (item), "Custom label");
  add_item_to_list (store, item, "New");
  ctk_toolbar_insert (CTK_TOOLBAR (toolbar), item, -1);
  cdk_threads_add_timeout (3000, (GSourceFunc) timeout_cb, item);
  ctk_tool_item_set_expand (item, TRUE);

  menu = ctk_menu_new ();
  for (i = 0; i < 20; i++)
    {
      char *text;
      text = g_strdup_printf ("Menuitem %d", i);
      menuitem = ctk_menu_item_new_with_label (text);
      g_free (text);
      ctk_widget_show (menuitem);
      ctk_menu_shell_append (CTK_MENU_SHELL (menu), menuitem);
    }

  item = ctk_menu_tool_button_new (NULL, NULL);
  ctk_tool_button_set_icon_name (CTK_TOOL_BUTTON (item), "document-open");
  ctk_tool_button_set_label (CTK_TOOL_BUTTON (item), "Open");
  ctk_menu_tool_button_set_menu (CTK_MENU_TOOL_BUTTON (item), menu);
  add_item_to_list (store, item, "Open");
  ctk_toolbar_insert (CTK_TOOLBAR (toolbar), item, -1);
  cdk_threads_add_timeout (3000, (GSourceFunc) timeout_cb1, item);
 
  menu = ctk_menu_new ();
  for (i = 0; i < 20; i++)
    {
      char *text;
      text = g_strdup_printf ("A%d", i);
      menuitem = ctk_menu_item_new_with_label (text);
      g_free (text);
      ctk_widget_show (menuitem);
      ctk_menu_shell_append (CTK_MENU_SHELL (menu), menuitem);
    }

  item = ctk_menu_tool_button_new (NULL, NULL);
  ctk_tool_button_set_icon_name (CTK_TOOL_BUTTON (item), "go-previous");
  ctk_tool_button_set_label (CTK_TOOL_BUTTON (item), "Back");
  ctk_menu_tool_button_set_menu (CTK_MENU_TOOL_BUTTON (item), menu);
  add_item_to_list (store, item, "BackWithHistory");
  ctk_toolbar_insert (CTK_TOOLBAR (toolbar), item, -1);
 
  item = ctk_separator_tool_item_new ();
  add_item_to_list (store, item, "-----");    
  ctk_toolbar_insert (CTK_TOOLBAR (toolbar), item, -1);
  
  image = ctk_image_new_from_icon_name ("dialog-warning", CTK_ICON_SIZE_DIALOG);
  item = ctk_tool_item_new ();
  ctk_widget_show (image);
  ctk_container_add (CTK_CONTAINER (item), image);
  add_item_to_list (store, item, "(Custom Item)");    
  ctk_toolbar_insert (CTK_TOOLBAR (toolbar), item, -1);
  
  item = ctk_tool_button_new (NULL, NULL);
  ctk_tool_button_set_icon_name (CTK_TOOL_BUTTON (item), "go-previous");
  ctk_tool_button_set_label (CTK_TOOL_BUTTON (item), "Back");
  add_item_to_list (store, item, "Back");    
  ctk_toolbar_insert (CTK_TOOLBAR (toolbar), item, -1);

  item = ctk_separator_tool_item_new ();
  add_item_to_list (store, item, "-----");  
  ctk_toolbar_insert (CTK_TOOLBAR (toolbar), item, -1);
  
  item = ctk_tool_button_new (NULL, NULL);
  ctk_tool_button_set_icon_name (CTK_TOOL_BUTTON (item), "go-next");
  ctk_tool_button_set_label (CTK_TOOL_BUTTON (item), "Forward");
  add_item_to_list (store, item, "Forward");  
  ctk_toolbar_insert (CTK_TOOLBAR (toolbar), item, -1);

  item = ctk_toggle_tool_button_new ();
  ctk_tool_button_set_label (CTK_TOOL_BUTTON (item), "Bold");
  ctk_tool_button_set_icon_name (CTK_TOOL_BUTTON (item), "format-text-bold");
  g_signal_connect (item, "toggled", G_CALLBACK (bold_toggled), NULL);
  add_item_to_list (store, item, "Bold");  
  ctk_toolbar_insert (CTK_TOOLBAR (toolbar), item, -1);
  ctk_widget_set_sensitive (CTK_WIDGET (item), FALSE);

  item = ctk_separator_tool_item_new ();
  add_item_to_list (store, item, "-----");  
  ctk_toolbar_insert (CTK_TOOLBAR (toolbar), item, -1);
  ctk_tool_item_set_expand (item, TRUE);
  ctk_separator_tool_item_set_draw (CTK_SEPARATOR_TOOL_ITEM (item), FALSE);
  g_assert (ctk_toolbar_get_nth_item (CTK_TOOLBAR (toolbar), 0) != 0);
  
  item = ctk_radio_tool_button_new (NULL);
  ctk_tool_button_set_label (CTK_TOOL_BUTTON (item), "Left");
  ctk_tool_button_set_icon_name (CTK_TOOL_BUTTON (item), "format-justify-left");
  group = ctk_radio_tool_button_get_group (CTK_RADIO_TOOL_BUTTON (item));
  add_item_to_list (store, item, "Left");
  ctk_toolbar_insert (CTK_TOOLBAR (toolbar), item, -1);
  
  
  item = ctk_radio_tool_button_new (group);
  ctk_tool_button_set_label (CTK_TOOL_BUTTON (item), "Center");
  ctk_tool_button_set_icon_name (CTK_TOOL_BUTTON (item), "format-justify-center");
  group = ctk_radio_tool_button_get_group (CTK_RADIO_TOOL_BUTTON (item));
  add_item_to_list (store, item, "Center");
  ctk_toolbar_insert (CTK_TOOLBAR (toolbar), item, -1);

  item = ctk_radio_tool_button_new (group);
  ctk_tool_button_set_label (CTK_TOOL_BUTTON (item), "Right");
  ctk_tool_button_set_icon_name (CTK_TOOL_BUTTON (item), "format-justify-right");
  add_item_to_list (store, item, "Right");
  ctk_toolbar_insert (CTK_TOOLBAR (toolbar), item, -1);

  item = ctk_tool_button_new (ctk_image_new_from_file ("apple-red.png"), "_Apple");
  add_item_to_list (store, item, "Apple");
  ctk_toolbar_insert (CTK_TOOLBAR (toolbar), item, -1);
  ctk_tool_button_set_use_underline (CTK_TOOL_BUTTON (item), TRUE);

  gicon = g_content_type_get_icon ("video/ogg");
  image = ctk_image_new_from_gicon (gicon, CTK_ICON_SIZE_LARGE_TOOLBAR);
  g_object_unref (gicon);
  item = ctk_tool_button_new (image, "Video");
  add_item_to_list (store, item, "Video");
  ctk_toolbar_insert (CTK_TOOLBAR (toolbar), item, -1);

  image = ctk_image_new_from_icon_name ("utilities-terminal", CTK_ICON_SIZE_LARGE_TOOLBAR);
  item = ctk_tool_button_new (image, "Terminal");
  add_item_to_list (store, item, "Terminal");
  ctk_toolbar_insert (CTK_TOOLBAR (toolbar), item, -1);

  image = ctk_spinner_new ();
  ctk_spinner_start (CTK_SPINNER (image));
  item = ctk_tool_button_new (image, "Spinner");
  add_item_to_list (store, item, "Spinner");
  ctk_toolbar_insert (CTK_TOOLBAR (toolbar), item, -1);

  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 5);
  ctk_container_set_border_width (CTK_CONTAINER (hbox), 5);
  ctk_widget_set_hexpand (hbox, TRUE);
  ctk_grid_attach (CTK_GRID (grid), hbox, 1, 4, 1, 1);

  button = ctk_button_new_with_label ("Drag me to the toolbar");
  ctk_box_pack_start (CTK_BOX (hbox), button, FALSE, FALSE, 0);

  label = ctk_label_new ("Drop index:");
  ctk_box_pack_start (CTK_BOX (hbox), label, FALSE, FALSE, 0);

  label = ctk_label_new ("");
  ctk_box_pack_start (CTK_BOX (hbox), label, FALSE, FALSE, 0);

  checkbox = ctk_check_button_new_with_mnemonic("_Right to left");
  if (ctk_widget_get_default_direction () == CTK_TEXT_DIR_RTL)
    ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (checkbox), TRUE);
  else
    ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (checkbox), FALSE);
  g_signal_connect (checkbox, "toggled", G_CALLBACK (rtl_toggled), NULL);

  ctk_box_pack_end (CTK_BOX (hbox), checkbox, FALSE, FALSE, 0);
  
  ctk_drag_source_set (button, CDK_BUTTON1_MASK,
		       target_table, G_N_ELEMENTS (target_table),
		       CDK_ACTION_MOVE);
  ctk_drag_dest_set (toolbar, CTK_DEST_DEFAULT_DROP,
		     target_table, G_N_ELEMENTS (target_table),
		     CDK_ACTION_MOVE);
  g_signal_connect (toolbar, "drag_motion",
		    G_CALLBACK (toolbar_drag_motion), NULL);
  g_signal_connect (toolbar, "drag_leave",
		    G_CALLBACK (toolbar_drag_leave), NULL);
  g_signal_connect (toolbar, "drag_drop",
		    G_CALLBACK (toolbar_drag_drop), label);

  ctk_widget_show_all (window);

  g_signal_connect (window, "delete_event", G_CALLBACK (ctk_main_quit), NULL);
  
  g_signal_connect (toolbar, "popup_context_menu", G_CALLBACK (popup_context_menu), NULL);
  
  ctk_main ();
  
  return 0;
}
