#include <ctk/ctk.h>

static void
print_hello (CtkWidget *widget G_GNUC_UNUSED,
             gpointer   data G_GNUC_UNUSED)
{
  g_print ("Hello World\n");
}

static void
activate (CtkApplication *app,
          gpointer        user_data G_GNUC_UNUSED)
{
  CtkWidget *window;
  CtkWidget *button;
  CtkWidget *button_box;

  window = ctk_application_window_new (app);
  ctk_window_set_title (CTK_WINDOW (window), "Window");
  ctk_window_set_default_size (CTK_WINDOW (window), 200, 200);

  button_box = ctk_button_box_new (CTK_ORIENTATION_HORIZONTAL);
  ctk_container_add (CTK_CONTAINER (window), button_box);

  button = ctk_button_new_with_label ("Hello World");
  g_signal_connect (button, "clicked", G_CALLBACK (print_hello), NULL);
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (ctk_widget_destroy), window);
  ctk_container_add (CTK_CONTAINER (button_box), button);

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
