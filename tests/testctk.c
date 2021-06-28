/* CTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the CTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/. 
 */


#include "config.h"

#undef	G_LOG_DOMAIN

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <math.h>
#include <time.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "ctk/ctk.h"
#include "cdk/cdk.h"
#include "cdk/cdkkeysyms.h"

#ifdef G_OS_WIN32
#define sleep(n) _sleep(n)
#endif

#include "test.xpm"

gboolean
file_exists (const char *filename)
{
  struct stat statbuf;

  return stat (filename, &statbuf) == 0;
}

CtkWidget *
shape_create_icon (CdkScreen *screen,
		   char      *xpm_file,
		   gint       x,
		   gint       y,
		   gint       px,
		   gint       py,
		   gint       window_type);

static CtkWidget *
build_option_menu (gchar           *items[],
		   gint             num_items,
		   gint             history,
		   void           (*func) (CtkWidget *widget, gpointer data),
		   gpointer         data);

/* macro, structure and variables used by tree window demos */
#define DEFAULT_NUMBER_OF_ITEM  3
#define DEFAULT_RECURSION_LEVEL 3

struct {
  GSList* selection_mode_group;
  CtkWidget* single_button;
  CtkWidget* browse_button;
  CtkWidget* multiple_button;
  CtkWidget* draw_line_button;
  CtkWidget* view_line_button;
  CtkWidget* no_root_item_button;
  CtkWidget* nb_item_spinner;
  CtkWidget* recursion_spinner;
} sTreeSampleSelection;

typedef struct sTreeButtons {
  guint nb_item_add;
  CtkWidget* add_button;
  CtkWidget* remove_button;
  CtkWidget* subtree_button;
} sTreeButtons;
/* end of tree section */

static CtkWidget *
build_option_menu (gchar           *items[],
		   gint             num_items,
		   gint             history,
		   void           (*func)(CtkWidget *widget, gpointer data),
		   gpointer         data)
{
  CtkWidget *omenu;
  gint i;

  omenu = ctk_combo_box_text_new ();
  g_signal_connect (omenu, "changed",
		    G_CALLBACK (func), data);
      
  for (i = 0; i < num_items; i++)
      ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (omenu), items[i]);

  ctk_combo_box_set_active (CTK_COMBO_BOX (omenu), history);
  
  return omenu;
}

/*
 * Windows with an alpha channel
 */


static gboolean
on_alpha_window_draw (CtkWidget *widget,
                      cairo_t   *cr)
{
  cairo_pattern_t *pattern;
  int radius, width, height;

  /* Get the child allocation to avoid painting over the borders */
  CtkWidget *child = ctk_bin_get_child (CTK_BIN (widget));
  CtkAllocation child_allocation;
  int border_width = ctk_container_get_border_width (CTK_CONTAINER (child));

  ctk_widget_get_allocation (child, &child_allocation);
  child_allocation.x -= border_width;
  child_allocation.y -= border_width;
  child_allocation.width += 2 * border_width;
  child_allocation.height += 2 * border_width;

  cairo_translate (cr, child_allocation.x, child_allocation.y);

  cairo_rectangle (cr, 0, 0, child_allocation.width, child_allocation.height);
  cairo_clip (cr);

  width = ctk_widget_get_allocated_width (widget);
  height = ctk_widget_get_allocated_height (widget);
  radius = MIN (width, height) / 2;
  pattern = cairo_pattern_create_radial (width / 2,
                                         height / 2,
					 0.0,
                                         width / 2,
                                         height / 2,
					 radius * 1.33);

  if (cdk_screen_get_rgba_visual (ctk_widget_get_screen (widget)) &&
      ctk_widget_is_composited (widget))
    cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.0); /* transparent */
  else
    cairo_set_source_rgb (cr, 1.0, 1.0, 1.0); /* opaque white */
    
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_paint (cr);
  
  cairo_pattern_add_color_stop_rgba (pattern, 0.0,
				     1.0, 0.75, 0.0, 1.0); /* solid orange */
  cairo_pattern_add_color_stop_rgba (pattern, 1.0,
				     1.0, 0.75, 0.0, 0.0); /* transparent orange */

  cairo_set_source (cr, pattern);
  cairo_pattern_destroy (pattern);
  
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  cairo_paint (cr);

  return FALSE;
}

static CtkWidget *
build_alpha_widgets (void)
{
  CtkWidget *grid;
  CtkWidget *radio_button;
  CtkWidget *check_button;
  CtkWidget *hbox;
  CtkWidget *label;
  CtkWidget *entry;

  grid = ctk_grid_new ();

  radio_button = ctk_radio_button_new_with_label (NULL, "Red");
  ctk_widget_set_hexpand (radio_button, TRUE);
  ctk_grid_attach (CTK_GRID (grid), radio_button, 0, 0, 1, 1);

  radio_button = ctk_radio_button_new_with_label_from_widget (CTK_RADIO_BUTTON (radio_button), "Green");
  ctk_widget_set_hexpand (radio_button, TRUE);
  ctk_grid_attach (CTK_GRID (grid), radio_button, 0, 1, 1, 1);

  radio_button = ctk_radio_button_new_with_label_from_widget (CTK_RADIO_BUTTON (radio_button), "Blue"),
  ctk_widget_set_hexpand (radio_button, TRUE);
  ctk_grid_attach (CTK_GRID (grid), radio_button, 0, 2, 1, 1);

  check_button = ctk_check_button_new_with_label ("Sedentary"),
  ctk_widget_set_hexpand (check_button, TRUE);
  ctk_grid_attach (CTK_GRID (grid), check_button, 1, 0, 1, 1);

  check_button = ctk_check_button_new_with_label ("Nocturnal"),
  ctk_widget_set_hexpand (check_button, TRUE);
  ctk_grid_attach (CTK_GRID (grid), check_button, 1, 1, 1, 1);

  check_button = ctk_check_button_new_with_label ("Compulsive"),
  ctk_widget_set_hexpand (check_button, TRUE);
  ctk_grid_attach (CTK_GRID (grid), check_button, 1, 2, 1, 1);

  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
  label = ctk_label_new (NULL);
  ctk_label_set_markup (CTK_LABEL (label), "<i>Entry: </i>");
  ctk_box_pack_start (CTK_BOX (hbox), label, FALSE, FALSE, 0);
  entry = ctk_entry_new ();
  ctk_box_pack_start (CTK_BOX (hbox), entry, TRUE, TRUE, 0);
  ctk_widget_set_hexpand (hbox, TRUE);
  ctk_grid_attach (CTK_GRID (grid), hbox, 0, 3, 2, 1);
  
  return grid;
}

static void
on_alpha_screen_changed (CtkWindow *window,
			 CdkScreen *old_screen,
			 CtkWidget *label)
{
  CdkScreen *screen = ctk_widget_get_screen (CTK_WIDGET (window));
  CdkVisual *visual = cdk_screen_get_rgba_visual (screen);

  if (!visual)
    {
      visual = cdk_screen_get_system_visual (screen);
      ctk_label_set_markup (CTK_LABEL (label), "<b>Screen doesn't support alpha</b>");
    }
  else
    {
      ctk_label_set_markup (CTK_LABEL (label), "<b>Screen supports alpha</b>");
    }

  ctk_widget_set_visual (CTK_WIDGET (window), visual);
}

static void
on_composited_changed (CtkWidget *window,
		      CtkLabel *label)
{
  gboolean is_composited = ctk_widget_is_composited (window);

  if (is_composited)
    ctk_label_set_text (label, "Composited");
  else
    ctk_label_set_text (label, "Not composited");
}

void
create_alpha_window (CtkWidget *widget)
{
  static CtkWidget *window;

  if (!window)
    {
      CtkWidget *content_area;
      CtkWidget *vbox;
      CtkWidget *label;
      
      window = ctk_dialog_new_with_buttons ("Alpha Window",
					    CTK_WINDOW (ctk_widget_get_toplevel (widget)), 0,
					    "_Close", 0,
					    NULL);

      ctk_widget_set_app_paintable (window, TRUE);
      g_signal_connect (window, "draw",
			G_CALLBACK (on_alpha_window_draw), NULL);

      content_area = ctk_dialog_get_content_area (CTK_DIALOG (window));
      
      vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 8);
      ctk_container_set_border_width (CTK_CONTAINER (vbox), 12);
      ctk_box_pack_start (CTK_BOX (content_area), vbox,
			  TRUE, TRUE, 0);

      label = ctk_label_new (NULL);
      ctk_box_pack_start (CTK_BOX (vbox), label, TRUE, TRUE, 0);
      on_alpha_screen_changed (CTK_WINDOW (window), NULL, label);
      g_signal_connect (window, "screen-changed",
			G_CALLBACK (on_alpha_screen_changed), label);
      
      label = ctk_label_new (NULL);
      ctk_box_pack_start (CTK_BOX (vbox), label, TRUE, TRUE, 0);
      on_composited_changed (window, CTK_LABEL (label));
      g_signal_connect (window, "composited_changed", G_CALLBACK (on_composited_changed), label);
      
      ctk_box_pack_start (CTK_BOX (vbox), build_alpha_widgets (), TRUE, TRUE, 0);

      g_signal_connect (window, "destroy",
			G_CALLBACK (ctk_widget_destroyed),
			&window);
      
      g_signal_connect (window, "response",
                        G_CALLBACK (ctk_widget_destroy),
                        NULL); 
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);
}

/*
 * Composited non-toplevel window
 */

/* The draw event handler for the event box.
 *
 * This function simply draws a transparency onto a widget on the area
 * for which it receives expose events.  This is intended to give the
 * event box a "transparent" background.
 *
 * In order for this to work properly, the widget must have an RGBA
 * colourmap.  The widget should also be set as app-paintable since it
 * doesn't make sense for CTK to draw a background if we are drawing it
 * (and because CTK might actually replace our transparency with its
 * default background colour).
 */
static gboolean
transparent_draw (CtkWidget *widget,
                  cairo_t   *cr)
{
  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);

  return FALSE;
}

/* The expose event handler for the window.
 *
 * This function performs the actual compositing of the event box onto
 * the already-existing background of the window at 50% normal opacity.
 *
 * In this case we do not want app-paintable to be set on the widget
 * since we want it to draw its own (red) background.  Because of this,
 * however, we must ensure that we use g_signal_register_after so that
 * this handler is called after the red has been drawn.  If it was
 * called before then CTK would just blindly paint over our work.
 */
static gboolean
window_draw (CtkWidget *widget,
             cairo_t   *cr)
{
  CtkAllocation allocation;
  CtkWidget *child;

  /* put a red background on the window */
  cairo_set_source_rgb (cr, 1, 0, 0);
  cairo_paint (cr);

  /* get our child (in this case, the event box) */ 
  child = ctk_bin_get_child (CTK_BIN (widget));

  ctk_widget_get_allocation (child, &allocation);

  /* the source data is the (composited) event box */
  cdk_cairo_set_source_window (cr, ctk_widget_get_window (child),
                               allocation.x,
                               allocation.y);

  /* composite, with a 50% opacity */
  cairo_paint_with_alpha (cr, 0.5);

  return FALSE;
}

void
create_composited_window (CtkWidget *widget)
{
  static CtkWidget *window;

  if (!window)
    {
      CtkWidget *event, *button;

      /* make the widgets */
      button = ctk_button_new_with_label ("A Button");
      event = ctk_event_box_new ();
      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);

      g_signal_connect (window, "destroy",
                        G_CALLBACK (ctk_widget_destroyed),
                        &window);

      /* set our event box to have a fully-transparent background
       * drawn on it.  currently there is no way to simply tell ctk
       * that "transparency" is the background colour for a widget.
       */
      ctk_widget_set_app_paintable (CTK_WIDGET (event), TRUE);
      g_signal_connect (event, "draw",
                        G_CALLBACK (transparent_draw), NULL);

      /* put them inside one another */
      ctk_container_set_border_width (CTK_CONTAINER (window), 10);
      ctk_container_add (CTK_CONTAINER (window), event);
      ctk_container_add (CTK_CONTAINER (event), button);

      /* realise and show everything */
      ctk_widget_realize (button);

      /* set the event box CdkWindow to be composited.
       * obviously must be performed after event box is realised.
       */
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      cdk_window_set_composited (ctk_widget_get_window (event),
                                 TRUE);
G_GNUC_END_IGNORE_DEPRECATIONS

      /* set up the compositing handler.
       * note that we do _after so that the normal (red) background is drawn
       * by ctk before our compositing occurs.
       */
      g_signal_connect_after (window, "draw",
                              G_CALLBACK (window_draw), NULL);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);
}

/*
 * Big windows and guffaw scrolling
 */

static void
pattern_set_bg (CtkWidget   *widget,
		CdkWindow   *child,
		gint         level)
{
  static CdkRGBA colors[] = {
    { 0.27, 0.27, 1.0, 1.0 },
    { 0.53, 0.53, 1.0, 1.0},
    { 0.67, 0.67, 1.0, 1.0 }
  };
    
  cdk_window_set_user_data (child, widget);
  cdk_window_set_background_rgba (child, &colors[level]);
}

static void
create_pattern (CtkWidget   *widget,
		CdkWindow   *parent,
		gint         level,
		gint         width,
		gint         height)
{
  gint h = 1;
  gint i = 0;
    
  CdkWindow *child;

  while (2 * h <= height)
    {
      gint w = 1;
      gint j = 0;
      
      while (2 * w <= width)
	{
	  if ((i + j) % 2 == 0)
	    {
	      gint x = w  - 1;
	      gint y = h - 1;
	      
	      CdkWindowAttr attributes;

	      attributes.window_type = CDK_WINDOW_CHILD;
	      attributes.x = x;
	      attributes.y = y;
	      attributes.width = w;
	      attributes.height = h;
	      attributes.wclass = CDK_INPUT_OUTPUT;
	      attributes.event_mask = CDK_EXPOSURE_MASK;
	      attributes.visual = ctk_widget_get_visual (widget);
	      
	      child = cdk_window_new (parent, &attributes,
				      CDK_WA_X | CDK_WA_Y | CDK_WA_VISUAL);

	      pattern_set_bg (widget, child, level);

	      if (level < 2)
		create_pattern (widget, child, level + 1, w, h);

	      cdk_window_show (child);
	    }
	  j++;
	  w *= 2;
	}
      i++;
      h *= 2;
    }
}

#define PATTERN_SIZE (1 << 18)

static void
pattern_hadj_changed (CtkAdjustment *adjustment,
		      CtkWidget     *darea)
{
  gint *old_value = g_object_get_data (G_OBJECT (adjustment), "old-value");
  gint new_value = ctk_adjustment_get_value (adjustment);

  if (ctk_widget_get_realized (darea))
    {
      cdk_window_scroll (ctk_widget_get_window (darea),
                         *old_value - new_value, 0);
      *old_value = new_value;
    }
}

static void
pattern_vadj_changed (CtkAdjustment *adjustment,
		      CtkWidget *darea)
{
  gint *old_value = g_object_get_data (G_OBJECT (adjustment), "old-value");
  gint new_value = ctk_adjustment_get_value (adjustment);

  if (ctk_widget_get_realized (darea))
    {
      cdk_window_scroll (ctk_widget_get_window (darea),
                         0, *old_value - new_value);
      *old_value = new_value;
    }
}

static void
pattern_realize (CtkWidget *widget,
		 gpointer   data)
{
  CdkWindow *window;

  window = ctk_widget_get_window (widget);
  pattern_set_bg (widget, window, 0);
  create_pattern (widget, window, 1, PATTERN_SIZE, PATTERN_SIZE);
}

static void 
create_big_windows (CtkWidget *widget)
{
  static CtkWidget *window = NULL;
  CtkWidget *content_area;
  CtkWidget *darea, *grid, *scrollbar;
  CtkWidget *eventbox;
  CtkAdjustment *hadjustment;
  CtkAdjustment *vadjustment;
  static gint current_x;
  static gint current_y;
 
  if (!window)
    {
      current_x = 0;
      current_y = 0;
      
      window = ctk_dialog_new_with_buttons ("Big Windows",
                                            NULL, 0,
                                            "_Close",
                                            CTK_RESPONSE_NONE,
                                            NULL);
 
      ctk_window_set_screen (CTK_WINDOW (window),
			     ctk_widget_get_screen (widget));

      ctk_window_set_default_size (CTK_WINDOW (window), 200, 300);

      g_signal_connect (window, "destroy",
			G_CALLBACK (ctk_widget_destroyed),
			&window);

      g_signal_connect (window, "response",
                        G_CALLBACK (ctk_widget_destroy),
                        NULL);

      content_area = ctk_dialog_get_content_area (CTK_DIALOG (window));

      grid = ctk_grid_new ();
      ctk_box_pack_start (CTK_BOX (content_area), grid, TRUE, TRUE, 0);

      darea = ctk_drawing_area_new ();

      hadjustment = ctk_adjustment_new (0, 0, PATTERN_SIZE, 10, 100, 100);
      g_signal_connect (hadjustment, "value_changed",
			G_CALLBACK (pattern_hadj_changed), darea);
      g_object_set_data (G_OBJECT (hadjustment), "old-value", &current_x);

      vadjustment = ctk_adjustment_new (0, 0, PATTERN_SIZE, 10, 100, 100);
      g_signal_connect (vadjustment, "value_changed",
			G_CALLBACK (pattern_vadj_changed), darea);
      g_object_set_data (G_OBJECT (vadjustment), "old-value", &current_y);
      
      g_signal_connect (darea, "realize",
                        G_CALLBACK (pattern_realize),
                        NULL);

      eventbox = ctk_event_box_new ();
      ctk_widget_set_hexpand (eventbox, TRUE);
      ctk_widget_set_vexpand (eventbox, TRUE);
      ctk_grid_attach (CTK_GRID (grid), eventbox, 0, 0, 1, 1);

      ctk_container_add (CTK_CONTAINER (eventbox), darea);

      scrollbar = ctk_scrollbar_new (CTK_ORIENTATION_HORIZONTAL, hadjustment);
      ctk_widget_set_hexpand (scrollbar, TRUE);
      ctk_grid_attach (CTK_GRID (grid), scrollbar, 0, 1, 1, 1);

      scrollbar = ctk_scrollbar_new (CTK_ORIENTATION_VERTICAL, vadjustment);
      ctk_widget_set_vexpand (scrollbar, TRUE);
      ctk_grid_attach (CTK_GRID (grid), scrollbar, 1, 0, 1, 1);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_hide (window);
}

/*
 * CtkButton
 */

static void
button_window (CtkWidget *widget,
	       CtkWidget *button)
{
  if (!ctk_widget_get_visible (button))
    ctk_widget_show (button);
  else
    ctk_widget_hide (button);
}

static void
create_buttons (CtkWidget *widget)
{
  static CtkWidget *window = NULL;
  CtkWidget *box1;
  CtkWidget *box2;
  CtkWidget *grid;
  CtkWidget *separator;
  CtkWidget *button[10];
  int button_x[9] = { 0, 1, 2, 0, 2, 1, 1, 2, 0 };
  int button_y[9] = { 0, 1, 2, 2, 0, 2, 0, 1, 1 };
  guint i;

  if (!window)
    {
      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_screen (CTK_WINDOW (window),
			     ctk_widget_get_screen (widget));

      g_signal_connect (window, "destroy",
			G_CALLBACK (ctk_widget_destroyed),
			&window);

      ctk_window_set_title (CTK_WINDOW (window), "CtkButton");
      ctk_container_set_border_width (CTK_CONTAINER (window), 0);

      box1 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
      ctk_container_add (CTK_CONTAINER (window), box1);

      grid = ctk_grid_new ();
      ctk_grid_set_row_spacing (CTK_GRID (grid), 5);
      ctk_grid_set_column_spacing (CTK_GRID (grid), 5);
      ctk_container_set_border_width (CTK_CONTAINER (grid), 10);
      ctk_box_pack_start (CTK_BOX (box1), grid, TRUE, TRUE, 0);

      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      button[0] = ctk_button_new_with_label ("button1");
      button[1] = ctk_button_new_with_mnemonic ("_button2");
      button[2] = ctk_button_new_with_mnemonic ("_button3");
      button[3] = ctk_button_new_from_stock (CTK_STOCK_OK);
      button[4] = ctk_button_new_with_label ("button5");
      button[5] = ctk_button_new_with_label ("button6");
      button[6] = ctk_button_new_with_label ("button7");
      button[7] = ctk_button_new_from_stock (CTK_STOCK_CLOSE);
      button[8] = ctk_button_new_with_label ("button9");
      G_GNUC_END_IGNORE_DEPRECATIONS;
      
      for (i = 0; i < 9; i++)
        {
          g_signal_connect (button[i], "clicked",
                            G_CALLBACK (button_window),
                            button[(i + 1) % 9]);
          ctk_widget_set_hexpand (button[i], TRUE);
          ctk_widget_set_vexpand (button[i], TRUE);

          ctk_grid_attach (CTK_GRID (grid), button[i],
                           button_x[i], button_y[i] + 1, 1, 1);
        }

      separator = ctk_separator_new (CTK_ORIENTATION_HORIZONTAL);
      ctk_box_pack_start (CTK_BOX (box1), separator, FALSE, TRUE, 0);

      box2 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 10);
      ctk_container_set_border_width (CTK_CONTAINER (box2), 10);
      ctk_box_pack_start (CTK_BOX (box1), box2, FALSE, TRUE, 0);

      button[9] = ctk_button_new_with_label ("close");
      g_signal_connect_swapped (button[9], "clicked",
				G_CALLBACK (ctk_widget_destroy),
				window);
      ctk_box_pack_start (CTK_BOX (box2), button[9], TRUE, TRUE, 0);
      ctk_widget_set_can_default (button[9], TRUE);
      ctk_widget_grab_default (button[9]);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);
}

/*
 * CtkToggleButton
 */

static void
create_toggle_buttons (CtkWidget *widget)
{
  static CtkWidget *window = NULL;
  CtkWidget *box1;
  CtkWidget *box2;
  CtkWidget *button;
  CtkWidget *separator;

  if (!window)
    {
      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_screen (CTK_WINDOW (window),
			     ctk_widget_get_screen (widget));

      g_signal_connect (window, "destroy",
			G_CALLBACK (ctk_widget_destroyed),
			&window);

      ctk_window_set_title (CTK_WINDOW (window), "CtkToggleButton");
      ctk_container_set_border_width (CTK_CONTAINER (window), 0);

      box1 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
      ctk_container_add (CTK_CONTAINER (window), box1);

      box2 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 10);
      ctk_container_set_border_width (CTK_CONTAINER (box2), 10);
      ctk_box_pack_start (CTK_BOX (box1), box2, TRUE, TRUE, 0);

      button = ctk_toggle_button_new_with_label ("button1");
      ctk_box_pack_start (CTK_BOX (box2), button, TRUE, TRUE, 0);

      button = ctk_toggle_button_new_with_label ("button2");
      ctk_box_pack_start (CTK_BOX (box2), button, TRUE, TRUE, 0);

      button = ctk_toggle_button_new_with_label ("button3");
      ctk_box_pack_start (CTK_BOX (box2), button, TRUE, TRUE, 0);

      button = ctk_toggle_button_new_with_label ("inconsistent");
      ctk_toggle_button_set_inconsistent (CTK_TOGGLE_BUTTON (button), TRUE);
      ctk_box_pack_start (CTK_BOX (box2), button, TRUE, TRUE, 0);
      
      separator = ctk_separator_new (CTK_ORIENTATION_HORIZONTAL);
      ctk_box_pack_start (CTK_BOX (box1), separator, FALSE, TRUE, 0);

      box2 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 10);
      ctk_container_set_border_width (CTK_CONTAINER (box2), 10);
      ctk_box_pack_start (CTK_BOX (box1), box2, FALSE, TRUE, 0);

      button = ctk_button_new_with_label ("close");
      g_signal_connect_swapped (button, "clicked",
			        G_CALLBACK (ctk_widget_destroy),
				window);
      ctk_box_pack_start (CTK_BOX (box2), button, TRUE, TRUE, 0);
      ctk_widget_set_can_default (button, TRUE);
      ctk_widget_grab_default (button);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);
}

static CtkWidget *
create_widget_grid (GType widget_type)
{
  CtkWidget *grid;
  CtkWidget *group_widget = NULL;
  gint i, j;
  
  grid = ctk_grid_new ();
  
  for (i = 0; i < 5; i++)
    {
      for (j = 0; j < 5; j++)
	{
	  CtkWidget *widget;
	  char *tmp;
	  
	  if (i == 0 && j == 0)
	    {
	      widget = NULL;
	    }
	  else if (i == 0)
	    {
	      tmp = g_strdup_printf ("%d", j);
	      widget = ctk_label_new (tmp);
	      g_free (tmp);
	    }
	  else if (j == 0)
	    {
	      tmp = g_strdup_printf ("%c", 'A' + i - 1);
	      widget = ctk_label_new (tmp);
	      g_free (tmp);
	    }
	  else
	    {
	      widget = g_object_new (widget_type, NULL);
	      
	      if (g_type_is_a (widget_type, CTK_TYPE_RADIO_BUTTON))
		{
		  if (!group_widget)
		    group_widget = widget;
		  else
		    g_object_set (widget, "group", group_widget, NULL);
		}
	    }
	  
	  if (widget)
	    ctk_grid_attach (CTK_GRID (grid), widget, i, j, 1, 1);
	}
    }

  return grid;
}

/*
 * CtkCheckButton
 */

static void
create_check_buttons (CtkWidget *widget)
{
  static CtkWidget *window = NULL;
  CtkWidget *box1;
  CtkWidget *box2;
  CtkWidget *button;
  CtkWidget *separator;
  CtkWidget *table;
  
  if (!window)
    {
      window = ctk_dialog_new_with_buttons ("Check Buttons",
                                            NULL, 0,
                                            "_Close",
                                            CTK_RESPONSE_NONE,
                                            NULL);

      ctk_window_set_screen (CTK_WINDOW (window), 
			     ctk_widget_get_screen (widget));

      g_signal_connect (window, "destroy",
			G_CALLBACK (ctk_widget_destroyed),
			&window);
      g_signal_connect (window, "response",
                        G_CALLBACK (ctk_widget_destroy),
                        NULL);

      box1 = ctk_dialog_get_content_area (CTK_DIALOG (window));
      
      box2 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 10);
      ctk_container_set_border_width (CTK_CONTAINER (box2), 10);
      ctk_box_pack_start (CTK_BOX (box1), box2, TRUE, TRUE, 0);

      button = ctk_check_button_new_with_mnemonic ("_button1");
      ctk_box_pack_start (CTK_BOX (box2), button, TRUE, TRUE, 0);

      button = ctk_check_button_new_with_label ("button2");
      ctk_box_pack_start (CTK_BOX (box2), button, TRUE, TRUE, 0);

      button = ctk_check_button_new_with_label ("button3");
      ctk_box_pack_start (CTK_BOX (box2), button, TRUE, TRUE, 0);

      button = ctk_check_button_new_with_label ("inconsistent");
      ctk_toggle_button_set_inconsistent (CTK_TOGGLE_BUTTON (button), TRUE);
      ctk_box_pack_start (CTK_BOX (box2), button, TRUE, TRUE, 0);

      separator = ctk_separator_new (CTK_ORIENTATION_HORIZONTAL);
      ctk_box_pack_start (CTK_BOX (box1), separator, FALSE, TRUE, 0);

      table = create_widget_grid (CTK_TYPE_CHECK_BUTTON);
      ctk_container_set_border_width (CTK_CONTAINER (table), 10);
      ctk_box_pack_start (CTK_BOX (box1), table, TRUE, TRUE, 0);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);
}

/*
 * CtkRadioButton
 */

static void
create_radio_buttons (CtkWidget *widget)
{
  static CtkWidget *window = NULL;
  CtkWidget *box1;
  CtkWidget *box2;
  CtkWidget *button;
  CtkWidget *separator;
  CtkWidget *table;

  if (!window)
    {
      window = ctk_dialog_new_with_buttons ("Radio Buttons",
                                            NULL, 0,
                                            "_Close",
                                            CTK_RESPONSE_NONE,
                                            NULL);

      ctk_window_set_screen (CTK_WINDOW (window),
			     ctk_widget_get_screen (widget));

      g_signal_connect (window, "destroy",
			G_CALLBACK (ctk_widget_destroyed),
			&window);
      g_signal_connect (window, "response",
                        G_CALLBACK (ctk_widget_destroy),
                        NULL);

      box1 = ctk_dialog_get_content_area (CTK_DIALOG (window));

      box2 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 10);
      ctk_container_set_border_width (CTK_CONTAINER (box2), 10);
      ctk_box_pack_start (CTK_BOX (box1), box2, TRUE, TRUE, 0);

      button = ctk_radio_button_new_with_label (NULL, "button1");
      ctk_box_pack_start (CTK_BOX (box2), button, TRUE, TRUE, 0);

      button = ctk_radio_button_new_with_label (
	         ctk_radio_button_get_group (CTK_RADIO_BUTTON (button)),
		 "button2");
      ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (button), TRUE);
      ctk_box_pack_start (CTK_BOX (box2), button, TRUE, TRUE, 0);

      button = ctk_radio_button_new_with_label (
                 ctk_radio_button_get_group (CTK_RADIO_BUTTON (button)),
		 "button3");
      ctk_box_pack_start (CTK_BOX (box2), button, TRUE, TRUE, 0);

      button = ctk_radio_button_new_with_label (
                 ctk_radio_button_get_group (CTK_RADIO_BUTTON (button)),
		 "inconsistent");
      ctk_toggle_button_set_inconsistent (CTK_TOGGLE_BUTTON (button), TRUE);
      ctk_box_pack_start (CTK_BOX (box2), button, TRUE, TRUE, 0);
      
      separator = ctk_separator_new (CTK_ORIENTATION_HORIZONTAL);
      ctk_box_pack_start (CTK_BOX (box1), separator, FALSE, TRUE, 0);

      box2 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 10);
      ctk_container_set_border_width (CTK_CONTAINER (box2), 10);
      ctk_box_pack_start (CTK_BOX (box1), box2, TRUE, TRUE, 0);

      button = ctk_radio_button_new_with_label (NULL, "button4");
      ctk_toggle_button_set_mode (CTK_TOGGLE_BUTTON (button), FALSE);
      ctk_box_pack_start (CTK_BOX (box2), button, TRUE, TRUE, 0);

      button = ctk_radio_button_new_with_label (
	         ctk_radio_button_get_group (CTK_RADIO_BUTTON (button)),
		 "button5");
      ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (button), TRUE);
      ctk_toggle_button_set_mode (CTK_TOGGLE_BUTTON (button), FALSE);
      ctk_box_pack_start (CTK_BOX (box2), button, TRUE, TRUE, 0);

      button = ctk_radio_button_new_with_label (
                 ctk_radio_button_get_group (CTK_RADIO_BUTTON (button)),
		 "button6");
      ctk_toggle_button_set_mode (CTK_TOGGLE_BUTTON (button), FALSE);
      ctk_box_pack_start (CTK_BOX (box2), button, TRUE, TRUE, 0);

      separator = ctk_separator_new (CTK_ORIENTATION_HORIZONTAL);
      ctk_box_pack_start (CTK_BOX (box1), separator, FALSE, TRUE, 0);

      table = create_widget_grid (CTK_TYPE_RADIO_BUTTON);
      ctk_container_set_border_width (CTK_CONTAINER (table), 10);
      ctk_box_pack_start (CTK_BOX (box1), table, TRUE, TRUE, 0);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);
}

/*
 * CtkButtonBox
 */

static CtkWidget *
create_bbox (gint  horizontal,
	     char* title, 
	     gint  spacing,
	     gint  child_w,
	     gint  child_h,
	     gint  layout)
{
  CtkWidget *frame;
  CtkWidget *bbox;
  CtkWidget *button;
	
  frame = ctk_frame_new (title);

  if (horizontal)
    bbox = ctk_button_box_new (CTK_ORIENTATION_HORIZONTAL);
  else
    bbox = ctk_button_box_new (CTK_ORIENTATION_VERTICAL);

  ctk_container_set_border_width (CTK_CONTAINER (bbox), 5);
  ctk_container_add (CTK_CONTAINER (frame), bbox);

  ctk_button_box_set_layout (CTK_BUTTON_BOX (bbox), layout);
  ctk_box_set_spacing (CTK_BOX (bbox), spacing);
  
  button = ctk_button_new_with_label ("OK");
  ctk_container_add (CTK_CONTAINER (bbox), button);
  
  button = ctk_button_new_with_label ("Cancel");
  ctk_container_add (CTK_CONTAINER (bbox), button);
  
  button = ctk_button_new_with_label ("Help");
  ctk_container_add (CTK_CONTAINER (bbox), button);

  return frame;
}

static void
create_button_box (CtkWidget *widget)
{
  static CtkWidget* window = NULL;
  CtkWidget *main_vbox;
  CtkWidget *vbox;
  CtkWidget *hbox;
  CtkWidget *frame_horz;
  CtkWidget *frame_vert;

  if (!window)
  {
    window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
    ctk_window_set_screen (CTK_WINDOW (window), ctk_widget_get_screen (widget));
    ctk_window_set_title (CTK_WINDOW (window), "Button Boxes");
    
    g_signal_connect (window, "destroy",
		      G_CALLBACK (ctk_widget_destroyed),
		      &window);

    ctk_container_set_border_width (CTK_CONTAINER (window), 10);

    main_vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
    ctk_container_add (CTK_CONTAINER (window), main_vbox);
    
    frame_horz = ctk_frame_new ("Horizontal Button Boxes");
    ctk_box_pack_start (CTK_BOX (main_vbox), frame_horz, TRUE, TRUE, 10);
    
    vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
    ctk_container_set_border_width (CTK_CONTAINER (vbox), 10);
    ctk_container_add (CTK_CONTAINER (frame_horz), vbox);
    
    ctk_box_pack_start (CTK_BOX (vbox), 
                        create_bbox (TRUE, "Spread", 40, 85, 20, CTK_BUTTONBOX_SPREAD),
			TRUE, TRUE, 0);
    
    ctk_box_pack_start (CTK_BOX (vbox), 
                        create_bbox (TRUE, "Edge", 40, 85, 20, CTK_BUTTONBOX_EDGE),
			TRUE, TRUE, 5);
    
    ctk_box_pack_start (CTK_BOX (vbox), 
                        create_bbox (TRUE, "Start", 40, 85, 20, CTK_BUTTONBOX_START),
			TRUE, TRUE, 5);
    
    ctk_box_pack_start (CTK_BOX (vbox), 
                        create_bbox (TRUE, "End", 40, 85, 20, CTK_BUTTONBOX_END),
			TRUE, TRUE, 5);
    
    ctk_box_pack_start (CTK_BOX (vbox),
                        create_bbox (TRUE, "Center", 40, 85, 20, CTK_BUTTONBOX_CENTER),
			TRUE, TRUE, 5);
    
    frame_vert = ctk_frame_new ("Vertical Button Boxes");
    ctk_box_pack_start (CTK_BOX (main_vbox), frame_vert, TRUE, TRUE, 10);
    
    hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
    ctk_container_set_border_width (CTK_CONTAINER (hbox), 10);
    ctk_container_add (CTK_CONTAINER (frame_vert), hbox);

    ctk_box_pack_start (CTK_BOX (hbox), 
                        create_bbox (FALSE, "Spread", 30, 85, 20, CTK_BUTTONBOX_SPREAD),
			TRUE, TRUE, 0);
    
    ctk_box_pack_start (CTK_BOX (hbox), 
                        create_bbox (FALSE, "Edge", 30, 85, 20, CTK_BUTTONBOX_EDGE),
			TRUE, TRUE, 5);
    
    ctk_box_pack_start (CTK_BOX (hbox), 
                        create_bbox (FALSE, "Start", 30, 85, 20, CTK_BUTTONBOX_START),
			TRUE, TRUE, 5);
    
    ctk_box_pack_start (CTK_BOX (hbox), 
                        create_bbox (FALSE, "End", 30, 85, 20, CTK_BUTTONBOX_END),
			TRUE, TRUE, 5);
    
    ctk_box_pack_start (CTK_BOX (hbox),
                        create_bbox (FALSE, "Center", 30, 85, 20, CTK_BUTTONBOX_CENTER),
			TRUE, TRUE, 5);
  }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);
}

/*
 * CtkToolBar
 */

static CtkWidget*
new_pixbuf (char      *filename,
	    CdkWindow *window)
{
  CtkWidget *widget;
  GdkPixbuf *pixbuf;

  if (strcmp (filename, "test.xpm") == 0)
    pixbuf = NULL;
  else
    pixbuf = gdk_pixbuf_new_from_file (filename, NULL);

  if (pixbuf == NULL)
    pixbuf = gdk_pixbuf_new_from_xpm_data ((const char **) openfile);
  
  widget = ctk_image_new_from_pixbuf (pixbuf);

  g_object_unref (pixbuf);

  return widget;
}


static void
set_toolbar_small_stock (CtkWidget *widget,
			 gpointer   data)
{
  ctk_toolbar_set_icon_size (CTK_TOOLBAR (data), CTK_ICON_SIZE_SMALL_TOOLBAR);
}

static void
set_toolbar_large_stock (CtkWidget *widget,
			 gpointer   data)
{
  ctk_toolbar_set_icon_size (CTK_TOOLBAR (data), CTK_ICON_SIZE_LARGE_TOOLBAR);
}

static void
set_toolbar_horizontal (CtkWidget *widget,
			gpointer   data)
{
  ctk_orientable_set_orientation (CTK_ORIENTABLE (data), CTK_ORIENTATION_HORIZONTAL);
}

static void
set_toolbar_vertical (CtkWidget *widget,
		      gpointer   data)
{
  ctk_orientable_set_orientation (CTK_ORIENTABLE (data), CTK_ORIENTATION_VERTICAL);
}

static void
set_toolbar_icons (CtkWidget *widget,
		   gpointer   data)
{
  ctk_toolbar_set_style (CTK_TOOLBAR (data), CTK_TOOLBAR_ICONS);
}

static void
set_toolbar_text (CtkWidget *widget,
	          gpointer   data)
{
  ctk_toolbar_set_style (CTK_TOOLBAR (data), CTK_TOOLBAR_TEXT);
}

static void
set_toolbar_both (CtkWidget *widget,
		  gpointer   data)
{
  ctk_toolbar_set_style (CTK_TOOLBAR (data), CTK_TOOLBAR_BOTH);
}

static void
set_toolbar_both_horiz (CtkWidget *widget,
			gpointer   data)
{
  ctk_toolbar_set_style (CTK_TOOLBAR (data), CTK_TOOLBAR_BOTH_HORIZ);
}

G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
static CtkActionEntry create_toolbar_items[] = {
    { NULL, CTK_STOCK_NEW, NULL, NULL, "Stock icon: New",
      G_CALLBACK (set_toolbar_small_stock) },
    { NULL, CTK_STOCK_OPEN, NULL, NULL, "Stock icon: Open",
      G_CALLBACK (set_toolbar_large_stock) },
    { NULL, NULL, "Horizontal", NULL, "Horizontal toolbar layout",
      G_CALLBACK (set_toolbar_horizontal) },
    { NULL, NULL, "Vertical", NULL, "Vertical toolbar layout",
      G_CALLBACK (set_toolbar_vertical) },
    { NULL },
    { NULL, NULL, "Icons", NULL, "Only show toolbar icons",
      G_CALLBACK (set_toolbar_icons) },
    { NULL, NULL, "Text", NULL, "Only show toolbar text",
      G_CALLBACK (set_toolbar_text) },
    { NULL, NULL, "Both", NULL, "Show toolbar icons and text",
      G_CALLBACK (set_toolbar_both) },
    { NULL, NULL, "Both (horizontal)", NULL, "Show toolbar icons and text in a horizontal fashion",
      G_CALLBACK (set_toolbar_both_horiz) },
    { NULL },
    { "entry", NULL, NULL, "This is an unusable CtkEntry ;)",
      NULL },
    { NULL },
    { NULL },
    { NULL, NULL, "Frobate", NULL, "Frobate tooltip",
      NULL },
    { NULL, NULL, "Baz", NULL, "Baz tooltip",
      NULL },
    { NULL },
    { NULL, NULL, "Blah", NULL, "Blash tooltip",
      NULL },
    { NULL, NULL, "Bar", NULL, "Bar tooltip",
      NULL },
};
G_GNUC_END_IGNORE_DEPRECATIONS;

static void
create_toolbar (CtkWidget *widget)
{
  static CtkWidget *window = NULL;
  CtkWidget *toolbar;

  if (!window)
    {
      guint i;

      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_screen (CTK_WINDOW (window),
			     ctk_widget_get_screen (widget));
      
      ctk_window_set_title (CTK_WINDOW (window), "Toolbar test");

      g_signal_connect (window, "destroy",
			G_CALLBACK (ctk_widget_destroyed),
			&window);

      ctk_container_set_border_width (CTK_CONTAINER (window), 0);
      ctk_widget_realize (window);

      toolbar = ctk_toolbar_new ();
      for (i = 0; i < G_N_ELEMENTS (create_toolbar_items); i++)
        {
          CtkToolItem *toolitem;

          if (create_toolbar_items[i].tooltip == NULL)
            toolitem = ctk_separator_tool_item_new ();
          else if (g_strcmp0 (create_toolbar_items[i].name, "entry") == 0)
            {
              CtkWidget *entry;

              toolitem = ctk_tool_item_new ();
              entry = ctk_entry_new ();
              ctk_container_add (CTK_CONTAINER (toolitem), entry);
            }
          else if (create_toolbar_items[i].stock_id)
            {
              G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
              toolitem = ctk_tool_button_new_from_stock (create_toolbar_items[i].stock_id);
              G_GNUC_END_IGNORE_DEPRECATIONS;
            }
          else
            {
              CtkWidget *icon;

              icon = new_pixbuf ("test.xpm", ctk_widget_get_window (window));
              toolitem = ctk_tool_button_new (icon, create_toolbar_items[i].label);
            }
          if (create_toolbar_items[i].callback)
            g_signal_connect (toolitem, "clicked",
                              create_toolbar_items[i].callback, toolbar);
          ctk_tool_item_set_tooltip_text (toolitem, create_toolbar_items[i].tooltip);
          ctk_toolbar_insert (CTK_TOOLBAR (toolbar), toolitem, -1);
        }

      ctk_container_add (CTK_CONTAINER (window), toolbar);

      ctk_widget_set_size_request (toolbar, 200, -1);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);
}

/*
 * CtkStatusBar
 */

static guint statusbar_counter = 1;

static void
statusbar_push (CtkWidget *button,
		CtkStatusbar *statusbar)
{
  gchar text[1024];

  sprintf (text, "something %d", statusbar_counter++);

  ctk_statusbar_push (statusbar, 1, text);
}

static void
statusbar_push_long (CtkWidget *button,
                     CtkStatusbar *statusbar)
{
  gchar text[1024];

  sprintf (text, "Just because a system has menu choices written with English words, phrases or sentences, that is no guarantee, that it is comprehensible. Individual words may not be familiar to some users (for example, \"repaginate\"), and two menu items may appear to satisfy the users's needs, whereas only one does (for example, \"put away\" or \"eject\").");

  ctk_statusbar_push (statusbar, 1, text);
}

static void
statusbar_pop (CtkWidget *button,
	       CtkStatusbar *statusbar)
{
  ctk_statusbar_pop (statusbar, 1);
}

static void
statusbar_steal (CtkWidget *button,
	         CtkStatusbar *statusbar)
{
  ctk_statusbar_remove (statusbar, 1, 4);
}

static void
statusbar_popped (CtkStatusbar  *statusbar,
		  guint          context_id,
		  const gchar	*text)
{
  if (!text)
    statusbar_counter = 1;
}

static void
statusbar_contexts (CtkStatusbar *statusbar)
{
  gchar *string;

  string = "any context";
  g_print ("CtkStatusBar: context=\"%s\", context_id=%d\n",
	   string,
	   ctk_statusbar_get_context_id (statusbar, string));
  
  string = "idle messages";
  g_print ("CtkStatusBar: context=\"%s\", context_id=%d\n",
	   string,
	   ctk_statusbar_get_context_id (statusbar, string));
  
  string = "some text";
  g_print ("CtkStatusBar: context=\"%s\", context_id=%d\n",
	   string,
	   ctk_statusbar_get_context_id (statusbar, string));

  string = "hit the mouse";
  g_print ("CtkStatusBar: context=\"%s\", context_id=%d\n",
	   string,
	   ctk_statusbar_get_context_id (statusbar, string));

  string = "hit the mouse2";
  g_print ("CtkStatusBar: context=\"%s\", context_id=%d\n",
	   string,
	   ctk_statusbar_get_context_id (statusbar, string));
}

static void
create_statusbar (CtkWidget *widget)
{
  static CtkWidget *window = NULL;
  CtkWidget *box1;
  CtkWidget *box2;
  CtkWidget *button;
  CtkWidget *separator;
  CtkWidget *statusbar;

  if (!window)
    {
      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_screen (CTK_WINDOW (window),
			     ctk_widget_get_screen (widget));

      g_signal_connect (window, "destroy",
			G_CALLBACK (ctk_widget_destroyed),
			&window);

      ctk_window_set_title (CTK_WINDOW (window), "statusbar");
      ctk_container_set_border_width (CTK_CONTAINER (window), 0);

      box1 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
      ctk_container_add (CTK_CONTAINER (window), box1);

      box2 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 10);
      ctk_container_set_border_width (CTK_CONTAINER (box2), 10);
      ctk_box_pack_start (CTK_BOX (box1), box2, TRUE, TRUE, 0);

      statusbar = ctk_statusbar_new ();
      ctk_box_pack_end (CTK_BOX (box1), statusbar, TRUE, TRUE, 0);
      g_signal_connect (statusbar,
			"text_popped",
			G_CALLBACK (statusbar_popped),
			NULL);

      button = g_object_new (ctk_button_get_type (),
			       "label", "push something",
			       "visible", TRUE,
			       "parent", box2,
			       NULL);
      g_object_connect (button,
			"signal::clicked", statusbar_push, statusbar,
			NULL);

      button = g_object_connect (g_object_new (ctk_button_get_type (),
						 "label", "pop",
						 "visible", TRUE,
						 "parent", box2,
						 NULL),
				 "signal_after::clicked", statusbar_pop, statusbar,
				 NULL);

      button = g_object_connect (g_object_new (ctk_button_get_type (),
						 "label", "steal #4",
						 "visible", TRUE,
						 "parent", box2,
						 NULL),
				 "signal_after::clicked", statusbar_steal, statusbar,
				 NULL);

      button = g_object_connect (g_object_new (ctk_button_get_type (),
						 "label", "test contexts",
						 "visible", TRUE,
						 "parent", box2,
						 NULL),
				 "swapped_signal_after::clicked", statusbar_contexts, statusbar,
				 NULL);

      button = g_object_connect (g_object_new (ctk_button_get_type (),
						 "label", "push something long",
						 "visible", TRUE,
						 "parent", box2,
						 NULL),
				 "signal_after::clicked", statusbar_push_long, statusbar,
				 NULL);
      
      separator = ctk_separator_new (CTK_ORIENTATION_HORIZONTAL);
      ctk_box_pack_start (CTK_BOX (box1), separator, FALSE, TRUE, 0);

      box2 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 10);
      ctk_container_set_border_width (CTK_CONTAINER (box2), 10);
      ctk_box_pack_start (CTK_BOX (box1), box2, FALSE, TRUE, 0);

      button = ctk_button_new_with_label ("close");
      g_signal_connect_swapped (button, "clicked",
			        G_CALLBACK (ctk_widget_destroy),
				window);
      ctk_box_pack_start (CTK_BOX (box2), button, TRUE, TRUE, 0);
      ctk_widget_set_can_default (button, TRUE);
      ctk_widget_grab_default (button);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);
}

/* Alpha demo */

static void
alpha_changed (CtkRange *range, CtkWidget *widget)
{
  gdouble alpha;

  alpha = ctk_range_get_value (range);

  ctk_widget_set_opacity (widget, alpha / 100.0);
}


void
create_alpha_widgets (CtkWidget *widget)
{
  static CtkWidget *window = NULL;

  if (!window)
    {
      CtkWidget *vbox2, *vbox, *main_hbox;
      CtkWidget *button, *event_box, *label, *scale;
      CtkWidget *alpha1, *alpha2, *alpha3;

      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_screen (CTK_WINDOW (window),
                             ctk_widget_get_screen (widget));
      ctk_window_set_default_size (CTK_WINDOW (window),
                                   450, 450);

      g_signal_connect (window, "destroy",
                        G_CALLBACK (ctk_widget_destroyed), &window);

      ctk_window_set_title (CTK_WINDOW (window), "Alpha");
      ctk_container_set_border_width (CTK_CONTAINER (window), 0);

      main_hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
      ctk_container_add (CTK_CONTAINER (window), main_hbox);

      vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);

      ctk_box_pack_start (CTK_BOX (main_hbox), vbox, FALSE, FALSE, 0);

      /* Plain button (no cdkwindows */

      label = ctk_label_new ("non-window widget");
      ctk_box_pack_start (CTK_BOX (vbox), label, FALSE, FALSE, 0);

      alpha1 = button = ctk_button_new_with_label ("A Button");
      ctk_box_pack_start (CTK_BOX (vbox), button, FALSE, FALSE, 0);

      /* windowed container with both windowed and normal button */
      label = ctk_label_new ("\nwindow widget");
      ctk_box_pack_start (CTK_BOX (vbox), label, FALSE, FALSE, 0);

      alpha2 = event_box = ctk_event_box_new ();
      ctk_box_pack_start (CTK_BOX (vbox), event_box, FALSE, FALSE, 0);

      vbox2 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
      ctk_container_add (CTK_CONTAINER (event_box), vbox2);

      button = ctk_button_new_with_label ("A Button");
      ctk_box_pack_start (CTK_BOX (vbox2), button, FALSE, FALSE, 0);

      event_box = ctk_event_box_new ();
      button = ctk_button_new_with_label ("A Button (in window)");
      ctk_container_add (CTK_CONTAINER (event_box), button);
      ctk_box_pack_start (CTK_BOX (vbox2), event_box, FALSE, FALSE, 0);

      /* non-windowed container with both windowed and normal button */
      label = ctk_label_new ("\nnon-window widget with widget child");
      ctk_box_pack_start (CTK_BOX (vbox), label, FALSE, FALSE, 0);

      alpha3 = vbox2 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
      ctk_box_pack_start (CTK_BOX (vbox), vbox2, FALSE, FALSE, 0);

      button = ctk_button_new_with_label ("A Button");
      ctk_box_pack_start (CTK_BOX (vbox2), button, FALSE, FALSE, 0);

      event_box = ctk_event_box_new ();
      button = ctk_button_new_with_label ("A Button (in window)");
      ctk_container_add (CTK_CONTAINER (event_box), button);
      ctk_box_pack_start (CTK_BOX (vbox2), event_box, FALSE, FALSE, 0);

      scale = ctk_scale_new_with_range (CTK_ORIENTATION_VERTICAL,
                                         0, 100, 1);
      ctk_box_pack_start (CTK_BOX (main_hbox), scale, FALSE, FALSE, 0);
      g_signal_connect (scale, "value_changed", G_CALLBACK (alpha_changed), alpha1);
      ctk_range_set_value (CTK_RANGE (scale), 50);

      scale = ctk_scale_new_with_range (CTK_ORIENTATION_VERTICAL,
                                         0, 100, 1);
      ctk_box_pack_start (CTK_BOX (main_hbox), scale, FALSE, FALSE, 0);
      g_signal_connect (scale, "value_changed", G_CALLBACK (alpha_changed), alpha2);
      ctk_range_set_value (CTK_RANGE (scale), 50);

      scale = ctk_scale_new_with_range (CTK_ORIENTATION_VERTICAL,
                                         0, 100, 1);
      ctk_box_pack_start (CTK_BOX (main_hbox), scale, FALSE, FALSE, 0);
      g_signal_connect (scale, "value_changed", G_CALLBACK (alpha_changed), alpha3);
      ctk_range_set_value (CTK_RANGE (scale), 50);

      ctk_widget_show_all (main_hbox);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show (window);
  else
    ctk_widget_destroy (window);
}


/* 
 * Label Demo
 */
static void
sensitivity_toggled (CtkWidget *toggle,
                     CtkWidget *widget)
{
  ctk_widget_set_sensitive (widget,
                            ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (toggle)));
}

static CtkWidget*
create_sensitivity_control (CtkWidget *widget)
{
  CtkWidget *button;

  button = ctk_toggle_button_new_with_label ("Sensitive");  

  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (button),
                                ctk_widget_is_sensitive (widget));
  
  g_signal_connect (button,
                    "toggled",
                    G_CALLBACK (sensitivity_toggled),
                    widget);
  
  ctk_widget_show_all (button);

  return button;
}

static void
set_selectable_recursive (CtkWidget *widget,
                          gboolean   setting)
{
  if (CTK_IS_CONTAINER (widget))
    {
      GList *children;
      GList *tmp;
      
      children = ctk_container_get_children (CTK_CONTAINER (widget));
      tmp = children;
      while (tmp)
        {
          set_selectable_recursive (tmp->data, setting);

          tmp = tmp->next;
        }
      g_list_free (children);
    }
  else if (CTK_IS_LABEL (widget))
    {
      ctk_label_set_selectable (CTK_LABEL (widget), setting);
    }
}

static void
selectable_toggled (CtkWidget *toggle,
                    CtkWidget *widget)
{
  set_selectable_recursive (widget,
                            ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (toggle)));
}

static CtkWidget*
create_selectable_control (CtkWidget *widget)
{
  CtkWidget *button;

  button = ctk_toggle_button_new_with_label ("Selectable");  

  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (button),
                                FALSE);
  
  g_signal_connect (button,
                    "toggled",
                    G_CALLBACK (selectable_toggled),
                    widget);
  
  ctk_widget_show_all (button);

  return button;
}

static void
dialog_response (CtkWidget *dialog, gint response_id, CtkLabel *label)
{
  const gchar *text;

  ctk_widget_destroy (dialog);

  text = "Some <a href=\"http://en.wikipedia.org/wiki/Text\" title=\"plain text\">text</a> may be marked up\n"
         "as hyperlinks, which can be clicked\n"
         "or activated via <a href=\"keynav\">keynav</a>.\n"
         "The links remain the same.";
  ctk_label_set_markup (label, text);
}

static gboolean
activate_link (CtkWidget *label, const gchar *uri, gpointer data)
{
  if (g_strcmp0 (uri, "keynav") == 0)
    {
      CtkWidget *dialog;

      dialog = ctk_message_dialog_new_with_markup (CTK_WINDOW (ctk_widget_get_toplevel (label)),
                                       CTK_DIALOG_DESTROY_WITH_PARENT,
                                       CTK_MESSAGE_INFO,
                                       CTK_BUTTONS_OK,
                                       "The term <i>keynav</i> is a shorthand for "
                                       "keyboard navigation and refers to the process of using a program "
                                       "(exclusively) via keyboard input.");

      ctk_window_present (CTK_WINDOW (dialog));

      g_signal_connect (dialog, "response", G_CALLBACK (dialog_response), label);

      return TRUE;
    }

  return FALSE;
}

void create_labels (CtkWidget *widget)
{
  static CtkWidget *window = NULL;
  CtkWidget *hbox;
  CtkWidget *vbox;
  CtkWidget *frame;
  CtkWidget *label;
  CtkWidget *button;

  if (!window)
    {
      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);

      ctk_window_set_screen (CTK_WINDOW (window),
			     ctk_widget_get_screen (widget));

      g_signal_connect (window, "destroy",
			G_CALLBACK (ctk_widget_destroyed),
			&window);

      ctk_window_set_title (CTK_WINDOW (window), "Label");

      vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 5);
      
      hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 5);
      ctk_container_add (CTK_CONTAINER (window), vbox);

      ctk_box_pack_end (CTK_BOX (vbox), hbox, FALSE, FALSE, 0);

      button = create_sensitivity_control (hbox);

      ctk_box_pack_start (CTK_BOX (vbox), button, FALSE, FALSE, 0);

      button = create_selectable_control (hbox);

      ctk_box_pack_start (CTK_BOX (vbox), button, FALSE, FALSE, 0);
      
      vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 5);
      
      ctk_box_pack_start (CTK_BOX (hbox), vbox, FALSE, FALSE, 0);
      ctk_container_set_border_width (CTK_CONTAINER (window), 5);

      frame = ctk_frame_new ("Normal Label");
      label = ctk_label_new ("This is a Normal label");
      ctk_label_set_ellipsize (CTK_LABEL (label), PANGO_ELLIPSIZE_START);
      ctk_container_add (CTK_CONTAINER (frame), label);
      ctk_box_pack_start (CTK_BOX (vbox), frame, FALSE, FALSE, 0);

      frame = ctk_frame_new ("Multi-line Label");
      label = ctk_label_new ("This is a Multi-line label.\nSecond line\nThird line");
      ctk_label_set_ellipsize (CTK_LABEL (label), PANGO_ELLIPSIZE_END);
      ctk_container_add (CTK_CONTAINER (frame), label);
      ctk_box_pack_start (CTK_BOX (vbox), frame, FALSE, FALSE, 0);

      frame = ctk_frame_new ("Left Justified Label");
      label = ctk_label_new ("This is a Left-Justified\nMulti-line label.\nThird      line");
      ctk_label_set_ellipsize (CTK_LABEL (label), PANGO_ELLIPSIZE_MIDDLE);
      ctk_label_set_justify (CTK_LABEL (label), CTK_JUSTIFY_LEFT);
      ctk_container_add (CTK_CONTAINER (frame), label);
      ctk_box_pack_start (CTK_BOX (vbox), frame, FALSE, FALSE, 0);

      frame = ctk_frame_new ("Right Justified Label");
      ctk_label_set_ellipsize (CTK_LABEL (label), PANGO_ELLIPSIZE_START);
      label = ctk_label_new ("This is a Right-Justified\nMulti-line label.\nFourth line, (j/k)");
      ctk_label_set_justify (CTK_LABEL (label), CTK_JUSTIFY_RIGHT);
      ctk_container_add (CTK_CONTAINER (frame), label);
      ctk_box_pack_start (CTK_BOX (vbox), frame, FALSE, FALSE, 0);

      frame = ctk_frame_new ("Internationalized Label");
      label = ctk_label_new (NULL);
      ctk_label_set_markup (CTK_LABEL (label),
			    "French (Fran\303\247ais) Bonjour, Salut\n"
			    "Korean (\355\225\234\352\270\200)   \354\225\210\353\205\225\355\225\230\354\204\270\354\232\224, \354\225\210\353\205\225\355\225\230\354\213\255\353\213\210\352\271\214\n"
			    "Russian (\320\240\321\203\321\201\321\201\320\272\320\270\320\271) \320\227\320\264\321\200\320\260\320\262\321\201\321\202\320\262\321\203\320\271\321\202\320\265!\n"
			    "Chinese (Simplified) <span lang=\"zh-cn\">\345\205\203\346\260\224	\345\274\200\345\217\221</span>\n"
			    "Chinese (Traditional) <span lang=\"zh-tw\">\345\205\203\346\260\243	\351\226\213\347\231\274</span>\n"
			    "Japanese <span lang=\"ja\">\345\205\203\346\260\227	\351\226\213\347\231\272</span>");
      ctk_label_set_justify (CTK_LABEL (label), CTK_JUSTIFY_LEFT);
      ctk_container_add (CTK_CONTAINER (frame), label);
      ctk_box_pack_start (CTK_BOX (vbox), frame, FALSE, FALSE, 0);

      frame = ctk_frame_new ("Bidirection Label");
      label = ctk_label_new ("\342\200\217Arabic	\330\247\331\204\330\263\331\204\330\247\331\205 \330\271\331\204\331\212\331\203\331\205\n"
			     "\342\200\217Hebrew	\327\251\327\234\327\225\327\235");
      ctk_container_add (CTK_CONTAINER (frame), label);
      ctk_box_pack_start (CTK_BOX (vbox), frame, FALSE, FALSE, 0);

      frame = ctk_frame_new ("Links in a label");
      label = ctk_label_new ("Some <a href=\"http://en.wikipedia.org/wiki/Text\" title=\"plain text\">text</a> may be marked up\n"
                             "as hyperlinks, which can be clicked\n"
                             "or activated via <a href=\"keynav\">keynav</a>");
      ctk_label_set_use_markup (CTK_LABEL (label), TRUE);
      ctk_container_add (CTK_CONTAINER (frame), label);
      ctk_box_pack_start (CTK_BOX (vbox), frame, FALSE, FALSE, 0);
      g_signal_connect (label, "activate-link", G_CALLBACK (activate_link), NULL);

      vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 5);
      ctk_box_pack_start (CTK_BOX (hbox), vbox, FALSE, FALSE, 0);
      frame = ctk_frame_new ("Line wrapped label");
      label = ctk_label_new ("This is an example of a line-wrapped label.  It should not be taking "\
			     "up the entire             "/* big space to test spacing */\
			     "width allocated to it, but automatically wraps the words to fit.  "\
			     "The time has come, for all good men, to come to the aid of their party.  "\
			     "The sixth sheik's six sheep's sick.\n"\
			     "     It supports multiple paragraphs correctly, and  correctly   adds "\
			     "many          extra  spaces. ");

      ctk_label_set_line_wrap (CTK_LABEL (label), TRUE);
      ctk_container_add (CTK_CONTAINER (frame), label);
      ctk_box_pack_start (CTK_BOX (vbox), frame, FALSE, FALSE, 0);

      frame = ctk_frame_new ("Filled, wrapped label");
      label = ctk_label_new ("This is an example of a line-wrapped, filled label.  It should be taking "\
			     "up the entire              width allocated to it.  Here is a seneance to prove "\
			     "my point.  Here is another sentence. "\
			     "Here comes the sun, do de do de do.\n"\
			     "    This is a new paragraph.\n"\
			     "    This is another newer, longer, better paragraph.  It is coming to an end, "\
			     "unfortunately.");
      ctk_label_set_justify (CTK_LABEL (label), CTK_JUSTIFY_FILL);
      ctk_label_set_line_wrap (CTK_LABEL (label), TRUE);
      ctk_container_add (CTK_CONTAINER (frame), label);
      ctk_box_pack_start (CTK_BOX (vbox), frame, FALSE, FALSE, 0);

      frame = ctk_frame_new ("Underlined label");
      label = ctk_label_new ("This label is underlined!\n"
			     "This one is underlined (\343\201\223\343\202\223\343\201\253\343\201\241\343\201\257) in quite a funky fashion");
      ctk_label_set_justify (CTK_LABEL (label), CTK_JUSTIFY_LEFT);
      ctk_label_set_pattern (CTK_LABEL (label), "_________________________ _ _________ _ _____ _ __ __  ___ ____ _____");
      ctk_container_add (CTK_CONTAINER (frame), label);
      ctk_box_pack_start (CTK_BOX (vbox), frame, FALSE, FALSE, 0);

      frame = ctk_frame_new ("Markup label");
      label = ctk_label_new (NULL);

      /* There's also a ctk_label_set_markup() without accel if you
       * don't have an accelerator key
       */
      ctk_label_set_markup_with_mnemonic (CTK_LABEL (label),
					  "This <span foreground=\"blue\" background=\"orange\">label</span> has "
					  "<b>markup</b> _such as "
					  "<big><i>Big Italics</i></big>\n"
					  "<tt>Monospace font</tt>\n"
					  "<u>Underline!</u>\n"
					  "foo\n"
					  "<span foreground=\"green\" background=\"red\">Ugly colors</span>\n"
					  "and nothing on this line,\n"
					  "or this.\n"
					  "or this either\n"
					  "or even on this one\n"
					  "la <big>la <big>la <big>la <big>la</big></big></big></big>\n"
					  "but this _word is <span foreground=\"purple\"><big>purple</big></span>\n"
					  "<span underline=\"double\">We like <sup>superscript</sup> and <sub>subscript</sub> too</span>");

      g_assert (ctk_label_get_mnemonic_keyval (CTK_LABEL (label)) == CDK_KEY_s);
      
      ctk_container_add (CTK_CONTAINER (frame), label);
      ctk_box_pack_start (CTK_BOX (vbox), frame, FALSE, FALSE, 0);
    }
  
  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);
}

static void
on_angle_scale_changed (CtkRange *range,
			CtkLabel *label)
{
  ctk_label_set_angle (CTK_LABEL (label), ctk_range_get_value (range));
}

static void
create_rotated_label (CtkWidget *widget)
{
  static CtkWidget *window = NULL;
  CtkWidget *content_area;
  CtkWidget *vbox;
  CtkWidget *hscale;
  CtkWidget *label;  
  CtkWidget *scale_label;  
  CtkWidget *scale_hbox;  

  if (!window)
    {
      window = ctk_dialog_new_with_buttons ("Rotated Label",
					    CTK_WINDOW (ctk_widget_get_toplevel (widget)), 0,
					    "_Close", CTK_RESPONSE_CLOSE,
					    NULL);

      ctk_window_set_resizable (CTK_WINDOW (window), TRUE);

      ctk_window_set_screen (CTK_WINDOW (window),
			     ctk_widget_get_screen (widget));

      g_signal_connect (window, "response",
			G_CALLBACK (ctk_widget_destroy), NULL);
      g_signal_connect (window, "destroy",
			G_CALLBACK (ctk_widget_destroyed), &window);

      content_area = ctk_dialog_get_content_area (CTK_DIALOG (window));

      vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 5);
      ctk_box_pack_start (CTK_BOX (content_area), vbox, TRUE, TRUE, 0);
      ctk_container_set_border_width (CTK_CONTAINER (vbox), 10);

      label = ctk_label_new (NULL);
      ctk_label_set_markup (CTK_LABEL (label), "Hello World\n<i>Rotate</i> <span underline='single' foreground='blue'>me</span>");
      ctk_box_pack_start (CTK_BOX (vbox), label, TRUE, TRUE, 0);

      scale_hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
      ctk_box_pack_start (CTK_BOX (vbox), scale_hbox, FALSE, FALSE, 0);
      
      scale_label = ctk_label_new (NULL);
      ctk_label_set_markup (CTK_LABEL (scale_label), "<i>Angle: </i>");
      ctk_box_pack_start (CTK_BOX (scale_hbox), scale_label, FALSE, FALSE, 0);

      hscale = ctk_scale_new_with_range (CTK_ORIENTATION_HORIZONTAL,
                                         0, 360, 5);
      g_signal_connect (hscale, "value-changed",
			G_CALLBACK (on_angle_scale_changed), label);
      
      ctk_range_set_value (CTK_RANGE (hscale), 45);
      ctk_widget_set_size_request (hscale, 200, -1);
      ctk_box_pack_start (CTK_BOX (scale_hbox), hscale, TRUE, TRUE, 0);
    }
  
  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);
}

#define DEFAULT_TEXT_RADIUS 200

static void
on_rotated_text_unrealize (CtkWidget *widget)
{
  g_object_set_data (G_OBJECT (widget), "text-gc", NULL);
}

static gboolean
on_rotated_text_draw (CtkWidget *widget,
                      cairo_t   *cr,
	              GdkPixbuf *tile_pixbuf)
{
  static const gchar *words[] = { "The", "grand", "old", "Duke", "of", "York",
                                  "had", "10,000", "men" };
  int n_words;
  int i;
  int width, height;
  double radius;
  PangoLayout *layout;
  PangoContext *context;
  PangoFontDescription *desc;

  cairo_set_source_rgb (cr, 1, 1, 1);
  cairo_paint (cr);

  if (tile_pixbuf)
    {
      cdk_cairo_set_source_pixbuf (cr, tile_pixbuf, 0, 0);
      cairo_pattern_set_extend (cairo_get_source (cr), CAIRO_EXTEND_REPEAT);
    }
  else
    cairo_set_source_rgb (cr, 0, 0, 0);

  width = ctk_widget_get_allocated_width (widget);
  height = ctk_widget_get_allocated_height (widget);
  radius = MIN (width, height) / 2.;

  cairo_translate (cr,
                   radius + (width - 2 * radius) / 2,
                   radius + (height - 2 * radius) / 2);
  cairo_scale (cr, radius / DEFAULT_TEXT_RADIUS, radius / DEFAULT_TEXT_RADIUS);

  context = ctk_widget_get_pango_context (widget);
  layout = pango_layout_new (context);
  desc = pango_font_description_from_string ("Sans Bold 30");
  pango_layout_set_font_description (layout, desc);
  pango_font_description_free (desc);
    
  n_words = G_N_ELEMENTS (words);
  for (i = 0; i < n_words; i++)
    {
      int width, height;

      cairo_save (cr);

      cairo_rotate (cr, 2 * G_PI * i / n_words);
      pango_cairo_update_layout (cr, layout);

      pango_layout_set_text (layout, words[i], -1);
      pango_layout_get_size (layout, &width, &height);

      cairo_move_to (cr, - width / 2 / PANGO_SCALE, - DEFAULT_TEXT_RADIUS);
      pango_cairo_show_layout (cr, layout);

      cairo_restore (cr);
    }
  
  g_object_unref (layout);

  return FALSE;
}

static void
create_rotated_text (CtkWidget *widget)
{
  static CtkWidget *window = NULL;

  if (!window)
    {
      CtkRequisition requisition;
      CtkWidget *content_area;
      CtkWidget *drawing_area;
      GdkPixbuf *tile_pixbuf;

      window = ctk_dialog_new_with_buttons ("Rotated Text",
					    CTK_WINDOW (ctk_widget_get_toplevel (widget)), 0,
					    "_Close", CTK_RESPONSE_CLOSE,
					    NULL);

      ctk_window_set_resizable (CTK_WINDOW (window), TRUE);

      ctk_window_set_screen (CTK_WINDOW (window),
			     ctk_widget_get_screen (widget));

      g_signal_connect (window, "response",
			G_CALLBACK (ctk_widget_destroy), NULL);
      g_signal_connect (window, "destroy",
			G_CALLBACK (ctk_widget_destroyed), &window);

      content_area = ctk_dialog_get_content_area (CTK_DIALOG (window));

      drawing_area = ctk_drawing_area_new ();
      ctk_box_pack_start (CTK_BOX (content_area), drawing_area, TRUE, TRUE, 0);

      tile_pixbuf = gdk_pixbuf_new_from_file ("marble.xpm", NULL);
      
      g_signal_connect (drawing_area, "draw",
			G_CALLBACK (on_rotated_text_draw), tile_pixbuf);
      g_signal_connect (drawing_area, "unrealize",
			G_CALLBACK (on_rotated_text_unrealize), NULL);

      ctk_widget_show_all (ctk_bin_get_child (CTK_BIN (window)));
      
      ctk_widget_set_size_request (drawing_area, DEFAULT_TEXT_RADIUS * 2, DEFAULT_TEXT_RADIUS * 2);
      ctk_widget_get_preferred_size ( (window),
                                 &requisition, NULL);
      ctk_widget_set_size_request (drawing_area, -1, -1);
      ctk_window_resize (CTK_WINDOW (window), requisition.width, requisition.height);
    }
  
  if (!ctk_widget_get_visible (window))
    ctk_widget_show (window);
  else
    ctk_widget_destroy (window);
}

/*
 * Reparent demo
 */

static void
reparent_label (CtkWidget *widget,
		CtkWidget *new_parent)
{
  CtkWidget *label;

  label = g_object_get_data (G_OBJECT (widget), "user_data");

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  ctk_widget_reparent (label, new_parent);
G_GNUC_END_IGNORE_DEPRECATIONS
}

static void
set_parent_signal (CtkWidget *child,
		   CtkWidget *old_parent,
		   gpointer   func_data)
{
  CtkWidget *parent;

  parent = ctk_widget_get_parent (child);
  g_message ("set_parent for \"%s\": new parent: \"%s\", old parent: \"%s\", data: %d\n",
             g_type_name (G_OBJECT_TYPE (child)),
             parent ? g_type_name (G_OBJECT_TYPE (parent)) : "NULL",
             old_parent ? g_type_name (G_OBJECT_TYPE (old_parent)) : "NULL",
             GPOINTER_TO_INT (func_data));
}

static void
create_reparent (CtkWidget *widget)
{
  static CtkWidget *window = NULL;
  CtkWidget *box1;
  CtkWidget *box2;
  CtkWidget *box3;
  CtkWidget *frame;
  CtkWidget *button;
  CtkWidget *label;
  CtkWidget *separator;
  CtkWidget *event_box;

  if (!window)
    {
      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);

      ctk_window_set_screen (CTK_WINDOW (window),
			     ctk_widget_get_screen (widget));

      g_signal_connect (window, "destroy",
			G_CALLBACK (ctk_widget_destroyed),
			&window);

      ctk_window_set_title (CTK_WINDOW (window), "reparent");
      ctk_container_set_border_width (CTK_CONTAINER (window), 0);

      box1 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
      ctk_container_add (CTK_CONTAINER (window), box1);

      box2 = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 5);
      ctk_container_set_border_width (CTK_CONTAINER (box2), 10);
      ctk_box_pack_start (CTK_BOX (box1), box2, TRUE, TRUE, 0);

      label = ctk_label_new ("Hello World");

      frame = ctk_frame_new ("Frame 1");
      ctk_box_pack_start (CTK_BOX (box2), frame, TRUE, TRUE, 0);

      box3 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 5);
      ctk_container_set_border_width (CTK_CONTAINER (box3), 5);
      ctk_container_add (CTK_CONTAINER (frame), box3);

      button = ctk_button_new_with_label ("switch");
      g_object_set_data (G_OBJECT (button), "user_data", label);
      ctk_box_pack_start (CTK_BOX (box3), button, FALSE, TRUE, 0);

      event_box = ctk_event_box_new ();
      ctk_box_pack_start (CTK_BOX (box3), event_box, FALSE, TRUE, 0);
      ctk_container_add (CTK_CONTAINER (event_box), label);
			 
      g_signal_connect (button, "clicked",
			G_CALLBACK (reparent_label),
			event_box);
      
      g_signal_connect (label, "parent_set",
			G_CALLBACK (set_parent_signal),
			GINT_TO_POINTER (42));

      frame = ctk_frame_new ("Frame 2");
      ctk_box_pack_start (CTK_BOX (box2), frame, TRUE, TRUE, 0);

      box3 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 5);
      ctk_container_set_border_width (CTK_CONTAINER (box3), 5);
      ctk_container_add (CTK_CONTAINER (frame), box3);

      button = ctk_button_new_with_label ("switch");
      g_object_set_data (G_OBJECT (button), "user_data", label);
      ctk_box_pack_start (CTK_BOX (box3), button, FALSE, TRUE, 0);
      
      event_box = ctk_event_box_new ();
      ctk_box_pack_start (CTK_BOX (box3), event_box, FALSE, TRUE, 0);

      g_signal_connect (button, "clicked",
			G_CALLBACK (reparent_label),
			event_box);

      separator = ctk_separator_new (CTK_ORIENTATION_HORIZONTAL);
      ctk_box_pack_start (CTK_BOX (box1), separator, FALSE, TRUE, 0);

      box2 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 10);
      ctk_container_set_border_width (CTK_CONTAINER (box2), 10);
      ctk_box_pack_start (CTK_BOX (box1), box2, FALSE, TRUE, 0);

      button = ctk_button_new_with_label ("close");
      g_signal_connect_swapped (button, "clicked",
			        G_CALLBACK (ctk_widget_destroy), window);
      ctk_box_pack_start (CTK_BOX (box2), button, TRUE, TRUE, 0);
      ctk_widget_set_can_default (button, TRUE);
      ctk_widget_grab_default (button);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);
}

/*
 * Resize Grips
 */
static gboolean
grippy_button_press (CtkWidget *area, CdkEventButton *event, CdkWindowEdge edge)
{
  if (event->type == CDK_BUTTON_PRESS) 
    {
      if (event->button == CDK_BUTTON_PRIMARY)
	ctk_window_begin_resize_drag (CTK_WINDOW (ctk_widget_get_toplevel (area)), edge,
				      event->button, event->x_root, event->y_root,
				      event->time);
      else if (event->button == CDK_BUTTON_MIDDLE)
	ctk_window_begin_move_drag (CTK_WINDOW (ctk_widget_get_toplevel (area)), 
				    event->button, event->x_root, event->y_root,
				    event->time);
    }
  return TRUE;
}

static gboolean
grippy_draw (CtkWidget *area, cairo_t *cr, CdkWindowEdge edge)
{
  CtkStyleContext *context;
  CtkJunctionSides sides;

  switch (edge)
    {
    case CDK_WINDOW_EDGE_NORTH_WEST:
      sides = CTK_JUNCTION_CORNER_TOPLEFT;
      break;
    case CDK_WINDOW_EDGE_NORTH:
      sides = CTK_JUNCTION_TOP;
      break;
    case CDK_WINDOW_EDGE_NORTH_EAST:
      sides = CTK_JUNCTION_CORNER_TOPRIGHT;
      break;
    case CDK_WINDOW_EDGE_WEST:
      sides = CTK_JUNCTION_LEFT;
      break;
    case CDK_WINDOW_EDGE_EAST:
      sides = CTK_JUNCTION_RIGHT;
      break;
    case CDK_WINDOW_EDGE_SOUTH_WEST:
      sides = CTK_JUNCTION_CORNER_BOTTOMLEFT;
      break;
    case CDK_WINDOW_EDGE_SOUTH:
      sides = CTK_JUNCTION_BOTTOM;
      break;
    case CDK_WINDOW_EDGE_SOUTH_EAST:
      sides = CTK_JUNCTION_CORNER_BOTTOMRIGHT;
      break;
    default:
      g_assert_not_reached();
    }

  context = ctk_widget_get_style_context (area);
  ctk_style_context_save (context);
  ctk_style_context_add_class (context, "grip");
  ctk_style_context_set_junction_sides (context, sides);
  ctk_render_handle (context, cr,
                     0, 0,
                     ctk_widget_get_allocated_width (area),
                     ctk_widget_get_allocated_height (area));

  ctk_style_context_restore (context);

  return TRUE;
}

static void
create_resize_grips (CtkWidget *widget)
{
  static CtkWidget *window = NULL;
  CtkWidget *area;
  CtkWidget *hbox, *vbox;
  if (!window)
    {
      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);

      ctk_window_set_screen (CTK_WINDOW (window),
			     ctk_widget_get_screen (widget));

      ctk_window_set_title (CTK_WINDOW (window), "resize grips");
      
      g_signal_connect (window, "destroy",
			G_CALLBACK (ctk_widget_destroyed),
			&window);

      vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
      ctk_container_add (CTK_CONTAINER (window), vbox);
      
      hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
      ctk_box_pack_start (CTK_BOX (vbox), hbox, TRUE, TRUE, 0);

      /* North west */
      area = ctk_drawing_area_new ();
      ctk_widget_add_events (area, CDK_BUTTON_PRESS_MASK);
      ctk_box_pack_start (CTK_BOX (hbox), area, TRUE, TRUE, 0);
      g_signal_connect (area, "draw", G_CALLBACK (grippy_draw),
			GINT_TO_POINTER (CDK_WINDOW_EDGE_NORTH_WEST));
      g_signal_connect (area, "button_press_event", G_CALLBACK (grippy_button_press),
			GINT_TO_POINTER (CDK_WINDOW_EDGE_NORTH_WEST));
      
      /* North */
      area = ctk_drawing_area_new ();
      ctk_widget_add_events (area, CDK_BUTTON_PRESS_MASK);
      ctk_box_pack_start (CTK_BOX (hbox), area, TRUE, TRUE, 0);
      g_signal_connect (area, "draw", G_CALLBACK (grippy_draw),
			GINT_TO_POINTER (CDK_WINDOW_EDGE_NORTH));
      g_signal_connect (area, "button_press_event", G_CALLBACK (grippy_button_press),
			GINT_TO_POINTER (CDK_WINDOW_EDGE_NORTH));

      /* North east */
      area = ctk_drawing_area_new ();
      ctk_widget_add_events (area, CDK_BUTTON_PRESS_MASK);
      ctk_box_pack_start (CTK_BOX (hbox), area, TRUE, TRUE, 0);
      g_signal_connect (area, "draw", G_CALLBACK (grippy_draw),
			GINT_TO_POINTER (CDK_WINDOW_EDGE_NORTH_EAST));
      g_signal_connect (area, "button_press_event", G_CALLBACK (grippy_button_press),
			GINT_TO_POINTER (CDK_WINDOW_EDGE_NORTH_EAST));

      hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
      ctk_box_pack_start (CTK_BOX (vbox), hbox, TRUE, TRUE, 0);

      /* West */
      area = ctk_drawing_area_new ();
      ctk_widget_add_events (area, CDK_BUTTON_PRESS_MASK);
      ctk_box_pack_start (CTK_BOX (hbox), area, TRUE, TRUE, 0);
      g_signal_connect (area, "draw", G_CALLBACK (grippy_draw),
			GINT_TO_POINTER (CDK_WINDOW_EDGE_WEST));
      g_signal_connect (area, "button_press_event", G_CALLBACK (grippy_button_press),
			GINT_TO_POINTER (CDK_WINDOW_EDGE_WEST));

      /* Middle */
      area = ctk_drawing_area_new ();
      ctk_box_pack_start (CTK_BOX (hbox), area, TRUE, TRUE, 0);

      /* East */
      area = ctk_drawing_area_new ();
      ctk_widget_add_events (area, CDK_BUTTON_PRESS_MASK);
      ctk_box_pack_start (CTK_BOX (hbox), area, TRUE, TRUE, 0);
      g_signal_connect (area, "draw", G_CALLBACK (grippy_draw),
			GINT_TO_POINTER (CDK_WINDOW_EDGE_EAST));
      g_signal_connect (area, "button_press_event", G_CALLBACK (grippy_button_press),
			GINT_TO_POINTER (CDK_WINDOW_EDGE_EAST));


      hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
      ctk_box_pack_start (CTK_BOX (vbox), hbox, TRUE, TRUE, 0);

      /* South west */
      area = ctk_drawing_area_new ();
      ctk_widget_add_events (area, CDK_BUTTON_PRESS_MASK);
      ctk_box_pack_start (CTK_BOX (hbox), area, TRUE, TRUE, 0);
      g_signal_connect (area, "draw", G_CALLBACK (grippy_draw),
			GINT_TO_POINTER (CDK_WINDOW_EDGE_SOUTH_WEST));
      g_signal_connect (area, "button_press_event", G_CALLBACK (grippy_button_press),
			GINT_TO_POINTER (CDK_WINDOW_EDGE_SOUTH_WEST));
      /* South */
      area = ctk_drawing_area_new ();
      ctk_widget_add_events (area, CDK_BUTTON_PRESS_MASK);
      ctk_box_pack_start (CTK_BOX (hbox), area, TRUE, TRUE, 0);
      g_signal_connect (area, "draw", G_CALLBACK (grippy_draw),
			GINT_TO_POINTER (CDK_WINDOW_EDGE_SOUTH));
      g_signal_connect (area, "button_press_event", G_CALLBACK (grippy_button_press),
			GINT_TO_POINTER (CDK_WINDOW_EDGE_SOUTH));
      
      /* South east */
      area = ctk_drawing_area_new ();
      ctk_widget_add_events (area, CDK_BUTTON_PRESS_MASK);
      ctk_box_pack_start (CTK_BOX (hbox), area, TRUE, TRUE, 0);
      g_signal_connect (area, "draw", G_CALLBACK (grippy_draw),
			GINT_TO_POINTER (CDK_WINDOW_EDGE_SOUTH_EAST));
      g_signal_connect (area, "button_press_event", G_CALLBACK (grippy_button_press),
			GINT_TO_POINTER (CDK_WINDOW_EDGE_SOUTH_EAST));
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);
}

/*
 * Saved Position
 */
gint upositionx = 0;
gint upositiony = 0;

static gint
uposition_configure (CtkWidget *window)
{
  CtkLabel *lx;
  CtkLabel *ly;
  gchar buffer[64];

  lx = g_object_get_data (G_OBJECT (window), "x");
  ly = g_object_get_data (G_OBJECT (window), "y");

  cdk_window_get_root_origin (ctk_widget_get_window (window),
                              &upositionx, &upositiony);
  sprintf (buffer, "%d", upositionx);
  ctk_label_set_text (lx, buffer);
  sprintf (buffer, "%d", upositiony);
  ctk_label_set_text (ly, buffer);

  return FALSE;
}

static void
uposition_stop_configure (CtkToggleButton *toggle,
			  GObject         *window)
{
  if (ctk_toggle_button_get_active (toggle))
    g_signal_handlers_block_by_func (window, G_CALLBACK (uposition_configure), NULL);
  else
    g_signal_handlers_unblock_by_func (window, G_CALLBACK (uposition_configure), NULL);
}

static void
create_saved_position (CtkWidget *widget)
{
  static CtkWidget *window = NULL;

  if (!window)
    {
      CtkWidget *hbox;
      CtkWidget *main_vbox;
      CtkWidget *vbox;
      CtkWidget *x_label;
      CtkWidget *y_label;
      CtkWidget *button;
      CtkWidget *label;
      CtkWidget *any;

      window = g_object_connect (g_object_new (CTK_TYPE_WINDOW,
						 "type", CTK_WINDOW_TOPLEVEL,
						 "title", "Saved Position",
						 NULL),
				 "signal::configure_event", uposition_configure, NULL,
				 NULL);

      ctk_window_move (CTK_WINDOW (window), upositionx, upositiony);

      ctk_window_set_screen (CTK_WINDOW (window),
			     ctk_widget_get_screen (widget));
      

      g_signal_connect (window, "destroy",
			G_CALLBACK (ctk_widget_destroyed),
			&window);

      main_vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 5);
      ctk_container_set_border_width (CTK_CONTAINER (main_vbox), 0);
      ctk_container_add (CTK_CONTAINER (window), main_vbox);

      vbox =
	g_object_new (CTK_TYPE_BOX,
                      "orientation", CTK_ORIENTATION_VERTICAL,
			"CtkBox::homogeneous", FALSE,
			"CtkBox::spacing", 5,
			"CtkContainer::border_width", 10,
			"CtkWidget::parent", main_vbox,
			"CtkWidget::visible", TRUE,
			"child", g_object_connect (g_object_new (CTK_TYPE_TOGGLE_BUTTON,
								   "label", "Stop Events",
								   "active", FALSE,
								   "visible", TRUE,
								   NULL),
						   "signal::clicked", uposition_stop_configure, window,
						   NULL),
			NULL);

      hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
      ctk_container_set_border_width (CTK_CONTAINER (hbox), 5);
      ctk_box_pack_start (CTK_BOX (vbox), hbox, FALSE, TRUE, 0);

      label = ctk_label_new ("X Origin : ");
      ctk_widget_set_halign (label, CTK_ALIGN_START);
      ctk_widget_set_valign (label, CTK_ALIGN_CENTER);
      ctk_box_pack_start (CTK_BOX (hbox), label, FALSE, TRUE, 0);

      x_label = ctk_label_new ("");
      ctk_box_pack_start (CTK_BOX (hbox), x_label, TRUE, TRUE, 0);
      g_object_set_data (G_OBJECT (window), "x", x_label);

      hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
      ctk_container_set_border_width (CTK_CONTAINER (hbox), 5);
      ctk_box_pack_start (CTK_BOX (vbox), hbox, FALSE, TRUE, 0);

      label = ctk_label_new ("Y Origin : ");
      ctk_widget_set_halign (label, CTK_ALIGN_START);
      ctk_widget_set_valign (label, CTK_ALIGN_CENTER);
      ctk_box_pack_start (CTK_BOX (hbox), label, FALSE, TRUE, 0);

      y_label = ctk_label_new ("");
      ctk_box_pack_start (CTK_BOX (hbox), y_label, TRUE, TRUE, 0);
      g_object_set_data (G_OBJECT (window), "y", y_label);

      any =
	g_object_new (ctk_separator_get_type (),
			"CtkWidget::visible", TRUE,
			NULL);
      ctk_box_pack_start (CTK_BOX (main_vbox), any, FALSE, TRUE, 0);

      hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
      ctk_container_set_border_width (CTK_CONTAINER (hbox), 10);
      ctk_box_pack_start (CTK_BOX (main_vbox), hbox, FALSE, TRUE, 0);

      button = ctk_button_new_with_label ("Close");
      g_signal_connect_swapped (button, "clicked",
			        G_CALLBACK (ctk_widget_destroy),
				window);
      ctk_box_pack_start (CTK_BOX (hbox), button, TRUE, TRUE, 5);
      ctk_widget_set_can_default (button, TRUE);
      ctk_widget_grab_default (button);
      
      ctk_widget_show_all (window);
    }
  else
    ctk_widget_destroy (window);
}

/*
 * CtkPixmap
 */

static void
create_pixbuf (CtkWidget *widget)
{
  static CtkWidget *window = NULL;
  CtkWidget *box1;
  CtkWidget *box2;
  CtkWidget *box3;
  CtkWidget *button;
  CtkWidget *label;
  CtkWidget *separator;
  CtkWidget *pixbufwid;
  CdkWindow *cdk_window;

  if (!window)
    {
      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);

      ctk_window_set_screen (CTK_WINDOW (window),
			     ctk_widget_get_screen (widget));

      g_signal_connect (window, "destroy",
                        G_CALLBACK (ctk_widget_destroyed),
                        &window);

      ctk_window_set_title (CTK_WINDOW (window), "CtkPixmap");
      ctk_container_set_border_width (CTK_CONTAINER (window), 0);
      ctk_widget_realize(window);

      box1 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
      ctk_container_add (CTK_CONTAINER (window), box1);

      box2 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 10);
      ctk_container_set_border_width (CTK_CONTAINER (box2), 10);
      ctk_box_pack_start (CTK_BOX (box1), box2, TRUE, TRUE, 0);

      button = ctk_button_new ();
      ctk_box_pack_start (CTK_BOX (box2), button, FALSE, FALSE, 0);

      cdk_window = ctk_widget_get_window (window);

      pixbufwid = new_pixbuf ("test.xpm", cdk_window);

      label = ctk_label_new ("Pixbuf\ntest");
      box3 = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
      ctk_container_set_border_width (CTK_CONTAINER (box3), 2);
      ctk_container_add (CTK_CONTAINER (box3), pixbufwid);
      ctk_container_add (CTK_CONTAINER (box3), label);
      ctk_container_add (CTK_CONTAINER (button), box3);

      button = ctk_button_new ();
      ctk_box_pack_start (CTK_BOX (box2), button, FALSE, FALSE, 0);

      pixbufwid = new_pixbuf ("test.xpm", cdk_window);

      label = ctk_label_new ("Pixbuf\ntest");
      box3 = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
      ctk_container_set_border_width (CTK_CONTAINER (box3), 2);
      ctk_container_add (CTK_CONTAINER (box3), pixbufwid);
      ctk_container_add (CTK_CONTAINER (box3), label);
      ctk_container_add (CTK_CONTAINER (button), box3);

      ctk_widget_set_sensitive (button, FALSE);
      
      separator = ctk_separator_new (CTK_ORIENTATION_HORIZONTAL);
      ctk_box_pack_start (CTK_BOX (box1), separator, FALSE, TRUE, 0);

      box2 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 10);
      ctk_container_set_border_width (CTK_CONTAINER (box2), 10);
      ctk_box_pack_start (CTK_BOX (box1), box2, FALSE, TRUE, 0);

      button = ctk_button_new_with_label ("close");
      g_signal_connect_swapped (button, "clicked",
			        G_CALLBACK (ctk_widget_destroy),
				window);
      ctk_box_pack_start (CTK_BOX (box2), button, TRUE, TRUE, 0);
      ctk_widget_set_can_default (button, TRUE);
      ctk_widget_grab_default (button);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);
}

static void
create_tooltips (CtkWidget *widget)
{
  static CtkWidget *window = NULL;
  CtkWidget *box1;
  CtkWidget *box2;
  CtkWidget *box3;
  CtkWidget *button;
  CtkWidget *toggle;
  CtkWidget *frame;
  CtkWidget *separator;

  if (!window)
    {
      window =
	g_object_new (ctk_window_get_type (),
			"CtkWindow::type", CTK_WINDOW_TOPLEVEL,
			"CtkContainer::border_width", 0,
			"CtkWindow::title", "Tooltips",
			"CtkWindow::resizable", FALSE,
			NULL);

      ctk_window_set_screen (CTK_WINDOW (window),
			     ctk_widget_get_screen (widget));

      box1 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
      ctk_container_add (CTK_CONTAINER (window), box1);

      box2 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 10);
      ctk_container_set_border_width (CTK_CONTAINER (box2), 10);
      ctk_box_pack_start (CTK_BOX (box1), box2, TRUE, TRUE, 0);

      button = ctk_toggle_button_new_with_label ("button1");
      ctk_box_pack_start (CTK_BOX (box2), button, TRUE, TRUE, 0);

      ctk_widget_set_tooltip_text (button, "This is button 1");

      button = ctk_toggle_button_new_with_label ("button2");
      ctk_box_pack_start (CTK_BOX (box2), button, TRUE, TRUE, 0);

      ctk_widget_set_tooltip_text (button,
        "This is button 2. This is also a really long tooltip which probably "
        "won't fit on a single line and will therefore need to be wrapped. "
        "Hopefully the wrapping will work correctly.");

      toggle = ctk_toggle_button_new_with_label ("Override TipsQuery Label");
      ctk_box_pack_start (CTK_BOX (box2), toggle, TRUE, TRUE, 0);

      ctk_widget_set_tooltip_text (toggle, "Toggle TipsQuery view.");

      box3 =
	g_object_new (CTK_TYPE_BOX,
                      "orientation", CTK_ORIENTATION_VERTICAL,
			"homogeneous", FALSE,
			"spacing", 5,
			"border_width", 5,
			"visible", TRUE,
			NULL);

      button =
	g_object_new (ctk_button_get_type (),
			"label", "[?]",
			"visible", TRUE,
			"parent", box3,
			NULL);
      ctk_box_set_child_packing (CTK_BOX (box3), button, FALSE, FALSE, 0, CTK_PACK_START);
      ctk_widget_set_tooltip_text (button, "Start the Tooltips Inspector");
      
      frame = g_object_new (ctk_frame_get_type (),
			      "label", "ToolTips Inspector",
			      "label_xalign", (double) 0.5,
			      "border_width", 0,
			      "visible", TRUE,
			      "parent", box2,
			      "child", box3,
			      NULL);
      ctk_box_set_child_packing (CTK_BOX (box2), frame, TRUE, TRUE, 10, CTK_PACK_START);

      separator = ctk_separator_new (CTK_ORIENTATION_HORIZONTAL);
      ctk_box_pack_start (CTK_BOX (box1), separator, FALSE, TRUE, 0);

      box2 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 10);
      ctk_container_set_border_width (CTK_CONTAINER (box2), 10);
      ctk_box_pack_start (CTK_BOX (box1), box2, FALSE, TRUE, 0);

      button = ctk_button_new_with_label ("close");
      g_signal_connect_swapped (button, "clicked",
			        G_CALLBACK (ctk_widget_destroy),
				window);
      ctk_box_pack_start (CTK_BOX (box2), button, TRUE, TRUE, 0);
      ctk_widget_set_can_default (button, TRUE);
      ctk_widget_grab_default (button);

      ctk_widget_set_tooltip_text (button, "Push this button to close window");
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);
}

/*
 * CtkImage
 */

static void
pack_image (CtkWidget *box,
            const gchar *text,
            CtkWidget *image)
{
  ctk_box_pack_start (CTK_BOX (box),
                      ctk_label_new (text),
                      FALSE, FALSE, 0);

  ctk_box_pack_start (CTK_BOX (box),
                      image,
                      TRUE, TRUE, 0);  
}

static void
create_image (CtkWidget *widget)
{
  static CtkWidget *window = NULL;

  if (window == NULL)
    {
      CtkWidget *vbox;
      GdkPixbuf *pixbuf;
        
      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      
      ctk_window_set_screen (CTK_WINDOW (window),
			     ctk_widget_get_screen (widget));

      /* this is bogus for testing drawing when allocation < request,
       * don't copy into real code
       */
      ctk_window_set_resizable (CTK_WINDOW (window), TRUE);

      g_signal_connect (window, "destroy",
			G_CALLBACK (ctk_widget_destroyed),
			&window);

      vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 5);

      ctk_container_add (CTK_CONTAINER (window), vbox);

      pack_image (vbox, "Stock Warning Dialog",
                  ctk_image_new_from_icon_name ("dialog-warning",
                                                CTK_ICON_SIZE_DIALOG));

      pixbuf = gdk_pixbuf_new_from_xpm_data ((const char **) openfile);
      
      pack_image (vbox, "Pixbuf",
                  ctk_image_new_from_pixbuf (pixbuf));

      g_object_unref (pixbuf);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);
}

/*
 * ListBox demo
 */

static int
list_sort_cb (CtkListBoxRow *a, CtkListBoxRow *b, gpointer data)
{
  gint aa = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (a), "value"));
  gint bb = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (b), "value"));
  return aa - bb;
}

static gboolean
list_filter_all_cb (CtkListBoxRow *row, gpointer data)
{
  return FALSE;
}

static gboolean
list_filter_odd_cb (CtkListBoxRow *row, gpointer data)
{
  gint value = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (row), "value"));

  return value % 2 == 0;
}

static void
list_sort_clicked_cb (CtkButton *button,
                      gpointer data)
{
  CtkListBox *list = data;

  ctk_list_box_set_sort_func (list, list_sort_cb, NULL, NULL);
}

static void
list_filter_odd_clicked_cb (CtkButton *button,
                            gpointer data)
{
  CtkListBox *list = data;

  ctk_list_box_set_filter_func (list, list_filter_odd_cb, NULL, NULL);
}

static void
list_filter_all_clicked_cb (CtkButton *button,
                            gpointer data)
{
  CtkListBox *list = data;

  ctk_list_box_set_filter_func (list, list_filter_all_cb, NULL, NULL);
}


static void
list_unfilter_clicked_cb (CtkButton *button,
                          gpointer data)
{
  CtkListBox *list = data;

  ctk_list_box_set_filter_func (list, NULL, NULL, NULL);
}

static void
add_placeholder_clicked_cb (CtkButton *button,
                            gpointer data)
{
  CtkListBox *list = data;
  CtkWidget *label;

  label = ctk_label_new ("You filtered everything!!!");
  ctk_widget_show (label);
  ctk_list_box_set_placeholder (CTK_LIST_BOX (list), label);
}

static void
remove_placeholder_clicked_cb (CtkButton *button,
                            gpointer data)
{
  CtkListBox *list = data;

  ctk_list_box_set_placeholder (CTK_LIST_BOX (list), NULL);
}


static void
create_listbox (CtkWidget *widget)
{
  static CtkWidget *window = NULL;

  if (!window)
    {
      CtkWidget *hbox, *vbox, *scrolled, *scrolled_box, *list, *label, *button;
      CdkScreen *screen = ctk_widget_get_screen (widget);
      int i;

      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_screen (CTK_WINDOW (window), screen);

      g_signal_connect (window, "destroy",
                        G_CALLBACK (ctk_widget_destroyed),
                        &window);
      g_signal_connect (window, "delete-event",
                        G_CALLBACK (ctk_true),
                        NULL);

      ctk_window_set_title (CTK_WINDOW (window), "listbox");

      hbox = ctk_box_new(CTK_ORIENTATION_HORIZONTAL, 0);
      ctk_container_add (CTK_CONTAINER (window), hbox);

      scrolled = ctk_scrolled_window_new (NULL, NULL);
      ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (scrolled), CTK_POLICY_NEVER, CTK_POLICY_AUTOMATIC);
      ctk_container_add (CTK_CONTAINER (hbox), scrolled);

      scrolled_box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
      ctk_container_add (CTK_CONTAINER (scrolled), scrolled_box);

      label = ctk_label_new ("This is \na LABEL\nwith rows");
      ctk_container_add (CTK_CONTAINER (scrolled_box), label);

      list = ctk_list_box_new();
      ctk_list_box_set_adjustment (CTK_LIST_BOX (list), ctk_scrolled_window_get_vadjustment (CTK_SCROLLED_WINDOW (scrolled)));
      ctk_container_add (CTK_CONTAINER (scrolled_box), list);

      for (i = 0; i < 1000; i++)
        {
          gint value = g_random_int_range (0, 10000);
          label = ctk_label_new (g_strdup_printf ("Value %u", value));
          ctk_widget_show (label);
          ctk_container_add (CTK_CONTAINER (list), label);
          g_object_set_data (G_OBJECT (ctk_widget_get_parent (label)), "value", GINT_TO_POINTER (value));
        }

      vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
      ctk_container_add (CTK_CONTAINER (hbox), vbox);

      button = ctk_button_new_with_label ("sort");
      ctk_container_add (CTK_CONTAINER (vbox), button);
      g_signal_connect (button, "clicked", G_CALLBACK (list_sort_clicked_cb), list);

      button = ctk_button_new_with_label ("filter odd");
      ctk_container_add (CTK_CONTAINER (vbox), button);
      g_signal_connect (button, "clicked", G_CALLBACK (list_filter_odd_clicked_cb), list);

      button = ctk_button_new_with_label ("filter all");
      ctk_container_add (CTK_CONTAINER (vbox), button);
      g_signal_connect (button, "clicked", G_CALLBACK (list_filter_all_clicked_cb), list);

      button = ctk_button_new_with_label ("unfilter");
      ctk_container_add (CTK_CONTAINER (vbox), button);
      g_signal_connect (button, "clicked", G_CALLBACK (list_unfilter_clicked_cb), list);

      button = ctk_button_new_with_label ("add placeholder");
      ctk_container_add (CTK_CONTAINER (vbox), button);
      g_signal_connect (button, "clicked", G_CALLBACK (add_placeholder_clicked_cb), list);

      button = ctk_button_new_with_label ("remove placeholder");
      ctk_container_add (CTK_CONTAINER (vbox), button);
      g_signal_connect (button, "clicked", G_CALLBACK (remove_placeholder_clicked_cb), list);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);
}


/*
 * Menu demo
 */

static CtkWidget*
create_menu (CdkScreen *screen, gint depth, gint length)
{
  CtkWidget *menu;
  CtkWidget *menuitem;
  CtkWidget *image;
  GSList *group;
  char buf[32];
  int i, j;

  if (depth < 1)
    return NULL;

  menu = ctk_menu_new ();
  ctk_menu_set_screen (CTK_MENU (menu), screen);

  group = NULL;

  image = ctk_image_new_from_icon_name ("document-open",
                                        CTK_ICON_SIZE_MENU);
  ctk_widget_show (image);
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  menuitem = ctk_image_menu_item_new_with_label ("Image item");
  ctk_image_menu_item_set_image (CTK_IMAGE_MENU_ITEM (menuitem), image);
  ctk_image_menu_item_set_always_show_image (CTK_IMAGE_MENU_ITEM (menuitem), TRUE);
  G_GNUC_END_IGNORE_DEPRECATIONS;
  ctk_menu_shell_append (CTK_MENU_SHELL (menu), menuitem);
  ctk_widget_show (menuitem);
  
  for (i = 0, j = 1; i < length; i++, j++)
    {
      sprintf (buf, "item %2d - %d", depth, j);

      menuitem = ctk_radio_menu_item_new_with_label (group, buf);
      group = ctk_radio_menu_item_get_group (CTK_RADIO_MENU_ITEM (menuitem));

      ctk_menu_shell_append (CTK_MENU_SHELL (menu), menuitem);
      ctk_widget_show (menuitem);
      if (i == 3)
	ctk_widget_set_sensitive (menuitem, FALSE);

      if (i == 5)
        ctk_check_menu_item_set_inconsistent (CTK_CHECK_MENU_ITEM (menuitem),
                                              TRUE);

      if (i < 5)
	ctk_menu_item_set_submenu (CTK_MENU_ITEM (menuitem), 
				   create_menu (screen, depth - 1, 5));
    }

  return menu;
}

static CtkWidget*
create_table_menu (CdkScreen *screen, gint cols, gint rows)
{
  CtkWidget *menu;
  CtkWidget *menuitem;
  CtkWidget *submenu;
  CtkWidget *image;
  char buf[32];
  int i, j;

  menu = ctk_menu_new ();
  ctk_menu_set_screen (CTK_MENU (menu), screen);

  j = 0;
  
  menuitem = ctk_menu_item_new_with_label ("items");
  ctk_menu_attach (CTK_MENU (menu), menuitem, 0, cols, j, j + 1);

  submenu = ctk_menu_new ();
  ctk_menu_set_screen (CTK_MENU (submenu), screen);
  ctk_menu_item_set_submenu (CTK_MENU_ITEM (menuitem), submenu);
  ctk_widget_show (menuitem);
  j++;

  /* now fill the items submenu */
  image = ctk_image_new_from_icon_name ("help-broswer",
                                        CTK_ICON_SIZE_MENU);
  ctk_widget_show (image);
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  menuitem = ctk_image_menu_item_new_with_label ("Image");
  ctk_image_menu_item_set_image (CTK_IMAGE_MENU_ITEM (menuitem), image);
  ctk_image_menu_item_set_always_show_image (CTK_IMAGE_MENU_ITEM (menuitem), TRUE);
  G_GNUC_END_IGNORE_DEPRECATIONS;
  ctk_menu_attach (CTK_MENU (submenu), menuitem, 0, 1, 0, 1);
  ctk_widget_show (menuitem);

  menuitem = ctk_menu_item_new_with_label ("x");
  ctk_menu_attach (CTK_MENU (submenu), menuitem, 1, 2, 0, 1);
  ctk_widget_show (menuitem);

  menuitem = ctk_menu_item_new_with_label ("x");
  ctk_menu_attach (CTK_MENU (submenu), menuitem, 0, 1, 1, 2);
  ctk_widget_show (menuitem);
  
  image = ctk_image_new_from_icon_name ("help-browser",
                                        CTK_ICON_SIZE_MENU);
  ctk_widget_show (image);
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  menuitem = ctk_image_menu_item_new_with_label ("Image");
  ctk_image_menu_item_set_image (CTK_IMAGE_MENU_ITEM (menuitem), image);
  ctk_image_menu_item_set_always_show_image (CTK_IMAGE_MENU_ITEM (menuitem), TRUE);
  G_GNUC_END_IGNORE_DEPRECATIONS;
  ctk_menu_attach (CTK_MENU (submenu), menuitem, 1, 2, 1, 2);
  ctk_widget_show (menuitem);

  menuitem = ctk_radio_menu_item_new_with_label (NULL, "Radio");
  ctk_menu_attach (CTK_MENU (submenu), menuitem, 0, 1, 2, 3);
  ctk_widget_show (menuitem);

  menuitem = ctk_menu_item_new_with_label ("x");
  ctk_menu_attach (CTK_MENU (submenu), menuitem, 1, 2, 2, 3);
  ctk_widget_show (menuitem);

  menuitem = ctk_menu_item_new_with_label ("x");
  ctk_menu_attach (CTK_MENU (submenu), menuitem, 0, 1, 3, 4);
  ctk_widget_show (menuitem);
  
  menuitem = ctk_radio_menu_item_new_with_label (NULL, "Radio");
  ctk_menu_attach (CTK_MENU (submenu), menuitem, 1, 2, 3, 4);
  ctk_widget_show (menuitem);

  menuitem = ctk_check_menu_item_new_with_label ("Check");
  ctk_menu_attach (CTK_MENU (submenu), menuitem, 0, 1, 4, 5);
  ctk_widget_show (menuitem);

  menuitem = ctk_menu_item_new_with_label ("x");
  ctk_menu_attach (CTK_MENU (submenu), menuitem, 1, 2, 4, 5);
  ctk_widget_show (menuitem);

  menuitem = ctk_menu_item_new_with_label ("x");
  ctk_menu_attach (CTK_MENU (submenu), menuitem, 0, 1, 5, 6);
  ctk_widget_show (menuitem);
  
  menuitem = ctk_check_menu_item_new_with_label ("Check");
  ctk_menu_attach (CTK_MENU (submenu), menuitem, 1, 2, 5, 6);
  ctk_widget_show (menuitem);

  menuitem = ctk_menu_item_new_with_label ("1. Inserted normally (8)");
  ctk_widget_show (menuitem);
  ctk_menu_shell_insert (CTK_MENU_SHELL (submenu), menuitem, 8);

  menuitem = ctk_menu_item_new_with_label ("2. Inserted normally (2)");
  ctk_widget_show (menuitem);
  ctk_menu_shell_insert (CTK_MENU_SHELL (submenu), menuitem, 2);

  menuitem = ctk_menu_item_new_with_label ("3. Inserted normally (0)");
  ctk_widget_show (menuitem);
  ctk_menu_shell_insert (CTK_MENU_SHELL (submenu), menuitem, 0);

  menuitem = ctk_menu_item_new_with_label ("4. Inserted normally (-1)");
  ctk_widget_show (menuitem);
  ctk_menu_shell_insert (CTK_MENU_SHELL (submenu), menuitem, -1);
  
  /* end of items submenu */

  menuitem = ctk_menu_item_new_with_label ("spanning");
  ctk_menu_attach (CTK_MENU (menu), menuitem, 0, cols, j, j + 1);

  submenu = ctk_menu_new ();
  ctk_menu_set_screen (CTK_MENU (submenu), screen);
  ctk_menu_item_set_submenu (CTK_MENU_ITEM (menuitem), submenu);
  ctk_widget_show (menuitem);
  j++;

  /* now fill the spanning submenu */
  menuitem = ctk_menu_item_new_with_label ("a");
  ctk_menu_attach (CTK_MENU (submenu), menuitem, 0, 2, 0, 1);
  ctk_widget_show (menuitem);

  menuitem = ctk_menu_item_new_with_label ("b");
  ctk_menu_attach (CTK_MENU (submenu), menuitem, 2, 3, 0, 2);
  ctk_widget_show (menuitem);

  menuitem = ctk_menu_item_new_with_label ("c");
  ctk_menu_attach (CTK_MENU (submenu), menuitem, 0, 1, 1, 3);
  ctk_widget_show (menuitem);

  menuitem = ctk_menu_item_new_with_label ("d");
  ctk_menu_attach (CTK_MENU (submenu), menuitem, 1, 2, 1, 2);
  ctk_widget_show (menuitem);

  menuitem = ctk_menu_item_new_with_label ("e");
  ctk_menu_attach (CTK_MENU (submenu), menuitem, 1, 3, 2, 3);
  ctk_widget_show (menuitem);
  /* end of spanning submenu */
  
  menuitem = ctk_menu_item_new_with_label ("left");
  ctk_menu_attach (CTK_MENU (menu), menuitem, 0, 1, j, j + 1);
  submenu = ctk_menu_new ();
  ctk_menu_set_screen (CTK_MENU (submenu), screen);
  ctk_menu_item_set_submenu (CTK_MENU_ITEM (menuitem), submenu);
  ctk_widget_show (menuitem);

  menuitem = ctk_menu_item_new_with_label ("Empty");
  ctk_menu_attach (CTK_MENU (submenu), menuitem, 0, 1, 0, 1);
  submenu = ctk_menu_new ();
  ctk_menu_set_screen (CTK_MENU (submenu), screen);
  ctk_menu_item_set_submenu (CTK_MENU_ITEM (menuitem), submenu);
  ctk_widget_show (menuitem);

  menuitem = ctk_menu_item_new_with_label ("right");
  ctk_menu_attach (CTK_MENU (menu), menuitem, 1, 2, j, j + 1);
  submenu = ctk_menu_new ();
  ctk_menu_set_screen (CTK_MENU (submenu), screen);
  ctk_menu_item_set_submenu (CTK_MENU_ITEM (menuitem), submenu);
  ctk_widget_show (menuitem);

  menuitem = ctk_menu_item_new_with_label ("Empty");
  ctk_menu_attach (CTK_MENU (submenu), menuitem, 0, 1, 0, 1);
  ctk_widget_show (menuitem);

  j++;

  for (; j < rows; j++)
      for (i = 0; i < cols; i++)
      {
	sprintf (buf, "(%d %d)", i, j);
	menuitem = ctk_menu_item_new_with_label (buf);
	ctk_menu_attach (CTK_MENU (menu), menuitem, i, i + 1, j, j + 1);
	ctk_widget_show (menuitem);
      }
  
  menuitem = ctk_menu_item_new_with_label ("1. Inserted normally (8)");
  ctk_menu_shell_insert (CTK_MENU_SHELL (menu), menuitem, 8);
  ctk_widget_show (menuitem);
  menuitem = ctk_menu_item_new_with_label ("2. Inserted normally (2)");
  ctk_menu_shell_insert (CTK_MENU_SHELL (menu), menuitem, 2);
  ctk_widget_show (menuitem);
  menuitem = ctk_menu_item_new_with_label ("3. Inserted normally (0)");
  ctk_menu_shell_insert (CTK_MENU_SHELL (menu), menuitem, 0);
  ctk_widget_show (menuitem);
  menuitem = ctk_menu_item_new_with_label ("4. Inserted normally (-1)");
  ctk_menu_shell_insert (CTK_MENU_SHELL (menu), menuitem, -1);
  ctk_widget_show (menuitem);
  
  return menu;
}

static void
create_menus (CtkWidget *widget)
{
  static CtkWidget *window = NULL;
  CtkWidget *box1;
  CtkWidget *box2;
  CtkWidget *button;
  CtkWidget *optionmenu;
  CtkWidget *separator;
  
  if (!window)
    {
      CtkWidget *menubar;
      CtkWidget *menu;
      CtkWidget *menuitem;
      CtkAccelGroup *accel_group;
      CtkWidget *image;
      CdkScreen *screen = ctk_widget_get_screen (widget);
      
      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);

      ctk_window_set_screen (CTK_WINDOW (window), screen);
      
      g_signal_connect (window, "destroy",
			G_CALLBACK (ctk_widget_destroyed),
			&window);
      g_signal_connect (window, "delete-event",
			G_CALLBACK (ctk_true),
			NULL);
      
      accel_group = ctk_accel_group_new ();
      ctk_window_add_accel_group (CTK_WINDOW (window), accel_group);

      ctk_window_set_title (CTK_WINDOW (window), "menus");
      ctk_container_set_border_width (CTK_CONTAINER (window), 0);
      
      
      box1 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
      ctk_container_add (CTK_CONTAINER (window), box1);
      ctk_widget_show (box1);
      
      menubar = ctk_menu_bar_new ();
      ctk_box_pack_start (CTK_BOX (box1), menubar, FALSE, TRUE, 0);
      ctk_widget_show (menubar);
      
      menu = create_menu (screen, 2, 50);
      
      menuitem = ctk_menu_item_new_with_label ("test\nline2");
      ctk_menu_item_set_submenu (CTK_MENU_ITEM (menuitem), menu);
      ctk_menu_shell_append (CTK_MENU_SHELL (menubar), menuitem);
      ctk_widget_show (menuitem);

      menu = create_table_menu (screen, 2, 50);
      
      menuitem = ctk_menu_item_new_with_label ("table");
      ctk_menu_item_set_submenu (CTK_MENU_ITEM (menuitem), menu);
      ctk_menu_shell_append (CTK_MENU_SHELL (menubar), menuitem);
      ctk_widget_show (menuitem);
      
      menuitem = ctk_menu_item_new_with_label ("foo");
      ctk_menu_item_set_submenu (CTK_MENU_ITEM (menuitem), create_menu (screen, 3, 5));
      ctk_menu_shell_append (CTK_MENU_SHELL (menubar), menuitem);
      ctk_widget_show (menuitem);

      image = ctk_image_new_from_icon_name ("help-browser",
                                            CTK_ICON_SIZE_MENU);
      ctk_widget_show (image);
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      menuitem = ctk_image_menu_item_new_with_label ("Help");
      ctk_image_menu_item_set_image (CTK_IMAGE_MENU_ITEM (menuitem), image);
      ctk_image_menu_item_set_always_show_image (CTK_IMAGE_MENU_ITEM (menuitem), TRUE);
      G_GNUC_END_IGNORE_DEPRECATIONS;
      ctk_menu_item_set_submenu (CTK_MENU_ITEM (menuitem), create_menu (screen, 4, 5));
      ctk_widget_set_hexpand (menuitem, TRUE);
      ctk_widget_set_halign (menuitem, CTK_ALIGN_END);
      ctk_menu_shell_append (CTK_MENU_SHELL (menubar), menuitem);
      ctk_widget_show (menuitem);
      
      menubar = ctk_menu_bar_new ();
      ctk_box_pack_start (CTK_BOX (box1), menubar, FALSE, TRUE, 0);
      ctk_widget_show (menubar);
      
      menu = create_menu (screen, 2, 10);
      
      menuitem = ctk_menu_item_new_with_label ("Second menu bar");
      ctk_menu_item_set_submenu (CTK_MENU_ITEM (menuitem), menu);
      ctk_menu_shell_append (CTK_MENU_SHELL (menubar), menuitem);
      ctk_widget_show (menuitem);
      
      box2 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 10);
      ctk_container_set_border_width (CTK_CONTAINER (box2), 10);
      ctk_box_pack_start (CTK_BOX (box1), box2, TRUE, TRUE, 0);
      ctk_widget_show (box2);
      
      menu = create_menu (screen, 1, 5);
      ctk_menu_set_accel_group (CTK_MENU (menu), accel_group);

      menuitem = ctk_check_menu_item_new_with_label ("Accelerate Me");
      ctk_menu_shell_append (CTK_MENU_SHELL (menu), menuitem);
      ctk_widget_show (menuitem);
      ctk_widget_add_accelerator (menuitem,
				  "activate",
				  accel_group,
				  CDK_KEY_F1,
				  0,
				  CTK_ACCEL_VISIBLE);
      menuitem = ctk_check_menu_item_new_with_label ("Accelerator Locked");
      ctk_menu_shell_append (CTK_MENU_SHELL (menu), menuitem);
      ctk_widget_show (menuitem);
      ctk_widget_add_accelerator (menuitem,
				  "activate",
				  accel_group,
				  CDK_KEY_F2,
				  0,
				  CTK_ACCEL_VISIBLE | CTK_ACCEL_LOCKED);
      menuitem = ctk_check_menu_item_new_with_label ("Accelerators Frozen");
      ctk_menu_shell_append (CTK_MENU_SHELL (menu), menuitem);
      ctk_widget_show (menuitem);
      ctk_widget_add_accelerator (menuitem,
				  "activate",
				  accel_group,
				  CDK_KEY_F2,
				  0,
				  CTK_ACCEL_VISIBLE);
      ctk_widget_add_accelerator (menuitem,
				  "activate",
				  accel_group,
				  CDK_KEY_F3,
				  0,
				  CTK_ACCEL_VISIBLE);

      optionmenu = ctk_combo_box_text_new ();
      ctk_combo_box_set_active (CTK_COMBO_BOX (optionmenu), 3);
      ctk_box_pack_start (CTK_BOX (box2), optionmenu, TRUE, TRUE, 0);
      ctk_widget_show (optionmenu);

      separator = ctk_separator_new (CTK_ORIENTATION_HORIZONTAL);
      ctk_box_pack_start (CTK_BOX (box1), separator, FALSE, TRUE, 0);
      ctk_widget_show (separator);

      box2 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 10);
      ctk_container_set_border_width (CTK_CONTAINER (box2), 10);
      ctk_box_pack_start (CTK_BOX (box1), box2, FALSE, TRUE, 0);
      ctk_widget_show (box2);

      button = ctk_button_new_with_label ("close");
      g_signal_connect_swapped (button, "clicked",
			        G_CALLBACK (ctk_widget_destroy),
				window);
      ctk_box_pack_start (CTK_BOX (box2), button, TRUE, TRUE, 0);
      ctk_widget_set_can_default (button, TRUE);
      ctk_widget_grab_default (button);
      ctk_widget_show (button);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show (window);
  else
    ctk_widget_destroy (window);
}


static CtkWidget *
accel_button_new (CtkAccelGroup *accel_group,
		  const gchar   *text,
		  const gchar   *accel)
{
  guint keyval;
  CdkModifierType modifiers;
  CtkWidget *button;
  CtkWidget *label;

  ctk_accelerator_parse (accel, &keyval, &modifiers);
  g_assert (keyval);

  button = ctk_button_new ();
  ctk_widget_add_accelerator (button, "activate", accel_group,
			      keyval, modifiers, CTK_ACCEL_VISIBLE | CTK_ACCEL_LOCKED);

  label = ctk_accel_label_new (text);
  ctk_accel_label_set_accel_widget (CTK_ACCEL_LABEL (label), button);
  ctk_widget_show (label);
  
  ctk_container_add (CTK_CONTAINER (button), label);

  return button;
}

static void
create_key_lookup (CtkWidget *widget)
{
  static CtkWidget *window = NULL;
  gpointer window_ptr;

  if (!window)
    {
      CtkAccelGroup *accel_group = ctk_accel_group_new ();
      CtkWidget *button;
      CtkWidget *content_area;

      window = ctk_dialog_new_with_buttons ("Key Lookup", NULL, 0,
					    "_Close", CTK_RESPONSE_CLOSE,
					    NULL);

      ctk_window_set_screen (CTK_WINDOW (window),
			     ctk_widget_get_screen (widget));

      /* We have to expand it so the accel labels will draw their labels
       */
      ctk_window_set_default_size (CTK_WINDOW (window), 300, -1);
      
      ctk_window_add_accel_group (CTK_WINDOW (window), accel_group);

      content_area = ctk_dialog_get_content_area (CTK_DIALOG (window));
      
      button = ctk_button_new_with_mnemonic ("Button 1 (_a)");
      ctk_box_pack_start (CTK_BOX (content_area), button, FALSE, FALSE, 0);
      button = ctk_button_new_with_mnemonic ("Button 2 (_A)");
      ctk_box_pack_start (CTK_BOX (content_area), button, FALSE, FALSE, 0);
      button = ctk_button_new_with_mnemonic ("Button 3 (_\321\204)");
      ctk_box_pack_start (CTK_BOX (content_area), button, FALSE, FALSE, 0);
      button = ctk_button_new_with_mnemonic ("Button 4 (_\320\244)");
      ctk_box_pack_start (CTK_BOX (content_area), button, FALSE, FALSE, 0);
      button = ctk_button_new_with_mnemonic ("Button 6 (_b)");
      ctk_box_pack_start (CTK_BOX (content_area), button, FALSE, FALSE, 0);
      button = accel_button_new (accel_group, "Button 7", "<Alt><Shift>b");
      ctk_box_pack_start (CTK_BOX (content_area), button, FALSE, FALSE, 0);
      button = accel_button_new (accel_group, "Button 8", "<Alt>d");
      ctk_box_pack_start (CTK_BOX (content_area), button, FALSE, FALSE, 0);
      button = accel_button_new (accel_group, "Button 9", "<Alt>Cyrillic_ve");
      ctk_box_pack_start (CTK_BOX (content_area), button, FALSE, FALSE, 0);
      button = ctk_button_new_with_mnemonic ("Button 10 (_1)");
      ctk_box_pack_start (CTK_BOX (content_area), button, FALSE, FALSE, 0);
      button = ctk_button_new_with_mnemonic ("Button 11 (_!)");
      ctk_box_pack_start (CTK_BOX (content_area), button, FALSE, FALSE, 0);
      button = accel_button_new (accel_group, "Button 12", "<Super>a");
      ctk_box_pack_start (CTK_BOX (content_area), button, FALSE, FALSE, 0);
      button = accel_button_new (accel_group, "Button 13", "<Hyper>a");
      ctk_box_pack_start (CTK_BOX (content_area), button, FALSE, FALSE, 0);
      button = accel_button_new (accel_group, "Button 14", "<Meta>a");
      ctk_box_pack_start (CTK_BOX (content_area), button, FALSE, FALSE, 0);
      button = accel_button_new (accel_group, "Button 15", "<Shift><Mod4>b");
      ctk_box_pack_start (CTK_BOX (content_area), button, FALSE, FALSE, 0);

      window_ptr = &window;
      g_object_add_weak_pointer (G_OBJECT (window), window_ptr);
      g_signal_connect (window, "response", G_CALLBACK (ctk_widget_destroy), NULL);

      ctk_widget_show_all (window);
    }
  else
    ctk_widget_destroy (window);
}


/*
 create_modal_window
 */

static gboolean
cmw_destroy_cb(CtkWidget *widget)
{
  /* This is needed to get out of ctk_main */
  ctk_main_quit ();

  return FALSE;
}

static void
cmw_color (CtkWidget *widget, CtkWidget *parent)
{
    CtkWidget *csd;

    csd = ctk_color_chooser_dialog_new ("This is a modal color selection dialog", CTK_WINDOW (parent));

    /* Set as modal */
    ctk_window_set_modal (CTK_WINDOW(csd),TRUE);

    g_signal_connect (csd, "destroy",
		      G_CALLBACK (cmw_destroy_cb), NULL);
    g_signal_connect (csd, "response",
                      G_CALLBACK (ctk_widget_destroy), NULL);
    
    /* wait until destroy calls ctk_main_quit */
    ctk_widget_show (csd);    
    ctk_main ();
}

static void
cmw_file (CtkWidget *widget, CtkWidget *parent)
{
    CtkWidget *fs;

    fs = ctk_file_chooser_dialog_new ("This is a modal file selection dialog",
                                      CTK_WINDOW (parent),
                                      CTK_FILE_CHOOSER_ACTION_OPEN,
                                      "_Open", CTK_RESPONSE_ACCEPT,
                                      "_Cancel", CTK_RESPONSE_CANCEL,
                                      NULL);
    ctk_window_set_screen (CTK_WINDOW (fs), ctk_widget_get_screen (parent));
    ctk_window_set_modal (CTK_WINDOW (fs), TRUE);

    g_signal_connect (fs, "destroy",
                      G_CALLBACK (cmw_destroy_cb), NULL);
    g_signal_connect_swapped (fs, "response",
                      G_CALLBACK (ctk_widget_destroy), fs);

    /* wait until destroy calls ctk_main_quit */
    ctk_widget_show (fs);
    ctk_main();
}


static void
create_modal_window (CtkWidget *widget)
{
  CtkWidget *window = NULL;
  CtkWidget *box1,*box2;
  CtkWidget *frame1;
  CtkWidget *btnColor,*btnFile,*btnClose;

  /* Create modal window (Here you can use any window descendent )*/
  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_screen (CTK_WINDOW (window),
			 ctk_widget_get_screen (widget));

  ctk_window_set_title (CTK_WINDOW(window),"This window is modal");

  /* Set window as modal */
  ctk_window_set_modal (CTK_WINDOW(window),TRUE);

  /* Create widgets */
  box1 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 5);
  frame1 = ctk_frame_new ("Standard dialogs in modal form");
  box2 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 5);
  ctk_box_set_homogeneous (CTK_BOX (box2), TRUE);
  btnColor = ctk_button_new_with_label ("Color");
  btnFile = ctk_button_new_with_label ("File Selection");
  btnClose = ctk_button_new_with_label ("Close");

  /* Init widgets */
  ctk_container_set_border_width (CTK_CONTAINER (box1), 3);
  ctk_container_set_border_width (CTK_CONTAINER (box2), 3);
    
  /* Pack widgets */
  ctk_container_add (CTK_CONTAINER (window), box1);
  ctk_box_pack_start (CTK_BOX (box1), frame1, TRUE, TRUE, 4);
  ctk_container_add (CTK_CONTAINER (frame1), box2);
  ctk_box_pack_start (CTK_BOX (box2), btnColor, FALSE, FALSE, 4);
  ctk_box_pack_start (CTK_BOX (box2), btnFile, FALSE, FALSE, 4);
  ctk_box_pack_start (CTK_BOX (box1), ctk_separator_new (CTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 4);
  ctk_box_pack_start (CTK_BOX (box1), btnClose, FALSE, FALSE, 4);
   
  /* connect signals */
  g_signal_connect_swapped (btnClose, "clicked",
			    G_CALLBACK (ctk_widget_destroy), window);

  g_signal_connect (window, "destroy",
                    G_CALLBACK (cmw_destroy_cb), NULL);
  
  g_signal_connect (btnColor, "clicked",
                    G_CALLBACK (cmw_color), window);
  g_signal_connect (btnFile, "clicked",
                    G_CALLBACK (cmw_file), window);

  /* Show widgets */
  ctk_widget_show_all (window);

  /* wait until dialog get destroyed */
  ctk_main();
}

/*
 * CtkMessageDialog
 */

static void
make_message_dialog (CdkScreen *screen,
		     CtkWidget **dialog,
                     CtkMessageType  type,
                     CtkButtonsType  buttons,
		     guint           default_response)
{
  if (*dialog)
    {
      ctk_widget_destroy (*dialog);

      return;
    }

  *dialog = ctk_message_dialog_new (NULL, 0, type, buttons,
                                    "This is a message dialog; it can wrap long lines. This is a long line. La la la. Look this line is wrapped. Blah blah blah blah blah blah. (Note: testctk has a nonstandard ctkrc that changes some of the message dialog icons.)");

  ctk_window_set_screen (CTK_WINDOW (*dialog), screen);

  g_signal_connect_swapped (*dialog,
			    "response",
			    G_CALLBACK (ctk_widget_destroy),
			    *dialog);
  
  g_signal_connect (*dialog,
                    "destroy",
                    G_CALLBACK (ctk_widget_destroyed),
                    dialog);

  ctk_dialog_set_default_response (CTK_DIALOG (*dialog), default_response);

  ctk_widget_show (*dialog);
}

static void
create_message_dialog (CtkWidget *widget)
{
  static CtkWidget *info = NULL;
  static CtkWidget *warning = NULL;
  static CtkWidget *error = NULL;
  static CtkWidget *question = NULL;
  CdkScreen *screen = ctk_widget_get_screen (widget);

  make_message_dialog (screen, &info, CTK_MESSAGE_INFO, CTK_BUTTONS_OK, CTK_RESPONSE_OK);
  make_message_dialog (screen, &warning, CTK_MESSAGE_WARNING, CTK_BUTTONS_CLOSE, CTK_RESPONSE_CLOSE);
  make_message_dialog (screen, &error, CTK_MESSAGE_ERROR, CTK_BUTTONS_OK_CANCEL, CTK_RESPONSE_OK);
  make_message_dialog (screen, &question, CTK_MESSAGE_QUESTION, CTK_BUTTONS_YES_NO, CTK_RESPONSE_NO);
}

/*
 * CtkScrolledWindow
 */

static CtkWidget *sw_parent = NULL;
static CtkWidget *sw_float_parent;
static gulong sw_destroyed_handler = 0;

static gboolean
scrolled_windows_delete_cb (CtkWidget *widget, CdkEventAny *event, CtkWidget *scrollwin)
{
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  ctk_widget_reparent (scrollwin, sw_parent);
G_GNUC_END_IGNORE_DEPRECATIONS
  
  g_signal_handler_disconnect (sw_parent, sw_destroyed_handler);
  sw_float_parent = NULL;
  sw_parent = NULL;
  sw_destroyed_handler = 0;

  return FALSE;
}

static void
scrolled_windows_destroy_cb (CtkWidget *widget, CtkWidget *scrollwin)
{
  ctk_widget_destroy (sw_float_parent);

  sw_float_parent = NULL;
  sw_parent = NULL;
  sw_destroyed_handler = 0;
}

static void
scrolled_windows_remove (CtkWidget *dialog, gint response, CtkWidget *scrollwin)
{
  if (response != CTK_RESPONSE_APPLY)
    {
      ctk_widget_destroy (dialog);
      return;
    }

  if (sw_parent)
    {
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      ctk_widget_reparent (scrollwin, sw_parent);
G_GNUC_END_IGNORE_DEPRECATIONS
      ctk_widget_destroy (sw_float_parent);

      g_signal_handler_disconnect (sw_parent, sw_destroyed_handler);
      sw_float_parent = NULL;
      sw_parent = NULL;
      sw_destroyed_handler = 0;
    }
  else
    {
      sw_parent = ctk_widget_get_parent (scrollwin);
      sw_float_parent = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_screen (CTK_WINDOW (sw_float_parent),
			     ctk_widget_get_screen (dialog));
      
      ctk_window_set_default_size (CTK_WINDOW (sw_float_parent), 200, 200);
      
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      ctk_widget_reparent (scrollwin, sw_float_parent);
G_GNUC_END_IGNORE_DEPRECATIONS
      ctk_widget_show (sw_float_parent);

      sw_destroyed_handler =
	g_signal_connect (sw_parent, "destroy",
			  G_CALLBACK (scrolled_windows_destroy_cb), scrollwin);
      g_signal_connect (sw_float_parent, "delete_event",
			G_CALLBACK (scrolled_windows_delete_cb), scrollwin);
    }
}

static void
create_scrolled_windows (CtkWidget *widget)
{
  static CtkWidget *window;
  CtkWidget *content_area;
  CtkWidget *scrolled_window;
  CtkWidget *button;
  CtkWidget *grid;
  char buffer[32];
  int i, j;

  if (!window)
    {
      window = ctk_dialog_new ();

      ctk_window_set_screen (CTK_WINDOW (window),
			     ctk_widget_get_screen (widget));

      g_signal_connect (window, "destroy",
			G_CALLBACK (ctk_widget_destroyed),
			&window);

      content_area = ctk_dialog_get_content_area (CTK_DIALOG (window));

      ctk_window_set_title (CTK_WINDOW (window), "dialog");
      ctk_container_set_border_width (CTK_CONTAINER (window), 0);

      scrolled_window = ctk_scrolled_window_new (NULL, NULL);
      ctk_container_set_border_width (CTK_CONTAINER (scrolled_window), 10);
      ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (scrolled_window),
				      CTK_POLICY_AUTOMATIC,
				      CTK_POLICY_AUTOMATIC);
      ctk_box_pack_start (CTK_BOX (content_area), scrolled_window, TRUE, TRUE, 0);
      ctk_widget_show (scrolled_window);

      grid = ctk_grid_new ();
      ctk_grid_set_row_spacing (CTK_GRID (grid), 10);
      ctk_grid_set_column_spacing (CTK_GRID (grid), 10);
      ctk_container_add (CTK_CONTAINER (scrolled_window), grid);
      ctk_container_set_focus_hadjustment (CTK_CONTAINER (grid),
					   ctk_scrolled_window_get_hadjustment (CTK_SCROLLED_WINDOW (scrolled_window)));
      ctk_container_set_focus_vadjustment (CTK_CONTAINER (grid),
					   ctk_scrolled_window_get_vadjustment (CTK_SCROLLED_WINDOW (scrolled_window)));
      ctk_widget_show (grid);

      for (i = 0; i < 20; i++)
	for (j = 0; j < 20; j++)
	  {
	    sprintf (buffer, "button (%d,%d)\n", i, j);
	    button = ctk_toggle_button_new_with_label (buffer);
	    ctk_grid_attach (CTK_GRID (grid), button, i, j, 1, 1);
	    ctk_widget_show (button);
	  }

      ctk_dialog_add_button (CTK_DIALOG (window),
                             "Close",
                             CTK_RESPONSE_CLOSE);

      ctk_dialog_add_button (CTK_DIALOG (window),
                             "Reparent Out",
                             CTK_RESPONSE_APPLY);

      g_signal_connect (window, "response",
			G_CALLBACK (scrolled_windows_remove),
			scrolled_window);

      ctk_window_set_default_size (CTK_WINDOW (window), 300, 300);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show (window);
  else
    ctk_widget_destroy (window);
}

/*
 * CtkEntry
 */

static void
entry_toggle_frame (CtkWidget *checkbutton,
                    CtkWidget *entry)
{
   ctk_entry_set_has_frame (CTK_ENTRY(entry),
                            ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (checkbutton)));
}

static void
entry_toggle_sensitive (CtkWidget *checkbutton,
			CtkWidget *entry)
{
   ctk_widget_set_sensitive (entry,
                             ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON(checkbutton)));
}

static gboolean
entry_progress_timeout (gpointer data)
{
  if (GPOINTER_TO_INT (g_object_get_data (G_OBJECT (data), "progress-pulse")))
    {
      ctk_entry_progress_pulse (CTK_ENTRY (data));
    }
  else
    {
      gdouble fraction;

      fraction = ctk_entry_get_progress_fraction (CTK_ENTRY (data));

      fraction += 0.05;
      if (fraction > 1.0001)
        fraction = 0.0;

      ctk_entry_set_progress_fraction (CTK_ENTRY (data), fraction);
    }

  return G_SOURCE_CONTINUE;
}

static void
entry_remove_timeout (gpointer data)
{
  g_source_remove (GPOINTER_TO_UINT (data));
}

static void
entry_toggle_progress (CtkWidget *checkbutton,
                       CtkWidget *entry)
{
  if (ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (checkbutton)))
    {
      guint timeout = cdk_threads_add_timeout (100,
                                               entry_progress_timeout,
                                               entry);
      g_object_set_data_full (G_OBJECT (entry), "timeout-id",
                              GUINT_TO_POINTER (timeout),
                              entry_remove_timeout);
    }
  else
    {
      g_object_set_data (G_OBJECT (entry), "timeout-id",
                         GUINT_TO_POINTER (0));

      ctk_entry_set_progress_fraction (CTK_ENTRY (entry), 0.0);
    }
}

static void
entry_toggle_pulse (CtkWidget *checkbutton,
                    CtkWidget *entry)
{
  g_object_set_data (G_OBJECT (entry), "progress-pulse",
                     GUINT_TO_POINTER ((guint) ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (checkbutton))));
}

static void
create_entry (CtkWidget *widget)
{
  static CtkWidget *window = NULL;
  CtkWidget *box1;
  CtkWidget *box2;
  CtkWidget *hbox;
  CtkWidget *has_frame_check;
  CtkWidget *sensitive_check;
  CtkWidget *progress_check;
  CtkWidget *entry;
  CtkComboBoxText *cb;
  CtkWidget *cb_entry;
  CtkWidget *button;
  CtkWidget *separator;

  if (!window)
    {
      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_screen (CTK_WINDOW (window),
			     ctk_widget_get_screen (widget));

      g_signal_connect (window, "destroy",
			G_CALLBACK (ctk_widget_destroyed),
			&window);

      ctk_window_set_title (CTK_WINDOW (window), "entry");
      ctk_container_set_border_width (CTK_CONTAINER (window), 0);


      box1 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
      ctk_container_add (CTK_CONTAINER (window), box1);


      box2 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 10);
      ctk_container_set_border_width (CTK_CONTAINER (box2), 10);
      ctk_box_pack_start (CTK_BOX (box1), box2, TRUE, TRUE, 0);

      hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 5);
      ctk_box_pack_start (CTK_BOX (box2), hbox, TRUE, TRUE, 0);
      
      entry = ctk_entry_new ();
      ctk_entry_set_text (CTK_ENTRY (entry), "hello world \330\247\331\204\330\263\331\204\330\247\331\205 \330\271\331\204\331\212\331\203\331\205");
      ctk_editable_select_region (CTK_EDITABLE (entry), 0, 5);
      ctk_box_pack_start (CTK_BOX (hbox), entry, TRUE, TRUE, 0);

      cb = CTK_COMBO_BOX_TEXT (ctk_combo_box_text_new_with_entry ());

      ctk_combo_box_text_append_text (cb, "item0");
      ctk_combo_box_text_append_text (cb, "item0");
      ctk_combo_box_text_append_text (cb, "item1 item1");
      ctk_combo_box_text_append_text (cb, "item2 item2 item2");
      ctk_combo_box_text_append_text (cb, "item3 item3 item3 item3");
      ctk_combo_box_text_append_text (cb, "item4 item4 item4 item4 item4");
      ctk_combo_box_text_append_text (cb, "item5 item5 item5 item5 item5 item5");
      ctk_combo_box_text_append_text (cb, "item6 item6 item6 item6 item6");
      ctk_combo_box_text_append_text (cb, "item7 item7 item7 item7");
      ctk_combo_box_text_append_text (cb, "item8 item8 item8");
      ctk_combo_box_text_append_text (cb, "item9 item9");

      cb_entry = ctk_bin_get_child (CTK_BIN (cb));
      ctk_entry_set_text (CTK_ENTRY (cb_entry), "hello world \n\n\n foo");
      ctk_editable_select_region (CTK_EDITABLE (cb_entry), 0, -1);
      ctk_box_pack_start (CTK_BOX (box2), CTK_WIDGET (cb), TRUE, TRUE, 0);

      sensitive_check = ctk_check_button_new_with_label("Sensitive");
      ctk_box_pack_start (CTK_BOX (box2), sensitive_check, FALSE, TRUE, 0);
      g_signal_connect (sensitive_check, "toggled",
			G_CALLBACK (entry_toggle_sensitive), entry);
      ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (sensitive_check), TRUE);

      has_frame_check = ctk_check_button_new_with_label("Has Frame");
      ctk_box_pack_start (CTK_BOX (box2), has_frame_check, FALSE, TRUE, 0);
      g_signal_connect (has_frame_check, "toggled",
			G_CALLBACK (entry_toggle_frame), entry);
      ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (has_frame_check), TRUE);

      progress_check = ctk_check_button_new_with_label("Show Progress");
      ctk_box_pack_start (CTK_BOX (box2), progress_check, FALSE, TRUE, 0);
      g_signal_connect (progress_check, "toggled",
			G_CALLBACK (entry_toggle_progress), entry);

      progress_check = ctk_check_button_new_with_label("Pulse Progress");
      ctk_box_pack_start (CTK_BOX (box2), progress_check, FALSE, TRUE, 0);
      g_signal_connect (progress_check, "toggled",
			G_CALLBACK (entry_toggle_pulse), entry);

      separator = ctk_separator_new (CTK_ORIENTATION_HORIZONTAL);
      ctk_box_pack_start (CTK_BOX (box1), separator, FALSE, TRUE, 0);

      box2 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 10);
      ctk_container_set_border_width (CTK_CONTAINER (box2), 10);
      ctk_box_pack_start (CTK_BOX (box1), box2, FALSE, TRUE, 0);

      button = ctk_button_new_with_label ("close");
      g_signal_connect_swapped (button, "clicked",
			        G_CALLBACK (ctk_widget_destroy),
				window);
      ctk_box_pack_start (CTK_BOX (box2), button, TRUE, TRUE, 0);
      ctk_widget_set_can_default (button, TRUE);
      ctk_widget_grab_default (button);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);
}

static void
create_expander (CtkWidget *widget)
{
  CtkWidget *box1;
  CtkWidget *expander;
  CtkWidget *hidden;
  static CtkWidget *window = NULL;

  if (!window)
    {
      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_screen (CTK_WINDOW (window),
			     ctk_widget_get_screen (widget));
      
      g_signal_connect (window, "destroy",
			G_CALLBACK (ctk_widget_destroyed),
			&window);
      
      ctk_window_set_title (CTK_WINDOW (window), "expander");
      ctk_container_set_border_width (CTK_CONTAINER (window), 0);
      
      box1 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
      ctk_container_add (CTK_CONTAINER (window), box1);
      
      expander = ctk_expander_new ("The Hidden");
      
      ctk_box_pack_start (CTK_BOX (box1), expander, TRUE, TRUE, 0);
      
      hidden = ctk_label_new ("Revealed!");
      
      ctk_container_add (CTK_CONTAINER (expander), hidden);
    }
  
  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);
}


/* CtkEventBox */


static gboolean
event_box_draw (CtkWidget *widget,
		cairo_t   *cr,
		gpointer   user_data)
{
  if (ctk_widget_get_window (widget) ==
      ctk_widget_get_window (ctk_widget_get_parent (widget)))
    return FALSE;

  cairo_set_source_rgb (cr, 0, 1, 0);
  cairo_paint (cr);

  return FALSE;
}

static void
event_box_label_pressed (CtkWidget        *widget,
			 CdkEventButton   *event,
			 gpointer user_data)
{
  g_print ("clicked on event box\n");
}

static void
event_box_button_clicked (CtkWidget *widget,
			  CtkWidget *button,
			  gpointer user_data)
{
  g_print ("pushed button\n");
}

static void
event_box_toggle_visible_window (CtkWidget *checkbutton,
				 CtkEventBox *event_box)
{
  ctk_event_box_set_visible_window (event_box,
                                    ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (checkbutton)));
}

static void
event_box_toggle_above_child (CtkWidget *checkbutton,
			      CtkEventBox *event_box)
{
  ctk_event_box_set_above_child (event_box,
                                 ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (checkbutton)));
}

static void
create_event_box (CtkWidget *widget)
{
  static CtkWidget *window = NULL;
  CtkWidget *box1;
  CtkWidget *box2;
  CtkWidget *hbox;
  CtkWidget *vbox;
  CtkWidget *button;
  CtkWidget *separator;
  CtkWidget *event_box;
  CtkWidget *label;
  CtkWidget *visible_window_check;
  CtkWidget *above_child_check;

  if (!window)
    {
      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_screen (CTK_WINDOW (window),
			     ctk_widget_get_screen (widget));

      g_signal_connect (window, "destroy",
			G_CALLBACK (ctk_widget_destroyed),
			&window);

      ctk_window_set_title (CTK_WINDOW (window), "event box");
      ctk_container_set_border_width (CTK_CONTAINER (window), 0);

      box1 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
      ctk_container_add (CTK_CONTAINER (window), box1);

      hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
      ctk_box_pack_start (CTK_BOX (box1), hbox, TRUE, FALSE, 0);
      
      event_box = ctk_event_box_new ();
      ctk_box_pack_start (CTK_BOX (hbox), event_box, TRUE, FALSE, 0);

      vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
      ctk_container_add (CTK_CONTAINER (event_box), vbox);
      g_signal_connect (event_box, "button_press_event",
			G_CALLBACK (event_box_label_pressed),
			NULL);
      g_signal_connect (event_box, "draw",
			G_CALLBACK (event_box_draw),
			NULL);
      
      label = ctk_label_new ("Click on this label");
      ctk_box_pack_start (CTK_BOX (vbox), label, TRUE, FALSE, 0);

      button = ctk_button_new_with_label ("button in eventbox");
      ctk_box_pack_start (CTK_BOX (vbox), button, TRUE, FALSE, 0);
      g_signal_connect (button, "clicked",
			G_CALLBACK (event_box_button_clicked),
			NULL);
      

      visible_window_check = ctk_check_button_new_with_label("Visible Window");
      ctk_box_pack_start (CTK_BOX (box1), visible_window_check, FALSE, TRUE, 0);
      g_signal_connect (visible_window_check, "toggled",
			G_CALLBACK (event_box_toggle_visible_window), event_box);
      ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (visible_window_check), TRUE);
      
      above_child_check = ctk_check_button_new_with_label("Above Child");
      ctk_box_pack_start (CTK_BOX (box1), above_child_check, FALSE, TRUE, 0);
      g_signal_connect (above_child_check, "toggled",
			G_CALLBACK (event_box_toggle_above_child), event_box);
      ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (above_child_check), FALSE);
      
      separator = ctk_separator_new (CTK_ORIENTATION_HORIZONTAL);
      ctk_box_pack_start (CTK_BOX (box1), separator, FALSE, TRUE, 0);

      box2 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 10);
      ctk_container_set_border_width (CTK_CONTAINER (box2), 10);
      ctk_box_pack_start (CTK_BOX (box1), box2, FALSE, TRUE, 0);

      button = ctk_button_new_with_label ("close");
      g_signal_connect_swapped (button, "clicked",
			        G_CALLBACK (ctk_widget_destroy),
				window);
      ctk_box_pack_start (CTK_BOX (box2), button, TRUE, TRUE, 0);
      ctk_widget_set_can_default (button, TRUE);
      ctk_widget_grab_default (button);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);
}


/*
 * CtkSizeGroup
 */

#define SIZE_GROUP_INITIAL_SIZE 50

static void
size_group_hsize_changed (CtkSpinButton *spin_button,
			  CtkWidget     *button)
{
  ctk_widget_set_size_request (ctk_bin_get_child (CTK_BIN (button)),
			       ctk_spin_button_get_value_as_int (spin_button),
			       -1);
}

static void
size_group_vsize_changed (CtkSpinButton *spin_button,
			  CtkWidget     *button)
{
  ctk_widget_set_size_request (ctk_bin_get_child (CTK_BIN (button)),
			       -1,
			       ctk_spin_button_get_value_as_int (spin_button));
}

static CtkWidget *
create_size_group_window (CdkScreen    *screen,
			  CtkSizeGroup *master_size_group)
{
  CtkWidget *content_area;
  CtkWidget *window;
  CtkWidget *grid;
  CtkWidget *main_button;
  CtkWidget *button;
  CtkWidget *spin_button;
  CtkWidget *hbox;
  CtkSizeGroup *hgroup1;
  CtkSizeGroup *hgroup2;
  CtkSizeGroup *vgroup1;
  CtkSizeGroup *vgroup2;

  window = ctk_dialog_new_with_buttons ("CtkSizeGroup",
					NULL, 0,
					"_Close",
					CTK_RESPONSE_NONE,
					NULL);

  ctk_window_set_screen (CTK_WINDOW (window), screen);

  ctk_window_set_resizable (CTK_WINDOW (window), TRUE);

  g_signal_connect (window, "response",
		    G_CALLBACK (ctk_widget_destroy),
		    NULL);

  content_area = ctk_dialog_get_content_area (CTK_DIALOG (window));

  grid = ctk_grid_new ();
  ctk_box_pack_start (CTK_BOX (content_area), grid, TRUE, TRUE, 0);

  ctk_grid_set_row_spacing (CTK_GRID (grid), 5);
  ctk_grid_set_column_spacing (CTK_GRID (grid), 5);
  ctk_container_set_border_width (CTK_CONTAINER (grid), 5);
  ctk_widget_set_size_request (grid, 250, 250);

  hgroup1 = ctk_size_group_new (CTK_SIZE_GROUP_HORIZONTAL);
  hgroup2 = ctk_size_group_new (CTK_SIZE_GROUP_HORIZONTAL);
  vgroup1 = ctk_size_group_new (CTK_SIZE_GROUP_VERTICAL);
  vgroup2 = ctk_size_group_new (CTK_SIZE_GROUP_VERTICAL);

  main_button = ctk_button_new_with_label ("X");
  ctk_widget_set_hexpand (main_button, TRUE);
  ctk_widget_set_vexpand (main_button, TRUE);
  ctk_widget_set_halign (main_button, CTK_ALIGN_CENTER);
  ctk_widget_set_valign (main_button, CTK_ALIGN_CENTER);
  ctk_grid_attach (CTK_GRID (grid), main_button, 0, 0, 1, 1);
  
  ctk_size_group_add_widget (master_size_group, main_button);
  ctk_size_group_add_widget (hgroup1, main_button);
  ctk_size_group_add_widget (vgroup1, main_button);
  ctk_widget_set_size_request (ctk_bin_get_child (CTK_BIN (main_button)),
			       SIZE_GROUP_INITIAL_SIZE,
			       SIZE_GROUP_INITIAL_SIZE);

  button = ctk_button_new ();
  ctk_widget_set_hexpand (button, TRUE);
  ctk_widget_set_vexpand (button, TRUE);
  ctk_widget_set_halign (button, CTK_ALIGN_CENTER);
  ctk_widget_set_valign (button, CTK_ALIGN_CENTER);
  ctk_grid_attach (CTK_GRID (grid), button, 1, 0, 1, 1);

  ctk_size_group_add_widget (vgroup1, button);
  ctk_size_group_add_widget (vgroup2, button);

  button = ctk_button_new ();
  ctk_widget_set_hexpand (button, TRUE);
  ctk_widget_set_vexpand (button, TRUE);
  ctk_widget_set_halign (button, CTK_ALIGN_CENTER);
  ctk_widget_set_valign (button, CTK_ALIGN_CENTER);
  ctk_grid_attach (CTK_GRID (grid), button, 0, 1, 1, 1);

  ctk_size_group_add_widget (hgroup1, button);
  ctk_size_group_add_widget (hgroup2, button);

  button = ctk_button_new ();
  ctk_widget_set_hexpand (button, TRUE);
  ctk_widget_set_vexpand (button, TRUE);
  ctk_widget_set_halign (button, CTK_ALIGN_CENTER);
  ctk_widget_set_valign (button, CTK_ALIGN_CENTER);
  ctk_grid_attach (CTK_GRID (grid), button, 1, 1, 1, 1);

  ctk_size_group_add_widget (hgroup2, button);
  ctk_size_group_add_widget (vgroup2, button);

  g_object_unref (hgroup1);
  g_object_unref (hgroup2);
  g_object_unref (vgroup1);
  g_object_unref (vgroup2);
  
  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 5);
  ctk_box_pack_start (CTK_BOX (content_area), hbox, FALSE, FALSE, 0);
  
  spin_button = ctk_spin_button_new_with_range (1, 100, 1);
  ctk_spin_button_set_value (CTK_SPIN_BUTTON (spin_button), SIZE_GROUP_INITIAL_SIZE);
  ctk_box_pack_start (CTK_BOX (hbox), spin_button, TRUE, TRUE, 0);
  g_signal_connect (spin_button, "value_changed",
		    G_CALLBACK (size_group_hsize_changed), main_button);

  spin_button = ctk_spin_button_new_with_range (1, 100, 1);
  ctk_spin_button_set_value (CTK_SPIN_BUTTON (spin_button), SIZE_GROUP_INITIAL_SIZE);
  ctk_box_pack_start (CTK_BOX (hbox), spin_button, TRUE, TRUE, 0);
  g_signal_connect (spin_button, "value_changed",
		    G_CALLBACK (size_group_vsize_changed), main_button);

  return window;
}

static void
create_size_groups (CtkWidget *widget)
{
  static CtkWidget *window1 = NULL;
  static CtkWidget *window2 = NULL;
  static CtkSizeGroup *master_size_group;

  if (!master_size_group)
    master_size_group = ctk_size_group_new (CTK_SIZE_GROUP_BOTH);

  if (!window1)
    {
      window1 = create_size_group_window (ctk_widget_get_screen (widget),
					  master_size_group);

      g_signal_connect (window1, "destroy",
			G_CALLBACK (ctk_widget_destroyed),
			&window1);
    }

  if (!window2)
    {
      window2 = create_size_group_window (ctk_widget_get_screen (widget),
					  master_size_group);

      g_signal_connect (window2, "destroy",
			G_CALLBACK (ctk_widget_destroyed),
			&window2);
    }

  if (ctk_widget_get_visible (window1) && ctk_widget_get_visible (window2))
    {
      ctk_widget_destroy (window1);
      ctk_widget_destroy (window2);
    }
  else
    {
      if (!ctk_widget_get_visible (window1))
	ctk_widget_show_all (window1);
      if (!ctk_widget_get_visible (window2))
	ctk_widget_show_all (window2);
    }
}

/*
 * CtkSpinButton
 */

static CtkWidget *spinner1;

static void
toggle_snap (CtkWidget *widget, CtkSpinButton *spin)
{
  ctk_spin_button_set_snap_to_ticks (spin,
                                     ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (widget)));
}

static void
toggle_numeric (CtkWidget *widget, CtkSpinButton *spin)
{
  ctk_spin_button_set_numeric (spin,
                               ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (widget)));
}

static void
change_digits (CtkWidget *widget, CtkSpinButton *spin)
{
  ctk_spin_button_set_digits (CTK_SPIN_BUTTON (spinner1),
			      ctk_spin_button_get_value_as_int (spin));
}

static void
get_value (CtkWidget *widget, gpointer data)
{
  gchar buf[32];
  CtkLabel *label;
  CtkSpinButton *spin;

  spin = CTK_SPIN_BUTTON (spinner1);
  label = CTK_LABEL (g_object_get_data (G_OBJECT (widget), "user_data"));
  if (GPOINTER_TO_INT (data) == 1)
    sprintf (buf, "%d", ctk_spin_button_get_value_as_int (spin));
  else
    sprintf (buf, "%0.*f",
             ctk_spin_button_get_digits (spin),
             ctk_spin_button_get_value (spin));

  ctk_label_set_text (label, buf);
}

static void
get_spin_value (CtkWidget *widget, gpointer data)
{
  gchar *buffer;
  CtkLabel *label;
  CtkSpinButton *spin;

  spin = CTK_SPIN_BUTTON (widget);
  label = CTK_LABEL (data);

  buffer = g_strdup_printf ("%0.*f",
                            ctk_spin_button_get_digits (spin),
			    ctk_spin_button_get_value (spin));
  ctk_label_set_text (label, buffer);

  g_free (buffer);
}

static gint
spin_button_time_output_func (CtkSpinButton *spin_button)
{
  CtkAdjustment *adjustment;
  static gchar buf[6];
  gdouble hours;
  gdouble minutes;

  adjustment = ctk_spin_button_get_adjustment (spin_button);
  hours = ctk_adjustment_get_value (adjustment) / 60.0;
  minutes = (fabs(floor (hours) - hours) < 1e-5) ? 0.0 : 30;
  sprintf (buf, "%02.0f:%02.0f", floor (hours), minutes);
  if (strcmp (buf, ctk_entry_get_text (CTK_ENTRY (spin_button))))
    ctk_entry_set_text (CTK_ENTRY (spin_button), buf);
  return TRUE;
}

static gint
spin_button_month_input_func (CtkSpinButton *spin_button,
			      gdouble       *new_val)
{
  gint i;
  static gchar *month[12] = { "January", "February", "March", "April",
			      "May", "June", "July", "August",
			      "September", "October", "November", "December" };
  gchar *tmp1, *tmp2;
  gboolean found = FALSE;

  for (i = 1; i <= 12; i++)
    {
      tmp1 = g_ascii_strup (month[i - 1], -1);
      tmp2 = g_ascii_strup (ctk_entry_get_text (CTK_ENTRY (spin_button)), -1);
      if (strstr (tmp1, tmp2) == tmp1)
	found = TRUE;
      g_free (tmp1);
      g_free (tmp2);
      if (found)
	break;
    }
  if (!found)
    {
      *new_val = 0.0;
      return CTK_INPUT_ERROR;
    }
  *new_val = (gdouble) i;
  return TRUE;
}

static gint
spin_button_month_output_func (CtkSpinButton *spin_button)
{
  CtkAdjustment *adjustment;
  gdouble value;
  gint i;
  static gchar *month[12] = { "January", "February", "March", "April",
			      "May", "June", "July", "August", "September",
			      "October", "November", "December" };

  adjustment = ctk_spin_button_get_adjustment (spin_button);
  value = ctk_adjustment_get_value (adjustment);
  for (i = 1; i <= 12; i++)
    if (fabs (value - (double)i) < 1e-5)
      {
	if (strcmp (month[i-1], ctk_entry_get_text (CTK_ENTRY (spin_button))))
	  ctk_entry_set_text (CTK_ENTRY (spin_button), month[i-1]);
      }
  return TRUE;
}

static gint
spin_button_hex_input_func (CtkSpinButton *spin_button,
			    gdouble       *new_val)
{
  const gchar *buf;
  gchar *err;
  gdouble res;

  buf = ctk_entry_get_text (CTK_ENTRY (spin_button));
  res = strtol(buf, &err, 16);
  *new_val = res;
  if (*err)
    return CTK_INPUT_ERROR;
  else
    return TRUE;
}

static gint
spin_button_hex_output_func (CtkSpinButton *spin_button)
{
  CtkAdjustment *adjustment;
  static gchar buf[7];
  gint val;

  adjustment = ctk_spin_button_get_adjustment (spin_button);
  val = (gint) ctk_adjustment_get_value (adjustment);
  if (fabs (val) < 1e-5)
    sprintf (buf, "0x00");
  else
    sprintf (buf, "0x%.2X", val);
  if (strcmp (buf, ctk_entry_get_text (CTK_ENTRY (spin_button))))
    ctk_entry_set_text (CTK_ENTRY (spin_button), buf);
  return TRUE;
}

static void
create_spins (CtkWidget *widget)
{
  static CtkWidget *window = NULL;
  CtkWidget *frame;
  CtkWidget *hbox;
  CtkWidget *main_vbox;
  CtkWidget *vbox;
  CtkWidget *vbox2;
  CtkWidget *spinner2;
  CtkWidget *spinner;
  CtkWidget *button;
  CtkWidget *label;
  CtkWidget *val_label;
  CtkAdjustment *adjustment;

  if (!window)
    {
      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_screen (CTK_WINDOW (window),
			     ctk_widget_get_screen (widget));
      
      g_signal_connect (window, "destroy",
			G_CALLBACK (ctk_widget_destroyed),
			&window);
      
      ctk_window_set_title (CTK_WINDOW (window), "CtkSpinButton");
      
      main_vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 5);
      ctk_container_set_border_width (CTK_CONTAINER (main_vbox), 10);
      ctk_container_add (CTK_CONTAINER (window), main_vbox);
      
      frame = ctk_frame_new ("Not accelerated");
      ctk_box_pack_start (CTK_BOX (main_vbox), frame, TRUE, TRUE, 0);
      
      vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
      ctk_container_set_border_width (CTK_CONTAINER (vbox), 5);
      ctk_container_add (CTK_CONTAINER (frame), vbox);
      
      /* Time, month, hex spinners */
      
      hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
      ctk_box_pack_start (CTK_BOX (vbox), hbox, TRUE, TRUE, 5);
      
      vbox2 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
      ctk_box_pack_start (CTK_BOX (hbox), vbox2, TRUE, TRUE, 5);
      
      label = ctk_label_new ("Time :");
      ctk_widget_set_halign (label, CTK_ALIGN_START);
      ctk_widget_set_valign (label, CTK_ALIGN_CENTER);
      ctk_box_pack_start (CTK_BOX (vbox2), label, FALSE, TRUE, 0);

      adjustment = ctk_adjustment_new (0, 0, 1410, 30, 60, 0);
      spinner = ctk_spin_button_new (adjustment, 0, 0);
      ctk_editable_set_editable (CTK_EDITABLE (spinner), FALSE);
      g_signal_connect (spinner,
			"output",
			G_CALLBACK (spin_button_time_output_func),
			NULL);
      ctk_spin_button_set_wrap (CTK_SPIN_BUTTON (spinner), TRUE);
      ctk_entry_set_width_chars (CTK_ENTRY (spinner), 5);
      ctk_box_pack_start (CTK_BOX (vbox2), spinner, FALSE, TRUE, 0);

      vbox2 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
      ctk_box_pack_start (CTK_BOX (hbox), vbox2, TRUE, TRUE, 5);
      
      label = ctk_label_new ("Month :");
      ctk_widget_set_halign (label, CTK_ALIGN_START);
      ctk_widget_set_valign (label, CTK_ALIGN_CENTER);
      ctk_box_pack_start (CTK_BOX (vbox2), label, FALSE, TRUE, 0);

      adjustment = ctk_adjustment_new (1.0, 1.0, 12.0, 1.0,
						  5.0, 0.0);
      spinner = ctk_spin_button_new (adjustment, 0, 0);
      ctk_spin_button_set_update_policy (CTK_SPIN_BUTTON (spinner),
					 CTK_UPDATE_IF_VALID);
      g_signal_connect (spinner,
			"input",
			G_CALLBACK (spin_button_month_input_func),
			NULL);
      g_signal_connect (spinner,
			"output",
			G_CALLBACK (spin_button_month_output_func),
			NULL);
      ctk_spin_button_set_wrap (CTK_SPIN_BUTTON (spinner), TRUE);
      ctk_entry_set_width_chars (CTK_ENTRY (spinner), 9);
      ctk_box_pack_start (CTK_BOX (vbox2), spinner, FALSE, TRUE, 0);
      
      vbox2 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
      ctk_box_pack_start (CTK_BOX (hbox), vbox2, TRUE, TRUE, 5);

      label = ctk_label_new ("Hex :");
      ctk_widget_set_halign (label, CTK_ALIGN_START);
      ctk_widget_set_valign (label, CTK_ALIGN_CENTER);
      ctk_box_pack_start (CTK_BOX (vbox2), label, FALSE, TRUE, 0);

      adjustment = ctk_adjustment_new (0, 0, 255, 1, 16, 0);
      spinner = ctk_spin_button_new (adjustment, 0, 0);
      ctk_editable_set_editable (CTK_EDITABLE (spinner), TRUE);
      g_signal_connect (spinner,
			"input",
			G_CALLBACK (spin_button_hex_input_func),
			NULL);
      g_signal_connect (spinner,
			"output",
			G_CALLBACK (spin_button_hex_output_func),
			NULL);
      ctk_spin_button_set_wrap (CTK_SPIN_BUTTON (spinner), TRUE);
      ctk_entry_set_width_chars (CTK_ENTRY (spinner), 4);
      ctk_box_pack_start (CTK_BOX (vbox2), spinner, FALSE, TRUE, 0);

      frame = ctk_frame_new ("Accelerated");
      ctk_box_pack_start (CTK_BOX (main_vbox), frame, TRUE, TRUE, 0);
  
      vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
      ctk_container_set_border_width (CTK_CONTAINER (vbox), 5);
      ctk_container_add (CTK_CONTAINER (frame), vbox);
      
      hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
      ctk_box_pack_start (CTK_BOX (vbox), hbox, FALSE, TRUE, 5);
      
      vbox2 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
      ctk_box_pack_start (CTK_BOX (hbox), vbox2, FALSE, FALSE, 5);
      
      label = ctk_label_new ("Value :");
      ctk_widget_set_halign (label, CTK_ALIGN_START);
      ctk_widget_set_valign (label, CTK_ALIGN_CENTER);
      ctk_box_pack_start (CTK_BOX (vbox2), label, FALSE, TRUE, 0);

      adjustment = ctk_adjustment_new (0.0, -10000.0, 10000.0,
						  0.5, 100.0, 0.0);
      spinner1 = ctk_spin_button_new (adjustment, 1.0, 2);
      ctk_spin_button_set_wrap (CTK_SPIN_BUTTON (spinner1), TRUE);
      ctk_box_pack_start (CTK_BOX (vbox2), spinner1, FALSE, TRUE, 0);

      vbox2 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
      ctk_box_pack_start (CTK_BOX (hbox), vbox2, FALSE, FALSE, 5);

      label = ctk_label_new ("Digits :");
      ctk_widget_set_halign (label, CTK_ALIGN_START);
      ctk_widget_set_valign (label, CTK_ALIGN_CENTER);
      ctk_box_pack_start (CTK_BOX (vbox2), label, FALSE, TRUE, 0);

      adjustment = ctk_adjustment_new (2, 1, 15, 1, 1, 0);
      spinner2 = ctk_spin_button_new (adjustment, 0.0, 0);
      g_signal_connect (adjustment, "value_changed",
			G_CALLBACK (change_digits),
			spinner2);
      ctk_box_pack_start (CTK_BOX (vbox2), spinner2, FALSE, TRUE, 0);

      hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
      ctk_box_pack_start (CTK_BOX (vbox), hbox, FALSE, FALSE, 5);

      button = ctk_check_button_new_with_label ("Snap to 0.5-ticks");
      g_signal_connect (button, "clicked",
			G_CALLBACK (toggle_snap),
			spinner1);
      ctk_box_pack_start (CTK_BOX (vbox), button, TRUE, TRUE, 0);
      ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (button), TRUE);

      button = ctk_check_button_new_with_label ("Numeric only input mode");
      g_signal_connect (button, "clicked",
			G_CALLBACK (toggle_numeric),
			spinner1);
      ctk_box_pack_start (CTK_BOX (vbox), button, TRUE, TRUE, 0);
      ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (button), TRUE);

      val_label = ctk_label_new ("");

      hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
      ctk_box_pack_start (CTK_BOX (vbox), hbox, FALSE, TRUE, 5);

      button = ctk_button_new_with_label ("Value as Int");
      g_object_set_data (G_OBJECT (button), "user_data", val_label);
      g_signal_connect (button, "clicked",
			G_CALLBACK (get_value),
			GINT_TO_POINTER (1));
      ctk_box_pack_start (CTK_BOX (hbox), button, TRUE, TRUE, 5);

      button = ctk_button_new_with_label ("Value as Float");
      g_object_set_data (G_OBJECT (button), "user_data", val_label);
      g_signal_connect (button, "clicked",
			G_CALLBACK (get_value),
			GINT_TO_POINTER (2));
      ctk_box_pack_start (CTK_BOX (hbox), button, TRUE, TRUE, 5);

      ctk_box_pack_start (CTK_BOX (vbox), val_label, TRUE, TRUE, 0);
      ctk_label_set_text (CTK_LABEL (val_label), "0");

      frame = ctk_frame_new ("Using Convenience Constructor");
      ctk_box_pack_start (CTK_BOX (main_vbox), frame, TRUE, TRUE, 0);
  
      hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
      ctk_container_set_border_width (CTK_CONTAINER (hbox), 5);
      ctk_container_add (CTK_CONTAINER (frame), hbox);
      
      val_label = ctk_label_new ("0.0");

      spinner = ctk_spin_button_new_with_range (0.0, 10.0, 0.009);
      ctk_spin_button_set_value (CTK_SPIN_BUTTON (spinner), 0.0);
      g_signal_connect (spinner, "value_changed",
			G_CALLBACK (get_spin_value), val_label);
      ctk_box_pack_start (CTK_BOX (hbox), spinner, TRUE, TRUE, 5);
      ctk_box_pack_start (CTK_BOX (hbox), val_label, TRUE, TRUE, 5);

      hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
      ctk_box_pack_start (CTK_BOX (main_vbox), hbox, FALSE, TRUE, 0);
  
      button = ctk_button_new_with_label ("Close");
      g_signal_connect_swapped (button, "clicked",
			        G_CALLBACK (ctk_widget_destroy),
				window);
      ctk_box_pack_start (CTK_BOX (hbox), button, TRUE, TRUE, 5);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);
}


/*
 * Cursors
 */

static gint
cursor_draw (CtkWidget *widget,
	     cairo_t   *cr,
	     gpointer   user_data)
{
  int width, height;

  width = ctk_widget_get_allocated_width (widget);
  height = ctk_widget_get_allocated_height (widget);

  cairo_set_fill_rule (cr, CAIRO_FILL_RULE_EVEN_ODD);
  cairo_rectangle (cr, 0, 0, width, height);
  cairo_rectangle (cr, width / 3, height / 3, width / 3, height / 3);
  cairo_clip (cr);

  cairo_set_source_rgb (cr, 1, 1, 1);
  cairo_rectangle (cr, 0, 0, width, height / 2);
  cairo_fill (cr);

  cairo_set_source_rgb (cr, 0, 0, 0);
  cairo_rectangle (cr, 0, height / 2, width, height / 2);
  cairo_fill (cr);

  return TRUE;
}

static const gchar *cursor_names[] = {
    "all-scroll",
    "arrow",
    "bd_double_arrow",
    "boat",
    "bottom_left_corner",
    "bottom_right_corner",
    "bottom_side",
    "bottom_tee",
    "box_spiral",
    "center_ptr",
    "circle",
    "clock",
    "coffee_mug",
    "copy",
    "cross",
    "crossed_circle",
    "cross_reverse",
    "crosshair",
    "diamond_cross",
    "dnd-ask",
    "dnd-copy",
    "dnd-link",
    "dnd-move",
    "dnd-none",
    "dot",
    "dotbox",
    "double_arrow",
    "draft_large",
    "draft_small",
    "draped_box",
    "exchange",
    "fd_double_arrow",
    "fleur",
    "gobbler",
    "gumby",
    "grab",
    "grabbing",
    "hand",
    "hand1",
    "hand2",
    "heart",
    "h_double_arrow",
    "help",
    "icon",
    "iron_cross",
    "left_ptr",
    "left_ptr_help",
    "left_ptr_watch",
    "left_side",
    "left_tee",
    "leftbutton",
    "link",
    "ll_angle",
    "lr_angle",
    "man",
    "middlebutton",
    "mouse",
    "move",
    "pencil",
    "pirate",
    "plus",
    "question_arrow",
    "right_ptr",
    "right_side",
    "right_tee",
    "rightbutton",
    "rtl_logo",
    "sailboat",
    "sb_down_arrow",
    "sb_h_double_arrow",
    "sb_left_arrow",
    "sb_right_arrow",
    "sb_up_arrow",
    "sb_v_double_arrow",
    "shuttle",
    "sizing",
    "spider",
    "spraycan",
    "star",
    "target",
    "tcross",
    "top_left_arrow",
    "top_left_corner",
    "top_right_corner",
    "top_side",
    "top_tee",
    "trek",
    "ul_angle",
    "umbrella",
    "ur_angle",
    "v_double_arrow",
    "vertical-text",
    "watch",
    "X_cursor",
    "xterm",
    "zoom-in",
    "zoom-out"
};

static CtkTreeModel *
cursor_model (void)
{
  CtkListStore *store;
  gint i;
  store = ctk_list_store_new (1, G_TYPE_STRING);

  for (i = 0; i < G_N_ELEMENTS (cursor_names); i++)
    ctk_list_store_insert_with_values (store, NULL, -1, 0, cursor_names[i], -1);

  return (CtkTreeModel *)store;
}

static gint
cursor_event (CtkWidget *widget,
              CdkEvent  *event,
              CtkWidget *entry)
{
  const gchar *name;
  gint i;
  const gint n = G_N_ELEMENTS (cursor_names);

  name = (const gchar *)g_object_get_data (G_OBJECT (widget), "name");
  if (name != NULL)
    {
      for (i = 0; i < n; i++)
        if (strcmp (name, cursor_names[i]) == 0)
          break;
    }
  else
    i = 0;

  if ((event->type == CDK_BUTTON_PRESS) &&
      ((event->button.button == CDK_BUTTON_PRIMARY) ||
       (event->button.button == CDK_BUTTON_SECONDARY)))
    {
      if (event->button.button == CDK_BUTTON_PRIMARY)
        i = (i + 1) % n;
      else
        i = (i + n - 1) % n;

      ctk_entry_set_text (CTK_ENTRY (entry), cursor_names[i]);

      return TRUE;
    }

  return FALSE;
}

static void
set_cursor_from_name (CtkWidget *entry,
                      CtkWidget *widget)
{
  const gchar *name;
  CdkCursor *cursor;

  name = ctk_entry_get_text (CTK_ENTRY (entry));
  cursor = cdk_cursor_new_from_name (ctk_widget_get_display (widget), name);

  if (cursor == NULL)
    {
      name = NULL;
      cursor = cdk_cursor_new_for_display (ctk_widget_get_display (widget), CDK_BLANK_CURSOR);
    }

  cdk_window_set_cursor (ctk_widget_get_window (widget), cursor);
  g_object_unref (cursor);

  g_object_set_data_full (G_OBJECT (widget), "name", g_strdup (name), g_free);
}

#ifdef CDK_WINDOWING_X11
#include "x11/cdkx.h"
#endif
#ifdef CDK_WINDOWING_WAYLAND
#include "wayland/cdkwayland.h"
#endif

static void
change_cursor_theme (CtkWidget *widget,
		     gpointer   data)
{
#if defined(CDK_WINDOWING_X11) || defined (CDK_WINDOWING_WAYLAND)
  const gchar *theme;
  gint size;
  GList *children;
  CdkDisplay *display;

  children = ctk_container_get_children (CTK_CONTAINER (data));

  theme = ctk_entry_get_text (CTK_ENTRY (children->next->data));
  size = (gint) ctk_spin_button_get_value (CTK_SPIN_BUTTON (children->next->next->data));

  g_list_free (children);

  display = ctk_widget_get_display (widget);
#ifdef CDK_WINDOWING_X11
  if (CDK_IS_X11_DISPLAY (display))
    cdk_x11_display_set_cursor_theme (display, theme, size);
#endif
#ifdef CDK_WINDOWING_WAYLAND
  if (CDK_IS_WAYLAND_DISPLAY (display))
    cdk_wayland_display_set_cursor_theme (display, theme, size);
#endif
#endif
}


static void
create_cursors (CtkWidget *widget)
{
  static CtkWidget *window = NULL;
  CtkWidget *frame;
  CtkWidget *hbox;
  CtkWidget *main_vbox;
  CtkWidget *vbox;
  CtkWidget *darea;
  CtkWidget *button;
  CtkWidget *label;
  CtkWidget *any;
  CtkWidget *entry;
  CtkWidget *size;
  CtkEntryCompletion *completion;
  CtkTreeModel *model;
  gboolean cursor_demo = FALSE;

  if (!window)
    {
      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_screen (CTK_WINDOW (window),
			     ctk_widget_get_screen (widget));

      g_signal_connect (window, "destroy",
			G_CALLBACK (ctk_widget_destroyed),
			&window);

      ctk_window_set_title (CTK_WINDOW (window), "Cursors");

      main_vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 5);
      ctk_container_set_border_width (CTK_CONTAINER (main_vbox), 0);
      ctk_container_add (CTK_CONTAINER (window), main_vbox);

      vbox =
	g_object_new (CTK_TYPE_BOX,
                      "orientation", CTK_ORIENTATION_VERTICAL,
			"CtkBox::homogeneous", FALSE,
			"CtkBox::spacing", 5,
			"CtkContainer::border_width", 10,
			"CtkWidget::parent", main_vbox,
			"CtkWidget::visible", TRUE,
			NULL);

#ifdef CDK_WINDOWING_X11
      if (CDK_IS_X11_DISPLAY (ctk_widget_get_display (vbox)))
        cursor_demo = TRUE;
#endif
#ifdef CDK_WINDOWING_WAYLAND
      if (CDK_IS_WAYLAND_DISPLAY (ctk_widget_get_display (vbox)))
        cursor_demo = TRUE;
#endif

    if (cursor_demo)
        {
          guint w, h;

          hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 6);
          ctk_container_set_border_width (CTK_CONTAINER (hbox), 5);
          ctk_box_pack_start (CTK_BOX (vbox), hbox, FALSE, TRUE, 0);

          label = ctk_label_new ("Cursor Theme:");
          ctk_widget_set_halign (label, CTK_ALIGN_START);
          ctk_widget_set_valign (label, CTK_ALIGN_CENTER);
          ctk_box_pack_start (CTK_BOX (hbox), label, FALSE, TRUE, 0);

          entry = ctk_entry_new ();
          ctk_entry_set_text (CTK_ENTRY (entry), "default");
          ctk_box_pack_start (CTK_BOX (hbox), entry, FALSE, TRUE, 0);

          cdk_display_get_maximal_cursor_size (ctk_widget_get_display (vbox), &w, &h);
          size = ctk_spin_button_new_with_range (1.0, MIN (w, h), 1.0);
          ctk_spin_button_set_value (CTK_SPIN_BUTTON (size), 24.0);
          ctk_box_pack_start (CTK_BOX (hbox), size, TRUE, TRUE, 0);

          g_signal_connect (entry, "changed",
                            G_CALLBACK (change_cursor_theme), hbox);
          g_signal_connect (size, "value-changed",
                            G_CALLBACK (change_cursor_theme), hbox);
        }

      hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 6);
      ctk_container_set_border_width (CTK_CONTAINER (hbox), 5);
      ctk_box_pack_start (CTK_BOX (vbox), hbox, FALSE, TRUE, 0);

      label = ctk_label_new ("Cursor Name:");
      ctk_widget_set_halign (label, CTK_ALIGN_START);
      ctk_widget_set_valign (label, CTK_ALIGN_CENTER);
      ctk_box_pack_start (CTK_BOX (hbox), label, FALSE, TRUE, 0);

      entry = ctk_entry_new ();
      completion = ctk_entry_completion_new ();
      model = cursor_model ();
      ctk_entry_completion_set_model (completion, model);
      ctk_entry_completion_set_text_column (completion, 0);
      ctk_entry_set_completion (CTK_ENTRY (entry), completion);
      g_object_unref (model);
      ctk_box_pack_start (CTK_BOX (hbox), entry, TRUE, TRUE, 0);

      frame =
	g_object_new (ctk_frame_get_type (),
			"CtkFrame::label_xalign", 0.5,
			"CtkFrame::label", "Cursor Area",
			"CtkContainer::border_width", 10,
			"CtkWidget::parent", vbox,
			"CtkWidget::visible", TRUE,
			NULL);

      darea = ctk_drawing_area_new ();
      ctk_widget_set_size_request (darea, 80, 80);
      ctk_container_add (CTK_CONTAINER (frame), darea);
      g_signal_connect (darea,
			"draw",
			G_CALLBACK (cursor_draw),
			NULL);
      ctk_widget_set_events (darea, CDK_EXPOSURE_MASK | CDK_BUTTON_PRESS_MASK);
      g_signal_connect (darea, "button_press_event",
			G_CALLBACK (cursor_event), entry);
      ctk_widget_show (darea);

      g_signal_connect (entry, "changed",
                        G_CALLBACK (set_cursor_from_name), darea);


      any = ctk_separator_new (CTK_ORIENTATION_HORIZONTAL);
      ctk_box_pack_start (CTK_BOX (main_vbox), any, FALSE, TRUE, 0);

      hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
      ctk_container_set_border_width (CTK_CONTAINER (hbox), 10);
      ctk_box_pack_start (CTK_BOX (main_vbox), hbox, FALSE, TRUE, 0);

      button = ctk_button_new_with_label ("Close");
      g_signal_connect_swapped (button, "clicked",
			        G_CALLBACK (ctk_widget_destroy),
				window);
      ctk_box_pack_start (CTK_BOX (hbox), button, TRUE, TRUE, 5);

      ctk_widget_show_all (window);

      ctk_entry_set_text (CTK_ENTRY (entry), "arrow");
    }
  else
    ctk_widget_destroy (window);
}

/*
 * CtkColorSelection
 */

void
create_color_selection (CtkWidget *widget)
{
  static CtkWidget *window = NULL;

  if (!window)
    {
      CtkWidget *picker;
      CtkWidget *hbox;
      CtkWidget *label;
      
      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_screen (CTK_WINDOW (window), 
			     ctk_widget_get_screen (widget));
			     
      g_signal_connect (window, "destroy",
			G_CALLBACK (ctk_widget_destroyed),
                        &window);

      ctk_window_set_title (CTK_WINDOW (window), "CtkColorButton");
      ctk_container_set_border_width (CTK_CONTAINER (window), 0);

      hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 8);
      ctk_container_set_border_width (CTK_CONTAINER (hbox), 8);
      ctk_container_add (CTK_CONTAINER (window), hbox);
      
      label = ctk_label_new ("Pick a color");
      ctk_container_add (CTK_CONTAINER (hbox), label);

      picker = ctk_color_button_new ();
      ctk_color_chooser_set_use_alpha (CTK_COLOR_CHOOSER (picker), TRUE);
      ctk_container_add (CTK_CONTAINER (hbox), picker);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);
}

void
flipping_toggled_cb (CtkWidget *widget, gpointer data)
{
  int state = ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (widget));
  int new_direction = state ? CTK_TEXT_DIR_RTL : CTK_TEXT_DIR_LTR;

  ctk_widget_set_default_direction (new_direction);
}

static void
orientable_toggle_orientation (CtkOrientable *orientable)
{
  CtkOrientation orientation;

  orientation = ctk_orientable_get_orientation (orientable);
  ctk_orientable_set_orientation (orientable,
                                  orientation == CTK_ORIENTATION_HORIZONTAL ?
                                  CTK_ORIENTATION_VERTICAL :
                                  CTK_ORIENTATION_HORIZONTAL);

  if (CTK_IS_CONTAINER (orientable))
    {
      GList *children;
      GList *child;

      children = ctk_container_get_children (CTK_CONTAINER (orientable));

      for (child = children; child; child = child->next)
        {
          if (CTK_IS_ORIENTABLE (child->data))
            orientable_toggle_orientation (child->data);
        }

      g_list_free (children);
    }
}

void
flipping_orientation_toggled_cb (CtkWidget *widget, gpointer data)
{
  CtkWidget *content_area;
  CtkWidget *toplevel;

  toplevel = ctk_widget_get_toplevel (widget);
  content_area = ctk_dialog_get_content_area (CTK_DIALOG (toplevel));
  orientable_toggle_orientation (CTK_ORIENTABLE (content_area));
}

static void
set_direction_recurse (CtkWidget *widget,
		       gpointer   data)
{
  CtkTextDirection *dir = data;
  
  ctk_widget_set_direction (widget, *dir);
  if (CTK_IS_CONTAINER (widget))
    ctk_container_foreach (CTK_CONTAINER (widget),
			   set_direction_recurse,
			   data);
}

static CtkWidget *
create_forward_back (const char       *title,
		     CtkTextDirection  text_dir)
{
  CtkWidget *frame = ctk_frame_new (title);
  CtkWidget *bbox = ctk_button_box_new (CTK_ORIENTATION_HORIZONTAL);
  CtkWidget *back_button = ctk_button_new_with_label ("Back");
  CtkWidget *forward_button = ctk_button_new_with_label ("Forward");

  ctk_container_set_border_width (CTK_CONTAINER (bbox), 5);
  
  ctk_container_add (CTK_CONTAINER (frame), bbox);
  ctk_container_add (CTK_CONTAINER (bbox), back_button);
  ctk_container_add (CTK_CONTAINER (bbox), forward_button);

  set_direction_recurse (frame, &text_dir);

  return frame;
}

void
create_flipping (CtkWidget *widget)
{
  static CtkWidget *window = NULL;
  CtkWidget *check_button;
  CtkWidget *content_area;

  if (!window)
    {
      window = ctk_dialog_new ();

      ctk_window_set_screen (CTK_WINDOW (window),
			     ctk_widget_get_screen (widget));

      g_signal_connect (window, "destroy",
			G_CALLBACK (ctk_widget_destroyed),
			&window);

      content_area = ctk_dialog_get_content_area (CTK_DIALOG (window));

      ctk_window_set_title (CTK_WINDOW (window), "Bidirectional Flipping");

      check_button = ctk_check_button_new_with_label ("Right-to-left global direction");
      ctk_container_set_border_width (CTK_CONTAINER (check_button), 10);
      ctk_box_pack_start (CTK_BOX (content_area), check_button, TRUE, TRUE, 0);

      if (ctk_widget_get_default_direction () == CTK_TEXT_DIR_RTL)
	ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (check_button), TRUE);

      g_signal_connect (check_button, "toggled",
			G_CALLBACK (flipping_toggled_cb), NULL);

      check_button = ctk_check_button_new_with_label ("Toggle orientation of all boxes");
      ctk_container_set_border_width (CTK_CONTAINER (check_button), 10);
      ctk_box_pack_start (CTK_BOX (content_area), check_button, TRUE, TRUE, 0);

      g_signal_connect (check_button, "toggled",
			G_CALLBACK (flipping_orientation_toggled_cb), NULL);

      ctk_box_pack_start (CTK_BOX (content_area),
			  create_forward_back ("Default", CTK_TEXT_DIR_NONE),
			  TRUE, TRUE, 0);

      ctk_box_pack_start (CTK_BOX (content_area),
			  create_forward_back ("Left-to-Right", CTK_TEXT_DIR_LTR),
			  TRUE, TRUE, 0);

      ctk_box_pack_start (CTK_BOX (content_area),
			  create_forward_back ("Right-to-Left", CTK_TEXT_DIR_RTL),
			  TRUE, TRUE, 0);

      ctk_dialog_add_button (CTK_DIALOG (window), "Close", CTK_RESPONSE_CLOSE);
      g_signal_connect (window, "response", G_CALLBACK (ctk_widget_destroy), NULL);
    }
  
  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);
}

/*
 * Focus test
 */

static CtkWidget*
make_focus_table (GList **list)
{
  CtkWidget *grid;
  gint i, j;
  
  grid = ctk_grid_new ();

  ctk_grid_set_row_spacing (CTK_GRID (grid), 10);
  ctk_grid_set_column_spacing (CTK_GRID (grid), 10);

  for (i = 0; i < 5; i++)
    {
      for (j = 0; j < 5; j++)
        {
          CtkWidget *widget;
          
          if ((i + j) % 2)
            widget = ctk_entry_new ();
          else
            widget = ctk_button_new_with_label ("Foo");

          *list = g_list_prepend (*list, widget);
          
          ctk_widget_set_hexpand (widget, TRUE);
          ctk_widget_set_vexpand (widget, TRUE);

          ctk_grid_attach (CTK_GRID (grid), widget, i, j, 1, 1);
        }
    }

  *list = g_list_reverse (*list);
  
  return grid;
}

static void
create_focus (CtkWidget *widget)
{
  static CtkWidget *window = NULL;
  
  if (!window)
    {
      CtkWidget *content_area;
      CtkWidget *table;
      CtkWidget *frame;
      GList *list = NULL;
      
      window = ctk_dialog_new_with_buttons ("Keyboard focus navigation",
                                            NULL, 0,
                                            "_Close",
                                            CTK_RESPONSE_NONE,
                                            NULL);

      ctk_window_set_screen (CTK_WINDOW (window),
			     ctk_widget_get_screen (widget));

      g_signal_connect (window, "destroy",
			G_CALLBACK (ctk_widget_destroyed),
			&window);

      g_signal_connect (window, "response",
                        G_CALLBACK (ctk_widget_destroy),
                        NULL);

      content_area = ctk_dialog_get_content_area (CTK_DIALOG (window));
      
      ctk_window_set_title (CTK_WINDOW (window), "Keyboard Focus Navigation");

      frame = ctk_frame_new ("Weird tab focus chain");

      ctk_box_pack_start (CTK_BOX (content_area), frame, TRUE, TRUE, 0);
      
      table = make_focus_table (&list);

      ctk_container_add (CTK_CONTAINER (frame), table);

      ctk_container_set_focus_chain (CTK_CONTAINER (table),
                                     list);

      g_list_free (list);
      
      frame = ctk_frame_new ("Default tab focus chain");

      ctk_box_pack_start (CTK_BOX (content_area), frame, TRUE, TRUE, 0);

      list = NULL;
      table = make_focus_table (&list);

      g_list_free (list);
      
      ctk_container_add (CTK_CONTAINER (frame), table);      
    }
  
  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);
}

/*
 * CtkFontSelection
 */

void
create_font_selection (CtkWidget *widget)
{
  static CtkWidget *window = NULL;

  if (!window)
    {
      CtkWidget *picker;
      CtkWidget *hbox;
      CtkWidget *label;
      
      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_screen (CTK_WINDOW (window),
			     ctk_widget_get_screen (widget));

      g_signal_connect (window, "destroy",
			G_CALLBACK (ctk_widget_destroyed),
			&window);

      ctk_window_set_title (CTK_WINDOW (window), "CtkFontButton");
      ctk_container_set_border_width (CTK_CONTAINER (window), 0);

      hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 8);
      ctk_container_set_border_width (CTK_CONTAINER (hbox), 8);
      ctk_container_add (CTK_CONTAINER (window), hbox);
      
      label = ctk_label_new ("Pick a font");
      ctk_container_add (CTK_CONTAINER (hbox), label);

      picker = ctk_font_button_new ();
      ctk_font_button_set_use_font (CTK_FONT_BUTTON (picker), TRUE);
      ctk_container_add (CTK_CONTAINER (hbox), picker);
    }
  
  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);
}

/*
 * CtkDialog
 */

static CtkWidget *dialog_window = NULL;

static void
dialog_response_cb (CtkWidget *widget, gint response, gpointer unused)
{
  CtkWidget *content_area;
  GList *l, *children;

  if (response == CTK_RESPONSE_APPLY)
    {
      content_area = ctk_dialog_get_content_area (CTK_DIALOG (dialog_window));
      children = ctk_container_get_children (CTK_CONTAINER (content_area));

      for (l = children; l; l = l->next)
        {
          if (CTK_IS_LABEL (l->data))
            {
              ctk_container_remove (CTK_CONTAINER (content_area), l->data);
              break;
            }
        }

      /* no label removed, so add one */
      if (l == NULL)
        {
          CtkWidget *label;
          
          label = ctk_label_new ("Dialog Test");
          g_object_set (label, "margin", 10, NULL);
          ctk_box_pack_start (CTK_BOX (content_area),
                              label, TRUE, TRUE, 0);
          ctk_widget_show (label);
        }
      
      g_list_free (children);
    }
}

static void
create_dialog (CtkWidget *widget)
{
  if (!dialog_window)
    {
      /* This is a terrible example; it's much simpler to create
       * dialogs than this. Don't use testctk for example code,
       * use ctk-demo ;-)
       */
      
      dialog_window = ctk_dialog_new ();
      ctk_window_set_screen (CTK_WINDOW (dialog_window),
			     ctk_widget_get_screen (widget));

      g_signal_connect (dialog_window, "destroy",
			G_CALLBACK (ctk_widget_destroyed),
			&dialog_window);


      ctk_window_set_title (CTK_WINDOW (dialog_window), "CtkDialog");
      ctk_container_set_border_width (CTK_CONTAINER (dialog_window), 0);

      ctk_dialog_add_button (CTK_DIALOG (dialog_window),
                             "OK",
                             CTK_RESPONSE_OK);

      ctk_dialog_add_button (CTK_DIALOG (dialog_window),
                             "Toggle",
                             CTK_RESPONSE_APPLY);
      
      g_signal_connect (dialog_window, "response",
			G_CALLBACK (dialog_response_cb),
			NULL);
    }

  if (!ctk_widget_get_visible (dialog_window))
    ctk_widget_show (dialog_window);
  else
    ctk_widget_destroy (dialog_window);
}

/* Display & Screen test 
 */

typedef struct
{
  CtkWidget *combo;
  CtkWidget *entry;
  CtkWidget *toplevel;
  CtkWidget *dialog_window;
} ScreenDisplaySelection;

static void
screen_display_check (CtkWidget *widget, ScreenDisplaySelection *data)
{
  const gchar *display_name;
  CdkDisplay *display;
  CtkWidget *dialog;
  CdkScreen *new_screen = NULL;
  CdkScreen *current_screen = ctk_widget_get_screen (widget);
  
  display_name = ctk_entry_get_text (CTK_ENTRY (data->entry));
  display = cdk_display_open (display_name);
      
  if (!display)
    {
      dialog = ctk_message_dialog_new (CTK_WINDOW (ctk_widget_get_toplevel (widget)),
                                       CTK_DIALOG_DESTROY_WITH_PARENT,
                                       CTK_MESSAGE_ERROR,
                                       CTK_BUTTONS_OK,
                                       "The display :\n%s\ncannot be opened",
                                       display_name);
      ctk_window_set_screen (CTK_WINDOW (dialog), current_screen);
      ctk_widget_show (dialog);
      g_signal_connect (dialog, "response",
                        G_CALLBACK (ctk_widget_destroy),
                        NULL);
    }
  else
    {
      CtkTreeModel *model = ctk_combo_box_get_model (CTK_COMBO_BOX (data->combo));
      gint i = 0;
      CtkTreeIter iter;
      gboolean found = FALSE;
      while (ctk_tree_model_iter_nth_child (model, &iter, NULL, i++))
        {
          gchar *name;
          ctk_tree_model_get (model, &iter, 0, &name, -1);
          found = !g_ascii_strcasecmp (display_name, name);
          g_free (name);

          if (found)
            break;
        }
      if (!found)
        ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (data->combo), display_name);
      new_screen = cdk_display_get_default_screen (display);

      ctk_window_set_screen (CTK_WINDOW (data->toplevel), new_screen);
      ctk_widget_destroy (data->dialog_window);
    }
}

void
screen_display_destroy_diag (CtkWidget *widget, CtkWidget *data)
{
  ctk_widget_destroy (data);
}

void
create_display_screen (CtkWidget *widget)
{
  CtkWidget *grid, *frame, *window, *combo_dpy, *vbox;
  CtkWidget *label_dpy, *applyb, *cancelb;
  CtkWidget *bbox;
  ScreenDisplaySelection *scr_dpy_data;
  CdkScreen *screen = ctk_widget_get_screen (widget);

  window = g_object_new (ctk_window_get_type (),
			 "screen", screen,
			 "type", CTK_WINDOW_TOPLEVEL,
			 "title", "Screen or Display selection",
			 "border_width",
                         10, NULL);
  g_signal_connect (window, "destroy", 
		    G_CALLBACK (ctk_widget_destroy), NULL);

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 3);
  ctk_container_add (CTK_CONTAINER (window), vbox);
  
  frame = ctk_frame_new ("Select display");
  ctk_container_add (CTK_CONTAINER (vbox), frame);
  
  grid = ctk_grid_new ();
  ctk_grid_set_row_spacing (CTK_GRID (grid), 3);
  ctk_grid_set_column_spacing (CTK_GRID (grid), 3);

  ctk_container_add (CTK_CONTAINER (frame), grid);

  label_dpy = ctk_label_new ("move to another X display");
  combo_dpy = ctk_combo_box_text_new_with_entry ();
  ctk_widget_set_hexpand (combo_dpy, TRUE);
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo_dpy), "diabolo:0.0");
  ctk_entry_set_text (CTK_ENTRY (ctk_bin_get_child (CTK_BIN (combo_dpy))),
                      "<hostname>:<X Server Num>.<Screen Num>");

  ctk_grid_attach (CTK_GRID (grid), label_dpy, 0, 0, 1, 1);
  ctk_grid_attach (CTK_GRID (grid), combo_dpy, 0, 1, 1, 1);

  bbox = ctk_button_box_new (CTK_ORIENTATION_HORIZONTAL);
  applyb = ctk_button_new_with_label ("_Apply");
  cancelb = ctk_button_new_with_label ("_Cancel");
  
  ctk_container_add (CTK_CONTAINER (vbox), bbox);

  ctk_container_add (CTK_CONTAINER (bbox), applyb);
  ctk_container_add (CTK_CONTAINER (bbox), cancelb);

  scr_dpy_data = g_new0 (ScreenDisplaySelection, 1);

  scr_dpy_data->entry = ctk_bin_get_child (CTK_BIN (combo_dpy));
  scr_dpy_data->toplevel = ctk_widget_get_toplevel (widget);
  scr_dpy_data->dialog_window = window;

  g_signal_connect (cancelb, "clicked", 
		    G_CALLBACK (screen_display_destroy_diag), window);
  g_signal_connect (applyb, "clicked", 
		    G_CALLBACK (screen_display_check), scr_dpy_data);
  ctk_widget_show_all (window);
}

/* Event Watcher
 */
static gulong event_watcher_enter_id = 0;
static gulong event_watcher_leave_id = 0;

static gboolean
event_watcher (GSignalInvocationHint *ihint,
	       guint                  n_param_values,
	       const GValue          *param_values,
	       gpointer               data)
{
  g_print ("Watch: \"%s\" emitted for %s\n",
	   g_signal_name (ihint->signal_id),
	   G_OBJECT_TYPE_NAME (g_value_get_object (param_values + 0)));

  return TRUE;
}

static void
event_watcher_down (void)
{
  if (event_watcher_enter_id)
    {
      guint signal_id;

      signal_id = g_signal_lookup ("enter_notify_event", CTK_TYPE_WIDGET);
      g_signal_remove_emission_hook (signal_id, event_watcher_enter_id);
      event_watcher_enter_id = 0;
      signal_id = g_signal_lookup ("leave_notify_event", CTK_TYPE_WIDGET);
      g_signal_remove_emission_hook (signal_id, event_watcher_leave_id);
      event_watcher_leave_id = 0;
    }
}

static void
event_watcher_toggle (void)
{
  if (event_watcher_enter_id)
    event_watcher_down ();
  else
    {
      guint signal_id;

      signal_id = g_signal_lookup ("enter_notify_event", CTK_TYPE_WIDGET);
      event_watcher_enter_id = g_signal_add_emission_hook (signal_id, 0, event_watcher, NULL, NULL);
      signal_id = g_signal_lookup ("leave_notify_event", CTK_TYPE_WIDGET);
      event_watcher_leave_id = g_signal_add_emission_hook (signal_id, 0, event_watcher, NULL, NULL);
    }
}

static void
create_event_watcher (CtkWidget *widget)
{
  CtkWidget *content_area;
  CtkWidget *button;

  if (!dialog_window)
    {
      dialog_window = ctk_dialog_new ();
      ctk_window_set_screen (CTK_WINDOW (dialog_window),
			     ctk_widget_get_screen (widget));

      g_signal_connect (dialog_window, "destroy",
			G_CALLBACK (ctk_widget_destroyed),
			&dialog_window);
      g_signal_connect (dialog_window, "destroy",
			G_CALLBACK (event_watcher_down),
			NULL);

      content_area = ctk_dialog_get_content_area (CTK_DIALOG (dialog_window));

      ctk_window_set_title (CTK_WINDOW (dialog_window), "Event Watcher");
      ctk_container_set_border_width (CTK_CONTAINER (dialog_window), 0);
      ctk_widget_set_size_request (dialog_window, 200, 110);

      button = ctk_toggle_button_new_with_label ("Activate Watch");
      g_signal_connect (button, "clicked",
			G_CALLBACK (event_watcher_toggle),
			NULL);
      ctk_container_set_border_width (CTK_CONTAINER (button), 10);
      ctk_box_pack_start (CTK_BOX (content_area), button, TRUE, TRUE, 0);
      ctk_widget_show (button);

      ctk_dialog_add_button (CTK_DIALOG (dialog_window), "Close", CTK_RESPONSE_CLOSE);
      g_signal_connect (dialog_window, "response", G_CALLBACK (ctk_widget_destroy), NULL);
    }

  if (!ctk_widget_get_visible (dialog_window))
    ctk_widget_show (dialog_window);
  else
    ctk_widget_destroy (dialog_window);
}

/*
 * CtkRange
 */

static gchar*
reformat_value (CtkScale *scale,
                gdouble   value)
{
  return g_strdup_printf ("-->%0.*g<--",
                          ctk_scale_get_digits (scale), value);
}

static void
create_range_controls (CtkWidget *widget)
{
  static CtkWidget *window = NULL;
  CtkWidget *box1;
  CtkWidget *box2;
  CtkWidget *button;
  CtkWidget *scrollbar;
  CtkWidget *scale;
  CtkWidget *separator;
  CtkAdjustment *adjustment;
  CtkWidget *hbox;

  if (!window)
    {
      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);

      ctk_window_set_screen (CTK_WINDOW (window),
			     ctk_widget_get_screen (widget));

      g_signal_connect (window, "destroy",
			G_CALLBACK (ctk_widget_destroyed),
			&window);

      ctk_window_set_title (CTK_WINDOW (window), "range controls");
      ctk_container_set_border_width (CTK_CONTAINER (window), 0);


      box1 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
      ctk_container_add (CTK_CONTAINER (window), box1);
      ctk_widget_show (box1);


      box2 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 10);
      ctk_container_set_border_width (CTK_CONTAINER (box2), 10);
      ctk_box_pack_start (CTK_BOX (box1), box2, TRUE, TRUE, 0);
      ctk_widget_show (box2);


      adjustment = ctk_adjustment_new (0.0, 0.0, 101.0, 0.1, 1.0, 1.0);

      scale = ctk_scale_new (CTK_ORIENTATION_HORIZONTAL, CTK_ADJUSTMENT (adjustment));
      ctk_widget_set_size_request (CTK_WIDGET (scale), 150, -1);
      ctk_scale_set_digits (CTK_SCALE (scale), 1);
      ctk_scale_set_draw_value (CTK_SCALE (scale), TRUE);
      ctk_box_pack_start (CTK_BOX (box2), scale, TRUE, TRUE, 0);
      ctk_widget_show (scale);

      scrollbar = ctk_scrollbar_new (CTK_ORIENTATION_HORIZONTAL, CTK_ADJUSTMENT (adjustment));
      ctk_box_pack_start (CTK_BOX (box2), scrollbar, TRUE, TRUE, 0);
      ctk_widget_show (scrollbar);

      scale = ctk_scale_new (CTK_ORIENTATION_HORIZONTAL, CTK_ADJUSTMENT (adjustment));
      ctk_scale_set_draw_value (CTK_SCALE (scale), TRUE);
      g_signal_connect (scale,
                        "format_value",
                        G_CALLBACK (reformat_value),
                        NULL);
      ctk_box_pack_start (CTK_BOX (box2), scale, TRUE, TRUE, 0);
      ctk_widget_show (scale);
      
      hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);

      scale = ctk_scale_new (CTK_ORIENTATION_VERTICAL, CTK_ADJUSTMENT (adjustment));
      ctk_widget_set_size_request (scale, -1, 200);
      ctk_scale_set_digits (CTK_SCALE (scale), 2);
      ctk_scale_set_draw_value (CTK_SCALE (scale), TRUE);
      ctk_box_pack_start (CTK_BOX (hbox), scale, TRUE, TRUE, 0);
      ctk_widget_show (scale);

      scale = ctk_scale_new (CTK_ORIENTATION_VERTICAL, CTK_ADJUSTMENT (adjustment));
      ctk_widget_set_size_request (scale, -1, 200);
      ctk_scale_set_digits (CTK_SCALE (scale), 2);
      ctk_scale_set_draw_value (CTK_SCALE (scale), TRUE);
      ctk_range_set_inverted (CTK_RANGE (scale), TRUE);
      ctk_box_pack_start (CTK_BOX (hbox), scale, TRUE, TRUE, 0);
      ctk_widget_show (scale);

      scale = ctk_scale_new (CTK_ORIENTATION_VERTICAL, CTK_ADJUSTMENT (adjustment));
      ctk_scale_set_draw_value (CTK_SCALE (scale), TRUE);
      g_signal_connect (scale,
                        "format_value",
                        G_CALLBACK (reformat_value),
                        NULL);
      ctk_box_pack_start (CTK_BOX (hbox), scale, TRUE, TRUE, 0);
      ctk_widget_show (scale);

      
      ctk_box_pack_start (CTK_BOX (box2), hbox, TRUE, TRUE, 0);
      ctk_widget_show (hbox);
      
      separator = ctk_separator_new (CTK_ORIENTATION_HORIZONTAL);
      ctk_box_pack_start (CTK_BOX (box1), separator, FALSE, TRUE, 0);
      ctk_widget_show (separator);


      box2 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 10);
      ctk_container_set_border_width (CTK_CONTAINER (box2), 10);
      ctk_box_pack_start (CTK_BOX (box1), box2, FALSE, TRUE, 0);
      ctk_widget_show (box2);


      button = ctk_button_new_with_label ("close");
      g_signal_connect_swapped (button, "clicked",
			        G_CALLBACK (ctk_widget_destroy),
				window);
      ctk_box_pack_start (CTK_BOX (box2), button, TRUE, TRUE, 0);
      ctk_widget_set_can_default (button, TRUE);
      ctk_widget_grab_default (button);
      ctk_widget_show (button);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show (window);
  else
    ctk_widget_destroy (window);
}

struct {
  CdkColor color;
  gchar *name;
} text_colors[] = {
 { { 0, 0x0000, 0x0000, 0x0000 }, "black" },
 { { 0, 0xFFFF, 0xFFFF, 0xFFFF }, "white" },
 { { 0, 0xFFFF, 0x0000, 0x0000 }, "red" },
 { { 0, 0x0000, 0xFFFF, 0x0000 }, "green" },
 { { 0, 0x0000, 0x0000, 0xFFFF }, "blue" }, 
 { { 0, 0x0000, 0xFFFF, 0xFFFF }, "cyan" },
 { { 0, 0xFFFF, 0x0000, 0xFFFF }, "magenta" },
 { { 0, 0xFFFF, 0xFFFF, 0x0000 }, "yellow" }
};

int ntext_colors = sizeof(text_colors) / sizeof(text_colors[0]);

/*
 * CtkNotebook
 */

static const char * book_open_xpm[] = {
"16 16 4 1",
"       c None s None",
".      c black",
"X      c #808080",
"o      c white",
"                ",
"  ..            ",
" .Xo.    ...    ",
" .Xoo. ..oo.    ",
" .Xooo.Xooo...  ",
" .Xooo.oooo.X.  ",
" .Xooo.Xooo.X.  ",
" .Xooo.oooo.X.  ",
" .Xooo.Xooo.X.  ",
" .Xooo.oooo.X.  ",
"  .Xoo.Xoo..X.  ",
"   .Xo.o..ooX.  ",
"    .X..XXXXX.  ",
"    ..X.......  ",
"     ..         ",
"                "};

static const char * book_closed_xpm[] = {
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
"                "};

GdkPixbuf *book_open;
GdkPixbuf *book_closed;
CtkWidget *sample_notebook;

static void
set_page_image (CtkNotebook *notebook, gint page_num, GdkPixbuf *pixbuf)
{
  CtkWidget *page_widget;
  CtkWidget *pixwid;

  page_widget = ctk_notebook_get_nth_page (notebook, page_num);

  pixwid = g_object_get_data (G_OBJECT (page_widget), "tab_pixmap");
  ctk_image_set_from_pixbuf (CTK_IMAGE (pixwid), pixbuf);
  
  pixwid = g_object_get_data (G_OBJECT (page_widget), "menu_pixmap");
  ctk_image_set_from_pixbuf (CTK_IMAGE (pixwid), pixbuf);
}

static void
page_switch (CtkWidget *widget, gpointer *page, gint page_num)
{
  CtkNotebook *notebook = CTK_NOTEBOOK (widget);
  gint old_page_num = ctk_notebook_get_current_page (notebook);
 
  if (page_num == old_page_num)
    return;

  set_page_image (notebook, page_num, book_open);

  if (old_page_num != -1)
    set_page_image (notebook, old_page_num, book_closed);
}

static void
tab_fill (CtkToggleButton *button, CtkWidget *child)
{
  ctk_container_child_set (CTK_CONTAINER (sample_notebook), child,
                           "tab-fill", ctk_toggle_button_get_active (button),
                           NULL);
}

static void
tab_expand (CtkToggleButton *button, CtkWidget *child)
{
  ctk_container_child_set (CTK_CONTAINER (sample_notebook), child,
                           "tab-expand", ctk_toggle_button_get_active (button),
                           NULL);
}

static void
create_pages (CtkNotebook *notebook, gint start, gint end)
{
  CtkWidget *child = NULL;
  CtkWidget *button;
  CtkWidget *label;
  CtkWidget *hbox;
  CtkWidget *vbox;
  CtkWidget *label_box;
  CtkWidget *menu_box;
  CtkWidget *pixwid;
  gint i;
  char buffer[32];
  char accel_buffer[32];

  for (i = start; i <= end; i++)
    {
      sprintf (buffer, "Page %d", i);
      sprintf (accel_buffer, "Page _%d", i);

      child = ctk_frame_new (buffer);
      ctk_container_set_border_width (CTK_CONTAINER (child), 10);

      vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
      ctk_box_set_homogeneous (CTK_BOX (vbox), TRUE);
      ctk_container_set_border_width (CTK_CONTAINER (vbox), 10);
      ctk_container_add (CTK_CONTAINER (child), vbox);

      hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
      ctk_box_set_homogeneous (CTK_BOX (hbox), TRUE);
      ctk_box_pack_start (CTK_BOX (vbox), hbox, FALSE, TRUE, 5);

      button = ctk_check_button_new_with_label ("Fill Tab");
      ctk_box_pack_start (CTK_BOX (hbox), button, TRUE, TRUE, 5);
      ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (button), TRUE);
      g_signal_connect (button, "toggled",
			G_CALLBACK (tab_fill), child);

      button = ctk_check_button_new_with_label ("Expand Tab");
      ctk_box_pack_start (CTK_BOX (hbox), button, TRUE, TRUE, 5);
      g_signal_connect (button, "toggled",
			G_CALLBACK (tab_expand), child);

      button = ctk_button_new_with_label ("Hide Page");
      ctk_box_pack_end (CTK_BOX (vbox), button, FALSE, FALSE, 5);
      g_signal_connect_swapped (button, "clicked",
				G_CALLBACK (ctk_widget_hide),
				child);

      ctk_widget_show_all (child);

      label_box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
      pixwid = ctk_image_new_from_pixbuf (book_closed);
      g_object_set_data (G_OBJECT (child), "tab_pixmap", pixwid);
			   
      ctk_box_pack_start (CTK_BOX (label_box), pixwid, FALSE, TRUE, 0);
      ctk_widget_set_margin_start (pixwid, 3);
      ctk_widget_set_margin_end (pixwid, 3);
      ctk_widget_set_margin_bottom (pixwid, 1);
      ctk_widget_set_margin_top (pixwid, 1);
      label = ctk_label_new_with_mnemonic (accel_buffer);
      ctk_box_pack_start (CTK_BOX (label_box), label, FALSE, TRUE, 0);
      ctk_widget_show_all (label_box);
      
				       
      menu_box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
      pixwid = ctk_image_new_from_pixbuf (book_closed);
      g_object_set_data (G_OBJECT (child), "menu_pixmap", pixwid);
      
      ctk_box_pack_start (CTK_BOX (menu_box), pixwid, FALSE, TRUE, 0);
      ctk_widget_set_margin_start (pixwid, 3);
      ctk_widget_set_margin_end (pixwid, 3);
      ctk_widget_set_margin_bottom (pixwid, 1);
      ctk_widget_set_margin_top (pixwid, 1);
      label = ctk_label_new (buffer);
      ctk_box_pack_start (CTK_BOX (menu_box), label, FALSE, TRUE, 0);
      ctk_widget_show_all (menu_box);

      ctk_notebook_append_page_menu (notebook, child, label_box, menu_box);
    }
}

static void
rotate_notebook (CtkButton   *button,
		 CtkNotebook *notebook)
{
  ctk_notebook_set_tab_pos (notebook, (ctk_notebook_get_tab_pos (notebook) + 1) % 4);
}

static void
show_all_pages (CtkButton   *button,
		CtkNotebook *notebook)
{  
  ctk_container_foreach (CTK_CONTAINER (notebook),
			 (CtkCallback) ctk_widget_show, NULL);
}

static void
notebook_type_changed (CtkWidget *optionmenu,
		       gpointer   data)
{
  CtkNotebook *notebook;
  gint i, c;

  enum {
    STANDARD,
    NOTABS,
    BORDERLESS,
    SCROLLABLE
  };

  notebook = CTK_NOTEBOOK (data);

  c = ctk_combo_box_get_active (CTK_COMBO_BOX (optionmenu));

  switch (c)
    {
    case STANDARD:
      /* standard notebook */
      ctk_notebook_set_show_tabs (notebook, TRUE);
      ctk_notebook_set_show_border (notebook, TRUE);
      ctk_notebook_set_scrollable (notebook, FALSE);
      break;

    case NOTABS:
      /* notabs notebook */
      ctk_notebook_set_show_tabs (notebook, FALSE);
      ctk_notebook_set_show_border (notebook, TRUE);
      break;

    case BORDERLESS:
      /* borderless */
      ctk_notebook_set_show_tabs (notebook, FALSE);
      ctk_notebook_set_show_border (notebook, FALSE);
      break;

    case SCROLLABLE:  
      /* scrollable */
      ctk_notebook_set_show_tabs (notebook, TRUE);
      ctk_notebook_set_show_border (notebook, TRUE);
      ctk_notebook_set_scrollable (notebook, TRUE);
      if (ctk_notebook_get_n_pages (notebook) == 5)
	create_pages (notebook, 6, 15);

      return;
      break;
    }

  if (ctk_notebook_get_n_pages (notebook) == 15)
    for (i = 0; i < 10; i++)
      ctk_notebook_remove_page (notebook, 5);
}

static void
notebook_popup (CtkToggleButton *button,
		CtkNotebook     *notebook)
{
  if (ctk_toggle_button_get_active (button))
    ctk_notebook_popup_enable (notebook);
  else
    ctk_notebook_popup_disable (notebook);
}

static void
create_notebook (CtkWidget *widget)
{
  static CtkWidget *window = NULL;
  CtkWidget *box1;
  CtkWidget *box2;
  CtkWidget *button;
  CtkWidget *separator;
  CtkWidget *omenu;
  CtkWidget *label;

  static gchar *items[] =
  {
    "Standard",
    "No tabs",
    "Borderless",
    "Scrollable"
  };
  
  if (!window)
    {
      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_screen (CTK_WINDOW (window),
			     ctk_widget_get_screen (widget));

      g_signal_connect (window, "destroy",
			G_CALLBACK (ctk_widget_destroyed),
			&window);

      ctk_window_set_title (CTK_WINDOW (window), "notebook");
      ctk_container_set_border_width (CTK_CONTAINER (window), 0);

      box1 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
      ctk_container_add (CTK_CONTAINER (window), box1);

      sample_notebook = ctk_notebook_new ();
      g_signal_connect (sample_notebook, "switch_page",
			G_CALLBACK (page_switch), NULL);
      ctk_notebook_set_tab_pos (CTK_NOTEBOOK (sample_notebook), CTK_POS_TOP);
      ctk_box_pack_start (CTK_BOX (box1), sample_notebook, TRUE, TRUE, 0);
      ctk_container_set_border_width (CTK_CONTAINER (sample_notebook), 10);

      ctk_widget_realize (sample_notebook);

      if (!book_open)
	book_open = gdk_pixbuf_new_from_xpm_data (book_open_xpm);
						  
      if (!book_closed)
	book_closed = gdk_pixbuf_new_from_xpm_data (book_closed_xpm);

      create_pages (CTK_NOTEBOOK (sample_notebook), 1, 5);

      separator = ctk_separator_new (CTK_ORIENTATION_HORIZONTAL);
      ctk_box_pack_start (CTK_BOX (box1), separator, FALSE, TRUE, 10);
      
      box2 = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 5);
      ctk_container_set_border_width (CTK_CONTAINER (box2), 10);
      ctk_box_pack_start (CTK_BOX (box1), box2, FALSE, TRUE, 0);

      button = ctk_check_button_new_with_label ("popup menu");
      ctk_box_pack_start (CTK_BOX (box2), button, TRUE, FALSE, 0);
      g_signal_connect (button, "clicked",
			G_CALLBACK (notebook_popup),
			sample_notebook);

      box2 = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 5);
      ctk_container_set_border_width (CTK_CONTAINER (box2), 10);
      ctk_box_pack_start (CTK_BOX (box1), box2, FALSE, TRUE, 0);

      label = ctk_label_new ("Notebook Style :");
      ctk_box_pack_start (CTK_BOX (box2), label, FALSE, TRUE, 0);

      omenu = build_option_menu (items, G_N_ELEMENTS (items), 0,
				 notebook_type_changed,
				 sample_notebook);
      ctk_box_pack_start (CTK_BOX (box2), omenu, FALSE, TRUE, 0);

      button = ctk_button_new_with_label ("Show all Pages");
      ctk_box_pack_start (CTK_BOX (box2), button, FALSE, TRUE, 0);
      g_signal_connect (button, "clicked",
			G_CALLBACK (show_all_pages), sample_notebook);

      box2 = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 10);
      ctk_box_set_homogeneous (CTK_BOX (box2), TRUE);
      ctk_container_set_border_width (CTK_CONTAINER (box2), 10);
      ctk_box_pack_start (CTK_BOX (box1), box2, FALSE, TRUE, 0);

      button = ctk_button_new_with_label ("prev");
      g_signal_connect_swapped (button, "clicked",
			        G_CALLBACK (ctk_notebook_prev_page),
				sample_notebook);
      ctk_box_pack_start (CTK_BOX (box2), button, TRUE, TRUE, 0);

      button = ctk_button_new_with_label ("next");
      g_signal_connect_swapped (button, "clicked",
			        G_CALLBACK (ctk_notebook_next_page),
				sample_notebook);
      ctk_box_pack_start (CTK_BOX (box2), button, TRUE, TRUE, 0);

      button = ctk_button_new_with_label ("rotate");
      g_signal_connect (button, "clicked",
			G_CALLBACK (rotate_notebook), sample_notebook);
      ctk_box_pack_start (CTK_BOX (box2), button, TRUE, TRUE, 0);

      separator = ctk_separator_new (CTK_ORIENTATION_HORIZONTAL);
      ctk_box_pack_start (CTK_BOX (box1), separator, FALSE, TRUE, 5);

      button = ctk_button_new_with_label ("close");
      ctk_container_set_border_width (CTK_CONTAINER (button), 5);
      g_signal_connect_swapped (button, "clicked",
			        G_CALLBACK (ctk_widget_destroy),
				window);
      ctk_box_pack_start (CTK_BOX (box1), button, FALSE, FALSE, 0);
      ctk_widget_set_can_default (button, TRUE);
      ctk_widget_grab_default (button);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);
}

/*
 * CtkPanes
 */

void
toggle_resize (CtkWidget *widget, CtkWidget *child)
{
  CtkContainer *container = CTK_CONTAINER (ctk_widget_get_parent (child));
  GValue value = G_VALUE_INIT;
  g_value_init (&value, G_TYPE_BOOLEAN);
  ctk_container_child_get_property (container, child, "resize", &value);
  g_value_set_boolean (&value, !g_value_get_boolean (&value));
  ctk_container_child_set_property (container, child, "resize", &value);
}

void
toggle_shrink (CtkWidget *widget, CtkWidget *child)
{
  CtkContainer *container = CTK_CONTAINER (ctk_widget_get_parent (child));
  GValue value = G_VALUE_INIT;
  g_value_init (&value, G_TYPE_BOOLEAN);
  ctk_container_child_get_property (container, child, "shrink", &value);
  g_value_set_boolean (&value, !g_value_get_boolean (&value));
  ctk_container_child_set_property (container, child, "shrink", &value);
}

CtkWidget *
create_pane_options (CtkPaned    *paned,
		     const gchar *frame_label,
		     const gchar *label1,
		     const gchar *label2)
{
  CtkWidget *child1, *child2;
  CtkWidget *frame;
  CtkWidget *grid;
  CtkWidget *label;
  CtkWidget *check_button;

  child1 = ctk_paned_get_child1 (paned);
  child2 = ctk_paned_get_child2 (paned);

  frame = ctk_frame_new (frame_label);
  ctk_container_set_border_width (CTK_CONTAINER (frame), 4);
  
  grid = ctk_grid_new ();
  ctk_container_add (CTK_CONTAINER (frame), grid);
  
  label = ctk_label_new (label1);
  ctk_grid_attach (CTK_GRID (grid), label, 0, 0, 1, 1);
  
  check_button = ctk_check_button_new_with_label ("Resize");
  ctk_grid_attach (CTK_GRID (grid), check_button, 0, 1, 1, 1);
  g_signal_connect (check_button, "toggled",
		    G_CALLBACK (toggle_resize),
                    child1);

  check_button = ctk_check_button_new_with_label ("Shrink");
  ctk_grid_attach (CTK_GRID (grid), check_button, 0, 2, 1, 1);
  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (check_button),
			       TRUE);
  g_signal_connect (check_button, "toggled",
		    G_CALLBACK (toggle_shrink),
                    child1);

  label = ctk_label_new (label2);
  ctk_grid_attach (CTK_GRID (grid), label, 1, 0, 1, 1);
  
  check_button = ctk_check_button_new_with_label ("Resize");
  ctk_grid_attach (CTK_GRID (grid), check_button, 1, 1, 1, 1);
  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (check_button),
			       TRUE);
  g_signal_connect (check_button, "toggled",
		    G_CALLBACK (toggle_resize),
                    child2);

  check_button = ctk_check_button_new_with_label ("Shrink");
  ctk_grid_attach (CTK_GRID (grid), check_button, 1, 2, 1, 1);
  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (check_button),
			       TRUE);
  g_signal_connect (check_button, "toggled",
		    G_CALLBACK (toggle_shrink),
                    child2);

  return frame;
}

void
create_panes (CtkWidget *widget)
{
  static CtkWidget *window = NULL;
  CtkWidget *frame;
  CtkWidget *hpaned;
  CtkWidget *vpaned;
  CtkWidget *button;
  CtkWidget *vbox;

  if (!window)
    {
      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);

      ctk_window_set_screen (CTK_WINDOW (window),
			     ctk_widget_get_screen (widget));
      
      g_signal_connect (window, "destroy",
			G_CALLBACK (ctk_widget_destroyed),
			&window);

      ctk_window_set_title (CTK_WINDOW (window), "Panes");
      ctk_container_set_border_width (CTK_CONTAINER (window), 0);

      vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
      ctk_container_add (CTK_CONTAINER (window), vbox);
      
      vpaned = ctk_paned_new (CTK_ORIENTATION_VERTICAL);
      ctk_box_pack_start (CTK_BOX (vbox), vpaned, TRUE, TRUE, 0);
      ctk_container_set_border_width (CTK_CONTAINER(vpaned), 5);

      hpaned = ctk_paned_new (CTK_ORIENTATION_HORIZONTAL);
      ctk_paned_add1 (CTK_PANED (vpaned), hpaned);

      frame = ctk_frame_new (NULL);
      ctk_frame_set_shadow_type (CTK_FRAME(frame), CTK_SHADOW_IN);
      ctk_widget_set_size_request (frame, 60, 60);
      ctk_paned_add1 (CTK_PANED (hpaned), frame);
      
      button = ctk_button_new_with_label ("Hi there");
      ctk_container_add (CTK_CONTAINER(frame), button);

      frame = ctk_frame_new (NULL);
      ctk_frame_set_shadow_type (CTK_FRAME(frame), CTK_SHADOW_IN);
      ctk_widget_set_size_request (frame, 80, 60);
      ctk_paned_add2 (CTK_PANED (hpaned), frame);

      frame = ctk_frame_new (NULL);
      ctk_frame_set_shadow_type (CTK_FRAME(frame), CTK_SHADOW_IN);
      ctk_widget_set_size_request (frame, 60, 80);
      ctk_paned_add2 (CTK_PANED (vpaned), frame);

      /* Now create toggle buttons to control sizing */

      ctk_box_pack_start (CTK_BOX (vbox),
			  create_pane_options (CTK_PANED (hpaned),
					       "Horizontal",
					       "Left",
					       "Right"),
			  FALSE, FALSE, 0);

      ctk_box_pack_start (CTK_BOX (vbox),
			  create_pane_options (CTK_PANED (vpaned),
					       "Vertical",
					       "Top",
					       "Bottom"),
			  FALSE, FALSE, 0);

      ctk_widget_show_all (vbox);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show (window);
  else
    ctk_widget_destroy (window);
}

/*
 * Paned keyboard navigation
 */

static CtkWidget*
paned_keyboard_window1 (CtkWidget *widget)
{
  CtkWidget *window1;
  CtkWidget *hpaned1;
  CtkWidget *frame1;
  CtkWidget *vbox1;
  CtkWidget *button7;
  CtkWidget *button8;
  CtkWidget *button9;
  CtkWidget *vpaned1;
  CtkWidget *frame2;
  CtkWidget *frame5;
  CtkWidget *hbox1;
  CtkWidget *button5;
  CtkWidget *button6;
  CtkWidget *frame3;
  CtkWidget *frame4;
  CtkWidget *grid1;
  CtkWidget *button1;
  CtkWidget *button2;
  CtkWidget *button3;
  CtkWidget *button4;

  window1 = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_title (CTK_WINDOW (window1), "Basic paned navigation");
  ctk_window_set_screen (CTK_WINDOW (window1), 
			 ctk_widget_get_screen (widget));

  hpaned1 = ctk_paned_new (CTK_ORIENTATION_HORIZONTAL);
  ctk_container_add (CTK_CONTAINER (window1), hpaned1);

  frame1 = ctk_frame_new (NULL);
  ctk_paned_pack1 (CTK_PANED (hpaned1), frame1, FALSE, TRUE);
  ctk_frame_set_shadow_type (CTK_FRAME (frame1), CTK_SHADOW_IN);

  vbox1 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  ctk_container_add (CTK_CONTAINER (frame1), vbox1);

  button7 = ctk_button_new_with_label ("button7");
  ctk_box_pack_start (CTK_BOX (vbox1), button7, FALSE, FALSE, 0);

  button8 = ctk_button_new_with_label ("button8");
  ctk_box_pack_start (CTK_BOX (vbox1), button8, FALSE, FALSE, 0);

  button9 = ctk_button_new_with_label ("button9");
  ctk_box_pack_start (CTK_BOX (vbox1), button9, FALSE, FALSE, 0);

  vpaned1 = ctk_paned_new (CTK_ORIENTATION_VERTICAL);
  ctk_paned_pack2 (CTK_PANED (hpaned1), vpaned1, TRUE, TRUE);

  frame2 = ctk_frame_new (NULL);
  ctk_paned_pack1 (CTK_PANED (vpaned1), frame2, FALSE, TRUE);
  ctk_frame_set_shadow_type (CTK_FRAME (frame2), CTK_SHADOW_IN);

  frame5 = ctk_frame_new (NULL);
  ctk_container_add (CTK_CONTAINER (frame2), frame5);

  hbox1 = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
  ctk_container_add (CTK_CONTAINER (frame5), hbox1);

  button5 = ctk_button_new_with_label ("button5");
  ctk_box_pack_start (CTK_BOX (hbox1), button5, FALSE, FALSE, 0);

  button6 = ctk_button_new_with_label ("button6");
  ctk_box_pack_start (CTK_BOX (hbox1), button6, FALSE, FALSE, 0);

  frame3 = ctk_frame_new (NULL);
  ctk_paned_pack2 (CTK_PANED (vpaned1), frame3, TRUE, TRUE);
  ctk_frame_set_shadow_type (CTK_FRAME (frame3), CTK_SHADOW_IN);

  frame4 = ctk_frame_new ("Buttons");
  ctk_container_add (CTK_CONTAINER (frame3), frame4);
  ctk_container_set_border_width (CTK_CONTAINER (frame4), 15);

  grid1 = ctk_grid_new ();
  ctk_container_add (CTK_CONTAINER (frame4), grid1);
  ctk_container_set_border_width (CTK_CONTAINER (grid1), 11);

  button1 = ctk_button_new_with_label ("button1");
  ctk_grid_attach (CTK_GRID (grid1), button1, 0, 0, 1, 1);

  button2 = ctk_button_new_with_label ("button2");
  ctk_grid_attach (CTK_GRID (grid1), button2, 1, 0, 1, 1);

  button3 = ctk_button_new_with_label ("button3");
  ctk_grid_attach (CTK_GRID (grid1), button3, 0, 1, 1, 1);

  button4 = ctk_button_new_with_label ("button4");
  ctk_grid_attach (CTK_GRID (grid1), button4, 1, 1, 1, 1);

  return window1;
}

static CtkWidget*
paned_keyboard_window2 (CtkWidget *widget)
{
  CtkWidget *window2;
  CtkWidget *hpaned2;
  CtkWidget *frame6;
  CtkWidget *button13;
  CtkWidget *hbox2;
  CtkWidget *vpaned2;
  CtkWidget *frame7;
  CtkWidget *button12;
  CtkWidget *frame8;
  CtkWidget *button11;
  CtkWidget *button10;

  window2 = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_title (CTK_WINDOW (window2), "\"button 10\" is not inside the horisontal pane");

  ctk_window_set_screen (CTK_WINDOW (window2), 
			 ctk_widget_get_screen (widget));

  hpaned2 = ctk_paned_new (CTK_ORIENTATION_HORIZONTAL);
  ctk_container_add (CTK_CONTAINER (window2), hpaned2);

  frame6 = ctk_frame_new (NULL);
  ctk_paned_pack1 (CTK_PANED (hpaned2), frame6, FALSE, TRUE);
  ctk_frame_set_shadow_type (CTK_FRAME (frame6), CTK_SHADOW_IN);

  button13 = ctk_button_new_with_label ("button13");
  ctk_container_add (CTK_CONTAINER (frame6), button13);

  hbox2 = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
  ctk_paned_pack2 (CTK_PANED (hpaned2), hbox2, TRUE, TRUE);

  vpaned2 = ctk_paned_new (CTK_ORIENTATION_VERTICAL);
  ctk_box_pack_start (CTK_BOX (hbox2), vpaned2, TRUE, TRUE, 0);

  frame7 = ctk_frame_new (NULL);
  ctk_paned_pack1 (CTK_PANED (vpaned2), frame7, FALSE, TRUE);
  ctk_frame_set_shadow_type (CTK_FRAME (frame7), CTK_SHADOW_IN);

  button12 = ctk_button_new_with_label ("button12");
  ctk_container_add (CTK_CONTAINER (frame7), button12);

  frame8 = ctk_frame_new (NULL);
  ctk_paned_pack2 (CTK_PANED (vpaned2), frame8, TRUE, TRUE);
  ctk_frame_set_shadow_type (CTK_FRAME (frame8), CTK_SHADOW_IN);

  button11 = ctk_button_new_with_label ("button11");
  ctk_container_add (CTK_CONTAINER (frame8), button11);

  button10 = ctk_button_new_with_label ("button10");
  ctk_box_pack_start (CTK_BOX (hbox2), button10, FALSE, FALSE, 0);

  return window2;
}

static CtkWidget*
paned_keyboard_window3 (CtkWidget *widget)
{
  CtkWidget *window3;
  CtkWidget *vbox2;
  CtkWidget *label1;
  CtkWidget *hpaned3;
  CtkWidget *frame9;
  CtkWidget *button14;
  CtkWidget *hpaned4;
  CtkWidget *frame10;
  CtkWidget *button15;
  CtkWidget *hpaned5;
  CtkWidget *frame11;
  CtkWidget *button16;
  CtkWidget *frame12;
  CtkWidget *button17;

  window3 = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  g_object_set_data (G_OBJECT (window3), "window3", window3);
  ctk_window_set_title (CTK_WINDOW (window3), "Nested panes");

  ctk_window_set_screen (CTK_WINDOW (window3), 
			 ctk_widget_get_screen (widget));
  

  vbox2 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  ctk_container_add (CTK_CONTAINER (window3), vbox2);

  label1 = ctk_label_new ("Three panes nested inside each other");
  ctk_box_pack_start (CTK_BOX (vbox2), label1, FALSE, FALSE, 0);

  hpaned3 = ctk_paned_new (CTK_ORIENTATION_HORIZONTAL);
  ctk_box_pack_start (CTK_BOX (vbox2), hpaned3, TRUE, TRUE, 0);

  frame9 = ctk_frame_new (NULL);
  ctk_paned_pack1 (CTK_PANED (hpaned3), frame9, FALSE, TRUE);
  ctk_frame_set_shadow_type (CTK_FRAME (frame9), CTK_SHADOW_IN);

  button14 = ctk_button_new_with_label ("button14");
  ctk_container_add (CTK_CONTAINER (frame9), button14);

  hpaned4 = ctk_paned_new (CTK_ORIENTATION_HORIZONTAL);
  ctk_paned_pack2 (CTK_PANED (hpaned3), hpaned4, TRUE, TRUE);

  frame10 = ctk_frame_new (NULL);
  ctk_paned_pack1 (CTK_PANED (hpaned4), frame10, FALSE, TRUE);
  ctk_frame_set_shadow_type (CTK_FRAME (frame10), CTK_SHADOW_IN);

  button15 = ctk_button_new_with_label ("button15");
  ctk_container_add (CTK_CONTAINER (frame10), button15);

  hpaned5 = ctk_paned_new (CTK_ORIENTATION_HORIZONTAL);
  ctk_paned_pack2 (CTK_PANED (hpaned4), hpaned5, TRUE, TRUE);

  frame11 = ctk_frame_new (NULL);
  ctk_paned_pack1 (CTK_PANED (hpaned5), frame11, FALSE, TRUE);
  ctk_frame_set_shadow_type (CTK_FRAME (frame11), CTK_SHADOW_IN);

  button16 = ctk_button_new_with_label ("button16");
  ctk_container_add (CTK_CONTAINER (frame11), button16);

  frame12 = ctk_frame_new (NULL);
  ctk_paned_pack2 (CTK_PANED (hpaned5), frame12, TRUE, TRUE);
  ctk_frame_set_shadow_type (CTK_FRAME (frame12), CTK_SHADOW_IN);

  button17 = ctk_button_new_with_label ("button17");
  ctk_container_add (CTK_CONTAINER (frame12), button17);

  return window3;
}

static CtkWidget*
paned_keyboard_window4 (CtkWidget *widget)
{
  CtkWidget *window4;
  CtkWidget *vbox3;
  CtkWidget *label2;
  CtkWidget *hpaned6;
  CtkWidget *vpaned3;
  CtkWidget *button19;
  CtkWidget *button18;
  CtkWidget *hbox3;
  CtkWidget *vpaned4;
  CtkWidget *button21;
  CtkWidget *button20;
  CtkWidget *vpaned5;
  CtkWidget *button23;
  CtkWidget *button22;
  CtkWidget *vpaned6;
  CtkWidget *button25;
  CtkWidget *button24;

  window4 = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  g_object_set_data (G_OBJECT (window4), "window4", window4);
  ctk_window_set_title (CTK_WINDOW (window4), "window4");

  ctk_window_set_screen (CTK_WINDOW (window4), 
			 ctk_widget_get_screen (widget));

  vbox3 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  ctk_container_add (CTK_CONTAINER (window4), vbox3);

  label2 = ctk_label_new ("Widget tree:\n\nhpaned \n - vpaned\n - hbox\n    - vpaned\n    - vpaned\n    - vpaned\n");
  ctk_box_pack_start (CTK_BOX (vbox3), label2, FALSE, FALSE, 0);
  ctk_label_set_justify (CTK_LABEL (label2), CTK_JUSTIFY_LEFT);

  hpaned6 = ctk_paned_new (CTK_ORIENTATION_HORIZONTAL);
  ctk_box_pack_start (CTK_BOX (vbox3), hpaned6, TRUE, TRUE, 0);

  vpaned3 = ctk_paned_new (CTK_ORIENTATION_VERTICAL);
  ctk_paned_pack1 (CTK_PANED (hpaned6), vpaned3, FALSE, TRUE);

  button19 = ctk_button_new_with_label ("button19");
  ctk_paned_pack1 (CTK_PANED (vpaned3), button19, FALSE, TRUE);

  button18 = ctk_button_new_with_label ("button18");
  ctk_paned_pack2 (CTK_PANED (vpaned3), button18, TRUE, TRUE);

  hbox3 = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
  ctk_paned_pack2 (CTK_PANED (hpaned6), hbox3, TRUE, TRUE);

  vpaned4 = ctk_paned_new (CTK_ORIENTATION_VERTICAL);
  ctk_box_pack_start (CTK_BOX (hbox3), vpaned4, TRUE, TRUE, 0);

  button21 = ctk_button_new_with_label ("button21");
  ctk_paned_pack1 (CTK_PANED (vpaned4), button21, FALSE, TRUE);

  button20 = ctk_button_new_with_label ("button20");
  ctk_paned_pack2 (CTK_PANED (vpaned4), button20, TRUE, TRUE);

  vpaned5 = ctk_paned_new (CTK_ORIENTATION_VERTICAL);
  ctk_box_pack_start (CTK_BOX (hbox3), vpaned5, TRUE, TRUE, 0);

  button23 = ctk_button_new_with_label ("button23");
  ctk_paned_pack1 (CTK_PANED (vpaned5), button23, FALSE, TRUE);

  button22 = ctk_button_new_with_label ("button22");
  ctk_paned_pack2 (CTK_PANED (vpaned5), button22, TRUE, TRUE);

  vpaned6 = ctk_paned_new (CTK_ORIENTATION_VERTICAL);
  ctk_box_pack_start (CTK_BOX (hbox3), vpaned6, TRUE, TRUE, 0);

  button25 = ctk_button_new_with_label ("button25");
  ctk_paned_pack1 (CTK_PANED (vpaned6), button25, FALSE, TRUE);

  button24 = ctk_button_new_with_label ("button24");
  ctk_paned_pack2 (CTK_PANED (vpaned6), button24, TRUE, TRUE);

  return window4;
}

static void
create_paned_keyboard_navigation (CtkWidget *widget)
{
  static CtkWidget *window1 = NULL;
  static CtkWidget *window2 = NULL;
  static CtkWidget *window3 = NULL;
  static CtkWidget *window4 = NULL;

  if (window1 && 
     (ctk_widget_get_screen (window1) != ctk_widget_get_screen (widget)))
    {
      ctk_widget_destroy (window1);
      ctk_widget_destroy (window2);
      ctk_widget_destroy (window3);
      ctk_widget_destroy (window4);
    }
  
  if (!window1)
    {
      window1 = paned_keyboard_window1 (widget);
      g_signal_connect (window1, "destroy",
			G_CALLBACK (ctk_widget_destroyed),
			&window1);
    }

  if (!window2)
    {
      window2 = paned_keyboard_window2 (widget);
      g_signal_connect (window2, "destroy",
			G_CALLBACK (ctk_widget_destroyed),
			&window2);
    }

  if (!window3)
    {
      window3 = paned_keyboard_window3 (widget);
      g_signal_connect (window3, "destroy",
			G_CALLBACK (ctk_widget_destroyed),
			&window3);
    }

  if (!window4)
    {
      window4 = paned_keyboard_window4 (widget);
      g_signal_connect (window4, "destroy",
			G_CALLBACK (ctk_widget_destroyed),
			&window4);
    }

  if (ctk_widget_get_visible (window1))
    ctk_widget_destroy (CTK_WIDGET (window1));
  else
    ctk_widget_show_all (CTK_WIDGET (window1));

  if (ctk_widget_get_visible (window2))
    ctk_widget_destroy (CTK_WIDGET (window2));
  else
    ctk_widget_show_all (CTK_WIDGET (window2));

  if (ctk_widget_get_visible (window3))
    ctk_widget_destroy (CTK_WIDGET (window3));
  else
    ctk_widget_show_all (CTK_WIDGET (window3));

  if (ctk_widget_get_visible (window4))
    ctk_widget_destroy (CTK_WIDGET (window4));
  else
    ctk_widget_show_all (CTK_WIDGET (window4));
}


/*
 * Shaped Windows
 */

typedef struct _cursoroffset {gint x,y;} CursorOffset;

static void
shape_pressed (CtkWidget *widget, CdkEventButton *event)
{
  CursorOffset *p;

  /* ignore double and triple click */
  if (event->type != CDK_BUTTON_PRESS)
    return;

  p = g_object_get_data (G_OBJECT (widget), "cursor_offset");
  p->x = (int) event->x;
  p->y = (int) event->y;

  ctk_grab_add (widget);
  cdk_seat_grab (cdk_event_get_seat ((CdkEvent *) event),
                 ctk_widget_get_window (widget),
                 CDK_SEAT_CAPABILITY_ALL_POINTING,
                 TRUE, NULL, (CdkEvent *) event, NULL, NULL);
}

static void
shape_released (CtkWidget      *widget,
                CdkEventButton *event)
{
  ctk_grab_remove (widget);
  cdk_seat_ungrab (cdk_event_get_seat ((CdkEvent *) event));
}

static void
shape_motion (CtkWidget      *widget,
	      CdkEventMotion *event)
{
  gint xp, yp;
  CursorOffset * p;

  p = g_object_get_data (G_OBJECT (widget), "cursor_offset");

  /*
   * Can't use event->x / event->y here 
   * because I need absolute coordinates.
   */
  cdk_window_get_device_position (cdk_screen_get_root_window (ctk_widget_get_screen (widget)),
                                  cdk_event_get_device ((CdkEvent *) event),
                                  &xp, &yp, NULL);
  ctk_window_move (CTK_WINDOW (widget), xp  - p->x, yp  - p->y);
}

CtkWidget *
shape_create_icon (CdkScreen *screen,
		   char      *xpm_file,
		   gint       x,
		   gint       y,
		   gint       px,
		   gint       py,
		   gint       window_type)
{
  CtkWidget *window;
  CtkWidget *image;
  CtkWidget *fixed;
  CursorOffset* icon_pos;
  cairo_surface_t *mask;
  cairo_region_t *mask_region;
  GdkPixbuf *pixbuf;
  cairo_t *cr;

  /*
   * CDK_WINDOW_TOPLEVEL works also, giving you a title border
   */
  window = ctk_window_new (window_type);
  ctk_window_set_screen (CTK_WINDOW (window), screen);
  
  fixed = ctk_fixed_new ();
  ctk_widget_set_size_request (fixed, 100, 100);
  ctk_container_add (CTK_CONTAINER (window), fixed);
  ctk_widget_show (fixed);
  
  ctk_widget_set_events (window, 
			 ctk_widget_get_events (window) |
			 CDK_BUTTON_MOTION_MASK |
			 CDK_BUTTON_PRESS_MASK);

  ctk_widget_realize (window);

  pixbuf = gdk_pixbuf_new_from_file (xpm_file, NULL);
  g_assert (pixbuf); /* FIXME: error handling */

  mask = cairo_image_surface_create (CAIRO_FORMAT_A1,
                                     gdk_pixbuf_get_width (pixbuf),
                                     gdk_pixbuf_get_height (pixbuf));
  cr = cairo_create (mask);
  cdk_cairo_set_source_pixbuf (cr, pixbuf, 0, 0);
  cairo_paint (cr);
  cairo_destroy (cr);

  mask_region = cdk_cairo_region_create_from_surface (mask);
                                                  
  cairo_region_translate (mask_region, px, py);

  image = ctk_image_new_from_pixbuf (pixbuf);
  ctk_fixed_put (CTK_FIXED (fixed), image, px,py);
  ctk_widget_show (image);
  
  ctk_widget_shape_combine_region (window, mask_region);
  
  cairo_region_destroy (mask_region);
  cairo_surface_destroy (mask);
  g_object_unref (pixbuf);

  g_signal_connect (window, "button_press_event",
		    G_CALLBACK (shape_pressed), NULL);
  g_signal_connect (window, "button_release_event",
		    G_CALLBACK (shape_released), NULL);
  g_signal_connect (window, "motion_notify_event",
		    G_CALLBACK (shape_motion), NULL);

  icon_pos = g_new (CursorOffset, 1);
  g_object_set_data (G_OBJECT (window), "cursor_offset", icon_pos);

  ctk_window_move (CTK_WINDOW (window), x, y);
  ctk_widget_show (window);
  
  return window;
}

void 
create_shapes (CtkWidget *widget)
{
  /* Variables used by the Drag/Drop and Shape Window demos */
  static CtkWidget *modeller = NULL;
  static CtkWidget *sheets = NULL;
  static CtkWidget *rings = NULL;
  static CtkWidget *with_region = NULL;
  CdkScreen *screen = ctk_widget_get_screen (widget);
  
  if (!(file_exists ("Modeller.xpm") &&
	file_exists ("FilesQueue.xpm") &&
	file_exists ("3DRings.xpm")))
    return;
  

  if (!modeller)
    {
      modeller = shape_create_icon (screen, "Modeller.xpm",
				    440, 140, 0,0, CTK_WINDOW_POPUP);

      g_signal_connect (modeller, "destroy",
			G_CALLBACK (ctk_widget_destroyed),
			&modeller);
    }
  else
    ctk_widget_destroy (modeller);

  if (!sheets)
    {
      sheets = shape_create_icon (screen, "FilesQueue.xpm",
				  580, 170, 0,0, CTK_WINDOW_POPUP);

      g_signal_connect (sheets, "destroy",
			G_CALLBACK (ctk_widget_destroyed),
			&sheets);

    }
  else
    ctk_widget_destroy (sheets);

  if (!rings)
    {
      rings = shape_create_icon (screen, "3DRings.xpm",
				 460, 270, 25,25, CTK_WINDOW_TOPLEVEL);

      g_signal_connect (rings, "destroy",
			G_CALLBACK (ctk_widget_destroyed),
			&rings);
    }
  else
    ctk_widget_destroy (rings);

  if (!with_region)
    {
      cairo_region_t *region;
      gint x, y;
      
      with_region = shape_create_icon (screen, "3DRings.xpm",
                                       460, 270, 25,25, CTK_WINDOW_TOPLEVEL);

      ctk_window_set_decorated (CTK_WINDOW (with_region), FALSE);
      
      g_signal_connect (with_region, "destroy",
			G_CALLBACK (ctk_widget_destroyed),
			&with_region);

      /* reset shape from mask to a region */
      x = 0;
      y = 0;
      region = cairo_region_create ();

      while (x < 460)
        {
          while (y < 270)
            {
              CdkRectangle rect;
              rect.x = x;
              rect.y = y;
              rect.width = 10;
              rect.height = 10;

              cairo_region_union_rectangle (region, &rect);
              
              y += 20;
            }
          y = 0;
          x += 20;
        }

      cdk_window_shape_combine_region (ctk_widget_get_window (with_region),
                                       region,
                                       0, 0);
    }
  else
    ctk_widget_destroy (with_region);
}

/*
 * WM Hints demo
 */

void
create_wmhints (CtkWidget *widget)
{
  static CtkWidget *window = NULL;
  CtkWidget *label;
  CtkWidget *separator;
  CtkWidget *button;
  CtkWidget *box1;
  CtkWidget *box2;
  CdkWindow *cdk_window;
  GdkPixbuf *pixbuf;
  GList *list;

  if (!window)
    {
      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);

      ctk_window_set_screen (CTK_WINDOW (window),
			     ctk_widget_get_screen (widget));
      
      g_signal_connect (window, "destroy",
			G_CALLBACK (ctk_widget_destroyed),
			&window);

      ctk_window_set_title (CTK_WINDOW (window), "WM Hints");
      ctk_container_set_border_width (CTK_CONTAINER (window), 0);

      ctk_widget_realize (window);

      cdk_window = ctk_widget_get_window (window);

      pixbuf = gdk_pixbuf_new_from_xpm_data ((const char **) openfile);
      list = g_list_prepend (NULL, pixbuf);

      cdk_window_set_icon_list (cdk_window, list);
      
      g_list_free (list);
      g_object_unref (pixbuf);

      cdk_window_set_icon_name (cdk_window, "WMHints Test Icon");
  
      cdk_window_set_decorations (cdk_window, CDK_DECOR_ALL | CDK_DECOR_MENU);
      cdk_window_set_functions (cdk_window, CDK_FUNC_ALL | CDK_FUNC_RESIZE);
      
      box1 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
      ctk_container_add (CTK_CONTAINER (window), box1);
      ctk_widget_show (box1);

      label = ctk_label_new ("Try iconizing me!");
      ctk_widget_set_size_request (label, 150, 50);
      ctk_box_pack_start (CTK_BOX (box1), label, TRUE, TRUE, 0);
      ctk_widget_show (label);


      separator = ctk_separator_new (CTK_ORIENTATION_HORIZONTAL);
      ctk_box_pack_start (CTK_BOX (box1), separator, FALSE, TRUE, 0);
      ctk_widget_show (separator);


      box2 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 10);
      ctk_container_set_border_width (CTK_CONTAINER (box2), 10);
      ctk_box_pack_start (CTK_BOX (box1), box2, FALSE, TRUE, 0);
      ctk_widget_show (box2);


      button = ctk_button_new_with_label ("close");

      g_signal_connect_swapped (button, "clicked",
				G_CALLBACK (ctk_widget_destroy),
				window);

      ctk_box_pack_start (CTK_BOX (box2), button, TRUE, TRUE, 0);
      ctk_widget_set_can_default (button, TRUE);
      ctk_widget_grab_default (button);
      ctk_widget_show (button);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show (window);
  else
    ctk_widget_destroy (window);
}


/*
 * Window state tracking
 */

static gint
window_state_callback (CtkWidget *widget,
                       CdkEventWindowState *event,
                       gpointer data)
{
  CtkWidget *label = data;
  gchar *msg;

  msg = g_strconcat (ctk_window_get_title (CTK_WINDOW (widget)), ": ",
                     (event->new_window_state & CDK_WINDOW_STATE_WITHDRAWN) ?
                     "withdrawn" : "not withdrawn", ", ",
                     (event->new_window_state & CDK_WINDOW_STATE_ICONIFIED) ?
                     "iconified" : "not iconified", ", ",
                     (event->new_window_state & CDK_WINDOW_STATE_STICKY) ?
                     "sticky" : "not sticky", ", ",
                     (event->new_window_state & CDK_WINDOW_STATE_MAXIMIZED) ?
                     "maximized" : "not maximized", ", ",
                     (event->new_window_state & CDK_WINDOW_STATE_FULLSCREEN) ?
                     "fullscreen" : "not fullscreen",
                     (event->new_window_state & CDK_WINDOW_STATE_ABOVE) ?
                     "above" : "not above", ", ",
                     (event->new_window_state & CDK_WINDOW_STATE_BELOW) ?
                     "below" : "not below", ", ",
                     NULL);
  
  ctk_label_set_text (CTK_LABEL (label), msg);

  g_free (msg);

  return FALSE;
}

static CtkWidget*
tracking_label (CtkWidget *window)
{
  CtkWidget *label;
  CtkWidget *hbox;
  CtkWidget *button;

  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 5);

  g_signal_connect_object (hbox,
			   "destroy",
			   G_CALLBACK (ctk_widget_destroy),
			   window,
			   G_CONNECT_SWAPPED);
  
  label = ctk_label_new ("<no window state events received>");
  ctk_label_set_line_wrap (CTK_LABEL (label), TRUE);
  ctk_box_pack_start (CTK_BOX (hbox), label, FALSE, FALSE, 0);
  
  g_signal_connect (window,
		    "window_state_event",
		    G_CALLBACK (window_state_callback),
		    label);

  button = ctk_button_new_with_label ("Deiconify");
  g_signal_connect_object (button,
			   "clicked",
			   G_CALLBACK (ctk_window_deiconify),
                           window,
			   G_CONNECT_SWAPPED);
  ctk_box_pack_end (CTK_BOX (hbox), button, FALSE, FALSE, 0);

  button = ctk_button_new_with_label ("Iconify");
  g_signal_connect_object (button,
			   "clicked",
			   G_CALLBACK (ctk_window_iconify),
                           window,
			   G_CONNECT_SWAPPED);
  ctk_box_pack_end (CTK_BOX (hbox), button, FALSE, FALSE, 0);

  button = ctk_button_new_with_label ("Fullscreen");
  g_signal_connect_object (button,
			   "clicked",
			   G_CALLBACK (ctk_window_fullscreen),
                           window,
			   G_CONNECT_SWAPPED);
  ctk_box_pack_end (CTK_BOX (hbox), button, FALSE, FALSE, 0);

  button = ctk_button_new_with_label ("Unfullscreen");
  g_signal_connect_object (button,
			   "clicked",
			   G_CALLBACK (ctk_window_unfullscreen),
                           window,
			   G_CONNECT_SWAPPED);
  ctk_box_pack_end (CTK_BOX (hbox), button, FALSE, FALSE, 0);
  
  button = ctk_button_new_with_label ("Present");
  g_signal_connect_object (button,
			   "clicked",
			   G_CALLBACK (ctk_window_present),
                           window,
			   G_CONNECT_SWAPPED);
  ctk_box_pack_end (CTK_BOX (hbox), button, FALSE, FALSE, 0);

  button = ctk_button_new_with_label ("Show");
  g_signal_connect_object (button,
			   "clicked",
			   G_CALLBACK (ctk_widget_show),
                           window,
			   G_CONNECT_SWAPPED);
  ctk_box_pack_end (CTK_BOX (hbox), button, FALSE, FALSE, 0);
  
  ctk_widget_show_all (hbox);
  
  return hbox;
}

void
keep_window_above (CtkToggleButton *togglebutton, gpointer data)
{
  CtkWidget *button = g_object_get_data (G_OBJECT (togglebutton), "radio");

  ctk_window_set_keep_above (CTK_WINDOW (data),
                             ctk_toggle_button_get_active (togglebutton));

  if (ctk_toggle_button_get_active (togglebutton))
    ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (button), FALSE);
}

void
keep_window_below (CtkToggleButton *togglebutton, gpointer data)
{
  CtkWidget *button = g_object_get_data (G_OBJECT (togglebutton), "radio");

  ctk_window_set_keep_below (CTK_WINDOW (data),
                             ctk_toggle_button_get_active (togglebutton));

  if (ctk_toggle_button_get_active (togglebutton))
    ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (button), FALSE);
}


static CtkWidget*
get_state_controls (CtkWidget *window)
{
  CtkWidget *vbox;
  CtkWidget *button;
  CtkWidget *button_above;
  CtkWidget *button_below;

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  
  button = ctk_button_new_with_label ("Stick");
  g_signal_connect_object (button,
			   "clicked",
			   G_CALLBACK (ctk_window_stick),
			   window,
			   G_CONNECT_SWAPPED);
  ctk_box_pack_start (CTK_BOX (vbox), button, FALSE, FALSE, 0);

  button = ctk_button_new_with_label ("Unstick");
  g_signal_connect_object (button,
			   "clicked",
			   G_CALLBACK (ctk_window_unstick),
			   window,
			   G_CONNECT_SWAPPED);
  ctk_box_pack_start (CTK_BOX (vbox), button, FALSE, FALSE, 0);
  
  button = ctk_button_new_with_label ("Maximize");
  g_signal_connect_object (button,
			   "clicked",
			   G_CALLBACK (ctk_window_maximize),
			   window,
			   G_CONNECT_SWAPPED);
  ctk_box_pack_start (CTK_BOX (vbox), button, FALSE, FALSE, 0);

  button = ctk_button_new_with_label ("Unmaximize");
  g_signal_connect_object (button,
			   "clicked",
			   G_CALLBACK (ctk_window_unmaximize),
			   window,
			   G_CONNECT_SWAPPED);
  ctk_box_pack_start (CTK_BOX (vbox), button, FALSE, FALSE, 0);

  button = ctk_button_new_with_label ("Iconify");
  g_signal_connect_object (button,
			   "clicked",
			   G_CALLBACK (ctk_window_iconify),
			   window,
			   G_CONNECT_SWAPPED);
  ctk_box_pack_start (CTK_BOX (vbox), button, FALSE, FALSE, 0);

  button = ctk_button_new_with_label ("Fullscreen");
  g_signal_connect_object (button,
			   "clicked",
			   G_CALLBACK (ctk_window_fullscreen),
                           window,
			   G_CONNECT_SWAPPED);
  ctk_box_pack_start (CTK_BOX (vbox), button, FALSE, FALSE, 0);

  button = ctk_button_new_with_label ("Unfullscreen");
  g_signal_connect_object (button,
			   "clicked",
                           G_CALLBACK (ctk_window_unfullscreen),
			   window,
			   G_CONNECT_SWAPPED);
  ctk_box_pack_start (CTK_BOX (vbox), button, FALSE, FALSE, 0);

  button_above = ctk_toggle_button_new_with_label ("Keep above");
  g_signal_connect (button_above,
		    "toggled",
		    G_CALLBACK (keep_window_above),
		    window);
  ctk_box_pack_start (CTK_BOX (vbox), button_above, FALSE, FALSE, 0);

  button_below = ctk_toggle_button_new_with_label ("Keep below");
  g_signal_connect (button_below,
		    "toggled",
		    G_CALLBACK (keep_window_below),
		    window);
  ctk_box_pack_start (CTK_BOX (vbox), button_below, FALSE, FALSE, 0);

  g_object_set_data (G_OBJECT (button_above), "radio", button_below);
  g_object_set_data (G_OBJECT (button_below), "radio", button_above);

  button = ctk_button_new_with_label ("Hide (withdraw)");
  g_signal_connect_object (button,
			   "clicked",
			   G_CALLBACK (ctk_widget_hide),
			   window,
			   G_CONNECT_SWAPPED);
  ctk_box_pack_start (CTK_BOX (vbox), button, FALSE, FALSE, 0);
  
  ctk_widget_show_all (vbox);

  return vbox;
}

void
create_window_states (CtkWidget *widget)
{
  static CtkWidget *window = NULL;
  CtkWidget *label;
  CtkWidget *box1;
  CtkWidget *iconified;
  CtkWidget *normal;
  CtkWidget *controls;

  if (!window)
    {
      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_screen (CTK_WINDOW (window),
			     ctk_widget_get_screen (widget));

      g_signal_connect (window, "destroy",
			G_CALLBACK (ctk_widget_destroyed),
			&window);

      ctk_window_set_title (CTK_WINDOW (window), "Window states");
      
      box1 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
      ctk_container_add (CTK_CONTAINER (window), box1);

      iconified = ctk_window_new (CTK_WINDOW_TOPLEVEL);

      ctk_window_set_screen (CTK_WINDOW (iconified),
			     ctk_widget_get_screen (widget));
      
      g_signal_connect_object (iconified, "destroy",
			       G_CALLBACK (ctk_widget_destroy),
			       window,
			       G_CONNECT_SWAPPED);
      ctk_window_iconify (CTK_WINDOW (iconified));
      ctk_window_set_title (CTK_WINDOW (iconified), "Iconified initially");
      controls = get_state_controls (iconified);
      ctk_container_add (CTK_CONTAINER (iconified), controls);
      
      normal = ctk_window_new (CTK_WINDOW_TOPLEVEL);

      ctk_window_set_screen (CTK_WINDOW (normal),
			     ctk_widget_get_screen (widget));
      
      g_signal_connect_object (normal, "destroy",
			       G_CALLBACK (ctk_widget_destroy),
			       window,
			       G_CONNECT_SWAPPED);
      
      ctk_window_set_title (CTK_WINDOW (normal), "Deiconified initially");
      controls = get_state_controls (normal);
      ctk_container_add (CTK_CONTAINER (normal), controls);
      
      label = tracking_label (iconified);
      ctk_container_add (CTK_CONTAINER (box1), label);

      label = tracking_label (normal);
      ctk_container_add (CTK_CONTAINER (box1), label);

      ctk_widget_show_all (iconified);
      ctk_widget_show_all (normal);
      ctk_widget_show_all (box1);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show (window);
  else
    ctk_widget_destroy (window);
}

/*
 * Window sizing
 */

static gint
configure_event_callback (CtkWidget *widget,
                          CdkEventConfigure *event,
                          gpointer data)
{
  CtkWidget *label = data;
  gchar *msg;
  gint x, y;
  
  ctk_window_get_position (CTK_WINDOW (widget), &x, &y);
  
  msg = g_strdup_printf ("event: %d,%d  %d x %d\n"
                         "position: %d, %d",
                         event->x, event->y, event->width, event->height,
                         x, y);
  
  ctk_label_set_text (CTK_LABEL (label), msg);

  g_free (msg);

  return FALSE;
}

static void
get_ints (CtkWidget *window,
          gint      *a,
          gint      *b)
{
  CtkWidget *spin1;
  CtkWidget *spin2;

  spin1 = g_object_get_data (G_OBJECT (window), "spin1");
  spin2 = g_object_get_data (G_OBJECT (window), "spin2");

  *a = ctk_spin_button_get_value_as_int (CTK_SPIN_BUTTON (spin1));
  *b = ctk_spin_button_get_value_as_int (CTK_SPIN_BUTTON (spin2));
}

static void
set_size_callback (CtkWidget *widget,
                   gpointer   data)
{
  gint w, h;
  
  get_ints (data, &w, &h);

  ctk_window_resize (CTK_WINDOW (g_object_get_data (data, "target")), w, h);
}

static void
unset_default_size_callback (CtkWidget *widget,
                             gpointer   data)
{
  ctk_window_set_default_size (g_object_get_data (data, "target"),
                               -1, -1);
}

static void
set_default_size_callback (CtkWidget *widget,
                           gpointer   data)
{
  gint w, h;
  
  get_ints (data, &w, &h);

  ctk_window_set_default_size (g_object_get_data (data, "target"),
                               w, h);
}

static void
unset_size_request_callback (CtkWidget *widget,
			     gpointer   data)
{
  ctk_widget_set_size_request (g_object_get_data (data, "target"),
                               -1, -1);
}

static void
set_size_request_callback (CtkWidget *widget,
			   gpointer   data)
{
  gint w, h;
  
  get_ints (data, &w, &h);

  ctk_widget_set_size_request (g_object_get_data (data, "target"),
                               w, h);
}

static void
set_location_callback (CtkWidget *widget,
                       gpointer   data)
{
  gint x, y;
  
  get_ints (data, &x, &y);

  ctk_window_move (g_object_get_data (data, "target"), x, y);
}

static void
move_to_position_callback (CtkWidget *widget,
                           gpointer   data)
{
  gint x, y;
  CtkWindow *window;

  window = g_object_get_data (data, "target");
  
  ctk_window_get_position (window, &x, &y);

  ctk_window_move (window, x, y);
}

static void
set_geometry_callback (CtkWidget *entry,
                       gpointer   data)
{
  gchar *text;
  CtkWindow *target;

  target = CTK_WINDOW (g_object_get_data (G_OBJECT (data), "target"));
  
  text = ctk_editable_get_chars (CTK_EDITABLE (entry), 0, -1);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  if (!ctk_window_parse_geometry (target, text))
    g_print ("Bad geometry string '%s'\n", text);
G_GNUC_END_IGNORE_DEPRECATIONS

  g_free (text);
}

static void
resizable_callback (CtkWidget *widget,
                     gpointer   data)
{
  g_object_set (g_object_get_data (data, "target"),
                "resizable", ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (widget)),
                NULL);
}

static void
gravity_selected (CtkWidget *widget,
                  gpointer   data)
{
  ctk_window_set_gravity (CTK_WINDOW (g_object_get_data (data, "target")),
                          ctk_combo_box_get_active (CTK_COMBO_BOX (widget)) + CDK_GRAVITY_NORTH_WEST);
}

static void
pos_selected (CtkWidget *widget,
              gpointer   data)
{
  ctk_window_set_position (CTK_WINDOW (g_object_get_data (data, "target")),
                           ctk_combo_box_get_active (CTK_COMBO_BOX (widget)) + CTK_WIN_POS_NONE);
}

static void
move_gravity_window_to_current_position (CtkWidget *widget,
                                         gpointer   data)
{
  gint x, y;
  CtkWindow *window;

  window = CTK_WINDOW (data);    
  
  ctk_window_get_position (window, &x, &y);

  ctk_window_move (window, x, y);
}

static void
get_screen_corner (CtkWindow *window,
                   gint      *x,
                   gint      *y)
{
  int w, h;
  CdkScreen * screen = ctk_window_get_screen (window);
  
  ctk_window_get_size (CTK_WINDOW (window), &w, &h);

  switch (ctk_window_get_gravity (window))
    {
    case CDK_GRAVITY_SOUTH_EAST:
      *x = cdk_screen_get_width (screen) - w;
      *y = cdk_screen_get_height (screen) - h;
      break;

    case CDK_GRAVITY_NORTH_EAST:
      *x = cdk_screen_get_width (screen) - w;
      *y = 0;
      break;

    case CDK_GRAVITY_SOUTH_WEST:
      *x = 0;
      *y = cdk_screen_get_height (screen) - h;
      break;

    case CDK_GRAVITY_NORTH_WEST:
      *x = 0;
      *y = 0;
      break;
      
    case CDK_GRAVITY_SOUTH:
      *x = (cdk_screen_get_width (screen) - w) / 2;
      *y = cdk_screen_get_height (screen) - h;
      break;

    case CDK_GRAVITY_NORTH:
      *x = (cdk_screen_get_width (screen) - w) / 2;
      *y = 0;
      break;

    case CDK_GRAVITY_WEST:
      *x = 0;
      *y = (cdk_screen_get_height (screen) - h) / 2;
      break;

    case CDK_GRAVITY_EAST:
      *x = cdk_screen_get_width (screen) - w;
      *y = (cdk_screen_get_height (screen) - h) / 2;
      break;

    case CDK_GRAVITY_CENTER:
      *x = (cdk_screen_get_width (screen) - w) / 2;
      *y = (cdk_screen_get_height (screen) - h) / 2;
      break;

    case CDK_GRAVITY_STATIC:
      /* pick some random numbers */
      *x = 350;
      *y = 350;
      break;

    default:
      g_assert_not_reached ();
      break;
    }
}

static void
move_gravity_window_to_starting_position (CtkWidget *widget,
                                          gpointer   data)
{
  gint x, y;
  CtkWindow *window;

  window = CTK_WINDOW (data);    
  
  get_screen_corner (window,
                     &x, &y);
  
  ctk_window_move (window, x, y);
}

static CtkWidget*
make_gravity_window (CtkWidget   *destroy_with,
                     CdkGravity   gravity,
                     const gchar *title)
{
  CtkWidget *window;
  CtkWidget *button;
  CtkWidget *vbox;
  int x, y;
  
  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);

  ctk_window_set_screen (CTK_WINDOW (window),
			 ctk_widget_get_screen (destroy_with));

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  ctk_widget_show (vbox);
  
  ctk_container_add (CTK_CONTAINER (window), vbox);
  ctk_window_set_title (CTK_WINDOW (window), title);
  ctk_window_set_gravity (CTK_WINDOW (window), gravity);

  g_signal_connect_object (destroy_with,
			   "destroy",
			   G_CALLBACK (ctk_widget_destroy),
			   window,
			   G_CONNECT_SWAPPED);

  
  button = ctk_button_new_with_mnemonic ("_Move to current position");

  g_signal_connect (button, "clicked",
                    G_CALLBACK (move_gravity_window_to_current_position),
                    window);

  ctk_container_add (CTK_CONTAINER (vbox), button);
  ctk_widget_show (button);

  button = ctk_button_new_with_mnemonic ("Move to _starting position");

  g_signal_connect (button, "clicked",
                    G_CALLBACK (move_gravity_window_to_starting_position),
                    window);

  ctk_container_add (CTK_CONTAINER (vbox), button);
  ctk_widget_show (button);
  
  /* Pretend this is the result of --geometry.
   * DO NOT COPY THIS CODE unless you are setting --geometry results,
   * and in that case you probably should just use ctk_window_parse_geometry().
   * AGAIN, DO NOT SET CDK_HINT_USER_POS! It violates the ICCCM unless
   * you are parsing --geometry or equivalent.
   */
  ctk_window_set_geometry_hints (CTK_WINDOW (window),
                                 NULL, NULL,
                                 CDK_HINT_USER_POS);

  ctk_window_set_default_size (CTK_WINDOW (window),
                               200, 200);

  get_screen_corner (CTK_WINDOW (window), &x, &y);
  
  ctk_window_move (CTK_WINDOW (window),
                   x, y);
  
  return window;
}

static void
do_gravity_test (CtkWidget *widget,
                 gpointer   data)
{
  CtkWidget *destroy_with = data;
  CtkWidget *window;
  
  /* We put a window at each gravity point on the screen. */
  window = make_gravity_window (destroy_with, CDK_GRAVITY_NORTH_WEST,
                                "NorthWest");
  ctk_widget_show (window);
  
  window = make_gravity_window (destroy_with, CDK_GRAVITY_SOUTH_EAST,
                                "SouthEast");
  ctk_widget_show (window);

  window = make_gravity_window (destroy_with, CDK_GRAVITY_NORTH_EAST,
                                "NorthEast");
  ctk_widget_show (window);

  window = make_gravity_window (destroy_with, CDK_GRAVITY_SOUTH_WEST,
                                "SouthWest");
  ctk_widget_show (window);

  window = make_gravity_window (destroy_with, CDK_GRAVITY_SOUTH,
                                "South");
  ctk_widget_show (window);

  window = make_gravity_window (destroy_with, CDK_GRAVITY_NORTH,
                                "North");
  ctk_widget_show (window);

  
  window = make_gravity_window (destroy_with, CDK_GRAVITY_WEST,
                                "West");
  ctk_widget_show (window);

    
  window = make_gravity_window (destroy_with, CDK_GRAVITY_EAST,
                                "East");
  ctk_widget_show (window);

  window = make_gravity_window (destroy_with, CDK_GRAVITY_CENTER,
                                "Center");
  ctk_widget_show (window);

  window = make_gravity_window (destroy_with, CDK_GRAVITY_STATIC,
                                "Static");
  ctk_widget_show (window);
}

static CtkWidget*
window_controls (CtkWidget *window)
{
  CtkWidget *control_window;
  CtkWidget *label;
  CtkWidget *vbox;
  CtkWidget *button;
  CtkWidget *spin;
  CtkAdjustment *adjustment;
  CtkWidget *entry;
  CtkWidget *om;
  gint i;
  
  control_window = ctk_window_new (CTK_WINDOW_TOPLEVEL);

  ctk_window_set_screen (CTK_WINDOW (control_window),
			 ctk_widget_get_screen (window));

  ctk_window_set_title (CTK_WINDOW (control_window), "Size controls");
  
  g_object_set_data (G_OBJECT (control_window),
                     "target",
                     window);
  
  g_signal_connect_object (control_window,
			   "destroy",
			   G_CALLBACK (ctk_widget_destroy),
                           window,
			   G_CONNECT_SWAPPED);

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 5);
  
  ctk_container_add (CTK_CONTAINER (control_window), vbox);
  
  label = ctk_label_new ("<no configure events>");
  ctk_box_pack_start (CTK_BOX (vbox), label, FALSE, FALSE, 0);
  
  g_signal_connect (window,
		    "configure_event",
		    G_CALLBACK (configure_event_callback),
		    label);

  adjustment = ctk_adjustment_new (10.0, -2000.0, 2000.0, 1.0, 5.0, 0.0);
  spin = ctk_spin_button_new (adjustment, 0, 0);

  ctk_box_pack_start (CTK_BOX (vbox), spin, FALSE, FALSE, 0);

  g_object_set_data (G_OBJECT (control_window), "spin1", spin);

  adjustment = ctk_adjustment_new (10.0, -2000.0, 2000.0, 1.0, 5.0, 0.0);
  spin = ctk_spin_button_new (adjustment, 0, 0);

  ctk_box_pack_start (CTK_BOX (vbox), spin, FALSE, FALSE, 0);

  g_object_set_data (G_OBJECT (control_window), "spin2", spin);

  entry = ctk_entry_new ();
  ctk_box_pack_start (CTK_BOX (vbox), entry, FALSE, FALSE, 0);

  g_signal_connect (entry, "changed",
		    G_CALLBACK (set_geometry_callback),
		    control_window);

  button = ctk_button_new_with_label ("Show gravity test windows");
  g_signal_connect_swapped (button,
			    "clicked",
			    G_CALLBACK (do_gravity_test),
			    control_window);
  ctk_box_pack_end (CTK_BOX (vbox), button, FALSE, FALSE, 0);

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  button = ctk_button_new_with_label ("Reshow with initial size");
  g_signal_connect_object (button,
			   "clicked",
			   G_CALLBACK (ctk_window_reshow_with_initial_size),
			   window,
			   G_CONNECT_SWAPPED);
  ctk_box_pack_end (CTK_BOX (vbox), button, FALSE, FALSE, 0);
  G_GNUC_END_IGNORE_DEPRECATIONS;
  
  button = ctk_button_new_with_label ("Queue resize");
  g_signal_connect_object (button,
			   "clicked",
			   G_CALLBACK (ctk_widget_queue_resize),
			   window,
			   G_CONNECT_SWAPPED);
  ctk_box_pack_end (CTK_BOX (vbox), button, FALSE, FALSE, 0);
  
  button = ctk_button_new_with_label ("Resize");
  g_signal_connect (button,
		    "clicked",
		    G_CALLBACK (set_size_callback),
		    control_window);
  ctk_box_pack_end (CTK_BOX (vbox), button, FALSE, FALSE, 0);

  button = ctk_button_new_with_label ("Set default size");
  g_signal_connect (button,
		    "clicked",
		    G_CALLBACK (set_default_size_callback),
		    control_window);
  ctk_box_pack_end (CTK_BOX (vbox), button, FALSE, FALSE, 0);

  button = ctk_button_new_with_label ("Unset default size");
  g_signal_connect (button,
		    "clicked",
		    G_CALLBACK (unset_default_size_callback),
                    control_window);
  ctk_box_pack_end (CTK_BOX (vbox), button, FALSE, FALSE, 0);
  
  button = ctk_button_new_with_label ("Set size request");
  g_signal_connect (button,
		    "clicked",
		    G_CALLBACK (set_size_request_callback),
		    control_window);
  ctk_box_pack_end (CTK_BOX (vbox), button, FALSE, FALSE, 0);

  button = ctk_button_new_with_label ("Unset size request");
  g_signal_connect (button,
		    "clicked",
		    G_CALLBACK (unset_size_request_callback),
                    control_window);
  ctk_box_pack_end (CTK_BOX (vbox), button, FALSE, FALSE, 0);
  
  button = ctk_button_new_with_label ("Move");
  g_signal_connect (button,
		    "clicked",
		    G_CALLBACK (set_location_callback),
		    control_window);
  ctk_box_pack_end (CTK_BOX (vbox), button, FALSE, FALSE, 0);

  button = ctk_button_new_with_label ("Move to current position");
  g_signal_connect (button,
		    "clicked",
		    G_CALLBACK (move_to_position_callback),
		    control_window);
  ctk_box_pack_end (CTK_BOX (vbox), button, FALSE, FALSE, 0);

  button = ctk_check_button_new_with_label ("Allow resize");
  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (button), TRUE);
  g_signal_connect (button,
		    "toggled",
		    G_CALLBACK (resizable_callback),
                    control_window);
  ctk_box_pack_end (CTK_BOX (vbox), button, FALSE, FALSE, 0);
  
  button = ctk_button_new_with_mnemonic ("_Show");
  g_signal_connect_object (button,
			   "clicked",
			   G_CALLBACK (ctk_widget_show),
			   window,
			   G_CONNECT_SWAPPED);
  ctk_box_pack_end (CTK_BOX (vbox), button, FALSE, FALSE, 0);

  button = ctk_button_new_with_mnemonic ("_Hide");
  g_signal_connect_object (button,
			   "clicked",
			   G_CALLBACK (ctk_widget_hide),
                           window,
			   G_CONNECT_SWAPPED);
  ctk_box_pack_end (CTK_BOX (vbox), button, FALSE, FALSE, 0);

  om = ctk_combo_box_text_new ();
  i = 0;
  while (i < 10)
    {
      static gchar *names[] = {
        "CDK_GRAVITY_NORTH_WEST",
        "CDK_GRAVITY_NORTH",
        "CDK_GRAVITY_NORTH_EAST",
        "CDK_GRAVITY_WEST",
        "CDK_GRAVITY_CENTER",
        "CDK_GRAVITY_EAST",
        "CDK_GRAVITY_SOUTH_WEST",
        "CDK_GRAVITY_SOUTH",
        "CDK_GRAVITY_SOUTH_EAST",
        "CDK_GRAVITY_STATIC",
        NULL
      };

      g_assert (names[i]);
      ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (om), names[i]);

      ++i;
    }
  
  g_signal_connect (om,
		    "changed",
		    G_CALLBACK (gravity_selected),
		    control_window);

  ctk_box_pack_end (CTK_BOX (vbox), om, FALSE, FALSE, 0);


  om = ctk_combo_box_text_new ();
  i = 0;
  while (i < 5)
    {
      static gchar *names[] = {
        "CTK_WIN_POS_NONE",
        "CTK_WIN_POS_CENTER",
        "CTK_WIN_POS_MOUSE",
        "CTK_WIN_POS_CENTER_ALWAYS",
        "CTK_WIN_POS_CENTER_ON_PARENT",
        NULL
      };

      g_assert (names[i]);
      ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (om), names[i]);

      ++i;
    }

  g_signal_connect (om,
		    "changed",
		    G_CALLBACK (pos_selected),
		    control_window);

  ctk_box_pack_end (CTK_BOX (vbox), om, FALSE, FALSE, 0);
  
  ctk_widget_show_all (vbox);
  
  return control_window;
}

void
create_window_sizing (CtkWidget *widget)
{
  static CtkWidget *window = NULL;
  static CtkWidget *target_window = NULL;

  if (!target_window)
    {
      CtkWidget *label;
      
      target_window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_screen (CTK_WINDOW (target_window),
			     ctk_widget_get_screen (widget));
      label = ctk_label_new (NULL);
      ctk_label_set_markup (CTK_LABEL (label), "<span foreground=\"purple\"><big>Window being resized</big></span>\nBlah blah blah blah\nblah blah blah\nblah blah blah blah blah");
      ctk_container_add (CTK_CONTAINER (target_window), label);
      ctk_widget_show (label);
      
      g_signal_connect (target_window, "destroy",
			G_CALLBACK (ctk_widget_destroyed),
			&target_window);

      window = window_controls (target_window);
      
      g_signal_connect (window, "destroy",
			G_CALLBACK (ctk_widget_destroyed),
			&window);
      
      ctk_window_set_title (CTK_WINDOW (target_window), "Window to size");
    }

  /* don't show target window by default, we want to allow testing
   * of behavior on first show.
   */
  
  if (!ctk_widget_get_visible (window))
    ctk_widget_show (window);
  else
    ctk_widget_destroy (window);
}

/*
 * CtkProgressBar
 */

typedef struct _ProgressData {
  CtkWidget *window;
  CtkWidget *pbar;
  CtkWidget *block_spin;
  CtkWidget *x_align_spin;
  CtkWidget *y_align_spin;
  CtkWidget *step_spin;
  CtkWidget *act_blocks_spin;
  CtkWidget *label;
  CtkWidget *omenu1;
  CtkWidget *elmenu;
  CtkWidget *omenu2;
  CtkWidget *entry;
  int timer;
  gboolean activity;
} ProgressData;

gboolean
progress_timeout (gpointer data)
{
  ProgressData *pdata = data;
  gdouble new_val;
  gchar *text;

  if (pdata->activity)
    {
      ctk_progress_bar_pulse (CTK_PROGRESS_BAR (pdata->pbar));

      text = g_strdup_printf ("%s", "???");
    }
  else
    {
      new_val = ctk_progress_bar_get_fraction (CTK_PROGRESS_BAR (pdata->pbar)) + 0.01;
      if (new_val > 1.00)
        new_val = 0.00;
      ctk_progress_bar_set_fraction (CTK_PROGRESS_BAR (pdata->pbar), new_val);

      text = g_strdup_printf ("%.0f%%", 100 * new_val);
    }

  ctk_label_set_text (CTK_LABEL (pdata->label), text);
  g_free (text);

  return TRUE;
}

static void
destroy_progress (CtkWidget     *widget,
		  ProgressData **pdata)
{
  if ((*pdata)->timer)
    {
      g_source_remove ((*pdata)->timer);
      (*pdata)->timer = 0;
    }
  (*pdata)->window = NULL;
  g_free (*pdata);
  *pdata = NULL;
}

static void
progressbar_toggle_orientation (CtkWidget *widget, gpointer data)
{
  ProgressData *pdata;
  gint i;

  pdata = (ProgressData *) data;

  if (!ctk_widget_get_mapped (widget))
    return;

  i = ctk_combo_box_get_active (CTK_COMBO_BOX (widget));

  if (i == 0 || i == 1)
    ctk_orientable_set_orientation (CTK_ORIENTABLE (pdata->pbar), CTK_ORIENTATION_HORIZONTAL);
  else
    ctk_orientable_set_orientation (CTK_ORIENTABLE (pdata->pbar), CTK_ORIENTATION_VERTICAL);
 
  if (i == 1 || i == 2)
    ctk_progress_bar_set_inverted (CTK_PROGRESS_BAR (pdata->pbar), TRUE);
  else
    ctk_progress_bar_set_inverted (CTK_PROGRESS_BAR (pdata->pbar), FALSE);
}

static void
toggle_show_text (CtkWidget *widget, ProgressData *pdata)
{
  gboolean active;

  active = ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (widget));
  ctk_progress_bar_set_show_text (CTK_PROGRESS_BAR (pdata->pbar), active);
}

static void
progressbar_toggle_ellipsize (CtkWidget *widget,
                              gpointer   data)
{
  ProgressData *pdata = data;
  if (ctk_widget_is_drawable (widget))
    {
      gint i = ctk_combo_box_get_active (CTK_COMBO_BOX (widget));
      ctk_progress_bar_set_ellipsize (CTK_PROGRESS_BAR (pdata->pbar), i);
    }
}

static void
toggle_activity_mode (CtkWidget *widget, ProgressData *pdata)
{
  pdata->activity = ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (widget));
}

static void
toggle_running (CtkWidget *widget, ProgressData *pdata)
{
  if (ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (widget)))
    {
      if (pdata->timer == 0)
        pdata->timer = g_timeout_add (100, (GSourceFunc)progress_timeout, pdata);
    }
  else
    {
      if (pdata->timer != 0)
        {
          g_source_remove (pdata->timer);
          pdata->timer = 0;
        }
    }
}

static void
entry_changed (CtkWidget *widget, ProgressData *pdata)
{
  ctk_progress_bar_set_text (CTK_PROGRESS_BAR (pdata->pbar),
			  ctk_entry_get_text (CTK_ENTRY (pdata->entry)));
}

void
create_progress_bar (CtkWidget *widget)
{
  CtkWidget *content_area;
  CtkWidget *vbox;
  CtkWidget *vbox2;
  CtkWidget *hbox;
  CtkWidget *check;
  CtkWidget *frame;
  CtkWidget *grid;
  CtkWidget *label;
  static ProgressData *pdata = NULL;

  static gchar *items1[] =
  {
    "Left-Right",
    "Right-Left",
    "Bottom-Top",
    "Top-Bottom"
  };

    static char *ellipsize_items[] = {
    "None",     // PANGO_ELLIPSIZE_NONE,
    "Start",    // PANGO_ELLIPSIZE_START,
    "Middle",   // PANGO_ELLIPSIZE_MIDDLE,
    "End",      // PANGO_ELLIPSIZE_END
  };
  
  if (!pdata)
    pdata = g_new0 (ProgressData, 1);

  if (!pdata->window)
    {
      pdata->window = ctk_dialog_new ();

      ctk_window_set_screen (CTK_WINDOW (pdata->window),
			     ctk_widget_get_screen (widget));

      ctk_window_set_resizable (CTK_WINDOW (pdata->window), TRUE);

      g_signal_connect (pdata->window, "destroy",
			G_CALLBACK (destroy_progress),
			&pdata);
      pdata->timer = 0;

      content_area = ctk_dialog_get_content_area (CTK_DIALOG (pdata->window));

      ctk_window_set_title (CTK_WINDOW (pdata->window), "CtkProgressBar");
      ctk_container_set_border_width (CTK_CONTAINER (pdata->window), 0);

      vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 5);
      ctk_container_set_border_width (CTK_CONTAINER (vbox), 10);
      ctk_box_pack_start (CTK_BOX (content_area), vbox, FALSE, TRUE, 0);

      frame = ctk_frame_new ("Progress");
      ctk_box_pack_start (CTK_BOX (vbox), frame, FALSE, TRUE, 0);

      vbox2 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 5);
      ctk_container_add (CTK_CONTAINER (frame), vbox2);

      pdata->pbar = ctk_progress_bar_new ();
      ctk_progress_bar_set_ellipsize (CTK_PROGRESS_BAR (pdata->pbar),
                                      PANGO_ELLIPSIZE_MIDDLE);
      ctk_widget_set_halign (pdata->pbar, CTK_ALIGN_CENTER);
      ctk_widget_set_valign (pdata->pbar, CTK_ALIGN_CENTER);
      ctk_box_pack_start (CTK_BOX (vbox2), pdata->pbar, FALSE, FALSE, 5);

      hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 5);
      ctk_widget_set_halign (hbox, CTK_ALIGN_CENTER);
      ctk_widget_set_valign (hbox, CTK_ALIGN_CENTER);
      ctk_box_pack_start (CTK_BOX (vbox2), hbox, FALSE, FALSE, 5);
      label = ctk_label_new ("Label updated by user :"); 
      ctk_box_pack_start (CTK_BOX (hbox), label, FALSE, TRUE, 0);
      pdata->label = ctk_label_new ("");
      ctk_box_pack_start (CTK_BOX (hbox), pdata->label, FALSE, TRUE, 0);

      frame = ctk_frame_new ("Options");
      ctk_box_pack_start (CTK_BOX (vbox), frame, FALSE, TRUE, 0);

      vbox2 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 5);
      ctk_container_add (CTK_CONTAINER (frame), vbox2);

      grid = ctk_grid_new ();
      ctk_grid_set_row_spacing (CTK_GRID (grid), 10);
      ctk_grid_set_column_spacing (CTK_GRID (grid), 10);
      ctk_box_pack_start (CTK_BOX (vbox2), grid, FALSE, TRUE, 0);

      label = ctk_label_new ("Orientation :");
      ctk_grid_attach (CTK_GRID (grid), label, 0, 0, 1, 1);
      ctk_widget_set_halign (label, CTK_ALIGN_START);
      ctk_widget_set_valign (label, CTK_ALIGN_CENTER);

      pdata->omenu1 = build_option_menu (items1, 4, 0,
					 progressbar_toggle_orientation,
					 pdata);
      ctk_grid_attach (CTK_GRID (grid), pdata->omenu1, 1, 0, 1, 1);
      
      check = ctk_check_button_new_with_label ("Running");
      g_signal_connect (check, "toggled",
			G_CALLBACK (toggle_running),
			pdata);
      ctk_grid_attach (CTK_GRID (grid), check, 0, 1, 2, 1);
      ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (check), TRUE);

      check = ctk_check_button_new_with_label ("Show text");
      g_signal_connect (check, "clicked",
			G_CALLBACK (toggle_show_text),
			pdata);
      ctk_grid_attach (CTK_GRID (grid), check, 0, 2, 1, 1);

      hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
      ctk_grid_attach (CTK_GRID (grid), hbox, 1, 2, 1, 1);

      label = ctk_label_new ("Text: ");
      ctk_box_pack_start (CTK_BOX (hbox), label, FALSE, TRUE, 0);

      pdata->entry = ctk_entry_new ();
      ctk_widget_set_hexpand (pdata->entry, TRUE);
      g_signal_connect (pdata->entry, "changed",
			G_CALLBACK (entry_changed),
			pdata);
      ctk_box_pack_start (CTK_BOX (hbox), pdata->entry, TRUE, TRUE, 0);
      ctk_widget_set_size_request (pdata->entry, 100, -1);

      label = ctk_label_new ("Ellipsize text :");
      ctk_grid_attach (CTK_GRID (grid), label, 0, 10, 1, 1);

      ctk_widget_set_halign (label, CTK_ALIGN_START);
      ctk_widget_set_valign (label, CTK_ALIGN_CENTER);
      pdata->elmenu = build_option_menu (ellipsize_items,
                                         sizeof (ellipsize_items) / sizeof (ellipsize_items[0]),
                                         2, // PANGO_ELLIPSIZE_MIDDLE
					 progressbar_toggle_ellipsize,
					 pdata);
      ctk_grid_attach (CTK_GRID (grid), pdata->elmenu, 1, 10, 1, 1);

      check = ctk_check_button_new_with_label ("Activity mode");
      g_signal_connect (check, "clicked",
			G_CALLBACK (toggle_activity_mode), pdata);
      ctk_grid_attach (CTK_GRID (grid), check, 0, 15, 1, 1);

      ctk_dialog_add_button (CTK_DIALOG (pdata->window), "Close", CTK_RESPONSE_CLOSE);
      g_signal_connect (pdata->window, "response",
			G_CALLBACK (ctk_widget_destroy),
			NULL);
    }

  if (!ctk_widget_get_visible (pdata->window))
    ctk_widget_show_all (pdata->window);
  else
    ctk_widget_destroy (pdata->window);
}

/*
 * Properties
 */

typedef struct {
  int x;
  int y;
  gboolean found;
  gboolean first;
  CtkWidget *res_widget;
} FindWidgetData;

static void
find_widget (CtkWidget *widget, FindWidgetData *data)
{
  CtkAllocation new_allocation;
  gint x_offset = 0;
  gint y_offset = 0;

  ctk_widget_get_allocation (widget, &new_allocation);

  if (data->found || !ctk_widget_get_mapped (widget))
    return;

  /* Note that in the following code, we only count the
   * position as being inside a WINDOW widget if it is inside
   * widget->window; points that are outside of widget->window
   * but within the allocation are not counted. This is consistent
   * with the way we highlight drag targets.
   */
  if (ctk_widget_get_has_window (widget))
    {
      new_allocation.x = 0;
      new_allocation.y = 0;
    }

  if (ctk_widget_get_parent (widget) && !data->first)
    {
      CdkWindow *window = ctk_widget_get_window (widget);
      while (window != ctk_widget_get_window (ctk_widget_get_parent (widget)))
	{
	  gint tx, ty, twidth, theight;
	  
          twidth = cdk_window_get_width (window);
          theight = cdk_window_get_height (window);

	  if (new_allocation.x < 0)
	    {
	      new_allocation.width += new_allocation.x;
	      new_allocation.x = 0;
	    }
	  if (new_allocation.y < 0)
	    {
	      new_allocation.height += new_allocation.y;
	      new_allocation.y = 0;
	    }
	  if (new_allocation.x + new_allocation.width > twidth)
	    new_allocation.width = twidth - new_allocation.x;
	  if (new_allocation.y + new_allocation.height > theight)
	    new_allocation.height = theight - new_allocation.y;

	  cdk_window_get_position (window, &tx, &ty);
	  new_allocation.x += tx;
	  x_offset += tx;
	  new_allocation.y += ty;
	  y_offset += ty;

	  window = cdk_window_get_parent (window);
	}
    }

  if ((data->x >= new_allocation.x) && (data->y >= new_allocation.y) &&
      (data->x < new_allocation.x + new_allocation.width) && 
      (data->y < new_allocation.y + new_allocation.height))
    {
      /* First, check if the drag is in a valid drop site in
       * one of our children 
       */
      if (CTK_IS_CONTAINER (widget))
	{
	  FindWidgetData new_data = *data;
	  
	  new_data.x -= x_offset;
	  new_data.y -= y_offset;
	  new_data.found = FALSE;
	  new_data.first = FALSE;
	  
	  ctk_container_forall (CTK_CONTAINER (widget),
				(CtkCallback)find_widget,
				&new_data);
	  
	  data->found = new_data.found;
	  if (data->found)
	    data->res_widget = new_data.res_widget;
	}

      /* If not, and this widget is registered as a drop site, check to
       * emit "drag_motion" to check if we are actually in
       * a drop site.
       */
      if (!data->found)
	{
	  data->found = TRUE;
	  data->res_widget = widget;
	}
    }
}

static CtkWidget *
find_widget_at_pointer (CdkDevice *device)
{
  CtkWidget *widget = NULL;
  CdkWindow *pointer_window;
  gint x, y;
  FindWidgetData data;
 
 pointer_window = cdk_device_get_window_at_position (device, NULL, NULL);
 
 if (pointer_window)
   {
     gpointer widget_ptr;

     cdk_window_get_user_data (pointer_window, &widget_ptr);
     widget = widget_ptr;
   }

 if (widget)
   {
     cdk_window_get_device_position (ctk_widget_get_window (widget),
                                     device,
			             &x, &y, NULL);
     
     data.x = x;
     data.y = y;
     data.found = FALSE;
     data.first = TRUE;

     find_widget (widget, &data);
     if (data.found)
       return data.res_widget;
     return widget;
   }
 return NULL;
}

struct SnapshotData {
  CtkWidget *toplevel_button;
  CtkWidget **window;
  CdkCursor *cursor;
  gboolean in_query;
  gboolean is_toplevel;
  gint handler;
};

static void
destroy_snapshot_data (CtkWidget             *widget,
		       struct SnapshotData *data)
{
  if (*data->window)
    *data->window = NULL;
  
  if (data->cursor)
    {
      g_object_unref (data->cursor);
      data->cursor = NULL;
    }

  if (data->handler)
    {
      g_signal_handler_disconnect (widget, data->handler);
      data->handler = 0;
    }

  g_free (data);
}

static gint
snapshot_widget_event (CtkWidget	       *widget,
		       CdkEvent	       *event,
		       struct SnapshotData *data)
{
  CtkWidget *res_widget = NULL;

  if (!data->in_query)
    return FALSE;
  
  if (event->type == CDK_BUTTON_RELEASE)
    {
      ctk_grab_remove (widget);
      cdk_seat_ungrab (cdk_event_get_seat (event));

      res_widget = find_widget_at_pointer (cdk_event_get_device (event));
      if (data->is_toplevel && res_widget)
	res_widget = ctk_widget_get_toplevel (res_widget);
      if (res_widget)
	{
	  cairo_surface_t *surface;
	  CtkWidget *window, *image;
          GdkPixbuf *pixbuf;
          int width, height;
          cairo_t *cr;

          width = ctk_widget_get_allocated_width (res_widget);
          height = ctk_widget_get_allocated_height (res_widget);

          surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);

          cr = cairo_create (surface);
          ctk_widget_draw (res_widget, cr);
          cairo_destroy (cr);

          pixbuf = gdk_pixbuf_get_from_surface (surface,
                                                0, 0,
                                                width, height);
          cairo_surface_destroy (surface);

	  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
          image = ctk_image_new_from_pixbuf (pixbuf);
          g_object_unref (pixbuf);

	  ctk_container_add (CTK_CONTAINER (window), image);
	  ctk_widget_show_all (window);
	}

      data->in_query = FALSE;
    }
  return FALSE;
}


static void
snapshot_widget (CtkButton *button,
		 struct SnapshotData *data)
{
  CtkWidget *widget = CTK_WIDGET (button);
  CdkDevice *device;

  device = ctk_get_current_event_device ();
  if (device == NULL)
    return;

  data->is_toplevel = widget == data->toplevel_button;

  if (!data->cursor)
    data->cursor = cdk_cursor_new_for_display (ctk_widget_get_display (widget),
					       CDK_TARGET);

  cdk_seat_grab (cdk_device_get_seat (device),
                 ctk_widget_get_window (widget),
                 CDK_SEAT_CAPABILITY_ALL_POINTING,
                 TRUE, data->cursor, NULL, NULL, NULL);

  g_signal_connect (button, "event",
		    G_CALLBACK (snapshot_widget_event), data);

  ctk_grab_add (widget);

  data->in_query = TRUE;
}

static void
create_snapshot (CtkWidget *widget)
{
  static CtkWidget *window = NULL;
  CtkWidget *button;
  CtkWidget *vbox;
  struct SnapshotData *data;

  data = g_new (struct SnapshotData, 1);
  data->window = &window;
  data->in_query = FALSE;
  data->cursor = NULL;
  data->handler = 0;

  if (!window)
    {
      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);

      ctk_window_set_screen (CTK_WINDOW (window),
			     ctk_widget_get_screen (widget));      

      data->handler = g_signal_connect (window, "destroy",
					G_CALLBACK (destroy_snapshot_data),
					data);

      ctk_window_set_title (CTK_WINDOW (window), "test snapshot");
      ctk_container_set_border_width (CTK_CONTAINER (window), 10);

      vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 1);
      ctk_container_add (CTK_CONTAINER (window), vbox);
            
      button = ctk_button_new_with_label ("Snapshot widget");
      ctk_box_pack_start (CTK_BOX (vbox), button, TRUE, TRUE, 0);
      g_signal_connect (button, "clicked",
			G_CALLBACK (snapshot_widget),
			data);
      
      button = ctk_button_new_with_label ("Snapshot toplevel");
      data->toplevel_button = button;
      ctk_box_pack_start (CTK_BOX (vbox), button, TRUE, TRUE, 0);
      g_signal_connect (button, "clicked",
			G_CALLBACK (snapshot_widget),
			data);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);
  
}

/*
 * Selection Test
 */

void
selection_test_received (CtkWidget        *tree_view,
                         CtkSelectionData *selection_data)
{
  CtkTreeModel *model;
  CtkListStore *store;
  CdkAtom *atoms;
  int i, l;

  if (ctk_selection_data_get_length (selection_data) < 0)
    {
      g_print ("Selection retrieval failed\n");
      return;
    }
  if (ctk_selection_data_get_data_type (selection_data) != CDK_SELECTION_TYPE_ATOM)
    {
      g_print ("Selection \"TARGETS\" was not returned as atoms!\n");
      return;
    }

  /* Clear out any current list items */

  model = ctk_tree_view_get_model (CTK_TREE_VIEW (tree_view));
  store = CTK_LIST_STORE (model);
  ctk_list_store_clear (store);

  /* Add new items to list */

  ctk_selection_data_get_targets (selection_data,
                                  &atoms, &l);

  for (i = 0; i < l; i++)
    {
      char *name;
      CtkTreeIter iter;

      name = cdk_atom_name (atoms[i]);
      if (name != NULL)
        {
          ctk_list_store_insert_with_values (store, &iter, i, 0, name, -1);
          g_free (name);
        }
      else
       ctk_list_store_insert_with_values (store, &iter, i, 0,  "(bad atom)", -1);
    }

  return;
}

void
selection_test_get_targets (CtkWidget *dialog, gint response, CtkWidget *tree_view)
{
  static CdkAtom targets_atom = CDK_NONE;

  if (response != CTK_RESPONSE_APPLY)
    {
      ctk_widget_destroy (dialog);
      return;
    }

  if (targets_atom == CDK_NONE)
    targets_atom = cdk_atom_intern ("TARGETS", FALSE);

  ctk_selection_convert (tree_view, CDK_SELECTION_PRIMARY, targets_atom,
			 CDK_CURRENT_TIME);
}

void
create_selection_test (CtkWidget *widget)
{
  static CtkWidget *window = NULL;
  CtkWidget *content_area;
  CtkWidget *vbox;
  CtkWidget *scrolled_win;
  CtkListStore* store;
  CtkWidget *tree_view;
  CtkTreeViewColumn *column;
  CtkCellRenderer *renderer;
  CtkWidget *label;

  if (!window)
    {
      window = ctk_dialog_new ();
      
      ctk_window_set_screen (CTK_WINDOW (window),
			     ctk_widget_get_screen (widget));

      g_signal_connect (window, "destroy",
			G_CALLBACK (ctk_widget_destroyed),
			&window);

      content_area = ctk_dialog_get_content_area (CTK_DIALOG (window));

      ctk_window_set_title (CTK_WINDOW (window), "Selection Test");
      ctk_container_set_border_width (CTK_CONTAINER (window), 0);

      /* Create the list */

      vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 5);
      ctk_container_set_border_width (CTK_CONTAINER (vbox), 10);
      ctk_box_pack_start (CTK_BOX (content_area), vbox, TRUE, TRUE, 0);

      label = ctk_label_new ("Gets available targets for current selection");
      ctk_box_pack_start (CTK_BOX (vbox), label, FALSE, FALSE, 0);

      scrolled_win = ctk_scrolled_window_new (NULL, NULL);
      ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (scrolled_win),
				      CTK_POLICY_AUTOMATIC, 
				      CTK_POLICY_AUTOMATIC);
      ctk_box_pack_start (CTK_BOX (vbox), scrolled_win, TRUE, TRUE, 0);
      ctk_widget_set_size_request (scrolled_win, 100, 200);

      store = ctk_list_store_new (1, G_TYPE_STRING);
      tree_view = ctk_tree_view_new_with_model (CTK_TREE_MODEL (store));
      ctk_container_add (CTK_CONTAINER (scrolled_win), tree_view);

      renderer = ctk_cell_renderer_text_new ();
      column = ctk_tree_view_column_new_with_attributes ("Target", renderer,
                                                         "text", 0, NULL);
      ctk_tree_view_append_column (CTK_TREE_VIEW (tree_view), column);

      g_signal_connect (tree_view, "selection_received",
			G_CALLBACK (selection_test_received), NULL);

      /* .. And create some buttons */
      ctk_dialog_add_button (CTK_DIALOG (window),
                             "Get Targets",
                             CTK_RESPONSE_APPLY);

      g_signal_connect (window, "response",
			G_CALLBACK (selection_test_get_targets), tree_view);

      ctk_dialog_add_button (CTK_DIALOG (window),
                             "Quit",
                             CTK_RESPONSE_CLOSE);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);
}

/*
 * Test scrolling
 */

static int scroll_test_pos = 0.0;

static gint
scroll_test_draw (CtkWidget     *widget,
                  cairo_t       *cr,
                  CtkAdjustment *adjustment)
{
  gint i,j;
  gint imin, imax, jmin, jmax;
  CdkRectangle clip;
  
  cdk_cairo_get_clip_rectangle (cr, &clip);

  imin = (clip.x) / 10;
  imax = (clip.x + clip.width + 9) / 10;

  jmin = ((int)ctk_adjustment_get_value (adjustment) + clip.y) / 10;
  jmax = ((int)ctk_adjustment_get_value (adjustment) + clip.y + clip.height + 9) / 10;

  for (i=imin; i<imax; i++)
    for (j=jmin; j<jmax; j++)
      if ((i+j) % 2)
	cairo_rectangle (cr, 10*i, 10*j - (int)ctk_adjustment_get_value (adjustment), 1+i%10, 1+j%10);

  cairo_fill (cr);

  return TRUE;
}

static gint
scroll_test_scroll (CtkWidget *widget, CdkEventScroll *event,
		    CtkAdjustment *adjustment)
{
  gdouble new_value = ctk_adjustment_get_value (adjustment) + ((event->direction == CDK_SCROLL_UP) ?
				    -ctk_adjustment_get_page_increment (adjustment) / 2:
				    ctk_adjustment_get_page_increment (adjustment) / 2);
  new_value = CLAMP (new_value, ctk_adjustment_get_lower (adjustment), ctk_adjustment_get_upper (adjustment) - ctk_adjustment_get_page_size (adjustment));
  ctk_adjustment_set_value (adjustment, new_value);  
  
  return TRUE;
}

static void
scroll_test_configure (CtkWidget *widget, CdkEventConfigure *event,
		       CtkAdjustment *adjustment)
{
  CtkAllocation allocation;

  ctk_widget_get_allocation (widget, &allocation);
  ctk_adjustment_configure (adjustment,
                            ctk_adjustment_get_value (adjustment),
                            ctk_adjustment_get_lower (adjustment),
                            ctk_adjustment_get_upper (adjustment),
                            0.1 * allocation.height,
                            0.9 * allocation.height,
                            allocation.height);
}

static void
scroll_test_adjustment_changed (CtkAdjustment *adjustment, CtkWidget *widget)
{
  CdkWindow *window;
  gint dy;

  dy = scroll_test_pos - (int)ctk_adjustment_get_value (adjustment);
  scroll_test_pos = ctk_adjustment_get_value (adjustment);

  if (!ctk_widget_is_drawable (widget))
    return;

  window = ctk_widget_get_window (widget);
  cdk_window_scroll (window, 0, dy);
  cdk_window_process_updates (window, FALSE);
}


void
create_scroll_test (CtkWidget *widget)
{
  static CtkWidget *window = NULL;
  CtkWidget *content_area;
  CtkWidget *hbox;
  CtkWidget *drawing_area;
  CtkWidget *scrollbar;
  CtkAdjustment *adjustment;
  CdkGeometry geometry;
  CdkWindowHints geometry_mask;

  if (!window)
    {
      window = ctk_dialog_new ();

      ctk_window_set_screen (CTK_WINDOW (window),
			     ctk_widget_get_screen (widget));

      g_signal_connect (window, "destroy",
			G_CALLBACK (ctk_widget_destroyed),
			&window);

      content_area = ctk_dialog_get_content_area (CTK_DIALOG (window));

      ctk_window_set_title (CTK_WINDOW (window), "Scroll Test");
      ctk_container_set_border_width (CTK_CONTAINER (window), 0);

      hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
      ctk_box_pack_start (CTK_BOX (content_area), hbox, TRUE, TRUE, 0);
      ctk_widget_show (hbox);

      drawing_area = ctk_drawing_area_new ();
      ctk_widget_set_size_request (drawing_area, 200, 200);
      ctk_box_pack_start (CTK_BOX (hbox), drawing_area, TRUE, TRUE, 0);
      ctk_widget_show (drawing_area);

      ctk_widget_set_events (drawing_area, CDK_EXPOSURE_MASK | CDK_SCROLL_MASK);

      adjustment = ctk_adjustment_new (0.0, 0.0, 1000.0, 1.0, 180.0, 200.0);
      scroll_test_pos = 0.0;

      scrollbar = ctk_scrollbar_new (CTK_ORIENTATION_VERTICAL, adjustment);
      ctk_box_pack_start (CTK_BOX (hbox), scrollbar, FALSE, FALSE, 0);
      ctk_widget_show (scrollbar);

      g_signal_connect (drawing_area, "draw",
			G_CALLBACK (scroll_test_draw), adjustment);
      g_signal_connect (drawing_area, "configure_event",
			G_CALLBACK (scroll_test_configure), adjustment);
      g_signal_connect (drawing_area, "scroll_event",
			G_CALLBACK (scroll_test_scroll), adjustment);
      
      g_signal_connect (adjustment, "value_changed",
			G_CALLBACK (scroll_test_adjustment_changed),
			drawing_area);
      
      /* .. And create some buttons */

      ctk_dialog_add_button (CTK_DIALOG (window),
                             "Quit",
                             CTK_RESPONSE_CLOSE);
      g_signal_connect_swapped (window, "response",
				G_CALLBACK (ctk_widget_destroy),
				window);

      /* Set up gridded geometry */

      geometry_mask = CDK_HINT_MIN_SIZE | 
	               CDK_HINT_BASE_SIZE | 
	               CDK_HINT_RESIZE_INC;

      geometry.min_width = 20;
      geometry.min_height = 20;
      geometry.base_width = 0;
      geometry.base_height = 0;
      geometry.width_inc = 10;
      geometry.height_inc = 10;
      
      ctk_window_set_geometry_hints (CTK_WINDOW (window),
			       drawing_area, &geometry, geometry_mask);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show (window);
  else
    ctk_widget_destroy (window);
}

/*
 * Timeout Test
 */

static int timer = 0;

gint
timeout_test (CtkWidget *label)
{
  static int count = 0;
  static char buffer[32];

  sprintf (buffer, "count: %d", ++count);
  ctk_label_set_text (CTK_LABEL (label), buffer);

  return TRUE;
}

void
start_timeout_test (CtkWidget *widget,
		    CtkWidget *label)
{
  if (!timer)
    {
      timer = g_timeout_add (100, (GSourceFunc)timeout_test, label);
    }
}

void
stop_timeout_test (CtkWidget *widget,
		   gpointer   data)
{
  if (timer)
    {
      g_source_remove (timer);
      timer = 0;
    }
}

void
destroy_timeout_test (CtkWidget  *widget,
		      CtkWidget **window)
{
  stop_timeout_test (NULL, NULL);

  *window = NULL;
}

void
create_timeout_test (CtkWidget *widget)
{
  static CtkWidget *window = NULL;
  CtkWidget *action_area, *content_area;
  CtkWidget *button;
  CtkWidget *label;

  if (!window)
    {
      window = ctk_dialog_new ();

      ctk_window_set_screen (CTK_WINDOW (window),
			     ctk_widget_get_screen (widget));

      g_signal_connect (window, "destroy",
			G_CALLBACK (destroy_timeout_test),
			&window);

      content_area = ctk_dialog_get_content_area (CTK_DIALOG (window));
      action_area = ctk_dialog_get_content_area (CTK_DIALOG (window));

      ctk_window_set_title (CTK_WINDOW (window), "Timeout Test");
      ctk_container_set_border_width (CTK_CONTAINER (window), 0);

      label = ctk_label_new ("count: 0");
      g_object_set (label, "margin", 10, NULL);
      ctk_box_pack_start (CTK_BOX (content_area), label, TRUE, TRUE, 0);
      ctk_widget_show (label);

      button = ctk_button_new_with_label ("close");
      g_signal_connect_swapped (button, "clicked",
				G_CALLBACK (ctk_widget_destroy),
				window);
      ctk_widget_set_can_default (button, TRUE);
      ctk_box_pack_start (CTK_BOX (action_area), button, TRUE, TRUE, 0);
      ctk_widget_grab_default (button);
      ctk_widget_show (button);

      button = ctk_button_new_with_label ("start");
      g_signal_connect (button, "clicked",
			G_CALLBACK(start_timeout_test),
			label);
      ctk_widget_set_can_default (button, TRUE);
      ctk_box_pack_start (CTK_BOX (action_area), button, TRUE, TRUE, 0);
      ctk_widget_show (button);

      button = ctk_button_new_with_label ("stop");
      g_signal_connect (button, "clicked",
			G_CALLBACK (stop_timeout_test),
			NULL);
      ctk_widget_set_can_default (button, TRUE);
      ctk_box_pack_start (CTK_BOX (action_area), button, TRUE, TRUE, 0);
      ctk_widget_show (button);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show (window);
  else
    ctk_widget_destroy (window);
}

/*
 * Test of recursive mainloop
 */

void
mainloop_destroyed (CtkWidget *w, CtkWidget **window)
{
  *window = NULL;
  ctk_main_quit ();
}

void
create_mainloop (CtkWidget *widget)
{
  static CtkWidget *window = NULL;
  CtkWidget *content_area;
  CtkWidget *label;

  if (!window)
    {
      window = ctk_dialog_new ();

      ctk_window_set_screen (CTK_WINDOW (window),
			     ctk_widget_get_screen (widget));

      ctk_window_set_title (CTK_WINDOW (window), "Test Main Loop");

      g_signal_connect (window, "destroy",
			G_CALLBACK (mainloop_destroyed),
			&window);

      content_area = ctk_dialog_get_content_area (CTK_DIALOG (window));

      label = ctk_label_new ("In recursive main loop...");
      g_object_set (label, "margin", 20, NULL);

      ctk_box_pack_start (CTK_BOX (content_area), label, TRUE, TRUE, 0);
      ctk_widget_show (label);

      ctk_dialog_add_button (CTK_DIALOG (window),
                             "Leave",
                             CTK_RESPONSE_OK);
      g_signal_connect_swapped (window, "response",
				G_CALLBACK (ctk_widget_destroy),
				window);
    }

  if (!ctk_widget_get_visible (window))
    {
      ctk_widget_show (window);

      g_print ("create_mainloop: start\n");
      ctk_main ();
      g_print ("create_mainloop: done\n");
    }
  else
    ctk_widget_destroy (window);
}

static gboolean
layout_draw_handler (CtkWidget *widget, cairo_t *cr)
{
  CtkLayout *layout;
  CdkWindow *bin_window;
  CdkRectangle clip;
  gint i,j,x,y;
  gint imin, imax, jmin, jmax;

  layout = CTK_LAYOUT (widget);
  bin_window = ctk_layout_get_bin_window (layout);

  if (!ctk_cairo_should_draw_window (cr, bin_window))
    return FALSE;
  
  cdk_window_get_position (bin_window, &x, &y);
  cairo_translate (cr, x, y);

  cdk_cairo_get_clip_rectangle (cr, &clip);

  imin = (clip.x) / 10;
  imax = (clip.x + clip.width + 9) / 10;

  jmin = (clip.y) / 10;
  jmax = (clip.y + clip.height + 9) / 10;

  for (i=imin; i<imax; i++)
    for (j=jmin; j<jmax; j++)
      if ((i+j) % 2)
	cairo_rectangle (cr,
			 10*i, 10*j, 
			 1+i%10, 1+j%10);
  
  cairo_fill (cr);

  return FALSE;
}

void create_layout (CtkWidget *widget)
{
  CtkAdjustment *hadjustment, *vadjustment;
  CtkLayout *layout;
  static CtkWidget *window = NULL;
  CtkWidget *layout_widget;
  CtkWidget *scrolledwindow;
  CtkWidget *button;

  if (!window)
    {
      gchar buf[16];

      gint i, j;
      
      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_screen (CTK_WINDOW (window),
			     ctk_widget_get_screen (widget));

      g_signal_connect (window, "destroy",
			G_CALLBACK (ctk_widget_destroyed),
			&window);

      ctk_window_set_title (CTK_WINDOW (window), "Layout");
      ctk_widget_set_size_request (window, 200, 200);

      scrolledwindow = ctk_scrolled_window_new (NULL, NULL);
      ctk_scrolled_window_set_shadow_type (CTK_SCROLLED_WINDOW (scrolledwindow),
					   CTK_SHADOW_IN);
      ctk_scrolled_window_set_placement (CTK_SCROLLED_WINDOW (scrolledwindow),
					 CTK_CORNER_TOP_RIGHT);

      ctk_container_add (CTK_CONTAINER (window), scrolledwindow);

      layout_widget = ctk_layout_new (NULL, NULL);
      layout = CTK_LAYOUT (layout_widget);
      ctk_container_add (CTK_CONTAINER (scrolledwindow), layout_widget);

      /* We set step sizes here since CtkLayout does not set
       * them itself.
       */
      hadjustment = ctk_scrollable_get_hadjustment (CTK_SCROLLABLE (layout));
      vadjustment = ctk_scrollable_get_vadjustment (CTK_SCROLLABLE (layout));
      ctk_adjustment_set_step_increment (hadjustment, 10.0);
      ctk_adjustment_set_step_increment (vadjustment, 10.0);
      ctk_scrollable_set_hadjustment (CTK_SCROLLABLE (layout), hadjustment);
      ctk_scrollable_set_vadjustment (CTK_SCROLLABLE (layout), vadjustment);

      ctk_widget_set_events (layout_widget, CDK_EXPOSURE_MASK);
      g_signal_connect (layout, "draw",
			G_CALLBACK (layout_draw_handler), NULL);

      ctk_layout_set_size (layout, 1600, 128000);

      for (i=0 ; i < 16 ; i++)
	for (j=0 ; j < 16 ; j++)
	  {
	    sprintf(buf, "Button %d, %d", i, j);
	    if ((i + j) % 2)
	      button = ctk_button_new_with_label (buf);
	    else
	      button = ctk_label_new (buf);

	    ctk_layout_put (layout, button, j*100, i*100);
	  }

      for (i=16; i < 1280; i++)
	{
	  sprintf(buf, "Button %d, %d", i, 0);
	  if (i % 2)
	    button = ctk_button_new_with_label (buf);
	  else
	    button = ctk_label_new (buf);

	  ctk_layout_put (layout, button, 0, i*100);
	}
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);
}

static void
show_native (CtkWidget *button,
             CtkFileChooserNative *native)
{
  ctk_native_dialog_show (CTK_NATIVE_DIALOG (native));
}

static void
hide_native (CtkWidget *button,
             CtkFileChooserNative *native)
{
  ctk_native_dialog_hide (CTK_NATIVE_DIALOG (native));
}

static void
native_response (CtkNativeDialog *self,
                 gint response_id,
                 CtkWidget *label)
{
  static int count = 0;
  char *res;
  GSList *uris, *l;
  GString *s;
  char *response;
  CtkFileFilter *filter;

  uris = ctk_file_chooser_get_uris (CTK_FILE_CHOOSER (self));
  filter = ctk_file_chooser_get_filter (CTK_FILE_CHOOSER (self));
  s = g_string_new ("");
  for (l = uris; l != NULL; l = l->next)
    {
      g_string_prepend (s, l->data);
      g_string_prepend (s, "\n");
    }

  switch (response_id)
    {
    case CTK_RESPONSE_NONE:
      response = g_strdup ("CTK_RESPONSE_NONE");
      break;
    case CTK_RESPONSE_ACCEPT:
      response = g_strdup ("CTK_RESPONSE_ACCEPT");
      break;
    case CTK_RESPONSE_CANCEL:
      response = g_strdup ("CTK_RESPONSE_CANCEL");
      break;
    case CTK_RESPONSE_DELETE_EVENT:
      response = g_strdup ("CTK_RESPONSE_DELETE_EVENT");
      break;
    default:
      response = g_strdup_printf ("%d", response_id);
      break;
    }

  if (filter)
    res = g_strdup_printf ("Response #%d: %s\n"
                           "Filter: %s\n"
                           "Files:\n"
                           "%s",
                           ++count,
                           response,
                           ctk_file_filter_get_name (filter),
                           s->str);
  else
    res = g_strdup_printf ("Response #%d: %s\n"
                           "NO Filter\n"
                           "Files:\n"
                           "%s",
                           ++count,
                           response,
                           s->str);
  ctk_label_set_text (CTK_LABEL (label), res);
  g_free (response);
  g_string_free (s, TRUE);
}

static void
native_modal_toggle (CtkWidget *checkbutton,
                     CtkFileChooserNative *native)
{
  ctk_native_dialog_set_modal (CTK_NATIVE_DIALOG (native),
                               ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON(checkbutton)));
}

static void
native_multi_select_toggle (CtkWidget *checkbutton,
                            CtkFileChooserNative *native)
{
  ctk_file_chooser_set_select_multiple (CTK_FILE_CHOOSER (native),
                                        ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON(checkbutton)));
}

static void
native_overwrite_confirmation_toggle (CtkWidget *checkbutton,
                                      CtkFileChooserNative *native)
{
  ctk_file_chooser_set_do_overwrite_confirmation (CTK_FILE_CHOOSER (native),
                                                  ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON(checkbutton)));
}

static void
native_extra_widget_toggle (CtkWidget *checkbutton,
                            CtkFileChooserNative *native)
{
  gboolean extra_widget = ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON(checkbutton));

  if (extra_widget)
    {
      CtkWidget *extra = ctk_check_button_new_with_label ("Extra toggle");
      ctk_widget_show (extra);
      ctk_file_chooser_set_extra_widget (CTK_FILE_CHOOSER (native), extra);
    }
  else
    {
      ctk_file_chooser_set_extra_widget (CTK_FILE_CHOOSER (native), NULL);
    }
}


static void
native_visible_notify_show (GObject	*object,
                            GParamSpec	*pspec,
                            CtkWidget   *show_button)
{
  CtkFileChooserNative *native = CTK_FILE_CHOOSER_NATIVE (object);
  gboolean visible;

  visible = ctk_native_dialog_get_visible (CTK_NATIVE_DIALOG (native));
  ctk_widget_set_sensitive (show_button, !visible);
}

static void
native_visible_notify_hide (GObject	*object,
                            GParamSpec	*pspec,
                            CtkWidget   *hide_button)
{
  CtkFileChooserNative *native = CTK_FILE_CHOOSER_NATIVE (object);
  gboolean visible;

  visible = ctk_native_dialog_get_visible (CTK_NATIVE_DIALOG (native));
  ctk_widget_set_sensitive (hide_button, visible);
}

static char *
get_some_file (void)
{
  GFile *dir = g_file_new_for_path (g_get_current_dir ());
  GFileEnumerator *e;
  char *res = NULL;

  e = g_file_enumerate_children (dir, "*", 0, NULL, NULL);
  if (e)
    {
      GFileInfo *info;

      while (res == NULL)
        {
          info = g_file_enumerator_next_file (e, NULL, NULL);
          if (info)
            {
              if (g_file_info_get_file_type (info) == G_FILE_TYPE_REGULAR)
                {
                  GFile *child = g_file_enumerator_get_child (e, info);
                  res = g_file_get_path (child);
                  g_object_unref (child);
                }
              g_object_unref (info);
            }
          else
            break;
        }
    }

  return res;
}

static void
native_action_changed (CtkWidget *combo,
                       CtkFileChooserNative *native)
{
  int i;
  gboolean save_as = FALSE;
  i = ctk_combo_box_get_active (CTK_COMBO_BOX (combo));

  if (i == 4) /* Save as */
    {
      save_as = TRUE;
      i = CTK_FILE_CHOOSER_ACTION_SAVE;
    }

  ctk_file_chooser_set_action (CTK_FILE_CHOOSER (native),
                               (CtkFileChooserAction) i);


  if (i == CTK_FILE_CHOOSER_ACTION_SAVE ||
      i == CTK_FILE_CHOOSER_ACTION_CREATE_FOLDER)
    {
      if (save_as)
        {
          char *file = get_some_file ();
          ctk_file_chooser_set_filename (CTK_FILE_CHOOSER (native), file);
          g_free (file);
        }
      else
        ctk_file_chooser_set_current_name (CTK_FILE_CHOOSER (native), "newname.txt");
    }
}

static void
native_filter_changed (CtkWidget *combo,
                       CtkFileChooserNative *native)
{
  int i;
  GSList *filters, *l;
  CtkFileFilter *filter;

  i = ctk_combo_box_get_active (CTK_COMBO_BOX (combo));

  filters = ctk_file_chooser_list_filters (CTK_FILE_CHOOSER (native));
  for (l = filters; l != NULL; l = l->next)
    ctk_file_chooser_remove_filter (CTK_FILE_CHOOSER (native), l->data);
  g_slist_free (filters);

  switch (i)
    {
    case 0:
      break;
    case 1:   /* pattern */
      filter = ctk_file_filter_new ();
      ctk_file_filter_set_name (filter, "Text");
      ctk_file_filter_add_pattern (filter, "*.doc");
      ctk_file_filter_add_pattern (filter, "*.txt");
      ctk_file_chooser_add_filter (CTK_FILE_CHOOSER (native), filter);

      filter = ctk_file_filter_new ();
      ctk_file_filter_set_name (filter, "Images");
      ctk_file_filter_add_pixbuf_formats (filter);
      ctk_file_chooser_add_filter (CTK_FILE_CHOOSER (native), filter);
      ctk_file_chooser_set_filter (CTK_FILE_CHOOSER (native), filter);

      filter = ctk_file_filter_new ();
      ctk_file_filter_set_name (filter, "All");
      ctk_file_filter_add_pattern (filter, "*");
      ctk_file_chooser_add_filter (CTK_FILE_CHOOSER (native), filter);
      break;

    case 2:   /* mimetype */
      filter = ctk_file_filter_new ();
      ctk_file_filter_set_name (filter, "Text");
      ctk_file_filter_add_mime_type (filter, "text/plain");
      ctk_file_chooser_add_filter (CTK_FILE_CHOOSER (native), filter);

      filter = ctk_file_filter_new ();
      ctk_file_filter_set_name (filter, "All");
      ctk_file_filter_add_pattern (filter, "*");
      ctk_file_chooser_add_filter (CTK_FILE_CHOOSER (native), filter);
      ctk_file_chooser_set_filter (CTK_FILE_CHOOSER (native), filter);
      break;
    }
}

static void
destroy_native (CtkFileChooserNative *native)
{
  ctk_native_dialog_destroy (CTK_NATIVE_DIALOG (native));
  g_object_unref (native);
}

void
create_native_dialogs (CtkWidget *widget)
{
  static CtkWidget *window = NULL;
  CtkWidget *box, *label;
  CtkWidget *show_button, *hide_button, *check_button;
  CtkFileChooserNative *native;
  CtkWidget *combo;

  if (!window)
    {
      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_screen (CTK_WINDOW (window),
                             ctk_widget_get_screen (widget));

      native = ctk_file_chooser_native_new ("Native title",
                                            CTK_WINDOW (window),
                                            CTK_FILE_CHOOSER_ACTION_OPEN,
                                            "_accept&native",
                                            "_cancel__native");

      g_signal_connect_swapped (G_OBJECT (window), "destroy", G_CALLBACK (destroy_native), native);

      ctk_file_chooser_add_shortcut_folder (CTK_FILE_CHOOSER (native),
                                            g_get_current_dir (),
                                            NULL);

      ctk_window_set_title (CTK_WINDOW(window), "Native dialog parent");

      box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 5);
      ctk_container_add (CTK_CONTAINER (window), box);

      label = ctk_label_new ("");
      ctk_box_pack_start (CTK_BOX (box), label, FALSE, FALSE, 4);

      combo = ctk_combo_box_text_new ();

      ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo), "Open");
      ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo), "Save");
      ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo), "Select Folder");
      ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo), "Create Folder");
      ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo), "Save as");

      g_signal_connect (combo, "changed",
                        G_CALLBACK (native_action_changed), native);
      ctk_combo_box_set_active (CTK_COMBO_BOX (combo), CTK_FILE_CHOOSER_ACTION_OPEN);
      ctk_box_pack_start (CTK_BOX (box), combo, FALSE, FALSE, 4);

      combo = ctk_combo_box_text_new ();

      ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo), "No filters");
      ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo), "Pattern filter");
      ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo), "Mimetype filter");

      g_signal_connect (combo, "changed",
                        G_CALLBACK (native_filter_changed), native);
      ctk_combo_box_set_active (CTK_COMBO_BOX (combo), 0);
      ctk_box_pack_start (CTK_BOX (box), combo, FALSE, FALSE, 4);

      check_button = ctk_check_button_new_with_label ("Modal");
      g_signal_connect (check_button, "toggled",
                        G_CALLBACK (native_modal_toggle), native);
      ctk_box_pack_start (CTK_BOX (box), check_button, FALSE, FALSE, 4);

      check_button = ctk_check_button_new_with_label ("Multiple select");
      g_signal_connect (check_button, "toggled",
                        G_CALLBACK (native_multi_select_toggle), native);
      ctk_box_pack_start (CTK_BOX (box), check_button, FALSE, FALSE, 4);

      check_button = ctk_check_button_new_with_label ("Confirm overwrite");
      g_signal_connect (check_button, "toggled",
                        G_CALLBACK (native_overwrite_confirmation_toggle), native);
      ctk_box_pack_start (CTK_BOX (box), check_button, FALSE, FALSE, 4);

      check_button = ctk_check_button_new_with_label ("Extra widget");
      g_signal_connect (check_button, "toggled",
                        G_CALLBACK (native_extra_widget_toggle), native);
      ctk_box_pack_start (CTK_BOX (box), check_button, FALSE, FALSE, 4);

      show_button = ctk_button_new_with_label ("Show");
      hide_button = ctk_button_new_with_label ("Hide");
      ctk_widget_set_sensitive (hide_button, FALSE);

      ctk_box_pack_start (CTK_BOX (box), show_button, FALSE, FALSE, 4);
      ctk_box_pack_start (CTK_BOX (box), hide_button, FALSE, FALSE, 4);

      /* connect signals */
      g_signal_connect (native, "response",
                        G_CALLBACK (native_response), label);
      g_signal_connect (show_button, "clicked",
                        G_CALLBACK (show_native), native);
      g_signal_connect (hide_button, "clicked",
                        G_CALLBACK (hide_native), native);

      g_signal_connect (native, "notify::visible",
                        G_CALLBACK (native_visible_notify_show), show_button);
      g_signal_connect (native, "notify::visible",
                        G_CALLBACK (native_visible_notify_hide), hide_button);

      g_signal_connect (window, "destroy",
                        G_CALLBACK (ctk_widget_destroyed),
                        &window);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);
}

/*
 * Main Window and Exit
 */

void
do_exit (CtkWidget *widget, CtkWidget *window)
{
  ctk_widget_destroy (window);
  ctk_main_quit ();
}

struct {
  char *label;
  void (*func) (CtkWidget *widget);
  gboolean do_not_benchmark;
} buttons[] =
{
  { "alpha window", create_alpha_window },
  { "alpha widget", create_alpha_widgets },
  { "big windows", create_big_windows },
  { "button box", create_button_box },
  { "buttons", create_buttons },
  { "check buttons", create_check_buttons },
  { "color selection", create_color_selection },
  { "composited window", create_composited_window },
  { "cursors", create_cursors },
  { "dialog", create_dialog },
  { "display", create_display_screen, TRUE },
  { "entry", create_entry },
  { "event box", create_event_box },
  { "event watcher", create_event_watcher },
  { "expander", create_expander },
  { "flipping", create_flipping },
  { "focus", create_focus },
  { "font selection", create_font_selection },
  { "image", create_image },
  { "key lookup", create_key_lookup },
  { "labels", create_labels },
  { "layout", create_layout },
  { "listbox", create_listbox },
  { "menus", create_menus },
  { "message dialog", create_message_dialog },
  { "modal window", create_modal_window, TRUE },
  { "native dialogs", create_native_dialogs },
  { "notebook", create_notebook },
  { "panes", create_panes },
  { "paned keyboard", create_paned_keyboard_navigation },
  { "pixbuf", create_pixbuf },
  { "progress bar", create_progress_bar },
  { "radio buttons", create_radio_buttons },
  { "range controls", create_range_controls },
  { "reparent", create_reparent },
  { "resize grips", create_resize_grips },
  { "rotated label", create_rotated_label },
  { "rotated text", create_rotated_text },
  { "saved position", create_saved_position },
  { "scrolled windows", create_scrolled_windows },
  { "shapes", create_shapes },
  { "size groups", create_size_groups },
  { "snapshot", create_snapshot },
  { "spinbutton", create_spins },
  { "statusbar", create_statusbar },
  { "test mainloop", create_mainloop, TRUE },
  { "test scrolling", create_scroll_test },
  { "test selection", create_selection_test },
  { "test timeout", create_timeout_test },
  { "toggle buttons", create_toggle_buttons },
  { "toolbar", create_toolbar },
  { "tooltips", create_tooltips },
  { "WM hints", create_wmhints },
  { "window sizing", create_window_sizing },
  { "window states", create_window_states }
};
int nbuttons = sizeof (buttons) / sizeof (buttons[0]);

void
create_main_window (void)
{
  CtkWidget *window;
  CtkWidget *box1;
  CtkWidget *box2;
  CtkWidget *scrolled_window;
  CtkWidget *button;
  CtkWidget *label;
  gchar buffer[64];
  CtkWidget *separator;
  CdkGeometry geometry;
  int i;

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_widget_set_name (window, "main_window");
  ctk_window_move (CTK_WINDOW (window), 50, 20);
  ctk_window_set_default_size (CTK_WINDOW (window), -1, 400);

  geometry.min_width = -1;
  geometry.min_height = -1;
  geometry.max_width = -1;
  geometry.max_height = G_MAXSHORT;
  ctk_window_set_geometry_hints (CTK_WINDOW (window), NULL,
				 &geometry,
				 CDK_HINT_MIN_SIZE | CDK_HINT_MAX_SIZE);

  g_signal_connect (window, "destroy",
		    G_CALLBACK (ctk_main_quit),
		    NULL);
  g_signal_connect (window, "delete-event",
		    G_CALLBACK (ctk_false),
		    NULL);

  box1 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  ctk_container_add (CTK_CONTAINER (window), box1);

  if (ctk_micro_version > 0)
    sprintf (buffer,
	     "Ctk+ v%d.%d.%d",
	     ctk_get_major_version (),
	     ctk_get_minor_version (),
	     ctk_get_micro_version ());
  else
    sprintf (buffer,
	     "Ctk+ v%d.%d",
	     ctk_get_major_version (),
	     ctk_get_minor_version ());

  label = ctk_label_new (buffer);
  ctk_box_pack_start (CTK_BOX (box1), label, FALSE, FALSE, 0);
  ctk_widget_set_name (label, "testctk-version-label");

  scrolled_window = ctk_scrolled_window_new (NULL, NULL);
  ctk_container_set_border_width (CTK_CONTAINER (scrolled_window), 10);
  ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (scrolled_window),
     		                  CTK_POLICY_NEVER, 
                                  CTK_POLICY_AUTOMATIC);
  ctk_box_pack_start (CTK_BOX (box1), scrolled_window, TRUE, TRUE, 0);

  box2 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  ctk_container_set_border_width (CTK_CONTAINER (box2), 10);
  ctk_container_add (CTK_CONTAINER (scrolled_window), box2);
  ctk_container_set_focus_vadjustment (CTK_CONTAINER (box2),
				       ctk_scrolled_window_get_vadjustment (CTK_SCROLLED_WINDOW (scrolled_window)));
  ctk_widget_show (box2);

  for (i = 0; i < nbuttons; i++)
    {
      button = ctk_button_new_with_label (buttons[i].label);
      if (buttons[i].func)
        g_signal_connect (button, 
			  "clicked", 
			  G_CALLBACK(buttons[i].func),
			  NULL);
      else
        ctk_widget_set_sensitive (button, FALSE);
      ctk_box_pack_start (CTK_BOX (box2), button, TRUE, TRUE, 0);
    }

  separator = ctk_separator_new (CTK_ORIENTATION_HORIZONTAL);
  ctk_box_pack_start (CTK_BOX (box1), separator, FALSE, TRUE, 0);

  box2 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 10);
  ctk_container_set_border_width (CTK_CONTAINER (box2), 10);
  ctk_box_pack_start (CTK_BOX (box1), box2, FALSE, TRUE, 0);

  button = ctk_button_new_with_mnemonic ("_Close");
  g_signal_connect (button, "clicked",
		    G_CALLBACK (do_exit),
		    window);
  ctk_box_pack_start (CTK_BOX (box2), button, TRUE, TRUE, 0);
  ctk_widget_set_can_default (button, TRUE);
  ctk_widget_grab_default (button);

  ctk_widget_show_all (window);
}

static void
test_init (void)
{
  if (g_file_test ("../modules/input/immodules.cache", G_FILE_TEST_EXISTS))
    g_setenv ("CTK_IM_MODULE_FILE", "../modules/input/immodules.cache", TRUE);
}

static char *
pad (const char *str, int to)
{
  static char buf[256];
  int len = strlen (str);
  int i;

  for (i = 0; i < to; i++)
    buf[i] = ' ';

  buf[to] = '\0';

  memcpy (buf, str, len);

  return buf;
}

static void
bench_iteration (CtkWidget *widget, void (* fn) (CtkWidget *widget))
{
  fn (widget); /* on */
  while (g_main_context_iteration (NULL, FALSE));
  fn (widget); /* off */
  while (g_main_context_iteration (NULL, FALSE));
}

void
do_real_bench (CtkWidget *widget, void (* fn) (CtkWidget *widget), char *name, int num)
{
  GTimeVal tv0, tv1;
  double dt_first;
  double dt;
  int n;
  static gboolean printed_headers = FALSE;

  if (!printed_headers) {
    g_print ("Test                 Iters      First      Other\n");
    g_print ("-------------------- ----- ---------- ----------\n");
    printed_headers = TRUE;
  }

  g_get_current_time (&tv0);
  bench_iteration (widget, fn); 
  g_get_current_time (&tv1);

  dt_first = ((double)tv1.tv_sec - tv0.tv_sec) * 1000.0
	+ (tv1.tv_usec - tv0.tv_usec) / 1000.0;

  g_get_current_time (&tv0);
  for (n = 0; n < num - 1; n++)
    bench_iteration (widget, fn); 
  g_get_current_time (&tv1);
  dt = ((double)tv1.tv_sec - tv0.tv_sec) * 1000.0
	+ (tv1.tv_usec - tv0.tv_usec) / 1000.0;

  g_print ("%s %5d ", pad (name, 20), num);
  if (num > 1)
    g_print ("%10.1f %10.1f\n", dt_first, dt/(num-1));
  else
    g_print ("%10.1f\n", dt_first);
}

void
do_bench (char* what, int num)
{
  int i;
  CtkWidget *widget;
  void (* fn) (CtkWidget *widget);
  fn = NULL;
  widget = ctk_window_new (CTK_WINDOW_TOPLEVEL);

  if (g_ascii_strcasecmp (what, "ALL") == 0)
    {
      for (i = 0; i < nbuttons; i++)
	{
	  if (!buttons[i].do_not_benchmark)
	    do_real_bench (widget, buttons[i].func, buttons[i].label, num);
	}

      return;
    }
  else
    {
      for (i = 0; i < nbuttons; i++)
	{
	  if (strcmp (buttons[i].label, what) == 0)
	    {
	      fn = buttons[i].func;
	      break;
	    }
	}
      
      if (!fn)
	g_print ("Can't bench: \"%s\" not found.\n", what);
      else
	do_real_bench (widget, fn, buttons[i].label, num);
    }
}

void 
usage (void)
{
  fprintf (stderr, "Usage: testctk [--bench ALL|<bench>[:<count>]]\n");
  exit (1);
}

int
main (int argc, char *argv[])
{
  CtkCssProvider *provider, *memory_provider;
  CdkDisplay *display;
  CdkScreen *screen;
  CtkBindingSet *binding_set;
  int i;
  gboolean done_benchmarks = FALSE;

  srand (time (NULL));

  test_init ();

  g_set_application_name ("CTK+ Test Program");

  ctk_init (&argc, &argv);

  provider = ctk_css_provider_new ();

  /* Check to see if we are being run from the correct
   * directory.
   */
  if (file_exists ("testctk.css"))
    ctk_css_provider_load_from_path (provider, "testctk.css", NULL);
  else if (file_exists ("tests/testctk.css"))
    ctk_css_provider_load_from_path (provider, "tests/testctk.css", NULL);
  else
    g_warning ("Couldn't find file \"testctk.css\".");

  display = cdk_display_get_default ();
  screen = cdk_display_get_default_screen (display);

  ctk_style_context_add_provider_for_screen (screen, CTK_STYLE_PROVIDER (provider),
                                             CTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_object_unref (provider);

  ctk_accelerator_set_default_mod_mask (CDK_SHIFT_MASK |
					CDK_CONTROL_MASK |
					CDK_MOD1_MASK | 
					CDK_META_MASK |
					CDK_SUPER_MASK |
					CDK_HYPER_MASK |
					CDK_MOD4_MASK);
  /*  benchmarking
   */
  for (i = 1; i < argc; i++)
    {
      if (strncmp (argv[i], "--bench", strlen("--bench")) == 0)
        {
          int num = 1;
	  char *nextarg;
	  char *what;
	  char *count;
	  
	  nextarg = strchr (argv[i], '=');
	  if (nextarg)
	    nextarg++;
	  else
	    {
	      i++;
	      if (i == argc)
		usage ();
	      nextarg = argv[i];
	    }

	  count = strchr (nextarg, ':');
	  if (count)
	    {
	      what = g_strndup (nextarg, count - nextarg);
	      count++;
	      num = atoi (count);
	      if (num <= 0)
		usage ();
	    }
	  else
	    what = g_strdup (nextarg);

          do_bench (what, num ? num : 1);
	  done_benchmarks = TRUE;
        }
      else
	usage ();
    }
  if (done_benchmarks)
    return 0;

  /* bindings test
   */
  binding_set = ctk_binding_set_by_class (g_type_class_ref (CTK_TYPE_WIDGET));
  ctk_binding_entry_add_signal (binding_set,
				'9', CDK_CONTROL_MASK | CDK_RELEASE_MASK,
				"debug_msg",
				1,
				G_TYPE_STRING, "CtkWidgetClass <ctrl><release>9 test");

  memory_provider = ctk_css_provider_new ();
  ctk_css_provider_load_from_data (memory_provider,
                                   "#testctk-version-label {\n"
                                   "  color: #f00;\n"
                                   "  font-family: Sans;\n"
                                   "  font-size: 18px;\n"
                                   "}",
                                   -1, NULL);
  ctk_style_context_add_provider_for_screen (screen, CTK_STYLE_PROVIDER (memory_provider),
                                             CTK_STYLE_PROVIDER_PRIORITY_APPLICATION + 1);

  create_main_window ();

  ctk_main ();

  if (1)
    {
      while (g_main_context_pending (NULL))
	g_main_context_iteration (NULL, FALSE);
#if 0
      sleep (1);
      while (g_main_context_pending (NULL))
	g_main_context_iteration (NULL, FALSE);
#endif
    }
  return 0;
}
