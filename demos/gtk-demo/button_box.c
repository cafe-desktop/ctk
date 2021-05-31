/* Button Boxes
 *
 * The Button Box widgets are used to arrange buttons with padding.
 */

#include <glib/gi18n.h>
#include <gtk/gtk.h>

static GtkWidget *
create_bbox (gint  horizontal,
             char *title,
             gint  spacing,
             gint  layout)
{
  GtkWidget *frame;
  GtkWidget *bbox;
  GtkWidget *button;

  frame = ctk_frame_new (title);

  if (horizontal)
    bbox = ctk_button_box_new (GTK_ORIENTATION_HORIZONTAL);
  else
    bbox = ctk_button_box_new (GTK_ORIENTATION_VERTICAL);

  ctk_container_set_border_width (GTK_CONTAINER (bbox), 5);
  ctk_container_add (GTK_CONTAINER (frame), bbox);

  ctk_button_box_set_layout (GTK_BUTTON_BOX (bbox), layout);
  ctk_box_set_spacing (GTK_BOX (bbox), spacing);

  button = ctk_button_new_with_label (_("OK"));
  ctk_container_add (GTK_CONTAINER (bbox), button);

  button = ctk_button_new_with_label (_("Cancel"));
  ctk_container_add (GTK_CONTAINER (bbox), button);

  button = ctk_button_new_with_label (_("Help"));
  ctk_container_add (GTK_CONTAINER (bbox), button);

  return frame;
}

GtkWidget *
do_button_box (GtkWidget *do_widget)
{
  static GtkWidget *window = NULL;
  GtkWidget *main_vbox;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *frame_horz;
  GtkWidget *frame_vert;

  if (!window)
  {
    window = ctk_window_new (GTK_WINDOW_TOPLEVEL);
    ctk_window_set_screen (GTK_WINDOW (window),
                           ctk_widget_get_screen (do_widget));
    ctk_window_set_title (GTK_WINDOW (window), "Button Boxes");

    g_signal_connect (window, "destroy",
                      G_CALLBACK (ctk_widget_destroyed),
                      &window);

    ctk_container_set_border_width (GTK_CONTAINER (window), 10);

    main_vbox = ctk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    ctk_container_add (GTK_CONTAINER (window), main_vbox);

    frame_horz = ctk_frame_new ("Horizontal Button Boxes");
    ctk_box_pack_start (GTK_BOX (main_vbox), frame_horz, TRUE, TRUE, 10);

    vbox = ctk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    ctk_container_set_border_width (GTK_CONTAINER (vbox), 10);
    ctk_container_add (GTK_CONTAINER (frame_horz), vbox);

    ctk_box_pack_start (GTK_BOX (vbox),
                        create_bbox (TRUE, "Spread", 40, GTK_BUTTONBOX_SPREAD),
                        TRUE, TRUE, 0);

    ctk_box_pack_start (GTK_BOX (vbox),
                        create_bbox (TRUE, "Edge", 40, GTK_BUTTONBOX_EDGE),
                        TRUE, TRUE, 5);

    ctk_box_pack_start (GTK_BOX (vbox),
                        create_bbox (TRUE, "Start", 40, GTK_BUTTONBOX_START),
                        TRUE, TRUE, 5);

    ctk_box_pack_start (GTK_BOX (vbox),
                        create_bbox (TRUE, "End", 40, GTK_BUTTONBOX_END),
                        TRUE, TRUE, 5);

    ctk_box_pack_start (GTK_BOX (vbox),
                        create_bbox (TRUE, "Center", 40, GTK_BUTTONBOX_CENTER),
                        TRUE, TRUE, 5);

    ctk_box_pack_start (GTK_BOX (vbox),
                        create_bbox (TRUE, "Expand", 0, GTK_BUTTONBOX_EXPAND),
                        TRUE, TRUE, 5);

    frame_vert = ctk_frame_new ("Vertical Button Boxes");
    ctk_box_pack_start (GTK_BOX (main_vbox), frame_vert, TRUE, TRUE, 10);

    hbox = ctk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    ctk_container_set_border_width (GTK_CONTAINER (hbox), 10);
    ctk_container_add (GTK_CONTAINER (frame_vert), hbox);

    ctk_box_pack_start (GTK_BOX (hbox),
                        create_bbox (FALSE, "Spread", 10, GTK_BUTTONBOX_SPREAD),
                        TRUE, TRUE, 0);

    ctk_box_pack_start (GTK_BOX (hbox),
                        create_bbox (FALSE, "Edge", 10, GTK_BUTTONBOX_EDGE),
                        TRUE, TRUE, 5);

    ctk_box_pack_start (GTK_BOX (hbox),
                        create_bbox (FALSE, "Start", 10, GTK_BUTTONBOX_START),
                        TRUE, TRUE, 5);

    ctk_box_pack_start (GTK_BOX (hbox),
                        create_bbox (FALSE, "End", 10, GTK_BUTTONBOX_END),
                        TRUE, TRUE, 5);
    ctk_box_pack_start (GTK_BOX (hbox),
                        create_bbox (FALSE, "Center", 10, GTK_BUTTONBOX_CENTER),
                        TRUE, TRUE, 5);
    ctk_box_pack_start (GTK_BOX (hbox),
                        create_bbox (FALSE, "Expand", 0, GTK_BUTTONBOX_EXPAND),
                        TRUE, TRUE, 5);
  }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);

  return window;
}
