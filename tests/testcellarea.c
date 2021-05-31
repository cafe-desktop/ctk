#include <ctk/ctk.h>

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
simple_list_model (void)
{
  CtkTreeIter   iter;
  CtkListStore *store = 
    ctk_list_store_new (N_SIMPLE_COLUMNS,
			G_TYPE_STRING,  /* name text */
			G_TYPE_STRING,  /* icon name */
			G_TYPE_STRING); /* description text */

  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 
		      SIMPLE_COLUMN_NAME, "Alice in wonderland",
		      SIMPLE_COLUMN_ICON, "system-run",
		      SIMPLE_COLUMN_DESCRIPTION, 
		      "Twas brillig, and the slithy toves "
		      "did gyre and gimble in the wabe; "
		      "all mimsy were the borogoves, "
		      "and the mome raths outgrabe",
		      -1);

  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 
		      SIMPLE_COLUMN_NAME, "Marry Poppins",
		      SIMPLE_COLUMN_ICON, "dialog-information",
		      SIMPLE_COLUMN_DESCRIPTION, "Supercalifragilisticexpialidocious",
		      -1);

  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 
		      SIMPLE_COLUMN_NAME, "George Bush",
		      SIMPLE_COLUMN_ICON, "dialog-warning",
		      SIMPLE_COLUMN_DESCRIPTION, "It's a very good question, very direct, "
		      "and I'm not going to answer it",
		      -1);

  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 
		      SIMPLE_COLUMN_NAME, "Whinnie the pooh",
		      SIMPLE_COLUMN_ICON, "process-stop",
		      SIMPLE_COLUMN_DESCRIPTION, "The most wonderful thing about tiggers, "
		      "is tiggers are wonderful things",
		      -1);

  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 
		      SIMPLE_COLUMN_NAME, "Aleister Crowley",
		      SIMPLE_COLUMN_ICON, "help-about",
		      SIMPLE_COLUMN_DESCRIPTION, 
		      "Thou shalt do what thou wilt shall be the whole of the law",
		      -1);

  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 
		      SIMPLE_COLUMN_NAME, "Mark Twain",
		      SIMPLE_COLUMN_ICON, "application-exit",
		      SIMPLE_COLUMN_DESCRIPTION, 
		      "Giving up smoking is the easiest thing in the world. "
		      "I know because I've done it thousands of times.",
		      -1);


  return (CtkTreeModel *)store;
}

static CtkWidget *
simple_iconview (void)
{
  CtkTreeModel *model;
  CtkWidget *iconview;
  CtkCellArea *area;
  CtkCellRenderer *renderer;

  iconview = ctk_icon_view_new ();
  ctk_widget_show (iconview);

  model = simple_list_model ();

  ctk_icon_view_set_model (CTK_ICON_VIEW (iconview), model);
  ctk_icon_view_set_item_orientation (CTK_ICON_VIEW (iconview), CTK_ORIENTATION_HORIZONTAL);

  area = ctk_cell_layout_get_area (CTK_CELL_LAYOUT (iconview));

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

  return iconview;
}

static void
orientation_changed (CtkComboBox      *combo,
		     CtkIconView *iconview)
{
  CtkOrientation orientation = ctk_combo_box_get_active (combo);

  ctk_icon_view_set_item_orientation (iconview, orientation);
}

static void
align_cell_2_toggled (CtkToggleButton  *toggle,
		      CtkIconView *iconview)
{
  CtkCellArea *area = ctk_cell_layout_get_area (CTK_CELL_LAYOUT (iconview));
  gboolean     align = ctk_toggle_button_get_active (toggle);

  ctk_cell_area_cell_set (area, cell_2, "align", align, NULL);
}

static void
align_cell_3_toggled (CtkToggleButton  *toggle,
		      CtkIconView *iconview)
{
  CtkCellArea *area = ctk_cell_layout_get_area (CTK_CELL_LAYOUT (iconview));
  gboolean     align = ctk_toggle_button_get_active (toggle);

  ctk_cell_area_cell_set (area, cell_3, "align", align, NULL);
}

static void
expand_cell_1_toggled (CtkToggleButton  *toggle,
		       CtkIconView *iconview)
{
  CtkCellArea *area = ctk_cell_layout_get_area (CTK_CELL_LAYOUT (iconview));
  gboolean     expand = ctk_toggle_button_get_active (toggle);

  ctk_cell_area_cell_set (area, cell_1, "expand", expand, NULL);
}

static void
expand_cell_2_toggled (CtkToggleButton  *toggle,
		       CtkIconView *iconview)
{
  CtkCellArea *area = ctk_cell_layout_get_area (CTK_CELL_LAYOUT (iconview));
  gboolean     expand = ctk_toggle_button_get_active (toggle);

  ctk_cell_area_cell_set (area, cell_2, "expand", expand, NULL);
}

static void
expand_cell_3_toggled (CtkToggleButton  *toggle,
		       CtkIconView *iconview)
{
  CtkCellArea *area = ctk_cell_layout_get_area (CTK_CELL_LAYOUT (iconview));
  gboolean     expand = ctk_toggle_button_get_active (toggle);

  ctk_cell_area_cell_set (area, cell_3, "expand", expand, NULL);
}

static void
simple_cell_area (void)
{
  CtkWidget *window, *widget;
  CtkWidget *iconview, *frame, *vbox, *hbox;

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);

  ctk_window_set_title (CTK_WINDOW (window), "CellArea expand and alignments");

  iconview = simple_iconview ();

  hbox  = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 4);
  frame = ctk_frame_new (NULL);
  ctk_widget_show (hbox);
  ctk_widget_show (frame);

  ctk_widget_set_valign (frame, CTK_ALIGN_CENTER);
  ctk_widget_set_halign (frame, CTK_ALIGN_FILL);

  ctk_container_add (CTK_CONTAINER (frame), iconview);

  ctk_box_pack_end (CTK_BOX (hbox), frame, TRUE, TRUE, 0);

  /* Now add some controls */
  vbox  = ctk_box_new (CTK_ORIENTATION_VERTICAL, 4);
  ctk_widget_show (vbox);
  ctk_box_pack_end (CTK_BOX (hbox), vbox, FALSE, FALSE, 0);

  widget = ctk_combo_box_text_new ();
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (widget), "Horizontal");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (widget), "Vertical");
  ctk_combo_box_set_active (CTK_COMBO_BOX (widget), 0);
  ctk_widget_show (widget);
  ctk_box_pack_start (CTK_BOX (vbox), widget, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (widget), "changed",
                    G_CALLBACK (orientation_changed), iconview);

  widget = ctk_check_button_new_with_label ("Align 2nd Cell");
  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (widget), FALSE);
  ctk_widget_show (widget);
  ctk_box_pack_start (CTK_BOX (vbox), widget, FALSE, FALSE, 0);
  
  g_signal_connect (G_OBJECT (widget), "toggled",
                    G_CALLBACK (align_cell_2_toggled), iconview);

  widget = ctk_check_button_new_with_label ("Align 3rd Cell");
  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (widget), TRUE);
  ctk_widget_show (widget);
  ctk_box_pack_start (CTK_BOX (vbox), widget, FALSE, FALSE, 0);
  
  g_signal_connect (G_OBJECT (widget), "toggled",
                    G_CALLBACK (align_cell_3_toggled), iconview);


  widget = ctk_check_button_new_with_label ("Expand 1st Cell");
  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (widget), FALSE);
  ctk_widget_show (widget);
  ctk_box_pack_start (CTK_BOX (vbox), widget, FALSE, FALSE, 0);
  
  g_signal_connect (G_OBJECT (widget), "toggled",
                    G_CALLBACK (expand_cell_1_toggled), iconview);

  widget = ctk_check_button_new_with_label ("Expand 2nd Cell");
  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (widget), TRUE);
  ctk_widget_show (widget);
  ctk_box_pack_start (CTK_BOX (vbox), widget, FALSE, FALSE, 0);
  
  g_signal_connect (G_OBJECT (widget), "toggled",
                    G_CALLBACK (expand_cell_2_toggled), iconview);

  widget = ctk_check_button_new_with_label ("Expand 3rd Cell");
  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (widget), FALSE);
  ctk_widget_show (widget);
  ctk_box_pack_start (CTK_BOX (vbox), widget, FALSE, FALSE, 0);
  
  g_signal_connect (G_OBJECT (widget), "toggled",
                    G_CALLBACK (expand_cell_3_toggled), iconview);

  ctk_container_add (CTK_CONTAINER (window), hbox);

  ctk_widget_show (window);
}

/*******************************************************
 *                      Focus Test                     *
 *******************************************************/
static CtkCellRenderer *focus_renderer, *sibling_renderer;

enum {
  FOCUS_COLUMN_NAME,
  FOCUS_COLUMN_CHECK,
  FOCUS_COLUMN_STATIC_TEXT,
  N_FOCUS_COLUMNS
};

static CtkTreeModel *
focus_list_model (void)
{
  CtkTreeIter   iter;
  CtkListStore *store = 
    ctk_list_store_new (N_FOCUS_COLUMNS,
			G_TYPE_STRING,  /* name text */
			G_TYPE_BOOLEAN, /* check */
			G_TYPE_STRING); /* static text */

  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 
		      FOCUS_COLUMN_NAME, "Enter a string",
		      FOCUS_COLUMN_CHECK, TRUE,
		      FOCUS_COLUMN_STATIC_TEXT, "Does it fly ?",
		      -1);

  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 
		      FOCUS_COLUMN_NAME, "Enter a string",
		      FOCUS_COLUMN_CHECK, FALSE,
		      FOCUS_COLUMN_STATIC_TEXT, "Would you put it in a toaster ?",
		      -1);

  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 
		      FOCUS_COLUMN_NAME, "Type something",
		      FOCUS_COLUMN_CHECK, FALSE,
		      FOCUS_COLUMN_STATIC_TEXT, "Does it feed on cute kittens ?",
		      -1);

  return (CtkTreeModel *)store;
}

static void
cell_toggled (CtkCellRendererToggle *cell_renderer,
	      const gchar           *path,
	      CtkIconView      *iconview)
{
  CtkTreeModel *model = ctk_icon_view_get_model (iconview);
  CtkTreeIter   iter;
  gboolean      active;

  g_print ("Cell toggled !\n");

  if (!ctk_tree_model_get_iter_from_string (model, &iter, path))
    return;

  ctk_tree_model_get (model, &iter, FOCUS_COLUMN_CHECK, &active, -1);
  ctk_list_store_set (CTK_LIST_STORE (model), &iter, FOCUS_COLUMN_CHECK, !active, -1);
}

static void
cell_edited (CtkCellRendererToggle *cell_renderer,
	     const gchar           *path,
	     const gchar           *new_text,
	     CtkIconView      *iconview)
{
  CtkTreeModel *model = ctk_icon_view_get_model (iconview);
  CtkTreeIter   iter;

  g_print ("Cell edited with new text '%s' !\n", new_text);

  if (!ctk_tree_model_get_iter_from_string (model, &iter, path))
    return;

  ctk_list_store_set (CTK_LIST_STORE (model), &iter, FOCUS_COLUMN_NAME, new_text, -1);
}

static CtkWidget *
focus_iconview (gboolean color_bg, CtkCellRenderer **focus, CtkCellRenderer **sibling)
{
  CtkTreeModel *model;
  CtkWidget *iconview;
  CtkCellArea *area;
  CtkCellRenderer *renderer, *toggle;

  iconview = ctk_icon_view_new ();
  ctk_widget_show (iconview);

  model = focus_list_model ();

  ctk_icon_view_set_model (CTK_ICON_VIEW (iconview), model);
  ctk_icon_view_set_item_orientation (CTK_ICON_VIEW (iconview), CTK_ORIENTATION_HORIZONTAL);

  area = ctk_cell_layout_get_area (CTK_CELL_LAYOUT (iconview));

  renderer = ctk_cell_renderer_text_new ();
  g_object_set (G_OBJECT (renderer), "editable", TRUE, NULL);
  ctk_cell_area_box_pack_start (CTK_CELL_AREA_BOX (area), renderer, TRUE, FALSE, FALSE);
  ctk_cell_area_attribute_connect (area, renderer, "text", FOCUS_COLUMN_NAME);

  if (color_bg)
    g_object_set (G_OBJECT (renderer), "cell-background", "red", NULL);

  g_signal_connect (G_OBJECT (renderer), "edited",
		    G_CALLBACK (cell_edited), iconview);

  toggle = renderer = ctk_cell_renderer_toggle_new ();
  g_object_set (G_OBJECT (renderer), "xalign", 0.0F, NULL);
  ctk_cell_area_box_pack_start (CTK_CELL_AREA_BOX (area), renderer, FALSE, TRUE, FALSE);
  ctk_cell_area_attribute_connect (area, renderer, "active", FOCUS_COLUMN_CHECK);

  if (color_bg)
    g_object_set (G_OBJECT (renderer), "cell-background", "green", NULL);

  if (focus)
    *focus = renderer;

  g_signal_connect (G_OBJECT (renderer), "toggled",
		    G_CALLBACK (cell_toggled), iconview);

  renderer = ctk_cell_renderer_text_new ();
  g_object_set (G_OBJECT (renderer), 
		"wrap-mode", PANGO_WRAP_WORD,
		"wrap-width", 150,
		NULL);

  if (color_bg)
    g_object_set (G_OBJECT (renderer), "cell-background", "blue", NULL);

  if (sibling)
    *sibling = renderer;

  ctk_cell_area_box_pack_start (CTK_CELL_AREA_BOX (area), renderer, FALSE, TRUE, FALSE);
  ctk_cell_area_attribute_connect (area, renderer, "text", FOCUS_COLUMN_STATIC_TEXT);

  ctk_cell_area_add_focus_sibling (area, toggle, renderer);

  return iconview;
}

static void
focus_sibling_toggled (CtkToggleButton  *toggle,
		       CtkIconView *iconview)
{
  CtkCellArea *area = ctk_cell_layout_get_area (CTK_CELL_LAYOUT (iconview));
  gboolean     active = ctk_toggle_button_get_active (toggle);

  if (active)
    ctk_cell_area_add_focus_sibling (area, focus_renderer, sibling_renderer);
  else
    ctk_cell_area_remove_focus_sibling (area, focus_renderer, sibling_renderer);

  ctk_widget_queue_draw (CTK_WIDGET (iconview));
}


static void
focus_cell_area (void)
{
  CtkWidget *window, *widget;
  CtkWidget *iconview, *frame, *vbox, *hbox;

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  hbox  = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 4);
  ctk_widget_show (hbox);

  ctk_window_set_title (CTK_WINDOW (window), "Focus and editable cells");

  iconview = focus_iconview (FALSE, &focus_renderer, &sibling_renderer);

  frame = ctk_frame_new (NULL);
  ctk_widget_show (frame);

  ctk_widget_set_valign (frame, CTK_ALIGN_CENTER);
  ctk_widget_set_halign (frame, CTK_ALIGN_FILL);

  ctk_container_add (CTK_CONTAINER (frame), iconview);

  ctk_box_pack_end (CTK_BOX (hbox), frame, TRUE, TRUE, 0);

  /* Now add some controls */
  vbox  = ctk_box_new (CTK_ORIENTATION_VERTICAL, 4);
  ctk_widget_show (vbox);
  ctk_box_pack_end (CTK_BOX (hbox), vbox, FALSE, FALSE, 0);

  widget = ctk_combo_box_text_new ();
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (widget), "Horizontal");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (widget), "Vertical");
  ctk_combo_box_set_active (CTK_COMBO_BOX (widget), 0);
  ctk_widget_show (widget);
  ctk_box_pack_start (CTK_BOX (vbox), widget, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (widget), "changed",
                    G_CALLBACK (orientation_changed), iconview);

  widget = ctk_check_button_new_with_label ("Focus Sibling");
  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (widget), TRUE);
  ctk_widget_show (widget);
  ctk_box_pack_start (CTK_BOX (vbox), widget, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (widget), "toggled",
                    G_CALLBACK (focus_sibling_toggled), iconview);

  ctk_container_add (CTK_CONTAINER (window), hbox);

  ctk_widget_show (window);
}



/*******************************************************
 *                  Background Area                    *
 *******************************************************/
static void
cell_spacing_changed (CtkSpinButton    *spin_button,
		      CtkIconView *iconview)
{
  CtkCellArea *area = ctk_cell_layout_get_area (CTK_CELL_LAYOUT (iconview));
  gint        value;

  value = (gint)ctk_spin_button_get_value (spin_button);

  ctk_cell_area_box_set_spacing (CTK_CELL_AREA_BOX (area), value);
}

static void
row_spacing_changed (CtkSpinButton    *spin_button,
		     CtkIconView *iconview)
{
  gint value;

  value = (gint)ctk_spin_button_get_value (spin_button);

  ctk_icon_view_set_row_spacing (iconview, value);
}

static void
item_padding_changed (CtkSpinButton    *spin_button,
		     CtkIconView *iconview)
{
  gint value;

  value = (gint)ctk_spin_button_get_value (spin_button);

  ctk_icon_view_set_item_padding (iconview, value);
}

static void
background_area (void)
{
  CtkWidget *window, *widget, *label, *main_vbox;
  CtkWidget *iconview, *frame, *vbox, *hbox;

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  hbox  = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 4);
  main_vbox  = ctk_box_new (CTK_ORIENTATION_VERTICAL, 4);
  ctk_widget_show (hbox);
  ctk_widget_show (main_vbox);
  ctk_container_add (CTK_CONTAINER (window), main_vbox);

  ctk_window_set_title (CTK_WINDOW (window), "Background Area");

  label = ctk_label_new ("In this example, row spacing gets devided into the background area, "
			 "column spacing is added between each background area, item_padding is "
			 "prepended space distributed to the background area.");
  ctk_label_set_line_wrap  (CTK_LABEL (label), TRUE);
  ctk_label_set_width_chars  (CTK_LABEL (label), 40);
  ctk_widget_show (label);
  ctk_box_pack_start (CTK_BOX (main_vbox), label, FALSE, FALSE, 0);

  iconview = focus_iconview (TRUE, NULL, NULL);

  frame = ctk_frame_new (NULL);
  ctk_widget_show (frame);

  ctk_widget_set_valign (frame, CTK_ALIGN_CENTER);
  ctk_widget_set_halign (frame, CTK_ALIGN_FILL);

  ctk_container_add (CTK_CONTAINER (frame), iconview);

  ctk_box_pack_end (CTK_BOX (hbox), frame, TRUE, TRUE, 0);

  /* Now add some controls */
  vbox  = ctk_box_new (CTK_ORIENTATION_VERTICAL, 4);
  ctk_widget_show (vbox);
  ctk_box_pack_end (CTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  ctk_box_pack_start (CTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);

  widget = ctk_combo_box_text_new ();
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (widget), "Horizontal");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (widget), "Vertical");
  ctk_combo_box_set_active (CTK_COMBO_BOX (widget), 0);
  ctk_widget_show (widget);
  ctk_box_pack_start (CTK_BOX (vbox), widget, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (widget), "changed",
                    G_CALLBACK (orientation_changed), iconview);

  widget = ctk_spin_button_new_with_range (0, 10, 1);
  label = ctk_label_new ("Cell spacing");
  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 4);
  ctk_widget_show (hbox);
  ctk_widget_show (label);
  ctk_widget_show (widget);
  ctk_box_pack_start (CTK_BOX (hbox), label, TRUE, TRUE, 0);
  ctk_box_pack_start (CTK_BOX (hbox), widget, FALSE, FALSE, 0);
  ctk_box_pack_start (CTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (widget), "value-changed",
                    G_CALLBACK (cell_spacing_changed), iconview);


  widget = ctk_spin_button_new_with_range (0, 10, 1);
  ctk_spin_button_set_value (CTK_SPIN_BUTTON (widget), ctk_icon_view_get_row_spacing (CTK_ICON_VIEW (iconview)));
  label = ctk_label_new ("Row spacing");
  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 4);
  ctk_widget_show (hbox);
  ctk_widget_show (label);
  ctk_widget_show (widget);
  ctk_box_pack_start (CTK_BOX (hbox), label, TRUE, TRUE, 0);
  ctk_box_pack_start (CTK_BOX (hbox), widget, FALSE, FALSE, 0);
  ctk_box_pack_start (CTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (widget), "value-changed",
                    G_CALLBACK (row_spacing_changed), iconview);

  widget = ctk_spin_button_new_with_range (0, 30, 1);
  label = ctk_label_new ("Item padding");
  ctk_spin_button_set_value (CTK_SPIN_BUTTON (widget), ctk_icon_view_get_item_padding (CTK_ICON_VIEW (iconview)));
  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 4);
  ctk_widget_show (hbox);
  ctk_widget_show (label);
  ctk_widget_show (widget);
  ctk_box_pack_start (CTK_BOX (hbox), label, TRUE, TRUE, 0);
  ctk_box_pack_start (CTK_BOX (hbox), widget, FALSE, FALSE, 0);
  ctk_box_pack_start (CTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (widget), "value-changed",
                    G_CALLBACK (item_padding_changed), iconview);

  ctk_widget_show (window);
}






int
main (int argc, char *argv[])
{
  ctk_init (NULL, NULL);

  if (g_getenv ("RTL"))
    ctk_widget_set_default_direction (CTK_TEXT_DIR_RTL);

  simple_cell_area ();
  focus_cell_area ();
  background_area ();

  ctk_main ();

  return 0;
}
