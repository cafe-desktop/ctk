/* testtreemodel.c
 * Copyright (C) 2004  Red Hat, Inc.,  Matthias Clasen <mclasen@redhat.com>
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

#include <string.h>

#ifdef HAVE_MALLINFO
#include <malloc.h>
#endif

#include <ctk/ctk.h>

static gint repeats = 2;
static gint max_size = 8;

static GOptionEntry entries[] = {
  { "repeats", 'r', 0, G_OPTION_ARG_INT, &repeats, "Average over N repetitions", "N" },
  { "max-size", 'm', 0, G_OPTION_ARG_INT, &max_size, "Test up to 2^M items", "M" },
  { NULL }
};


typedef void (ClearFunc)(GtkTreeModel *model);
typedef void (InsertFunc)(GtkTreeModel *model,
			  gint          items,
			  gint          i);

static void
list_store_append (GtkTreeModel *model,
		   gint          items,
		   gint          i)
{
  GtkListStore *store = CTK_LIST_STORE (model);
  GtkTreeIter iter;
  gchar *text;

  text = g_strdup_printf ("row %d", i);
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 0, i, 1, text, -1);
  g_free (text);
}

static void
list_store_prepend (GtkTreeModel *model,
		    gint          items,
		    gint          i)
{
  GtkListStore *store = CTK_LIST_STORE (model);
  GtkTreeIter iter;
  gchar *text;

  text = g_strdup_printf ("row %d", i);
  ctk_list_store_prepend (store, &iter);
  ctk_list_store_set (store, &iter, 0, i, 1, text, -1);
  g_free (text);
}

static void
list_store_insert (GtkTreeModel *model,
		   gint          items,
		   gint          i)
{
  GtkListStore *store = CTK_LIST_STORE (model);
  GtkTreeIter iter;
  gchar *text;
  gint n;

  text = g_strdup_printf ("row %d", i);
  n = g_random_int_range (0, i + 1);
  ctk_list_store_insert (store, &iter, n);
  ctk_list_store_set (store, &iter, 0, i, 1, text, -1);
  g_free (text);
}

static gint
compare (GtkTreeModel *model,
	 GtkTreeIter  *a,
	 GtkTreeIter  *b,
	 gpointer      data)
{
  gchar *str_a, *str_b;
  gint result;

  ctk_tree_model_get (model, a, 1, &str_a, -1);
  ctk_tree_model_get (model, b, 1, &str_b, -1);
  
  result = strcmp (str_a, str_b);

  g_free (str_a);
  g_free (str_b);

  return result;
}

static void
tree_store_append (GtkTreeModel *model,
		   gint          items,
		   gint          i)
{
  GtkTreeStore *store = CTK_TREE_STORE (model);
  GtkTreeIter iter;
  gchar *text;

  text = g_strdup_printf ("row %d", i);
  ctk_tree_store_append (store, &iter, NULL);
  ctk_tree_store_set (store, &iter, 0, i, 1, text, -1);
  g_free (text);
}

static void
tree_store_prepend (GtkTreeModel *model,
		    gint          items,
		    gint          i)
{
  GtkTreeStore *store = CTK_TREE_STORE (model);
  GtkTreeIter iter;
  gchar *text;

  text = g_strdup_printf ("row %d", i);
  ctk_tree_store_prepend (store, &iter, NULL);
  ctk_tree_store_set (store, &iter, 0, i, 1, text, -1);
  g_free (text);
}

static void
tree_store_insert_flat (GtkTreeModel *model,
			gint          items,
			gint          i)
{
  GtkTreeStore *store = CTK_TREE_STORE (model);
  GtkTreeIter iter;
  gchar *text;
  gint n;

  text = g_strdup_printf ("row %d", i);
  n = g_random_int_range (0, i + 1);
  ctk_tree_store_insert (store, &iter, NULL, n);
  ctk_tree_store_set (store, &iter, 0, i, 1, text, -1);
  g_free (text);
}

typedef struct {
  gint i;
  gint n;
  gboolean found;
  GtkTreeIter iter;
} FindData;

static gboolean
find_nth (GtkTreeModel *model,
	  GtkTreePath  *path,
	  GtkTreeIter  *iter,
	  gpointer      data)
{
  FindData *fdata = (FindData *)data; 

  if (fdata->i >= fdata->n)
    {
      fdata->iter = *iter;
      fdata->found = TRUE;
      return TRUE;
    }

  fdata->i++;

  return FALSE;
}

static void
tree_store_insert_deep (GtkTreeModel *model,
			gint          items,
			gint          i)
{
  GtkTreeStore *store = CTK_TREE_STORE (model);
  GtkTreeIter iter;
  gchar *text;
  FindData data;

  text = g_strdup_printf ("row %d", i);
  data.n = g_random_int_range (0, items);
  data.i = 0;
  data.found = FALSE;
  if (data.n < i)
    ctk_tree_model_foreach (model, find_nth, &data);
  ctk_tree_store_insert (store, &iter, data.found ? &(data.iter) : NULL, data.n);
  ctk_tree_store_set (store, &iter, 0, i, 1, text, -1);
  g_free (text);
}


static void
test_run (gchar        *title,
	  GtkTreeModel *store,
	  ClearFunc    *clear,
	  InsertFunc   *insert)
{
  gint i, k, d, items;
  GTimer *timer;
  gdouble elapsed;
  int memused;
#ifdef HAVE_MALLINFO
  int uordblks_before = 0;
#endif

  g_print ("%s (average over %d runs, time in milliseconds)\n"
	   "items \ttime      \ttime/item \tused memory\n", title, repeats);

  timer = g_timer_new ();

  for (k = 0; k < max_size; k++)
    {
      items = 1 << k;
      elapsed = 0.0;
      for (d = 0; d < repeats; d++)
	{
	  (*clear)(store);
#ifdef HAVE_MALLINFO
	  /* Peculiar location of this, btw.  -- MW.  */
	  uordblks_before = mallinfo().uordblks;
#endif
	  g_timer_reset (timer);
	  g_timer_start (timer);
	  for (i = 0; i < items; i++)
	    (*insert) (store, items, i);
	  g_timer_stop (timer);
	  elapsed += g_timer_elapsed (timer, NULL);
	}
      
      elapsed = elapsed * 1000 / repeats;
#ifdef HAVE_MALLINFO
      memused = (mallinfo().uordblks - uordblks_before) / 1024;
#else
      memused = 0;
#endif
      g_print ("%d \t%f \t%f  \t%dk\n", 
	       items, elapsed, elapsed/items, memused);
    }  
}

int
main (int argc, char *argv[])
{
  GtkTreeModel *model;
  
  ctk_init_with_args (&argc, &argv, NULL, entries, NULL, NULL);

  model = CTK_TREE_MODEL (ctk_list_store_new (2, G_TYPE_INT, G_TYPE_STRING));
  
  test_run ("list store append", 
	    model, 
	    (ClearFunc*)ctk_list_store_clear, 
	    (InsertFunc*)list_store_append);

  test_run ("list store prepend", 
	    model, 
	    (ClearFunc*)ctk_list_store_clear, 
	    (InsertFunc*)list_store_prepend);

  test_run ("list store insert", 
	    model, 
	    (ClearFunc*)ctk_list_store_clear, 
	    (InsertFunc*)list_store_insert);

  ctk_tree_sortable_set_default_sort_func (CTK_TREE_SORTABLE (model), 
					   compare, NULL, NULL);
  ctk_tree_sortable_set_sort_column_id (CTK_TREE_SORTABLE (model), 
					CTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
					CTK_SORT_ASCENDING);

  test_run ("list store insert (sorted)", 
	    model, 
	    (ClearFunc*)ctk_list_store_clear, 
	    (InsertFunc*)list_store_insert);

  g_object_unref (model);
  
  model = CTK_TREE_MODEL (ctk_tree_store_new (2, G_TYPE_INT, G_TYPE_STRING));

  test_run ("tree store append", 
	    model, 
	    (ClearFunc*)ctk_tree_store_clear, 
	    (InsertFunc*)tree_store_append);

  test_run ("tree store prepend", 
	    model, 
	    (ClearFunc*)ctk_tree_store_clear, 
	    (InsertFunc*)tree_store_prepend);

  test_run ("tree store insert (flat)", 
	    model, 
	    (ClearFunc*)ctk_tree_store_clear, 
	    (InsertFunc*)tree_store_insert_flat);

  test_run ("tree store insert (deep)", 
	    model, 
	    (ClearFunc*)ctk_tree_store_clear, 
	    (InsertFunc*)tree_store_insert_deep);

  ctk_tree_sortable_set_default_sort_func (CTK_TREE_SORTABLE (model), 
					   compare, NULL, NULL);
  ctk_tree_sortable_set_sort_column_id (CTK_TREE_SORTABLE (model), 
					CTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
					CTK_SORT_ASCENDING);

  test_run ("tree store insert (flat, sorted)", 
	    model, 
	    (ClearFunc*)ctk_tree_store_clear, 
	    (InsertFunc*)tree_store_insert_flat);

  test_run ("tree store insert (deep, sorted)", 
	    model, 
	    (ClearFunc*)ctk_tree_store_clear, 
	    (InsertFunc*)tree_store_insert_deep);

  return 0;
}
