/* testiconview.c
 * Copyright (C) 2002  Anders Carlsson <andersca@gnu.org>
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

#include <ctk/ctk.h>
#include <sys/types.h>
#include <string.h>

#define NUMBER_OF_ITEMS   10
#define SOME_ITEMS       100
#define MANY_ITEMS     10000

static void
fill_model (CtkTreeModel *model)
{
  GdkPixbuf *pixbuf;
  int i;
  char *str, *str2;
  CtkTreeIter iter;
  CtkListStore *store = CTK_LIST_STORE (model);
  gint32 size;
  
  pixbuf = cdk_pixbuf_new_from_file ("gnome-textfile.png", NULL);

  i = 0;
  
  ctk_list_store_prepend (store, &iter);

  ctk_list_store_set (store, &iter,
		      0, pixbuf,
		      1, "Really really\nreally really loooooooooong item name",
		      2, 0,
		      3, "This is a <b>Test</b> of <i>markup</i>",
		      4, TRUE,
		      -1);

  while (i < NUMBER_OF_ITEMS - 1)
    {
      GdkPixbuf *pb;
      size = g_random_int_range (20, 70);
      pb = cdk_pixbuf_scale_simple (pixbuf, size, size, GDK_INTERP_NEAREST);

      str = g_strdup_printf ("Icon %d", i);
      str2 = g_strdup_printf ("Icon <b>%d</b>", i);	
      ctk_list_store_prepend (store, &iter);
      ctk_list_store_set (store, &iter,
			  0, pb,
			  1, str,
			  2, i,
			  3, str2,
			  4, TRUE,
			  -1);
      g_free (str);
      g_free (str2);
      i++;
    }
  
  //  ctk_tree_sortable_set_sort_column_id (CTK_TREE_SORTABLE (store), 2, CTK_SORT_ASCENDING);
}

static CtkTreeModel *
create_model (void)
{
  CtkListStore *store;
  
  store = ctk_list_store_new (5, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING, G_TYPE_BOOLEAN);

  return CTK_TREE_MODEL (store);
}


static void
foreach_selected_remove (CtkWidget *button, CtkIconView *icon_list)
{
  CtkTreeIter iter;
  CtkTreeModel *model;

  GList *list, *selected;

  selected = ctk_icon_view_get_selected_items (icon_list);
  model = ctk_icon_view_get_model (icon_list);
  
  for (list = selected; list; list = list->next)
    {
      CtkTreePath *path = list->data;

      ctk_tree_model_get_iter (model, &iter, path);
      ctk_list_store_remove (CTK_LIST_STORE (model), &iter);
      
      ctk_tree_path_free (path);
    } 
  
  g_list_free (selected);
}


static void
swap_rows (CtkWidget *button, CtkIconView *icon_list)
{
  CtkTreeIter iter, iter2;
  CtkTreeModel *model;

  model = ctk_icon_view_get_model (icon_list);
  ctk_tree_sortable_set_sort_column_id (CTK_TREE_SORTABLE (model), -2, CTK_SORT_ASCENDING);

  ctk_tree_model_get_iter_first (model, &iter);
  iter2 = iter;
  ctk_tree_model_iter_next (model, &iter2);
  ctk_list_store_swap (CTK_LIST_STORE (model), &iter, &iter2);
}

static void
add_n_items (CtkIconView *icon_list, gint n)
{
  static gint count = NUMBER_OF_ITEMS;

  CtkTreeIter iter;
  CtkListStore *store;
  GdkPixbuf *pixbuf;
  gchar *str, *str2;
  gint i;

  store = CTK_LIST_STORE (ctk_icon_view_get_model (icon_list));
  pixbuf = cdk_pixbuf_new_from_file ("gnome-textfile.png", NULL);


  for (i = 0; i < n; i++)
    {
      str = g_strdup_printf ("Icon %d", count);
      str2 = g_strdup_printf ("Icon <b>%d</b>", count);	
      ctk_list_store_prepend (store, &iter);
      ctk_list_store_set (store, &iter,
			  0, pixbuf,
			  1, str,
			  2, i,
			  3, str2,
			  -1);
      g_free (str);
      g_free (str2);
      count++;
    }
}

static void
add_some (CtkWidget *button, CtkIconView *icon_list)
{
  add_n_items (icon_list, SOME_ITEMS);
}

static void
add_many (CtkWidget *button, CtkIconView *icon_list)
{
  add_n_items (icon_list, MANY_ITEMS);
}

static void
add_large (CtkWidget *button, CtkIconView *icon_list)
{
  CtkListStore *store;
  CtkTreeIter iter;

  GdkPixbuf *pixbuf, *pb;
  gchar *str;

  store = CTK_LIST_STORE (ctk_icon_view_get_model (icon_list));
  pixbuf = cdk_pixbuf_new_from_file ("gnome-textfile.png", NULL);

  pb = cdk_pixbuf_scale_simple (pixbuf, 
				2 * cdk_pixbuf_get_width (pixbuf),
				2 * cdk_pixbuf_get_height (pixbuf),
				GDK_INTERP_BILINEAR);

  str = g_strdup_printf ("Some really long text");
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter,
		      0, pb,
		      1, str,
		      2, 0,
		      3, str,
		      -1);
  g_object_unref (pb);
  g_free (str);
  
  pb = cdk_pixbuf_scale_simple (pixbuf, 
				3 * cdk_pixbuf_get_width (pixbuf),
				3 * cdk_pixbuf_get_height (pixbuf),
				GDK_INTERP_BILINEAR);

  str = g_strdup ("see how long text behaves when placed underneath "
		  "an oversized icon which would allow for long lines");
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter,
		      0, pb,
		      1, str,
		      2, 1,
		      3, str,
		      -1);
  g_object_unref (pb);
  g_free (str);

  pb = cdk_pixbuf_scale_simple (pixbuf, 
				3 * cdk_pixbuf_get_width (pixbuf),
				3 * cdk_pixbuf_get_height (pixbuf),
				GDK_INTERP_BILINEAR);

  str = g_strdup ("short text");
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter,
		      0, pb,
		      1, str,
		      2, 2,
		      3, str,
		      -1);
  g_object_unref (pb);
  g_free (str);

  g_object_unref (pixbuf);
}

static void
select_all (CtkWidget *button, CtkIconView *icon_list)
{
  ctk_icon_view_select_all (icon_list);
}

static void
select_nonexisting (CtkWidget *button, CtkIconView *icon_list)
{  
  CtkTreePath *path = ctk_tree_path_new_from_indices (999999, -1);
  ctk_icon_view_select_path (icon_list, path);
  ctk_tree_path_free (path);
}

static void
unselect_all (CtkWidget *button, CtkIconView *icon_list)
{
  ctk_icon_view_unselect_all (icon_list);
}

static void
selection_changed (CtkIconView *icon_list)
{
  g_print ("Selection changed!\n");
}

typedef struct {
  CtkIconView     *icon_list;
  CtkTreePath     *path;
} ItemData;

static void
free_item_data (ItemData *data)
{
  ctk_tree_path_free (data->path);
  g_free (data);
}

static void
item_activated (CtkIconView *icon_view,
		CtkTreePath *path)
{
  CtkTreeIter iter;
  CtkTreeModel *model;
  gchar *text;

  model = ctk_icon_view_get_model (icon_view);
  ctk_tree_model_get_iter (model, &iter, path);

  ctk_tree_model_get (model, &iter, 1, &text, -1);
  g_print ("Item activated, text is %s\n", text);
  g_free (text);
  
}

static void
toggled (CtkCellRendererToggle *cell,
	 gchar                 *path_string,
	 gpointer               data)
{
  CtkTreeModel *model = CTK_TREE_MODEL (data);
  CtkTreeIter iter;
  CtkTreePath *path = ctk_tree_path_new_from_string (path_string);
  gboolean value;

  ctk_tree_model_get_iter (model, &iter, path);
  ctk_tree_model_get (model, &iter, 4, &value, -1);

  value = !value;
  ctk_list_store_set (CTK_LIST_STORE (model), &iter, 4, value, -1);

  ctk_tree_path_free (path);
}

static void
edited (CtkCellRendererText *cell,
	gchar               *path_string,
	gchar               *new_text,
	gpointer             data)
{
  CtkTreeModel *model = CTK_TREE_MODEL (data);
  CtkTreeIter iter;
  CtkTreePath *path = ctk_tree_path_new_from_string (path_string);

  ctk_tree_model_get_iter (model, &iter, path);
  ctk_list_store_set (CTK_LIST_STORE (model), &iter, 1, new_text, -1);

  ctk_tree_path_free (path);
}

static void
item_cb (CtkWidget *menuitem,
	 ItemData  *data)
{
  item_activated (data->icon_list, data->path);
}

static void
do_popup_menu (CtkWidget      *icon_list, 
	       GdkEventButton *event)
{
  CtkIconView *icon_view = CTK_ICON_VIEW (icon_list); 
  CtkWidget *menu;
  CtkWidget *menuitem;
  CtkTreePath *path = NULL;
  int button, event_time;
  ItemData *data;
  GList *list;

  if (event)
    path = ctk_icon_view_get_path_at_pos (icon_view, event->x, event->y);
  else
    {
      list = ctk_icon_view_get_selected_items (icon_view);

      if (list)
        {
          path = (CtkTreePath*)list->data;
          g_list_free_full (list, (GDestroyNotify) ctk_tree_path_free);
        }
    }

  if (!path)
    return;

  menu = ctk_menu_new ();

  data = g_new0 (ItemData, 1);
  data->icon_list = icon_view;
  data->path = path;
  g_object_set_data_full (G_OBJECT (menu), "item-path", data, (GDestroyNotify)free_item_data);

  menuitem = ctk_menu_item_new_with_label ("Activate");
  ctk_widget_show (menuitem);
  ctk_menu_shell_append (CTK_MENU_SHELL (menu), menuitem);
  g_signal_connect (menuitem, "activate", G_CALLBACK (item_cb), data);

  if (event)
    {
      button = event->button;
      event_time = event->time;
    }
  else
    {
      button = 0;
      event_time = ctk_get_current_event_time ();
    }

  ctk_menu_popup (CTK_MENU (menu), NULL, NULL, NULL, NULL, 
                  button, event_time);
}
	

static gboolean
button_press_event_handler (CtkWidget      *widget, 
			    GdkEventButton *event)
{
  /* Ignore double-clicks and triple-clicks */
  if (cdk_event_triggers_context_menu ((GdkEvent *) event) &&
      event->type == GDK_BUTTON_PRESS)
    {
      do_popup_menu (widget, event);
      return TRUE;
    }

  return FALSE;
}

static gboolean
popup_menu_handler (CtkWidget *widget)
{
  do_popup_menu (widget, NULL);
  return TRUE;
}

static const CtkTargetEntry item_targets[] = {
  { "CTK_TREE_MODEL_ROW", CTK_TARGET_SAME_APP, 0 }
};
	
gint
main (gint argc, gchar **argv)
{
  CtkWidget *paned, *tv;
  CtkWidget *window, *icon_list, *scrolled_window;
  CtkWidget *vbox, *bbox;
  CtkWidget *button;
  CtkTreeModel *model;
  CtkCellRenderer *cell;
  CtkTreeViewColumn *tvc;
  
  ctk_init (&argc, &argv);

  /* to test rtl layout, set RTL=1 in the environment */
  if (g_getenv ("RTL"))
    ctk_widget_set_default_direction (CTK_TEXT_DIR_RTL);

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_default_size (CTK_WINDOW (window), 700, 400);

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  ctk_container_add (CTK_CONTAINER (window), vbox);

  paned = ctk_paned_new (CTK_ORIENTATION_HORIZONTAL);
  ctk_box_pack_start (CTK_BOX (vbox), paned, TRUE, TRUE, 0);

  icon_list = ctk_icon_view_new ();
  ctk_icon_view_set_selection_mode (CTK_ICON_VIEW (icon_list), CTK_SELECTION_MULTIPLE);

  tv = ctk_tree_view_new ();
  tvc = ctk_tree_view_column_new ();
  ctk_tree_view_append_column (CTK_TREE_VIEW (tv), tvc);

  g_signal_connect_after (icon_list, "button_press_event",
			  G_CALLBACK (button_press_event_handler), NULL);
  g_signal_connect (icon_list, "selection_changed",
		    G_CALLBACK (selection_changed), NULL);
  g_signal_connect (icon_list, "popup_menu",
		    G_CALLBACK (popup_menu_handler), NULL);

  g_signal_connect (icon_list, "item_activated",
		    G_CALLBACK (item_activated), NULL);
  
  model = create_model ();
  ctk_icon_view_set_model (CTK_ICON_VIEW (icon_list), model);
  ctk_tree_view_set_model (CTK_TREE_VIEW (tv), model);
  fill_model (model);

#if 0

  ctk_icon_view_set_pixbuf_column (CTK_ICON_VIEW (icon_list), 0);
  ctk_icon_view_set_text_column (CTK_ICON_VIEW (icon_list), 1);

#else

  cell = ctk_cell_renderer_toggle_new ();
  ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (icon_list), cell, FALSE);
  g_object_set (cell, "activatable", TRUE, NULL);
  ctk_cell_layout_set_attributes (CTK_CELL_LAYOUT (icon_list),
				  cell, "active", 4, NULL);
  g_signal_connect (cell, "toggled", G_CALLBACK (toggled), model);

  cell = ctk_cell_renderer_pixbuf_new ();
  ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (icon_list), cell, FALSE);
  g_object_set (cell, 
		"follow-state", TRUE, 
		NULL);
  ctk_cell_layout_set_attributes (CTK_CELL_LAYOUT (icon_list),
				  cell, "pixbuf", 0, NULL);

  cell = ctk_cell_renderer_text_new ();
  ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (icon_list), cell, FALSE);
  g_object_set (cell, 
		"editable", TRUE, 
		"xalign", 0.5,
		"wrap-mode", PANGO_WRAP_WORD_CHAR,
		"wrap-width", 100,
		NULL);
  ctk_cell_layout_set_attributes (CTK_CELL_LAYOUT (icon_list),
				  cell, "text", 1, NULL);
  g_signal_connect (cell, "edited", G_CALLBACK (edited), model);

  /* now the tree view... */
  cell = ctk_cell_renderer_toggle_new ();
  ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (tvc), cell, FALSE);
  g_object_set (cell, "activatable", TRUE, NULL);
  ctk_cell_layout_set_attributes (CTK_CELL_LAYOUT (tvc),
				  cell, "active", 4, NULL);
  g_signal_connect (cell, "toggled", G_CALLBACK (toggled), model);

  cell = ctk_cell_renderer_pixbuf_new ();
  ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (tvc), cell, FALSE);
  g_object_set (cell, 
		"follow-state", TRUE, 
		NULL);
  ctk_cell_layout_set_attributes (CTK_CELL_LAYOUT (tvc),
				  cell, "pixbuf", 0, NULL);

  cell = ctk_cell_renderer_text_new ();
  ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (tvc), cell, FALSE);
  g_object_set (cell, "editable", TRUE, NULL);
  ctk_cell_layout_set_attributes (CTK_CELL_LAYOUT (tvc),
				  cell, "text", 1, NULL);
  g_signal_connect (cell, "edited", G_CALLBACK (edited), model);
#endif
  /* Allow DND between the icon view and the tree view */
  
  ctk_icon_view_enable_model_drag_source (CTK_ICON_VIEW (icon_list),
					  GDK_BUTTON1_MASK,
					  item_targets,
					  G_N_ELEMENTS (item_targets),
					  GDK_ACTION_MOVE);
  ctk_icon_view_enable_model_drag_dest (CTK_ICON_VIEW (icon_list),
					item_targets,
					G_N_ELEMENTS (item_targets),
					GDK_ACTION_MOVE);

  ctk_tree_view_enable_model_drag_source (CTK_TREE_VIEW (tv),
					  GDK_BUTTON1_MASK,
					  item_targets,
					  G_N_ELEMENTS (item_targets),
					  GDK_ACTION_MOVE);
  ctk_tree_view_enable_model_drag_dest (CTK_TREE_VIEW (tv),
					item_targets,
					G_N_ELEMENTS (item_targets),
					GDK_ACTION_MOVE);

			      
  scrolled_window = ctk_scrolled_window_new (NULL, NULL);
  ctk_container_add (CTK_CONTAINER (scrolled_window), icon_list);
  ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (scrolled_window),
  				  CTK_POLICY_AUTOMATIC, CTK_POLICY_AUTOMATIC);

  ctk_paned_add1 (CTK_PANED (paned), scrolled_window);

  scrolled_window = ctk_scrolled_window_new (NULL, NULL);
  ctk_container_add (CTK_CONTAINER (scrolled_window), tv);
  ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (scrolled_window),
  				  CTK_POLICY_AUTOMATIC, CTK_POLICY_AUTOMATIC);

  ctk_paned_add2 (CTK_PANED (paned), scrolled_window);

  bbox = ctk_button_box_new (CTK_ORIENTATION_HORIZONTAL);
  ctk_button_box_set_layout (CTK_BUTTON_BOX (bbox), CTK_BUTTONBOX_START);
  ctk_box_pack_start (CTK_BOX (vbox), bbox, FALSE, FALSE, 0);

  button = ctk_button_new_with_label ("Add some");
  g_signal_connect (button, "clicked", G_CALLBACK (add_some), icon_list);
  ctk_box_pack_start (CTK_BOX (bbox), button, TRUE, TRUE, 0);

  button = ctk_button_new_with_label ("Add many");
  g_signal_connect (button, "clicked", G_CALLBACK (add_many), icon_list);
  ctk_box_pack_start (CTK_BOX (bbox), button, TRUE, TRUE, 0);

  button = ctk_button_new_with_label ("Add large");
  g_signal_connect (button, "clicked", G_CALLBACK (add_large), icon_list);
  ctk_box_pack_start (CTK_BOX (bbox), button, TRUE, TRUE, 0);

  button = ctk_button_new_with_label ("Remove selected");
  g_signal_connect (button, "clicked", G_CALLBACK (foreach_selected_remove), icon_list);
  ctk_box_pack_start (CTK_BOX (bbox), button, TRUE, TRUE, 0);

  button = ctk_button_new_with_label ("Swap");
  g_signal_connect (button, "clicked", G_CALLBACK (swap_rows), icon_list);
  ctk_box_pack_start (CTK_BOX (bbox), button, TRUE, TRUE, 0);

  bbox = ctk_button_box_new (CTK_ORIENTATION_HORIZONTAL);
  ctk_button_box_set_layout (CTK_BUTTON_BOX (bbox), CTK_BUTTONBOX_START);
  ctk_box_pack_start (CTK_BOX (vbox), bbox, FALSE, FALSE, 0);

  button = ctk_button_new_with_label ("Select all");
  g_signal_connect (button, "clicked", G_CALLBACK (select_all), icon_list);
  ctk_box_pack_start (CTK_BOX (bbox), button, TRUE, TRUE, 0);

  button = ctk_button_new_with_label ("Unselect all");
  g_signal_connect (button, "clicked", G_CALLBACK (unselect_all), icon_list);
  ctk_box_pack_start (CTK_BOX (bbox), button, TRUE, TRUE, 0);

  button = ctk_button_new_with_label ("Select nonexisting");
  g_signal_connect (button, "clicked", G_CALLBACK (select_nonexisting), icon_list);
  ctk_box_pack_start (CTK_BOX (bbox), button, TRUE, TRUE, 0);

  icon_list = ctk_icon_view_new ();

  scrolled_window = ctk_scrolled_window_new (NULL, NULL);
  ctk_container_add (CTK_CONTAINER (scrolled_window), icon_list);
  ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (scrolled_window),
				  CTK_POLICY_AUTOMATIC, CTK_POLICY_AUTOMATIC);
  ctk_paned_add2 (CTK_PANED (paned), scrolled_window);

  ctk_widget_show_all (window);

  ctk_main ();

  return 0;
}
