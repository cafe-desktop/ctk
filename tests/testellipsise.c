/* GTK - The GIMP Toolkit
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
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.Free
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

#include <gtk/gtk.h>

static void
redraw_event_box (GtkWidget *widget)
{
  while (widget)
    {
      if (CTK_IS_EVENT_BOX (widget))
        {
          ctk_widget_queue_draw (widget);
          break;
        }

      widget = ctk_widget_get_parent (widget);
    }
}

static void
combo_changed_cb (GtkWidget *combo,
		  gpointer   data)
{
  GtkWidget *label = CTK_WIDGET (data);
  gint active;

  active = ctk_combo_box_get_active (CTK_COMBO_BOX (combo));
  ctk_label_set_ellipsize (CTK_LABEL (label), (PangoEllipsizeMode)active);
  redraw_event_box (label);
}

static void
scale_changed_cb (GtkRange *range,
		  gpointer   data)
{
  double angle = ctk_range_get_value (range);
  GtkWidget *label = CTK_WIDGET (data);
  
  ctk_label_set_angle (CTK_LABEL (label), angle);
  redraw_event_box (label);
}

static gboolean
ebox_draw_cb (GtkWidget *widget,
              cairo_t   *cr,
              gpointer   data)
{
  PangoLayout *layout;
  const double dashes[] = { 6, 18 };
  GtkAllocation label_allocation;
  GtkRequisition minimum_size, natural_size;
  GtkWidget *label = data;
  gint x, y;

  cairo_translate (cr, -0.5, -0.5);
  cairo_set_line_width (cr, 1);

  cairo_set_source_rgb (cr, 1, 1, 1);
  cairo_paint (cr);

  ctk_widget_translate_coordinates (label, widget, 0, 0, &x, &y);
  layout = ctk_widget_create_pango_layout (widget, "");

  ctk_widget_get_preferred_size (label, &minimum_size, &natural_size); 

  pango_layout_set_markup (layout,
    "<span color='#c33'>\342\227\217 requisition</span>\n"
    "<span color='#3c3'>\342\227\217 natural size</span>\n"
    "<span color='#33c'>\342\227\217 allocation</span>", -1);

  pango_cairo_show_layout (cr, layout);
  g_object_unref (layout);

  ctk_widget_get_allocation (label, &label_allocation);

  cairo_rectangle (cr,
                   x + 0.5 * (label_allocation.width - minimum_size.width),
                   y + 0.5 * (label_allocation.height - minimum_size.height),
                   minimum_size.width, minimum_size.height);
  cairo_set_source_rgb (cr, 0.8, 0.2, 0.2);
  cairo_set_dash (cr, NULL, 0, 0);
  cairo_stroke (cr);

  cairo_rectangle (cr, x, y, label_allocation.width, label_allocation.height);
  cairo_set_source_rgb (cr, 0.2, 0.2, 0.8);
  cairo_set_dash (cr, dashes, 2, 0.5);
  cairo_stroke (cr);

  cairo_rectangle (cr,
                   x + 0.5 * (label_allocation.width - natural_size.width),
                   y + 0.5 * (label_allocation.height - natural_size.height),
                   natural_size.width, natural_size.height);
  cairo_set_source_rgb (cr, 0.2, 0.8, 0.2);
  cairo_set_dash (cr, dashes, 2, 12.5);
  cairo_stroke (cr);

  return FALSE;
}

int
main (int argc, char *argv[])
{
  GtkWidget *window, *vbox, *label;
  GtkWidget *combo, *scale, *ebox;

  ctk_init (&argc, &argv);

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_container_set_border_width (CTK_CONTAINER (window), 12);
  ctk_window_set_default_size (CTK_WINDOW (window), 400, 300);
  g_signal_connect (window, "destroy", G_CALLBACK (ctk_main_quit), NULL);

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 6);
  ctk_container_add (CTK_CONTAINER (window), vbox);

  combo = ctk_combo_box_text_new ();
  scale = ctk_scale_new_with_range (CTK_ORIENTATION_HORIZONTAL,
                                    0, 360, 1);
  label = ctk_label_new ("This label may be ellipsized\nto make it fit.");

  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo), "NONE");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo), "START");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo), "MIDDLE");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo), "END");
  ctk_combo_box_set_active (CTK_COMBO_BOX (combo), 0);

  ctk_widget_set_halign (label, CTK_ALIGN_CENTER);
  ctk_widget_set_valign (label, CTK_ALIGN_CENTER);

  ebox = ctk_event_box_new ();
  ctk_widget_set_app_paintable (ebox, TRUE);
  ctk_container_add (CTK_CONTAINER (ebox), label);

  ctk_box_pack_start (CTK_BOX (vbox), combo, FALSE, TRUE, 0);
  ctk_box_pack_start (CTK_BOX (vbox), scale, FALSE, TRUE, 0);
  ctk_box_pack_start (CTK_BOX (vbox), ebox, TRUE, TRUE, 0);

  g_object_set_data (G_OBJECT (label), "combo", combo);

  g_signal_connect (combo, "changed", G_CALLBACK (combo_changed_cb), label);
  g_signal_connect (scale, "value-changed", G_CALLBACK (scale_changed_cb), label);
  g_signal_connect (ebox, "draw", G_CALLBACK (ebox_draw_cb), label);

  ctk_widget_show_all (window);

  ctk_main ();

  return 0;
}
