/* testadjustsize.c
 * Copyright (C) 2010 Havoc Pennington
 *
 * Author:
 *      Havoc Pennington <hp@pobox.com>
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

static CtkWidget *test_window;

enum {
  TEST_WIDGET_LABEL,
  TEST_WIDGET_VERTICAL_LABEL,
  TEST_WIDGET_WRAP_LABEL,
  TEST_WIDGET_ALIGNMENT,
  TEST_WIDGET_IMAGE,
  TEST_WIDGET_BUTTON,
  TEST_WIDGET_LAST
};

static CtkWidget *test_widgets[TEST_WIDGET_LAST];

static CtkWidget*
create_image (void)
{
  return ctk_image_new_from_icon_name ("document-open",
                                       CTK_ICON_SIZE_BUTTON);
}

static CtkWidget*
create_label (gboolean vertical,
              gboolean wrap)
{
  CtkWidget *widget;

  widget = ctk_label_new ("This is a label, label label label");

  if (vertical)
    ctk_label_set_angle (CTK_LABEL (widget), 90);

  if (wrap)
    ctk_label_set_line_wrap (CTK_LABEL (widget), TRUE);

  return widget;
}

static CtkWidget*
create_button (void)
{
  return ctk_button_new_with_label ("BUTTON!");
}

static gboolean
on_draw_alignment (CtkWidget *widget G_GNUC_UNUSED,
                   cairo_t   *cr,
                   void      *data G_GNUC_UNUSED)
{
  cairo_set_source_rgb (cr, 1.0, 0.0, 0.0);
  cairo_paint (cr);

  return FALSE;
}

static CtkWidget*
create_alignment (void)
{
  CtkWidget *alignment;

  alignment = ctk_alignment_new (0.5, 0.5, 1.0, 1.0);

  /* make the alignment visible */
  ctk_widget_set_redraw_on_allocate (alignment, TRUE);
  g_signal_connect (G_OBJECT (alignment),
                    "draw",
                    G_CALLBACK (on_draw_alignment),
                    NULL);

  return alignment;
}

static void
open_test_window (void)
{
  CtkWidget *grid;
  int i;

  test_window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_title (CTK_WINDOW (test_window), "Tests");

  g_signal_connect (test_window, "delete-event",
                    G_CALLBACK (ctk_main_quit), test_window);

  ctk_window_set_resizable (CTK_WINDOW (test_window), FALSE);

  test_widgets[TEST_WIDGET_IMAGE] = create_image ();
  test_widgets[TEST_WIDGET_LABEL] = create_label (FALSE, FALSE);
  test_widgets[TEST_WIDGET_VERTICAL_LABEL] = create_label (TRUE, FALSE);
  test_widgets[TEST_WIDGET_WRAP_LABEL] = create_label (FALSE, TRUE);
  test_widgets[TEST_WIDGET_BUTTON] = create_button ();
  test_widgets[TEST_WIDGET_ALIGNMENT] = create_alignment ();

  grid = ctk_grid_new ();

  ctk_container_add (CTK_CONTAINER (test_window), grid);

  for (i = 0; i < TEST_WIDGET_LAST; ++i)
    {
      ctk_grid_attach (CTK_GRID (grid), test_widgets[i], i % 3, i / 3, 1, 1);
    }

  ctk_widget_show_all (test_window);
}

static void
on_toggle_border_widths (CtkToggleButton *button,
                         void            *data G_GNUC_UNUSED)
{
  gboolean has_border;
  int i;

  has_border = ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (button));

  for (i = 0; i < TEST_WIDGET_LAST; ++i)
    {
      if (CTK_IS_CONTAINER (test_widgets[i]))
        {
          ctk_container_set_border_width (CTK_CONTAINER (test_widgets[i]),
                                          has_border ? 50 : 0);
        }
    }
}

static void
on_set_small_size_requests (CtkToggleButton *button,
                            void            *data G_GNUC_UNUSED)
{
  gboolean has_small_size_requests;
  int i;

  has_small_size_requests = ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (button));

  for (i = 0; i < TEST_WIDGET_LAST; ++i)
    {
      ctk_widget_set_size_request (test_widgets[i],
                                   has_small_size_requests ? 5 : -1,
                                   has_small_size_requests ? 5 : -1);
    }
}

static void
on_set_large_size_requests (CtkToggleButton *button,
                            void            *data G_GNUC_UNUSED)
{
  gboolean has_large_size_requests;
  int i;

  has_large_size_requests = ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (button));

  for (i = 0; i < TEST_WIDGET_LAST; ++i)
    {
      ctk_widget_set_size_request (test_widgets[i],
                                   has_large_size_requests ? 200 : -1,
                                   has_large_size_requests ? 200 : -1);
    }
}

static void
open_control_window (void)
{
  CtkWidget *window;
  CtkWidget *box;
  CtkWidget *toggle;

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_title (CTK_WINDOW (window), "Controls");

  g_signal_connect (window, "delete-event",
                    G_CALLBACK (ctk_main_quit), window);

  box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  ctk_container_add (CTK_CONTAINER (window), box);

  toggle =
    ctk_toggle_button_new_with_label ("Containers have borders");
  g_signal_connect (G_OBJECT (toggle),
                    "toggled", G_CALLBACK (on_toggle_border_widths),
                    NULL);
  ctk_container_add (CTK_CONTAINER (box), toggle);

  toggle =
    ctk_toggle_button_new_with_label ("Set small size requests");
  g_signal_connect (G_OBJECT (toggle),
                    "toggled", G_CALLBACK (on_set_small_size_requests),
                    NULL);
  ctk_container_add (CTK_CONTAINER (box), toggle);

  toggle =
    ctk_toggle_button_new_with_label ("Set large size requests");
  g_signal_connect (G_OBJECT (toggle),
                    "toggled", G_CALLBACK (on_set_large_size_requests),
                    NULL);
  ctk_container_add (CTK_CONTAINER (box), toggle);


  ctk_widget_show_all (window);
}

#define TEST_WIDGET(outer) (ctk_bin_get_child (CTK_BIN (ctk_bin_get_child (CTK_BIN(outer)))))

static CtkWidget*
create_widget_visible_border (const char *text)
{
  CtkWidget *outer_box;
  CtkWidget *inner_box;
  CtkWidget *test_widget;
  CtkWidget *label;

  outer_box = ctk_event_box_new ();
  ctk_style_context_add_class (ctk_widget_get_style_context (outer_box), "black-bg");

  inner_box = ctk_event_box_new ();
  ctk_container_set_border_width (CTK_CONTAINER (inner_box), 5);
  ctk_style_context_add_class (ctk_widget_get_style_context (inner_box), "blue-bg");

  ctk_container_add (CTK_CONTAINER (outer_box), inner_box);


  test_widget = ctk_event_box_new ();
  ctk_style_context_add_class (ctk_widget_get_style_context (test_widget), "red-bg");

  ctk_container_add (CTK_CONTAINER (inner_box), test_widget);

  label = ctk_label_new (text);
  ctk_container_add (CTK_CONTAINER (test_widget), label);

  g_assert (TEST_WIDGET (outer_box) == test_widget);

  ctk_widget_show_all (outer_box);

  return outer_box;
}

static const char*
enum_to_string (GType enum_type,
                int   value)
{
  GEnumValue *v;

  v = g_enum_get_value (g_type_class_peek (enum_type), value);

  return v->value_nick;
}

static CtkWidget*
create_aligned (CtkAlign halign,
                CtkAlign valign)
{
  CtkWidget *widget;
  char *label;

  label = g_strdup_printf ("h=%s v=%s",
                           enum_to_string (CTK_TYPE_ALIGN, halign),
                           enum_to_string (CTK_TYPE_ALIGN, valign));

  widget = create_widget_visible_border (label);

  g_object_set (G_OBJECT (TEST_WIDGET (widget)),
                "halign", halign,
                "valign", valign,
                "hexpand", TRUE,
                "vexpand", TRUE,
                NULL);

  return widget;
}

static void
open_alignment_window (void)
{
  CtkWidget *grid;
  int i;
  GEnumClass *align_class;

  test_window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_title (CTK_WINDOW (test_window), "Alignment");

  g_signal_connect (test_window, "delete-event",
                    G_CALLBACK (ctk_main_quit), test_window);

  ctk_window_set_resizable (CTK_WINDOW (test_window), TRUE);
  ctk_window_set_default_size (CTK_WINDOW (test_window), 500, 500);

  align_class = g_type_class_peek (CTK_TYPE_ALIGN);

  grid = ctk_grid_new ();
  ctk_grid_set_row_homogeneous (CTK_GRID (grid), TRUE);
  ctk_grid_set_column_homogeneous (CTK_GRID (grid), TRUE);

  ctk_container_add (CTK_CONTAINER (test_window), grid);

  for (i = 0; i < align_class->n_values; ++i)
    {
      int j;
      for (j = 0; j < align_class->n_values; ++j)
        {
          CtkWidget *child =
            create_aligned(align_class->values[i].value,
                           align_class->values[j].value);

          ctk_grid_attach (CTK_GRID (grid), child, i, j, 1, 1);
        }
    }

  ctk_widget_show_all (test_window);
}

static CtkWidget*
create_margined (const char *propname)
{
  CtkWidget *widget;

  widget = create_widget_visible_border (propname);

  g_object_set (G_OBJECT (TEST_WIDGET (widget)),
                propname, 15,
                "hexpand", TRUE,
                "vexpand", TRUE,
                NULL);

  return widget;
}

static void
open_margin_window (void)
{
  CtkWidget *box;
  int i;
  const char * margins[] = {
    "margin-start",
    "margin-end",
    "margin-top",
    "margin-bottom",
    "margin"
  };

  test_window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_title (CTK_WINDOW (test_window), "Margin");

  g_signal_connect (test_window, "delete-event",
                    G_CALLBACK (ctk_main_quit), test_window);

  ctk_window_set_resizable (CTK_WINDOW (test_window), TRUE);

  box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);

  ctk_container_add (CTK_CONTAINER (test_window), box);

  for (i = 0; i < (int) G_N_ELEMENTS (margins); ++i)
    {
      CtkWidget *child =
        create_margined(margins[i]);

      ctk_container_add (CTK_CONTAINER (box), child);
    }

  ctk_widget_show_all (test_window);
}

static void
open_valigned_label_window (void)
{
  CtkWidget *window, *box, *label, *frame;

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);

  g_signal_connect (test_window, "delete-event",
                    G_CALLBACK (ctk_main_quit), test_window);

  box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  ctk_widget_show (box);
  ctk_container_add (CTK_CONTAINER (window), box);

  label = ctk_label_new ("Both labels expand");
  ctk_widget_show (label);
  ctk_box_pack_start (CTK_BOX (box), label, TRUE, TRUE, 0);

  label = ctk_label_new ("Some wrapping text with width-chars = 15 and max-width-chars = 35");
  ctk_label_set_line_wrap  (CTK_LABEL (label), TRUE);
  ctk_label_set_width_chars  (CTK_LABEL (label), 15);
  ctk_label_set_max_width_chars  (CTK_LABEL (label), 35);

  ctk_widget_show (label);

  frame  = ctk_frame_new (NULL);
  ctk_widget_show (frame);
  ctk_container_add (CTK_CONTAINER (frame), label);

  ctk_widget_set_valign (frame, CTK_ALIGN_CENTER);
  ctk_widget_set_halign (frame, CTK_ALIGN_CENTER);

  ctk_box_pack_start (CTK_BOX (box), frame, TRUE, TRUE, 0);

  ctk_window_present (CTK_WINDOW (window));
}

int
main (int argc, char *argv[])
{
  CtkCssProvider *provider;

  ctk_init (&argc, &argv);

  provider = ctk_css_provider_new ();
  ctk_css_provider_load_from_data (provider,
    ".black-bg { background-color: black; }"
    ".red-bg { background-color: red; }"
    ".blue-bg { background-color: blue; }", -1, NULL);
  ctk_style_context_add_provider_for_screen (cdk_screen_get_default (),
                                             CTK_STYLE_PROVIDER (provider),
                                             CTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_object_unref (provider);
  
  if (g_getenv ("RTL"))
    ctk_widget_set_default_direction (CTK_TEXT_DIR_RTL);

  open_test_window ();
  open_control_window ();
  open_alignment_window ();
  open_margin_window ();
  open_valigned_label_window ();

  ctk_main ();

  return 0;
}
