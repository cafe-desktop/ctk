/* testgeometry.c
 * Author: Owen Taylor <otaylor@redhat.com>
 * Copyright © 2010 Red Hat, Inc.
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

#define GRID_SIZE 20
#define BORDER 6

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static int window_count = 0;
const char *geometry_string;

static void
on_window_destroy (CtkWidget *widget)
{
  window_count--;
  if (window_count == 0)
    ctk_main_quit();
}

static gboolean
on_drawing_area_draw (CtkWidget *drawing_area,
		      cairo_t   *cr,
		      gpointer   data)
{
  int width = ctk_widget_get_allocated_width (drawing_area);
  int height = ctk_widget_get_allocated_height (drawing_area);
  int x, y;
  int border = 0;
  GdkWindowHints mask = GPOINTER_TO_UINT(data);

  cairo_set_source_rgb (cr, 1, 1, 1);
  cairo_paint (cr);

  if ((mask & GDK_HINT_BASE_SIZE) != 0)
    border = BORDER;

  cairo_set_source_rgb (cr, 0, 0, 0);
  for (y = 0; y < height - 2 * border; y += GRID_SIZE)
    for (x = 0; x < width - 2 * border; x += GRID_SIZE)
      if (((x + y) / GRID_SIZE) % 2 == 0)
	{
	  cairo_rectangle (cr, border + x, border + y, GRID_SIZE, GRID_SIZE);
	  cairo_fill (cr);
	}

  if (border > 0)
    {
      cairo_set_source_rgb (cr, 0, 0, 1);
      cairo_save (cr);
      cairo_set_line_width (cr, border);
      cairo_rectangle (cr,
		       border / 2., border / 2., width - border, height - border);
      cairo_stroke (cr);
    }

  return FALSE;
}

static void
on_resize_clicked (CtkWidget *button,
		   gpointer   data)
{
  CtkWidget *window = ctk_widget_get_toplevel (button);
  GdkWindowHints mask = GPOINTER_TO_UINT(data);

  if ((mask & GDK_HINT_RESIZE_INC) != 0)
    ctk_window_resize_to_geometry (CTK_WINDOW (window), 8, 8);
  else
    ctk_window_resize_to_geometry (CTK_WINDOW (window), 8 * GRID_SIZE, 8 * GRID_SIZE);
}

static void
create_window (GdkWindowHints  mask)
{
  CtkWidget *window;
  CtkWidget *drawing_area;
  CtkWidget *grid;
  CtkWidget *label;
  CtkWidget *button;
  GdkGeometry geometry;
  GString *label_text = g_string_new (NULL);
  int border = 0;

  if ((mask & GDK_HINT_RESIZE_INC) != 0)
    g_string_append (label_text, "Gridded\n");
  if ((mask & GDK_HINT_BASE_SIZE) != 0)
    g_string_append (label_text, "Base\n");
  if ((mask & GDK_HINT_MIN_SIZE) != 0)
    {
      g_string_append (label_text, "Minimum\n");
      if ((mask & GDK_HINT_BASE_SIZE) == 0)
	g_string_append (label_text, "(base=min)\n");
    }
  if ((mask & GDK_HINT_MAX_SIZE) != 0)
    g_string_append (label_text, "Maximum\n");

  if (label_text->len > 0)
    g_string_erase (label_text, label_text->len - 1, 1);
  else
    g_string_append (label_text, "No Options");

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  g_signal_connect (window, "destroy",
		    G_CALLBACK (on_window_destroy), NULL);

  grid = ctk_grid_new ();
  ctk_container_set_border_width (CTK_CONTAINER (grid), 10);

  label = ctk_label_new (label_text->str);
  ctk_widget_set_hexpand (label, TRUE);
  ctk_grid_attach (CTK_GRID (grid), label, 0, 0, 1, 1);

  label = ctk_label_new ("A\nB\nC\nD\nE");
  ctk_grid_attach (CTK_GRID (grid), label, 1, 1, 1, 1);

  drawing_area = ctk_drawing_area_new ();
  g_signal_connect (drawing_area, "draw",
		    G_CALLBACK (on_drawing_area_draw),
		    GUINT_TO_POINTER (mask));
  ctk_widget_set_hexpand (drawing_area, TRUE);
  ctk_widget_set_vexpand (drawing_area, TRUE);
  ctk_grid_attach (CTK_GRID (grid), drawing_area, 0, 1, 1, 1);

  button = ctk_button_new_with_label ("Resize");
  g_signal_connect (button, "clicked",
		    G_CALLBACK (on_resize_clicked),
		    GUINT_TO_POINTER (mask));
  ctk_widget_set_hexpand (button, TRUE);
  ctk_grid_attach (CTK_GRID (grid), button, 0, 2, 1, 1);

  ctk_container_add (CTK_CONTAINER (window), grid);

  if ((mask & GDK_HINT_BASE_SIZE) != 0)
    {
      border = BORDER;
      geometry.base_width = border * 2;
      geometry.base_height = border * 2;
    }

  if ((mask & GDK_HINT_RESIZE_INC) != 0)
    {
      geometry.width_inc = GRID_SIZE;
      geometry.height_inc = GRID_SIZE;
    }

  if ((mask & GDK_HINT_MIN_SIZE) != 0)
    {
      geometry.min_width = 5 * GRID_SIZE + 2 * border;
      geometry.min_height = 5 * GRID_SIZE + 2 * border;
    }

  if ((mask & GDK_HINT_MAX_SIZE) != 0)
    {
      geometry.max_width = 15 * GRID_SIZE + 2 * border;
      geometry.max_height = 15 * GRID_SIZE + 2 * border;
    }

  /* Contents must be set up before ctk_window_parse_geometry() */
  ctk_widget_show_all (grid);

  ctk_window_set_geometry_hints (CTK_WINDOW (window),
				 drawing_area,
				 &geometry,
				 mask);

  if ((mask & GDK_HINT_RESIZE_INC) != 0)
    {
      if (geometry_string)
	ctk_window_parse_geometry (CTK_WINDOW (window), geometry_string);
      else
	ctk_window_set_default_geometry (CTK_WINDOW (window), 10, 10);
    }
  else
    {
      ctk_window_set_default_geometry (CTK_WINDOW (window), 10 * GRID_SIZE, 10 * GRID_SIZE);
    }

  ctk_widget_show (window);
  window_count++;
}

int
main(int argc, char **argv)
{
  GError *error;
  GOptionEntry options[] = {
    { "geometry", 'g', 0, G_OPTION_ARG_STRING, &geometry_string, "Window geometry (only for gridded windows)", "GEOMETRY" },
    { NULL }
  };

  if (!ctk_init_with_args (&argc, &argv, "", options, NULL, &error))
    {
      g_print ("Failed to parse args: %s\n", error->message);
      g_error_free (error);
      return 1;
    }

  create_window (GDK_HINT_MIN_SIZE);
  create_window (GDK_HINT_MIN_SIZE | GDK_HINT_BASE_SIZE);
  create_window (GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE);
  create_window (GDK_HINT_RESIZE_INC | GDK_HINT_MIN_SIZE);
  create_window (GDK_HINT_RESIZE_INC | GDK_HINT_MAX_SIZE);
  create_window (GDK_HINT_RESIZE_INC | GDK_HINT_BASE_SIZE);
  create_window (GDK_HINT_RESIZE_INC | GDK_HINT_BASE_SIZE | GDK_HINT_MIN_SIZE);

  ctk_main ();

  return 0;
}
