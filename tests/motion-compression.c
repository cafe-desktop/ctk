#include <ctk/ctk.h>
#include <math.h>

GtkAdjustment *adjustment;
int cursor_x, cursor_y;

static void
on_motion_notify (GtkWidget      *window,
                  GdkEventMotion *event)
{
  if (event->window == ctk_widget_get_window (window))
    {
      float processing_ms = ctk_adjustment_get_value (adjustment);
      g_usleep (processing_ms * 1000);
      cursor_x = event->x;
      cursor_y = event->y;
      ctk_widget_queue_draw (window);
    }
}

static void
on_draw (GtkWidget *window,
         cairo_t   *cr)
{
  cairo_set_source_rgb (cr, 1, 1, 1);
  cairo_paint (cr);

  cairo_set_source_rgb (cr, 0, 0.5, 0.5);

  cairo_arc (cr, cursor_x, cursor_y, 10, 0, 2 * M_PI);
  cairo_stroke (cr);
}

int
main (int argc, char **argv)
{
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *scale;

  ctk_init (&argc, &argv);

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_default_size (CTK_WINDOW (window), 300, 300);
  ctk_widget_set_app_paintable (window, TRUE);
  ctk_widget_add_events (window, GDK_POINTER_MOTION_MASK);
  ctk_widget_set_app_paintable (window, TRUE);

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  ctk_container_add (CTK_CONTAINER (window), vbox);

  adjustment = ctk_adjustment_new (20, 0, 200, 1, 10, 0);
  scale = ctk_scale_new (CTK_ORIENTATION_HORIZONTAL, adjustment);
  ctk_box_pack_end (CTK_BOX (vbox), scale, FALSE, FALSE, 0);

  label = ctk_label_new ("Event processing time (ms):");
  ctk_widget_set_halign (label, CTK_ALIGN_CENTER);
  ctk_box_pack_end (CTK_BOX (vbox), label, FALSE, FALSE, 0);

  g_signal_connect (window, "motion-notify-event",
                    G_CALLBACK (on_motion_notify), NULL);
  g_signal_connect (window, "draw",
                    G_CALLBACK (on_draw), NULL);
  g_signal_connect (window, "destroy",
                    G_CALLBACK (ctk_main_quit), NULL);

  ctk_widget_show_all (window);
  ctk_main ();

  return 0;
}
