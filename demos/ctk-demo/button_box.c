/* Button Boxes
 *
 * The Button Box widgets are used to arrange buttons with padding.
 */

#include <glib/gi18n.h>
#include <ctk/ctk.h>

static CtkWidget *
create_bbox (gint  horizontal,
             char *title,
             gint  spacing,
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

  button = ctk_button_new_with_label (_("OK"));
  ctk_container_add (CTK_CONTAINER (bbox), button);

  button = ctk_button_new_with_label (_("Cancel"));
  ctk_container_add (CTK_CONTAINER (bbox), button);

  button = ctk_button_new_with_label (_("Help"));
  ctk_container_add (CTK_CONTAINER (bbox), button);

  return frame;
}

CtkWidget *
do_button_box (CtkWidget *do_widget)
{
  static CtkWidget *window = NULL;
  CtkWidget *main_vbox;
  CtkWidget *vbox;
  CtkWidget *hbox;
  CtkWidget *frame_horz;
  CtkWidget *frame_vert;

  if (!window)
  {
    window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
    ctk_window_set_screen (CTK_WINDOW (window),
                           ctk_widget_get_screen (do_widget));
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
                        create_bbox (TRUE, "Spread", 40, CTK_BUTTONBOX_SPREAD),
                        TRUE, TRUE, 0);

    ctk_box_pack_start (CTK_BOX (vbox),
                        create_bbox (TRUE, "Edge", 40, CTK_BUTTONBOX_EDGE),
                        TRUE, TRUE, 5);

    ctk_box_pack_start (CTK_BOX (vbox),
                        create_bbox (TRUE, "Start", 40, CTK_BUTTONBOX_START),
                        TRUE, TRUE, 5);

    ctk_box_pack_start (CTK_BOX (vbox),
                        create_bbox (TRUE, "End", 40, CTK_BUTTONBOX_END),
                        TRUE, TRUE, 5);

    ctk_box_pack_start (CTK_BOX (vbox),
                        create_bbox (TRUE, "Center", 40, CTK_BUTTONBOX_CENTER),
                        TRUE, TRUE, 5);

    ctk_box_pack_start (CTK_BOX (vbox),
                        create_bbox (TRUE, "Expand", 0, CTK_BUTTONBOX_EXPAND),
                        TRUE, TRUE, 5);

    frame_vert = ctk_frame_new ("Vertical Button Boxes");
    ctk_box_pack_start (CTK_BOX (main_vbox), frame_vert, TRUE, TRUE, 10);

    hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
    ctk_container_set_border_width (CTK_CONTAINER (hbox), 10);
    ctk_container_add (CTK_CONTAINER (frame_vert), hbox);

    ctk_box_pack_start (CTK_BOX (hbox),
                        create_bbox (FALSE, "Spread", 10, CTK_BUTTONBOX_SPREAD),
                        TRUE, TRUE, 0);

    ctk_box_pack_start (CTK_BOX (hbox),
                        create_bbox (FALSE, "Edge", 10, CTK_BUTTONBOX_EDGE),
                        TRUE, TRUE, 5);

    ctk_box_pack_start (CTK_BOX (hbox),
                        create_bbox (FALSE, "Start", 10, CTK_BUTTONBOX_START),
                        TRUE, TRUE, 5);

    ctk_box_pack_start (CTK_BOX (hbox),
                        create_bbox (FALSE, "End", 10, CTK_BUTTONBOX_END),
                        TRUE, TRUE, 5);
    ctk_box_pack_start (CTK_BOX (hbox),
                        create_bbox (FALSE, "Center", 10, CTK_BUTTONBOX_CENTER),
                        TRUE, TRUE, 5);
    ctk_box_pack_start (CTK_BOX (hbox),
                        create_bbox (FALSE, "Expand", 0, CTK_BUTTONBOX_EXPAND),
                        TRUE, TRUE, 5);
  }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);

  return window;
}
