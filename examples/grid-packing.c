#include <gtk/gtk.h>

static void
print_hello (GtkWidget *widget,
             gpointer   data)
{
  g_print ("Hello World\n");
}

static void
activate (GtkApplication *app,
          gpointer        user_data)
{
  GtkWidget *window;
  GtkWidget *grid;
  GtkWidget *button;

  /* create a new window, and set its title */
  window = ctk_application_window_new (app);
  ctk_window_set_title (CTK_WINDOW (window), "Window");
  ctk_container_set_border_width (CTK_CONTAINER (window), 10);

  /* Here we construct the container that is going pack our buttons */
  grid = ctk_grid_new ();

  /* Pack the container in the window */
  ctk_container_add (CTK_CONTAINER (window), grid);

  button = ctk_button_new_with_label ("Button 1");
  g_signal_connect (button, "clicked", G_CALLBACK (print_hello), NULL);

  /* Place the first button in the grid cell (0, 0), and make it fill
   * just 1 cell horizontally and vertically (ie no spanning)
   */
  ctk_grid_attach (CTK_GRID (grid), button, 0, 0, 1, 1);

  button = ctk_button_new_with_label ("Button 2");
  g_signal_connect (button, "clicked", G_CALLBACK (print_hello), NULL);

  /* Place the second button in the grid cell (1, 0), and make it fill
   * just 1 cell horizontally and vertically (ie no spanning)
   */
  ctk_grid_attach (CTK_GRID (grid), button, 1, 0, 1, 1);

  button = ctk_button_new_with_label ("Quit");
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (ctk_widget_destroy), window);

  /* Place the Quit button in the grid cell (0, 1), and make it
   * span 2 columns.
   */
  ctk_grid_attach (CTK_GRID (grid), button, 0, 1, 2, 1);

  /* Now that we are done packing our widgets, we show them all
   * in one go, by calling ctk_widget_show_all() on the window.
   * This call recursively calls ctk_widget_show() on all widgets
   * that are contained in the window, directly or indirectly.
   */
  ctk_widget_show_all (window);

}

int
main (int    argc,
      char **argv)
{
  GtkApplication *app;
  int status;

  app = ctk_application_new ("org.gtk.example", G_APPLICATION_FLAGS_NONE);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  return status;
}
