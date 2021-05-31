/* Header Bar
 *
 * CtkHeaderBar is a container that is suitable for implementing
 * window titlebars. One of its features is that it can position
 * a title (and optional subtitle) centered with regard to the
 * full width, regardless of variable-width content at the left
 * or right.
 *
 * It is commonly used with ctk_window_set_titlebar()
 */

#include <ctk/ctk.h>

CtkWidget *
do_headerbar (CtkWidget *do_widget)
{
  static CtkWidget *window = NULL;
  CtkWidget *header;
  CtkWidget *button;
  CtkWidget *box;
  CtkWidget *image;
  GIcon *icon;

  if (!window)
    {
      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_screen (CTK_WINDOW (window), ctk_widget_get_screen (do_widget));
      g_signal_connect (window, "destroy",
                        G_CALLBACK (ctk_widget_destroyed), &window);
      ctk_window_set_default_size (CTK_WINDOW (window), 600, 400);

      header = ctk_header_bar_new ();
      ctk_header_bar_set_show_close_button (CTK_HEADER_BAR (header), TRUE);
      ctk_header_bar_set_title (CTK_HEADER_BAR (header), "Welcome to Facebook - Log in, sign up or learn more");
      ctk_header_bar_set_has_subtitle (CTK_HEADER_BAR (header), FALSE);

      button = ctk_button_new ();
      icon = g_themed_icon_new ("mail-send-receive-symbolic");
      image = ctk_image_new_from_gicon (icon, CTK_ICON_SIZE_BUTTON);
      g_object_unref (icon);
      ctk_container_add (CTK_CONTAINER (button), image);
      ctk_header_bar_pack_end (CTK_HEADER_BAR (header), button);

      box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
      ctk_style_context_add_class (ctk_widget_get_style_context (box), "linked");
      button = ctk_button_new ();
      ctk_container_add (CTK_CONTAINER (button), ctk_image_new_from_icon_name ("pan-start-symbolic", CTK_ICON_SIZE_BUTTON));
      ctk_container_add (CTK_CONTAINER (box), button);
      button = ctk_button_new ();
      ctk_container_add (CTK_CONTAINER (button), ctk_image_new_from_icon_name ("pan-end-symbolic", CTK_ICON_SIZE_BUTTON));
      ctk_container_add (CTK_CONTAINER (box), button);

      ctk_header_bar_pack_start (CTK_HEADER_BAR (header), box);

      ctk_window_set_titlebar (CTK_WINDOW (window), header);

      ctk_container_add (CTK_CONTAINER (window), ctk_text_view_new ());
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);

  return window;
}
