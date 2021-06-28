/* testmerge.c
 * Copyright (C) 2003 James Henstridge
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

#include <stdio.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#define GDK_DISABLE_DEPRECATION_WARNINGS

#include <ctk/ctk.h>

#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1 
#endif

struct { const gchar *filename; guint merge_id; } merge_ids[] = {
  { "merge-1.ui", 0 },
  { "merge-2.ui", 0 },
  { "merge-3.ui", 0 }
};

static void
dump_tree (CtkWidget    *button, 
	   CtkUIManager *merge)
{
  gchar *dump;

  dump = ctk_ui_manager_get_ui (merge);
  g_message ("%s", dump);
  g_free (dump);
}

static void
dump_accels (void)
{
  ctk_accel_map_save_fd (STDOUT_FILENO);
}

static void
print_toplevel (CtkWidget *widget, gpointer user_data)
{
  g_print ("%s\n", G_OBJECT_TYPE_NAME (widget));
}

static void
dump_toplevels (CtkWidget    *button, 
		CtkUIManager *merge)
{
  GSList *toplevels;

  toplevels = ctk_ui_manager_get_toplevels (merge, 
					    CTK_UI_MANAGER_MENUBAR |
					    CTK_UI_MANAGER_TOOLBAR |
					    CTK_UI_MANAGER_POPUP);

  g_slist_foreach (toplevels, (GFunc) print_toplevel, NULL);
  g_slist_free (toplevels);
}

static void
toggle_tearoffs (CtkWidget    *button, 
		 CtkUIManager *merge)
{
  gboolean add_tearoffs;

  add_tearoffs = ctk_ui_manager_get_add_tearoffs (merge);
  
  ctk_ui_manager_set_add_tearoffs (merge, !add_tearoffs);
}

static gint
delayed_toggle_dynamic (CtkUIManager *merge)
{
  CtkAction *dyn;
  static CtkActionGroup *dynamic = NULL;
  static guint merge_id = 0;

  if (!dynamic)
    {
      dynamic = ctk_action_group_new ("dynamic");
      ctk_ui_manager_insert_action_group (merge, dynamic, 0);
      dyn = g_object_new (CTK_TYPE_ACTION,
			  "name", "dyn1",
			  "label", "Dynamic action 1",
			  "stock_id", CTK_STOCK_COPY,
			  NULL);
      ctk_action_group_add_action (dynamic, dyn);
      dyn = g_object_new (CTK_TYPE_ACTION,
			  "name", "dyn2",
			  "label", "Dynamic action 2",
			  "stock_id", CTK_STOCK_EXECUTE,
			  NULL);
      ctk_action_group_add_action (dynamic, dyn);
    }
  
  if (merge_id == 0)
    {
      merge_id = ctk_ui_manager_new_merge_id (merge);
      ctk_ui_manager_add_ui (merge, merge_id, "/toolbar1/ToolbarPlaceholder", 
			     "dyn1", "dyn1", 0, 0);
      ctk_ui_manager_add_ui (merge, merge_id, "/toolbar1/ToolbarPlaceholder", 
			     "dynsep", NULL, CTK_UI_MANAGER_SEPARATOR, 0);
      ctk_ui_manager_add_ui (merge, merge_id, "/toolbar1/ToolbarPlaceholder", 
			     "dyn2", "dyn2", 0, 0);

      ctk_ui_manager_add_ui (merge, merge_id, "/menubar/EditMenu", 
			     "dyn1menu", "dyn1", CTK_UI_MANAGER_MENU, 0);
      ctk_ui_manager_add_ui (merge, merge_id, "/menubar/EditMenu/dyn1menu", 
			     "dyn1", "dyn1", CTK_UI_MANAGER_MENUITEM, 0);
      ctk_ui_manager_add_ui (merge, merge_id, "/menubar/EditMenu/dyn1menu/dyn1", 
			     "dyn2", "dyn2", CTK_UI_MANAGER_AUTO, FALSE);
    }
  else 
    {
      ctk_ui_manager_remove_ui (merge, merge_id);
      merge_id = 0;
    }

  return FALSE;
}

static void
toggle_dynamic (CtkWidget    *button, 
		CtkUIManager *merge)
{
  cdk_threads_add_timeout (2000, (GSourceFunc)delayed_toggle_dynamic, merge);
}

static void
activate_action (CtkAction *action)
{
  const gchar *name = ctk_action_get_name (action);
  const gchar *typename = G_OBJECT_TYPE_NAME (action);

  g_message ("Action %s (type=%s) activated", name, typename);
}

static void
toggle_action (CtkAction *action)
{
  const gchar *name = ctk_action_get_name (action);
  const gchar *typename = G_OBJECT_TYPE_NAME (action);

  g_message ("ToggleAction %s (type=%s) toggled (active=%d)", name, typename,
	     ctk_toggle_action_get_active (CTK_TOGGLE_ACTION (action)));
}


static void
radio_action_changed (CtkAction *action, CtkRadioAction *current)
{
  g_message ("RadioAction %s (type=%s) activated (active=%d) (value %d)", 
	     ctk_action_get_name (CTK_ACTION (current)), 
	     G_OBJECT_TYPE_NAME (CTK_ACTION (current)),
	     ctk_toggle_action_get_active (CTK_TOGGLE_ACTION (current)),
	     ctk_radio_action_get_current_value (current));
}

static CtkActionEntry entries[] = {
  { "FileMenuAction", NULL, "_File" },
  { "EditMenuAction", NULL, "_Edit" },
  { "HelpMenuAction", NULL, "_Help" },
  { "JustifyMenuAction", NULL, "_Justify" },
  { "EmptyMenu1Action", NULL, "Empty 1" },
  { "EmptyMenu2Action", NULL, "Empty 2" },
  { "Test", NULL, "Test" },

  { "QuitAction",  CTK_STOCK_QUIT,  NULL,     "<control>q", "Quit", G_CALLBACK (ctk_main_quit) },
  { "NewAction",   CTK_STOCK_NEW,   NULL,     "<control>n", "Create something", G_CALLBACK (activate_action) },
  { "New2Action",  CTK_STOCK_NEW,   NULL,     "<control>m", "Create something else", G_CALLBACK (activate_action) },
  { "OpenAction",  CTK_STOCK_OPEN,  NULL,     NULL,         "Open it", G_CALLBACK (activate_action) },
  { "CutAction",   CTK_STOCK_CUT,   NULL,     "<control>x", "Knive", G_CALLBACK (activate_action) },
  { "CopyAction",  CTK_STOCK_COPY,  NULL,     "<control>c", "Copy", G_CALLBACK (activate_action) },
  { "PasteAction", CTK_STOCK_PASTE, NULL,     "<control>v", "Paste", G_CALLBACK (activate_action) },
  { "AboutAction", NULL,            "_About", NULL,         "About", G_CALLBACK (activate_action) },
};
static guint n_entries = G_N_ELEMENTS (entries);

static CtkToggleActionEntry toggle_entries[] = {
  { "BoldAction",  CTK_STOCK_BOLD,  "_Bold",  "<control>b", "Make it bold", G_CALLBACK (toggle_action), 
    TRUE },
};
static guint n_toggle_entries = G_N_ELEMENTS (toggle_entries);

enum {
  JUSTIFY_LEFT,
  JUSTIFY_CENTER,
  JUSTIFY_RIGHT,
  JUSTIFY_FILL
};

static CtkRadioActionEntry radio_entries[] = {
  { "justify-left", CTK_STOCK_JUSTIFY_LEFT, NULL, "<control>L", 
    "Left justify the text", JUSTIFY_LEFT },
  { "justify-center", CTK_STOCK_JUSTIFY_CENTER, NULL, "<super>E",
    "Center justify the text", JUSTIFY_CENTER },
  { "justify-right", CTK_STOCK_JUSTIFY_RIGHT, NULL, "<hyper>R",
    "Right justify the text", JUSTIFY_RIGHT },
  { "justify-fill", CTK_STOCK_JUSTIFY_FILL, NULL, "<super><hyper>J",
    "Fill justify the text", JUSTIFY_FILL },
};
static guint n_radio_entries = G_N_ELEMENTS (radio_entries);

static void
add_widget (CtkUIManager *merge, 
	    CtkWidget    *widget, 
	    CtkBox       *box)
{
  ctk_box_pack_start (box, widget, FALSE, FALSE, 0);
  ctk_widget_show (widget);
}

static void
toggle_merge (CtkWidget    *button, 
	      CtkUIManager *merge)
{
  gint mergenum;

  mergenum = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button), "mergenum"));

  if (ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (button)))
    {
      GError *err = NULL;

      g_message ("merging %s", merge_ids[mergenum].filename);
      merge_ids[mergenum].merge_id =
	ctk_ui_manager_add_ui_from_file (merge, merge_ids[mergenum].filename, &err);
      if (err != NULL)
	{
	  CtkWidget *dialog;

	  dialog = ctk_message_dialog_new (CTK_WINDOW (ctk_widget_get_toplevel (button)),
					   0, CTK_MESSAGE_WARNING, CTK_BUTTONS_OK,
					   "could not merge %s: %s", merge_ids[mergenum].filename,
					   err->message);

	  g_signal_connect (dialog, "response", G_CALLBACK (ctk_widget_destroy), NULL);
	  ctk_widget_show (dialog);

	  g_clear_error (&err);
	}
    }
  else
    {
      g_message ("unmerging %s (merge_id=%u)", merge_ids[mergenum].filename,
		 merge_ids[mergenum].merge_id);
      ctk_ui_manager_remove_ui (merge, merge_ids[mergenum].merge_id);
    }
}

static void  
set_name_func (CtkTreeViewColumn *tree_column,
	       CtkCellRenderer   *cell,
	       CtkTreeModel      *tree_model,
	       CtkTreeIter       *iter,
	       gpointer           data)
{
  CtkAction *action;
  char *name;
  
  ctk_tree_model_get (tree_model, iter, 0, &action, -1);
  g_object_get (action, "name", &name, NULL);
  g_object_set (cell, "text", name, NULL);
  g_free (name);
  g_object_unref (action);
}

static void
set_sensitive_func (CtkTreeViewColumn *tree_column,
		    CtkCellRenderer   *cell,
		    CtkTreeModel      *tree_model,
		    CtkTreeIter       *iter,
		    gpointer           data)
{
  CtkAction *action;
  gboolean sensitive;
  
  ctk_tree_model_get (tree_model, iter, 0, &action, -1);
  g_object_get (action, "sensitive", &sensitive, NULL);
  g_object_set (cell, "active", sensitive, NULL);
  g_object_unref (action);
}


static void
set_visible_func (CtkTreeViewColumn *tree_column,
		  CtkCellRenderer   *cell,
		  CtkTreeModel      *tree_model,
		  CtkTreeIter       *iter,
		  gpointer           data)
{
  CtkAction *action;
  gboolean visible;
  
  ctk_tree_model_get (tree_model, iter, 0, &action, -1);
  g_object_get (action, "visible", &visible, NULL);
  g_object_set (cell, "active", visible, NULL);
  g_object_unref (action);
}

static void
sensitivity_toggled (CtkCellRendererToggle *cell, 
		     const gchar           *path_str,
		     CtkTreeModel          *model)
{
  CtkTreePath *path;
  CtkTreeIter iter;
  CtkAction *action;
  gboolean sensitive;

  path = ctk_tree_path_new_from_string (path_str);
  ctk_tree_model_get_iter (model, &iter, path);

  ctk_tree_model_get (model, &iter, 0, &action, -1);
  g_object_get (action, "sensitive", &sensitive, NULL);
  g_object_set (action, "sensitive", !sensitive, NULL);
  ctk_tree_model_row_changed (model, path, &iter);
  ctk_tree_path_free (path);
}

static void
visibility_toggled (CtkCellRendererToggle *cell, 
		    const gchar           *path_str, 
		    CtkTreeModel          *model)
{
  CtkTreePath *path;
  CtkTreeIter iter;
  CtkAction *action;
  gboolean visible;

  path = ctk_tree_path_new_from_string (path_str);
  ctk_tree_model_get_iter (model, &iter, path);

  ctk_tree_model_get (model, &iter, 0, &action, -1);
  g_object_get (action, "visible", &visible, NULL);
  g_object_set (action, "visible", !visible, NULL);
  ctk_tree_model_row_changed (model, path, &iter);
  ctk_tree_path_free (path);
}

static gint
iter_compare_func (CtkTreeModel *model, 
		   CtkTreeIter  *a, 
		   CtkTreeIter  *b,
		   gpointer      user_data)
{
  GValue a_value = G_VALUE_INIT, b_value = G_VALUE_INIT;
  CtkAction *a_action, *b_action;
  const gchar *a_name, *b_name;
  gint retval = 0;

  ctk_tree_model_get_value (model, a, 0, &a_value);
  ctk_tree_model_get_value (model, b, 0, &b_value);
  a_action = CTK_ACTION (g_value_get_object (&a_value));
  b_action = CTK_ACTION (g_value_get_object (&b_value));

  a_name = ctk_action_get_name (a_action);
  b_name = ctk_action_get_name (b_action);
  if (a_name == NULL && b_name == NULL) 
    retval = 0;
  else if (a_name == NULL)
    retval = -1;
  else if (b_name == NULL) 
    retval = 1;
  else 
    retval = strcmp (a_name, b_name);

  g_value_unset (&b_value);
  g_value_unset (&a_value);

  return retval;
}

static CtkWidget *
create_tree_view (CtkUIManager *merge)
{
  CtkWidget *tree_view, *sw;
  CtkListStore *store;
  GList *p;
  CtkCellRenderer *cell;
  
  store = ctk_list_store_new (1, CTK_TYPE_ACTION);
  ctk_tree_sortable_set_sort_func (CTK_TREE_SORTABLE (store), 0,
				   iter_compare_func, NULL, NULL);
  ctk_tree_sortable_set_sort_column_id (CTK_TREE_SORTABLE (store), 0,
					CTK_SORT_ASCENDING);
  
  for (p = ctk_ui_manager_get_action_groups (merge); p; p = p->next)
    {
      GList *actions, *l;

      actions = ctk_action_group_list_actions (p->data);

      for (l = actions; l; l = l->next)
	{
	  CtkTreeIter iter;

	  ctk_list_store_append (store, &iter);
	  ctk_list_store_set (store, &iter, 0, l->data, -1);
	}

      g_list_free (actions);
    }
  
  tree_view = ctk_tree_view_new_with_model (CTK_TREE_MODEL (store));
  g_object_unref (store);

  ctk_tree_view_insert_column_with_data_func (CTK_TREE_VIEW (tree_view),
					      -1, "Action",
					      ctk_cell_renderer_text_new (),
					      set_name_func, NULL, NULL);

  ctk_tree_view_column_set_sort_column_id (ctk_tree_view_get_column (CTK_TREE_VIEW (tree_view), 0), 0);

  cell = ctk_cell_renderer_toggle_new ();
  g_signal_connect (cell, "toggled", G_CALLBACK (sensitivity_toggled), store);
  ctk_tree_view_insert_column_with_data_func (CTK_TREE_VIEW (tree_view),
					      -1, "Sensitive",
					      cell,
					      set_sensitive_func, NULL, NULL);

  cell = ctk_cell_renderer_toggle_new ();
  g_signal_connect (cell, "toggled", G_CALLBACK (visibility_toggled), store);
  ctk_tree_view_insert_column_with_data_func (CTK_TREE_VIEW (tree_view),
					      -1, "Visible",
					      cell,
					      set_visible_func, NULL, NULL);

  sw = ctk_scrolled_window_new (NULL, NULL);
  ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (sw),
				  CTK_POLICY_NEVER, CTK_POLICY_AUTOMATIC);
  ctk_container_add (CTK_CONTAINER (sw), tree_view);
  
  return sw;
}

static gboolean
area_press (CtkWidget      *drawing_area,
	    GdkEventButton *event,
	    CtkUIManager   *merge)
{
  ctk_widget_grab_focus (drawing_area);

  if (cdk_event_triggers_context_menu ((GdkEvent *) event) &&
      event->type == GDK_BUTTON_PRESS)
    {
      CtkWidget *menu = ctk_ui_manager_get_widget (merge, "/FileMenu");
      
      if (CTK_IS_MENU (menu)) 
	{
	  ctk_menu_popup (CTK_MENU (menu), NULL, NULL,
			  NULL, drawing_area,
			  3, event->time);
	  return TRUE;
	}
    }

  return FALSE;
  
}

static void
activate_path (CtkWidget      *button,
	       CtkUIManager   *merge)
{
  CtkAction *action = ctk_ui_manager_get_action (merge, 
						 "/menubar/HelpMenu/About");
  if (action)
    ctk_action_activate (action);
  else 
    g_message ("no action found");
}

typedef struct _ActionStatus ActionStatus;

struct _ActionStatus {
  CtkAction *action;
  CtkWidget *statusbar;
};

static void
action_status_destroy (gpointer data)
{
  ActionStatus *action_status = data;

  g_object_unref (action_status->action);
  g_object_unref (action_status->statusbar);

  g_free (action_status);
}

static void
set_tip (CtkWidget *widget)
{
  ActionStatus *data;
  gchar *tooltip;
  
  data = g_object_get_data (G_OBJECT (widget), "action-status");
  
  if (data) 
    {
      g_object_get (data->action, "tooltip", &tooltip, NULL);
      
      ctk_statusbar_push (CTK_STATUSBAR (data->statusbar), 0, 
			  tooltip ? tooltip : "");
      
      g_free (tooltip);
    }
}

static void
unset_tip (CtkWidget *widget)
{
  ActionStatus *data;

  data = g_object_get_data (G_OBJECT (widget), "action-status");

  if (data)
    ctk_statusbar_pop (CTK_STATUSBAR (data->statusbar), 0);
}
		    
static void
connect_proxy (CtkUIManager *merge,
	       CtkAction    *action,
	       CtkWidget    *proxy,
	       CtkWidget    *statusbar)
{
  if (CTK_IS_MENU_ITEM (proxy)) 
    {
      ActionStatus *data;

      data = g_object_get_data (G_OBJECT (proxy), "action-status");
      if (data)
	{
	  g_object_unref (data->action);
	  g_object_unref (data->statusbar);

	  data->action = g_object_ref (action);
	  data->statusbar = g_object_ref (statusbar);
	}
      else
	{
	  data = g_new0 (ActionStatus, 1);

	  data->action = g_object_ref (action);
	  data->statusbar = g_object_ref (statusbar);

	  g_object_set_data_full (G_OBJECT (proxy), "action-status", 
				  data, action_status_destroy);
	  
	  g_signal_connect (proxy, "select",  G_CALLBACK (set_tip), NULL);
	  g_signal_connect (proxy, "deselect", G_CALLBACK (unset_tip), NULL);
	}
    }
}

int
main (int argc, char **argv)
{
  CtkActionGroup *action_group;
  CtkAction *action;
  CtkUIManager *merge;
  CtkWidget *window, *grid, *frame, *menu_box, *vbox, *view;
  CtkWidget *button, *area, *statusbar;
  CtkWidget *box;
  gint i;
  
  ctk_init (&argc, &argv);

  action_group = ctk_action_group_new ("TestActions");
  ctk_action_group_add_actions (action_group, 
				entries, n_entries, 
				NULL);
  action = ctk_action_group_get_action (action_group, "EmptyMenu1Action");
  g_object_set (action, "hide_if_empty", FALSE, NULL);
  action = ctk_action_group_get_action (action_group, "EmptyMenu2Action");
  g_object_set (action, "hide_if_empty", TRUE, NULL);
  ctk_action_group_add_toggle_actions (action_group, 
				       toggle_entries, n_toggle_entries, 
				       NULL);
  ctk_action_group_add_radio_actions (action_group, 
				      radio_entries, n_radio_entries, 
				      JUSTIFY_RIGHT,
				      G_CALLBACK (radio_action_changed), NULL);

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_default_size (CTK_WINDOW (window), -1, 400);
  g_signal_connect (window, "destroy", G_CALLBACK (ctk_main_quit), NULL);

  grid = ctk_grid_new ();
  ctk_grid_set_row_spacing (CTK_GRID (grid), 2);
  ctk_grid_set_column_spacing (CTK_GRID (grid), 2);
  ctk_container_set_border_width (CTK_CONTAINER (grid), 2);
  ctk_container_add (CTK_CONTAINER (window), grid);

  frame = ctk_frame_new ("Menus and Toolbars");
  ctk_grid_attach (CTK_GRID (grid), frame, 0, 1, 2, 1);
  
  menu_box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  ctk_container_set_border_width (CTK_CONTAINER (menu_box), 2);
  ctk_container_add (CTK_CONTAINER (frame), menu_box);

  statusbar = ctk_statusbar_new ();
  ctk_box_pack_end (CTK_BOX (menu_box), statusbar, FALSE, FALSE, 0);
    
  area = ctk_drawing_area_new ();
  ctk_widget_set_events (area, GDK_BUTTON_PRESS_MASK);
  ctk_widget_set_size_request (area, -1, 40);
  ctk_box_pack_end (CTK_BOX (menu_box), area, FALSE, FALSE, 0);
  ctk_widget_show (area);

  button = ctk_button_new ();
  ctk_box_pack_end (CTK_BOX (menu_box), button, FALSE, FALSE, 0);
  ctk_activatable_set_related_action (CTK_ACTIVATABLE (button),
			    ctk_action_group_get_action (action_group, "AboutAction"));

  ctk_widget_show (button);

  button = ctk_check_button_new ();
  ctk_box_pack_end (CTK_BOX (menu_box), button, FALSE, FALSE, 0);
  ctk_activatable_set_related_action (CTK_ACTIVATABLE (button),
			    ctk_action_group_get_action (action_group, "BoldAction"));
  ctk_widget_show (button);

  box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 6);
  ctk_box_pack_end (CTK_BOX (menu_box), box, FALSE, FALSE, 0);
  ctk_container_add (CTK_CONTAINER (box), ctk_label_new ("Bold:"));
  button = ctk_switch_new ();
  ctk_container_add (CTK_CONTAINER (box), button);
  ctk_activatable_set_related_action (CTK_ACTIVATABLE (button),
                            ctk_action_group_get_action (action_group, "BoldAction"));
  ctk_widget_show_all (box);

  merge = ctk_ui_manager_new ();

  g_signal_connect (merge, "connect-proxy", G_CALLBACK (connect_proxy), statusbar);
  g_signal_connect (area, "button_press_event", G_CALLBACK (area_press), merge);

  ctk_ui_manager_insert_action_group (merge, action_group, 0);
  g_signal_connect (merge, "add_widget", G_CALLBACK (add_widget), menu_box);

  ctk_window_add_accel_group (CTK_WINDOW (window), 
			      ctk_ui_manager_get_accel_group (merge));
  
  frame = ctk_frame_new ("UI Files");
  ctk_widget_set_vexpand (frame, TRUE);
  ctk_grid_attach (CTK_GRID (grid), frame, 0, 0, 1, 1);

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 2);
  ctk_container_set_border_width (CTK_CONTAINER (vbox), 2);
  ctk_container_add (CTK_CONTAINER (frame), vbox);

  for (i = 0; i < G_N_ELEMENTS (merge_ids); i++)
    {
      button = ctk_check_button_new_with_label (merge_ids[i].filename);
      g_object_set_data (G_OBJECT (button), "mergenum", GINT_TO_POINTER (i));
      g_signal_connect (button, "toggled", G_CALLBACK (toggle_merge), merge);
      ctk_box_pack_start (CTK_BOX (vbox), button, FALSE, FALSE, 0);
      ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (button), TRUE);
    }

  button = ctk_check_button_new_with_label ("Tearoffs");
  g_signal_connect (button, "clicked", G_CALLBACK (toggle_tearoffs), merge);
  ctk_box_pack_end (CTK_BOX (vbox), button, FALSE, FALSE, 0);

  button = ctk_check_button_new_with_label ("Dynamic");
  g_signal_connect (button, "clicked", G_CALLBACK (toggle_dynamic), merge);
  ctk_box_pack_end (CTK_BOX (vbox), button, FALSE, FALSE, 0);

  button = ctk_button_new_with_label ("Activate path");
  g_signal_connect (button, "clicked", G_CALLBACK (activate_path), merge);
  ctk_box_pack_end (CTK_BOX (vbox), button, FALSE, FALSE, 0);

  button = ctk_button_new_with_label ("Dump Tree");
  g_signal_connect (button, "clicked", G_CALLBACK (dump_tree), merge);
  ctk_box_pack_end (CTK_BOX (vbox), button, FALSE, FALSE, 0);

  button = ctk_button_new_with_label ("Dump Toplevels");
  g_signal_connect (button, "clicked", G_CALLBACK (dump_toplevels), merge);
  ctk_box_pack_end (CTK_BOX (vbox), button, FALSE, FALSE, 0);

  button = ctk_button_new_with_label ("Dump Accels");
  g_signal_connect (button, "clicked", G_CALLBACK (dump_accels), NULL);
  ctk_box_pack_end (CTK_BOX (vbox), button, FALSE, FALSE, 0);

  view = create_tree_view (merge);
  ctk_widget_set_hexpand (view, TRUE);
  ctk_widget_set_vexpand (view, TRUE);
  ctk_grid_attach (CTK_GRID (grid), view, 1, 0, 1, 1);

  ctk_widget_show_all (window);
  ctk_main ();

#ifdef DEBUG_UI_MANAGER
  {
    GList *action;
    
    g_print ("\n> before unreffing the ui manager <\n");
    for (action = ctk_action_group_list_actions (action_group);
	 action; 
	 action = action->next)
      {
	CtkAction *a = action->data;
	g_print ("  action %s ref count %d\n", 
		 ctk_action_get_name (a), G_OBJECT (a)->ref_count);
      }
  }
#endif

  g_object_unref (merge);

#ifdef DEBUG_UI_MANAGER
  {
    GList *action;

    g_print ("\n> after unreffing the ui manager <\n");
    for (action = ctk_action_group_list_actions (action_group);
	 action; 
	 action = action->next)
      {
	CtkAction *a = action->data;
	g_print ("  action %s ref count %d\n", 
		 ctk_action_get_name (a), G_OBJECT (a)->ref_count);
      }
  }
#endif

  g_object_unref (action_group);

  return 0;
}
