#include <ctk/ctk.h>

/* Surface to store current scribbles */
static cairo_surface_t *surface = NULL;

static void
clear_surface (void)
{
  cairo_t *cr;

  cr = cairo_create (surface);

  cairo_set_source_rgb (cr, 1, 1, 1);
  cairo_paint (cr);

  cairo_destroy (cr);
}

/* Create a new surface of the appropriate size to store our scribbles */
static gboolean
configure_event_cb (CtkWidget         *widget,
                    CdkEventConfigure *event,
                    gpointer           data)
{
  if (surface)
    cairo_surface_destroy (surface);

  surface = cdk_window_create_similar_surface (ctk_widget_get_window (widget),
                                               CAIRO_CONTENT_COLOR,
                                               ctk_widget_get_allocated_width (widget),
                                               ctk_widget_get_allocated_height (widget));

  /* Initialize the surface to white */
  clear_surface ();

  /* We've handled the configure event, no need for further processing. */
  return TRUE;
}

/* Redraw the screen from the surface. Note that the ::draw
 * signal receives a ready-to-be-used cairo_t that is already
 * clipped to only draw the exposed areas of the widget
 */
static gboolean
draw_cb (CtkWidget *widget,
         cairo_t   *cr,
         gpointer   data)
{
  cairo_set_source_surface (cr, surface, 0, 0);
  cairo_paint (cr);

  return FALSE;
}

/* Draw a rectangle on the surface at the given position */
static void
draw_brush (CtkWidget *widget,
            gdouble    x,
            gdouble    y)
{
  cairo_t *cr;

  /* Paint to the surface, where we store our state */
  cr = cairo_create (surface);

  cairo_rectangle (cr, x - 3, y - 3, 6, 6);
  cairo_fill (cr);

  cairo_destroy (cr);

  /* Now invalidate the affected region of the drawing area. */
  ctk_widget_queue_draw_area (widget, x - 3, y - 3, 6, 6);
}

/* Handle button press events by either drawing a rectangle
 * or clearing the surface, depending on which button was pressed.
 * The ::button-press signal handler receives a CdkEventButton
 * struct which contains this information.
 */
static gboolean
button_press_event_cb (CtkWidget      *widget,
                       CdkEventButton *event,
                       gpointer        data)
{
  /* paranoia check, in case we haven't gotten a configure event */
  if (surface == NULL)
    return FALSE;

  if (event->button == CDK_BUTTON_PRIMARY)
    {
      draw_brush (widget, event->x, event->y);
    }
  else if (event->button == CDK_BUTTON_SECONDARY)
    {
      clear_surface ();
      ctk_widget_queue_draw (widget);
    }

  /* We've handled the event, stop processing */
  return TRUE;
}

/* Handle motion events by continuing to draw if button 1 is
 * still held down. The ::motion-notify signal handler receives
 * a CdkEventMotion struct which contains this information.
 */
static gboolean
motion_notify_event_cb (CtkWidget      *widget,
                        CdkEventMotion *event,
                        gpointer        data)
{
  /* paranoia check, in case we haven't gotten a configure event */
  if (surface == NULL)
    return FALSE;

  if (event->state & CDK_BUTTON1_MASK)
    draw_brush (widget, event->x, event->y);

  /* We've handled it, stop processing */
  return TRUE;
}

static void
close_window (void)
{
  if (surface)
    cairo_surface_destroy (surface);
}

static void
activate (CtkApplication *app,
          gpointer        user_data)
{
  CtkWidget *window;
  CtkWidget *frame;
  CtkWidget *drawing_area;

  window = ctk_application_window_new (app);
  ctk_window_set_title (CTK_WINDOW (window), "Drawing Area");

  g_signal_connect (window, "destroy", G_CALLBACK (close_window), NULL);

  ctk_container_set_border_width (CTK_CONTAINER (window), 8);

  frame = ctk_frame_new (NULL);
  ctk_frame_set_shadow_type (CTK_FRAME (frame), CTK_SHADOW_IN);
  ctk_container_add (CTK_CONTAINER (window), frame);

  drawing_area = ctk_drawing_area_new ();
  /* set a minimum size */
  ctk_widget_set_size_request (drawing_area, 100, 100);

  ctk_container_add (CTK_CONTAINER (frame), drawing_area);

  /* Signals used to handle the backing surface */
  g_signal_connect (drawing_area, "draw",
                    G_CALLBACK (draw_cb), NULL);
  g_signal_connect (drawing_area,"configure-event",
                    G_CALLBACK (configure_event_cb), NULL);

  /* Event signals */
  g_signal_connect (drawing_area, "motion-notify-event",
                    G_CALLBACK (motion_notify_event_cb), NULL);
  g_signal_connect (drawing_area, "button-press-event",
                    G_CALLBACK (button_press_event_cb), NULL);

  /* Ask to receive events the drawing area doesn't normally
   * subscribe to. In particular, we need to ask for the
   * button press and motion notify events that want to handle.
   */
  ctk_widget_set_events (drawing_area, ctk_widget_get_events (drawing_area)
                                     | CDK_BUTTON_PRESS_MASK
                                     | CDK_POINTER_MOTION_MASK);

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
