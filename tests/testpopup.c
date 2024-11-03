#include <ctk/ctk.h>

static gboolean
draw_popup (CtkWidget *widget G_GNUC_UNUSED,
            cairo_t   *cr,
            gpointer   data G_GNUC_UNUSED)
{
  cairo_set_source_rgb (cr, 1, 0, 0);
  cairo_paint (cr);

  return FALSE;
}

static gboolean
place_popup (CtkWidget *parent G_GNUC_UNUSED,
             CdkEvent  *event,
             CtkWidget *popup)
{
  CdkEventMotion *ev_motion = (CdkEventMotion *) event;
  gint width, height;

  ctk_window_get_size (CTK_WINDOW (popup), &width, &height);
  ctk_window_move (CTK_WINDOW (popup),
                   (int) ev_motion->x_root - width / 2,
                   (int) ev_motion->y_root - height / 2);

  return FALSE;
}

static gboolean
on_map_event (CtkWidget *parent,
              CdkEvent  *event G_GNUC_UNUSED,
              gpointer   data G_GNUC_UNUSED)
{
  CtkWidget *popup;

  popup = ctk_window_new (CTK_WINDOW_POPUP);

  ctk_widget_set_size_request (CTK_WIDGET (popup), 20, 20);
  ctk_widget_set_app_paintable (CTK_WIDGET (popup), TRUE);
  ctk_window_set_transient_for (CTK_WINDOW (popup), CTK_WINDOW (parent));
  g_signal_connect (popup, "draw", G_CALLBACK (draw_popup), NULL);
  g_signal_connect (parent, "motion-notify-event", G_CALLBACK (place_popup), popup);

  ctk_widget_show (popup);

  return FALSE;
}

int
main (int argc, char *argv[])
{
  CtkWidget *window;

  ctk_init (&argc, &argv);

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);

  ctk_widget_set_events (window, CDK_POINTER_MOTION_MASK);
  g_signal_connect (window, "destroy", ctk_main_quit, NULL);
  g_signal_connect (window, "map-event", G_CALLBACK (on_map_event), NULL);

  ctk_widget_show (window);

  ctk_main ();

  return 0;
}
