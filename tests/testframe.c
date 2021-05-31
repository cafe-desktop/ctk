/* testframe.c
 * Copyright (C) 2007  Xan López <xan@gnome.org>
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
#include <math.h>

static gint hpadding = 0, vpadding = 0;

static void
spin_hpadding_cb (GtkSpinButton *spin, gpointer user_data)
{
  GtkWidget *frame = user_data;
  GtkCssProvider *provider;
  GtkStyleContext *context;
  gchar *data;

  context = ctk_widget_get_style_context (frame);
  provider = g_object_get_data (G_OBJECT (frame), "provider");
  if (provider == NULL)
    {
      provider = ctk_css_provider_new ();
      g_object_set_data (G_OBJECT (frame), "provider", provider);
      ctk_style_context_add_provider (context,
                                      CTK_STYLE_PROVIDER (provider),
                                      CTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }

  hpadding = (gint)ctk_spin_button_get_value (spin);
  data = g_strdup_printf ("frame > border { padding: %dpx %dpx }",
                          vpadding, hpadding);

  ctk_css_provider_load_from_data (provider, data, -1, NULL);
  g_free (data);

  ctk_widget_queue_resize (frame);
}

static void
spin_vpadding_cb (GtkSpinButton *spin, gpointer user_data)
{
  GtkWidget *frame = user_data;
  GtkCssProvider *provider;
  GtkStyleContext *context;
  gchar *data;

  context = ctk_widget_get_style_context (frame);
  provider = g_object_get_data (G_OBJECT (frame), "provider");
  if (provider == NULL)
    {
      provider = ctk_css_provider_new ();
      g_object_set_data (G_OBJECT (frame), "provider", provider);
      ctk_style_context_add_provider (context,
                                      CTK_STYLE_PROVIDER (provider),
                                      CTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }

  vpadding = (gint)ctk_spin_button_get_value (spin);
  data = g_strdup_printf ("frame > border { padding: %dpx %dpx }",
                          vpadding, hpadding);

  ctk_css_provider_load_from_data (provider, data, -1, NULL);
  g_free (data);

  ctk_widget_queue_resize (frame);
}

/* Function to normalize rounding errors in FP arithmetic to
   our desired limits */

#define EPSILON 1e-10

static gdouble
double_normalize (gdouble n)
{
  if (fabs (1.0 - n) < EPSILON)
    n = 1.0;
  else if (n < EPSILON)
    n = 0.0;

  return n;
}

static void
spin_xalign_cb (GtkSpinButton *spin, GtkFrame *frame)
{
  gdouble xalign;
  gfloat yalign;

  xalign = double_normalize (ctk_spin_button_get_value (spin));
  ctk_frame_get_label_align (frame, NULL, &yalign);
  ctk_frame_set_label_align (frame, xalign, yalign);
}

static void
spin_yalign_cb (GtkSpinButton *spin, GtkFrame *frame)
{
  gdouble yalign;
  gfloat xalign;

  yalign = double_normalize (ctk_spin_button_get_value (spin));
  ctk_frame_get_label_align (frame, &xalign, NULL);
  ctk_frame_set_label_align (frame, xalign, yalign);
}

static void
draw_border_cb (GtkToggleButton *toggle_button, GtkFrame *frame)
{
  GtkShadowType shadow_type = ctk_toggle_button_get_active (toggle_button)
                              ? CTK_SHADOW_IN : CTK_SHADOW_NONE;

  ctk_frame_set_shadow_type (frame, shadow_type);
}

int main (int argc, char **argv)
{
  GtkWidget *window, *widget;
  GtkBox *vbox;
  GtkFrame *frame;
  GtkGrid *grid;
  gfloat xalign, yalign;
  gboolean draw_border;

  ctk_init (&argc, &argv);

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_container_set_border_width (CTK_CONTAINER (window), 5);
  ctk_window_set_default_size (CTK_WINDOW (window), 300, 300);

  g_signal_connect (window, "delete-event", ctk_main_quit, NULL);

  vbox = CTK_BOX (ctk_box_new (CTK_ORIENTATION_VERTICAL, 5));
  g_object_set (vbox, "margin", 12, NULL);
  ctk_container_add (CTK_CONTAINER (window), CTK_WIDGET (vbox));

  frame = CTK_FRAME (ctk_frame_new ("Test GtkFrame"));
  ctk_box_pack_start (vbox, CTK_WIDGET (frame), TRUE, TRUE, 0);

  widget = ctk_button_new_with_label ("Hello!");
  ctk_container_add (CTK_CONTAINER (frame), widget);

  grid = CTK_GRID (ctk_grid_new ());
  ctk_grid_set_row_spacing (grid, 12);
  ctk_grid_set_column_spacing (grid, 6);
  ctk_box_pack_start (vbox, CTK_WIDGET (grid), FALSE, FALSE, 0);

  ctk_frame_get_label_align (frame, &xalign, &yalign);

  /* Spin to control :label-xalign */
  widget = ctk_label_new ("label xalign:");
  ctk_grid_attach (grid, widget, 0, 0, 1, 1);

  widget = ctk_spin_button_new_with_range (0.0, 1.0, 0.1);
  ctk_spin_button_set_value (CTK_SPIN_BUTTON (widget), xalign);
  g_signal_connect (widget, "value-changed", G_CALLBACK (spin_xalign_cb), frame);
  ctk_grid_attach (grid, widget, 1, 0, 1, 1);

  /* Spin to control :label-yalign */
  widget = ctk_label_new ("label yalign:");
  ctk_grid_attach (grid, widget, 0, 1, 1, 1);

  widget = ctk_spin_button_new_with_range (0.0, 1.0, 0.1);
  ctk_spin_button_set_value (CTK_SPIN_BUTTON (widget), yalign);
  g_signal_connect (widget, "value-changed", G_CALLBACK (spin_yalign_cb), frame);
  ctk_grid_attach (grid, widget, 1, 1, 1, 1);

  /* Spin to control vertical padding */
  widget = ctk_label_new ("vertical padding:");
  ctk_grid_attach (grid, widget, 0, 2, 1, 1);

  widget = ctk_spin_button_new_with_range (0, 250, 1);
  g_signal_connect (widget, "value-changed", G_CALLBACK (spin_vpadding_cb), frame);
  ctk_spin_button_set_value (CTK_SPIN_BUTTON (widget), vpadding);
  ctk_grid_attach (grid, widget, 1, 2, 1, 1);

  /* Spin to control horizontal padding */
  widget = ctk_label_new ("horizontal padding:");
  ctk_grid_attach (grid, widget, 0, 3, 1, 1);

  widget = ctk_spin_button_new_with_range (0, 250, 1);
  g_signal_connect (widget, "value-changed", G_CALLBACK (spin_hpadding_cb), frame);
  ctk_spin_button_set_value (CTK_SPIN_BUTTON (widget), hpadding);
  ctk_grid_attach (grid, widget, 1, 3, 1, 1);

  /* CheckButton to control whether to draw border */
  draw_border = ctk_frame_get_shadow_type (frame) != CTK_SHADOW_NONE;
  widget = ctk_check_button_new_with_label ("draw border");
  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (widget), draw_border);
  g_signal_connect (widget, "toggled", G_CALLBACK (draw_border_cb), frame);
  ctk_grid_attach (grid, widget, 0, 4, 2, 1);

  ctk_widget_show_all (window);

  ctk_main ();

  return 0;
}
