#include <ctk/ctk.h>
#ifdef CDK_WINDOWING_X11
#include <X11/Xlib.h>
#endif

static CtkWidget *darea;
static CtkTreeStore *window_store = NULL;
static CtkWidget *treeview;

static void update_store (void);

static CtkWidget *main_window;


CdkWindow *
create_window (CdkWindow *parent,
	       int x, int y, int w, int h,
	       CdkRGBA *color)
{
  CdkWindowAttr attributes;
  gint attributes_mask;
  CdkWindow *window;
  CdkRGBA *bg;

  attributes.x = x;
  attributes.y = y;
  attributes.width = w;
  attributes.height = h;
  attributes.window_type = CDK_WINDOW_CHILD;
  attributes.event_mask = CDK_STRUCTURE_MASK
			| CDK_BUTTON_MOTION_MASK
			| CDK_BUTTON_PRESS_MASK
			| CDK_BUTTON_RELEASE_MASK
			| CDK_EXPOSURE_MASK
			| CDK_ENTER_NOTIFY_MASK
			| CDK_LEAVE_NOTIFY_MASK;
  attributes.wclass = CDK_INPUT_OUTPUT;
      
  attributes_mask = CDK_WA_X | CDK_WA_Y;
      
  window = cdk_window_new (parent, &attributes, attributes_mask);
  cdk_window_set_user_data (window, darea);

  bg = g_new (CdkRGBA, 1);
  if (color)
    *bg = *color;
  else
    {
      bg->red = g_random_double ();
      bg->blue = g_random_double ();
      bg->green = g_random_double ();
      bg->alpha = 1.0;
    }
  
  cdk_window_set_background_rgba (window, bg);
  g_object_set_data_full (G_OBJECT (window), "color", bg, g_free);
  
  cdk_window_show (window);
  
  return window;
}

static void
add_window_cb (CtkTreeModel      *model,
	       CtkTreePath       *path,
	       CtkTreeIter       *iter,
	       gpointer           data)
{
  GList **selected = data;
  CdkWindow *window;

  ctk_tree_model_get (CTK_TREE_MODEL (window_store),
		      iter,
		      0, &window,
		      -1);

  *selected = g_list_prepend (*selected, window);
}

static GList *
get_selected_windows (void)
{
  CtkTreeSelection *sel;
  GList *selected;

  sel = ctk_tree_view_get_selection (CTK_TREE_VIEW (treeview));

  selected = NULL;
  ctk_tree_selection_selected_foreach (sel, add_window_cb, &selected);
  
  return selected;
}

static gboolean
find_window_helper (CtkTreeModel *model,
		    CdkWindow *window,
		    CtkTreeIter *iter,
		    CtkTreeIter *selected_iter)
{
  CtkTreeIter child_iter;
  CdkWindow *w;

  do
    {
      ctk_tree_model_get (model, iter,
			  0, &w,
			  -1);
      if (w == window)
	{
	  *selected_iter = *iter;
	  return TRUE;
	}
      
      if (ctk_tree_model_iter_children (model,
					&child_iter,
					iter))
	{
	  if (find_window_helper (model, window, &child_iter, selected_iter))
	    return TRUE;
	}
    } while (ctk_tree_model_iter_next (model, iter));

  return FALSE;
}

static gboolean
find_window (CdkWindow *window,
	     CtkTreeIter *window_iter)
{
  CtkTreeIter iter;

  if (!ctk_tree_model_get_iter_first  (CTK_TREE_MODEL (window_store), &iter))
    return FALSE;

  return find_window_helper (CTK_TREE_MODEL (window_store),
			     window,
			     &iter,
			     window_iter);
}

static void
toggle_selection_window (CdkWindow *window)
{
  CtkTreeSelection *selection;
  CtkTreeIter iter;

  selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (treeview));

  if (window != NULL &&
      find_window (window, &iter))
    {
      if (ctk_tree_selection_iter_is_selected (selection, &iter))
	ctk_tree_selection_unselect_iter (selection,  &iter);
      else
	ctk_tree_selection_select_iter (selection,  &iter);
    }
}

static void
unselect_windows (void)
{
  CtkTreeSelection *selection;

  selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (treeview));
  
  ctk_tree_selection_unselect_all (selection);
}


static void
select_window (CdkWindow *window)
{
  CtkTreeSelection *selection;
  CtkTreeIter iter;

  selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (treeview));

  if (window != NULL &&
      find_window (window, &iter))
    ctk_tree_selection_select_iter (selection,  &iter);
}

static void
select_windows (GList *windows)
{
  CtkTreeSelection *selection;
  CtkTreeIter iter;
  GList *l;

  selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (treeview));
  ctk_tree_selection_unselect_all (selection);
  
  for (l = windows; l != NULL; l = l->next)
    {
      if (find_window (l->data, &iter))
	ctk_tree_selection_select_iter (selection,  &iter);
    }
}

static void
add_window_clicked (CtkWidget *button, 
		    gpointer data)
{
  CdkWindow *parent;
  GList *l;

  l = get_selected_windows ();
  if (l != NULL)
    parent = l->data;
  else
    parent = ctk_widget_get_window (darea);

  g_list_free (l);
  
  create_window (parent, 10, 10, 100, 100, NULL);
  update_store ();
}

static void
remove_window_clicked (CtkWidget *button, 
		       gpointer data)
{
  GList *l, *selected;

  selected = get_selected_windows ();

  for (l = selected; l != NULL; l = l->next)
    cdk_window_destroy (l->data);

  g_list_free (selected);

  update_store ();
}

static void save_children (GString *s, CdkWindow *window);

static void
save_window (GString *s,
	     CdkWindow *window)
{
  gint x, y;
  CdkRGBA *color;

  cdk_window_get_position (window, &x, &y);
  color = g_object_get_data (G_OBJECT (window), "color");
  
  g_string_append_printf (s, "%d,%d %dx%d (%f,%f,%f,%f) %d %d\n",
			  x, y,
                          cdk_window_get_width (window),
                          cdk_window_get_height (window),
			  color->red, color->green, color->blue, color->alpha,
			  cdk_window_has_native (window),
			  g_list_length (cdk_window_peek_children (window)));

  save_children (s, window);
}


static void
save_children (GString *s,
	       CdkWindow *window)
{
  GList *l;

  for (l = g_list_reverse (cdk_window_peek_children (window));
       l != NULL;
       l = l->next)
    {
      CdkWindow *child;

      child = l->data;

      save_window (s, child);
    }
}


static void
refresh_clicked (CtkWidget *button, 
		 gpointer data)
{
  ctk_widget_queue_draw (darea);
}

static void
save_clicked (CtkWidget *button, 
	      gpointer data)
{
  GString *s;
  CtkWidget *dialog;

  s = g_string_new ("");

  save_children (s, ctk_widget_get_window (darea));

  dialog = ctk_file_chooser_dialog_new ("Filename for window data",
					NULL,
					CTK_FILE_CHOOSER_ACTION_SAVE,
					"_Cancel", CTK_RESPONSE_CANCEL,
					"_Save", CTK_RESPONSE_ACCEPT,
					NULL);
  
  ctk_file_chooser_set_do_overwrite_confirmation (CTK_FILE_CHOOSER (dialog), TRUE);
  
  if (ctk_dialog_run (CTK_DIALOG (dialog)) == CTK_RESPONSE_ACCEPT)
    {
      GFile *file;

      file = ctk_file_chooser_get_file (CTK_FILE_CHOOSER (dialog));

      g_file_replace_contents (file,
			       s->str, s->len,
			       NULL, FALSE,
			       0, NULL, NULL, NULL);

      g_object_unref (file);
    }

  ctk_widget_destroy (dialog);
  g_string_free (s, TRUE);
}

static void
destroy_children (CdkWindow *window)
{
  GList *l;

  for (l = cdk_window_peek_children (window);
       l != NULL;
       l = l->next)
    {
      CdkWindow *child;

      child = l->data;
      
      destroy_children (child);
      cdk_window_destroy (child);
    }
}

static char **
parse_window (CdkWindow *parent, char **lines)
{
  int x, y, w, h, native, n_children;
  double r, g, b, a;
  CdkRGBA color;

  if (*lines == NULL)
    return lines;
  
  if (sscanf(*lines, "%d,%d %dx%d (%lf,%lf,%lf,%lf) %d %d",
	     &x, &y, &w, &h, &r, &g, &b, &a, &native, &n_children) == 10)
    {
      CdkWindow *window;
      int i;

      lines++;
      color.red = r;
      color.green = g;
      color.blue = b;
      color.alpha = a;
      window = create_window (parent, x, y, w, h, &color);
      if (native)
	cdk_window_ensure_native (window);
      
      for (i = 0; i < n_children; i++)
	lines = parse_window (window, lines);
    }
  else
    lines++;
  
  return lines;
}
  
static void
load_file (GFile *file)
{
  char *data;
  char **lines;
  
  if (g_file_load_contents (file, NULL, &data, NULL, NULL, NULL))
    {
      CdkWindow *window;
      char **l;

      window = ctk_widget_get_window (darea);

      destroy_children (window);

      lines = g_strsplit (data, "\n", -1);

      l = lines;
      while (*l != NULL)
	l = parse_window (window, l);
    }

  update_store ();
}

static void
move_window_clicked (CtkWidget *button, 
		     gpointer data)
{
  CtkDirectionType direction;
  GList *selected, *l;
  gint x, y;

  direction = GPOINTER_TO_INT (data);
    
  selected = get_selected_windows ();

  for (l = selected; l != NULL; l = l->next)
    {
      CdkWindow *window;

      window = l->data;
      
      cdk_window_get_position (window, &x, &y);
      
      switch (direction) {
      case CTK_DIR_UP:
	y -= 10;
	break;
      case CTK_DIR_DOWN:
	y += 10;
	break;
      case CTK_DIR_LEFT:
	x -= 10;
	break;
      case CTK_DIR_RIGHT:
	x += 10;
	break;
      default:
	break;
      }

      cdk_window_move (window, x, y);
    }

  g_list_free (selected);
}

static void
manual_clicked (CtkWidget *button, 
		gpointer data)
{
  GList *selected, *l;
  int x, y, w, h;
  CtkWidget *dialog, *grid, *label, *xspin, *yspin, *wspin, *hspin;
  

  selected = get_selected_windows ();

  if (selected == NULL)
    return;

  cdk_window_get_position (selected->data, &x, &y);
  w = cdk_window_get_width (selected->data);
  h = cdk_window_get_height (selected->data);

  dialog = ctk_dialog_new_with_buttons ("Select new position and size",
					CTK_WINDOW (main_window),
					CTK_DIALOG_MODAL,
					"_OK", CTK_RESPONSE_OK,
					NULL);
  

  grid = ctk_grid_new ();
  ctk_box_pack_start (CTK_BOX (ctk_dialog_get_content_area (CTK_DIALOG (dialog))),
		      grid,
		      FALSE, FALSE,
		      2);

  
  label = ctk_label_new ("x:");
  ctk_widget_set_halign (label, CTK_ALIGN_START);
  ctk_grid_attach (CTK_GRID (grid), label, 0, 0, 1, 1);
  label = ctk_label_new ("y:");
  ctk_widget_set_halign (label, CTK_ALIGN_START);
  ctk_grid_attach (CTK_GRID (grid), label, 0, 1, 1, 1);
  label = ctk_label_new ("width:");
  ctk_widget_set_halign (label, CTK_ALIGN_START);
  ctk_grid_attach (CTK_GRID (grid), label, 0, 2, 1, 1);
  label = ctk_label_new ("height:");
  ctk_widget_set_halign (label, CTK_ALIGN_START);
  ctk_grid_attach (CTK_GRID (grid), label, 0, 3, 1, 1);

  xspin = ctk_spin_button_new_with_range (G_MININT, G_MAXINT, 1);
  ctk_widget_set_hexpand (xspin, TRUE);
  ctk_spin_button_set_value (CTK_SPIN_BUTTON (xspin), x);
  ctk_grid_attach (CTK_GRID (grid), xspin, 1, 0, 1, 1);
  yspin = ctk_spin_button_new_with_range (G_MININT, G_MAXINT, 1);
  ctk_widget_set_hexpand (yspin, TRUE);
  ctk_spin_button_set_value (CTK_SPIN_BUTTON (yspin), y);
  ctk_grid_attach (CTK_GRID (grid), yspin, 1, 1, 1, 1);
  wspin = ctk_spin_button_new_with_range (G_MININT, G_MAXINT, 1);
  ctk_widget_set_hexpand (wspin, TRUE);
  ctk_spin_button_set_value (CTK_SPIN_BUTTON (wspin), w);
  ctk_grid_attach (CTK_GRID (grid), wspin, 1, 2, 1, 1);
  hspin = ctk_spin_button_new_with_range (G_MININT, G_MAXINT, 1);
  ctk_widget_set_hexpand (hspin, TRUE);
  ctk_spin_button_set_value (CTK_SPIN_BUTTON (hspin), h);
  ctk_grid_attach (CTK_GRID (grid), hspin, 1, 3, 1, 1);
  
  ctk_widget_show_all (dialog);
  
  ctk_dialog_run (CTK_DIALOG (dialog));

  x = ctk_spin_button_get_value_as_int (CTK_SPIN_BUTTON (xspin));
  y = ctk_spin_button_get_value_as_int (CTK_SPIN_BUTTON (yspin));
  w = ctk_spin_button_get_value_as_int (CTK_SPIN_BUTTON (wspin));
  h = ctk_spin_button_get_value_as_int (CTK_SPIN_BUTTON (hspin));

  ctk_widget_destroy (dialog);
  
  for (l = selected; l != NULL; l = l->next)
    {
      CdkWindow *window;

      window = l->data;
      
      cdk_window_move_resize (window, x, y, w, h);
    }

  g_list_free (selected);
}

static void
restack_clicked (CtkWidget *button,
		 gpointer data)
{
  GList *selected;

  selected = get_selected_windows ();

  if (g_list_length (selected) != 2)
    {
      g_warning ("select two windows");
    }

  cdk_window_restack (selected->data,
		      selected->next->data,
		      GPOINTER_TO_INT (data));

  g_list_free (selected);

  update_store ();
}

static void
scroll_window_clicked (CtkWidget *button, 
		       gpointer data)
{
  CtkDirectionType direction;
  GList *selected, *l;
  gint dx, dy;

  direction = GPOINTER_TO_INT (data);
    
  selected = get_selected_windows ();

  dx = 0; dy = 0;
  switch (direction) {
  case CTK_DIR_UP:
    dy = 10;
    break;
  case CTK_DIR_DOWN:
    dy = -10;
    break;
  case CTK_DIR_LEFT:
    dx = 10;
    break;
  case CTK_DIR_RIGHT:
    dx = -10;
    break;
  default:
    break;
  }
  
  for (l = selected; l != NULL; l = l->next)
    {
      CdkWindow *window;

      window = l->data;

      cdk_window_scroll (window, dx, dy);
    }

  g_list_free (selected);
}


static void
raise_window_clicked (CtkWidget *button, 
		      gpointer data)
{
  GList *selected, *l;
    
  selected = get_selected_windows ();

  for (l = selected; l != NULL; l = l->next)
    {
      CdkWindow *window;

      window = l->data;
      
      cdk_window_raise (window);
    }

  g_list_free (selected);
  
  update_store ();
}

static void
lower_window_clicked (CtkWidget *button, 
		      gpointer data)
{
  GList *selected, *l;
    
  selected = get_selected_windows ();

  for (l = selected; l != NULL; l = l->next)
    {
      CdkWindow *window;

      window = l->data;
      
      cdk_window_lower (window);
    }

  g_list_free (selected);
  
  update_store ();
}


static void
smaller_window_clicked (CtkWidget *button, 
			gpointer data)
{
  GList *selected, *l;

  selected = get_selected_windows ();

  for (l = selected; l != NULL; l = l->next)
    {
      CdkWindow *window;
      int w, h;

      window = l->data;
      
      w = cdk_window_get_width (window) - 10;
      h = cdk_window_get_height (window) - 10;
      if (w < 1)
	w = 1;
      if (h < 1)
	h = 1;
      
      cdk_window_resize (window, w, h);
    }

  g_list_free (selected);
}

static void
larger_window_clicked (CtkWidget *button, 
			gpointer data)
{
  GList *selected, *l;

  selected = get_selected_windows ();

  for (l = selected; l != NULL; l = l->next)
    {
      CdkWindow *window;
      int w, h;

      window = l->data;
      
      w = cdk_window_get_width (window) + 10;
      h = cdk_window_get_height (window) + 10;
      
      cdk_window_resize (window, w, h);
    }

  g_list_free (selected);
}

static void
native_window_clicked (CtkWidget *button, 
			gpointer data)
{
  GList *selected, *l;

  selected = get_selected_windows ();

  for (l = selected; l != NULL; l = l->next)
    {
      CdkWindow *window;

      window = l->data;
      
      cdk_window_ensure_native (window);
    }
  
  g_list_free (selected);
  
  update_store ();
}

static void
alpha_clicked (CtkWidget *button, 
	       gpointer data)
{
  GList *selected, *l;

  selected = get_selected_windows ();

  for (l = selected; l != NULL; l = l->next)
    {
      CdkWindow *window;
      CdkRGBA *color;

      window = l->data;

      color = g_object_get_data (G_OBJECT (window), "color");
      if (GPOINTER_TO_INT(data) > 0)
	color->alpha += 0.2;
      else
	color->alpha -= 0.2;

      if (color->alpha < 0)
	color->alpha = 0;
      if (color->alpha > 1)
	color->alpha = 1;

      cdk_window_set_background_rgba (window, color);
    }
  
  g_list_free (selected);
  
  update_store ();
}

static gboolean
darea_button_release_event (CtkWidget *widget,
			    CdkEventButton *event)
{
  if ((event->state & CDK_CONTROL_MASK) != 0)
    {
      toggle_selection_window (event->window);
    }
  else
    {
      unselect_windows ();
      select_window (event->window);
    }
    
  return TRUE;
}

static void
render_window_cell (CtkTreeViewColumn *tree_column,
		    CtkCellRenderer   *cell,
		    CtkTreeModel      *tree_model,
		    CtkTreeIter       *iter,
		    gpointer           data)
{
  CdkWindow *window;
  char *name;

  ctk_tree_model_get (CTK_TREE_MODEL (window_store),
		      iter,
		      0, &window,
		      -1);

  if (cdk_window_has_native (window))
      name = g_strdup_printf ("%p (native)", window);
  else
      name = g_strdup_printf ("%p", window);

  g_object_set (cell,
		"text", name,
		NULL);
}

static void
add_children (CtkTreeStore *store,
	      CdkWindow *window,
	      CtkTreeIter *window_iter)
{
  GList *l;
  CtkTreeIter child_iter;

  for (l = cdk_window_peek_children (window);
       l != NULL;
       l = l->next)
    {
      ctk_tree_store_append (store, &child_iter, window_iter);
      ctk_tree_store_set (store, &child_iter,
			  0, l->data,
			  -1);

      add_children (store, l->data, &child_iter);
    }
}

static void
update_store (void)
{
  GList *selected;

  selected = get_selected_windows ();

  ctk_tree_store_clear (window_store);

  add_children (window_store, ctk_widget_get_window (darea), NULL);
  ctk_tree_view_expand_all (CTK_TREE_VIEW (treeview));

  select_windows (selected);
  g_list_free (selected);
}


int
main (int argc, char **argv)
{
  CtkWidget *window, *vbox, *hbox, *frame;
  CtkWidget *button, *scrolled, *grid;
  CtkTreeViewColumn *column;
  CtkCellRenderer *renderer;
  
  ctk_init (&argc, &argv);

  main_window = window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_container_set_border_width (CTK_CONTAINER (window), 0);

  g_signal_connect (G_OBJECT (window), "delete-event", ctk_main_quit, NULL);

  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 5);
  ctk_container_add (CTK_CONTAINER (window), hbox);
  ctk_widget_show (hbox);

  frame = ctk_frame_new ("CdkWindows");
  ctk_box_pack_start (CTK_BOX (hbox),
		      frame,
		      FALSE, FALSE,
		      5);
  ctk_widget_show (frame);

  darea =  ctk_drawing_area_new ();
  ctk_widget_add_events (darea, CDK_BUTTON_PRESS_MASK | CDK_BUTTON_RELEASE_MASK);
  ctk_widget_set_size_request (darea, 500, 500);
  g_signal_connect (darea, "button_release_event", 
		    G_CALLBACK (darea_button_release_event), 
		    NULL);

  
  ctk_container_add (CTK_CONTAINER (frame), darea);
  ctk_widget_realize (darea);
  ctk_widget_show (darea);


  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 5);
  ctk_box_pack_start (CTK_BOX (hbox),
		      vbox,
		      FALSE, FALSE,
		      5);
  ctk_widget_show (vbox);

  window_store = ctk_tree_store_new (1, CDK_TYPE_WINDOW);
  
  treeview = ctk_tree_view_new_with_model (CTK_TREE_MODEL (window_store));
  ctk_tree_selection_set_mode (ctk_tree_view_get_selection (CTK_TREE_VIEW (treeview)),
			       CTK_SELECTION_MULTIPLE);
  column = ctk_tree_view_column_new ();
  ctk_tree_view_column_set_title (column, "Window");
  renderer = ctk_cell_renderer_text_new ();
  ctk_tree_view_column_pack_start (column, renderer, TRUE);
  ctk_tree_view_column_set_cell_data_func (column,
					   renderer,
					   render_window_cell,
					   NULL, NULL);

  ctk_tree_view_append_column (CTK_TREE_VIEW (treeview), column);


  scrolled = ctk_scrolled_window_new (NULL, NULL);
  ctk_widget_set_size_request (scrolled, 200, 400);
  ctk_container_add (CTK_CONTAINER (scrolled), treeview);
  ctk_box_pack_start (CTK_BOX (vbox),
		      scrolled,
		      FALSE, FALSE,
		      5);
  ctk_widget_show (scrolled);
  ctk_widget_show (treeview);
  
  grid = ctk_grid_new ();
  ctk_grid_set_row_homogeneous (CTK_GRID (grid), TRUE);
  ctk_grid_set_column_homogeneous (CTK_GRID (grid), TRUE);
  ctk_box_pack_start (CTK_BOX (vbox),
		      grid,
		      FALSE, FALSE,
		      2);
  ctk_widget_show (grid);

  button = ctk_button_new ();
  ctk_button_set_image (CTK_BUTTON (button),
			ctk_image_new_from_icon_name ("go-previous-symbolic",
                                                      CTK_ICON_SIZE_BUTTON));
  g_signal_connect (button, "clicked", 
		    G_CALLBACK (move_window_clicked), 
		    GINT_TO_POINTER (CTK_DIR_LEFT));
  ctk_grid_attach (CTK_GRID (grid), button, 0, 1, 1, 1);
  ctk_widget_show (button);

  button = ctk_button_new ();
  ctk_button_set_image (CTK_BUTTON (button),
			ctk_image_new_from_icon_name ("go-up-symbolic",
                                                      CTK_ICON_SIZE_BUTTON));
  g_signal_connect (button, "clicked", 
		    G_CALLBACK (move_window_clicked), 
		    GINT_TO_POINTER (CTK_DIR_UP));
  ctk_grid_attach (CTK_GRID (grid), button, 1, 0, 1, 1);
  ctk_widget_show (button);

  button = ctk_button_new ();
  ctk_button_set_image (CTK_BUTTON (button),
			ctk_image_new_from_icon_name ("go-next-symbolic",
                                                      CTK_ICON_SIZE_BUTTON));
  g_signal_connect (button, "clicked", 
		    G_CALLBACK (move_window_clicked), 
		    GINT_TO_POINTER (CTK_DIR_RIGHT));
  ctk_grid_attach (CTK_GRID (grid), button, 2, 1, 1, 1);
  ctk_widget_show (button);

  button = ctk_button_new ();
  ctk_button_set_image (CTK_BUTTON (button),
			ctk_image_new_from_icon_name ("go-down-symbolic",
                                                      CTK_ICON_SIZE_BUTTON));
  g_signal_connect (button, "clicked", 
		    G_CALLBACK (move_window_clicked), 
		    GINT_TO_POINTER (CTK_DIR_DOWN));
  ctk_grid_attach (CTK_GRID (grid), button, 1, 2, 1, 1);
  ctk_widget_show (button);


  button = ctk_button_new_with_label ("Raise");
  g_signal_connect (button, "clicked", 
		    G_CALLBACK (raise_window_clicked), 
		    NULL);
  ctk_grid_attach (CTK_GRID (grid), button, 0, 0, 1, 1);
  ctk_widget_show (button);

  button = ctk_button_new_with_label ("Lower");
  g_signal_connect (button, "clicked", 
		    G_CALLBACK (lower_window_clicked), 
		    NULL);
  ctk_grid_attach (CTK_GRID (grid), button, 0, 2, 1, 1);
  ctk_widget_show (button);


  button = ctk_button_new_with_label ("Smaller");
  g_signal_connect (button, "clicked", 
		    G_CALLBACK (smaller_window_clicked), 
		    NULL);
  ctk_grid_attach (CTK_GRID (grid), button, 2, 0, 1, 1);
  ctk_widget_show (button);

  button = ctk_button_new_with_label ("Larger");
  g_signal_connect (button, "clicked", 
		    G_CALLBACK (larger_window_clicked), 
		    NULL);
  ctk_grid_attach (CTK_GRID (grid), button, 2, 2, 1, 1);
  ctk_widget_show (button);

  button = ctk_button_new_with_label ("Native");
  g_signal_connect (button, "clicked", 
		    G_CALLBACK (native_window_clicked), 
		    NULL);
  ctk_grid_attach (CTK_GRID (grid), button, 1, 1, 1, 1);
  ctk_widget_show (button);


  button = ctk_button_new_with_label ("scroll");
  ctk_button_set_image (CTK_BUTTON (button),
			ctk_image_new_from_icon_name ("go-up-symbolic",
                                                      CTK_ICON_SIZE_BUTTON));
  g_signal_connect (button, "clicked", 
		    G_CALLBACK (scroll_window_clicked), 
		    GINT_TO_POINTER (CTK_DIR_UP));
  ctk_grid_attach (CTK_GRID (grid), button, 3, 0, 1, 1);
  ctk_widget_show (button);

  button = ctk_button_new_with_label ("scroll");
  ctk_button_set_image (CTK_BUTTON (button),
			ctk_image_new_from_icon_name ("go-down-symbolic",
                                                      CTK_ICON_SIZE_BUTTON));
  g_signal_connect (button, "clicked", 
		    G_CALLBACK (scroll_window_clicked), 
		    GINT_TO_POINTER (CTK_DIR_DOWN));
  ctk_grid_attach (CTK_GRID (grid), button, 3, 1, 1, 1);
  ctk_widget_show (button);

  button = ctk_button_new_with_label ("Manual");
  g_signal_connect (button, "clicked", 
		    G_CALLBACK (manual_clicked),
		    NULL);
  ctk_grid_attach (CTK_GRID (grid), button, 3, 2, 1, 1);
  ctk_widget_show (button);

  button = ctk_button_new_with_label ("More transparent");
  g_signal_connect (button, "clicked",
		    G_CALLBACK (alpha_clicked),
		    GINT_TO_POINTER (-1));
  ctk_grid_attach (CTK_GRID (grid), button, 0, 3, 1, 1);
  ctk_widget_show (button);

  button = ctk_button_new_with_label ("Less transparent");
  g_signal_connect (button, "clicked",
		    G_CALLBACK (alpha_clicked),
		    GINT_TO_POINTER (1));
  ctk_grid_attach (CTK_GRID (grid), button, 1, 3, 1, 1);
  ctk_widget_show (button);

  button = ctk_button_new_with_label ("Restack above");
  g_signal_connect (button, "clicked",
		    G_CALLBACK (restack_clicked),
		    GINT_TO_POINTER (1));
  ctk_grid_attach (CTK_GRID (grid), button, 2, 3, 1, 1);
  ctk_widget_show (button);

  button = ctk_button_new_with_label ("Restack below");
  g_signal_connect (button, "clicked",
		    G_CALLBACK (restack_clicked),
		    0);
  ctk_grid_attach (CTK_GRID (grid), button, 3, 3, 1, 1);
  ctk_widget_show (button);

  button = ctk_button_new_with_label ("Add window");
  ctk_box_pack_start (CTK_BOX (vbox),
		      button,
		      FALSE, FALSE,
		      2);
  ctk_widget_show (button);
  g_signal_connect (button, "clicked", 
		    G_CALLBACK (add_window_clicked), 
		    NULL);
  
  button = ctk_button_new_with_label ("Remove window");
  ctk_box_pack_start (CTK_BOX (vbox),
		      button,
		      FALSE, FALSE,
		      2);
  ctk_widget_show (button);
  g_signal_connect (button, "clicked", 
		    G_CALLBACK (remove_window_clicked), 
		    NULL);

  button = ctk_button_new_with_label ("Save");
  ctk_box_pack_start (CTK_BOX (vbox),
		      button,
		      FALSE, FALSE,
		      2);
  ctk_widget_show (button);
  g_signal_connect (button, "clicked", 
		    G_CALLBACK (save_clicked), 
		    NULL);

  button = ctk_button_new_with_label ("Refresh");
  ctk_box_pack_start (CTK_BOX (vbox),
		      button,
		      FALSE, FALSE,
		      2);
  ctk_widget_show (button);
  g_signal_connect (button, "clicked", 
		    G_CALLBACK (refresh_clicked), 
		    NULL);

  
  ctk_widget_show (window);

  if (argc == 2)
    {
      GFile *file;

      file = g_file_new_for_commandline_arg (argv[1]);
      load_file (file);
      g_object_unref (file);
    }
  
  ctk_main ();

  return 0;
}
