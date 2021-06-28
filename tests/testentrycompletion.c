/* testentrycompletion.c
 * Copyright (C) 2004  Red Hat, Inc.
 * Author: Matthias Clasen
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
#include <stdlib.h>
#include <string.h>
#include <ctk/ctk.h>

/* Don't copy this bad example; inline RGB data is always a better
 * idea than inline XPMs.
 */
static char  *book_closed_xpm[] = {
"16 16 6 1",
"       c None s None",
".      c black",
"X      c red",
"o      c yellow",
"O      c #808080",
"#      c white",
"                ",
"       ..       ",
"     ..XX.      ",
"   ..XXXXX.     ",
" ..XXXXXXXX.    ",
".ooXXXXXXXXX.   ",
"..ooXXXXXXXXX.  ",
".X.ooXXXXXXXXX. ",
".XX.ooXXXXXX..  ",
" .XX.ooXXX..#O  ",
"  .XX.oo..##OO. ",
"   .XX..##OO..  ",
"    .X.#OO..    ",
"     ..O..      ",
"      ..        ",
"                "
};

static CtkWidget *window = NULL;


/* Creates a tree model containing the completions */
CtkTreeModel *
create_simple_completion_model (void)
{
  CtkListStore *store;
  CtkTreeIter iter;
  
  store = ctk_list_store_new (1, G_TYPE_STRING);

  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 0, "GNOME", -1);
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 0, "gnominious", -1);
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 0, "Gnomonic projection", -1);

  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 0, "total", -1);
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 0, "totally", -1);
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 0, "toto", -1);
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 0, "tottery", -1);
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 0, "totterer", -1);
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 0, "Totten trust", -1);
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 0, "totipotent", -1);
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 0, "totipotency", -1);
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 0, "totemism", -1);
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 0, "totem pole", -1);
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 0, "Totara", -1);
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 0, "totalizer", -1);
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 0, "totalizator", -1);
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 0, "totalitarianism", -1);
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 0, "total parenteral nutrition", -1);
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 0, "total hysterectomy", -1);
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 0, "total eclipse", -1);
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 0, "Totipresence", -1);
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 0, "Totipalmi", -1);
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 0, "zombie", -1);
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 0, "a\303\246x", -1);
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 0, "a\303\246y", -1);
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 0, "a\303\246z", -1);
 
  return CTK_TREE_MODEL (store);
}

/* Creates a tree model containing the completions */
CtkTreeModel *
create_completion_model (void)
{
  CtkListStore *store;
  CtkTreeIter iter;
  GdkPixbuf *pixbuf;

  pixbuf = cdk_pixbuf_new_from_xpm_data ((const char **)book_closed_xpm);

  store = ctk_list_store_new (2, CDK_TYPE_PIXBUF, G_TYPE_STRING);

  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 0, pixbuf, 1, "ambient", -1);
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 0, pixbuf, 1, "ambidextrously", -1);
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 0, pixbuf, 1, "ambidexter", -1);
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 0, pixbuf, 1, "ambiguity", -1);
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 0, pixbuf, 1, "American Party", -1);
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 0, pixbuf, 1, "American mountain ash", -1);
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 0, pixbuf, 1, "amelioration", -1);
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 0, pixbuf, 1, "Amelia Earhart", -1);
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 0, pixbuf, 1, "Totten trust", -1);
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 0, pixbuf, 1, "Laminated arch", -1);
 
  return CTK_TREE_MODEL (store);
}

static gboolean
match_func (CtkEntryCompletion *completion,
	    const gchar        *key,
	    CtkTreeIter        *iter,
	    gpointer            user_data)
{
  gchar *item = NULL;
  CtkTreeModel *model;

  gboolean ret = FALSE;

  model = ctk_entry_completion_get_model (completion);

  ctk_tree_model_get (model, iter, 1, &item, -1);

  if (item != NULL)
    {
      g_print ("compare %s %s\n", key, item);
      if (strncmp (key, item, strlen (key)) == 0)
	ret = TRUE;

      g_free (item);
    }

  return ret;
}

static void
activated_cb (CtkEntryCompletion *completion, 
	      gint                index,
	      gpointer            user_data)
{
  g_print ("action activated: %d\n", index);
}

static gint timer_count = 0;

static gchar *dynamic_completions[] = {
  "GNOME",
  "gnominious",
  "Gnomonic projection",
  "total",
  "totally",
  "toto",
  "tottery",
  "totterer",
  "Totten trust",
  "totipotent",
  "totipotency",
  "totemism",
  "totem pole",
  "Totara",
  "totalizer",
  "totalizator",
  "totalitarianism",
  "total parenteral nutrition",
  "total hysterectomy",
  "total eclipse",
  "Totipresence",
  "Totipalmi",
  "zombie"
};

static gint
animation_timer (CtkEntryCompletion *completion)
{
  CtkTreeIter iter;
  gint n_completions = G_N_ELEMENTS (dynamic_completions);
  gint n;
  static CtkListStore *old_store = NULL;
  CtkListStore *store = CTK_LIST_STORE (ctk_entry_completion_get_model (completion));

  if (timer_count % 10 == 0)
    {
      if (!old_store)
	{
	  g_print ("removing model!\n");

	  old_store = CTK_LIST_STORE (g_object_ref (store));
	  ctk_entry_completion_set_model (completion, NULL);
	}
      else
	{
	  g_print ("readding model!\n");
	  
	  ctk_entry_completion_set_model (completion, CTK_TREE_MODEL (old_store));
	  g_object_unref (old_store);
	  old_store = NULL;
	}

      timer_count ++;
      return TRUE;
    }

  if (!old_store)
    {
      if ((timer_count / n_completions) % 2 == 0)
	{
	  n = timer_count % n_completions;
	  ctk_list_store_append (store, &iter);
	  ctk_list_store_set (store, &iter, 0, dynamic_completions[n], -1);
	  
	}
      else
	{
	  if (ctk_tree_model_get_iter_first (CTK_TREE_MODEL (store), &iter))
	    ctk_list_store_remove (store, &iter);
	}
    }
  
  timer_count++;
  return TRUE;
}

gboolean 
match_selected_cb (CtkEntryCompletion *completion,
		   CtkTreeModel       *model,
		   CtkTreeIter        *iter)
{
  gchar *str;
  CtkWidget *entry;

  entry = ctk_entry_completion_get_entry (completion);
  ctk_tree_model_get (CTK_TREE_MODEL (model), iter, 1, &str, -1);
  ctk_entry_set_text (CTK_ENTRY (entry), str);
  ctk_editable_set_position (CTK_EDITABLE (entry), -1);
  g_free (str);

  return TRUE;
}

int 
main (int argc, char *argv[])
{
  CtkWidget *vbox;
  CtkWidget *label;
  CtkWidget *entry;
  CtkEntryCompletion *completion;
  CtkTreeModel *completion_model;
  CtkCellRenderer *cell;

  ctk_init (&argc, &argv);

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_container_set_border_width (CTK_CONTAINER (window), 5);
  g_signal_connect (window, "delete_event", ctk_main_quit, NULL);
  
  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 2);
  ctk_container_add (CTK_CONTAINER (window), vbox);
    
  ctk_container_set_border_width (CTK_CONTAINER (vbox), 5);
  
  label = ctk_label_new (NULL);

  ctk_label_set_markup (CTK_LABEL (label), "Completion demo, try writing <b>total</b> or <b>gnome</b> for example.");
  ctk_box_pack_start (CTK_BOX (vbox), label, FALSE, FALSE, 0);

  /* Create our first entry */
  entry = ctk_entry_new ();
  
  /* Create the completion object */
  completion = ctk_entry_completion_new ();
  ctk_entry_completion_set_inline_completion (completion, TRUE);
  
  /* Assign the completion to the entry */
  ctk_entry_set_completion (CTK_ENTRY (entry), completion);
  g_object_unref (completion);
  
  ctk_container_add (CTK_CONTAINER (vbox), entry);

  /* Create a tree model and use it as the completion model */
  completion_model = create_simple_completion_model ();
  ctk_entry_completion_set_model (completion, completion_model);
  g_object_unref (completion_model);
  
  /* Use model column 0 as the text column */
  ctk_entry_completion_set_text_column (completion, 0);

  /* Create our second entry */
  entry = ctk_entry_new ();

  /* Create the completion object */
  completion = ctk_entry_completion_new ();
  
  /* Assign the completion to the entry */
  ctk_entry_set_completion (CTK_ENTRY (entry), completion);
  g_object_unref (completion);
  
  ctk_container_add (CTK_CONTAINER (vbox), entry);

  /* Create a tree model and use it as the completion model */
  completion_model = create_completion_model ();
  ctk_entry_completion_set_model (completion, completion_model);
  ctk_entry_completion_set_minimum_key_length (completion, 2);
  g_object_unref (completion_model);
  
  /* Use model column 1 as the text column */
  cell = ctk_cell_renderer_pixbuf_new ();
  ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (completion), cell, FALSE);
  ctk_cell_layout_set_attributes (CTK_CELL_LAYOUT (completion), cell, 
				  "pixbuf", 0, NULL); 

  cell = ctk_cell_renderer_text_new ();
  ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (completion), cell, FALSE);
  ctk_cell_layout_set_attributes (CTK_CELL_LAYOUT (completion), cell, 
				  "text", 1, NULL); 
  
  ctk_entry_completion_set_match_func (completion, match_func, NULL, NULL);
  g_signal_connect (completion, "match-selected", 
		    G_CALLBACK (match_selected_cb), NULL);

  ctk_entry_completion_insert_action_text (completion, 100, "action!");
  ctk_entry_completion_insert_action_text (completion, 101, "'nother action!");
  g_signal_connect (completion, "action_activated", G_CALLBACK (activated_cb), NULL);

  /* Create our third entry */
  entry = ctk_entry_new ();

  /* Create the completion object */
  completion = ctk_entry_completion_new ();
  
  /* Assign the completion to the entry */
  ctk_entry_set_completion (CTK_ENTRY (entry), completion);
  g_object_unref (completion);
  
  ctk_container_add (CTK_CONTAINER (vbox), entry);

  /* Create a tree model and use it as the completion model */
  completion_model = CTK_TREE_MODEL (ctk_list_store_new (1, G_TYPE_STRING));

  ctk_entry_completion_set_model (completion, completion_model);
  g_object_unref (completion_model);

  /* Use model column 0 as the text column */
  ctk_entry_completion_set_text_column (completion, 0);

  /* Fill the completion dynamically */
  cdk_threads_add_timeout (1000, (GSourceFunc) animation_timer, completion);

  /* Fourth entry */
  ctk_box_pack_start (CTK_BOX (vbox), ctk_label_new ("Model-less entry completion"), FALSE, FALSE, 0);

  entry = ctk_entry_new ();

  /* Create the completion object */
  completion = ctk_entry_completion_new ();
  
  /* Assign the completion to the entry */
  ctk_entry_set_completion (CTK_ENTRY (entry), completion);
  g_object_unref (completion);
  
  ctk_container_add (CTK_CONTAINER (vbox), entry);

  ctk_widget_show_all (window);

  ctk_main ();
  
  return 0;
}


