#include <gtk/gtk.h>
#define GTK_COMPILATION
#include "gtk/gtkplacesviewprivate.h"

int
main (int argc, char *argv[])
{
  GtkWidget *win;
  GtkWidget *view;

  ctk_init (&argc, &argv);

  win = ctk_window_new (GTK_WINDOW_TOPLEVEL);
  ctk_window_set_default_size (GTK_WINDOW (win), 400, 600);

  view = ctk_places_view_new ();

  ctk_container_add (GTK_CONTAINER (win), view);
  ctk_widget_show_all (win);

  g_signal_connect (win, "delete-event", G_CALLBACK (ctk_main_quit), win);

  ctk_main ();

  return 0;
}
