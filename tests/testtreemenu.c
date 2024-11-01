#include <ctk/ctk.h>


/*******************************************************
 *                       Grid Test                     *
 *******************************************************/

#if _CTK_TREE_MENU_WAS_A_PUBLIC_CLASS_
static GdkPixbuf *
create_color_pixbuf (const char *color)
{
  GdkPixbuf *pixbuf;
  CdkRGBA rgba;

  int x;
  int num;
  int rowstride;
  guchar *pixels, *p;
  
  if (!cdk_rgba_parse (color, &col))
    return NULL;
  
  pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB,
			   FALSE, 8,
			   16, 16);
  
  rowstride = gdk_pixbuf_get_rowstride (pixbuf);
  p = pixels = gdk_pixbuf_get_pixels (pixbuf);
  
  num = gdk_pixbuf_get_width (pixbuf) *
    gdk_pixbuf_get_height (pixbuf);
  
  for (x = 0; x < num; x++) {
    p[0] = col.red * 255;
    p[1] = col.green * 255;
    p[2] = col.blue * 255;
    p += 3;
  }
  
  return pixbuf;
}

static CtkWidget *
create_menu_grid_demo (void)
{
  CtkWidget *menu;
  CtkTreeIter iter;
  GdkPixbuf *pixbuf;
  CtkCellRenderer *cell = ctk_cell_renderer_pixbuf_new ();
  CtkListStore *store;
  
  store = ctk_list_store_new (1, GDK_TYPE_PIXBUF);

  menu = ctk_tree_menu_new_full (NULL, CTK_TREE_MODEL (store), NULL);
  ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (menu), cell, TRUE);
  ctk_cell_layout_set_attributes (CTK_CELL_LAYOUT (menu), cell, "pixbuf", 0, NULL);
  
  ctk_tree_menu_set_wrap_width (CTK_TREE_MENU (menu), 3);

  /* first row */
  pixbuf = create_color_pixbuf ("red");
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter,
		      0, pixbuf,
		      -1);
  g_object_unref (pixbuf);
  
  pixbuf = create_color_pixbuf ("green");
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter,
		      0, pixbuf,
		      -1);
  g_object_unref (pixbuf);
  
  pixbuf = create_color_pixbuf ("blue");
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter,
		      0, pixbuf,
		      -1);
  g_object_unref (pixbuf);
  
  /* second row */
  pixbuf = create_color_pixbuf ("yellow");
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter,
		      0, pixbuf,
		      -1);
  g_object_unref (pixbuf);
  
  pixbuf = create_color_pixbuf ("black");
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter,
		      0, pixbuf,
		      -1);
  g_object_unref (pixbuf);
  
  pixbuf = create_color_pixbuf ("white");
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter,
		      0, pixbuf,
		      -1);
  g_object_unref (pixbuf);
  
  /* third row */
  pixbuf = create_color_pixbuf ("gray");
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter,
		      0, pixbuf,
		      -1);
  g_object_unref (pixbuf);
  
  pixbuf = create_color_pixbuf ("snow");
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter,
		      0, pixbuf,
		      -1);
  g_object_unref (pixbuf);
  
  pixbuf = create_color_pixbuf ("magenta");
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter,
		      0, pixbuf,
		      -1);
  g_object_unref (pixbuf);

  g_object_unref (store);
  
  return menu;
}
#endif

/*******************************************************
 *                      Simple Test                    *
 *******************************************************/
enum {
  SIMPLE_COLUMN_NAME,
  SIMPLE_COLUMN_ICON,
  SIMPLE_COLUMN_DESCRIPTION,
  N_SIMPLE_COLUMNS
};

static CtkCellRenderer *cell_1 = NULL, *cell_2 = NULL, *cell_3 = NULL;

static CtkTreeModel *
simple_tree_model (void)
{
  CtkTreeIter   iter, parent, child;
  CtkTreeStore *store = 
    ctk_tree_store_new (N_SIMPLE_COLUMNS,
			G_TYPE_STRING,  /* name text */
			G_TYPE_STRING,  /* icon name */
			G_TYPE_STRING); /* description text */


  ctk_tree_store_append (store, &parent, NULL);
  ctk_tree_store_set (store, &parent, 
		      SIMPLE_COLUMN_NAME, "Alice in wonderland",
		      SIMPLE_COLUMN_ICON, "system-run",
		      SIMPLE_COLUMN_DESCRIPTION, 
		      "Twas brillig, and the slithy toves "
		      "did gyre and gimble in the wabe",
		      -1);

  ctk_tree_store_append (store, &iter, &parent);
  ctk_tree_store_set (store, &iter, 
		      SIMPLE_COLUMN_NAME, "Go ask",
		      SIMPLE_COLUMN_ICON, "zoom-out",
		      SIMPLE_COLUMN_DESCRIPTION, "One pill makes you shorter",
		      -1);

  ctk_tree_store_append (store, &iter, &parent);
  ctk_tree_store_set (store, &iter, 
		      SIMPLE_COLUMN_NAME, "Alice",
		      SIMPLE_COLUMN_ICON, "zoom-in",
		      SIMPLE_COLUMN_DESCRIPTION, "Another one makes you tall",
		      -1);

  ctk_tree_store_append (store, &iter, &parent);
  ctk_tree_store_set (store, &iter, 
		      SIMPLE_COLUMN_NAME, "Jefferson Airplane",
		      SIMPLE_COLUMN_ICON, "zoom-fit-best",
		      SIMPLE_COLUMN_DESCRIPTION, "The one's that mother gives you dont do anything at all",
		      -1);

  ctk_tree_store_append (store, &iter, NULL);
  ctk_tree_store_set (store, &iter, 
		      SIMPLE_COLUMN_NAME, "Marry Poppins",
		      SIMPLE_COLUMN_ICON, "dialog-information",
		      SIMPLE_COLUMN_DESCRIPTION, "Supercalifragilisticexpialidocious",
		      -1);

  ctk_tree_store_append (store, &iter, NULL);
  ctk_tree_store_set (store, &iter, 
		      SIMPLE_COLUMN_NAME, "George Bush",
		      SIMPLE_COLUMN_ICON, "dialog-question",
		      SIMPLE_COLUMN_DESCRIPTION, "It's a very good question, very direct, "
		      "and I'm not going to answer it",
		      -1);

  ctk_tree_store_append (store, &parent, NULL);
  ctk_tree_store_set (store, &parent, 
		      SIMPLE_COLUMN_NAME, "Whinnie the pooh",
		      SIMPLE_COLUMN_ICON, "process-stop",
		      SIMPLE_COLUMN_DESCRIPTION, "The most wonderful thing about tiggers, "
		      "is tiggers are wonderful things",
		      -1);

  ctk_tree_store_append (store, &iter, &parent);
  ctk_tree_store_set (store, &iter, 
		      SIMPLE_COLUMN_NAME, "Tigger",
		      SIMPLE_COLUMN_ICON, "dialog-information",
		      SIMPLE_COLUMN_DESCRIPTION, "Eager",
		      -1);

  ctk_tree_store_append (store, &child, &iter);
  ctk_tree_store_set (store, &child, 
		      SIMPLE_COLUMN_NAME, "Jump",
		      SIMPLE_COLUMN_ICON, "dialog-information",
		      SIMPLE_COLUMN_DESCRIPTION, "Very High",
		      -1);

  ctk_tree_store_append (store, &child, &iter);
  ctk_tree_store_set (store, &child, 
		      SIMPLE_COLUMN_NAME, "Pounce",
		      SIMPLE_COLUMN_ICON, "dialog-question",
		      SIMPLE_COLUMN_DESCRIPTION, "On Pooh",
		      -1);

  ctk_tree_store_append (store, &child, &iter);
  ctk_tree_store_set (store, &child, 
		      SIMPLE_COLUMN_NAME, "Bounce",
		      SIMPLE_COLUMN_ICON, "dialog-error",
		      SIMPLE_COLUMN_DESCRIPTION, "Around",
		      -1);

  ctk_tree_store_append (store, &iter, &parent);
  ctk_tree_store_set (store, &iter, 
		      SIMPLE_COLUMN_NAME, "Owl",
		      SIMPLE_COLUMN_ICON, "process-stop",
		      SIMPLE_COLUMN_DESCRIPTION, "Wise",
		      -1);

  ctk_tree_store_append (store, &iter, &parent);
  ctk_tree_store_set (store, &iter, 
		      SIMPLE_COLUMN_NAME, "Eor",
		      SIMPLE_COLUMN_ICON, "dialog-question",
		      SIMPLE_COLUMN_DESCRIPTION, "Depressed",
		      -1);

  ctk_tree_store_append (store, &iter, &parent);
  ctk_tree_store_set (store, &iter, 
		      SIMPLE_COLUMN_NAME, "Piglet",
		      SIMPLE_COLUMN_ICON, "media-playback-start",
		      SIMPLE_COLUMN_DESCRIPTION, "Insecure",
		      -1);

  ctk_tree_store_append (store, &iter, NULL);
  ctk_tree_store_set (store, &iter, 
		      SIMPLE_COLUMN_NAME, "Aleister Crowley",
		      SIMPLE_COLUMN_ICON, "help-about",
		      SIMPLE_COLUMN_DESCRIPTION, 
		      "Thou shalt do what thou wilt shall be the whole of the law",
		      -1);

  ctk_tree_store_append (store, &iter, NULL);
  ctk_tree_store_set (store, &iter, 
		      SIMPLE_COLUMN_NAME, "Mark Twain",
		      SIMPLE_COLUMN_ICON, "application-exit",
		      SIMPLE_COLUMN_DESCRIPTION, 
		      "Giving up smoking is the easiest thing in the world. "
		      "I know because I've done it thousands of times.",
		      -1);


  return (CtkTreeModel *)store;
}

static CtkCellArea *
create_cell_area (void)
{
  CtkCellArea *area;
  CtkCellRenderer *renderer;

  area = ctk_cell_area_box_new ();

  cell_1 = renderer = ctk_cell_renderer_text_new ();
  ctk_cell_area_box_pack_start (CTK_CELL_AREA_BOX (area), renderer, FALSE, FALSE, FALSE);
  ctk_cell_area_attribute_connect (area, renderer, "text", SIMPLE_COLUMN_NAME);

  cell_2 = renderer = ctk_cell_renderer_pixbuf_new ();
  g_object_set (G_OBJECT (renderer), "xalign", 0.0F, NULL);
  ctk_cell_area_box_pack_start (CTK_CELL_AREA_BOX (area), renderer, TRUE, FALSE, FALSE);
  ctk_cell_area_attribute_connect (area, renderer, "icon-name", SIMPLE_COLUMN_ICON);

  cell_3 = renderer = ctk_cell_renderer_text_new ();
  g_object_set (G_OBJECT (renderer), 
		"wrap-mode", PANGO_WRAP_WORD,
		"wrap-width", 215,
		NULL);
  ctk_cell_area_box_pack_start (CTK_CELL_AREA_BOX (area), renderer, FALSE, TRUE, FALSE);
  ctk_cell_area_attribute_connect (area, renderer, "text", SIMPLE_COLUMN_DESCRIPTION);

  return area;
}

#if _CTK_TREE_MENU_WAS_A_PUBLIC_CLASS_
static CtkWidget *
simple_tree_menu (CtkCellArea *area)
{
  CtkTreeModel *model;
  CtkWidget *menu;

  model = simple_tree_model ();

  menu = ctk_tree_menu_new_with_area (area);
  ctk_tree_menu_set_model (CTK_TREE_MENU (menu), model);

  return menu;
}
#endif

static void
orientation_changed (CtkComboBox  *combo,
		     CtkCellArea  *area)
{
  CtkOrientation orientation = ctk_combo_box_get_active (combo);

  ctk_orientable_set_orientation (CTK_ORIENTABLE (area), orientation);
}

static void
align_cell_2_toggled (CtkToggleButton  *toggle,
		      CtkCellArea      *area)
{
  gboolean align = ctk_toggle_button_get_active (toggle);

  ctk_cell_area_cell_set (area, cell_2, "align", align, NULL);
}

static void
align_cell_3_toggled (CtkToggleButton  *toggle,
		      CtkCellArea      *area)
{
  gboolean align = ctk_toggle_button_get_active (toggle);

  ctk_cell_area_cell_set (area, cell_3, "align", align, NULL);
}

static void
expand_cell_1_toggled (CtkToggleButton  *toggle,
		       CtkCellArea      *area)
{
  gboolean expand = ctk_toggle_button_get_active (toggle);

  ctk_cell_area_cell_set (area, cell_1, "expand", expand, NULL);
}

static void
expand_cell_2_toggled (CtkToggleButton  *toggle,
		       CtkCellArea      *area)
{
  gboolean expand = ctk_toggle_button_get_active (toggle);

  ctk_cell_area_cell_set (area, cell_2, "expand", expand, NULL);
}

static void
expand_cell_3_toggled (CtkToggleButton  *toggle,
		       CtkCellArea      *area)
{
  gboolean expand = ctk_toggle_button_get_active (toggle);

  ctk_cell_area_cell_set (area, cell_3, "expand", expand, NULL);
}

gboolean 
enable_submenu_headers (CtkTreeModel *model G_GNUC_UNUSED,
			CtkTreeIter  *iter G_GNUC_UNUSED,
			gpointer      data G_GNUC_UNUSED)
{
  return TRUE;
}


#if _CTK_TREE_MENU_WAS_A_PUBLIC_CLASS_
static void
menu_activated_cb (CtkTreeMenu *menu,
		   const gchar *path,
		   gpointer     unused)
{
  CtkTreeModel *model = ctk_tree_menu_get_model (menu);
  CtkTreeIter   iter;
  gchar        *row_name;

  if (!ctk_tree_model_get_iter_from_string (model, &iter, path))
    return;

  ctk_tree_model_get (model, &iter, SIMPLE_COLUMN_NAME, &row_name, -1);

  g_print ("Item activated: %s\n", row_name);

  g_free (row_name);
}

static void
submenu_headers_toggled (CtkToggleButton  *toggle,
			 CtkTreeMenu      *menu)
{
  if (ctk_toggle_button_get_active (toggle))
    ctk_tree_menu_set_header_func (menu, enable_submenu_headers, NULL, NULL);
  else
    ctk_tree_menu_set_header_func (menu, NULL, NULL, NULL);
}

static void
tearoff_toggled (CtkToggleButton *toggle,
		 CtkTreeMenu     *menu)
{
  ctk_tree_menu_set_tearoff (menu, ctk_toggle_button_get_active (toggle));
}
#endif

static void
tree_menu (void)
{
  CtkWidget *window, *widget;
  CtkWidget *menubar, *vbox;
  CtkCellArea *area;
  CtkTreeModel *store;

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);

  ctk_window_set_title (CTK_WINDOW (window), "CtkTreeMenu");

  vbox  = ctk_box_new (CTK_ORIENTATION_VERTICAL, 4);
  ctk_widget_show (vbox);

  menubar = ctk_menu_bar_new ();
  ctk_widget_show (menubar);

  store = simple_tree_model ();
  area  = create_cell_area ();

#if _CTK_TREE_MENU_WAS_A_PUBLIC_CLASS_
  menuitem = ctk_menu_item_new_with_label ("Grid");
  menu = create_menu_grid_demo ();
  ctk_widget_show (menu);
  ctk_widget_show (menuitem);
  ctk_menu_shell_append (CTK_MENU_SHELL (menubar), menuitem);
  ctk_menu_item_set_submenu (CTK_MENU_ITEM (menuitem), menu);

  menuitem = ctk_menu_item_new_with_label ("Tree");
  menu = simple_tree_menu ();
  ctk_widget_show (menu);
  ctk_widget_show (menuitem);
  ctk_menu_shell_prepend (CTK_MENU_SHELL (menubar), menuitem);
  ctk_menu_item_set_submenu (CTK_MENU_ITEM (menuitem), menu);

  g_signal_connect (menu, "menu-activate", G_CALLBACK (menu_activated_cb), NULL);

  ctk_box_pack_start (CTK_BOX (vbox), menubar, FALSE, FALSE, 0);
#endif

  /* Add a combo box with the same menu ! */
  widget = ctk_combo_box_new_with_area (area);
  ctk_combo_box_set_model (CTK_COMBO_BOX (widget), store);
  ctk_combo_box_set_active (CTK_COMBO_BOX (widget), 0);
  ctk_widget_show (widget);
  ctk_box_pack_end (CTK_BOX (vbox), widget, FALSE, FALSE, 0);

  /* Now add some controls */
  widget = ctk_combo_box_text_new ();
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (widget), "Horizontal");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (widget), "Vertical");
  ctk_combo_box_set_active (CTK_COMBO_BOX (widget), 0);
  ctk_widget_show (widget);
  ctk_box_pack_start (CTK_BOX (vbox), widget, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (widget), "changed",
                    G_CALLBACK (orientation_changed), area);

  widget = ctk_check_button_new_with_label ("Align 2nd Cell");
  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (widget), FALSE);
  ctk_widget_show (widget);
  ctk_box_pack_start (CTK_BOX (vbox), widget, FALSE, FALSE, 0);
  
  g_signal_connect (G_OBJECT (widget), "toggled",
                    G_CALLBACK (align_cell_2_toggled), area);

  widget = ctk_check_button_new_with_label ("Align 3rd Cell");
  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (widget), TRUE);
  ctk_widget_show (widget);
  ctk_box_pack_start (CTK_BOX (vbox), widget, FALSE, FALSE, 0);
  
  g_signal_connect (G_OBJECT (widget), "toggled",
                    G_CALLBACK (align_cell_3_toggled), area);

  widget = ctk_check_button_new_with_label ("Expand 1st Cell");
  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (widget), FALSE);
  ctk_widget_show (widget);
  ctk_box_pack_start (CTK_BOX (vbox), widget, FALSE, FALSE, 0);
  
  g_signal_connect (G_OBJECT (widget), "toggled",
                    G_CALLBACK (expand_cell_1_toggled), area);

  widget = ctk_check_button_new_with_label ("Expand 2nd Cell");
  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (widget), TRUE);
  ctk_widget_show (widget);
  ctk_box_pack_start (CTK_BOX (vbox), widget, FALSE, FALSE, 0);
  
  g_signal_connect (G_OBJECT (widget), "toggled",
                    G_CALLBACK (expand_cell_2_toggled), area);

  widget = ctk_check_button_new_with_label ("Expand 3rd Cell");
  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (widget), FALSE);
  ctk_widget_show (widget);
  ctk_box_pack_start (CTK_BOX (vbox), widget, FALSE, FALSE, 0);
  
  g_signal_connect (G_OBJECT (widget), "toggled",
                    G_CALLBACK (expand_cell_3_toggled), area);

#if _CTK_TREE_MENU_WAS_A_PUBLIC_CLASS_
  widget = ctk_check_button_new_with_label ("Submenu Headers");
  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (widget), FALSE);
  ctk_widget_show (widget);
  ctk_box_pack_start (CTK_BOX (vbox), widget, FALSE, FALSE, 0);
  
  g_signal_connect (G_OBJECT (widget), "toggled",
                    G_CALLBACK (submenu_headers_toggled), menu);

  widget = ctk_check_button_new_with_label ("Tearoff menu");
  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (widget), FALSE);
  ctk_widget_show (widget);
  ctk_box_pack_start (CTK_BOX (vbox), widget, FALSE, FALSE, 0);
  
  g_signal_connect (G_OBJECT (widget), "toggled",
                    G_CALLBACK (tearoff_toggled), menu);
#endif

  ctk_container_add (CTK_CONTAINER (window), vbox);

  ctk_widget_show (window);
}

int
main (int   argc G_GNUC_UNUSED,
      char *argv[] G_GNUC_UNUSED)
{
  ctk_init (NULL, NULL);

  tree_menu ();

  ctk_main ();

  return 0;
}
