/* Gestures
 *
 * Perform gestures on touchscreens and other input devices. This
 * demo reacts to long presses and swipes from all devices, plus
 * multi-touch rotate and zoom gestures.
 */

#include <ctk/ctk.h>

static GtkGesture *rotate = NULL;
static GtkGesture *zoom = NULL;
static gdouble swipe_x = 0;
static gdouble swipe_y = 0;
static gboolean long_pressed = FALSE;

static gboolean
touchpad_swipe_gesture_begin (GtkGesture       *gesture,
                              GdkEventSequence *sequence,
                              GtkWidget        *widget)
{
  /* Disallow touchscreen events here */
  if (sequence != NULL)
    ctk_gesture_set_state (gesture, CTK_EVENT_SEQUENCE_DENIED);
  return sequence == NULL;
}

static void
swipe_gesture_swept (GtkGestureSwipe *gesture,
                     gdouble          velocity_x,
                     gdouble          velocity_y,
                     GtkWidget       *widget)
{
  swipe_x = velocity_x / 10;
  swipe_y = velocity_y / 10;
  ctk_widget_queue_draw (widget);
}

static void
long_press_gesture_pressed (GtkGestureLongPress *gesture,
                            gdouble              x,
                            gdouble              y,
                            GtkWidget           *widget)
{
  long_pressed = TRUE;
  ctk_widget_queue_draw (widget);
}

static void
long_press_gesture_end (GtkGesture       *gesture,
                        GdkEventSequence *sequence,
                        GtkWidget        *widget)
{
  long_pressed = FALSE;
  ctk_widget_queue_draw (widget);
}

static void
rotation_angle_changed (GtkGestureRotate *gesture,
                        gdouble           angle,
                        gdouble           delta,
                        GtkWidget        *widget)
{
  ctk_widget_queue_draw (widget);
}

static void
zoom_scale_changed (GtkGestureZoom *gesture,
                    gdouble         scale,
                    GtkWidget      *widget)
{
  ctk_widget_queue_draw (widget);
}

static gboolean
drawing_area_draw (GtkWidget *widget,
                   cairo_t   *cr)
{
  GtkAllocation allocation;

  ctk_widget_get_allocation (widget, &allocation);

  if (swipe_x != 0 || swipe_y != 0)
    {
      cairo_save (cr);
      cairo_set_line_width (cr, 6);
      cairo_move_to (cr, allocation.width / 2,
                     allocation.height / 2);
      cairo_rel_line_to (cr, swipe_x, swipe_y);
      cairo_set_source_rgba (cr, 1, 0, 0, 0.5);
      cairo_stroke (cr);
      cairo_restore (cr);
    }

  if (ctk_gesture_is_recognized (rotate) || ctk_gesture_is_recognized (zoom))
    {
      cairo_pattern_t *pat;
      cairo_matrix_t matrix;
      gdouble angle, scale;
      gdouble x_center, y_center;

      ctk_gesture_get_bounding_box_center (CTK_GESTURE (zoom), &x_center, &y_center);

      cairo_get_matrix (cr, &matrix);
      cairo_matrix_translate (&matrix, x_center, y_center);

      cairo_save (cr);

      angle = ctk_gesture_rotate_get_angle_delta (CTK_GESTURE_ROTATE (rotate));
      cairo_matrix_rotate (&matrix, angle);

      scale = ctk_gesture_zoom_get_scale_delta (CTK_GESTURE_ZOOM (zoom));
      cairo_matrix_scale (&matrix, scale, scale);

      cairo_set_matrix (cr, &matrix);
      cairo_rectangle (cr, -100, -100, 200, 200);

      pat = cairo_pattern_create_linear (-100, 0, 200, 0);
      cairo_pattern_add_color_stop_rgb (pat, 0, 0, 0, 1);
      cairo_pattern_add_color_stop_rgb (pat, 1, 1, 0, 0);
      cairo_set_source (cr, pat);
      cairo_fill (cr);

      cairo_restore (cr);

      cairo_pattern_destroy (pat);
    }

  if (long_pressed)
    {
      cairo_save (cr);
      cairo_arc (cr, allocation.width / 2,
                 allocation.height / 2,
                 50, 0, 2 * G_PI);

      cairo_set_source_rgba (cr, 0, 1, 0, 0.5);
      cairo_stroke (cr);

      cairo_restore (cr);
    }

  return TRUE;
}

GtkWidget *
do_gestures (GtkWidget *do_widget)
{
  static GtkWidget *window = NULL;
  GtkWidget *drawing_area;
  GtkGesture *gesture;

  if (!window)
    {
      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_default_size (CTK_WINDOW (window), 400, 400);
      ctk_window_set_title (CTK_WINDOW (window), "Gestures");
      g_signal_connect (window, "destroy",
                        G_CALLBACK (ctk_widget_destroyed), &window);

      drawing_area = ctk_drawing_area_new ();
      ctk_container_add (CTK_CONTAINER (window), drawing_area);
      ctk_widget_add_events (drawing_area,
                             GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
                             GDK_POINTER_MOTION_MASK | GDK_TOUCH_MASK);

      g_signal_connect (drawing_area, "draw",
                        G_CALLBACK (drawing_area_draw), NULL);

      /* Swipe */
      gesture = ctk_gesture_swipe_new (drawing_area);
      g_signal_connect (gesture, "swipe",
                        G_CALLBACK (swipe_gesture_swept), drawing_area);
      ctk_event_controller_set_propagation_phase (CTK_EVENT_CONTROLLER (gesture),
                                                  CTK_PHASE_BUBBLE);
      g_object_weak_ref (G_OBJECT (drawing_area), (GWeakNotify) g_object_unref, gesture);

      /* 3fg swipe for touchpads */
      gesture = g_object_new (CTK_TYPE_GESTURE_SWIPE,
                              "widget", drawing_area,
                              "n-points", 3,
                              NULL);
      g_signal_connect (gesture, "begin",
                        G_CALLBACK (touchpad_swipe_gesture_begin), drawing_area);
      g_signal_connect (gesture, "swipe",
                        G_CALLBACK (swipe_gesture_swept), drawing_area);
      ctk_event_controller_set_propagation_phase (CTK_EVENT_CONTROLLER (gesture),
                                                  CTK_PHASE_BUBBLE);
      g_object_weak_ref (G_OBJECT (drawing_area), (GWeakNotify) g_object_unref, gesture);

      /* Long press */
      gesture = ctk_gesture_long_press_new (drawing_area);
      g_signal_connect (gesture, "pressed",
                        G_CALLBACK (long_press_gesture_pressed), drawing_area);
      g_signal_connect (gesture, "end",
                        G_CALLBACK (long_press_gesture_end), drawing_area);
      ctk_event_controller_set_propagation_phase (CTK_EVENT_CONTROLLER (gesture),
                                                  CTK_PHASE_BUBBLE);
      g_object_weak_ref (G_OBJECT (drawing_area), (GWeakNotify) g_object_unref, gesture);

      /* Rotate */
      rotate = gesture = ctk_gesture_rotate_new (drawing_area);
      g_signal_connect (gesture, "angle-changed",
                        G_CALLBACK (rotation_angle_changed), drawing_area);
      ctk_event_controller_set_propagation_phase (CTK_EVENT_CONTROLLER (gesture),
                                                  CTK_PHASE_BUBBLE);
      g_object_weak_ref (G_OBJECT (drawing_area), (GWeakNotify) g_object_unref, gesture);

      /* Zoom */
      zoom = gesture = ctk_gesture_zoom_new (drawing_area);
      g_signal_connect (gesture, "scale-changed",
                        G_CALLBACK (zoom_scale_changed), drawing_area);
      ctk_event_controller_set_propagation_phase (CTK_EVENT_CONTROLLER (gesture),
                                                  CTK_PHASE_BUBBLE);
      g_object_weak_ref (G_OBJECT (drawing_area), (GWeakNotify) g_object_unref, gesture);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);

  return window;
}
