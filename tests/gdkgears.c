#include <stdlib.h>
#include <ctk/ctk.h>

#include "ctkgears.h"

/************************************************************************
 *                 DEMO CODE                                            *
 ************************************************************************/

static void
toggle_alpha (GtkWidget *checkbutton,
              GtkWidget *gears)
{
  ctk_gl_area_set_has_alpha (CTK_GL_AREA (gears),
                             ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON(checkbutton)));
}

static void
toggle_overlay (GtkWidget *checkbutton,
		GtkWidget *revealer)
{
  ctk_revealer_set_reveal_child (CTK_REVEALER (revealer),
				 ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON(checkbutton)));
}

static void
toggle_spin (GtkWidget *checkbutton,
             GtkWidget *spinner)
{
  if (ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON(checkbutton)))
    ctk_spinner_start (CTK_SPINNER (spinner));
  else
    ctk_spinner_stop (CTK_SPINNER (spinner));
}

static void
on_axis_value_change (GtkAdjustment *adjustment,
                      gpointer       data)
{
  GtkGears *gears = CTK_GEARS (data);
  int axis = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (adjustment), "axis"));

  ctk_gears_set_axis (gears, axis, ctk_adjustment_get_value (adjustment));
}


static GtkWidget *
create_axis_slider (GtkGears *gears,
                    int axis)
{
  GtkWidget *box, *label, *slider;
  GtkAdjustment *adj;
  const char *text;

  box = ctk_box_new (CTK_ORIENTATION_VERTICAL, FALSE);

  switch (axis)
    {
    case CTK_GEARS_X_AXIS:
      text = "X";
      break;

    case CTK_GEARS_Y_AXIS:
      text = "Y";
      break;

    case CTK_GEARS_Z_AXIS:
      text = "Z";
      break;

    default:
      g_assert_not_reached ();
    }

  label = ctk_label_new (text);
  ctk_container_add (CTK_CONTAINER (box), label);
  ctk_widget_show (label);

  adj = ctk_adjustment_new (ctk_gears_get_axis (gears, axis), 0.0, 360.0, 1.0, 12.0, 0.0);
  g_object_set_data (G_OBJECT (adj), "axis", GINT_TO_POINTER (axis));
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (on_axis_value_change),
                    gears);
  slider = ctk_scale_new (CTK_ORIENTATION_VERTICAL, adj);
  ctk_scale_set_draw_value (CTK_SCALE (slider), FALSE);
  ctk_container_add (CTK_CONTAINER (box), slider);
  ctk_widget_set_vexpand (slider, TRUE);
  ctk_widget_show (slider);

  ctk_widget_show (box);

  return box;
}

static void
moar_gears (GtkButton *button, gpointer data)
{
  GtkContainer *container = CTK_CONTAINER (data);
  GtkWidget *gears;

  gears = ctk_gears_new ();
  ctk_widget_set_size_request (gears, 100, 100);
  ctk_container_add (container, gears);
  ctk_widget_show (gears);
}

int
main (int argc, char *argv[])
{
  GtkWidget *window, *box, *hbox, *button, *spinner, *check,
    *fps_label, *gears, *extra_hbox, *bbox, *overlay,
    *revealer, *frame, *label, *scrolled, *popover;
  int i;

  ctk_init (&argc, &argv);

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_titlebar (CTK_WINDOW (window), g_object_new (CTK_TYPE_HEADER_BAR, "visible", TRUE, "title", "GdkGears", NULL));
  ctk_window_set_default_size (CTK_WINDOW (window), 640, 640);
  ctk_container_set_border_width (CTK_CONTAINER (window), 12);
  g_signal_connect (window, "destroy", G_CALLBACK (ctk_main_quit), NULL);

  overlay = ctk_overlay_new ();
  ctk_container_add (CTK_CONTAINER (window), overlay);
  ctk_widget_show (overlay);

  revealer = ctk_revealer_new ();
  ctk_widget_set_halign (revealer, CTK_ALIGN_END);
  ctk_widget_set_valign (revealer, CTK_ALIGN_START);
  ctk_overlay_add_overlay (CTK_OVERLAY (overlay),
			   revealer);
  ctk_widget_show (revealer);

  frame = ctk_frame_new (NULL);
  ctk_style_context_add_class (ctk_widget_get_style_context (frame),
			       "app-notification");
  ctk_container_add (CTK_CONTAINER (revealer), frame);
  ctk_widget_show (frame);

  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, FALSE);
  ctk_box_set_spacing (CTK_BOX (hbox), 6);
  ctk_container_add (CTK_CONTAINER (frame), hbox);
  ctk_widget_show (hbox);

  label = ctk_label_new ("This is a transparent overlay widget!!!!\nAmazing, eh?");
  ctk_container_add (CTK_CONTAINER (hbox), label);
  ctk_widget_show (label);

  box = ctk_box_new (CTK_ORIENTATION_VERTICAL, FALSE);
  ctk_box_set_spacing (CTK_BOX (box), 6);
  ctk_container_add (CTK_CONTAINER (overlay), box);
  ctk_widget_show (box);

  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, FALSE);
  ctk_box_set_spacing (CTK_BOX (box), 6);
  ctk_container_add (CTK_CONTAINER (box), hbox);
  ctk_widget_show (hbox);

  gears = ctk_gears_new ();
  ctk_widget_set_hexpand (gears, TRUE);
  ctk_widget_set_vexpand (gears, TRUE);
  ctk_container_add (CTK_CONTAINER (hbox), gears);
  ctk_widget_show (gears);

  for (i = 0; i < CTK_GEARS_N_AXIS; i++)
    ctk_container_add (CTK_CONTAINER (hbox), create_axis_slider (CTK_GEARS (gears), i));

  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, FALSE);
  ctk_box_set_spacing (CTK_BOX (hbox), 6);
  ctk_container_add (CTK_CONTAINER (box), hbox);
  ctk_widget_show (hbox);

  fps_label = ctk_label_new ("");
  ctk_container_add (CTK_CONTAINER (hbox), fps_label);
  ctk_widget_show (fps_label);
  ctk_gears_set_fps_label (CTK_GEARS (gears), CTK_LABEL (fps_label));

  spinner = ctk_spinner_new ();
  ctk_box_pack_end (CTK_BOX (hbox), spinner, FALSE, FALSE, 0);
  ctk_widget_show (spinner);
  ctk_spinner_start (CTK_SPINNER (spinner));

  check = ctk_check_button_new_with_label ("Animate spinner");
  ctk_box_pack_end (CTK_BOX (hbox), check, FALSE, FALSE, 0);
  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (check), TRUE);
  ctk_widget_show (check);
  g_signal_connect (check, "toggled",
                    G_CALLBACK (toggle_spin), spinner);

  check = ctk_check_button_new_with_label ("Alpha");
  ctk_box_pack_end (CTK_BOX (hbox), check, FALSE, FALSE, 0);
  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (check), FALSE);
  ctk_widget_show (check);
  g_signal_connect (check, "toggled",
                    G_CALLBACK (toggle_alpha), gears);

  check = ctk_check_button_new_with_label ("Overlay");
  ctk_box_pack_end (CTK_BOX (hbox), check, FALSE, FALSE, 0);
  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (check), FALSE);
  ctk_widget_show (check);
  g_signal_connect (check, "toggled",
                    G_CALLBACK (toggle_overlay), revealer);
  button = ctk_menu_button_new ();
  ctk_menu_button_set_direction (CTK_MENU_BUTTON (button), CTK_ARROW_UP);
  popover = ctk_popover_new (NULL);
  ctk_container_set_border_width (CTK_CONTAINER (popover), 10);
  label = ctk_label_new ("Popovers work too!");
  ctk_widget_show (label);
  ctk_container_add (CTK_CONTAINER (popover), label);
  ctk_menu_button_set_popover (CTK_MENU_BUTTON (button), popover);
  ctk_widget_show (button);
  ctk_box_pack_end (CTK_BOX (hbox), button, FALSE, FALSE, 0);

  scrolled = ctk_scrolled_window_new (NULL, NULL);
  ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (scrolled),
                                  CTK_POLICY_AUTOMATIC,
                                  CTK_POLICY_NEVER);
  ctk_container_add (CTK_CONTAINER (box), scrolled);
  ctk_widget_show (scrolled);

  extra_hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, FALSE);
  ctk_box_set_spacing (CTK_BOX (extra_hbox), 6);
  ctk_container_add (CTK_CONTAINER (scrolled), extra_hbox);
  ctk_widget_show (extra_hbox);

  bbox = ctk_button_box_new (CTK_ORIENTATION_HORIZONTAL);
  ctk_box_set_spacing (CTK_BOX (bbox), 6);
  ctk_container_add (CTK_CONTAINER (box), bbox);
  ctk_widget_show (bbox);

  button = ctk_button_new_with_label ("Moar gears!");
  ctk_widget_set_hexpand (button, TRUE);
  ctk_container_add (CTK_CONTAINER (bbox), button);
  g_signal_connect (button, "clicked", G_CALLBACK (moar_gears), extra_hbox);
  ctk_widget_show (button);

  button = ctk_button_new_with_label ("Quit");
  ctk_widget_set_hexpand (button, TRUE);
  ctk_container_add (CTK_CONTAINER (bbox), button);
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (ctk_widget_destroy), window);
  ctk_widget_show (button);

  ctk_widget_show (window);

  ctk_main ();

  return EXIT_SUCCESS;
}
