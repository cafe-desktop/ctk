#include <gtk/gtk.h>

static gboolean
window_key_press_event_cb (GtkWidget *window,
    GdkEvent *event,
    GtkSearchBar *search_bar)
{
  return ctk_search_bar_handle_event (search_bar, event);
}

static void
activate_cb (GtkApplication *app,
    gpointer user_data)
{
  GtkWidget *window;
  GtkWidget *search_bar;
  GtkWidget *box;
  GtkWidget *entry;
  GtkWidget *menu_button;

  window = ctk_application_window_new (app);
  ctk_widget_show (window);

  search_bar = ctk_search_bar_new ();
  ctk_container_add (CTK_CONTAINER (window), search_bar);
  ctk_widget_show (search_bar);

  box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 6);
  ctk_container_add (CTK_CONTAINER (search_bar), box);
  ctk_widget_show (box);

  entry = ctk_search_entry_new ();
  ctk_box_pack_start (CTK_BOX (box), entry, TRUE, TRUE, 0);
  ctk_widget_show (entry);

  menu_button = ctk_menu_button_new ();
  ctk_box_pack_start (CTK_BOX (box), menu_button, FALSE, FALSE, 0);
  ctk_widget_show (menu_button);

  ctk_search_bar_connect_entry (CTK_SEARCH_BAR (search_bar), CTK_ENTRY (entry));

  g_signal_connect (window, "key-press-event",
      G_CALLBACK (window_key_press_event_cb), search_bar);
}

gint
main (gint argc,
    gchar *argv[])
{
  GtkApplication *app;

  app = ctk_application_new ("org.gtk.Example.GtkSearchBar",
      G_APPLICATION_FLAGS_NONE);
  g_signal_connect (app, "activate",
      G_CALLBACK (activate_cb), NULL);

  return g_application_run (G_APPLICATION (app), argc, argv);
}
