/* testtreefocus.c
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

typedef struct _TreeStruct TreeStruct;
struct _TreeStruct
{
  const gchar *label;
  gboolean alex;
  gboolean havoc;
  gboolean tim;
  gboolean owen;
  gboolean dave;
  gboolean world_holiday; /* shared by the european hackers */
  TreeStruct *children;
};


static TreeStruct january[] =
{
  {"New Years Day", TRUE, TRUE, TRUE, TRUE, FALSE, TRUE, NULL },
  {"Presidential Inauguration", FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, NULL },
  {"Martin Luther King Jr. day", FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, NULL },
  { NULL }
};

static TreeStruct february[] =
{
  { "Presidents' Day", FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, NULL },
  { "Groundhog Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Valentine's Day", FALSE, FALSE, FALSE, FALSE, TRUE, TRUE, NULL },
  { NULL }
};

static TreeStruct march[] =
{
  { "National Tree Planting Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "St Patrick's Day", FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, NULL },
  { NULL }
};

static TreeStruct april[] =
{
  { "April Fools' Day", FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, NULL },
  { "Army Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Earth Day", FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, NULL },
  { "Administrative Professionals' Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { NULL }
};

static TreeStruct may[] =
{
  { "Nurses' Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "National Day of Prayer", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Mothers' Day", FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, NULL },
  { "Armed Forces Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Memorial Day", TRUE, TRUE, TRUE, TRUE, FALSE, TRUE, NULL },
  { NULL }
};

static TreeStruct june[] =
{
  { "June Fathers' Day", FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, NULL },
  { "Juneteenth (Liberation of Slaves)", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Flag Day", FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, NULL },
  { NULL }
};

static TreeStruct july[] =
{
  { "Parents' Day", FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, NULL },
  { "Independence Day", FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, NULL },
  { NULL }
};

static TreeStruct august[] =
{
  { "Air Force Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Coast Guard Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Friendship Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { NULL }
};

static TreeStruct september[] =
{
  { "Grandparents' Day", FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, NULL },
  { "Citizenship Day or Constitution Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Labor Day", TRUE, TRUE, TRUE, TRUE, FALSE, TRUE, NULL },
  { NULL }
};

static TreeStruct october[] =
{
  { "National Children's Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Bosses' Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Sweetest Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Mother-in-Law's Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Navy Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Columbus Day", FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, NULL },
  { "Halloween", FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, NULL },
  { NULL }
};

static TreeStruct november[] =
{
  { "Marine Corps Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Veterans' Day", TRUE, TRUE, TRUE, TRUE, FALSE, TRUE, NULL },
  { "Thanksgiving", FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, NULL },
  { NULL }
};

static TreeStruct december[] =
{
  { "Pearl Harbor Remembrance Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Christmas", TRUE, TRUE, TRUE, TRUE, FALSE, TRUE, NULL },
  { "Kwanzaa", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { NULL }
};


static TreeStruct toplevel[] =
{
  {"January", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, january},
  {"February", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, february},
  {"March", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, march},
  {"April", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, april},
  {"May", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, may},
  {"June", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, june},
  {"July", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, july},
  {"August", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, august},
  {"September", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, september},
  {"October", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, october},
  {"November", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, november},
  {"December", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, december},
  {NULL}
};


enum
{
  HOLIDAY_COLUMN = 0,
  ALEX_COLUMN,
  HAVOC_COLUMN,
  TIM_COLUMN,
  OWEN_COLUMN,
  DAVE_COLUMN,
  VISIBLE_COLUMN,
  WORLD_COLUMN,
  NUM_COLUMNS
};

static CtkTreeModel *
make_model (void)
{
  CtkTreeStore *model;
  TreeStruct *month = toplevel;
  CtkTreeIter iter;

  model = ctk_tree_store_new (NUM_COLUMNS,
			      G_TYPE_STRING,
			      G_TYPE_BOOLEAN,
			      G_TYPE_BOOLEAN,
			      G_TYPE_BOOLEAN,
			      G_TYPE_BOOLEAN,
			      G_TYPE_BOOLEAN,
			      G_TYPE_BOOLEAN,
			      G_TYPE_BOOLEAN);

  while (month->label)
    {
      TreeStruct *holiday = month->children;

      ctk_tree_store_append (model, &iter, NULL);
      ctk_tree_store_set (model, &iter,
			  HOLIDAY_COLUMN, month->label,
			  ALEX_COLUMN, FALSE,
			  HAVOC_COLUMN, FALSE,
			  TIM_COLUMN, FALSE,
			  OWEN_COLUMN, FALSE,
			  DAVE_COLUMN, FALSE,
			  VISIBLE_COLUMN, FALSE,
			  WORLD_COLUMN, FALSE,
			  -1);
      while (holiday->label)
	{
	  CtkTreeIter child_iter;

	  ctk_tree_store_append (model, &child_iter, &iter);
	  ctk_tree_store_set (model, &child_iter,
			      HOLIDAY_COLUMN, holiday->label,
			      ALEX_COLUMN, holiday->alex,
			      HAVOC_COLUMN, holiday->havoc,
			      TIM_COLUMN, holiday->tim,
			      OWEN_COLUMN, holiday->owen,
			      DAVE_COLUMN, holiday->dave,
			      VISIBLE_COLUMN, TRUE,
			      WORLD_COLUMN, holiday->world_holiday,
			      -1);

	  holiday ++;
	}
      month ++;
    }

  return CTK_TREE_MODEL (model);
}

static void
alex_toggled (CtkCellRendererToggle *cell G_GNUC_UNUSED,
	      gchar                 *path_str,
	      gpointer               data)
{
  CtkTreeModel *model = (CtkTreeModel *) data;
  CtkTreeIter iter;
  CtkTreePath *path = ctk_tree_path_new_from_string (path_str);
  gboolean alex;

  ctk_tree_model_get_iter (model, &iter, path);
  ctk_tree_model_get (model, &iter, ALEX_COLUMN, &alex, -1);

  alex = !alex;
  ctk_tree_store_set (CTK_TREE_STORE (model), &iter, ALEX_COLUMN, alex, -1);

  ctk_tree_path_free (path);
}

static void
havoc_toggled (CtkCellRendererToggle *cell G_GNUC_UNUSED,
	       gchar                 *path_str,
	       gpointer               data)
{
  CtkTreeModel *model = (CtkTreeModel *) data;
  CtkTreeIter iter;
  CtkTreePath *path = ctk_tree_path_new_from_string (path_str);
  gboolean havoc;

  ctk_tree_model_get_iter (model, &iter, path);
  ctk_tree_model_get (model, &iter, HAVOC_COLUMN, &havoc, -1);

  havoc = !havoc;
  ctk_tree_store_set (CTK_TREE_STORE (model), &iter, HAVOC_COLUMN, havoc, -1);

  ctk_tree_path_free (path);
}

static void
owen_toggled (CtkCellRendererToggle *cell G_GNUC_UNUSED,
	      gchar                 *path_str,
	      gpointer               data)
{
  CtkTreeModel *model = (CtkTreeModel *) data;
  CtkTreeIter iter;
  CtkTreePath *path = ctk_tree_path_new_from_string (path_str);
  gboolean owen;

  ctk_tree_model_get_iter (model, &iter, path);
  ctk_tree_model_get (model, &iter, OWEN_COLUMN, &owen, -1);

  owen = !owen;
  ctk_tree_store_set (CTK_TREE_STORE (model), &iter, OWEN_COLUMN, owen, -1);

  ctk_tree_path_free (path);
}

static void
tim_toggled (CtkCellRendererToggle *cell G_GNUC_UNUSED,
	     gchar                 *path_str,
	     gpointer               data)
{
  CtkTreeModel *model = (CtkTreeModel *) data;
  CtkTreeIter iter;
  CtkTreePath *path = ctk_tree_path_new_from_string (path_str);
  gboolean tim;

  ctk_tree_model_get_iter (model, &iter, path);
  ctk_tree_model_get (model, &iter, TIM_COLUMN, &tim, -1);

  tim = !tim;
  ctk_tree_store_set (CTK_TREE_STORE (model), &iter, TIM_COLUMN, tim, -1);

  ctk_tree_path_free (path);
}

static void
dave_toggled (CtkCellRendererToggle *cell G_GNUC_UNUSED,
	      gchar                 *path_str,
	      gpointer               data)
{
  CtkTreeModel *model = (CtkTreeModel *) data;
  CtkTreeIter iter;
  CtkTreePath *path = ctk_tree_path_new_from_string (path_str);
  gboolean dave;

  ctk_tree_model_get_iter (model, &iter, path);
  ctk_tree_model_get (model, &iter, DAVE_COLUMN, &dave, -1);

  dave = !dave;
  ctk_tree_store_set (CTK_TREE_STORE (model), &iter, DAVE_COLUMN, dave, -1);

  ctk_tree_path_free (path);
}

static void
set_indicator_size (CtkTreeViewColumn *column G_GNUC_UNUSED,
		    CtkCellRenderer   *cell,
		    CtkTreeModel      *model,
		    CtkTreeIter       *iter,
		    gpointer           data G_GNUC_UNUSED)
{
  gint size;
  CtkTreePath *path;

  path = ctk_tree_model_get_path (model, iter);
  size = ctk_tree_path_get_indices (path)[0]  * 2 + 10;
  ctk_tree_path_free (path);

  g_object_set (cell, "indicator_size", size, NULL);
}

int
main (int argc, char *argv[])
{
  CtkWidget *window;
  CtkWidget *vbox;
  CtkWidget *scrolled_window;
  CtkWidget *tree_view;
  CtkTreeModel *model;
  CtkCellRenderer *renderer;
  gint col_offset;
  CtkTreeViewColumn *column;

  ctk_init (&argc, &argv);

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_title (CTK_WINDOW (window), "Card planning sheet");
  g_signal_connect (window, "destroy", ctk_main_quit, NULL);
  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 8);
  ctk_container_set_border_width (CTK_CONTAINER (vbox), 8);
  ctk_box_pack_start (CTK_BOX (vbox), ctk_label_new ("Jonathan's Holiday Card Planning Sheet"), FALSE, FALSE, 0);
  ctk_container_add (CTK_CONTAINER (window), vbox);

  scrolled_window = ctk_scrolled_window_new (NULL, NULL);
  ctk_scrolled_window_set_shadow_type (CTK_SCROLLED_WINDOW (scrolled_window), CTK_SHADOW_ETCHED_IN);
  ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (scrolled_window), CTK_POLICY_AUTOMATIC, CTK_POLICY_AUTOMATIC);
  ctk_box_pack_start (CTK_BOX (vbox), scrolled_window, TRUE, TRUE, 0);

  model = make_model ();
  tree_view = ctk_tree_view_new_with_model (model);
  ctk_tree_selection_set_mode (ctk_tree_view_get_selection (CTK_TREE_VIEW (tree_view)),
			       CTK_SELECTION_MULTIPLE);
  renderer = ctk_cell_renderer_text_new ();
  col_offset = ctk_tree_view_insert_column_with_attributes (CTK_TREE_VIEW (tree_view),
							    -1, "Holiday",
							    renderer,
							    "text", HOLIDAY_COLUMN, NULL);
  column = ctk_tree_view_get_column (CTK_TREE_VIEW (tree_view), col_offset - 1);
  ctk_tree_view_column_set_clickable (CTK_TREE_VIEW_COLUMN (column), TRUE);

  /* Alex Column */
  renderer = ctk_cell_renderer_toggle_new ();
  g_signal_connect (renderer, "toggled", G_CALLBACK (alex_toggled), model);

  g_object_set (renderer, "xalign", 0.0, NULL);
  col_offset = ctk_tree_view_insert_column_with_attributes (CTK_TREE_VIEW (tree_view),
							    -1, "Alex",
							    renderer,
							    "active", ALEX_COLUMN,
							    "visible", VISIBLE_COLUMN,
							    "activatable", WORLD_COLUMN,
							    NULL);
  column = ctk_tree_view_get_column (CTK_TREE_VIEW (tree_view), col_offset - 1);
  ctk_tree_view_column_set_sizing (CTK_TREE_VIEW_COLUMN (column), CTK_TREE_VIEW_COLUMN_FIXED);
  ctk_tree_view_column_set_fixed_width (CTK_TREE_VIEW_COLUMN (column), 50);
  ctk_tree_view_column_set_clickable (CTK_TREE_VIEW_COLUMN (column), TRUE);

  /* Havoc Column */
  renderer = ctk_cell_renderer_toggle_new ();
  g_signal_connect (renderer, "toggled", G_CALLBACK (havoc_toggled), model);

  g_object_set (renderer, "xalign", 0.0, NULL);
  col_offset = ctk_tree_view_insert_column_with_attributes (CTK_TREE_VIEW (tree_view),
							    -1, "Havoc",
							    renderer,
							    "active", HAVOC_COLUMN,
							    "visible", VISIBLE_COLUMN,
							    NULL);
  column = ctk_tree_view_get_column (CTK_TREE_VIEW (tree_view), col_offset - 1);
  ctk_tree_view_column_set_sizing (CTK_TREE_VIEW_COLUMN (column), CTK_TREE_VIEW_COLUMN_FIXED);
  ctk_tree_view_column_set_fixed_width (CTK_TREE_VIEW_COLUMN (column), 50);
  ctk_tree_view_column_set_clickable (CTK_TREE_VIEW_COLUMN (column), TRUE);

  /* Tim Column */
  renderer = ctk_cell_renderer_toggle_new ();
  g_signal_connect (renderer, "toggled", G_CALLBACK (tim_toggled), model);

  g_object_set (renderer, "xalign", 0.0, NULL);
  col_offset = ctk_tree_view_insert_column_with_attributes (CTK_TREE_VIEW (tree_view),
					       -1, "Tim",
					       renderer,
					       "active", TIM_COLUMN,
					       "visible", VISIBLE_COLUMN,
					       "activatable", WORLD_COLUMN,
					       NULL);
  column = ctk_tree_view_get_column (CTK_TREE_VIEW (tree_view), col_offset - 1);
  ctk_tree_view_column_set_sizing (CTK_TREE_VIEW_COLUMN (column), CTK_TREE_VIEW_COLUMN_FIXED);
  ctk_tree_view_column_set_clickable (CTK_TREE_VIEW_COLUMN (column), TRUE);
  ctk_tree_view_column_set_fixed_width (CTK_TREE_VIEW_COLUMN (column), 50);

  /* Owen Column */
  renderer = ctk_cell_renderer_toggle_new ();
  g_signal_connect (renderer, "toggled", G_CALLBACK (owen_toggled), model);
  g_object_set (renderer, "xalign", 0.0, NULL);
  col_offset = ctk_tree_view_insert_column_with_attributes (CTK_TREE_VIEW (tree_view),
					       -1, "Owen",
					       renderer,
					       "active", OWEN_COLUMN,
					       "visible", VISIBLE_COLUMN,
					       NULL);
  column = ctk_tree_view_get_column (CTK_TREE_VIEW (tree_view), col_offset - 1);
  ctk_tree_view_column_set_sizing (CTK_TREE_VIEW_COLUMN (column), CTK_TREE_VIEW_COLUMN_FIXED);
  ctk_tree_view_column_set_clickable (CTK_TREE_VIEW_COLUMN (column), TRUE);
  ctk_tree_view_column_set_fixed_width (CTK_TREE_VIEW_COLUMN (column), 50);

  /* Owen Column */
  renderer = ctk_cell_renderer_toggle_new ();
  g_signal_connect (renderer, "toggled", G_CALLBACK (dave_toggled), model);
  g_object_set (renderer, "xalign", 0.0, NULL);
  col_offset = ctk_tree_view_insert_column_with_attributes (CTK_TREE_VIEW (tree_view),
					       -1, "Dave",
					       renderer,
					       "active", DAVE_COLUMN,
					       "visible", VISIBLE_COLUMN,
					       NULL);
  column = ctk_tree_view_get_column (CTK_TREE_VIEW (tree_view), col_offset - 1);
  ctk_tree_view_column_set_cell_data_func (column, renderer, set_indicator_size, NULL, NULL);
  ctk_tree_view_column_set_sizing (CTK_TREE_VIEW_COLUMN (column), CTK_TREE_VIEW_COLUMN_FIXED);
  ctk_tree_view_column_set_fixed_width (CTK_TREE_VIEW_COLUMN (column), 50);
  ctk_tree_view_column_set_clickable (CTK_TREE_VIEW_COLUMN (column), TRUE);

  ctk_container_add (CTK_CONTAINER (scrolled_window), tree_view);

  g_signal_connect (tree_view, "realize",
		    G_CALLBACK (ctk_tree_view_expand_all),
		    NULL);
  ctk_window_set_default_size (CTK_WINDOW (window),
			       650, 400);
  ctk_widget_show_all (window);

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_title (CTK_WINDOW (window), "Model");
  g_signal_connect (window, "destroy", ctk_main_quit, NULL);
  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 8);
  ctk_container_set_border_width (CTK_CONTAINER (vbox), 8);
  ctk_box_pack_start (CTK_BOX (vbox), ctk_label_new ("The model revealed"), FALSE, FALSE, 0);
  ctk_container_add (CTK_CONTAINER (window), vbox);

  scrolled_window = ctk_scrolled_window_new (NULL, NULL);
  ctk_scrolled_window_set_shadow_type (CTK_SCROLLED_WINDOW (scrolled_window), CTK_SHADOW_ETCHED_IN);
  ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (scrolled_window), CTK_POLICY_AUTOMATIC, CTK_POLICY_AUTOMATIC);
  ctk_box_pack_start (CTK_BOX (vbox), scrolled_window, TRUE, TRUE, 0);


  tree_view = ctk_tree_view_new_with_model (model);
  g_object_unref (model);

  ctk_tree_view_insert_column_with_attributes (CTK_TREE_VIEW (tree_view),
					       -1, "Holiday Column",
					       ctk_cell_renderer_text_new (),
					       "text", 0, NULL);

  ctk_tree_view_insert_column_with_attributes (CTK_TREE_VIEW (tree_view),
					       -1, "Alex Column",
					       ctk_cell_renderer_text_new (),
					       "text", 1, NULL);

  ctk_tree_view_insert_column_with_attributes (CTK_TREE_VIEW (tree_view),
					       -1, "Havoc Column",
					       ctk_cell_renderer_text_new (),
					       "text", 2, NULL);

  ctk_tree_view_insert_column_with_attributes (CTK_TREE_VIEW (tree_view),
					       -1, "Tim Column",
					       ctk_cell_renderer_text_new (),
					       "text", 3, NULL);

  ctk_tree_view_insert_column_with_attributes (CTK_TREE_VIEW (tree_view),
					       -1, "Owen Column",
					       ctk_cell_renderer_text_new (),
					       "text", 4, NULL);

  ctk_tree_view_insert_column_with_attributes (CTK_TREE_VIEW (tree_view),
					       -1, "Dave Column",
					       ctk_cell_renderer_text_new (),
					       "text", 5, NULL);

  ctk_tree_view_insert_column_with_attributes (CTK_TREE_VIEW (tree_view),
					       -1, "Visible Column",
					       ctk_cell_renderer_text_new (),
					       "text", 6, NULL);

  ctk_tree_view_insert_column_with_attributes (CTK_TREE_VIEW (tree_view),
					       -1, "World Holiday",
					       ctk_cell_renderer_text_new (),
					       "text", 7, NULL);

  g_signal_connect (tree_view, "realize",
		    G_CALLBACK (ctk_tree_view_expand_all),
		    NULL);
			   
  ctk_container_add (CTK_CONTAINER (scrolled_window), tree_view);


  ctk_window_set_default_size (CTK_WINDOW (window),
			       650, 400);

  ctk_widget_show_all (window);
  ctk_main ();

  return 0;
}

