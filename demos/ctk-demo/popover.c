/* Popovers
 *
 * A bubble-like window containing contextual information or options.
 * CtkPopovers can be attached to any widget, and will be displayed
 * within the same window, but on top of all its content.
 */

#include <ctk/ctk.h>

static void
toggle_changed_cb (CtkToggleButton *button,
                   CtkWidget       *popover)
{
  ctk_widget_set_visible (popover,
                          ctk_toggle_button_get_active (button));
}

static CtkWidget *
create_popover (CtkWidget       *parent,
                CtkWidget       *child,
                CtkPositionType  pos)
{
  CtkWidget *popover;

  popover = ctk_popover_new (parent);
  ctk_popover_set_position (CTK_POPOVER (popover), pos);
  ctk_container_add (CTK_CONTAINER (popover), child);
  ctk_container_set_border_width (CTK_CONTAINER (popover), 6);
  ctk_widget_show (child);

  return popover;
}

static CtkWidget *
create_complex_popover (CtkWidget       *parent,
                        CtkPositionType  pos)
{
  CtkWidget *popover, *window, *content;
  CtkBuilder *builder;

  builder = ctk_builder_new ();
  ctk_builder_add_from_resource (builder, "/popover/popover.ui", NULL);
  window = CTK_WIDGET (ctk_builder_get_object (builder, "window"));
  content = ctk_bin_get_child (CTK_BIN (window));
  g_object_ref (content);
  ctk_container_remove (CTK_CONTAINER (ctk_widget_get_parent (content)),
                        content);
  ctk_widget_destroy (window);
  g_object_unref (builder);

  popover = create_popover (parent, content, CTK_POS_BOTTOM);
  g_object_unref (content);

  return popover;
}

static void
entry_size_allocate_cb (CtkEntry      *entry,
                        CtkAllocation *allocation,
                        gpointer       user_data)
{
  CtkEntryIconPosition popover_pos;
  CtkPopover *popover = user_data;
  cairo_rectangle_int_t rect;

  if (ctk_widget_is_visible (CTK_WIDGET (popover)))
    {
      popover_pos =
        GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (entry),
                                             "popover-icon-pos"));
      ctk_entry_get_icon_area (entry, popover_pos, &rect);
      ctk_popover_set_pointing_to (CTK_POPOVER (popover), &rect);
    }
}

static void
entry_icon_press_cb (CtkEntry             *entry,
                     CtkEntryIconPosition  icon_pos,
                     CdkEvent             *event,
                     gpointer              user_data)
{
  CtkWidget *popover = user_data;
  cairo_rectangle_int_t rect;

  ctk_entry_get_icon_area (entry, icon_pos, &rect);
  ctk_popover_set_pointing_to (CTK_POPOVER (popover), &rect);
  ctk_widget_show (popover);

  g_object_set_data (G_OBJECT (entry), "popover-icon-pos",
                     GUINT_TO_POINTER (icon_pos));
}

static void
day_selected_cb (CtkCalendar *calendar,
                 gpointer     user_data)
{
  cairo_rectangle_int_t rect;
  CtkAllocation allocation;
  CtkWidget *popover;
  CdkEvent *event;

  event = ctk_get_current_event ();

  if (event->type != CDK_BUTTON_PRESS)
    return;

  cdk_window_coords_to_parent (event->button.window,
                               event->button.x, event->button.y,
                               &event->button.x, &event->button.y);
  ctk_widget_get_allocation (CTK_WIDGET (calendar), &allocation);
  rect.x = event->button.x - allocation.x;
  rect.y = event->button.y - allocation.y;
  rect.width = rect.height = 1;

  popover = create_popover (CTK_WIDGET (calendar),
                            ctk_entry_new (),
                            CTK_POS_BOTTOM);
  ctk_popover_set_pointing_to (CTK_POPOVER (popover), &rect);

  ctk_widget_show (popover);

  cdk_event_free (event);
}

CtkWidget *
do_popover (CtkWidget *do_widget)
{
  static CtkWidget *window = NULL;
  CtkWidget *popover, *box, *widget;

  if (!window)
    {
      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 24);
      ctk_container_set_border_width (CTK_CONTAINER (box), 24);
      ctk_container_add (CTK_CONTAINER (window), box);

      g_signal_connect (window, "destroy",
                        G_CALLBACK (ctk_widget_destroyed), &window);

      widget = ctk_toggle_button_new_with_label ("Button");
      popover = create_popover (widget,
                                ctk_label_new ("This popover does not grab input"),
                                CTK_POS_TOP);
      ctk_popover_set_modal (CTK_POPOVER (popover), FALSE);
      g_signal_connect (widget, "toggled",
                        G_CALLBACK (toggle_changed_cb), popover);
      ctk_container_add (CTK_CONTAINER (box), widget);

      widget = ctk_entry_new ();
      popover = create_complex_popover (widget, CTK_POS_TOP);
      ctk_entry_set_icon_from_icon_name (CTK_ENTRY (widget),
                                         CTK_ENTRY_ICON_PRIMARY, "edit-find");
      ctk_entry_set_icon_from_icon_name (CTK_ENTRY (widget),
                                         CTK_ENTRY_ICON_SECONDARY, "edit-clear");

      g_signal_connect (widget, "icon-press",
                        G_CALLBACK (entry_icon_press_cb), popover);
      g_signal_connect (widget, "size-allocate",
                        G_CALLBACK (entry_size_allocate_cb), popover);
      ctk_container_add (CTK_CONTAINER (box), widget);

      widget = ctk_calendar_new ();
      g_signal_connect (widget, "day-selected",
                        G_CALLBACK (day_selected_cb), NULL);
      ctk_container_add (CTK_CONTAINER (box), widget);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);

  return window;
}
