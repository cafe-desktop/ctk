#include <stdlib.h>
#include <gtk/gtk.h>

#include "gtkgears.h"

int
main (int argc, char *argv[])
{
  GtkWidget *window, *fixed, *gears, *spinner;

  ctk_init (&argc, &argv);

  window = ctk_window_new (GTK_WINDOW_TOPLEVEL);
  ctk_window_set_title (GTK_WINDOW (window), "Test GL/gtk inter-blending");
  ctk_window_set_default_size (GTK_WINDOW (window), 250, 250);
  ctk_container_set_border_width (GTK_CONTAINER (window), 12);
  g_signal_connect (window, "destroy", G_CALLBACK (ctk_main_quit), NULL);

  fixed = ctk_fixed_new ();
  ctk_container_add (GTK_CONTAINER (window), fixed);

  spinner = ctk_spinner_new ();
  ctk_spinner_start (GTK_SPINNER (spinner));
  ctk_widget_set_size_request (spinner, 50, 50);
  ctk_fixed_put (GTK_FIXED (fixed), spinner, 90, 80);

  spinner = ctk_spinner_new ();
  ctk_spinner_start (GTK_SPINNER (spinner));
  ctk_widget_set_size_request (spinner, 50, 50);
  ctk_fixed_put (GTK_FIXED (fixed), spinner, 100, 80);

  spinner = ctk_spinner_new ();
  ctk_spinner_start (GTK_SPINNER (spinner));
  ctk_widget_set_size_request (spinner, 50, 50);
  ctk_fixed_put (GTK_FIXED (fixed), spinner, 110, 80);


  gears = ctk_gears_new ();
  ctk_widget_set_size_request (gears, 70, 50);
  ctk_fixed_put (GTK_FIXED (fixed), gears, 60, 100);

  gears = ctk_gears_new ();
  ctk_gl_area_set_has_alpha (GTK_GL_AREA (gears), TRUE);
  ctk_widget_set_size_request (gears, 70, 50);
  ctk_fixed_put (GTK_FIXED (fixed), gears, 120, 100);


  spinner = ctk_spinner_new ();
  ctk_spinner_start (GTK_SPINNER (spinner));
  ctk_widget_set_size_request (spinner, 50, 50);
  ctk_fixed_put (GTK_FIXED (fixed), spinner, 90, 110);

  spinner = ctk_spinner_new ();
  ctk_spinner_start (GTK_SPINNER (spinner));
  ctk_widget_set_size_request (spinner, 50, 50);
  ctk_fixed_put (GTK_FIXED (fixed), spinner, 100, 110);

  spinner = ctk_spinner_new ();
  ctk_spinner_start (GTK_SPINNER (spinner));
  ctk_widget_set_size_request (spinner, 50, 50);
  ctk_fixed_put (GTK_FIXED (fixed), spinner, 110, 110);


  gears = ctk_gears_new ();
  ctk_widget_set_size_request (gears, 70, 50);
  ctk_fixed_put (GTK_FIXED (fixed), gears, 60, 130);

  gears = ctk_gears_new ();
  ctk_gl_area_set_has_alpha (GTK_GL_AREA (gears), TRUE);
  ctk_widget_set_size_request (gears, 70, 50);
  ctk_fixed_put (GTK_FIXED (fixed), gears, 120, 130);


  spinner = ctk_spinner_new ();
  ctk_spinner_start (GTK_SPINNER (spinner));
  ctk_widget_set_size_request (spinner, 50, 50);
  ctk_fixed_put (GTK_FIXED (fixed), spinner, 90, 150);

  spinner = ctk_spinner_new ();
  ctk_spinner_start (GTK_SPINNER (spinner));
  ctk_widget_set_size_request (spinner, 50, 50);
  ctk_fixed_put (GTK_FIXED (fixed), spinner, 100, 150);

  spinner = ctk_spinner_new ();
  ctk_spinner_start (GTK_SPINNER (spinner));
  ctk_widget_set_size_request (spinner, 50, 50);
  ctk_fixed_put (GTK_FIXED (fixed), spinner, 110, 150);

  ctk_widget_show_all (window);

  ctk_main ();

  return EXIT_SUCCESS;
}
