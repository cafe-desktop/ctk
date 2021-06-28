/* Text View/Hypertext
 *
 * Usually, tags modify the appearance of text in the view, e.g. making it
 * bold or colored or underlined. But tags are not restricted to appearance.
 * They can also affect the behavior of mouse and key presses, as this demo
 * shows.
 */

#include <ctk/ctk.h>
#include <cdk/cdkkeysyms.h>

/* Inserts a piece of text into the buffer, giving it the usual
 * appearance of a hyperlink in a web browser: blue and underlined.
 * Additionally, attaches some data on the tag, to make it recognizable
 * as a link.
 */
static void
insert_link (CtkTextBuffer *buffer,
             CtkTextIter   *iter,
             gchar         *text,
             gint           page)
{
  CtkTextTag *tag;

  tag = ctk_text_buffer_create_tag (buffer, NULL,
                                    "foreground", "blue",
                                    "underline", PANGO_UNDERLINE_SINGLE,
                                    NULL);
  g_object_set_data (G_OBJECT (tag), "page", GINT_TO_POINTER (page));
  ctk_text_buffer_insert_with_tags (buffer, iter, text, -1, tag, NULL);
}

/* Fills the buffer with text and interspersed links. In any real
 * hypertext app, this method would parse a file to identify the links.
 */
static void
show_page (CtkTextBuffer *buffer,
           gint           page)
{
  CtkTextIter iter;

  ctk_text_buffer_set_text (buffer, "", 0);
  ctk_text_buffer_get_iter_at_offset (buffer, &iter, 0);
  if (page == 1)
    {
      ctk_text_buffer_insert (buffer, &iter, "Some text to show that simple ", -1);
      insert_link (buffer, &iter, "hyper text", 3);
      ctk_text_buffer_insert (buffer, &iter, " can easily be realized with ", -1);
      insert_link (buffer, &iter, "tags", 2);
      ctk_text_buffer_insert (buffer, &iter, ".", -1);
    }
  else if (page == 2)
    {
      ctk_text_buffer_insert (buffer, &iter,
                              "A tag is an attribute that can be applied to some range of text. "
                              "For example, a tag might be called \"bold\" and make the text inside "
                              "the tag bold. However, the tag concept is more general than that; "
                              "tags don't have to affect appearance. They can instead affect the "
                              "behavior of mouse and key presses, \"lock\" a range of text so the "
                              "user can't edit it, or countless other things.\n", -1);
      insert_link (buffer, &iter, "Go back", 1);
    }
  else if (page == 3)
    {
      CtkTextTag *tag;

      tag = ctk_text_buffer_create_tag (buffer, NULL,
                                        "weight", PANGO_WEIGHT_BOLD,
                                        NULL);
      ctk_text_buffer_insert_with_tags (buffer, &iter, "hypertext:\n", -1, tag, NULL);
      ctk_text_buffer_insert (buffer, &iter,
                              "machine-readable text that is not sequential but is organized "
                              "so that related items of information are connected.\n", -1);
      insert_link (buffer, &iter, "Go back", 1);
    }
}

/* Looks at all tags covering the position of iter in the text view,
 * and if one of them is a link, follow it by showing the page identified
 * by the data attached to it.
 */
static void
follow_if_link (CtkWidget   *text_view,
                CtkTextIter *iter)
{
  GSList *tags = NULL, *tagp = NULL;

  tags = ctk_text_iter_get_tags (iter);
  for (tagp = tags;  tagp != NULL;  tagp = tagp->next)
    {
      CtkTextTag *tag = tagp->data;
      gint page = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (tag), "page"));

      if (page != 0)
        {
          show_page (ctk_text_view_get_buffer (CTK_TEXT_VIEW (text_view)), page);
          break;
        }
    }

  if (tags)
    g_slist_free (tags);
}

/* Links can be activated by pressing Enter.
 */
static gboolean
key_press_event (CtkWidget *text_view,
                 CdkEventKey *event)
{
  CtkTextIter iter;
  CtkTextBuffer *buffer;

  switch (event->keyval)
    {
      case CDK_KEY_Return:
      case CDK_KEY_KP_Enter:
        buffer = ctk_text_view_get_buffer (CTK_TEXT_VIEW (text_view));
        ctk_text_buffer_get_iter_at_mark (buffer, &iter,
                                          ctk_text_buffer_get_insert (buffer));
        follow_if_link (text_view, &iter);
        break;

      default:
        break;
    }

  return FALSE;
}

/* Links can also be activated by clicking or tapping.
 */
static gboolean
event_after (CtkWidget *text_view,
             CdkEvent  *ev)
{
  CtkTextIter start, end, iter;
  CtkTextBuffer *buffer;
  gdouble ex, ey;
  gint x, y;

  if (ev->type == CDK_BUTTON_RELEASE)
    {
      CdkEventButton *event;

      event = (CdkEventButton *)ev;
      if (event->button != CDK_BUTTON_PRIMARY)
        return FALSE;

      ex = event->x;
      ey = event->y;
    }
  else if (ev->type == CDK_TOUCH_END)
    {
      CdkEventTouch *event;

      event = (CdkEventTouch *)ev;

      ex = event->x;
      ey = event->y;
    }
  else
    return FALSE;

  buffer = ctk_text_view_get_buffer (CTK_TEXT_VIEW (text_view));

  /* we shouldn't follow a link if the user has selected something */
  ctk_text_buffer_get_selection_bounds (buffer, &start, &end);
  if (ctk_text_iter_get_offset (&start) != ctk_text_iter_get_offset (&end))
    return FALSE;

  ctk_text_view_window_to_buffer_coords (CTK_TEXT_VIEW (text_view),
                                         CTK_TEXT_WINDOW_WIDGET,
                                         ex, ey, &x, &y);

  if (ctk_text_view_get_iter_at_location (CTK_TEXT_VIEW (text_view), &iter, x, y))
    follow_if_link (text_view, &iter);

  return TRUE;
}

static gboolean hovering_over_link = FALSE;
static CdkCursor *hand_cursor = NULL;
static CdkCursor *regular_cursor = NULL;

/* Looks at all tags covering the position (x, y) in the text view,
 * and if one of them is a link, change the cursor to the "hands" cursor
 * typically used by web browsers.
 */
static void
set_cursor_if_appropriate (CtkTextView    *text_view,
                           gint            x,
                           gint            y)
{
  GSList *tags = NULL, *tagp = NULL;
  CtkTextIter iter;
  gboolean hovering = FALSE;

  if (ctk_text_view_get_iter_at_location (text_view, &iter, x, y))
    {
      tags = ctk_text_iter_get_tags (&iter);
      for (tagp = tags;  tagp != NULL;  tagp = tagp->next)
        {
          CtkTextTag *tag = tagp->data;
          gint page = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (tag), "page"));

          if (page != 0)
            {
              hovering = TRUE;
              break;
            }
        }
    }

  if (hovering != hovering_over_link)
    {
      hovering_over_link = hovering;

      if (hovering_over_link)
        cdk_window_set_cursor (ctk_text_view_get_window (text_view, CTK_TEXT_WINDOW_TEXT), hand_cursor);
      else
        cdk_window_set_cursor (ctk_text_view_get_window (text_view, CTK_TEXT_WINDOW_TEXT), regular_cursor);
    }

  if (tags)
    g_slist_free (tags);
}

/* Update the cursor image if the pointer moved.
 */
static gboolean
motion_notify_event (CtkWidget      *text_view,
                     CdkEventMotion *event)
{
  gint x, y;

  ctk_text_view_window_to_buffer_coords (CTK_TEXT_VIEW (text_view),
                                         CTK_TEXT_WINDOW_WIDGET,
                                         event->x, event->y, &x, &y);

  set_cursor_if_appropriate (CTK_TEXT_VIEW (text_view), x, y);

  return FALSE;
}

CtkWidget *
do_hypertext (CtkWidget *do_widget)
{
  static CtkWidget *window = NULL;

  if (!window)
    {
      CtkWidget *view;
      CtkWidget *sw;
      CtkTextBuffer *buffer;
      CdkDisplay *display;

      display = ctk_widget_get_display (do_widget);
      hand_cursor = cdk_cursor_new_from_name (display, "pointer");
      regular_cursor = cdk_cursor_new_from_name (display, "text");

      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_title (CTK_WINDOW (window), "Hypertext");
      ctk_window_set_screen (CTK_WINDOW (window),
                             ctk_widget_get_screen (do_widget));
      ctk_window_set_default_size (CTK_WINDOW (window), 450, 450);

      g_signal_connect (window, "destroy",
                        G_CALLBACK (ctk_widget_destroyed), &window);

      ctk_container_set_border_width (CTK_CONTAINER (window), 0);

      view = ctk_text_view_new ();
      ctk_text_view_set_wrap_mode (CTK_TEXT_VIEW (view), CTK_WRAP_WORD);
      ctk_text_view_set_left_margin (CTK_TEXT_VIEW (view), 20);
      ctk_text_view_set_right_margin (CTK_TEXT_VIEW (view), 20);
      g_signal_connect (view, "key-press-event",
                        G_CALLBACK (key_press_event), NULL);
      g_signal_connect (view, "event-after",
                        G_CALLBACK (event_after), NULL);
      g_signal_connect (view, "motion-notify-event",
                        G_CALLBACK (motion_notify_event), NULL);

      buffer = ctk_text_view_get_buffer (CTK_TEXT_VIEW (view));

      sw = ctk_scrolled_window_new (NULL, NULL);
      ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (sw),
                                      CTK_POLICY_AUTOMATIC,
                                      CTK_POLICY_AUTOMATIC);
      ctk_container_add (CTK_CONTAINER (window), sw);
      ctk_container_add (CTK_CONTAINER (sw), view);

      show_page (buffer, 1);

      ctk_widget_show_all (sw);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show (window);
  else
    ctk_widget_destroy (window);

  return window;
}
