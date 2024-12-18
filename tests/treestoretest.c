/* treestoretest.c
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
#include <stdlib.h>
#include <string.h>

CtkTreeStore *base_model;
static gint node_count = 0;

static void
selection_changed (CtkTreeSelection *selection,
		   CtkWidget        *button)
{
  if (ctk_tree_selection_get_selected (selection, NULL, NULL))
    ctk_widget_set_sensitive (button, TRUE);
  else
    ctk_widget_set_sensitive (button, FALSE);
}

static void
node_set (CtkTreeIter *iter)
{
  gint n;
  gchar *str;

  str = g_strdup_printf ("Row (<span color=\"red\">%d</span>)", node_count++);
  ctk_tree_store_set (base_model, iter, 0, str, -1);
  g_free (str);

  n = g_random_int_range (10000,99999);
  if (n < 0)
    n *= -1;
  str = g_strdup_printf ("%d", n);

  ctk_tree_store_set (base_model, iter, 1, str, -1);
  g_free (str);
}

static void
iter_remove (CtkWidget   *button G_GNUC_UNUSED,
	     CtkTreeView *tree_view)
{
  CtkTreeIter selected;
  CtkTreeModel *model;

  model = ctk_tree_view_get_model (tree_view);

  if (ctk_tree_selection_get_selected (ctk_tree_view_get_selection (tree_view),
				       NULL,
				       &selected))
    {
      if (CTK_IS_TREE_STORE (model))
	{
	  ctk_tree_store_remove (CTK_TREE_STORE (model), &selected);
	}
    }
}

static void
iter_insert (CtkWidget *button, CtkTreeView *tree_view)
{
  CtkWidget *entry;
  CtkTreeIter iter;
  CtkTreeIter selected;
  CtkTreeModel *model = ctk_tree_view_get_model (tree_view);

  entry = g_object_get_data (G_OBJECT (button), "user_data");
  if (ctk_tree_selection_get_selected (ctk_tree_view_get_selection (CTK_TREE_VIEW (tree_view)),
				       NULL,
				       &selected))
    {
      ctk_tree_store_insert (CTK_TREE_STORE (model),
			     &iter,
			     &selected,
			     atoi (ctk_entry_get_text (CTK_ENTRY (entry))));
    }
  else
    {
      ctk_tree_store_insert (CTK_TREE_STORE (model),
			     &iter,
			     NULL,
			     atoi (ctk_entry_get_text (CTK_ENTRY (entry))));
    }

  node_set (&iter);
}

static void
iter_change (CtkWidget *button, CtkTreeView *tree_view)
{
  CtkWidget *entry;
  CtkTreeIter selected;
  CtkTreeModel *model = ctk_tree_view_get_model (tree_view);

  entry = g_object_get_data (G_OBJECT (button), "user_data");
  if (ctk_tree_selection_get_selected (ctk_tree_view_get_selection (CTK_TREE_VIEW (tree_view)),
				       NULL, &selected))
    {
      ctk_tree_store_set (CTK_TREE_STORE (model),
			  &selected,
			  1,
			  ctk_entry_get_text (CTK_ENTRY (entry)),
			  -1);
    }
}

static void
iter_insert_with_values (CtkWidget *button, CtkTreeView *tree_view)
{
  CtkWidget *entry;
  CtkTreeIter iter;
  CtkTreeIter selected;
  CtkTreeModel *model = ctk_tree_view_get_model (tree_view);
  gchar *str1, *str2;

  entry = g_object_get_data (G_OBJECT (button), "user_data");
  str1 = g_strdup_printf ("Row (<span color=\"red\">%d</span>)", node_count++);
  str2 = g_strdup_printf ("%d", atoi (ctk_entry_get_text (CTK_ENTRY (entry))));

  if (ctk_tree_selection_get_selected (ctk_tree_view_get_selection (CTK_TREE_VIEW (tree_view)),
				       NULL,
				       &selected))
    {
      ctk_tree_store_insert_with_values (CTK_TREE_STORE (model),
					 &iter,
					 &selected,
					 -1,
					 0, str1,
					 1, str2,
					 -1);
    }
  else
    {
      ctk_tree_store_insert_with_values (CTK_TREE_STORE (model),
					 &iter,
					 NULL,
					 -1,
					 0, str1,
					 1, str2,
					 -1);
    }

  g_free (str1);
  g_free (str2);
}

static void
iter_insert_before  (CtkWidget   *button G_GNUC_UNUSED,
		     CtkTreeView *tree_view)
{
  CtkTreeIter iter;
  CtkTreeIter selected;
  CtkTreeModel *model = ctk_tree_view_get_model (tree_view);

  if (ctk_tree_selection_get_selected (ctk_tree_view_get_selection (CTK_TREE_VIEW (tree_view)),
				       NULL,
				       &selected))
    {
      ctk_tree_store_insert_before (CTK_TREE_STORE (model),
				    &iter,
				    NULL,
				    &selected);
    }
  else
    {
      ctk_tree_store_insert_before (CTK_TREE_STORE (model),
				    &iter,
				    NULL,
				    NULL);
    }

  node_set (&iter);
}

static void
iter_insert_after (CtkWidget   *button G_GNUC_UNUSED,
		   CtkTreeView *tree_view)
{
  CtkTreeIter iter;
  CtkTreeIter selected;
  CtkTreeModel *model = ctk_tree_view_get_model (tree_view);

  if (ctk_tree_selection_get_selected (ctk_tree_view_get_selection (CTK_TREE_VIEW (tree_view)),
				       NULL,
				       &selected))
    {
      if (CTK_IS_TREE_STORE (model))
	{
	  ctk_tree_store_insert_after (CTK_TREE_STORE (model),
				       &iter,
				       NULL,
				       &selected);
	  node_set (&iter);
	}
    }
  else
    {
      if (CTK_IS_TREE_STORE (model))
	{
	  ctk_tree_store_insert_after (CTK_TREE_STORE (model),
				       &iter,
				       NULL,
				       NULL);
	  node_set (&iter);
	}
    }
}

static void
iter_prepend (CtkWidget   *button G_GNUC_UNUSED,
	      CtkTreeView *tree_view)
{
  CtkTreeIter iter;
  CtkTreeIter selected;
  CtkTreeModel *model = ctk_tree_view_get_model (tree_view);
  CtkTreeSelection *selection = ctk_tree_view_get_selection (tree_view);

  if (ctk_tree_selection_get_selected (selection, NULL, &selected))
    {
      if (CTK_IS_TREE_STORE (model))
	{
	  ctk_tree_store_prepend (CTK_TREE_STORE (model),
				  &iter,
				  &selected);
	  node_set (&iter);
	}
    }
  else
    {
      if (CTK_IS_TREE_STORE (model))
	{
	  ctk_tree_store_prepend (CTK_TREE_STORE (model),
				  &iter,
				  NULL);
	  node_set (&iter);
	}
    }
}

static void
iter_append (CtkWidget   *button G_GNUC_UNUSED,
	     CtkTreeView *tree_view)
{
  CtkTreeIter iter;
  CtkTreeIter selected;
  CtkTreeModel *model = ctk_tree_view_get_model (tree_view);

  if (ctk_tree_selection_get_selected (ctk_tree_view_get_selection (CTK_TREE_VIEW (tree_view)),
				       NULL,
				       &selected))
    {
      if (CTK_IS_TREE_STORE (model))
	{
	  ctk_tree_store_append (CTK_TREE_STORE (model), &iter, &selected);
	  node_set (&iter);
	}
    }
  else
    {
      if (CTK_IS_TREE_STORE (model))
	{
	  ctk_tree_store_append (CTK_TREE_STORE (model), &iter, NULL);
	  node_set (&iter);
	}
    }
}

static void
make_window (gint view_type)
{
  CtkWidget *window;
  CtkWidget *vbox;
  CtkWidget *hbox, *entry;
  CtkWidget *button;
  CtkWidget *scrolled_window;
  CtkWidget *tree_view;
  CtkTreeViewColumn *column;
  CtkCellRenderer *cell;
  GObject *selection;

  /* Make the Widgets/Objects */
  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  switch (view_type)
    {
    case 0:
      ctk_window_set_title (CTK_WINDOW (window), "Unsorted list");
      break;
    case 1:
      ctk_window_set_title (CTK_WINDOW (window), "Sorted list");
      break;
    }

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 8);
  ctk_container_set_border_width (CTK_CONTAINER (vbox), 8);
  ctk_window_set_default_size (CTK_WINDOW (window), 300, 350);
  scrolled_window = ctk_scrolled_window_new (NULL, NULL);
  switch (view_type)
    {
    case 0:
      tree_view = ctk_tree_view_new_with_model (CTK_TREE_MODEL (base_model));
      break;
    case 1:
      {
	CtkTreeModel *sort_model;
	
	sort_model = ctk_tree_model_sort_new_with_model (CTK_TREE_MODEL (base_model));
	tree_view = ctk_tree_view_new_with_model (CTK_TREE_MODEL (sort_model));
      }
      break;
    default:
      g_assert_not_reached ();
      tree_view = NULL; /* Quiet compiler */
      break;
    }

  selection = G_OBJECT (ctk_tree_view_get_selection (CTK_TREE_VIEW (tree_view)));
  ctk_tree_selection_set_mode (CTK_TREE_SELECTION (selection), CTK_SELECTION_SINGLE);

  /* Put them together */
  ctk_container_add (CTK_CONTAINER (scrolled_window), tree_view);
  ctk_box_pack_start (CTK_BOX (vbox), scrolled_window, TRUE, TRUE, 0);
  ctk_container_add (CTK_CONTAINER (window), vbox);
  ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (scrolled_window),
				  CTK_POLICY_AUTOMATIC,
				  CTK_POLICY_AUTOMATIC);
  g_signal_connect (window, "destroy", ctk_main_quit, NULL);

  /* buttons */
  button = ctk_button_new_with_label ("ctk_tree_store_remove");
  ctk_box_pack_start (CTK_BOX (vbox), button, FALSE, FALSE, 0);
  g_signal_connect (selection, "changed",
                    G_CALLBACK (selection_changed),
                    button);
  g_signal_connect (button, "clicked", 
                    G_CALLBACK (iter_remove), 
                    tree_view);
  ctk_widget_set_sensitive (button, FALSE);

  button = ctk_button_new_with_label ("ctk_tree_store_insert");
  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 8);
  entry = ctk_entry_new ();
  ctk_box_pack_start (CTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  ctk_box_pack_start (CTK_BOX (hbox), button, TRUE, TRUE, 0);
  ctk_box_pack_start (CTK_BOX (hbox), entry, FALSE, FALSE, 0);
  g_object_set_data (G_OBJECT (button), "user_data", entry);
  g_signal_connect (button, "clicked", 
                    G_CALLBACK (iter_insert), 
                    tree_view);
  
  button = ctk_button_new_with_label ("ctk_tree_store_set");
  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 8);
  entry = ctk_entry_new ();
  ctk_box_pack_start (CTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  ctk_box_pack_start (CTK_BOX (hbox), button, TRUE, TRUE, 0);
  ctk_box_pack_start (CTK_BOX (hbox), entry, FALSE, FALSE, 0);
  g_object_set_data (G_OBJECT (button), "user_data", entry);
  g_signal_connect (button, "clicked",
		    G_CALLBACK (iter_change),
		    tree_view);

  button = ctk_button_new_with_label ("ctk_tree_store_insert_with_values");
  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 8);
  entry = ctk_entry_new ();
  ctk_box_pack_start (CTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  ctk_box_pack_start (CTK_BOX (hbox), button, TRUE, TRUE, 0);
  ctk_box_pack_start (CTK_BOX (hbox), entry, FALSE, FALSE, 0);
  g_object_set_data (G_OBJECT (button), "user_data", entry);
  g_signal_connect (button, "clicked",
		    G_CALLBACK (iter_insert_with_values),
		    tree_view);
  
  button = ctk_button_new_with_label ("ctk_tree_store_insert_before");
  ctk_box_pack_start (CTK_BOX (vbox), button, FALSE, FALSE, 0);
  g_signal_connect (button, "clicked", 
                    G_CALLBACK (iter_insert_before), 
                    tree_view);
  g_signal_connect (selection, "changed",
                    G_CALLBACK (selection_changed),
                    button);
  ctk_widget_set_sensitive (button, FALSE);

  button = ctk_button_new_with_label ("ctk_tree_store_insert_after");
  ctk_box_pack_start (CTK_BOX (vbox), button, FALSE, FALSE, 0);
  g_signal_connect (button, "clicked", 
                    G_CALLBACK (iter_insert_after), 
                    tree_view);
  g_signal_connect (selection, "changed",
                    G_CALLBACK (selection_changed),
                    button);
  ctk_widget_set_sensitive (button, FALSE);

  button = ctk_button_new_with_label ("ctk_tree_store_prepend");
  ctk_box_pack_start (CTK_BOX (vbox), button, FALSE, FALSE, 0);
  g_signal_connect (button, "clicked", 
                    G_CALLBACK (iter_prepend), 
                    tree_view);

  button = ctk_button_new_with_label ("ctk_tree_store_append");
  ctk_box_pack_start (CTK_BOX (vbox), button, FALSE, FALSE, 0);
  g_signal_connect (button, "clicked", 
                    G_CALLBACK (iter_append), 
                    tree_view);

  /* The selected column */
  cell = ctk_cell_renderer_text_new ();
  column = ctk_tree_view_column_new_with_attributes ("Node ID", cell, "markup", 0, NULL);
  ctk_tree_view_column_set_sort_column_id (column, 0);
  ctk_tree_view_append_column (CTK_TREE_VIEW (tree_view), column);

  cell = ctk_cell_renderer_text_new ();
  column = ctk_tree_view_column_new_with_attributes ("Random Number", cell, "text", 1, NULL);
  ctk_tree_view_column_set_sort_column_id (column, 1);
  ctk_tree_view_append_column (CTK_TREE_VIEW (tree_view), column);

  /* A few to start */
  if (view_type == 0)
    {
      iter_append (NULL, CTK_TREE_VIEW (tree_view));
      iter_append (NULL, CTK_TREE_VIEW (tree_view));
      iter_append (NULL, CTK_TREE_VIEW (tree_view));
      iter_append (NULL, CTK_TREE_VIEW (tree_view));
      iter_append (NULL, CTK_TREE_VIEW (tree_view));
      iter_append (NULL, CTK_TREE_VIEW (tree_view));
    }
  /* Show it all */
  ctk_widget_show_all (window);
}

int
main (int argc, char *argv[])
{
  ctk_init (&argc, &argv);

  base_model = ctk_tree_store_new (2, G_TYPE_STRING, G_TYPE_STRING);

  /* FIXME: reverse this */
  make_window (0);
  make_window (1);

  ctk_main ();

  return 0;
}

