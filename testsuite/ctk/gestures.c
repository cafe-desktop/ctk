#include <ctk/ctk.h>

typedef struct {
  CtkWidget *widget;
  gint x;
  gint y;
  guint state;
  guint pressed : 1;
} PointState;

static PointState mouse_state;
static PointState touch_state[10]; /* touchpoint 0 gets pointer emulation,
                                    * use it first in tests for consistency.
                                    */

#define EVENT_SEQUENCE(point) (CdkEventSequence*) ((point) - touch_state + 1)

static void
point_press (PointState *point,
             CtkWidget  *widget,
             guint       button)
{
  CdkDisplay *display;
  CdkDevice *device;
  CdkSeat *seat;
  CdkEvent *ev;

  display = ctk_widget_get_display (widget);
  seat = cdk_display_get_default_seat (display);
  device = cdk_seat_get_pointer (seat);

  if (point == &mouse_state)
    {
      ev = cdk_event_new (CDK_BUTTON_PRESS);
      ev->any.window = g_object_ref (ctk_widget_get_window (widget));
      ev->button.time = CDK_CURRENT_TIME;
      ev->button.x = point->x;
      ev->button.y = point->y;
      ev->button.button = button;
      ev->button.state = point->state;

      point->state |= CDK_BUTTON1_MASK << (button - 1);
    }
  else
    {
      ev = cdk_event_new (CDK_TOUCH_BEGIN);
      ev->any.window = g_object_ref (ctk_widget_get_window (widget));
      ev->touch.time = CDK_CURRENT_TIME;
      ev->touch.x = point->x;
      ev->touch.y = point->y;
      ev->touch.sequence = EVENT_SEQUENCE (point);

      if (point == &touch_state[0])
        ev->touch.emulating_pointer = TRUE;
    }

  cdk_event_set_device (ev, device);

  ctk_main_do_event (ev);

  cdk_event_free (ev);

  point->widget = widget;
}

static void
point_update (PointState *point,
              CtkWidget  *widget,
              gdouble     x,
              gdouble     y)
{
  CdkDisplay *display;
  CdkDevice *device;
  CdkSeat *seat;
  CdkEvent *ev;

  display = ctk_widget_get_display (widget);
  seat = cdk_display_get_default_seat (display);
  device = cdk_seat_get_pointer (seat);

  point->x = x;
  point->y = y;

  if (point == &mouse_state)
    {
      ev = cdk_event_new (CDK_MOTION_NOTIFY);
      ev->any.window = g_object_ref (ctk_widget_get_window (widget));
      ev->button.time = CDK_CURRENT_TIME;
      ev->motion.x = x;
      ev->motion.y = y;
      ev->motion.state = point->state;
    }
  else
    {
      if (!point->widget || widget != point->widget)
        return;

      ev = cdk_event_new (CDK_TOUCH_UPDATE);
      ev->any.window = g_object_ref (ctk_widget_get_window (widget));
      ev->touch.time = CDK_CURRENT_TIME;
      ev->touch.x = x;
      ev->touch.y = y;
      ev->touch.sequence = EVENT_SEQUENCE (point);
      ev->touch.state = 0;

      if (point == &touch_state[0])
        ev->touch.emulating_pointer = TRUE;
    }

  cdk_event_set_device (ev, device);

  ctk_main_do_event (ev);

  cdk_event_free (ev);
}

static void
point_release (PointState *point,
               guint       button)
{
  CdkDisplay *display;
  CdkDevice *device;
  CdkSeat *seat;
  CdkEvent *ev;

  if (point->widget == NULL)
    return;

  display = ctk_widget_get_display (point->widget);
  seat = cdk_display_get_default_seat (display);
  device = cdk_seat_get_pointer (seat);

  if (!point->widget)
    return;

  if (point == &mouse_state)
    {
      if ((point->state & (CDK_BUTTON1_MASK << (button - 1))) == 0)
        return;

      ev = cdk_event_new (CDK_BUTTON_RELEASE);
      ev->any.window = g_object_ref (ctk_widget_get_window (point->widget));
      ev->button.time = CDK_CURRENT_TIME;
      ev->button.x = point->x;
      ev->button.y = point->y;
      ev->button.state = point->state;

      point->state &= ~(CDK_BUTTON1_MASK << (button - 1));
    }
  else
    {
      ev = cdk_event_new (CDK_TOUCH_END);
      ev->any.window = g_object_ref (ctk_widget_get_window (point->widget));
      ev->touch.time = CDK_CURRENT_TIME;
      ev->touch.x = point->x;
      ev->touch.y = point->y;
      ev->touch.sequence = EVENT_SEQUENCE (point);
      ev->touch.state = point->state;

      if (point == &touch_state[0])
        ev->touch.emulating_pointer = TRUE;
    }

  cdk_event_set_device (ev, device);

  ctk_main_do_event (ev);

  cdk_event_free (ev);
}

static const gchar *
phase_nick (CtkPropagationPhase phase)
{
 GTypeClass *class;
 GEnumValue *value;

 class = g_type_class_ref (CTK_TYPE_PROPAGATION_PHASE);
 value = g_enum_get_value ((GEnumClass*)class, phase);
 g_type_class_unref (class);  

 return value->value_nick;
}

static const gchar *
state_nick (CtkEventSequenceState state)
{
 GTypeClass *class;
 GEnumValue *value;

 class = g_type_class_ref (CTK_TYPE_EVENT_SEQUENCE_STATE);
 value = g_enum_get_value ((GEnumClass*)class, state);
 g_type_class_unref (class);  

 return value->value_nick;
}

typedef struct {
  GString *str;
  gboolean exit;
} LegacyData;

static gboolean
legacy_cb (CtkWidget      *w,
	   CdkEventButton *button G_GNUC_UNUSED,
	   gpointer        data)
{
  LegacyData *ld = data;

  if (ld->str->len > 0)
    g_string_append (ld->str, ", ");
  g_string_append_printf (ld->str, "legacy %s", ctk_widget_get_name (w));

  return ld->exit;
}

typedef struct {
  GString *str;
  CtkEventSequenceState state;
} GestureData;

static void
press_cb (CtkGesture *g,
	  gint        n_press G_GNUC_UNUSED,
	  gdouble     x G_GNUC_UNUSED,
	  gdouble     y G_GNUC_UNUSED,
	  gpointer    data)
{
  CtkEventController *c = CTK_EVENT_CONTROLLER (g);
  CdkEventSequence *sequence;
  CtkPropagationPhase phase;
  GestureData *gd = data;
  const gchar *name;
  
  name = g_object_get_data (G_OBJECT (g), "name");
  phase = ctk_event_controller_get_propagation_phase (c);

  if (gd->str->len > 0)
    g_string_append (gd->str, ", ");
  g_string_append_printf (gd->str, "%s %s", phase_nick (phase), name);

  sequence = ctk_gesture_get_last_updated_sequence (g);

  if (sequence)
    g_string_append_printf (gd->str, " (%x)", GPOINTER_TO_UINT (sequence));

  if (gd->state != CTK_EVENT_SEQUENCE_NONE)
    ctk_gesture_set_state (g, gd->state);
}

static void
cancel_cb (CtkGesture       *g,
	   CdkEventSequence *sequence G_GNUC_UNUSED,
	   gpointer          data)
{
  GestureData *gd = data;
  const gchar *name;
  
  name = g_object_get_data (G_OBJECT (g), "name");
  
  if (gd->str->len > 0)
    g_string_append (gd->str, ", ");
  g_string_append_printf (gd->str, "%s cancelled", name);
}

static void
begin_cb (CtkGesture       *g,
	  CdkEventSequence *sequence G_GNUC_UNUSED,
	  gpointer          data)
{
  GestureData *gd = data;
  const gchar *name;

  name = g_object_get_data (G_OBJECT (g), "name");

  if (gd->str->len > 0)
    g_string_append (gd->str, ", ");
  g_string_append_printf (gd->str, "%s began", name);

  if (gd->state != CTK_EVENT_SEQUENCE_NONE)
    ctk_gesture_set_state (g, gd->state);
}

static void
end_cb (CtkGesture       *g,
	CdkEventSequence *sequence G_GNUC_UNUSED,
	gpointer          data)
{
  GestureData *gd = data;
  const gchar *name;

  name = g_object_get_data (G_OBJECT (g), "name");

  if (gd->str->len > 0)
    g_string_append (gd->str, ", ");
  g_string_append_printf (gd->str, "%s ended", name);
}

static void
update_cb (CtkGesture       *g,
	   CdkEventSequence *sequence G_GNUC_UNUSED,
	   gpointer          data)
{
  GestureData *gd = data;
  const gchar *name;
  
  name = g_object_get_data (G_OBJECT (g), "name");
  
  if (gd->str->len > 0)
    g_string_append (gd->str, ", ");
  g_string_append_printf (gd->str, "%s updated", name);
}

static void
state_changed_cb (CtkGesture *g, CdkEventSequence *sequence, CtkEventSequenceState state, gpointer data)
{
  GestureData *gd = data;
  const gchar *name;
  
  name = g_object_get_data (G_OBJECT (g), "name");
  
  if (gd->str->len > 0)
    g_string_append (gd->str, ", ");
  g_string_append_printf (gd->str, "%s state %s", name, state_nick (state));

  if (sequence != NULL)
    g_string_append_printf (gd->str, " (%x)", GPOINTER_TO_UINT (sequence));
}


static CtkGesture *
add_gesture (CtkWidget *w, const gchar *name, CtkPropagationPhase phase, GString *str, CtkEventSequenceState state)
{
  CtkGesture *g;
  GestureData *data;

  data = g_new (GestureData, 1);
  data->str = str;
  data->state = state;

  g = ctk_gesture_multi_press_new (w);
  ctk_gesture_single_set_touch_only (CTK_GESTURE_SINGLE (g), FALSE);
  ctk_gesture_single_set_button (CTK_GESTURE_SINGLE (g), 1);
  ctk_event_controller_set_propagation_phase (CTK_EVENT_CONTROLLER (g), phase);

  g_object_set_data (G_OBJECT (g), "name", (gpointer)name);

  g_signal_connect (g, "pressed", G_CALLBACK (press_cb), data);
  g_signal_connect (g, "cancel", G_CALLBACK (cancel_cb), data);
  g_signal_connect (g, "update", G_CALLBACK (update_cb), data);
  g_signal_connect (g, "sequence-state-changed", G_CALLBACK (state_changed_cb), data);

  return g;
}

static CtkGesture *
add_mt_gesture (CtkWidget *w, const gchar *name, CtkPropagationPhase phase, GString *str, CtkEventSequenceState state)
{
  CtkGesture *g;
  GestureData *data;

  data = g_new (GestureData, 1);
  data->str = str;
  data->state = state;

  g = ctk_gesture_rotate_new (w);
  ctk_event_controller_set_propagation_phase (CTK_EVENT_CONTROLLER (g), phase);

  g_object_set_data (G_OBJECT (g), "name", (gpointer)name);

  g_signal_connect (g, "begin", G_CALLBACK (begin_cb), data);
  g_signal_connect (g, "update", G_CALLBACK (update_cb), data);
  g_signal_connect (g, "end", G_CALLBACK (end_cb), data);
  g_signal_connect (g, "sequence-state-changed", G_CALLBACK (state_changed_cb), data);

  return g;
}

static void
add_legacy (CtkWidget *w, GString *str, gboolean exit)
{
  LegacyData *data;

  data = g_new (LegacyData, 1);
  data->str = str;
  data->exit = exit;
  g_signal_connect (w, "button-press-event", G_CALLBACK (legacy_cb), data);
}

static void
test_phases (void)
{
  CtkWidget *A, *B, *C;
  GString *str;

  A = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_widget_set_name (A, "A");
  B = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
  ctk_widget_set_name (B, "B");
  C = ctk_event_box_new ();
  ctk_widget_set_hexpand (C, TRUE);
  ctk_widget_set_vexpand (C, TRUE);
  ctk_widget_set_name (C, "C");

  ctk_container_add (CTK_CONTAINER (A), B);
  ctk_container_add (CTK_CONTAINER (B), C);

  ctk_widget_show_all (A);

  str = g_string_new ("");

  add_gesture (A, "a1", CTK_PHASE_CAPTURE, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (B, "b1", CTK_PHASE_CAPTURE, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (C, "c1", CTK_PHASE_CAPTURE, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (A, "a2", CTK_PHASE_TARGET, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (B, "b2", CTK_PHASE_TARGET, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (C, "c2", CTK_PHASE_TARGET, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (A, "a3", CTK_PHASE_BUBBLE, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (B, "b3", CTK_PHASE_BUBBLE, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (C, "c3", CTK_PHASE_BUBBLE, str, CTK_EVENT_SEQUENCE_NONE);

  point_update (&mouse_state, C, 10, 10);
  point_press (&mouse_state, C, 1);

  g_assert_cmpstr (str->str, ==,
                   "capture a1, "
                   "capture b1, "
                   "capture c1, "
                   "target c2, "
                   "bubble c3, "
                   "bubble b3, "
                   "bubble a3");

  g_string_free (str, TRUE);

  ctk_widget_destroy (A);
}

static void
test_mixed (void)
{
  CtkWidget *A, *B, *C;
  GString *str;

  A = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_widget_set_name (A, "A");
  B = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
  ctk_widget_set_name (B, "B");
  C = ctk_event_box_new ();
  ctk_widget_set_hexpand (C, TRUE);
  ctk_widget_set_vexpand (C, TRUE);
  ctk_widget_set_name (C, "C");

  ctk_container_add (CTK_CONTAINER (A), B);
  ctk_container_add (CTK_CONTAINER (B), C);

  ctk_widget_show_all (A);

  str = g_string_new ("");

  add_gesture (A, "a1", CTK_PHASE_CAPTURE, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (B, "b1", CTK_PHASE_CAPTURE, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (C, "c1", CTK_PHASE_CAPTURE, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (A, "a2", CTK_PHASE_TARGET, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (B, "b2", CTK_PHASE_TARGET, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (C, "c2", CTK_PHASE_TARGET, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (A, "a3", CTK_PHASE_BUBBLE, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (B, "b3", CTK_PHASE_BUBBLE, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (C, "c3", CTK_PHASE_BUBBLE, str, CTK_EVENT_SEQUENCE_NONE);

  add_legacy (A, str, CDK_EVENT_PROPAGATE);
  add_legacy (B, str, CDK_EVENT_PROPAGATE);
  add_legacy (C, str, CDK_EVENT_PROPAGATE);

  point_update (&mouse_state, C, 10, 10);
  point_press (&mouse_state, C, 1);

  g_assert_cmpstr (str->str, ==,
                   "capture a1, "
                   "capture b1, "
                   "capture c1, "
                   "target c2, "
                   "legacy C, "
                   "bubble c3, "
                   "legacy B, "
                   "bubble b3, "
                   "legacy A, "
                   "bubble a3");

  g_string_free (str, TRUE);

  ctk_widget_destroy (A);
}

static void
test_early_exit (void)
{
  CtkWidget *A, *B, *C;
  GString *str;

  A = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_widget_set_name (A, "A");
  B = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
  ctk_widget_set_name (B, "B");
  C = ctk_event_box_new ();
  ctk_widget_set_hexpand (C, TRUE);
  ctk_widget_set_vexpand (C, TRUE);
  ctk_widget_set_name (C, "C");

  ctk_container_add (CTK_CONTAINER (A), B);
  ctk_container_add (CTK_CONTAINER (B), C);

  ctk_widget_show_all (A);

  str = g_string_new ("");

  add_gesture (A, "a1", CTK_PHASE_CAPTURE, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (B, "b1", CTK_PHASE_CAPTURE, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (C, "c1", CTK_PHASE_CAPTURE, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (C, "c2", CTK_PHASE_TARGET, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (A, "a3", CTK_PHASE_BUBBLE, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (B, "b3", CTK_PHASE_BUBBLE, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (C, "c3", CTK_PHASE_BUBBLE, str, CTK_EVENT_SEQUENCE_NONE);

  add_legacy (A, str, CDK_EVENT_PROPAGATE);
  add_legacy (B, str, CDK_EVENT_STOP);
  add_legacy (C, str, CDK_EVENT_PROPAGATE);

  point_update (&mouse_state, C, 10, 10);
  point_press (&mouse_state, C, 1);

  g_assert_cmpstr (str->str, ==,
                   "capture a1, "
                   "capture b1, "
                   "capture c1, "
                   "target c2, "
                   "legacy C, "
                   "bubble c3, "
                   "legacy B");

  g_string_free (str, TRUE);

  ctk_widget_destroy (A);
}

static void
test_claim_capture (void)
{
  CtkWidget *A, *B, *C;
  GString *str;

  A = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_widget_set_name (A, "A");
  B = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
  ctk_widget_set_name (B, "B");
  C = ctk_event_box_new ();
  ctk_widget_set_hexpand (C, TRUE);
  ctk_widget_set_vexpand (C, TRUE);
  ctk_widget_set_name (C, "C");

  ctk_container_add (CTK_CONTAINER (A), B);
  ctk_container_add (CTK_CONTAINER (B), C);

  ctk_widget_show_all (A);

  str = g_string_new ("");

  add_gesture (A, "a1", CTK_PHASE_CAPTURE, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (B, "b1", CTK_PHASE_CAPTURE, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (C, "c1", CTK_PHASE_CAPTURE, str, CTK_EVENT_SEQUENCE_CLAIMED);
  add_gesture (C, "c2", CTK_PHASE_TARGET, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (A, "a3", CTK_PHASE_BUBBLE, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (B, "b3", CTK_PHASE_BUBBLE, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (C, "c3", CTK_PHASE_BUBBLE, str, CTK_EVENT_SEQUENCE_NONE);

  point_update (&mouse_state, C, 10, 10);
  point_press (&mouse_state, C, 1);

  g_assert_cmpstr (str->str, ==,
                   "capture a1, "
                   "capture b1, "
                   "capture c1, "
                   "c1 state claimed");

  g_string_free (str, TRUE);

  ctk_widget_destroy (A);
}

static void
test_claim_target (void)
{
  CtkWidget *A, *B, *C;
  GString *str;

  A = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_widget_set_name (A, "A");
  B = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
  ctk_widget_set_name (B, "B");
  C = ctk_event_box_new ();
  ctk_widget_set_hexpand (C, TRUE);
  ctk_widget_set_vexpand (C, TRUE);
  ctk_widget_set_name (C, "C");

  ctk_container_add (CTK_CONTAINER (A), B);
  ctk_container_add (CTK_CONTAINER (B), C);

  ctk_widget_show_all (A);

  str = g_string_new ("");

  add_gesture (A, "a1", CTK_PHASE_CAPTURE, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (B, "b1", CTK_PHASE_CAPTURE, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (C, "c1", CTK_PHASE_CAPTURE, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (C, "c2", CTK_PHASE_TARGET, str, CTK_EVENT_SEQUENCE_CLAIMED);
  add_gesture (A, "a3", CTK_PHASE_BUBBLE, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (B, "b3", CTK_PHASE_BUBBLE, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (C, "c3", CTK_PHASE_BUBBLE, str, CTK_EVENT_SEQUENCE_NONE);

  point_update (&mouse_state, C, 10, 10);
  point_press (&mouse_state, C, 1);

  g_assert_cmpstr (str->str, ==,
                   "capture a1, "
                   "capture b1, "
                   "capture c1, "
                   "target c2, "
                   "c2 state claimed");

  g_string_free (str, TRUE);

  ctk_widget_destroy (A);
}

static void
test_claim_bubble (void)
{
  CtkWidget *A, *B, *C;
  GString *str;

  A = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_widget_set_name (A, "A");
  B = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
  ctk_widget_set_name (B, "B");
  C = ctk_event_box_new ();
  ctk_widget_set_hexpand (C, TRUE);
  ctk_widget_set_vexpand (C, TRUE);
  ctk_widget_set_name (C, "C");

  ctk_container_add (CTK_CONTAINER (A), B);
  ctk_container_add (CTK_CONTAINER (B), C);

  ctk_widget_show_all (A);

  str = g_string_new ("");

  add_gesture (A, "a1", CTK_PHASE_CAPTURE, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (B, "b1", CTK_PHASE_CAPTURE, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (C, "c1", CTK_PHASE_CAPTURE, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (C, "c2", CTK_PHASE_TARGET, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (A, "a3", CTK_PHASE_BUBBLE, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (B, "b3", CTK_PHASE_BUBBLE, str, CTK_EVENT_SEQUENCE_CLAIMED);
  add_gesture (C, "c3", CTK_PHASE_BUBBLE, str, CTK_EVENT_SEQUENCE_NONE);

  point_update (&mouse_state, C, 10, 10);
  point_press (&mouse_state, C, 1);

  g_assert_cmpstr (str->str, ==,
                   "capture a1, "
                   "capture b1, "
                   "capture c1, "
                   "target c2, "
                   "bubble c3, "
                   "bubble b3, "
                   "c3 cancelled, "
                   "c2 cancelled, "
                   "c1 cancelled, "
                   "b3 state claimed"
                   );

  g_string_free (str, TRUE);

  ctk_widget_destroy (A);
}

static void
test_early_claim_capture (void)
{
  CtkWidget *A, *B, *C;
  CtkGesture *g;
  GString *str;

  A = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_widget_set_name (A, "A");
  B = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
  ctk_widget_set_name (B, "B");
  C = ctk_event_box_new ();
  ctk_widget_set_hexpand (C, TRUE);
  ctk_widget_set_vexpand (C, TRUE);
  ctk_widget_set_name (C, "C");

  ctk_container_add (CTK_CONTAINER (A), B);
  ctk_container_add (CTK_CONTAINER (B), C);

  ctk_widget_show_all (A);

  str = g_string_new ("");

  add_gesture (A, "a1", CTK_PHASE_CAPTURE, str, CTK_EVENT_SEQUENCE_NONE);
  g = add_gesture (B, "b1", CTK_PHASE_CAPTURE, str, CTK_EVENT_SEQUENCE_CLAIMED);
  add_gesture (C, "c1", CTK_PHASE_CAPTURE, str, CTK_EVENT_SEQUENCE_CLAIMED);
  add_gesture (C, "c2", CTK_PHASE_TARGET, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (A, "a3", CTK_PHASE_BUBBLE, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (B, "b3", CTK_PHASE_BUBBLE, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (C, "c3", CTK_PHASE_BUBBLE, str, CTK_EVENT_SEQUENCE_NONE);

  point_update (&mouse_state, C, 10, 10);
  point_press (&mouse_state, C, 1);

  g_assert_cmpstr (str->str, ==,
                   "capture a1, "
                   "capture b1, "
                   "b1 state claimed");

  /* Reset the string */
  g_string_erase (str, 0, str->len);

  ctk_gesture_set_state (g, CTK_EVENT_SEQUENCE_DENIED);

  g_assert_cmpstr (str->str, ==,
                   "capture c1, "
                   "c1 state claimed, "
                   "b1 state denied");

  point_release (&mouse_state, 1);

  g_string_free (str, TRUE);
  ctk_widget_destroy (A);
}

static void
test_late_claim_capture (void)
{
  CtkWidget *A, *B, *C;
  CtkGesture *g;
  GString *str;

  A = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_widget_set_name (A, "A");
  B = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
  ctk_widget_set_name (B, "B");
  C = ctk_event_box_new ();
  ctk_widget_set_hexpand (C, TRUE);
  ctk_widget_set_vexpand (C, TRUE);
  ctk_widget_set_name (C, "C");

  ctk_container_add (CTK_CONTAINER (A), B);
  ctk_container_add (CTK_CONTAINER (B), C);

  ctk_widget_show_all (A);

  str = g_string_new ("");

  add_gesture (A, "a1", CTK_PHASE_CAPTURE, str, CTK_EVENT_SEQUENCE_NONE);
  g = add_gesture (B, "b1", CTK_PHASE_CAPTURE, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (C, "c1", CTK_PHASE_CAPTURE, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (C, "c2", CTK_PHASE_TARGET, str, CTK_EVENT_SEQUENCE_CLAIMED);
  add_gesture (A, "a3", CTK_PHASE_BUBBLE, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (B, "b3", CTK_PHASE_BUBBLE, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (C, "c3", CTK_PHASE_BUBBLE, str, CTK_EVENT_SEQUENCE_NONE);

  point_update (&mouse_state, C, 10, 10);
  point_press (&mouse_state, C, 1);

  g_assert_cmpstr (str->str, ==,
                   "capture a1, "
                   "capture b1, "
                   "capture c1, "
                   "target c2, "
                   "c2 state claimed");

  /* Reset the string */
  g_string_erase (str, 0, str->len);

  ctk_gesture_set_state (g, CTK_EVENT_SEQUENCE_CLAIMED);

  g_assert_cmpstr (str->str, ==,
                   "c2 cancelled, "
                   "c1 cancelled, "
                   "b1 state claimed");

  point_release (&mouse_state, 1);

  g_string_free (str, TRUE);
  ctk_widget_destroy (A);
}

static void
test_group (void)
{
  CtkWidget *A, *B, *C;
  GString *str;
  CtkGesture *g1, *g2;

  A = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_widget_set_name (A, "A");
  B = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
  ctk_widget_set_name (B, "B");
  C = ctk_event_box_new ();
  ctk_widget_set_hexpand (C, TRUE);
  ctk_widget_set_vexpand (C, TRUE);
  ctk_widget_set_name (C, "C");

  ctk_container_add (CTK_CONTAINER (A), B);
  ctk_container_add (CTK_CONTAINER (B), C);

  ctk_widget_show_all (A);

  str = g_string_new ("");

  add_gesture (A, "a1", CTK_PHASE_CAPTURE, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (B, "b1", CTK_PHASE_CAPTURE, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (C, "c1", CTK_PHASE_CAPTURE, str, CTK_EVENT_SEQUENCE_NONE);
  g1 = add_gesture (C, "c2", CTK_PHASE_TARGET, str, CTK_EVENT_SEQUENCE_NONE);
  g2 = add_gesture (C, "c3", CTK_PHASE_TARGET, str, CTK_EVENT_SEQUENCE_CLAIMED);
  ctk_gesture_group (g1, g2);
  add_gesture (A, "a3", CTK_PHASE_BUBBLE, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (B, "b3", CTK_PHASE_BUBBLE, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (C, "c4", CTK_PHASE_BUBBLE, str, CTK_EVENT_SEQUENCE_NONE);

  point_update (&mouse_state, C, 10, 10);
  point_press (&mouse_state, C, 1);

  g_assert_cmpstr (str->str, ==,
                   "capture a1, "
                   "capture b1, "
                   "capture c1, "
                   "target c3, "
                   "c3 state claimed, "
                   "c2 state claimed, "
                   "target c2");

  g_string_free (str, TRUE);

  ctk_widget_destroy (A);
}

static void
test_gestures_outside_grab (void)
{
  CtkWidget *A, *B, *C, *D;
  GString *str;

  A = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_widget_set_name (A, "A");
  B = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
  ctk_widget_set_name (B, "B");
  C = ctk_event_box_new ();
  ctk_widget_set_hexpand (C, TRUE);
  ctk_widget_set_vexpand (C, TRUE);
  ctk_widget_set_name (C, "C");

  ctk_container_add (CTK_CONTAINER (A), B);
  ctk_container_add (CTK_CONTAINER (B), C);

  ctk_widget_show_all (A);

  D = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_widget_show (D);

  str = g_string_new ("");

  add_gesture (A, "a1", CTK_PHASE_CAPTURE, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (B, "b1", CTK_PHASE_CAPTURE, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (C, "c1", CTK_PHASE_CAPTURE, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (C, "c2", CTK_PHASE_TARGET, str, CTK_EVENT_SEQUENCE_CLAIMED);
  add_gesture (B, "b2", CTK_PHASE_BUBBLE, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (A, "a2", CTK_PHASE_BUBBLE, str, CTK_EVENT_SEQUENCE_NONE);

  point_update (&mouse_state, C, 10, 10);
  point_press (&mouse_state, C, 1);

  g_assert_cmpstr (str->str, ==,
                   "capture a1, "
                   "capture b1, "
                   "capture c1, "
                   "target c2, "
                   "c2 state claimed");

  /* Set a grab on another window */
  g_string_erase (str, 0, str->len);
  ctk_grab_add (D);

  g_assert_cmpstr (str->str, ==,
                   "c1 cancelled, "
                   "c2 cancelled, "
                   "b1 cancelled, "
                   "a1 cancelled");

  g_string_free (str, TRUE);

  ctk_widget_destroy (A);
  ctk_widget_destroy (D);
}

static void
test_gestures_inside_grab (void)
{
  CtkWidget *A, *B, *C;
  GString *str;

  A = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_widget_set_name (A, "A");
  B = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
  ctk_widget_set_name (B, "B");
  C = ctk_event_box_new ();
  ctk_widget_set_hexpand (C, TRUE);
  ctk_widget_set_vexpand (C, TRUE);
  ctk_widget_set_name (C, "C");

  ctk_container_add (CTK_CONTAINER (A), B);
  ctk_container_add (CTK_CONTAINER (B), C);

  ctk_widget_show_all (A);

  str = g_string_new ("");

  add_gesture (A, "a1", CTK_PHASE_CAPTURE, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (B, "b1", CTK_PHASE_CAPTURE, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (C, "c1", CTK_PHASE_CAPTURE, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (C, "c2", CTK_PHASE_TARGET, str, CTK_EVENT_SEQUENCE_CLAIMED);
  add_gesture (B, "b2", CTK_PHASE_BUBBLE, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (A, "a2", CTK_PHASE_BUBBLE, str, CTK_EVENT_SEQUENCE_NONE);

  point_update (&mouse_state, C, 10, 10);
  point_press (&mouse_state, C, 1);

  g_assert_cmpstr (str->str, ==,
                   "capture a1, "
                   "capture b1, "
                   "capture c1, "
                   "target c2, "
                   "c2 state claimed");

  /* Set a grab on B */
  g_string_erase (str, 0, str->len);
  ctk_grab_add (B);
  g_assert_cmpstr (str->str, ==,
                   "a1 cancelled");

  /* Update with the grab under effect */
  g_string_erase (str, 0, str->len);
  point_update (&mouse_state, C, 20, 20);
  g_assert_cmpstr (str->str, ==,
                   "b1 updated, "
                   "c1 updated, "
                   "c2 updated");

  g_string_free (str, TRUE);

  ctk_widget_destroy (A);
}

static void
test_multitouch_on_single (void)
{
  CtkWidget *A, *B, *C;
  GString *str;

  A = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_widget_set_name (A, "A");
  B = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
  ctk_widget_set_name (B, "B");
  C = ctk_event_box_new ();
  ctk_widget_set_hexpand (C, TRUE);
  ctk_widget_set_vexpand (C, TRUE);
  ctk_widget_set_name (C, "C");

  ctk_container_add (CTK_CONTAINER (A), B);
  ctk_container_add (CTK_CONTAINER (B), C);

  ctk_widget_show_all (A);

  str = g_string_new ("");

  add_gesture (A, "a1", CTK_PHASE_CAPTURE, str, CTK_EVENT_SEQUENCE_NONE);
  add_gesture (B, "b1", CTK_PHASE_CAPTURE, str, CTK_EVENT_SEQUENCE_CLAIMED);

  /* First touch down */
  point_update (&touch_state[0], C, 10, 10);
  point_press (&touch_state[0], C, 1);

  g_assert_cmpstr (str->str, ==,
                   "capture a1 (1), "
                   "capture b1 (1), "
                   "b1 state claimed (1)");

  /* Second touch down */
  g_string_erase (str, 0, str->len);
  point_update (&touch_state[1], C, 20, 20);
  point_press (&touch_state[1], C, 1);

  g_assert_cmpstr (str->str, ==,
                   "a1 state denied (2), "
                   "b1 state denied (2)");

  g_string_free (str, TRUE);

  ctk_widget_destroy (A);
}

static void
test_multitouch_activation (void)
{
  CtkWidget *A, *B, *C;
  GString *str;

  A = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_widget_set_name (A, "A");
  B = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
  ctk_widget_set_name (B, "B");
  C = ctk_event_box_new ();
  ctk_widget_set_hexpand (C, TRUE);
  ctk_widget_set_vexpand (C, TRUE);
  ctk_widget_set_name (C, "C");

  ctk_container_add (CTK_CONTAINER (A), B);
  ctk_container_add (CTK_CONTAINER (B), C);

  ctk_widget_show_all (A);

  str = g_string_new ("");

  add_mt_gesture (C, "c1", CTK_PHASE_BUBBLE, str, CTK_EVENT_SEQUENCE_CLAIMED);

  /* First touch down */
  point_update (&touch_state[0], C, 10, 10);
  point_press (&touch_state[0], C, 1);

  g_assert_cmpstr (str->str, ==, "");

  /* Second touch down */
  point_update (&touch_state[1], C, 20, 20);
  point_press (&touch_state[1], C, 1);

  g_assert_cmpstr (str->str, ==,
                   "c1 began, "
                   "c1 state claimed (2), "
                   "c1 state claimed");

  /* First touch up */
  g_string_erase (str, 0, str->len);
  point_release (&touch_state[0], 1);

  g_assert_cmpstr (str->str, ==,
                   "c1 ended");

  /* A third touch down triggering again action */
  g_string_erase (str, 0, str->len);
  point_update (&touch_state[2], C, 20, 20);
  point_press (&touch_state[2], C, 1);

  g_assert_cmpstr (str->str, ==,
                   "c1 began, "
                   "c1 state claimed (3)");

  /* One touch up, gesture is finished again */
  g_string_erase (str, 0, str->len);
  point_release (&touch_state[2], 1);

  g_assert_cmpstr (str->str, ==,
                   "c1 ended");

  /* Another touch up, gesture remains inactive */
  g_string_erase (str, 0, str->len);
  point_release (&touch_state[1], 1);

  g_assert_cmpstr (str->str, ==, "");

  g_string_free (str, TRUE);

  ctk_widget_destroy (A);
}

static void
test_multitouch_interaction (void)
{
  CtkWidget *A, *B, *C;
  CtkGesture *g;
  GString *str;

  A = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_widget_set_name (A, "A");
  B = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
  ctk_widget_set_name (B, "B");
  C = ctk_event_box_new ();
  ctk_widget_set_hexpand (C, TRUE);
  ctk_widget_set_vexpand (C, TRUE);
  ctk_widget_set_name (C, "C");

  ctk_container_add (CTK_CONTAINER (A), B);
  ctk_container_add (CTK_CONTAINER (B), C);

  ctk_widget_show_all (A);

  str = g_string_new ("");

  g = add_gesture (A, "a1", CTK_PHASE_CAPTURE, str, CTK_EVENT_SEQUENCE_CLAIMED);
  add_mt_gesture (C, "c1", CTK_PHASE_BUBBLE, str, CTK_EVENT_SEQUENCE_CLAIMED);

  /* First touch down, a1 claims the sequence */
  point_update (&touch_state[0], C, 10, 10);
  point_press (&touch_state[0], C, 1);

  g_assert_cmpstr (str->str, ==,
                   "capture a1 (1), "
                   "a1 state claimed (1)");

  /* Second touch down, a1 denies and c1 takes over */
  g_string_erase (str, 0, str->len);
  point_update (&touch_state[1], C, 20, 20);
  point_press (&touch_state[1], C, 1);

  /* Denying sequences in touch-excess situation is a responsibility of the caller */
  ctk_gesture_set_state (g, CTK_EVENT_SEQUENCE_DENIED);

  g_assert_cmpstr (str->str, ==,
                   "a1 state denied (2), "
                   "c1 began, "
                   "c1 state claimed, "
                   "c1 state claimed (2), "
                   "a1 state denied (1)");

  /* Move first point, only c1 should update */
  g_string_erase (str, 0, str->len);
  point_update (&touch_state[0], C, 30, 30);

  g_assert_cmpstr (str->str, ==,
                   "c1 updated");

  /* First touch up */
  g_string_erase (str, 0, str->len);
  point_release (&touch_state[0], 1);

  g_assert_cmpstr (str->str, ==,
                   "c1 ended");

  /* A third touch down triggering again action on c1 */
  g_string_erase (str, 0, str->len);
  point_update (&touch_state[2], C, 20, 20);
  point_press (&touch_state[2], C, 1);

  g_assert_cmpstr (str->str, ==,
                   "a1 state denied (3), "
                   "c1 began, "
                   "c1 state claimed (3)");

  /* One touch up, gesture is finished again */
  g_string_erase (str, 0, str->len);
  point_release (&touch_state[2], 1);

  g_assert_cmpstr (str->str, ==,
                   "c1 ended");

  /* Another touch up, gesture remains inactive */
  g_string_erase (str, 0, str->len);
  point_release (&touch_state[1], 1);

  g_assert_cmpstr (str->str, ==, "");

  g_string_free (str, TRUE);

  ctk_widget_destroy (A);
}

int
main (int argc, char *argv[])
{
  ctk_test_init (&argc, &argv);

  g_test_add_func ("/gestures/propagation/phases", test_phases);
  g_test_add_func ("/gestures/propagation/mixed", test_mixed);
  g_test_add_func ("/gestures/propagation/early-exit", test_early_exit);
  g_test_add_func ("/gestures/claim/capture", test_claim_capture);
  g_test_add_func ("/gestures/claim/target", test_claim_target);
  g_test_add_func ("/gestures/claim/bubble", test_claim_bubble);
  g_test_add_func ("/gestures/claim/early-capture", test_early_claim_capture);
  g_test_add_func ("/gestures/claim/late-capture", test_late_claim_capture);
  g_test_add_func ("/gestures/group", test_group);
  g_test_add_func ("/gestures/grabs/gestures-outside-grab", test_gestures_outside_grab);
  g_test_add_func ("/gestures/grabs/gestures-inside-grab", test_gestures_inside_grab);
  g_test_add_func ("/gestures/multitouch/gesture-single", test_multitouch_on_single);
  g_test_add_func ("/gestures/multitouch/multitouch-activation", test_multitouch_activation);
  g_test_add_func ("/gestures/multitouch/interaction", test_multitouch_interaction);

  return g_test_run ();
}
