/* testtreeedit.c
 * Copyright (C) 2001 Red Hat, Inc
 * Author: Jonathan Blandford
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

typedef struct {
  const gchar *string;
  gboolean is_editable;
  gboolean is_sensitive;
  gint progress;
} ListEntry;

enum {
  STRING_COLUMN,
  IS_EDITABLE_COLUMN,
  IS_SENSITIVE_COLUMN,
  ICON_NAME_COLUMN,
  LAST_ICON_NAME_COLUMN,
  PROGRESS_COLUMN,
  NUM_COLUMNS
};

static ListEntry model_strings[] =
{
  {"A simple string", TRUE, TRUE, 0 },
  {"Another string!", TRUE, TRUE, 10 },
  {"", TRUE, TRUE, 0 },
  {"Guess what, a third string. This one can't be edited", FALSE, TRUE, 47 },
  {"And then a fourth string. Neither can this", FALSE, TRUE, 48 },
  {"Multiline\nFun!", TRUE, FALSE, 75 },
  { NULL }
};

static CtkTreeModel *
create_model (void)
{
  CtkTreeStore *model;
  CtkTreeIter iter;
  gint i;

  model = ctk_tree_store_new (NUM_COLUMNS,
			      G_TYPE_STRING,
			      G_TYPE_BOOLEAN,
			      G_TYPE_BOOLEAN,
			      G_TYPE_STRING,
			      G_TYPE_STRING,
			      G_TYPE_INT);

  for (i = 0; model_strings[i].string != NULL; i++)
    {
      ctk_tree_store_append (model, &iter, NULL);

      ctk_tree_store_set (model, &iter,
			  STRING_COLUMN, model_strings[i].string,
			  IS_EDITABLE_COLUMN, model_strings[i].is_editable,
			  IS_SENSITIVE_COLUMN, model_strings[i].is_sensitive,
			  ICON_NAME_COLUMN, "document-new",
			  LAST_ICON_NAME_COLUMN, "edit-delete",
			  PROGRESS_COLUMN, model_strings[i].progress,
			  -1);
    }
  
  return CTK_TREE_MODEL (model);
}

static void
editable_toggled (CtkCellRendererToggle *cell G_GNUC_UNUSED,
		  gchar                 *path_string,
		  gpointer               data)
{
  CtkTreeModel *model = CTK_TREE_MODEL (data);
  CtkTreeIter iter;
  CtkTreePath *path = ctk_tree_path_new_from_string (path_string);
  gboolean value;

  ctk_tree_model_get_iter (model, &iter, path);
  ctk_tree_model_get (model, &iter, IS_EDITABLE_COLUMN, &value, -1);

  value = !value;
  ctk_tree_store_set (CTK_TREE_STORE (model), &iter, IS_EDITABLE_COLUMN, value, -1);

  ctk_tree_path_free (path);
}

static void
sensitive_toggled (CtkCellRendererToggle *cell G_GNUC_UNUSED,
		   gchar                 *path_string,
		   gpointer               data)
{
  CtkTreeModel *model = CTK_TREE_MODEL (data);
  CtkTreeIter iter;
  CtkTreePath *path = ctk_tree_path_new_from_string (path_string);
  gboolean value;

  ctk_tree_model_get_iter (model, &iter, path);
  ctk_tree_model_get (model, &iter, IS_SENSITIVE_COLUMN, &value, -1);

  value = !value;
  ctk_tree_store_set (CTK_TREE_STORE (model), &iter, IS_SENSITIVE_COLUMN, value, -1);

  ctk_tree_path_free (path);
}

static void
edited (CtkCellRendererText *cell G_GNUC_UNUSED,
	gchar               *path_string,
	gchar               *new_text,
	gpointer             data)
{
  CtkTreeModel *model = CTK_TREE_MODEL (data);
  CtkTreeIter iter;
  CtkTreePath *path = ctk_tree_path_new_from_string (path_string);

  ctk_tree_model_get_iter (model, &iter, path);
  ctk_tree_store_set (CTK_TREE_STORE (model), &iter, STRING_COLUMN, new_text, -1);

  ctk_tree_path_free (path);
}

static gboolean
button_press_event (CtkWidget *widget, CdkEventButton *event, gpointer callback_data G_GNUC_UNUSED)
{
	/* Deselect if people click outside any row. */
	if (event->window == ctk_tree_view_get_bin_window (CTK_TREE_VIEW (widget))
	    && !ctk_tree_view_get_path_at_pos (CTK_TREE_VIEW (widget),
					       event->x, event->y, NULL, NULL, NULL, NULL)) {
		ctk_tree_selection_unselect_all (ctk_tree_view_get_selection (CTK_TREE_VIEW (widget)));
	}

	/* Let the default code run in any case; it won't reselect anything. */
	return FALSE;
}

typedef struct {
  CtkCellArea     *area;
  CtkCellRenderer *renderer;
} CallbackData;

static void
align_cell_toggled (CtkToggleButton  *toggle,
		    CallbackData     *data)
{
  gboolean active = ctk_toggle_button_get_active (toggle);

  ctk_cell_area_cell_set (data->area, data->renderer, "align", active, NULL);
}

static void
expand_cell_toggled (CtkToggleButton  *toggle,
		     CallbackData     *data)
{
  gboolean active = ctk_toggle_button_get_active (toggle);

  ctk_cell_area_cell_set (data->area, data->renderer, "expand", active, NULL);
}

static void
fixed_cell_toggled (CtkToggleButton  *toggle,
		    CallbackData     *data)
{
  gboolean active = ctk_toggle_button_get_active (toggle);

  ctk_cell_area_cell_set (data->area, data->renderer, "fixed-size", active, NULL);
}

enum {
  CNTL_EXPAND,
  CNTL_ALIGN,
  CNTL_FIXED
};

static void
create_control (CtkWidget *box, gint number, gint cntl, CallbackData *data)
{
  CtkWidget *checkbutton;
  GCallback  callback = NULL;
  gchar *name = NULL;

  switch (cntl)
    {
    case CNTL_EXPAND: 
      name = g_strdup_printf ("Expand Cell #%d", number); 
      callback = G_CALLBACK (expand_cell_toggled);
      break;
    case CNTL_ALIGN: 
      name = g_strdup_printf ("Align Cell #%d", number); 
      callback = G_CALLBACK (align_cell_toggled);
      break;
    case CNTL_FIXED: 
      name = g_strdup_printf ("Fix size Cell #%d", number); 
      callback = G_CALLBACK (fixed_cell_toggled);
      break;
    }

  checkbutton = ctk_check_button_new_with_label (name);
  ctk_widget_show (checkbutton);
  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (checkbutton), cntl == CNTL_FIXED);
  ctk_box_pack_start (CTK_BOX (box), checkbutton, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (checkbutton), "toggled", callback, data);
  g_free (name);
}

gint
main (gint argc, gchar **argv)
{
  CtkWidget *window;
  CtkWidget *scrolled_window;
  CtkWidget *tree_view;
  CtkWidget *vbox, *hbox, *cntl_vbox;
  CtkTreeModel *tree_model;
  CtkCellRenderer *renderer;
  CtkTreeViewColumn *column;
  CtkCellArea *area;
  CallbackData callback[4];
  
  ctk_init (&argc, &argv);

  if (g_getenv ("RTL"))
    ctk_widget_set_default_direction (CTK_TEXT_DIR_RTL);

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_title (CTK_WINDOW (window), "CtkTreeView editing sample");
  g_signal_connect (window, "destroy", ctk_main_quit, NULL);

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 6);
  ctk_widget_show (vbox);
  ctk_container_add (CTK_CONTAINER (window), vbox);

  scrolled_window = ctk_scrolled_window_new (NULL, NULL);
  ctk_scrolled_window_set_shadow_type (CTK_SCROLLED_WINDOW (scrolled_window), CTK_SHADOW_ETCHED_IN);
  ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (scrolled_window), 
				  CTK_POLICY_AUTOMATIC, CTK_POLICY_AUTOMATIC);
  ctk_box_pack_start (CTK_BOX (vbox), scrolled_window, TRUE, TRUE, 0);

  tree_model = create_model ();
  tree_view = ctk_tree_view_new_with_model (tree_model);
  g_signal_connect (tree_view, "button_press_event", G_CALLBACK (button_press_event), NULL);
  ctk_tree_view_set_headers_visible (CTK_TREE_VIEW (tree_view), TRUE);

  column = ctk_tree_view_column_new ();
  ctk_tree_view_column_set_title (column, "String");
  area = ctk_cell_layout_get_area (CTK_CELL_LAYOUT (column));

  renderer = ctk_cell_renderer_pixbuf_new ();
  ctk_tree_view_column_pack_start (column, renderer, FALSE);
  ctk_tree_view_column_set_attributes (column, renderer,
				       "icon-name", ICON_NAME_COLUMN, 
				       "sensitive", IS_SENSITIVE_COLUMN,
				       NULL);
  callback[0].area = area;
  callback[0].renderer = renderer;

  renderer = ctk_cell_renderer_text_new ();
  ctk_tree_view_column_pack_start (column, renderer, FALSE);
  ctk_tree_view_column_set_attributes (column, renderer,
				       "text", STRING_COLUMN,
				       "editable", IS_EDITABLE_COLUMN,
				       "sensitive", IS_SENSITIVE_COLUMN,
				       NULL);
  callback[1].area = area;
  callback[1].renderer = renderer;
  g_signal_connect (renderer, "edited",
		    G_CALLBACK (edited), tree_model);
  g_object_set (renderer,
                "placeholder-text", "Type here",
                NULL);

  renderer = ctk_cell_renderer_text_new ();
  ctk_tree_view_column_pack_start (column, renderer, FALSE);
  ctk_tree_view_column_set_attributes (column, renderer,
		  		       "text", STRING_COLUMN,
				       "editable", IS_EDITABLE_COLUMN,
				       "sensitive", IS_SENSITIVE_COLUMN,
				       NULL);
  callback[2].area = area;
  callback[2].renderer = renderer;
  g_signal_connect (renderer, "edited",
		    G_CALLBACK (edited), tree_model);
  g_object_set (renderer,
                "placeholder-text", "Type here too",
                NULL);

  renderer = ctk_cell_renderer_pixbuf_new ();
  g_object_set (renderer,
		"xalign", 0.0,
		NULL);
  ctk_tree_view_column_pack_start (column, renderer, FALSE);
  ctk_tree_view_column_set_attributes (column, renderer,
				       "icon-name", LAST_ICON_NAME_COLUMN, 
				       "sensitive", IS_SENSITIVE_COLUMN,
				       NULL);
  callback[3].area = area;
  callback[3].renderer = renderer;

  ctk_tree_view_append_column (CTK_TREE_VIEW (tree_view), column);

  renderer = ctk_cell_renderer_toggle_new ();
  g_signal_connect (renderer, "toggled",
		    G_CALLBACK (editable_toggled), tree_model);
  
  g_object_set (renderer,
		"xalign", 0.0,
		NULL);
  ctk_tree_view_insert_column_with_attributes (CTK_TREE_VIEW (tree_view),
					       -1, "Editable",
					       renderer,
					       "active", IS_EDITABLE_COLUMN,
					       NULL);

  renderer = ctk_cell_renderer_toggle_new ();
  g_signal_connect (renderer, "toggled",
		    G_CALLBACK (sensitive_toggled), tree_model);
  
  g_object_set (renderer,
		"xalign", 0.0,
		NULL);
  ctk_tree_view_insert_column_with_attributes (CTK_TREE_VIEW (tree_view),
					       -1, "Sensitive",
					       renderer,
					       "active", IS_SENSITIVE_COLUMN,
					       NULL);

  renderer = ctk_cell_renderer_progress_new ();
  ctk_tree_view_insert_column_with_attributes (CTK_TREE_VIEW (tree_view),
					       -1, "Progress",
					       renderer,
					       "value", PROGRESS_COLUMN,
					       NULL);

  ctk_container_add (CTK_CONTAINER (scrolled_window), tree_view);
  
  ctk_window_set_default_size (CTK_WINDOW (window),
			       800, 250);

  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 6);
  ctk_widget_show (hbox);
  ctk_box_pack_start (CTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  /* Alignment controls */
  cntl_vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 2);
  ctk_widget_show (cntl_vbox);
  ctk_box_pack_start (CTK_BOX (hbox), cntl_vbox, FALSE, FALSE, 0);

  create_control (cntl_vbox, 1, CNTL_ALIGN, &callback[0]);
  create_control (cntl_vbox, 2, CNTL_ALIGN, &callback[1]);
  create_control (cntl_vbox, 3, CNTL_ALIGN, &callback[2]);
  create_control (cntl_vbox, 4, CNTL_ALIGN, &callback[3]);

  /* Expand controls */
  cntl_vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 2);
  ctk_widget_show (cntl_vbox);
  ctk_box_pack_start (CTK_BOX (hbox), cntl_vbox, FALSE, FALSE, 0);

  create_control (cntl_vbox, 1, CNTL_EXPAND, &callback[0]);
  create_control (cntl_vbox, 2, CNTL_EXPAND, &callback[1]);
  create_control (cntl_vbox, 3, CNTL_EXPAND, &callback[2]);
  create_control (cntl_vbox, 4, CNTL_EXPAND, &callback[3]);

  /* Fixed controls */
  cntl_vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 2);
  ctk_widget_show (cntl_vbox);
  ctk_box_pack_start (CTK_BOX (hbox), cntl_vbox, FALSE, FALSE, 0);

  create_control (cntl_vbox, 1, CNTL_FIXED, &callback[0]);
  create_control (cntl_vbox, 2, CNTL_FIXED, &callback[1]);
  create_control (cntl_vbox, 3, CNTL_FIXED, &callback[2]);
  create_control (cntl_vbox, 4, CNTL_FIXED, &callback[3]);

  ctk_widget_show_all (window);
  ctk_main ();

  return 0;
}
