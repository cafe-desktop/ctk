#include <ctk/ctk.h>

static const gchar css[] =
 ".main.background { "
 " background-image: -ctk-gradient (linear, center top, center bottom, "
 "      from (red), "
 "      to (blue)); "
 " border-width: 0px; "
 "}"
 ".titlebar.backdrop { "
 " background-image: none; "
 " background-color: @bg_color; "
 " border-radius: 10px 10px 0px 0px; "
 "}"
 ".titlebar { "
 " background-image: -ctk-gradient (linear, center top, center bottom, "
 "      from (white), "
 "      to (@bg_color)); "
 " border-radius: 10px 10px 0px 0px; "
 "}";

static void
on_bookmark_clicked (GtkButton *button, gpointer data)
{
  GtkWindow *window = CTK_WINDOW (data);
  GtkWidget *chooser;

  chooser = ctk_file_chooser_dialog_new ("File Chooser Test",
                                         window,
                                         CTK_FILE_CHOOSER_ACTION_OPEN,
                                         "_Close",
                                         CTK_RESPONSE_CLOSE,
                                         NULL);

  g_signal_connect (chooser, "response",
                    G_CALLBACK (ctk_widget_destroy), NULL);

  ctk_widget_show (chooser);
}

static GtkWidget *header;

static void
change_subtitle (GtkButton *button, gpointer data)
{
  if (!CTK_IS_HEADER_BAR (header))
    return;

  if (ctk_header_bar_get_subtitle (CTK_HEADER_BAR (header)) == NULL)
    {
      ctk_header_bar_set_subtitle (CTK_HEADER_BAR (header), "(subtle subtitle)");
    }
  else
    {
      ctk_header_bar_set_subtitle (CTK_HEADER_BAR (header), NULL);
    }
}

static void
toggle_fullscreen (GtkButton *button, gpointer data)
{
  GtkWidget *window = CTK_WIDGET (data);
  static gboolean fullscreen = FALSE;

  if (fullscreen)
    {
      ctk_window_unfullscreen (CTK_WINDOW (window));
      fullscreen = FALSE;
    }
  else
    {
      ctk_window_fullscreen (CTK_WINDOW (window));
      fullscreen = TRUE;
    }
}

static void
change_header (GtkButton *button, gpointer data)
{
  GtkWidget *window = CTK_WIDGET (data);
  GtkWidget *label;
  GtkWidget *widget;
  GtkWidget *image;
  GtkWidget *box;

  if (button && ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (button)))
    {
      header = ctk_event_box_new ();
      ctk_style_context_add_class (ctk_widget_get_style_context (header), "titlebar");
      ctk_style_context_add_class (ctk_widget_get_style_context (header), "header-bar");
      box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 10);
      g_object_set (box, "margin", 10, NULL);
      ctk_container_add (CTK_CONTAINER (header), box);
      label = ctk_label_new ("Label");
      ctk_box_pack_start (CTK_BOX (box), label, FALSE, TRUE, 0);
      widget = ctk_level_bar_new ();
      ctk_level_bar_set_value (CTK_LEVEL_BAR (widget), 0.4);
      ctk_box_pack_start (CTK_BOX (box), widget, TRUE, TRUE, 0);
      ctk_widget_show_all (header);
    }
  else
    {
      header = ctk_header_bar_new ();
      ctk_style_context_add_class (ctk_widget_get_style_context (header), "titlebar");
      ctk_header_bar_set_title (CTK_HEADER_BAR (header), "Example header");

      widget = ctk_button_new_with_label ("_Close");
      ctk_button_set_use_underline (CTK_BUTTON (widget), TRUE);
      ctk_style_context_add_class (ctk_widget_get_style_context (widget), "suggested-action");
      g_signal_connect (widget, "clicked", G_CALLBACK (ctk_main_quit), NULL);

      ctk_header_bar_pack_end (CTK_HEADER_BAR (header), widget);

      widget= ctk_button_new ();
      image = ctk_image_new_from_icon_name ("bookmark-new-symbolic", CTK_ICON_SIZE_BUTTON);
      g_signal_connect (widget, "clicked", G_CALLBACK (on_bookmark_clicked), window);
      ctk_container_add (CTK_CONTAINER (widget), image);

      ctk_header_bar_pack_start (CTK_HEADER_BAR (header), widget);
      ctk_widget_show_all (header);
    }

  ctk_window_set_titlebar (CTK_WINDOW (window), header);
}

int
main (int argc, char *argv[])
{
  GtkWidget *window;
  GtkWidget *box;
  GtkWidget *footer;
  GtkWidget *button;
  GtkWidget *content;
  GtkCssProvider *provider;

  ctk_init (NULL, NULL);

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_style_context_add_class (ctk_widget_get_style_context (window), "main");

  provider = ctk_css_provider_new ();
  ctk_css_provider_load_from_data (provider, css, -1, NULL);
  ctk_style_context_add_provider_for_screen (ctk_widget_get_screen (window),
                                             CTK_STYLE_PROVIDER (provider),
                                             CTK_STYLE_PROVIDER_PRIORITY_USER);


  change_header (NULL, window);

  box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  ctk_container_add (CTK_CONTAINER (window), box);

  footer = ctk_action_bar_new ();
  ctk_action_bar_set_center_widget (CTK_ACTION_BAR (footer), ctk_check_button_new_with_label ("Middle"));
  button = ctk_toggle_button_new_with_label ("Custom");
  g_signal_connect (button, "clicked", G_CALLBACK (change_header), window);
  ctk_action_bar_pack_start (CTK_ACTION_BAR (footer), button);
  button = ctk_button_new_with_label ("Subtitle");
  g_signal_connect (button, "clicked", G_CALLBACK (change_subtitle), NULL);
  ctk_action_bar_pack_end (CTK_ACTION_BAR (footer), button);
  button = ctk_button_new_with_label ("Fullscreen");
  ctk_action_bar_pack_end (CTK_ACTION_BAR (footer), button);
  g_signal_connect (button, "clicked", G_CALLBACK (toggle_fullscreen), window);
  ctk_box_pack_end (CTK_BOX (box), footer, FALSE, FALSE, 0);

  content = ctk_image_new_from_icon_name ("start-here-symbolic", CTK_ICON_SIZE_DIALOG);
  ctk_image_set_pixel_size (CTK_IMAGE (content), 512);

  ctk_box_pack_start (CTK_BOX (box), content, FALSE, TRUE, 0);

  ctk_widget_show_all (window);

  ctk_main ();

  ctk_widget_destroy (window);

  return 0;
}
