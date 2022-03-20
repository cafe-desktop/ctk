/* testtreesort.c
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


typedef struct _ListSort ListSort;
struct _ListSort
{
  const gchar *word_1;
  const gchar *word_2;
  const gchar *word_3;
  const gchar *word_4;
  gint number_1;
};

static ListSort data[] =
{
  { "Apples", "Transmogrify long word to demonstrate weirdness", "Exculpatory", "Gesundheit", 30 },
  { "Oranges", "Wicker", "Adamantine", "Convivial", 10 },
  { "Bovine Spongiform Encephilopathy", "Sleazebucket", "Mountaineer", "Pander", 40 },
  { "Foot and Mouth", "Lampshade", "Skim Milk\nFull Milk", "Viewless", 20 },
  { "Blood,\nsweat,\ntears", "The Man", "Horses", "Muckety-Muck", 435 },
  { "Rare Steak", "Siam", "Watchdog", "Xantippe" , 99999 },
  { "SIGINT", "Rabbit Breath", "Alligator", "Bloodstained", 4123 },
  { "Google", "Chrysanthemums", "Hobnob", "Leapfrog", 1 },
  { "Technology fibre optic", "Turtle", "Academe", "Lonely", 3 },
  { "Freon", "Harpes", "Quidditch", "Reagan", 6},
  { "Transposition", "Fruit Basket", "Monkey Wort", "Glogg", 54 },
  { "Fern", "Glasnost and Perestroika", "Latitude", "Bomberman!!!", 2 },
  {NULL, }
};

static ListSort childdata[] =
{
  { "Heineken", "Nederland", "Wanda de vis", "Electronische post", 2},
  { "Hottentottententententoonstelling", "Rotterdam", "Ionentransport", "Palm", 45},
  { "Fruitvlieg", "Eigenfrequentie", "Supernoodles", "Ramen", 2002},
  { "Gereedschapskist", "Stelsel van lineaire vergelijkingen", "Tulpen", "Badlaken", 1311},
  { "Stereoinstallatie", "Rood tapijt", "Het periodieke systeem der elementen", "Laaste woord", 200},
  {NULL, }
};
  

enum
{
  WORD_COLUMN_1 = 0,
  WORD_COLUMN_2,
  WORD_COLUMN_3,
  WORD_COLUMN_4,
  NUMBER_COLUMN_1,
  NUM_COLUMNS
};

gboolean
select_func (CtkTreeSelection  *selection,
	     CtkTreeModel      *model,
	     CtkTreePath       *path,
	     gboolean           path_currently_selected,
	     gpointer           data)
{
  if (ctk_tree_path_get_depth (path) > 1)
    return TRUE;
  return FALSE;
}

static void
switch_search_method (CtkWidget *button,
		      gpointer   tree_view)
{
  if (!ctk_tree_view_get_search_entry (CTK_TREE_VIEW (tree_view)))
    {
      gpointer data = g_object_get_data (tree_view, "my-search-entry");
      ctk_tree_view_set_search_entry (CTK_TREE_VIEW (tree_view), CTK_ENTRY (data));
    }
  else
    ctk_tree_view_set_search_entry (CTK_TREE_VIEW (tree_view), NULL);
}

int
main (int argc, char *argv[])
{
  CtkWidget *window;
  CtkWidget *vbox;
  CtkWidget *scrolled_window;
  CtkWidget *tree_view;
  CtkTreeStore *model;
  CtkTreeModel *smodel = NULL;
  CtkTreeModel *ssmodel = NULL;
  CtkCellRenderer *renderer;
  CtkTreeViewColumn *column;
  CtkTreeIter iter;
  gint i;

  CtkWidget *entry, *button;

  ctk_init (&argc, &argv);

  /**
   * First window - Just a CtkTreeStore
   */

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_title (CTK_WINDOW (window), "Words, words, words - Window 1");
  g_signal_connect (window, "destroy", ctk_main_quit, NULL);
  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 8);
  ctk_container_set_border_width (CTK_CONTAINER (vbox), 8);
  ctk_box_pack_start (CTK_BOX (vbox), ctk_label_new ("Jonathan and Kristian's list of cool words. (And Anders' cool list of numbers) \n\nThis is just a CtkTreeStore"), FALSE, FALSE, 0);
  ctk_container_add (CTK_CONTAINER (window), vbox);

  entry = ctk_entry_new ();
  ctk_box_pack_start (CTK_BOX (vbox), entry, FALSE, FALSE, 0);

  button = ctk_button_new_with_label ("Switch search method");
  ctk_box_pack_start (CTK_BOX (vbox), button, FALSE, FALSE, 0);

  scrolled_window = ctk_scrolled_window_new (NULL, NULL);
  ctk_scrolled_window_set_shadow_type (CTK_SCROLLED_WINDOW (scrolled_window), CTK_SHADOW_ETCHED_IN);
  ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (scrolled_window), CTK_POLICY_AUTOMATIC, CTK_POLICY_AUTOMATIC);
  ctk_box_pack_start (CTK_BOX (vbox), scrolled_window, TRUE, TRUE, 0);

  model = ctk_tree_store_new (NUM_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);

/*
  smodel = ctk_tree_model_sort_new_with_model (CTK_TREE_MODEL (model));
  ssmodel = ctk_tree_model_sort_new_with_model (CTK_TREE_MODEL (smodel));
*/
  tree_view = ctk_tree_view_new_with_model (CTK_TREE_MODEL (model));

  ctk_tree_view_set_search_entry (CTK_TREE_VIEW (tree_view), CTK_ENTRY (entry));
  g_object_set_data (G_OBJECT (tree_view), "my-search-entry", entry);
  g_signal_connect (button, "clicked",
		    G_CALLBACK (switch_search_method), tree_view);

 /* ctk_tree_selection_set_select_function (ctk_tree_view_get_selection (CTK_TREE_VIEW (tree_view)), select_func, NULL, NULL);*/

  /* 12 iters now, 12 later... */
  for (i = 0; data[i].word_1 != NULL; i++)
    {
      gint k;
      CtkTreeIter child_iter;


      ctk_tree_store_prepend (CTK_TREE_STORE (model), &iter, NULL);
      ctk_tree_store_set (CTK_TREE_STORE (model), &iter,
			  WORD_COLUMN_1, data[i].word_1,
			  WORD_COLUMN_2, data[i].word_2,
			  WORD_COLUMN_3, data[i].word_3,
			  WORD_COLUMN_4, data[i].word_4,
			  NUMBER_COLUMN_1, data[i].number_1,
			  -1);

      ctk_tree_store_append (CTK_TREE_STORE (model), &child_iter, &iter);
      ctk_tree_store_set (CTK_TREE_STORE (model), &child_iter,
			  WORD_COLUMN_1, data[i].word_1,
			  WORD_COLUMN_2, data[i].word_2,
			  WORD_COLUMN_3, data[i].word_3,
			  WORD_COLUMN_4, data[i].word_4,
			  NUMBER_COLUMN_1, data[i].number_1,
			  -1);

      for (k = 0; childdata[k].word_1 != NULL; k++)
	{
	  ctk_tree_store_append (CTK_TREE_STORE (model), &child_iter, &iter);
	  ctk_tree_store_set (CTK_TREE_STORE (model), &child_iter,
			      WORD_COLUMN_1, childdata[k].word_1,
			      WORD_COLUMN_2, childdata[k].word_2,
			      WORD_COLUMN_3, childdata[k].word_3,
			      WORD_COLUMN_4, childdata[k].word_4,
			      NUMBER_COLUMN_1, childdata[k].number_1,
			      -1);

	}

    }
  
  smodel = ctk_tree_model_sort_new_with_model (CTK_TREE_MODEL (model));
  ssmodel = ctk_tree_model_sort_new_with_model (CTK_TREE_MODEL (smodel));
  g_object_unref (model);

  renderer = ctk_cell_renderer_text_new ();
  column = ctk_tree_view_column_new_with_attributes ("First Word", renderer,
						     "text", WORD_COLUMN_1,
						     NULL);
  ctk_tree_view_append_column (CTK_TREE_VIEW (tree_view), column);
  ctk_tree_view_column_set_sort_column_id (column, WORD_COLUMN_1);

  renderer = ctk_cell_renderer_text_new ();
  column = ctk_tree_view_column_new_with_attributes ("Second Word", renderer,
						     "text", WORD_COLUMN_2,
						     NULL);
  ctk_tree_view_column_set_sort_column_id (column, WORD_COLUMN_2);
  ctk_tree_view_append_column (CTK_TREE_VIEW (tree_view), column);

  renderer = ctk_cell_renderer_text_new ();
  column = ctk_tree_view_column_new_with_attributes ("Third Word", renderer,
						     "text", WORD_COLUMN_3,
						     NULL);
  ctk_tree_view_column_set_sort_column_id (column, WORD_COLUMN_3);
  ctk_tree_view_append_column (CTK_TREE_VIEW (tree_view), column);

  renderer = ctk_cell_renderer_text_new ();
  column = ctk_tree_view_column_new_with_attributes ("Fourth Word", renderer,
						     "text", WORD_COLUMN_4,
						     NULL);
  ctk_tree_view_column_set_sort_column_id (column, WORD_COLUMN_4);
  ctk_tree_view_append_column (CTK_TREE_VIEW (tree_view), column);
  
  renderer = ctk_cell_renderer_text_new ();
  column = ctk_tree_view_column_new_with_attributes ("First Number", renderer,
						     "text", NUMBER_COLUMN_1,
						     NULL);
  ctk_tree_view_column_set_sort_column_id (column, NUMBER_COLUMN_1);
  ctk_tree_view_append_column (CTK_TREE_VIEW (tree_view), column);

  /*  ctk_tree_sortable_set_sort_column_id (CTK_TREE_SORTABLE (smodel),
					WORD_COLUMN_1,
					CTK_SORT_ASCENDING);*/

  ctk_container_add (CTK_CONTAINER (scrolled_window), tree_view);
  ctk_window_set_default_size (CTK_WINDOW (window), 400, 400);
  ctk_widget_show_all (window);

  /**
   * Second window - CtkTreeModelSort wrapping the CtkTreeStore
   */

  if (smodel)
    {
      CtkWidget *window2, *vbox2, *scrolled_window2, *tree_view2;

      window2 = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_title (CTK_WINDOW (window2), 
			    "Words, words, words - window 2");
      g_signal_connect (window2, "destroy", ctk_main_quit, NULL);
      vbox2 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 8);
      ctk_container_set_border_width (CTK_CONTAINER (vbox2), 8);
      ctk_box_pack_start (CTK_BOX (vbox2), 
			  ctk_label_new ("Jonathan and Kristian's list of words.\n\nA CtkTreeModelSort wrapping the CtkTreeStore of window 1"),
			  FALSE, FALSE, 0);
      ctk_container_add (CTK_CONTAINER (window2), vbox2);
      
      scrolled_window2 = ctk_scrolled_window_new (NULL, NULL);
      ctk_scrolled_window_set_shadow_type (CTK_SCROLLED_WINDOW (scrolled_window2),
					   CTK_SHADOW_ETCHED_IN);
      ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (scrolled_window2),
				      CTK_POLICY_AUTOMATIC,
				      CTK_POLICY_AUTOMATIC);
      ctk_box_pack_start (CTK_BOX (vbox2), scrolled_window2, TRUE, TRUE, 0);
      
      
      tree_view2 = ctk_tree_view_new_with_model (smodel);
      
      renderer = ctk_cell_renderer_text_new ();
      column = ctk_tree_view_column_new_with_attributes ("First Word", renderer,
							 "text", WORD_COLUMN_1,
							 NULL);
      ctk_tree_view_append_column (CTK_TREE_VIEW (tree_view2), column);
      ctk_tree_view_column_set_sort_column_id (column, WORD_COLUMN_1);
      
      renderer = ctk_cell_renderer_text_new ();
      column = ctk_tree_view_column_new_with_attributes ("Second Word", renderer,
							 "text", WORD_COLUMN_2,
							 NULL);
      ctk_tree_view_column_set_sort_column_id (column, WORD_COLUMN_2);
      ctk_tree_view_append_column (CTK_TREE_VIEW (tree_view2), column);
      
      renderer = ctk_cell_renderer_text_new ();
      column = ctk_tree_view_column_new_with_attributes ("Third Word", renderer,
							 "text", WORD_COLUMN_3,
							 NULL);
      ctk_tree_view_column_set_sort_column_id (column, WORD_COLUMN_3);
      ctk_tree_view_append_column (CTK_TREE_VIEW (tree_view2), column);
      
      renderer = ctk_cell_renderer_text_new ();
      column = ctk_tree_view_column_new_with_attributes ("Fourth Word", renderer,
							 "text", WORD_COLUMN_4,
							 NULL);
      ctk_tree_view_column_set_sort_column_id (column, WORD_COLUMN_4);
      ctk_tree_view_append_column (CTK_TREE_VIEW (tree_view2), column);
      
      /*      ctk_tree_sortable_set_default_sort_func (CTK_TREE_SORTABLE (smodel),
					       (CtkTreeIterCompareFunc)ctk_tree_data_list_compare_func,
					       NULL, NULL);
      ctk_tree_sortable_set_sort_column_id (CTK_TREE_SORTABLE (smodel),
					    WORD_COLUMN_1,
					    CTK_SORT_DESCENDING);*/
      
      
      ctk_container_add (CTK_CONTAINER (scrolled_window2), tree_view2);
      ctk_window_set_default_size (CTK_WINDOW (window2), 400, 400);
      ctk_widget_show_all (window2);
    }
  
  /**
   * Third window - CtkTreeModelSort wrapping the CtkTreeModelSort which
   * is wrapping the CtkTreeStore.
   */
  
  if (ssmodel)
    {
      CtkWidget *window3, *vbox3, *scrolled_window3, *tree_view3;

      window3 = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_title (CTK_WINDOW (window3), 
			    "Words, words, words - Window 3");
      g_signal_connect (window3, "destroy", ctk_main_quit, NULL);
      vbox3 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 8);
      ctk_container_set_border_width (CTK_CONTAINER (vbox3), 8);
      ctk_box_pack_start (CTK_BOX (vbox3), 
			  ctk_label_new ("Jonathan and Kristian's list of words.\n\nA CtkTreeModelSort wrapping the CtkTreeModelSort of window 2"),
			  FALSE, FALSE, 0);
      ctk_container_add (CTK_CONTAINER (window3), vbox3);
      
      scrolled_window3 = ctk_scrolled_window_new (NULL, NULL);
      ctk_scrolled_window_set_shadow_type (CTK_SCROLLED_WINDOW (scrolled_window3),
					   CTK_SHADOW_ETCHED_IN);
      ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (scrolled_window3),
				      CTK_POLICY_AUTOMATIC,
				      CTK_POLICY_AUTOMATIC);
      ctk_box_pack_start (CTK_BOX (vbox3), scrolled_window3, TRUE, TRUE, 0);
      
      
      tree_view3 = ctk_tree_view_new_with_model (ssmodel);
      
      renderer = ctk_cell_renderer_text_new ();
      column = ctk_tree_view_column_new_with_attributes ("First Word", renderer,
							 "text", WORD_COLUMN_1,
							 NULL);
      ctk_tree_view_append_column (CTK_TREE_VIEW (tree_view3), column);
      ctk_tree_view_column_set_sort_column_id (column, WORD_COLUMN_1);
      
      renderer = ctk_cell_renderer_text_new ();
      column = ctk_tree_view_column_new_with_attributes ("Second Word", renderer,
							 "text", WORD_COLUMN_2,
							 NULL);
      ctk_tree_view_column_set_sort_column_id (column, WORD_COLUMN_2);
      ctk_tree_view_append_column (CTK_TREE_VIEW (tree_view3), column);
      
      renderer = ctk_cell_renderer_text_new ();
      column = ctk_tree_view_column_new_with_attributes ("Third Word", renderer,
							 "text", WORD_COLUMN_3,
							 NULL);
      ctk_tree_view_column_set_sort_column_id (column, WORD_COLUMN_3);
      ctk_tree_view_append_column (CTK_TREE_VIEW (tree_view3), column);
      
      renderer = ctk_cell_renderer_text_new ();
      column = ctk_tree_view_column_new_with_attributes ("Fourth Word", renderer,
							 "text", WORD_COLUMN_4,
							 NULL);
      ctk_tree_view_column_set_sort_column_id (column, WORD_COLUMN_4);
      ctk_tree_view_append_column (CTK_TREE_VIEW (tree_view3), column);
      
      /*      ctk_tree_sortable_set_default_sort_func (CTK_TREE_SORTABLE (ssmodel),
					       (CtkTreeIterCompareFunc)ctk_tree_data_list_compare_func,
					       NULL, NULL);
      ctk_tree_sortable_set_sort_column_id (CTK_TREE_SORTABLE (ssmodel),
					    WORD_COLUMN_1,
					    CTK_SORT_ASCENDING);*/
      
      ctk_container_add (CTK_CONTAINER (scrolled_window3), tree_view3);
      ctk_window_set_default_size (CTK_WINDOW (window3), 400, 400);
      ctk_widget_show_all (window3);
    }

  for (i = 0; data[i].word_1 != NULL; i++)
    {
      gint k;
      
      ctk_tree_store_prepend (CTK_TREE_STORE (model), &iter, NULL);
      ctk_tree_store_set (CTK_TREE_STORE (model), &iter,
			  WORD_COLUMN_1, data[i].word_1,
			  WORD_COLUMN_2, data[i].word_2,
			  WORD_COLUMN_3, data[i].word_3,
			  WORD_COLUMN_4, data[i].word_4,
			  -1);
      for (k = 0; childdata[k].word_1 != NULL; k++)
	{
	  CtkTreeIter child_iter;
	  
	  ctk_tree_store_append (CTK_TREE_STORE (model), &child_iter, &iter);
	  ctk_tree_store_set (CTK_TREE_STORE (model), &child_iter,
			      WORD_COLUMN_1, childdata[k].word_1,
			      WORD_COLUMN_2, childdata[k].word_2,
			      WORD_COLUMN_3, childdata[k].word_3,
			      WORD_COLUMN_4, childdata[k].word_4,
			      -1);
	}
    }

  ctk_main ();
  
  return 0;
}
