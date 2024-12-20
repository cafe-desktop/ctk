/* Drawing Area
 *
 * CtkDrawingArea is a blank area where you can draw custom displays
 * of various kinds.
 *
 * This demo has two drawing areas. The checkerboard area shows
 * how you can just draw something; all you have to do is write
 * a signal handler for expose_event, as shown here.
 *
 * The "scribble" area is a bit more advanced, and shows how to handle
 * events such as button presses and mouse motion. Click the mouse
 * and drag in the scribble area to draw squiggles. Resize the window
 * to clear the area.
 */

#include <ctk/ctk.h>

static CtkWidget *window = NULL;
/* Pixmap for scribble area, to store current scribbles */
static cairo_surface_t *surface = NULL;

/* Create a new surface of the appropriate size to store our scribbles */
static gboolean
scribble_configure_event (CtkWidget         *widget,
                          CdkEventConfigure *event G_GNUC_UNUSED,
                          gpointer           data G_GNUC_UNUSED)
{
  CtkAllocation allocation;
  cairo_t *cr;

  if (surface)
    cairo_surface_destroy (surface);

  ctk_widget_get_allocation (widget, &allocation);
  surface = cdk_window_create_similar_surface (ctk_widget_get_window (widget),
                                               CAIRO_CONTENT_COLOR,
                                               allocation.width,
                                               allocation.height);

  /* Initialize the surface to white */
  cr = cairo_create (surface);

  cairo_set_source_rgb (cr, 1, 1, 1);
  cairo_paint (cr);

  cairo_destroy (cr);

  /* We've handled the configure event, no need for further processing. */
  return TRUE;
}

/* Redraw the screen from the surface */
static gboolean
scribble_draw (CtkWidget *widget G_GNUC_UNUSED,
               cairo_t   *cr,
               gpointer   data G_GNUC_UNUSED)
{
  cairo_set_source_surface (cr, surface, 0, 0);
  cairo_paint (cr);

  return FALSE;
}

/* Draw a rectangle on the screen */
static void
draw_brush (CtkWidget *widget,
            gdouble    x,
            gdouble    y)
{
  CdkRectangle update_rect;
  cairo_t *cr;

  update_rect.x = x - 3;
  update_rect.y = y - 3;
  update_rect.width = 6;
  update_rect.height = 6;

  /* Paint to the surface, where we store our state */
  cr = cairo_create (surface);

  cdk_cairo_rectangle (cr, &update_rect);
  cairo_fill (cr);

  cairo_destroy (cr);

  /* Now invalidate the affected region of the drawing area. */
  cdk_window_invalidate_rect (ctk_widget_get_window (widget),
                              &update_rect,
                              FALSE);
}

static gboolean
scribble_button_press_event (CtkWidget      *widget,
                             CdkEventButton *event,
                             gpointer        data G_GNUC_UNUSED)
{
  if (surface == NULL)
    return FALSE; /* paranoia check, in case we haven't gotten a configure event */

  if (event->button == CDK_BUTTON_PRIMARY)
    draw_brush (widget, event->x, event->y);

  /* We've handled the event, stop processing */
  return TRUE;
}

static gboolean
scribble_motion_notify_event (CtkWidget      *widget,
                              CdkEventMotion *event,
                              gpointer        data G_GNUC_UNUSED)
{
  int x, y;
  CdkModifierType state;

  if (surface == NULL)
    return FALSE; /* paranoia check, in case we haven't gotten a configure event */

  /* This call is very important; it requests the next motion event.
   * If you don't call cdk_window_get_pointer() you'll only get
   * a single motion event. The reason is that we specified
   * CDK_POINTER_MOTION_HINT_MASK to ctk_widget_set_events().
   * If we hadn't specified that, we could just use event->x, event->y
   * as the pointer location. But we'd also get deluged in events.
   * By requesting the next event as we handle the current one,
   * we avoid getting a huge number of events faster than we
   * can cope.
   */

  cdk_window_get_device_position (event->window, event->device, &x, &y, &state);

  if (state & CDK_BUTTON1_MASK)
    draw_brush (widget, x, y);

  /* We've handled it, stop processing */
  return TRUE;
}


static gboolean
checkerboard_draw (CtkWidget *da,
                   cairo_t   *cr,
                   gpointer   data G_GNUC_UNUSED)
{
  gint i, j, xcount, ycount, width, height;

#define CHECK_SIZE 10
#define SPACING 2

  /* At the start of a draw handler, a clip region has been set on
   * the Cairo context, and the contents have been cleared to the
   * widget's background color. The docs for
   * cdk_window_begin_paint_region() give more details on how this
   * works.
   */

  xcount = 0;
  width = ctk_widget_get_allocated_width (da);
  height = ctk_widget_get_allocated_height (da);
  i = SPACING;
  while (i < width)
    {
      j = SPACING;
      ycount = xcount % 2; /* start with even/odd depending on row */
      while (j < height)
        {
          if (ycount % 2)
            cairo_set_source_rgb (cr, 0.45777, 0, 0.45777);
          else
            cairo_set_source_rgb (cr, 1, 1, 1);

          /* If we're outside the clip, this will do nothing.
           */
          cairo_rectangle (cr, i, j, CHECK_SIZE, CHECK_SIZE);
          cairo_fill (cr);

          j += CHECK_SIZE + SPACING;
          ++ycount;
        }

      i += CHECK_SIZE + SPACING;
      ++xcount;
    }

  /* return TRUE because we've handled this event, so no
   * further processing is required.
   */
  return TRUE;
}

static void
close_window (void)
{
  window = NULL;

  if (surface)
    cairo_surface_destroy (surface);
  surface = NULL;
}

CtkWidget *
do_drawingarea (CtkWidget *do_widget)
{
  if (!window)
    {
      CtkWidget *frame;
      CtkWidget *vbox;
      CtkWidget *da;
      CtkWidget *label;

      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_screen (CTK_WINDOW (window),
                             ctk_widget_get_screen (do_widget));
      ctk_window_set_title (CTK_WINDOW (window), "Drawing Area");

      g_signal_connect (window, "destroy",
                        G_CALLBACK (close_window), NULL);

      ctk_container_set_border_width (CTK_CONTAINER (window), 8);

      vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 8);
      ctk_container_set_border_width (CTK_CONTAINER (vbox), 8);
      ctk_container_add (CTK_CONTAINER (window), vbox);

      /*
       * Create the checkerboard area
       */

      label = ctk_label_new (NULL);
      ctk_label_set_markup (CTK_LABEL (label),
                            "<u>Checkerboard pattern</u>");
      ctk_box_pack_start (CTK_BOX (vbox), label, FALSE, FALSE, 0);

      frame = ctk_frame_new (NULL);
      ctk_frame_set_shadow_type (CTK_FRAME (frame), CTK_SHADOW_IN);
      ctk_box_pack_start (CTK_BOX (vbox), frame, TRUE, TRUE, 0);

      da = ctk_drawing_area_new ();
      /* set a minimum size */
      ctk_widget_set_size_request (da, 100, 100);

      ctk_container_add (CTK_CONTAINER (frame), da);

      g_signal_connect (da, "draw",
                        G_CALLBACK (checkerboard_draw), NULL);

      /*
       * Create the scribble area
       */

      label = ctk_label_new (NULL);
      ctk_label_set_markup (CTK_LABEL (label),
                            "<u>Scribble area</u>");
      ctk_box_pack_start (CTK_BOX (vbox), label, FALSE, FALSE, 0);

      frame = ctk_frame_new (NULL);
      ctk_frame_set_shadow_type (CTK_FRAME (frame), CTK_SHADOW_IN);
      ctk_box_pack_start (CTK_BOX (vbox), frame, TRUE, TRUE, 0);

      da = ctk_drawing_area_new ();
      /* set a minimum size */
      ctk_widget_set_size_request (da, 100, 100);

      ctk_container_add (CTK_CONTAINER (frame), da);

      /* Signals used to handle backing surface */

      g_signal_connect (da, "draw",
                        G_CALLBACK (scribble_draw), NULL);
      g_signal_connect (da,"configure-event",
                        G_CALLBACK (scribble_configure_event), NULL);

      /* Event signals */

      g_signal_connect (da, "motion-notify-event",
                        G_CALLBACK (scribble_motion_notify_event), NULL);
      g_signal_connect (da, "button-press-event",
                        G_CALLBACK (scribble_button_press_event), NULL);


      /* Ask to receive events the drawing area doesn't normally
       * subscribe to
       */
      ctk_widget_set_events (da, ctk_widget_get_events (da)
                             | CDK_LEAVE_NOTIFY_MASK
                             | CDK_BUTTON_PRESS_MASK
                             | CDK_POINTER_MOTION_MASK
                             | CDK_POINTER_MOTION_HINT_MASK);

    }

  if (!ctk_widget_get_visible (window))
      ctk_widget_show_all (window);
  else
      ctk_widget_destroy (window);

  return window;
}
