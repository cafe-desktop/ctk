/* ctkcellrendereraccel.h
 * Copyright (C) 2000  Red Hat, Inc.,  Jonathan Blandford <jrb@redhat.com>
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
#include <cdk/cdkkeysyms.h>

static void
accel_edited_callback (CtkCellRendererText *cell,
                       const char          *path_string,
                       guint                keyval,
                       CdkModifierType      mask,
                       guint                hardware_keycode,
                       gpointer             data)
{
  CtkTreeModel *model = (CtkTreeModel *)data;
  CtkTreePath *path = ctk_tree_path_new_from_string (path_string);
  CtkTreeIter iter;

  ctk_tree_model_get_iter (model, &iter, path);

  g_print ("%u %d %u\n", keyval, mask, hardware_keycode);
  
  ctk_list_store_set (CTK_LIST_STORE (model), &iter,
		      0, (gint)mask,
		      1, keyval,
		      2, hardware_keycode,
		      -1);
  ctk_tree_path_free (path);
}

static void
accel_cleared_callback (CtkCellRendererText *cell,
                        const char          *path_string,
                        gpointer             data)
{
  CtkTreeModel *model = (CtkTreeModel *)data;
  CtkTreePath *path;
  CtkTreeIter iter;

  path = ctk_tree_path_new_from_string (path_string);
  ctk_tree_model_get_iter (model, &iter, path);
  ctk_list_store_set (CTK_LIST_STORE (model), &iter, 0, 0, 1, 0, 2, 0, -1);
  ctk_tree_path_free (path);
}
static CtkWidget *
key_test (void)
{
	CtkWidget *window, *sw, *tv;
	CtkListStore *store;
	CtkTreeViewColumn *column;
	CtkCellRenderer *rend;
	gint i;
        CtkWidget *box, *entry;

	/* create window */
	window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
        ctk_window_set_default_size (CTK_WINDOW (window), 400, 400);

	sw = ctk_scrolled_window_new (NULL, NULL);
        box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 10);
        ctk_widget_show (box);
	ctk_container_add (CTK_CONTAINER (window), box);
        ctk_box_pack_start (CTK_BOX (box), sw, TRUE, TRUE, 0);

	store = ctk_list_store_new (3, G_TYPE_INT, G_TYPE_UINT, G_TYPE_UINT);
	tv = ctk_tree_view_new_with_model (CTK_TREE_MODEL (store));
	ctk_container_add (CTK_CONTAINER (sw), tv);
	column = ctk_tree_view_column_new ();
	rend = ctk_cell_renderer_accel_new ();
	g_object_set (G_OBJECT (rend), 
		      "accel-mode", CTK_CELL_RENDERER_ACCEL_MODE_CTK, 
                      "editable", TRUE, 
		      NULL);
	g_signal_connect (G_OBJECT (rend),
			  "accel-edited",
			  G_CALLBACK (accel_edited_callback),
			  store);
	g_signal_connect (G_OBJECT (rend),
			  "accel-cleared",
			  G_CALLBACK (accel_cleared_callback),
			  store);

	ctk_tree_view_column_pack_start (column, rend,
					 TRUE);
	ctk_tree_view_column_set_attributes (column, rend,
					     "accel-mods", 0,
					     "accel-key", 1,
					     "keycode", 2,
					     NULL);
	ctk_tree_view_append_column (CTK_TREE_VIEW (tv), column);

	for (i = 0; i < 10; i++) {
		CtkTreeIter iter;

		ctk_list_store_append (store, &iter);
	}

        entry = ctk_entry_new ();
        ctk_widget_show (entry);
        ctk_container_add (CTK_CONTAINER (box), entry);
 
	return window;
}

gint
main (gint argc, gchar **argv)
{
  CtkWidget *dialog;
  
  ctk_init (&argc, &argv);

  dialog = key_test ();

  ctk_widget_show_all (dialog);

  ctk_main ();

  return 0;
}
