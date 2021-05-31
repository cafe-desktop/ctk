#include <gtk/gtk.h>
#ifdef GDK_WINDOWING_X11
#include <X11/Xlib.h>
#endif

static GtkWidget *darea;
static GtkTreeStore *window_store = NULL;
static GtkWidget *treeview;

static void update_store (void);

static GtkWidget *main_window;


GdkWindow *
create_window (GdkWindow *parent,
	       int x, int y, int w, int h,
	       GdkRGBA *color)
{
  GdkWindowAttr attributes;
  gint attributes_mask;
  GdkWindow *window;
  GdkRGBA *bg;

  attributes.x = x;
  attributes.y = y;
  attributes.width = w;
  attributes.height = h;
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.event_mask = GDK_STRUCTURE_MASK
			| GDK_BUTTON_MOTION_MASK
			| GDK_BUTTON_PRESS_MASK
			| GDK_BUTTON_RELEASE_MASK
			| GDK_EXPOSURE_MASK
			| GDK_ENTER_NOTIFY_MASK
			| GDK_LEAVE_NOTIFY_MASK;
  attributes.wclass = GDK_INPUT_OUTPUT;
      
  attributes_mask = GDK_WA_X | GDK_WA_Y;
      
  window = gdk_window_new (parent, &attributes, attributes_mask);
  gdk_window_set_user_data (window, darea);

  bg = g_new (GdkRGBA, 1);
  if (color)
    *bg = *color;
  else
    {
      bg->red = g_random_double ();
      bg->blue = g_random_double ();
      bg->green = g_random_double ();
      bg->alpha = 1.0;
    }
  
  gdk_window_set_background_rgba (window, bg);
  g_object_set_data_full (G_OBJECT (window), "color", bg, g_free);
  
  gdk_window_show (window);
  
  return window;
}

static void
add_window_cb (GtkTreeModel      *model,
	       GtkTreePath       *path,
	       GtkTreeIter       *iter,
	       gpointer           data)
{
  GList **selected = data;
  GdkWindow *window;

  ctk_tree_model_get (GTK_TREE_MODEL (window_store),
		      iter,
		      0, &window,
		      -1);

  *selected = g_list_prepend (*selected, window);
}

static GList *
get_selected_windows (void)
{
  GtkTreeSelection *sel;
  GList *selected;

  sel = ctk_tree_view_get_selection (GTK_TREE_VIEW (treeview));

  selected = NULL;
  ctk_tree_selection_selected_foreach (sel, add_window_cb, &selected);
  
  return selected;
}

static gboolean
find_window_helper (GtkTreeModel *model,
		    GdkWindow *window,
		    GtkTreeIter *iter,
		    GtkTreeIter *selected_iter)
{
  GtkTreeIter child_iter;
  GdkWindow *w;

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
find_window (GdkWindow *window,
	     GtkTreeIter *window_iter)
{
  GtkTreeIter iter;

  if (!ctk_tree_model_get_iter_first  (GTK_TREE_MODEL (window_store), &iter))
    return FALSE;

  return find_window_helper (GTK_TREE_MODEL (window_store),
			     window,
			     &iter,
			     window_iter);
}

static void
toggle_selection_window (GdkWindow *window)
{
  GtkTreeSelection *selection;
  GtkTreeIter iter;

  selection = ctk_tree_view_get_selection (GTK_TREE_VIEW (treeview));

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
  GtkTreeSelection *selection;

  selection = ctk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
  
  ctk_tree_selection_unselect_all (selection);
}


static void
select_window (GdkWindow *window)
{
  GtkTreeSelection *selection;
  GtkTreeIter iter;

  selection = ctk_tree_view_get_selection (GTK_TREE_VIEW (treeview));

  if (window != NULL &&
      find_window (window, &iter))
    ctk_tree_selection_select_iter (selection,  &iter);
}

static void
select_windows (GList *windows)
{
  GtkTreeSelection *selection;
  GtkTreeIter iter;
  GList *l;

  selection = ctk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
  ctk_tree_selection_unselect_all (selection);
  
  for (l = windows; l != NULL; l = l->next)
    {
      if (find_window (l->data, &iter))
	ctk_tree_selection_select_iter (selection,  &iter);
    }
}

static void
add_window_clicked (GtkWidget *button, 
		    gpointer data)
{
  GdkWindow *parent;
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
remove_window_clicked (GtkWidget *button, 
		       gpointer data)
{
  GList *l, *selected;

  selected = get_selected_windows ();

  for (l = selected; l != NULL; l = l->next)
    gdk_window_destroy (l->data);

  g_list_free (selected);

  update_store ();
}

static void save_children (GString *s, GdkWindow *window);

static void
save_window (GString *s,
	     GdkWindow *window)
{
  gint x, y;
  GdkRGBA *color;

  gdk_window_get_position (window, &x, &y);
  color = g_object_get_data (G_OBJECT (window), "color");
  
  g_string_append_printf (s, "%d,%d %dx%d (%f,%f,%f,%f) %d %d\n",
			  x, y,
                          gdk_window_get_width (window),
                          gdk_window_get_height (window),
			  color->red, color->green, color->blue, color->alpha,
			  gdk_window_has_native (window),
			  g_list_length (gdk_window_peek_children (window)));

  save_children (s, window);
}


static void
save_children (GString *s,
	       GdkWindow *window)
{
  GList *l;
  GdkWindow *child;

  for (l = g_list_reverse (gdk_window_peek_children (window));
       l != NULL;
       l = l->next)
    {
      child = l->data;

      save_window (s, child);
    }
}


static void
refresh_clicked (GtkWidget *button, 
		 gpointer data)
{
  ctk_widget_queue_draw (darea);
}

static void
save_clicked (GtkWidget *button, 
	      gpointer data)
{
  GString *s;
  GtkWidget *dialog;
  GFile *file;

  s = g_string_new ("");

  save_children (s, ctk_widget_get_window (darea));

  dialog = ctk_file_chooser_dialog_new ("Filename for window data",
					NULL,
					GTK_FILE_CHOOSER_ACTION_SAVE,
					"_Cancel", GTK_RESPONSE_CANCEL,
					"_Save", GTK_RESPONSE_ACCEPT,
					NULL);
  
  ctk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);
  
  if (ctk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
      file = ctk_file_chooser_get_file (GTK_FILE_CHOOSER (dialog));

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
destroy_children (GdkWindow *window)
{
  GList *l;
  GdkWindow *child;

  for (l = gdk_window_peek_children (window);
       l != NULL;
       l = l->next)
    {
      child = l->data;
      
      destroy_children (child);
      gdk_window_destroy (child);
    }
}

static char **
parse_window (GdkWindow *parent, char **lines)
{
  int x, y, w, h, native, n_children;
  double r, g, b, a;
  GdkWindow *window;
  GdkRGBA color;
  int i;

  if (*lines == NULL)
    return lines;
  
  if (sscanf(*lines, "%d,%d %dx%d (%lf,%lf,%lf,%lf) %d %d",
	     &x, &y, &w, &h, &r, &g, &b, &a, &native, &n_children) == 10)
    {
      lines++;
      color.red = r;
      color.green = g;
      color.blue = b;
      color.alpha = a;
      window = create_window (parent, x, y, w, h, &color);
      if (native)
	gdk_window_ensure_native (window);
      
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
  GdkWindow *window;
  char *data;
  char **lines, **l;
  
  if (g_file_load_contents (file, NULL, &data, NULL, NULL, NULL))
    {
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
move_window_clicked (GtkWidget *button, 
		     gpointer data)
{
  GdkWindow *window;
  GtkDirectionType direction;
  GList *selected, *l;
  gint x, y;

  direction = GPOINTER_TO_INT (data);
    
  selected = get_selected_windows ();

  for (l = selected; l != NULL; l = l->next)
    {
      window = l->data;
      
      gdk_window_get_position (window, &x, &y);
      
      switch (direction) {
      case GTK_DIR_UP:
	y -= 10;
	break;
      case GTK_DIR_DOWN:
	y += 10;
	break;
      case GTK_DIR_LEFT:
	x -= 10;
	break;
      case GTK_DIR_RIGHT:
	x += 10;
	break;
      default:
	break;
      }

      gdk_window_move (window, x, y);
    }

  g_list_free (selected);
}

static void
manual_clicked (GtkWidget *button, 
		gpointer data)
{
  GdkWindow *window;
  GList *selected, *l;
  int x, y, w, h;
  GtkWidget *dialog, *grid, *label, *xspin, *yspin, *wspin, *hspin;
  

  selected = get_selected_windows ();

  if (selected == NULL)
    return;

  gdk_window_get_position (selected->data, &x, &y);
  w = gdk_window_get_width (selected->data);
  h = gdk_window_get_height (selected->data);

  dialog = ctk_dialog_new_with_buttons ("Select new position and size",
					GTK_WINDOW (main_window),
					GTK_DIALOG_MODAL,
					"_OK", GTK_RESPONSE_OK,
					NULL);
  

  grid = ctk_grid_new ();
  ctk_box_pack_start (GTK_BOX (ctk_dialog_get_content_area (GTK_DIALOG (dialog))),
		      grid,
		      FALSE, FALSE,
		      2);

  
  label = ctk_label_new ("x:");
  ctk_widget_set_halign (label, GTK_ALIGN_START);
  ctk_grid_attach (GTK_GRID (grid), label, 0, 0, 1, 1);
  label = ctk_label_new ("y:");
  ctk_widget_set_halign (label, GTK_ALIGN_START);
  ctk_grid_attach (GTK_GRID (grid), label, 0, 1, 1, 1);
  label = ctk_label_new ("width:");
  ctk_widget_set_halign (label, GTK_ALIGN_START);
  ctk_grid_attach (GTK_GRID (grid), label, 0, 2, 1, 1);
  label = ctk_label_new ("height:");
  ctk_widget_set_halign (label, GTK_ALIGN_START);
  ctk_grid_attach (GTK_GRID (grid), label, 0, 3, 1, 1);

  xspin = ctk_spin_button_new_with_range (G_MININT, G_MAXINT, 1);
  ctk_widget_set_hexpand (xspin, TRUE);
  ctk_spin_button_set_value (GTK_SPIN_BUTTON (xspin), x);
  ctk_grid_attach (GTK_GRID (grid), xspin, 1, 0, 1, 1);
  yspin = ctk_spin_button_new_with_range (G_MININT, G_MAXINT, 1);
  ctk_widget_set_hexpand (yspin, TRUE);
  ctk_spin_button_set_value (GTK_SPIN_BUTTON (yspin), y);
  ctk_grid_attach (GTK_GRID (grid), yspin, 1, 1, 1, 1);
  wspin = ctk_spin_button_new_with_range (G_MININT, G_MAXINT, 1);
  ctk_widget_set_hexpand (wspin, TRUE);
  ctk_spin_button_set_value (GTK_SPIN_BUTTON (wspin), w);
  ctk_grid_attach (GTK_GRID (grid), wspin, 1, 2, 1, 1);
  hspin = ctk_spin_button_new_with_range (G_MININT, G_MAXINT, 1);
  ctk_widget_set_hexpand (hspin, TRUE);
  ctk_spin_button_set_value (GTK_SPIN_BUTTON (hspin), h);
  ctk_grid_attach (GTK_GRID (grid), hspin, 1, 3, 1, 1);
  
  ctk_widget_show_all (dialog);
  
  ctk_dialog_run (GTK_DIALOG (dialog));

  x = ctk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (xspin));
  y = ctk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (yspin));
  w = ctk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (wspin));
  h = ctk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (hspin));

  ctk_widget_destroy (dialog);
  
  for (l = selected; l != NULL; l = l->next)
    {
      window = l->data;
      
      gdk_window_move_resize (window, x, y, w, h);
    }

  g_list_free (selected);
}

static void
restack_clicked (GtkWidget *button,
		 gpointer data)
{
  GList *selected;

  selected = get_selected_windows ();

  if (g_list_length (selected) != 2)
    {
      g_warning ("select two windows");
    }

  gdk_window_restack (selected->data,
		      selected->next->data,
		      GPOINTER_TO_INT (data));

  g_list_free (selected);

  update_store ();
}

static void
scroll_window_clicked (GtkWidget *button, 
		       gpointer data)
{
  GdkWindow *window;
  GtkDirectionType direction;
  GList *selected, *l;
  gint dx, dy;

  direction = GPOINTER_TO_INT (data);
    
  selected = get_selected_windows ();

  dx = 0; dy = 0;
  switch (direction) {
  case GTK_DIR_UP:
    dy = 10;
    break;
  case GTK_DIR_DOWN:
    dy = -10;
    break;
  case GTK_DIR_LEFT:
    dx = 10;
    break;
  case GTK_DIR_RIGHT:
    dx = -10;
    break;
  default:
    break;
  }
  
  for (l = selected; l != NULL; l = l->next)
    {
      window = l->data;

      gdk_window_scroll (window, dx, dy);
    }

  g_list_free (selected);
}


static void
raise_window_clicked (GtkWidget *button, 
		      gpointer data)
{
  GList *selected, *l;
  GdkWindow *window;
    
  selected = get_selected_windows ();

  for (l = selected; l != NULL; l = l->next)
    {
      window = l->data;
      
      gdk_window_raise (window);
    }

  g_list_free (selected);
  
  update_store ();
}

static void
lower_window_clicked (GtkWidget *button, 
		      gpointer data)
{
  GList *selected, *l;
  GdkWindow *window;
    
  selected = get_selected_windows ();

  for (l = selected; l != NULL; l = l->next)
    {
      window = l->data;
      
      gdk_window_lower (window);
    }

  g_list_free (selected);
  
  update_store ();
}


static void
smaller_window_clicked (GtkWidget *button, 
			gpointer data)
{
  GList *selected, *l;
  GdkWindow *window;
  int w, h;

  selected = get_selected_windows ();

  for (l = selected; l != NULL; l = l->next)
    {
      window = l->data;
      
      w = gdk_window_get_width (window) - 10;
      h = gdk_window_get_height (window) - 10;
      if (w < 1)
	w = 1;
      if (h < 1)
	h = 1;
      
      gdk_window_resize (window, w, h);
    }

  g_list_free (selected);
}

static void
larger_window_clicked (GtkWidget *button, 
			gpointer data)
{
  GList *selected, *l;
  GdkWindow *window;
  int w, h;

  selected = get_selected_windows ();

  for (l = selected; l != NULL; l = l->next)
    {
      window = l->data;
      
      w = gdk_window_get_width (window) + 10;
      h = gdk_window_get_height (window) + 10;
      
      gdk_window_resize (window, w, h);
    }

  g_list_free (selected);
}

static void
native_window_clicked (GtkWidget *button, 
			gpointer data)
{
  GList *selected, *l;
  GdkWindow *window;

  selected = get_selected_windows ();

  for (l = selected; l != NULL; l = l->next)
    {
      window = l->data;
      
      gdk_window_ensure_native (window);
    }
  
  g_list_free (selected);
  
  update_store ();
}

static void
alpha_clicked (GtkWidget *button, 
	       gpointer data)
{
  GList *selected, *l;
  GdkWindow *window;
  GdkRGBA *color;

  selected = get_selected_windows ();

  for (l = selected; l != NULL; l = l->next)
    {
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

      gdk_window_set_background_rgba (window, color);
    }
  
  g_list_free (selected);
  
  update_store ();
}

static gboolean
darea_button_release_event (GtkWidget *widget,
			    GdkEventButton *event)
{
  if ((event->state & GDK_CONTROL_MASK) != 0)
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
render_window_cell (GtkTreeViewColumn *tree_column,
		    GtkCellRenderer   *cell,
		    GtkTreeModel      *tree_model,
		    GtkTreeIter       *iter,
		    gpointer           data)
{
  GdkWindow *window;
  char *name;

  ctk_tree_model_get (GTK_TREE_MODEL (window_store),
		      iter,
		      0, &window,
		      -1);

  if (gdk_window_has_native (window))
      name = g_strdup_printf ("%p (native)", window);
  else
      name = g_strdup_printf ("%p", window);

  g_object_set (cell,
		"text", name,
		NULL);
}

static void
add_children (GtkTreeStore *store,
	      GdkWindow *window,
	      GtkTreeIter *window_iter)
{
  GList *l;
  GtkTreeIter child_iter;

  for (l = gdk_window_peek_children (window);
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
  ctk_tree_view_expand_all (GTK_TREE_VIEW (treeview));

  select_windows (selected);
  g_list_free (selected);
}


int
main (int argc, char **argv)
{
  GtkWidget *window, *vbox, *hbox, *frame;
  GtkWidget *button, *scrolled, *grid;
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  GFile *file;
  
  ctk_init (&argc, &argv);

  main_window = window = ctk_window_new (GTK_WINDOW_TOPLEVEL);
  ctk_container_set_border_width (GTK_CONTAINER (window), 0);

  g_signal_connect (G_OBJECT (window), "delete-event", ctk_main_quit, NULL);

  hbox = ctk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
  ctk_container_add (GTK_CONTAINER (window), hbox);
  ctk_widget_show (hbox);

  frame = ctk_frame_new ("GdkWindows");
  ctk_box_pack_start (GTK_BOX (hbox),
		      frame,
		      FALSE, FALSE,
		      5);
  ctk_widget_show (frame);

  darea =  ctk_drawing_area_new ();
  ctk_widget_add_events (darea, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
  ctk_widget_set_size_request (darea, 500, 500);
  g_signal_connect (darea, "button_release_event", 
		    G_CALLBACK (darea_button_release_event), 
		    NULL);

  
  ctk_container_add (GTK_CONTAINER (frame), darea);
  ctk_widget_realize (darea);
  ctk_widget_show (darea);


  vbox = ctk_box_new (GTK_ORIENTATION_VERTICAL, 5);
  ctk_box_pack_start (GTK_BOX (hbox),
		      vbox,
		      FALSE, FALSE,
		      5);
  ctk_widget_show (vbox);

  window_store = ctk_tree_store_new (1, GDK_TYPE_WINDOW);
  
  treeview = ctk_tree_view_new_with_model (GTK_TREE_MODEL (window_store));
  ctk_tree_selection_set_mode (ctk_tree_view_get_selection (GTK_TREE_VIEW (treeview)),
			       GTK_SELECTION_MULTIPLE);
  column = ctk_tree_view_column_new ();
  ctk_tree_view_column_set_title (column, "Window");
  renderer = ctk_cell_renderer_text_new ();
  ctk_tree_view_column_pack_start (column, renderer, TRUE);
  ctk_tree_view_column_set_cell_data_func (column,
					   renderer,
					   render_window_cell,
					   NULL, NULL);

  ctk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);


  scrolled = ctk_scrolled_window_new (NULL, NULL);
  ctk_widget_set_size_request (scrolled, 200, 400);
  ctk_container_add (GTK_CONTAINER (scrolled), treeview);
  ctk_box_pack_start (GTK_BOX (vbox),
		      scrolled,
		      FALSE, FALSE,
		      5);
  ctk_widget_show (scrolled);
  ctk_widget_show (treeview);
  
  grid = ctk_grid_new ();
  ctk_grid_set_row_homogeneous (GTK_GRID (grid), TRUE);
  ctk_grid_set_column_homogeneous (GTK_GRID (grid), TRUE);
  ctk_box_pack_start (GTK_BOX (vbox),
		      grid,
		      FALSE, FALSE,
		      2);
  ctk_widget_show (grid);

  button = ctk_button_new ();
  ctk_button_set_image (GTK_BUTTON (button),
			ctk_image_new_from_icon_name ("go-previous-symbolic",
                                                      GTK_ICON_SIZE_BUTTON));
  g_signal_connect (button, "clicked", 
		    G_CALLBACK (move_window_clicked), 
		    GINT_TO_POINTER (GTK_DIR_LEFT));
  ctk_grid_attach (GTK_GRID (grid), button, 0, 1, 1, 1);
  ctk_widget_show (button);

  button = ctk_button_new ();
  ctk_button_set_image (GTK_BUTTON (button),
			ctk_image_new_from_icon_name ("go-up-symbolic",
                                                      GTK_ICON_SIZE_BUTTON));
  g_signal_connect (button, "clicked", 
		    G_CALLBACK (move_window_clicked), 
		    GINT_TO_POINTER (GTK_DIR_UP));
  ctk_grid_attach (GTK_GRID (grid), button, 1, 0, 1, 1);
  ctk_widget_show (button);

  button = ctk_button_new ();
  ctk_button_set_image (GTK_BUTTON (button),
			ctk_image_new_from_icon_name ("go-next-symbolic",
                                                      GTK_ICON_SIZE_BUTTON));
  g_signal_connect (button, "clicked", 
		    G_CALLBACK (move_window_clicked), 
		    GINT_TO_POINTER (GTK_DIR_RIGHT));
  ctk_grid_attach (GTK_GRID (grid), button, 2, 1, 1, 1);
  ctk_widget_show (button);

  button = ctk_button_new ();
  ctk_button_set_image (GTK_BUTTON (button),
			ctk_image_new_from_icon_name ("go-down-symbolic",
                                                      GTK_ICON_SIZE_BUTTON));
  g_signal_connect (button, "clicked", 
		    G_CALLBACK (move_window_clicked), 
		    GINT_TO_POINTER (GTK_DIR_DOWN));
  ctk_grid_attach (GTK_GRID (grid), button, 1, 2, 1, 1);
  ctk_widget_show (button);


  button = ctk_button_new_with_label ("Raise");
  g_signal_connect (button, "clicked", 
		    G_CALLBACK (raise_window_clicked), 
		    NULL);
  ctk_grid_attach (GTK_GRID (grid), button, 0, 0, 1, 1);
  ctk_widget_show (button);

  button = ctk_button_new_with_label ("Lower");
  g_signal_connect (button, "clicked", 
		    G_CALLBACK (lower_window_clicked), 
		    NULL);
  ctk_grid_attach (GTK_GRID (grid), button, 0, 2, 1, 1);
  ctk_widget_show (button);


  button = ctk_button_new_with_label ("Smaller");
  g_signal_connect (button, "clicked", 
		    G_CALLBACK (smaller_window_clicked), 
		    NULL);
  ctk_grid_attach (GTK_GRID (grid), button, 2, 0, 1, 1);
  ctk_widget_show (button);

  button = ctk_button_new_with_label ("Larger");
  g_signal_connect (button, "clicked", 
		    G_CALLBACK (larger_window_clicked), 
		    NULL);
  ctk_grid_attach (GTK_GRID (grid), button, 2, 2, 1, 1);
  ctk_widget_show (button);

  button = ctk_button_new_with_label ("Native");
  g_signal_connect (button, "clicked", 
		    G_CALLBACK (native_window_clicked), 
		    NULL);
  ctk_grid_attach (GTK_GRID (grid), button, 1, 1, 1, 1);
  ctk_widget_show (button);


  button = ctk_button_new_with_label ("scroll");
  ctk_button_set_image (GTK_BUTTON (button),
			ctk_image_new_from_icon_name ("go-up-symbolic",
                                                      GTK_ICON_SIZE_BUTTON));
  g_signal_connect (button, "clicked", 
		    G_CALLBACK (scroll_window_clicked), 
		    GINT_TO_POINTER (GTK_DIR_UP));
  ctk_grid_attach (GTK_GRID (grid), button, 3, 0, 1, 1);
  ctk_widget_show (button);

  button = ctk_button_new_with_label ("scroll");
  ctk_button_set_image (GTK_BUTTON (button),
			ctk_image_new_from_icon_name ("go-down-symbolic",
                                                      GTK_ICON_SIZE_BUTTON));
  g_signal_connect (button, "clicked", 
		    G_CALLBACK (scroll_window_clicked), 
		    GINT_TO_POINTER (GTK_DIR_DOWN));
  ctk_grid_attach (GTK_GRID (grid), button, 3, 1, 1, 1);
  ctk_widget_show (button);

  button = ctk_button_new_with_label ("Manual");
  g_signal_connect (button, "clicked", 
		    G_CALLBACK (manual_clicked),
		    NULL);
  ctk_grid_attach (GTK_GRID (grid), button, 3, 2, 1, 1);
  ctk_widget_show (button);

  button = ctk_button_new_with_label ("More transparent");
  g_signal_connect (button, "clicked",
		    G_CALLBACK (alpha_clicked),
		    GINT_TO_POINTER (-1));
  ctk_grid_attach (GTK_GRID (grid), button, 0, 3, 1, 1);
  ctk_widget_show (button);

  button = ctk_button_new_with_label ("Less transparent");
  g_signal_connect (button, "clicked",
		    G_CALLBACK (alpha_clicked),
		    GINT_TO_POINTER (1));
  ctk_grid_attach (GTK_GRID (grid), button, 1, 3, 1, 1);
  ctk_widget_show (button);

  button = ctk_button_new_with_label ("Restack above");
  g_signal_connect (button, "clicked",
		    G_CALLBACK (restack_clicked),
		    GINT_TO_POINTER (1));
  ctk_grid_attach (GTK_GRID (grid), button, 2, 3, 1, 1);
  ctk_widget_show (button);

  button = ctk_button_new_with_label ("Restack below");
  g_signal_connect (button, "clicked",
		    G_CALLBACK (restack_clicked),
		    0);
  ctk_grid_attach (GTK_GRID (grid), button, 3, 3, 1, 1);
  ctk_widget_show (button);

  button = ctk_button_new_with_label ("Add window");
  ctk_box_pack_start (GTK_BOX (vbox),
		      button,
		      FALSE, FALSE,
		      2);
  ctk_widget_show (button);
  g_signal_connect (button, "clicked", 
		    G_CALLBACK (add_window_clicked), 
		    NULL);
  
  button = ctk_button_new_with_label ("Remove window");
  ctk_box_pack_start (GTK_BOX (vbox),
		      button,
		      FALSE, FALSE,
		      2);
  ctk_widget_show (button);
  g_signal_connect (button, "clicked", 
		    G_CALLBACK (remove_window_clicked), 
		    NULL);

  button = ctk_button_new_with_label ("Save");
  ctk_box_pack_start (GTK_BOX (vbox),
		      button,
		      FALSE, FALSE,
		      2);
  ctk_widget_show (button);
  g_signal_connect (button, "clicked", 
		    G_CALLBACK (save_clicked), 
		    NULL);

  button = ctk_button_new_with_label ("Refresh");
  ctk_box_pack_start (GTK_BOX (vbox),
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
      file = g_file_new_for_commandline_arg (argv[1]);
      load_file (file);
      g_object_unref (file);
    }
  
  ctk_main ();

  return 0;
}
