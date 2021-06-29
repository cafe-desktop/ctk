/* testcombo.c
 * Copyright (C) 2003  Kristian Rietveld
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

#include <string.h>
#include <stdio.h>

/**
 * oh yes, this test app surely has a lot of ugly code
 */

/* grid combo demo */
static CdkPixbuf *
create_color_pixbuf (const char *color)
{
        CdkPixbuf *pixbuf;
        CdkRGBA rgba;

        int x;
        int num;
        guchar *pixels, *p;

        if (!cdk_rgba_parse (&rgba, color))
                return NULL;

        pixbuf = cdk_pixbuf_new (GDK_COLORSPACE_RGB,
                                 FALSE, 8,
                                 16, 16);

        p = pixels = cdk_pixbuf_get_pixels (pixbuf);

        num = cdk_pixbuf_get_width (pixbuf) *
                cdk_pixbuf_get_height (pixbuf);

        for (x = 0; x < num; x++) {
                p[0] = rgba.red * 255;
                p[1] = rgba.green * 255;
                p[2] = rgba.blue * 255;
                p += 3;
        }

        return pixbuf;
}

static CtkWidget *
create_combo_box_grid_demo (void)
{
        CtkWidget *combo;
        CtkTreeIter iter;
        CdkPixbuf *pixbuf;
        CtkCellRenderer *cell = ctk_cell_renderer_pixbuf_new ();
        CtkListStore *store;

        store = ctk_list_store_new (3, GDK_TYPE_PIXBUF, G_TYPE_INT, G_TYPE_INT);

        /* first row */
        pixbuf = create_color_pixbuf ("red");
        ctk_list_store_append (store, &iter);
        ctk_list_store_set (store, &iter,
                            0, pixbuf,
                            1, 1, /* row span */
                            2, 1, /* column span */
                            -1);
        g_object_unref (pixbuf);

        pixbuf = create_color_pixbuf ("green");
        ctk_list_store_append (store, &iter);
        ctk_list_store_set (store, &iter,
                            0, pixbuf,
                            1, 1,
                            2, 1,
                            -1);
        g_object_unref (pixbuf);

        pixbuf = create_color_pixbuf ("blue");
        ctk_list_store_append (store, &iter);
        ctk_list_store_set (store, &iter,
                            0, pixbuf,
                            1, 1,
                            2, 1,
                            -1);
        g_object_unref (pixbuf);

        /* second row */
        pixbuf = create_color_pixbuf ("yellow");
        ctk_list_store_append (store, &iter);
        ctk_list_store_set (store, &iter,
                            0, pixbuf,
                            1, 1,
                            2, 2, /* Span 2 columns */
                            -1);
        g_object_unref (pixbuf);

        pixbuf = create_color_pixbuf ("black");
        ctk_list_store_append (store, &iter);
        ctk_list_store_set (store, &iter,
                            0, pixbuf,
                            1, 2, /* Span 2 rows */
                            2, 1,
                            -1);
        g_object_unref (pixbuf);

        /* third row */
        pixbuf = create_color_pixbuf ("gray");
        ctk_list_store_append (store, &iter);
        ctk_list_store_set (store, &iter,
                            0, pixbuf,
                            1, 1,
                            2, 1,
                            -1);
        g_object_unref (pixbuf);

        pixbuf = create_color_pixbuf ("magenta");
        ctk_list_store_append (store, &iter);
        ctk_list_store_set (store, &iter,
                            0, pixbuf,
                            1, 1,
                            2, 1,
                            -1);
        g_object_unref (pixbuf);

        /* Create ComboBox after model to avoid ctk_menu_attach() warnings(?) */
        combo = ctk_combo_box_new_with_model (CTK_TREE_MODEL (store));
        g_object_unref (store);

        ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (combo),
                                    cell, TRUE);
        ctk_cell_layout_set_attributes (CTK_CELL_LAYOUT (combo),
                                        cell, "pixbuf", 0, NULL);

        /* Set wrap-width != 0 to enforce grid mode */
        ctk_combo_box_set_wrap_width (CTK_COMBO_BOX (combo), 3);
        ctk_combo_box_set_row_span_column (CTK_COMBO_BOX (combo), 1);
        ctk_combo_box_set_column_span_column (CTK_COMBO_BOX (combo), 2);

        ctk_combo_box_set_active (CTK_COMBO_BOX (combo), 0);

        return combo;
}

/* blaat */
static CtkTreeModel *
create_tree_blaat (void)
{
        CtkWidget *cellview;
        CtkTreeIter iter, iter2;
        CtkTreeStore *store;

        cellview = ctk_cell_view_new ();

	store = ctk_tree_store_new (3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN);

        ctk_tree_store_append (store, &iter, NULL);
        ctk_tree_store_set (store, &iter,
                            0, "dialog-warning",
                            1, "dialog-warning",
			    2, FALSE,
                            -1);

        ctk_tree_store_append (store, &iter2, &iter);			       
        ctk_tree_store_set (store, &iter2,
                            0, "process-stop",
                            1, "process-stop",
			    2, FALSE,
                            -1);

        ctk_tree_store_append (store, &iter2, &iter);			       
        ctk_tree_store_set (store, &iter2,
                            0, "document-new",
                            1, "document-new",
			    2, FALSE,
                            -1);

        ctk_tree_store_append (store, &iter, NULL);
        ctk_tree_store_set (store, &iter,
                            0, "edit-clear",
                            1, "edit-clear",
			    2, FALSE,
                            -1);

#if 0
        ctk_tree_store_append (store, &iter, NULL);
        ctk_tree_store_set (store, &iter,
                            0, NULL,
                            1, "separator",
			    2, TRUE,
                            -1);
#endif

        ctk_tree_store_append (store, &iter, NULL);
        ctk_tree_store_set (store, &iter,
                            0, "document-open",
                            1, "document-open",
			    2, FALSE,
                            -1);

        ctk_widget_destroy (cellview);

        return CTK_TREE_MODEL (store);
}

static CtkTreeModel *
create_empty_list_blaat (void)
{
        CtkWidget *cellview;
        CtkTreeIter iter;
        CtkListStore *store;

        cellview = ctk_cell_view_new ();

        store = ctk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);

        ctk_list_store_append (store, &iter);
        ctk_list_store_set (store, &iter,
                            0, "dialog-warning",
                            1, "dialog-warning",
                            -1);

        ctk_widget_destroy (cellview);

        return CTK_TREE_MODEL (store);
}

static void
populate_list_blaat (gpointer data)
{
  CtkComboBox *combo_box = CTK_COMBO_BOX (data);
  CtkListStore *store;
  CtkWidget *cellview;
  CtkTreeIter iter;
  
  store = CTK_LIST_STORE (ctk_combo_box_get_model (combo_box));

  ctk_tree_model_get_iter_first (CTK_TREE_MODEL (store), &iter);

  if (ctk_tree_model_iter_next (CTK_TREE_MODEL (store), &iter))
    return;

  cellview = ctk_cell_view_new ();
  
  ctk_list_store_append (store, &iter);			       
  ctk_list_store_set (store, &iter,
		      0, "process-stop",
		      1, "process-stop",
		      -1);
  
  ctk_list_store_append (store, &iter);			       
  ctk_list_store_set (store, &iter,
		      0, "document-new",
		      1, "document-new",
		      -1);
  
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter,
		      0, "edit-clear",
		      1, "edit-clear",
		      -1);
  
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter,
		      0, NULL,
		      1, "separator",
		      -1);
  
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter,
		      0, "document-open",
		      1, "document-open",
		      -1);
  
  ctk_widget_destroy (cellview);  
}

static CtkTreeModel *
create_list_blaat (void)
{
        CtkWidget *cellview;
        CtkTreeIter iter;
        CtkListStore *store;

        cellview = ctk_cell_view_new ();

        store = ctk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);

        ctk_list_store_append (store, &iter);
        ctk_list_store_set (store, &iter,
                            0, "dialog-warning",
                            1, "dialog-warning",
                            -1);

        ctk_list_store_append (store, &iter);			       
        ctk_list_store_set (store, &iter,
                            0, "process-stop",
                            1, "process-stop",
                            -1);

        ctk_list_store_append (store, &iter);			       
        ctk_list_store_set (store, &iter,
                            0, "document-new",
                            1, "document-new",
                            -1);

        ctk_list_store_append (store, &iter);
        ctk_list_store_set (store, &iter,
                            0, "edit-clear",
                            1, "edit-clear",
                            -1);

        ctk_list_store_append (store, &iter);
        ctk_list_store_set (store, &iter,
                            0, NULL,
                            1, "separator",
                            -1);

        ctk_list_store_append (store, &iter);
        ctk_list_store_set (store, &iter,
                            0, "document-open",
                            1, "document-open",
                            -1);

        ctk_widget_destroy (cellview);

        return CTK_TREE_MODEL (store);
}


static CtkTreeModel *
create_list_long (void)
{
        CtkTreeIter iter;
        CtkListStore *store;

        store = ctk_list_store_new (1, G_TYPE_STRING);

        ctk_list_store_append (store, &iter);
        ctk_list_store_set (store, &iter,
                            0, "here is some long long text that grows out of the combo's allocation",
                            -1);


        ctk_list_store_append (store, &iter);
        ctk_list_store_set (store, &iter,
                            0, "with at least a few of these rows",
                            -1);

        ctk_list_store_append (store, &iter);
        ctk_list_store_set (store, &iter,
                            0, "so that we can get some ellipsized text here",
                            -1);

        ctk_list_store_append (store, &iter);
        ctk_list_store_set (store, &iter,
                            0, "and see the combo box menu being allocated without any constraints",
                            -1);

        return CTK_TREE_MODEL (store);
}

static CtkTreeModel *
create_food_list (void)
{
        CtkTreeIter iter;
        CtkListStore *store;

        store = ctk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
        ctk_list_store_append (store, &iter);
        ctk_list_store_set (store, &iter,
                            0, "Pepperoni",
                            1, "Pizza",
                            -1);

        ctk_list_store_append (store, &iter);			       
        ctk_list_store_set (store, &iter,
                            0, "Cheese",
                            1, "Burger",
                            -1);

        ctk_list_store_append (store, &iter);			       
        ctk_list_store_set (store, &iter,
                            0, "Pineapple",
                            1, "Milkshake",
                            -1);

        ctk_list_store_append (store, &iter);			       
        ctk_list_store_set (store, &iter,
                            0, "Orange",
                            1, "Soda",
                            -1);

        ctk_list_store_append (store, &iter);			       
        ctk_list_store_set (store, &iter,
                            0, "Club",
                            1, "Sandwich",
                            -1);

        return CTK_TREE_MODEL (store);
}


/* blaat */
static CtkTreeModel *
create_phylogenetic_tree (void)
{
        CtkTreeIter iter, iter2, iter3;
        CtkTreeStore *store;

	store = ctk_tree_store_new (1,G_TYPE_STRING);

        ctk_tree_store_append (store, &iter, NULL);
        ctk_tree_store_set (store, &iter,
                            0, "Eubacteria",
                            -1);

        ctk_tree_store_append (store, &iter2, &iter);			       
        ctk_tree_store_set (store, &iter2,
                            0, "Aquifecales",
                            -1);

        ctk_tree_store_append (store, &iter2, &iter);			       
        ctk_tree_store_set (store, &iter2,
                            0, "Thermotogales",
                            -1);

        ctk_tree_store_append (store, &iter2, &iter);			       
        ctk_tree_store_set (store, &iter2,
                            0, "Thermodesulfobacterium",
                            -1);

        ctk_tree_store_append (store, &iter2, &iter);			       
        ctk_tree_store_set (store, &iter2,
                            0, "Thermus-Deinococcus group",
                            -1);

        ctk_tree_store_append (store, &iter2, &iter);			       
        ctk_tree_store_set (store, &iter2,
                            0, "Chloroflecales",
                            -1);

        ctk_tree_store_append (store, &iter2, &iter);			       
        ctk_tree_store_set (store, &iter2,
                            0, "Cyanobacteria",
                            -1);

        ctk_tree_store_append (store, &iter2, &iter);			       
        ctk_tree_store_set (store, &iter2,
                            0, "Firmicutes",
                            -1);

        ctk_tree_store_append (store, &iter2, &iter);			       
        ctk_tree_store_set (store, &iter2,
                            0, "Leptospirillium Group",
                            -1);

        ctk_tree_store_append (store, &iter2, &iter);			       
        ctk_tree_store_set (store, &iter2,
                            0, "Synergistes",
                            -1);
        ctk_tree_store_append (store, &iter2, &iter);			       
        ctk_tree_store_set (store, &iter2,
                            0, "Chlorobium-Flavobacteria group",
                            -1);
        ctk_tree_store_append (store, &iter2, &iter);			       
        ctk_tree_store_set (store, &iter2,
                            0, "Chlamydia-Verrucomicrobia group",
                            -1);

        ctk_tree_store_append (store, &iter3, &iter2);			       
        ctk_tree_store_set (store, &iter3,
                            0, "Verrucomicrobia",
                            -1);
        ctk_tree_store_append (store, &iter3, &iter2);			       
        ctk_tree_store_set (store, &iter3,
                            0, "Chlamydia",
                            -1);

        ctk_tree_store_append (store, &iter2, &iter);			       
        ctk_tree_store_set (store, &iter2,
                            0, "Flexistipes",
                            -1);


        ctk_tree_store_append (store, &iter2, &iter);			       
        ctk_tree_store_set (store, &iter2,
                            0, "Fibrobacter group",
                            -1);


        ctk_tree_store_append (store, &iter2, &iter);			       
        ctk_tree_store_set (store, &iter2,
                            0, "spirocheteus",
                            -1);


        ctk_tree_store_append (store, &iter2, &iter);			       
        ctk_tree_store_set (store, &iter2,
                            0, "Proteobacteria",
                            -1);


        ctk_tree_store_append (store, &iter3, &iter2);			       
        ctk_tree_store_set (store, &iter3,
                            0, "alpha",
                            -1);


        ctk_tree_store_append (store, &iter3, &iter2);			       
        ctk_tree_store_set (store, &iter3,
                            0, "beta",
                            -1);


        ctk_tree_store_append (store, &iter3, &iter2);			       
        ctk_tree_store_set (store, &iter3,
                            0, "delta ",
                            -1);


        ctk_tree_store_append (store, &iter3, &iter2);			       
        ctk_tree_store_set (store, &iter3,
                            0, "epsilon",
                            -1);


        ctk_tree_store_append (store, &iter3, &iter2);  
        ctk_tree_store_set (store, &iter3,
                            0, "gamma ",
                            -1);


        ctk_tree_store_append (store, &iter, NULL);			       
        ctk_tree_store_set (store, &iter,
                            0, "Eukaryotes",
                            -1);


        ctk_tree_store_append (store, &iter2, &iter);			       
        ctk_tree_store_set (store, &iter2,
                            0, "Metazoa",
                            -1);


        ctk_tree_store_append (store, &iter2, &iter);			       
        ctk_tree_store_set (store, &iter2,
                            0, "Bilateria",
                            -1);


        ctk_tree_store_append (store, &iter2, &iter);			       
        ctk_tree_store_set (store, &iter2,
                            0, "Myxozoa",
                            -1);


        ctk_tree_store_append (store, &iter2, &iter);			       
        ctk_tree_store_set (store, &iter2,
                            0, "Cnidaria",
                            -1);


        ctk_tree_store_append (store, &iter2, &iter);			       
        ctk_tree_store_set (store, &iter2,
                            0, "Ctenophora",
                            -1);


        ctk_tree_store_append (store, &iter2, &iter);			       
        ctk_tree_store_set (store, &iter2,
                            0, "Placozoa",
                            -1);


        ctk_tree_store_append (store, &iter2, &iter);			       
        ctk_tree_store_set (store, &iter2,
                            0, "Porifera",
                            -1);


        ctk_tree_store_append (store, &iter2, &iter);			       
        ctk_tree_store_set (store, &iter2,
                            0, "choanoflagellates",
                            -1);


        ctk_tree_store_append (store, &iter2, &iter);			       
        ctk_tree_store_set (store, &iter2,
                            0, "Fungi",
                            -1);


        ctk_tree_store_append (store, &iter2, &iter);			       
        ctk_tree_store_set (store, &iter2,
                            0, "Microsporidia",
                            -1);


        ctk_tree_store_append (store, &iter2, &iter);			       
        ctk_tree_store_set (store, &iter2,
                            0, "Aleveolates",
                            -1);


        ctk_tree_store_append (store, &iter2, &iter);			       
        ctk_tree_store_set (store, &iter2,
                            0, "Stramenopiles",
                            -1);


        ctk_tree_store_append (store, &iter2, &iter);			       
        ctk_tree_store_set (store, &iter2,
                            0, "Rhodophyta",
                            -1);


        ctk_tree_store_append (store, &iter2, &iter);			       
        ctk_tree_store_set (store, &iter2,
                            0, "Viridaeplantae",
                            -1);


        ctk_tree_store_append (store, &iter2, &iter);			       
        ctk_tree_store_set (store, &iter2,
                            0, "crytomonads et al",
                            -1);


        ctk_tree_store_append (store, &iter, NULL);			       
        ctk_tree_store_set (store, &iter,
                            0, "Archaea ",
                            -1);


        ctk_tree_store_append (store, &iter2, &iter);			       
        ctk_tree_store_set (store, &iter2,
                            0, "Korarchaeota",
                            -1);


        ctk_tree_store_append (store, &iter2, &iter);			       
        ctk_tree_store_set (store, &iter2,
                            0, "Crenarchaeota",
                            -1);


        ctk_tree_store_append (store, &iter2, &iter);			       
        ctk_tree_store_set (store, &iter2,
                            0, "Buryarchaeota",
                            -1);

        return CTK_TREE_MODEL (store);
}


/* blaat */
static CtkTreeModel *
create_capital_tree (void)
{
        CtkTreeIter iter, iter2;
        CtkTreeStore *store;

	store = ctk_tree_store_new (1, G_TYPE_STRING);

        ctk_tree_store_append (store, &iter, NULL);
        ctk_tree_store_set (store, &iter, 0, "A - B", -1);

        ctk_tree_store_append (store, &iter2, &iter);
        ctk_tree_store_set (store, &iter2, 0, "Albany", -1);

        ctk_tree_store_append (store, &iter2, &iter);
        ctk_tree_store_set (store, &iter2, 0, "Annapolis", -1);

        ctk_tree_store_append (store, &iter2, &iter);
        ctk_tree_store_set (store, &iter2, 0, "Atlanta", -1);

        ctk_tree_store_append (store, &iter2, &iter);
        ctk_tree_store_set (store, &iter2, 0, "Augusta", -1);

        ctk_tree_store_append (store, &iter2, &iter);
        ctk_tree_store_set (store, &iter2, 0, "Austin", -1);

        ctk_tree_store_append (store, &iter2, &iter);
        ctk_tree_store_set (store, &iter2, 0, "Baton Rouge", -1);

        ctk_tree_store_append (store, &iter2, &iter);
        ctk_tree_store_set (store, &iter2, 0, "Bismarck", -1);

        ctk_tree_store_append (store, &iter2, &iter);
        ctk_tree_store_set (store, &iter2, 0, "Boise", -1);

        ctk_tree_store_append (store, &iter2, &iter);
        ctk_tree_store_set (store, &iter2, 0, "Boston", -1);

        ctk_tree_store_append (store, &iter, NULL);
        ctk_tree_store_set (store, &iter, 0, "C - D", -1);

        ctk_tree_store_append (store, &iter2, &iter);
        ctk_tree_store_set (store, &iter2, 0, "Carson City", -1);

        ctk_tree_store_append (store, &iter2, &iter);
        ctk_tree_store_set (store, &iter2, 0, "Charleston", -1);

        ctk_tree_store_append (store, &iter2, &iter);
        ctk_tree_store_set (store, &iter2, 0, "Cheyenne", -1);

        ctk_tree_store_append (store, &iter2, &iter);
        ctk_tree_store_set (store, &iter2, 0, "Columbia", -1);

        ctk_tree_store_append (store, &iter2, &iter);
        ctk_tree_store_set (store, &iter2, 0, "Columbus", -1);

        ctk_tree_store_append (store, &iter2, &iter);
        ctk_tree_store_set (store, &iter2, 0, "Concord", -1);

        ctk_tree_store_append (store, &iter2, &iter);
        ctk_tree_store_set (store, &iter2, 0, "Denver", -1);

        ctk_tree_store_append (store, &iter2, &iter);
        ctk_tree_store_set (store, &iter2, 0, "Des Moines", -1);

        ctk_tree_store_append (store, &iter2, &iter);
        ctk_tree_store_set (store, &iter2, 0, "Dover", -1);


        ctk_tree_store_append (store, &iter, NULL);
        ctk_tree_store_set (store, &iter, 0, "E - J", -1);

        ctk_tree_store_append (store, &iter2, &iter);
        ctk_tree_store_set (store, &iter2, 0, "Frankfort", -1);

        ctk_tree_store_append (store, &iter2, &iter);
        ctk_tree_store_set (store, &iter2, 0, "Harrisburg", -1);

        ctk_tree_store_append (store, &iter2, &iter);
        ctk_tree_store_set (store, &iter2, 0, "Hartford", -1);

        ctk_tree_store_append (store, &iter2, &iter);
        ctk_tree_store_set (store, &iter2, 0, "Helena", -1);

        ctk_tree_store_append (store, &iter2, &iter);
        ctk_tree_store_set (store, &iter2, 0, "Honolulu", -1);

        ctk_tree_store_append (store, &iter2, &iter);
        ctk_tree_store_set (store, &iter2, 0, "Indianapolis", -1);

        ctk_tree_store_append (store, &iter2, &iter);
        ctk_tree_store_set (store, &iter2, 0, "Jackson", -1);

        ctk_tree_store_append (store, &iter2, &iter);
        ctk_tree_store_set (store, &iter2, 0, "Jefferson City", -1);

        ctk_tree_store_append (store, &iter2, &iter);
        ctk_tree_store_set (store, &iter2, 0, "Juneau", -1);


        ctk_tree_store_append (store, &iter, NULL);
        ctk_tree_store_set (store, &iter, 0, "K - O", -1);

        ctk_tree_store_append (store, &iter2, &iter);
        ctk_tree_store_set (store, &iter2, 0, "Lansing", -1);

        ctk_tree_store_append (store, &iter2, &iter);
        ctk_tree_store_set (store, &iter2, 0, "Lincoln", -1);

        ctk_tree_store_append (store, &iter2, &iter);
        ctk_tree_store_set (store, &iter2, 0, "Little Rock", -1);

        ctk_tree_store_append (store, &iter2, &iter);
        ctk_tree_store_set (store, &iter2, 0, "Madison", -1);

        ctk_tree_store_append (store, &iter2, &iter);
        ctk_tree_store_set (store, &iter2, 0, "Montgomery", -1);

        ctk_tree_store_append (store, &iter2, &iter);
        ctk_tree_store_set (store, &iter2, 0, "Montpelier", -1);

        ctk_tree_store_append (store, &iter2, &iter);
        ctk_tree_store_set (store, &iter2, 0, "Nashville", -1);

        ctk_tree_store_append (store, &iter2, &iter);
        ctk_tree_store_set (store, &iter2, 0, "Oklahoma City", -1);

        ctk_tree_store_append (store, &iter2, &iter);
        ctk_tree_store_set (store, &iter2, 0, "Olympia", -1);


        ctk_tree_store_append (store, &iter, NULL);
        ctk_tree_store_set (store, &iter, 0, "P - S", -1);

        ctk_tree_store_append (store, &iter2, &iter);
        ctk_tree_store_set (store, &iter2, 0, "Phoenix", -1);

        ctk_tree_store_append (store, &iter2, &iter);
        ctk_tree_store_set (store, &iter2, 0, "Pierre", -1);

        ctk_tree_store_append (store, &iter2, &iter);
        ctk_tree_store_set (store, &iter2, 0, "Providence", -1);

        ctk_tree_store_append (store, &iter2, &iter);
        ctk_tree_store_set (store, &iter2, 0, "Raleigh", -1);

        ctk_tree_store_append (store, &iter2, &iter);
        ctk_tree_store_set (store, &iter2, 0, "Richmond", -1);

        ctk_tree_store_append (store, &iter2, &iter);
        ctk_tree_store_set (store, &iter2, 0, "Sacramento", -1);

        ctk_tree_store_append (store, &iter2, &iter);
        ctk_tree_store_set (store, &iter2, 0, "Salem", -1);

        ctk_tree_store_append (store, &iter2, &iter);
        ctk_tree_store_set (store, &iter2, 0, "Salt Lake City", -1);

        ctk_tree_store_append (store, &iter2, &iter);
        ctk_tree_store_set (store, &iter2, 0, "Santa Fe", -1);

        ctk_tree_store_append (store, &iter2, &iter);
        ctk_tree_store_set (store, &iter2, 0, "Springfield", -1);

        ctk_tree_store_append (store, &iter2, &iter);
        ctk_tree_store_set (store, &iter2, 0, "St. Paul", -1);


        ctk_tree_store_append (store, &iter, NULL);
        ctk_tree_store_set (store, &iter, 0, "T - Z", -1);

        ctk_tree_store_append (store, &iter2, &iter);
        ctk_tree_store_set (store, &iter2, 0, "Tallahassee", -1);

        ctk_tree_store_append (store, &iter2, &iter);
        ctk_tree_store_set (store, &iter2, 0, "Topeka", -1);

        ctk_tree_store_append (store, &iter2, &iter);
        ctk_tree_store_set (store, &iter2, 0, "Trenton", -1);

        return CTK_TREE_MODEL (store);
}

static void
capital_sensitive (CtkCellLayout   *cell_layout,
		   CtkCellRenderer *cell,
		   CtkTreeModel    *tree_model,
		   CtkTreeIter     *iter,
		   gpointer         data)
{
  gboolean sensitive;

  sensitive = !ctk_tree_model_iter_has_child (tree_model, iter);

  g_object_set (cell, "sensitive", sensitive, NULL);
}

static gboolean
capital_animation (gpointer data)
{
  static gint insert_count = 0;
  CtkTreeModel *model = CTK_TREE_MODEL (data);
  CtkTreePath *path;
  CtkTreeIter iter, parent;

  switch (insert_count % 8)
    {
    case 0:
      ctk_tree_store_insert (CTK_TREE_STORE (model), &iter, NULL, 0);
      ctk_tree_store_set (CTK_TREE_STORE (model), 
			  &iter,
			  0, "Europe", -1);
      break;

    case 1:
      path = ctk_tree_path_new_from_indices (0, -1);
      ctk_tree_model_get_iter (model, &parent, path);
      ctk_tree_path_free (path);
      ctk_tree_store_insert (CTK_TREE_STORE (model), &iter, &parent, 0);
      ctk_tree_store_set (CTK_TREE_STORE (model), 
			  &iter,
			  0, "Berlin", -1);
      break;

    case 2:
      path = ctk_tree_path_new_from_indices (0, -1);
      ctk_tree_model_get_iter (model, &parent, path);
      ctk_tree_path_free (path);
      ctk_tree_store_insert (CTK_TREE_STORE (model), &iter, &parent, 1);
      ctk_tree_store_set (CTK_TREE_STORE (model), 
			  &iter,
			  0, "London", -1);
      break;

    case 3:
      path = ctk_tree_path_new_from_indices (0, -1);
      ctk_tree_model_get_iter (model, &parent, path);
      ctk_tree_path_free (path);
      ctk_tree_store_insert (CTK_TREE_STORE (model), &iter, &parent, 2);
      ctk_tree_store_set (CTK_TREE_STORE (model), 
			  &iter,
			  0, "Paris", -1);
      break;

    case 4:
      path = ctk_tree_path_new_from_indices (0, 2, -1);
      ctk_tree_model_get_iter (model, &iter, path);
      ctk_tree_path_free (path);
      ctk_tree_store_remove (CTK_TREE_STORE (model), &iter);
      break;

    case 5:
      path = ctk_tree_path_new_from_indices (0, 1, -1);
      ctk_tree_model_get_iter (model, &iter, path);
      ctk_tree_path_free (path);
      ctk_tree_store_remove (CTK_TREE_STORE (model), &iter);
      break;

    case 6:
      path = ctk_tree_path_new_from_indices (0, 0, -1);
      ctk_tree_model_get_iter (model, &iter, path);
      ctk_tree_path_free (path);
      ctk_tree_store_remove (CTK_TREE_STORE (model), &iter);
      break;

    case 7:
      path = ctk_tree_path_new_from_indices (0, -1);
      ctk_tree_model_get_iter (model, &iter, path);
      ctk_tree_path_free (path);
      ctk_tree_store_remove (CTK_TREE_STORE (model), &iter);
      break;

    default: ;

    }
  insert_count++;

  return TRUE;
}

static void
setup_combo_entry (CtkComboBoxText *combo)
{
  ctk_combo_box_text_append_text (combo,
				   "dum de dum");
  ctk_combo_box_text_append_text (combo,
				   "la la la");
  ctk_combo_box_text_append_text (combo,
				   "la la la dum de dum la la la la la la boom de da la la");
  ctk_combo_box_text_append_text (combo,
				   "bloop");
  ctk_combo_box_text_append_text (combo,
				   "bleep");
  ctk_combo_box_text_append_text (combo,
				   "klaas");
  ctk_combo_box_text_append_text (combo,
				   "klaas0");
  ctk_combo_box_text_append_text (combo,
				   "klaas1");
  ctk_combo_box_text_append_text (combo,
				   "klaas2");
  ctk_combo_box_text_append_text (combo,
				   "klaas3");
  ctk_combo_box_text_append_text (combo,
				   "klaas4");
  ctk_combo_box_text_append_text (combo,
				   "klaas5");
  ctk_combo_box_text_append_text (combo,
				   "klaas6");
  ctk_combo_box_text_append_text (combo,
				   "klaas7");
  ctk_combo_box_text_append_text (combo,
				   "klaas8");
  ctk_combo_box_text_append_text (combo,
				   "klaas9");
  ctk_combo_box_text_append_text (combo,
				   "klaasa");
  ctk_combo_box_text_append_text (combo,
				   "klaasb");
  ctk_combo_box_text_append_text (combo,
				   "klaasc");
  ctk_combo_box_text_append_text (combo,
				   "klaasd");
  ctk_combo_box_text_append_text (combo,
				   "klaase");
  ctk_combo_box_text_append_text (combo,
				   "klaasf");
  ctk_combo_box_text_append_text (combo,
				   "klaas10");
  ctk_combo_box_text_append_text (combo,
				   "klaas11");
  ctk_combo_box_text_append_text (combo,
				   "klaas12");
}

static void
set_sensitive (CtkCellLayout   *cell_layout,
	       CtkCellRenderer *cell,
	       CtkTreeModel    *tree_model,
	       CtkTreeIter     *iter,
	       gpointer         data)
{
  CtkTreePath *path;
  gint *indices;
  gboolean sensitive;

  path = ctk_tree_model_get_path (tree_model, iter);
  indices = ctk_tree_path_get_indices (path);
  sensitive = indices[0] != 1;
  ctk_tree_path_free (path);

  g_object_set (cell, "sensitive", sensitive, NULL);
}

static gboolean
is_separator (CtkTreeModel *model,
	      CtkTreeIter  *iter,
	      gpointer      data)
{
  CtkTreePath *path;
  gboolean result;

  path = ctk_tree_model_get_path (model, iter);
  result = ctk_tree_path_get_indices (path)[0] == 4;
  ctk_tree_path_free (path);

  return result;
  
}

static void
displayed_row_changed (CtkComboBox *combo,
                       CtkCellView *cell)
{
  gint row;
  CtkTreePath *path;

  row = ctk_combo_box_get_active (combo);
  path = ctk_tree_path_new_from_indices (row, -1);
  ctk_cell_view_set_displayed_row (cell, path);
  ctk_tree_path_free (path);
}

int
main (int argc, char **argv)
{
        CtkWidget *window, *cellview, *mainbox;
        CtkWidget *combobox, *comboboxtext, *comboboxgrid;
        CtkWidget *tmp, *boom;
        CtkCellRenderer *renderer;
        CtkTreeModel *model;
	CtkTreePath *path;
	CtkTreeIter iter;
	CdkRGBA color;
	CtkCellArea *area;
        gchar *text;
        gint i;

        ctk_init (&argc, &argv);

	if (g_getenv ("RTL"))
	  ctk_widget_set_default_direction (CTK_TEXT_DIR_RTL);

	if (g_getenv ("LISTMODE"))
	  {
	    CtkCssProvider *provider = ctk_css_provider_new ();

	    ctk_css_provider_load_from_data (provider,
					     "* { -CtkComboBox-appears-as-list: true; }", 
					     -1, NULL);

	    ctk_style_context_add_provider_for_screen (cdk_screen_get_default (),
						       CTK_STYLE_PROVIDER (provider),
						       CTK_STYLE_PROVIDER_PRIORITY_FALLBACK);

	  }

        window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
        ctk_container_set_border_width (CTK_CONTAINER (window), 5);
        g_signal_connect (window, "destroy", ctk_main_quit, NULL);

        mainbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 2);
        ctk_container_add (CTK_CONTAINER (window), mainbox);

        /* CtkCellView */
        tmp = ctk_frame_new ("CtkCellView");
        ctk_box_pack_start (CTK_BOX (mainbox), tmp, FALSE, FALSE, 0);

        boom = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
        ctk_container_set_border_width (CTK_CONTAINER (boom), 5);
        ctk_container_add (CTK_CONTAINER (tmp), boom);

        cellview = ctk_cell_view_new ();
        renderer = ctk_cell_renderer_pixbuf_new ();
        ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (cellview),
                                    renderer,
                                    FALSE);
        g_object_set (renderer, "icon-name", "dialog-warning", NULL);

        renderer = ctk_cell_renderer_text_new ();
        ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (cellview),
                                    renderer,
                                    TRUE);
        g_object_set (renderer, "text", "la la la", NULL);
        ctk_container_add (CTK_CONTAINER (boom), cellview);

        /* CtkComboBox list */
        tmp = ctk_frame_new ("CtkComboBox (list)");
        ctk_box_pack_start (CTK_BOX (mainbox), tmp, FALSE, FALSE, 0);

        boom = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
        ctk_container_set_border_width (CTK_CONTAINER (boom), 5);
        ctk_container_add (CTK_CONTAINER (tmp), boom);

        model = create_list_blaat ();
        combobox = ctk_combo_box_new_with_model (model);
        g_object_unref (model);
        ctk_container_add (CTK_CONTAINER (boom), combobox);

        renderer = ctk_cell_renderer_pixbuf_new ();
        ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (combobox),
                                    renderer,
                                    FALSE);
        ctk_cell_layout_set_attributes (CTK_CELL_LAYOUT (combobox), renderer,
                                        "icon-name", 0,
                                        NULL);
	ctk_cell_layout_set_cell_data_func (CTK_CELL_LAYOUT (combobox),
					    renderer,
					    set_sensitive,
					    NULL, NULL);

        renderer = ctk_cell_renderer_text_new ();
        ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (combobox),
                                    renderer,
                                    TRUE);
        ctk_cell_layout_set_attributes (CTK_CELL_LAYOUT (combobox), renderer,
                                        "text", 1,
                                        NULL);
	ctk_cell_layout_set_cell_data_func (CTK_CELL_LAYOUT (combobox),
					    renderer,
					    set_sensitive,
					    NULL, NULL);
	ctk_combo_box_set_row_separator_func (CTK_COMBO_BOX (combobox), 
					      is_separator, NULL, NULL);
						
        ctk_combo_box_set_active (CTK_COMBO_BOX (combobox), 0);

        /* CtkComboBox dynamic list */
        tmp = ctk_frame_new ("CtkComboBox (dynamic list)");
        ctk_box_pack_start (CTK_BOX (mainbox), tmp, FALSE, FALSE, 0);

        boom = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
        ctk_container_set_border_width (CTK_CONTAINER (boom), 5);
        ctk_container_add (CTK_CONTAINER (tmp), boom);

        model = create_empty_list_blaat ();
        combobox = ctk_combo_box_new_with_model (model);
	g_signal_connect (combobox, "notify::popup-shown", 
			  G_CALLBACK (populate_list_blaat), combobox);

        g_object_unref (model);
        ctk_container_add (CTK_CONTAINER (boom), combobox);

        renderer = ctk_cell_renderer_pixbuf_new ();
        ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (combobox),
                                    renderer,
                                    FALSE);
        ctk_cell_layout_set_attributes (CTK_CELL_LAYOUT (combobox), renderer,
                                        "icon-name", 0,
                                        NULL);
	ctk_cell_layout_set_cell_data_func (CTK_CELL_LAYOUT (combobox),
					    renderer,
					    set_sensitive,
					    NULL, NULL);

        renderer = ctk_cell_renderer_text_new ();
        ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (combobox),
                                    renderer,
                                    TRUE);
        ctk_cell_layout_set_attributes (CTK_CELL_LAYOUT (combobox), renderer,
                                        "text", 1,
                                        NULL);
	ctk_cell_layout_set_cell_data_func (CTK_CELL_LAYOUT (combobox),
					    renderer,
					    set_sensitive,
					    NULL, NULL);
	ctk_combo_box_set_row_separator_func (CTK_COMBO_BOX (combobox), 
					      is_separator, NULL, NULL);
						
        ctk_combo_box_set_active (CTK_COMBO_BOX (combobox), 0);

        /* CtkComboBox custom entry */
        tmp = ctk_frame_new ("CtkComboBox (custom)");
        ctk_box_pack_start (CTK_BOX (mainbox), tmp, FALSE, FALSE, 0);

        boom = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
        ctk_container_set_border_width (CTK_CONTAINER (boom), 5);
        ctk_container_add (CTK_CONTAINER (tmp), boom);

        model = create_list_blaat ();
        combobox = ctk_combo_box_new_with_model (model);
        g_object_unref (model);
        ctk_container_add (CTK_CONTAINER (boom), combobox);

        renderer = ctk_cell_renderer_pixbuf_new ();
        ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (combobox),
                                    renderer,
                                    FALSE);
        ctk_cell_layout_set_attributes (CTK_CELL_LAYOUT (combobox), renderer,
                                        "icon-name", 0,
                                        NULL);
	ctk_cell_layout_set_cell_data_func (CTK_CELL_LAYOUT (combobox),
					    renderer,
					    set_sensitive,
					    NULL, NULL);

        renderer = ctk_cell_renderer_text_new ();
        ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (combobox),
                                    renderer,
                                    TRUE);
        ctk_cell_layout_set_attributes (CTK_CELL_LAYOUT (combobox), renderer,
                                        "text", 1,
                                        NULL);
	ctk_cell_layout_set_cell_data_func (CTK_CELL_LAYOUT (combobox),
					    renderer,
					    set_sensitive,
					    NULL, NULL);
	ctk_combo_box_set_row_separator_func (CTK_COMBO_BOX (combobox), 
					      is_separator, NULL, NULL);
						
        ctk_combo_box_set_active (CTK_COMBO_BOX (combobox), 0);

        tmp = ctk_cell_view_new ();
        ctk_widget_show (tmp);
        ctk_cell_view_set_model (CTK_CELL_VIEW (tmp), model);

        renderer = ctk_cell_renderer_text_new ();
        ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (tmp), renderer, TRUE);
        ctk_cell_layout_set_attributes (CTK_CELL_LAYOUT (tmp), renderer,
                                        "text", 1,
                                        NULL);
        color.red = 1.0;
        color.blue = 1.0;
        color.green = 0;
        color.alpha = 1.0;
        ctk_cell_view_set_background_rgba (CTK_CELL_VIEW (tmp), &color);
        displayed_row_changed (CTK_COMBO_BOX (combobox), CTK_CELL_VIEW (tmp));
        g_signal_connect (combobox, "changed", G_CALLBACK (displayed_row_changed), tmp); 
           
        ctk_container_add (CTK_CONTAINER (combobox), tmp);

        /* CtkComboBox tree */
        tmp = ctk_frame_new ("CtkComboBox (tree)");
        ctk_box_pack_start (CTK_BOX (mainbox), tmp, FALSE, FALSE, 0);

        boom = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
        ctk_container_set_border_width (CTK_CONTAINER (boom), 5);
        ctk_container_add (CTK_CONTAINER (tmp), boom);

        model = create_tree_blaat ();
        combobox = ctk_combo_box_new_with_model (model);
        g_object_unref (model);
        ctk_container_add (CTK_CONTAINER (boom), combobox);

        renderer = ctk_cell_renderer_pixbuf_new ();
        ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (combobox),
                                    renderer,
                                    FALSE);
        ctk_cell_layout_set_attributes (CTK_CELL_LAYOUT (combobox), renderer,
                                        "icon-name", 0,
                                        NULL);
	ctk_cell_layout_set_cell_data_func (CTK_CELL_LAYOUT (combobox),
					    renderer,
					    set_sensitive,
					    NULL, NULL);

        renderer = ctk_cell_renderer_text_new ();
        ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (combobox),
                                    renderer,
                                    TRUE);
        ctk_cell_layout_set_attributes (CTK_CELL_LAYOUT (combobox), renderer,
                                        "text", 1,
                                        NULL);
	ctk_cell_layout_set_cell_data_func (CTK_CELL_LAYOUT (combobox),
					    renderer,
					    set_sensitive,
					    NULL, NULL);
	ctk_combo_box_set_row_separator_func (CTK_COMBO_BOX (combobox), 
					      is_separator, NULL, NULL);
						
        ctk_combo_box_set_active (CTK_COMBO_BOX (combobox), 0);
#if 0
	g_timeout_add (1000, (GSourceFunc) animation_timer, model);
#endif

        /* CtkComboBox (grid mode) */
        tmp = ctk_frame_new ("CtkComboBox (grid mode)");
        ctk_box_pack_start (CTK_BOX (mainbox), tmp, FALSE, FALSE, 0);

        boom = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
        ctk_container_set_border_width (CTK_CONTAINER (boom), 5);
        ctk_container_add (CTK_CONTAINER (tmp), boom);

        comboboxgrid = create_combo_box_grid_demo ();
        ctk_box_pack_start (CTK_BOX (boom), comboboxgrid, FALSE, FALSE, 0);


        /* CtkComboBoxEntry */
        tmp = ctk_frame_new ("CtkComboBox with entry");
        ctk_box_pack_start (CTK_BOX (mainbox), tmp, FALSE, FALSE, 0);

        boom = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
        ctk_container_set_border_width (CTK_CONTAINER (boom), 5);
        ctk_container_add (CTK_CONTAINER (tmp), boom);

        comboboxtext = ctk_combo_box_text_new_with_entry ();
        setup_combo_entry (CTK_COMBO_BOX_TEXT (comboboxtext));
        ctk_container_add (CTK_CONTAINER (boom), comboboxtext);


        /* Phylogenetic tree */
        tmp = ctk_frame_new ("What are you ?");
        ctk_box_pack_start (CTK_BOX (mainbox), tmp, FALSE, FALSE, 0);

        boom = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
        ctk_container_set_border_width (CTK_CONTAINER (boom), 5);
        ctk_container_add (CTK_CONTAINER (tmp), boom);

        model = create_phylogenetic_tree ();
        combobox = ctk_combo_box_new_with_model (model);
        g_object_unref (model);
        ctk_container_add (CTK_CONTAINER (boom), combobox);

        renderer = ctk_cell_renderer_text_new ();
        ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (combobox),
                                    renderer,
                                    TRUE);
        ctk_cell_layout_set_attributes (CTK_CELL_LAYOUT (combobox), renderer,
                                        "text", 0,
                                        NULL);
	
        ctk_combo_box_set_active (CTK_COMBO_BOX (combobox), 0);

        /* Capitals */
        tmp = ctk_frame_new ("Where are you ?");
        ctk_box_pack_start (CTK_BOX (mainbox), tmp, FALSE, FALSE, 0);

        boom = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
        ctk_container_set_border_width (CTK_CONTAINER (boom), 5);
        ctk_container_add (CTK_CONTAINER (tmp), boom);

        model = create_capital_tree ();
	combobox = ctk_combo_box_new_with_model (model);
        g_object_unref (model);
        ctk_container_add (CTK_CONTAINER (boom), combobox);
        renderer = ctk_cell_renderer_text_new ();
        ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (combobox),
                                    renderer,
                                    TRUE);
        ctk_cell_layout_set_attributes (CTK_CELL_LAYOUT (combobox), renderer,
                                        "text", 0,
                                        NULL);
	ctk_cell_layout_set_cell_data_func (CTK_CELL_LAYOUT (combobox),
					    renderer,
					    capital_sensitive,
					    NULL, NULL);
	path = ctk_tree_path_new_from_indices (0, 8, -1);
	ctk_tree_model_get_iter (model, &iter, path);
	ctk_tree_path_free (path);
        ctk_combo_box_set_active_iter (CTK_COMBO_BOX (combobox), &iter);

#if 1
	cdk_threads_add_timeout (1000, (GSourceFunc) capital_animation, model);
#endif

        /* Aligned Food */
        tmp = ctk_frame_new ("Hungry ?");
        ctk_box_pack_start (CTK_BOX (mainbox), tmp, FALSE, FALSE, 0);

        boom = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
        ctk_container_set_border_width (CTK_CONTAINER (boom), 5);
        ctk_container_add (CTK_CONTAINER (tmp), boom);

        model = create_food_list ();
	combobox = ctk_combo_box_new_with_model (model);
        g_object_unref (model);
        ctk_container_add (CTK_CONTAINER (boom), combobox);

	area = ctk_cell_layout_get_area (CTK_CELL_LAYOUT (combobox));

        renderer = ctk_cell_renderer_text_new ();
	ctk_cell_area_add_with_properties (area, renderer, 
					   "align", TRUE, 
					   "expand", TRUE, 
					   NULL);
        ctk_cell_layout_set_attributes (CTK_CELL_LAYOUT (combobox), renderer,
                                        "text", 0,
                                        NULL);

        renderer = ctk_cell_renderer_text_new ();
	ctk_cell_area_add_with_properties (area, renderer, 
					   "align", TRUE, 
					   "expand", TRUE, 
					   NULL);
        ctk_cell_layout_set_attributes (CTK_CELL_LAYOUT (combobox), renderer,
                                        "text", 1,
                                        NULL);

        ctk_combo_box_set_active (CTK_COMBO_BOX (combobox), 0);

	/* Ellipsizing growing combos */
        tmp = ctk_frame_new ("Unconstrained Menu");
        ctk_box_pack_start (CTK_BOX (mainbox), tmp, FALSE, FALSE, 0);

        boom = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
        ctk_container_set_border_width (CTK_CONTAINER (boom), 5);
        ctk_container_add (CTK_CONTAINER (tmp), boom);

	model = create_list_long ();
	combobox = ctk_combo_box_new_with_model (model);
        g_object_unref (model);
        ctk_container_add (CTK_CONTAINER (boom), combobox);
        renderer = ctk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);

        ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (combobox), renderer, TRUE);
        ctk_cell_layout_set_attributes (CTK_CELL_LAYOUT (combobox), renderer,
                                        "text", 0, NULL);
        ctk_combo_box_set_active (CTK_COMBO_BOX (combobox), 0);
	ctk_combo_box_set_popup_fixed_width (CTK_COMBO_BOX (combobox), FALSE);

        tmp = ctk_frame_new ("Looong");
        ctk_box_pack_start (CTK_BOX (mainbox), tmp, FALSE, FALSE, 0);
        combobox = ctk_combo_box_text_new ();
        for (i = 0; i < 200; i++)
          {
            text = g_strdup_printf ("Item %d", i);
            ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combobox), text);
            g_free (text);
          }
        ctk_combo_box_set_active (CTK_COMBO_BOX (combobox), 53);
        ctk_container_add (CTK_CONTAINER (tmp), combobox);

        ctk_widget_show_all (window);

        ctk_main ();

        return 0;
}

/* vim:expandtab
 */
