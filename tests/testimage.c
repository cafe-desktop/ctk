/* testimage.c
 * Copyright (C) 2004  Red Hat, Inc.
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
#include <gio/gio.h>

static void
drag_begin (CtkWidget      *widget,
	    GdkDragContext *context,
	    gpointer        data)
{
  CtkWidget *image = CTK_WIDGET (data);

  GdkPixbuf *pixbuf = ctk_image_get_pixbuf (CTK_IMAGE (image));

  ctk_drag_set_icon_pixbuf (context, pixbuf, -2, -2);
}

void  
drag_data_get  (CtkWidget        *widget,
		GdkDragContext   *context,
		CtkSelectionData *selection_data,
		guint             info,
		guint             time,
		gpointer          data)
{
  CtkWidget *image = CTK_WIDGET (data);

  GdkPixbuf *pixbuf = ctk_image_get_pixbuf (CTK_IMAGE (image));

  ctk_selection_data_set_pixbuf (selection_data, pixbuf);
}

static void
drag_data_received (CtkWidget        *widget,
		    GdkDragContext   *context,
		    gint              x,
		    gint              y,
		    CtkSelectionData *selection_data,
		    guint             info,
		    guint32           time,
		    gpointer          data)
{
  CtkWidget *image = CTK_WIDGET (data);

  GdkPixbuf *pixbuf;

  if (ctk_selection_data_get_length (selection_data) < 0)
    return;

  pixbuf = ctk_selection_data_get_pixbuf (selection_data);

  ctk_image_set_from_pixbuf (CTK_IMAGE (image), pixbuf);
}

static gboolean
idle_func (gpointer data)
{
  g_print ("keep me busy\n");

  return G_SOURCE_CONTINUE;
}

static gboolean
anim_image_draw (CtkWidget *widget,
                 cairo_t   *cr,
                 gpointer   data)
{
  g_print ("start busyness\n");

  g_signal_handlers_disconnect_by_func (widget, anim_image_draw, data);

  /* produce high load */
  g_idle_add_full (G_PRIORITY_DEFAULT,
                   idle_func, NULL, NULL);

  return FALSE;
}

int
main (int argc, char **argv)
{
  CtkWidget *window, *grid;
  CtkWidget *label, *image, *box;
  CtkIconTheme *theme;
  GdkPixbuf *pixbuf;
  CtkIconSet *iconset;
  CtkIconSource *iconsource;
  gchar *icon_name = "gnome-terminal";
  gchar *anim_filename = NULL;
  GIcon *icon;
  GFile *file;
  GdkGeometry geo;

  ctk_init (&argc, &argv);

  if (argc > 1)
    icon_name = argv[1];

  if (argc > 2)
    anim_filename = argv[2];

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);

  geo.min_width = 400;
  geo.min_height = 300;
  geo.max_width = 800;
  geo.max_height = 600;

  ctk_window_set_geometry_hints (CTK_WINDOW (window), NULL, &geo, GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE);

  grid = ctk_grid_new ();
  ctk_container_add (CTK_CONTAINER (window), grid);

  label = ctk_label_new ("symbolic size");
  ctk_grid_attach (CTK_GRID (grid), label, 1, 0, 1, 1);
  label = ctk_label_new ("fixed size");
  ctk_grid_attach (CTK_GRID (grid), label, 2, 0, 1, 1);

  label = ctk_label_new ("CTK_IMAGE_PIXBUF");
  ctk_grid_attach (CTK_GRID (grid), label, 0, 1, 1, 1);

  theme = ctk_icon_theme_get_default ();
  pixbuf = ctk_icon_theme_load_icon (theme, icon_name, 48, 0, NULL);
  image = ctk_image_new_from_pixbuf (pixbuf);
  box = ctk_event_box_new ();
  ctk_container_add (CTK_CONTAINER (box), image);
  ctk_grid_attach (CTK_GRID (grid), box, 2, 1, 1, 1);

  ctk_drag_source_set (box, GDK_BUTTON1_MASK, 
		       NULL, 0,
		       GDK_ACTION_COPY);
  ctk_drag_source_add_image_targets (box);
  g_signal_connect (box, "drag_begin", G_CALLBACK (drag_begin), image);
  g_signal_connect (box, "drag_data_get", G_CALLBACK (drag_data_get), image);

  ctk_drag_dest_set (box,
                     CTK_DEST_DEFAULT_MOTION |
                     CTK_DEST_DEFAULT_HIGHLIGHT |
                     CTK_DEST_DEFAULT_DROP,
                     NULL, 0, GDK_ACTION_COPY);
  ctk_drag_dest_add_image_targets (box);
  g_signal_connect (box, "drag_data_received", 
		    G_CALLBACK (drag_data_received), image);

  label = ctk_label_new ("CTK_IMAGE_STOCK");
  ctk_grid_attach (CTK_GRID (grid), label, 0, 2, 1, 1);

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  image = ctk_image_new_from_stock (CTK_STOCK_REDO, CTK_ICON_SIZE_DIALOG);
  ctk_grid_attach (CTK_GRID (grid), image, 1, 2, 1, 1);

  label = ctk_label_new ("CTK_IMAGE_ICON_SET");
  ctk_grid_attach (CTK_GRID (grid), label, 0, 3, 1, 1);

  iconsource = ctk_icon_source_new ();
  ctk_icon_source_set_icon_name (iconsource, icon_name);
  iconset = ctk_icon_set_new ();
  ctk_icon_set_add_source (iconset, iconsource);
  image = ctk_image_new_from_icon_set (iconset, CTK_ICON_SIZE_DIALOG);
  ctk_grid_attach (CTK_GRID (grid), image, 1, 3, 1, 1);
  G_GNUC_END_IGNORE_DEPRECATIONS;

  label = ctk_label_new ("CTK_IMAGE_ICON_NAME");
  ctk_grid_attach (CTK_GRID (grid), label, 0, 4, 1, 1);
  image = ctk_image_new_from_icon_name (icon_name, CTK_ICON_SIZE_DIALOG);
  ctk_grid_attach (CTK_GRID (grid), image, 1, 4, 1, 1);
  image = ctk_image_new_from_icon_name (icon_name, CTK_ICON_SIZE_DIALOG);
  ctk_image_set_pixel_size (CTK_IMAGE (image), 30);
  ctk_grid_attach (CTK_GRID (grid), image, 2, 4, 1, 1);

  label = ctk_label_new ("CTK_IMAGE_GICON");
  ctk_grid_attach (CTK_GRID (grid), label, 0, 5, 1, 1);
  icon = g_themed_icon_new_with_default_fallbacks ("folder-remote");
  image = ctk_image_new_from_gicon (icon, CTK_ICON_SIZE_DIALOG);
  g_object_unref (icon);
  ctk_grid_attach (CTK_GRID (grid), image, 1, 5, 1, 1);
  file = g_file_new_for_path ("apple-red.png");
  icon = g_file_icon_new (file);
  image = ctk_image_new_from_gicon (icon, CTK_ICON_SIZE_DIALOG);
  g_object_unref (icon);
  ctk_image_set_pixel_size (CTK_IMAGE (image), 30);
  ctk_grid_attach (CTK_GRID (grid), image, 2, 5, 1, 1);

  
  if (anim_filename)
    {
      label = ctk_label_new ("CTK_IMAGE_ANIMATION (from file)");
      ctk_grid_attach (CTK_GRID (grid), label, 0, 6, 1, 1);
      image = ctk_image_new_from_file (anim_filename);
      ctk_image_set_pixel_size (CTK_IMAGE (image), 30);
      ctk_grid_attach (CTK_GRID (grid), image, 2, 6, 1, 1);

      /* produce high load */
      g_signal_connect_after (image, "draw",
                              G_CALLBACK (anim_image_draw),
                              NULL);
    }

  ctk_widget_show_all (window);

  ctk_main ();

  return 0;
}
