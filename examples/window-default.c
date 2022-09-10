#include <ctk/ctk.h>

static void
activate (CtkApplication* app,
          gpointer        user_data)
{
  CtkWidget *window;

  window = ctk_application_window_new (app);
  ctk_window_set_title (CTK_WINDOW (window), "Window");
  ctk_window_set_default_size (CTK_WINDOW (window), 200, 200);
  ctk_widget_show_all (window);
}

int
main (int    argc,
      char **argv)
{
  CtkApplication *app;
  int status;

  app = ctk_application_new ("org.ctk.example", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  return status;
}
