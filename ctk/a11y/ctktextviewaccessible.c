/* CTK+ - accessibility implementations
 * Copyright 2001, 2002, 2003 Sun Microsystems Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <sys/types.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <glib-object.h>
#include <glib/gstdio.h>
#include <ctk/ctk.h>
#include "ctktextviewaccessibleprivate.h"
#include "ctktextbufferprivate.h"
#include "ctk/ctkwidgetprivate.h"

struct _CtkTextViewAccessiblePrivate
{
  gint insert_offset;
  gint selection_bound;
};

static void       insert_text_cb        (CtkTextBuffer    *buffer,
                                                         CtkTextIter      *arg1,
                                                         gchar            *arg2,
                                                         gint             arg3,
                                                         gpointer         user_data);
static void       delete_range_cb       (CtkTextBuffer    *buffer,
                                                         CtkTextIter      *arg1,
                                                         CtkTextIter      *arg2,
                                                         gpointer         user_data);
static void       delete_range_after_cb (CtkTextBuffer    *buffer,
                                                         CtkTextIter      *arg1,
                                                         CtkTextIter      *arg2,
                                                         gpointer         user_data);
static void       mark_set_cb           (CtkTextBuffer    *buffer,
                                                         CtkTextIter      *arg1,
                                                         CtkTextMark      *arg2,
                                                         gpointer         user_data);


static void atk_editable_text_interface_init      (AtkEditableTextIface      *iface);
static void atk_text_interface_init               (AtkTextIface              *iface);
static void atk_streamable_content_interface_init (AtkStreamableContentIface *iface);

G_DEFINE_TYPE_WITH_CODE (CtkTextViewAccessible, ctk_text_view_accessible, CTK_TYPE_CONTAINER_ACCESSIBLE,
                         G_ADD_PRIVATE (CtkTextViewAccessible)
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_EDITABLE_TEXT, atk_editable_text_interface_init)
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_TEXT, atk_text_interface_init)
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_STREAMABLE_CONTENT, atk_streamable_content_interface_init))


static void
ctk_text_view_accessible_initialize (AtkObject *obj,
                                     gpointer   data)
{
  ATK_OBJECT_CLASS (ctk_text_view_accessible_parent_class)->initialize (obj, data);

  obj->role = ATK_ROLE_TEXT;
}

static void
ctk_text_view_accessible_notify_ctk (GObject    *obj,
                                     GParamSpec *pspec)
{
  AtkObject *atk_obj;

  atk_obj = ctk_widget_get_accessible (CTK_WIDGET (obj));

  if (!strcmp (pspec->name, "editable"))
    {
      gboolean editable;

      editable = ctk_text_view_get_editable (CTK_TEXT_VIEW (obj));
      atk_object_notify_state_change (atk_obj, ATK_STATE_EDITABLE, editable);
    }
  else
    CTK_WIDGET_ACCESSIBLE_CLASS (ctk_text_view_accessible_parent_class)->notify_ctk (obj, pspec);
}

static AtkStateSet*
ctk_text_view_accessible_ref_state_set (AtkObject *accessible)
{
  AtkStateSet *state_set;
  CtkWidget *widget;

  state_set = ATK_OBJECT_CLASS (ctk_text_view_accessible_parent_class)->ref_state_set (accessible);

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (accessible));
  if (widget == NULL)
    {
      atk_state_set_add_state (state_set, ATK_STATE_DEFUNCT);
      return state_set;
    }

  if (ctk_text_view_get_editable (CTK_TEXT_VIEW (widget)))
    atk_state_set_add_state (state_set, ATK_STATE_EDITABLE);
  atk_state_set_add_state (state_set, ATK_STATE_MULTI_LINE);

  return state_set;
}

static void
ctk_text_view_accessible_change_buffer (CtkTextViewAccessible *accessible,
                                        CtkTextBuffer         *old_buffer,
                                        CtkTextBuffer         *new_buffer)
{
  if (old_buffer)
    {
      g_signal_handlers_disconnect_matched (old_buffer, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, accessible);

      g_signal_emit_by_name (accessible,
                             "text-changed::delete",
                             0,
                             ctk_text_buffer_get_char_count (old_buffer));
    }

  if (new_buffer)
    {
      g_signal_connect_after (new_buffer, "insert-text", G_CALLBACK (insert_text_cb), accessible);
      g_signal_connect (new_buffer, "delete-range", G_CALLBACK (delete_range_cb), accessible);
      g_signal_connect_after (new_buffer, "delete-range", G_CALLBACK (delete_range_after_cb), accessible);
      g_signal_connect_after (new_buffer, "mark-set", G_CALLBACK (mark_set_cb), accessible);

      g_signal_emit_by_name (accessible,
                             "text-changed::insert",
                             0,
                             ctk_text_buffer_get_char_count (new_buffer));
    }
}

static void
ctk_text_view_accessible_widget_set (CtkAccessible *accessible)
{
  ctk_text_view_accessible_change_buffer (CTK_TEXT_VIEW_ACCESSIBLE (accessible),
                                          NULL,
                                          ctk_text_view_get_buffer (CTK_TEXT_VIEW (ctk_accessible_get_widget (accessible))));
}

static void
ctk_text_view_accessible_widget_unset (CtkAccessible *accessible)
{
  ctk_text_view_accessible_change_buffer (CTK_TEXT_VIEW_ACCESSIBLE (accessible),
                                          ctk_text_view_get_buffer (CTK_TEXT_VIEW (ctk_accessible_get_widget (accessible))),
                                          NULL);
}

static void
ctk_text_view_accessible_class_init (CtkTextViewAccessibleClass *klass)
{
  AtkObjectClass  *class = ATK_OBJECT_CLASS (klass);
  CtkAccessibleClass *accessible_class = CTK_ACCESSIBLE_CLASS (klass);
  CtkWidgetAccessibleClass *widget_class = (CtkWidgetAccessibleClass*)klass;

  accessible_class->widget_set = ctk_text_view_accessible_widget_set;
  accessible_class->widget_unset = ctk_text_view_accessible_widget_unset;

  class->ref_state_set = ctk_text_view_accessible_ref_state_set;
  class->initialize = ctk_text_view_accessible_initialize;

  widget_class->notify_ctk = ctk_text_view_accessible_notify_ctk;
}

static void
ctk_text_view_accessible_init (CtkTextViewAccessible *accessible)
{
  accessible->priv = ctk_text_view_accessible_get_instance_private (accessible);
}

static gchar *
ctk_text_view_accessible_get_text (AtkText *text,
                                   gint     start_offset,
                                   gint     end_offset)
{
  CtkTextView *view;
  CtkTextBuffer *buffer;
  CtkTextIter start, end;
  CtkWidget *widget;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return NULL;

  view = CTK_TEXT_VIEW (widget);
  buffer = ctk_text_view_get_buffer (view);
  ctk_text_buffer_get_iter_at_offset (buffer, &start, start_offset);
  ctk_text_buffer_get_iter_at_offset (buffer, &end, end_offset);

  return ctk_text_buffer_get_text (buffer, &start, &end, FALSE);
}

static gchar *
ctk_text_view_accessible_get_text_after_offset (AtkText         *text,
                                                gint             offset,
                                                AtkTextBoundary  boundary_type,
                                                gint            *start_offset,
                                                gint            *end_offset)
{
  CtkWidget *widget;
  CtkTextView *view;
  CtkTextBuffer *buffer;
  CtkTextIter pos;
  CtkTextIter start, end;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return NULL;

  view = CTK_TEXT_VIEW (widget);
  buffer = ctk_text_view_get_buffer (view);
  ctk_text_buffer_get_iter_at_offset (buffer, &pos, offset);
  start = end = pos;
  if (boundary_type == ATK_TEXT_BOUNDARY_LINE_START)
    {
      ctk_text_view_forward_display_line (view, &end);
      start = end;
      ctk_text_view_forward_display_line (view, &end);
    }
  else if (boundary_type == ATK_TEXT_BOUNDARY_LINE_END)
    {
      ctk_text_view_forward_display_line_end (view, &end);
      start = end;
      ctk_text_view_forward_display_line (view, &end);
      ctk_text_view_forward_display_line_end (view, &end);
    }
  else
    _ctk_text_buffer_get_text_after (buffer, boundary_type, &pos, &start, &end);

  *start_offset = ctk_text_iter_get_offset (&start);
  *end_offset = ctk_text_iter_get_offset (&end);

  return ctk_text_buffer_get_slice (buffer, &start, &end, FALSE);
}

static gchar *
ctk_text_view_accessible_get_text_at_offset (AtkText         *text,
                                             gint             offset,
                                             AtkTextBoundary  boundary_type,
                                             gint            *start_offset,
                                             gint            *end_offset)
{
  CtkWidget *widget;
  CtkTextView *view;
  CtkTextBuffer *buffer;
  CtkTextIter pos;
  CtkTextIter start, end;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return NULL;

  view = CTK_TEXT_VIEW (widget);
  buffer = ctk_text_view_get_buffer (view);
  ctk_text_buffer_get_iter_at_offset (buffer, &pos, offset);
  start = end = pos;
  if (boundary_type == ATK_TEXT_BOUNDARY_LINE_START)
    {
      ctk_text_view_backward_display_line_start (view, &start);
      ctk_text_view_forward_display_line (view, &end);
    }
  else if (boundary_type == ATK_TEXT_BOUNDARY_LINE_END)
    {
      ctk_text_view_backward_display_line_start (view, &start);
      if (!ctk_text_iter_is_start (&start))
        {
          ctk_text_view_backward_display_line (view, &start);
          ctk_text_view_forward_display_line_end (view, &start);
        }
      ctk_text_view_forward_display_line_end (view, &end);
    }
  else
    _ctk_text_buffer_get_text_at (buffer, boundary_type, &pos, &start, &end);

  *start_offset = ctk_text_iter_get_offset (&start);
  *end_offset = ctk_text_iter_get_offset (&end);

  return ctk_text_buffer_get_slice (buffer, &start, &end, FALSE);
}

static gchar *
ctk_text_view_accessible_get_text_before_offset (AtkText         *text,
                                                 gint             offset,
                                                 AtkTextBoundary  boundary_type,
                                                 gint            *start_offset,
                                                 gint            *end_offset)
{
  CtkWidget *widget;
  CtkTextView *view;
  CtkTextBuffer *buffer;
  CtkTextIter pos;
  CtkTextIter start, end;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return NULL;

  view = CTK_TEXT_VIEW (widget);
  buffer = ctk_text_view_get_buffer (view);
  ctk_text_buffer_get_iter_at_offset (buffer, &pos, offset);
  start = end = pos;

  if (boundary_type == ATK_TEXT_BOUNDARY_LINE_START)
    {
      ctk_text_view_backward_display_line_start (view, &start);
      end = start;
      ctk_text_view_backward_display_line (view, &start);
      ctk_text_view_backward_display_line_start (view, &start);
    }
  else if (boundary_type == ATK_TEXT_BOUNDARY_LINE_END)
    {
      ctk_text_view_backward_display_line_start (view, &start);
      if (!ctk_text_iter_is_start (&start))
        {
          ctk_text_view_backward_display_line (view, &start);
          end = start;
          ctk_text_view_forward_display_line_end (view, &end);
          if (!ctk_text_iter_is_start (&start))
            {
              if (ctk_text_view_backward_display_line (view, &start))
                ctk_text_view_forward_display_line_end (view, &start);
              else
                ctk_text_iter_set_offset (&start, 0);
            }
        }
      else
        end = start;
    }
  else
    _ctk_text_buffer_get_text_before (buffer, boundary_type, &pos, &start, &end);

  *start_offset = ctk_text_iter_get_offset (&start);
  *end_offset = ctk_text_iter_get_offset (&end);

  return ctk_text_buffer_get_slice (buffer, &start, &end, FALSE);
}

static gunichar
ctk_text_view_accessible_get_character_at_offset (AtkText *text,
                                                  gint     offset)
{
  CtkWidget *widget;
  CtkTextIter start, end;
  CtkTextBuffer *buffer;
  gchar *string;
  gunichar unichar;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return '\0';

  buffer = ctk_text_view_get_buffer (CTK_TEXT_VIEW (widget));
  if (offset >= ctk_text_buffer_get_char_count (buffer))
    return '\0';

  ctk_text_buffer_get_iter_at_offset (buffer, &start, offset);
  end = start;
  ctk_text_iter_forward_char (&end);
  string = ctk_text_buffer_get_slice (buffer, &start, &end, FALSE);
  unichar = g_utf8_get_char (string);
  g_free (string);

  return unichar;
}

static gint
ctk_text_view_accessible_get_character_count (AtkText *text)
{
  CtkWidget *widget;
  CtkTextBuffer *buffer;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return 0;

  buffer = ctk_text_view_get_buffer (CTK_TEXT_VIEW (widget));
  return ctk_text_buffer_get_char_count (buffer);
}

static gint
get_insert_offset (CtkTextBuffer *buffer)
{
  CtkTextMark *insert;
  CtkTextIter iter;

  insert = ctk_text_buffer_get_insert (buffer);
  ctk_text_buffer_get_iter_at_mark (buffer, &iter, insert);
  return ctk_text_iter_get_offset (&iter);
}

static gint
ctk_text_view_accessible_get_caret_offset (AtkText *text)
{
  CtkWidget *widget;
  CtkTextBuffer *buffer;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return 0;

  buffer = ctk_text_view_get_buffer (CTK_TEXT_VIEW (widget));
  return get_insert_offset (buffer);
}

static gboolean
ctk_text_view_accessible_set_caret_offset (AtkText *text,
                                           gint     offset)
{
  CtkTextView *view;
  CtkWidget *widget;
  CtkTextBuffer *buffer;
  CtkTextIter iter;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return FALSE;

  view = CTK_TEXT_VIEW (widget);
  buffer = ctk_text_view_get_buffer (view);

  ctk_text_buffer_get_iter_at_offset (buffer,  &iter, offset);
  ctk_text_buffer_place_cursor (buffer, &iter);
  ctk_text_view_scroll_to_iter (view, &iter, 0, FALSE, 0, 0);

  return TRUE;
}

static gint
ctk_text_view_accessible_get_offset_at_point (AtkText      *text,
                                              gint          x,
                                              gint          y,
                                              AtkCoordType  coords)
{
  CtkTextView *view;
  CtkTextIter iter;
  gint x_widget, y_widget, x_window, y_window, buff_x, buff_y;
  CtkWidget *widget;
  CdkWindow *window;
  CdkRectangle rect;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return -1;

  view = CTK_TEXT_VIEW (widget);
  window = ctk_text_view_get_window (view, CTK_TEXT_WINDOW_WIDGET);
  cdk_window_get_origin (window, &x_widget, &y_widget);

  if (coords == ATK_XY_SCREEN)
    {
      x = x - x_widget;
      y = y - y_widget;
    }
  else if (coords == ATK_XY_WINDOW)
    {
      window = cdk_window_get_toplevel (window);
      cdk_window_get_origin (window, &x_window, &y_window);

      x = x - x_widget + x_window;
      y = y - y_widget + y_window;
    }
  else
    return -1;

  ctk_text_view_window_to_buffer_coords (view, CTK_TEXT_WINDOW_WIDGET,
                                         x, y, &buff_x, &buff_y);
  ctk_text_view_get_visible_rect (view, &rect);

  /* Clamp point to visible rectangle */
  buff_x = CLAMP (buff_x, rect.x, rect.x + rect.width - 1);
  buff_y = CLAMP (buff_y, rect.y, rect.y + rect.height - 1);

  ctk_text_view_get_iter_at_location (view, &iter, buff_x, buff_y);

  /* The iter at a location sometimes points to the next character.
   * See bug 111031. We work around that
   */
  ctk_text_view_get_iter_location (view, &iter, &rect);
  if (buff_x < rect.x)
    ctk_text_iter_backward_char (&iter);
  return ctk_text_iter_get_offset (&iter);
}

static void
ctk_text_view_accessible_get_character_extents (AtkText      *text,
                                                gint          offset,
                                                gint         *x,
                                                gint         *y,
                                                gint         *width,
                                                gint         *height,
                                                AtkCoordType  coords)
{
  CtkTextView *view;
  CtkTextBuffer *buffer;
  CtkTextIter iter;
  CtkWidget *widget;
  CdkRectangle rectangle;
  CdkWindow *window;
  gint x_widget, y_widget, x_window, y_window;

  *x = 0;
  *y = 0;
  *width = 0;
  *height = 0;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return;

  view = CTK_TEXT_VIEW (widget);
  buffer = ctk_text_view_get_buffer (view);
  ctk_text_buffer_get_iter_at_offset (buffer, &iter, offset);
  ctk_text_view_get_iter_location (view, &iter, &rectangle);

  window = ctk_text_view_get_window (view, CTK_TEXT_WINDOW_WIDGET);
  if (window == NULL)
    return;

  cdk_window_get_origin (window, &x_widget, &y_widget);

  *height = rectangle.height;
  *width = rectangle.width;

  ctk_text_view_buffer_to_window_coords (view, CTK_TEXT_WINDOW_WIDGET,
    rectangle.x, rectangle.y, x, y);
  if (coords == ATK_XY_WINDOW)
    {
      window = cdk_window_get_toplevel (window);
      cdk_window_get_origin (window, &x_window, &y_window);
      *x += x_widget - x_window;
      *y += y_widget - y_window;
    }
  else if (coords == ATK_XY_SCREEN)
    {
      *x += x_widget;
      *y += y_widget;
    }
  else
    {
      *x = 0;
      *y = 0;
      *height = 0;
      *width = 0;
    }
}

static AtkAttributeSet *
add_text_attribute (AtkAttributeSet  *attributes,
                    AtkTextAttribute  attr,
                    gchar            *value)
{
  AtkAttribute *at;

  at = g_new (AtkAttribute, 1);
  at->name = g_strdup (atk_text_attribute_get_name (attr));
  at->value = value;

  return g_slist_prepend (attributes, at);
}

static AtkAttributeSet *
add_text_int_attribute (AtkAttributeSet  *attributes,
                        AtkTextAttribute  attr,
                        gint              i)

{
  gchar *value;

  value = g_strdup (atk_text_attribute_get_value (attr, i));

  return add_text_attribute (attributes, attr, value);
}

static AtkAttributeSet *
ctk_text_view_accessible_get_run_attributes (AtkText *text,
                                             gint     offset,
                                             gint    *start_offset,
                                             gint    *end_offset)
{
  CtkTextView *view;
  CtkTextBuffer *buffer;
  CtkWidget *widget;
  CtkTextIter iter;
  AtkAttributeSet *attrib_set = NULL;
  GSList *tags, *temp_tags;
  gdouble scale = 1;
  gboolean val_set = FALSE;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return NULL;

  view = CTK_TEXT_VIEW (widget);
  buffer = ctk_text_view_get_buffer (view);

  ctk_text_buffer_get_iter_at_offset (buffer, &iter, offset);

  ctk_text_iter_forward_to_tag_toggle (&iter, NULL);
  *end_offset = ctk_text_iter_get_offset (&iter);

  ctk_text_iter_backward_to_tag_toggle (&iter, NULL);
  *start_offset = ctk_text_iter_get_offset (&iter);

  ctk_text_buffer_get_iter_at_offset (buffer, &iter, offset);

  tags = ctk_text_iter_get_tags (&iter);
  tags = g_slist_reverse (tags);

  temp_tags = tags;
  while (temp_tags && !val_set)
    {
      CtkTextTag *tag = CTK_TEXT_TAG (temp_tags->data);

      g_object_get (tag, "style-set", &val_set, NULL);
      if (val_set)
        {
          PangoStyle style;
          g_object_get (tag, "style", &style, NULL);
          attrib_set = add_text_int_attribute (attrib_set, ATK_TEXT_ATTR_STYLE, style);
        }
      temp_tags = temp_tags->next;
    }
  val_set = FALSE;

  temp_tags = tags;
  while (temp_tags && !val_set)
    {
      CtkTextTag *tag = CTK_TEXT_TAG (temp_tags->data);

      g_object_get (tag, "variant-set", &val_set, NULL);
      if (val_set)
        {
          PangoVariant variant;
          g_object_get (tag, "variant", &variant, NULL);
          attrib_set = add_text_int_attribute (attrib_set, ATK_TEXT_ATTR_VARIANT, variant);
        }
      temp_tags = temp_tags->next;
    }
  val_set = FALSE;

  temp_tags = tags;
  while (temp_tags && !val_set)
    {
      CtkTextTag *tag = CTK_TEXT_TAG (temp_tags->data);

      g_object_get (tag, "stretch-set", &val_set, NULL);
      if (val_set)
        {
          PangoStretch stretch;
          g_object_get (tag, "stretch", &stretch, NULL);
          attrib_set = add_text_int_attribute (attrib_set, ATK_TEXT_ATTR_STRETCH, stretch);
        }
      temp_tags = temp_tags->next;
    }
  val_set = FALSE;

  temp_tags = tags;
  while (temp_tags && !val_set)
    {
      CtkTextTag *tag = CTK_TEXT_TAG (temp_tags->data);

      g_object_get (tag, "justification-set", &val_set, NULL);
      if (val_set)
        {
          CtkJustification justification;
          g_object_get (tag, "justification", &justification, NULL);
          attrib_set = add_text_int_attribute (attrib_set, ATK_TEXT_ATTR_JUSTIFICATION, justification);
        }
      temp_tags = temp_tags->next;
    }
  val_set = FALSE;

  temp_tags = tags;
  while (temp_tags && !val_set)
    {
      CtkTextTag *tag = CTK_TEXT_TAG (temp_tags->data);
      CtkTextDirection direction;

      g_object_get (tag, "direction", &direction, NULL);

      if (direction != CTK_TEXT_DIR_NONE)
        {
          val_set = TRUE;
          attrib_set = add_text_int_attribute (attrib_set, ATK_TEXT_ATTR_DIRECTION, direction);
        }
      temp_tags = temp_tags->next;
    }
  val_set = FALSE;

  temp_tags = tags;
  while (temp_tags && !val_set)
    {
      CtkTextTag *tag = CTK_TEXT_TAG (temp_tags->data);

      g_object_get (tag, "wrap-mode-set", &val_set, NULL);
      if (val_set)
        {
          CtkWrapMode wrap_mode;
          g_object_get (tag, "wrap-mode", &wrap_mode, NULL);
          attrib_set = add_text_int_attribute (attrib_set, ATK_TEXT_ATTR_WRAP_MODE, wrap_mode);
        }
      temp_tags = temp_tags->next;
    }
  val_set = FALSE;

  temp_tags = tags;
  while (temp_tags && !val_set)
    {
      CtkTextTag *tag = CTK_TEXT_TAG (temp_tags->data);

      g_object_get (tag, "foreground-set", &val_set, NULL);
      if (val_set)
        {
          CdkRGBA *rgba;
          gchar *value;

          g_object_get (tag, "foreground-rgba", &rgba, NULL);
          value = g_strdup_printf ("%u,%u,%u",
                                   (guint) rgba->red * 65535,
                                   (guint) rgba->green * 65535,
                                   (guint) rgba->blue * 65535);
          cdk_rgba_free (rgba);
          attrib_set = add_text_attribute (attrib_set, ATK_TEXT_ATTR_FG_COLOR, value);
        }
      temp_tags = temp_tags->next;
    }
  val_set = FALSE;

  temp_tags = tags;
  while (temp_tags && !val_set)
    {
      CtkTextTag *tag = CTK_TEXT_TAG (temp_tags->data);

      g_object_get (tag, "background-set", &val_set, NULL);
      if (val_set)
        {
          CdkRGBA *rgba;
          gchar *value;

          g_object_get (tag, "background-rgba", &rgba, NULL);
          value = g_strdup_printf ("%u,%u,%u",
                                   (guint) rgba->red * 65535,
                                   (guint) rgba->green * 65535,
                                   (guint) rgba->blue * 65535);
          cdk_rgba_free (rgba);
          attrib_set = add_text_attribute (attrib_set, ATK_TEXT_ATTR_BG_COLOR, value);
        }
      temp_tags = temp_tags->next;
    }
  val_set = FALSE;

  temp_tags = tags;
  while (temp_tags && !val_set)
    {
      CtkTextTag *tag = CTK_TEXT_TAG (temp_tags->data);

      g_object_get (tag, "family-set", &val_set, NULL);

      if (val_set)
        {
          gchar *value;
          g_object_get (tag, "family", &value, NULL);
          attrib_set = add_text_attribute (attrib_set, ATK_TEXT_ATTR_FAMILY_NAME, value);
        }
      temp_tags = temp_tags->next;
    }
  val_set = FALSE;

  temp_tags = tags;
  while (temp_tags && !val_set)
    {
      CtkTextTag *tag = CTK_TEXT_TAG (temp_tags->data);

      g_object_get (tag, "language-set", &val_set, NULL);

      if (val_set)
        {
          gchar *value;
          g_object_get (tag, "language", &value, NULL);
          attrib_set = add_text_attribute (attrib_set, ATK_TEXT_ATTR_LANGUAGE, value);
        }
      temp_tags = temp_tags->next;
    }
  val_set = FALSE;

  temp_tags = tags;
  while (temp_tags && !val_set)
    {
      CtkTextTag *tag = CTK_TEXT_TAG (temp_tags->data);

      g_object_get (tag, "weight-set", &val_set, NULL);

      if (val_set)
        {
          gint weight;
          gchar *value;
          g_object_get (tag, "weight", &weight, NULL);
          value = g_strdup_printf ("%d", weight);
          attrib_set = add_text_attribute (attrib_set, ATK_TEXT_ATTR_WEIGHT, value);
        }
      temp_tags = temp_tags->next;
    }
  val_set = FALSE;

  /* scale is special as the effective value is the product
   * of all specified values
   */
  temp_tags = tags;
  while (temp_tags)
    {
      CtkTextTag *tag = CTK_TEXT_TAG (temp_tags->data);
      gboolean scale_set;

      g_object_get (tag, "scale-set", &scale_set, NULL);
      if (scale_set)
        {
          gdouble font_scale;
          g_object_get (tag, "scale", &font_scale, NULL);
          val_set = TRUE;
          scale *= font_scale;
        }
      temp_tags = temp_tags->next;
    }
  if (val_set)
    {
      gchar *value;
      value = g_strdup_printf ("%g", scale);
      attrib_set = add_text_attribute (attrib_set, ATK_TEXT_ATTR_SCALE, value);
    }
  val_set = FALSE;

  temp_tags = tags;
  while (temp_tags && !val_set)
    {
      CtkTextTag *tag = CTK_TEXT_TAG (temp_tags->data);

      g_object_get (tag, "size-set", &val_set, NULL);
      if (val_set)
        {
          gint size;
          gchar *value;
          g_object_get (tag, "size", &size, NULL);
          value = g_strdup_printf ("%i", size);
          attrib_set = add_text_attribute (attrib_set, ATK_TEXT_ATTR_SIZE, value);
        }
      temp_tags = temp_tags->next;
    }
  val_set = FALSE;

  temp_tags = tags;
  while (temp_tags && !val_set)
    {
      CtkTextTag *tag = CTK_TEXT_TAG (temp_tags->data);

      g_object_get (tag, "strikethrough-set", &val_set, NULL);
      if (val_set)
        {
          gboolean strikethrough;
          g_object_get (tag, "strikethrough", &strikethrough, NULL);
          attrib_set = add_text_int_attribute (attrib_set, ATK_TEXT_ATTR_STRIKETHROUGH, strikethrough);
        }
      temp_tags = temp_tags->next;
    }
  val_set = FALSE;

  temp_tags = tags;
  while (temp_tags && !val_set)
    {
      CtkTextTag *tag = CTK_TEXT_TAG (temp_tags->data);

      g_object_get (tag, "underline-set", &val_set, NULL);
      if (val_set)
        {
          PangoUnderline underline;
          g_object_get (tag, "underline", &underline, NULL);
          attrib_set = add_text_int_attribute (attrib_set, ATK_TEXT_ATTR_UNDERLINE, underline);
        }
      temp_tags = temp_tags->next;
    }
  val_set = FALSE;

  temp_tags = tags;
  while (temp_tags && !val_set)
    {
      CtkTextTag *tag = CTK_TEXT_TAG (temp_tags->data);

      g_object_get (tag, "rise-set", &val_set, NULL);
      if (val_set)
        {
          gint rise;
          gchar *value;
          g_object_get (tag, "rise", &rise, NULL);
          value = g_strdup_printf ("%i", rise);
          attrib_set = add_text_attribute (attrib_set, ATK_TEXT_ATTR_RISE, value);
        }
      temp_tags = temp_tags->next;
    }
  val_set = FALSE;

  temp_tags = tags;
  while (temp_tags && !val_set)
    {
      CtkTextTag *tag = CTK_TEXT_TAG (temp_tags->data);

      g_object_get (tag, "background-full-height-set", &val_set, NULL);
      if (val_set)
        {
          gboolean bg_full_height;
          g_object_get (tag, "background-full-height", &bg_full_height, NULL);
          attrib_set = add_text_int_attribute (attrib_set, ATK_TEXT_ATTR_BG_FULL_HEIGHT, bg_full_height);
        }
      temp_tags = temp_tags->next;
    }
  val_set = FALSE;

  temp_tags = tags;
  while (temp_tags && !val_set)
    {
      CtkTextTag *tag = CTK_TEXT_TAG (temp_tags->data);

      g_object_get (tag, "pixels-inside-wrap-set", &val_set, NULL);
      if (val_set)
        {
          gint pixels;
          gchar *value;
          g_object_get (tag, "pixels-inside-wrap", &pixels, NULL);
          value = g_strdup_printf ("%i", pixels);
          attrib_set = add_text_attribute (attrib_set, ATK_TEXT_ATTR_PIXELS_INSIDE_WRAP, value);
        }
      temp_tags = temp_tags->next;
    }
  val_set = FALSE;

  temp_tags = tags;
  while (temp_tags && !val_set)
    {
      CtkTextTag *tag = CTK_TEXT_TAG (temp_tags->data);

      g_object_get (tag, "pixels-below-lines-set", &val_set, NULL);
      if (val_set)
        {
          gint pixels;
          gchar *value;
          g_object_get (tag, "pixels-below-lines", &pixels, NULL);
          value = g_strdup_printf ("%i", pixels);
          attrib_set = add_text_attribute (attrib_set, ATK_TEXT_ATTR_PIXELS_BELOW_LINES, value);
        }
      temp_tags = temp_tags->next;
    }
  val_set = FALSE;

  temp_tags = tags;
  while (temp_tags && !val_set)
    {
      CtkTextTag *tag = CTK_TEXT_TAG (temp_tags->data);

      g_object_get (tag, "pixels-above-lines-set", &val_set, NULL);
      if (val_set)
        {
          gint pixels;
          gchar *value;
          g_object_get (tag, "pixels-above-lines", &pixels, NULL);
          value = g_strdup_printf ("%i", pixels);
          attrib_set = add_text_attribute (attrib_set, ATK_TEXT_ATTR_PIXELS_ABOVE_LINES, value);
        }
      temp_tags = temp_tags->next;
    }
  val_set = FALSE;

  temp_tags = tags;
  while (temp_tags && !val_set)
    {
      CtkTextTag *tag = CTK_TEXT_TAG (temp_tags->data);

      g_object_get (tag, "editable-set", &val_set, NULL);
      if (val_set)
        {
          gboolean editable;
          g_object_get (tag, "editable", &editable, NULL);
          attrib_set = add_text_int_attribute (attrib_set, ATK_TEXT_ATTR_EDITABLE, editable);
        }
      temp_tags = temp_tags->next;
    }
  val_set = FALSE;

  temp_tags = tags;
  while (temp_tags && !val_set)
    {
      CtkTextTag *tag = CTK_TEXT_TAG (temp_tags->data);

      g_object_get (tag, "invisible-set", &val_set, NULL);
      if (val_set)
        {
          gboolean invisible;
          g_object_get (tag, "invisible", &invisible, NULL);
          attrib_set = add_text_int_attribute (attrib_set, ATK_TEXT_ATTR_INVISIBLE, invisible);
        }
      temp_tags = temp_tags->next;
    }
  val_set = FALSE;

  temp_tags = tags;
  while (temp_tags && !val_set)
    {
      CtkTextTag *tag = CTK_TEXT_TAG (temp_tags->data);

      g_object_get (tag, "indent-set", &val_set, NULL);
      if (val_set)
        {
          gint indent;
          gchar *value;
          g_object_get (tag, "indent", &indent, NULL);
          value = g_strdup_printf ("%i", indent);
          attrib_set = add_text_attribute (attrib_set, ATK_TEXT_ATTR_INDENT, value);
        }
      temp_tags = temp_tags->next;
    }
  val_set = FALSE;

  temp_tags = tags;
  while (temp_tags && !val_set)
    {
      CtkTextTag *tag = CTK_TEXT_TAG (temp_tags->data);

      g_object_get (tag, "right-margin-set", &val_set, NULL);
      if (val_set)
        {
          gint margin;
          gchar *value;
          g_object_get (tag, "right-margin", &margin, NULL);
          value = g_strdup_printf ("%i", margin);
          attrib_set = add_text_attribute (attrib_set, ATK_TEXT_ATTR_RIGHT_MARGIN, value);
        }
      temp_tags = temp_tags->next;
    }
  val_set = FALSE;

  temp_tags = tags;
  while (temp_tags && !val_set)
    {
      CtkTextTag *tag = CTK_TEXT_TAG (temp_tags->data);

      g_object_get (tag, "left-margin-set", &val_set, NULL);
      if (val_set)
        {
          gint margin;
          gchar *value;
          g_object_get (tag, "left-margin", &margin, NULL);
          value = g_strdup_printf ("%i", margin);
          attrib_set = add_text_attribute (attrib_set, ATK_TEXT_ATTR_LEFT_MARGIN, value);
        }
      temp_tags = temp_tags->next;
    }
  val_set = FALSE;

  g_slist_free (tags);
  return attrib_set;
}

static AtkAttributeSet *
ctk_text_view_accessible_get_default_attributes (AtkText *text)
{
  CtkTextView *view;
  CtkWidget *widget;
  CtkTextAttributes *text_attrs;
  AtkAttributeSet *attributes;
  PangoFontDescription *font;
  gchar *value;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return NULL;

  view = CTK_TEXT_VIEW (widget);
  text_attrs = ctk_text_view_get_default_attributes (view);

  attributes = NULL;

  font = text_attrs->font;

  if (font)
    {
      attributes = add_text_int_attribute (attributes, ATK_TEXT_ATTR_STYLE,
                                           pango_font_description_get_style (font));

      attributes = add_text_int_attribute (attributes, ATK_TEXT_ATTR_VARIANT,
                                           pango_font_description_get_variant (font));

      attributes = add_text_int_attribute (attributes, ATK_TEXT_ATTR_STRETCH,
                                           pango_font_description_get_stretch (font));

      value = g_strdup (pango_font_description_get_family (font));
      attributes = add_text_attribute (attributes, ATK_TEXT_ATTR_FAMILY_NAME, value);

      value = g_strdup_printf ("%d", pango_font_description_get_weight (font));
      attributes = add_text_attribute (attributes, ATK_TEXT_ATTR_WEIGHT, value);

      value = g_strdup_printf ("%i", pango_font_description_get_size (font) / PANGO_SCALE);
      attributes = add_text_attribute (attributes, ATK_TEXT_ATTR_SIZE, value);
    }

  attributes = add_text_int_attribute (attributes, ATK_TEXT_ATTR_JUSTIFICATION, text_attrs->justification);
  attributes = add_text_int_attribute (attributes, ATK_TEXT_ATTR_DIRECTION, text_attrs->direction);
  attributes = add_text_int_attribute (attributes, ATK_TEXT_ATTR_WRAP_MODE, text_attrs->wrap_mode);
  attributes = add_text_int_attribute (attributes, ATK_TEXT_ATTR_EDITABLE, text_attrs->editable);
  attributes = add_text_int_attribute (attributes, ATK_TEXT_ATTR_INVISIBLE, text_attrs->invisible);
  attributes = add_text_int_attribute (attributes, ATK_TEXT_ATTR_BG_FULL_HEIGHT, text_attrs->bg_full_height);

  attributes = add_text_int_attribute (attributes, ATK_TEXT_ATTR_STRIKETHROUGH,
                                       text_attrs->appearance.strikethrough);
  attributes = add_text_int_attribute (attributes, ATK_TEXT_ATTR_UNDERLINE,
                                       text_attrs->appearance.underline);

  value = g_strdup_printf ("%u,%u,%u",
                           text_attrs->appearance.bg_color.red,
                           text_attrs->appearance.bg_color.green,
                           text_attrs->appearance.bg_color.blue);
  attributes = add_text_attribute (attributes, ATK_TEXT_ATTR_BG_COLOR, value);

  value = g_strdup_printf ("%u,%u,%u",
                           text_attrs->appearance.fg_color.red,
                           text_attrs->appearance.fg_color.green,
                           text_attrs->appearance.fg_color.blue);
  attributes = add_text_attribute (attributes, ATK_TEXT_ATTR_FG_COLOR, value);

  value = g_strdup_printf ("%g", text_attrs->font_scale);
  attributes = add_text_attribute (attributes, ATK_TEXT_ATTR_SCALE, value);

  value = g_strdup ((gchar *)(text_attrs->language));
  attributes = add_text_attribute (attributes, ATK_TEXT_ATTR_LANGUAGE, value);

  value = g_strdup_printf ("%i", text_attrs->appearance.rise);
  attributes = add_text_attribute (attributes, ATK_TEXT_ATTR_RISE, value);

  value = g_strdup_printf ("%i", text_attrs->pixels_inside_wrap);
  attributes = add_text_attribute (attributes, ATK_TEXT_ATTR_PIXELS_INSIDE_WRAP, value);

  value = g_strdup_printf ("%i", text_attrs->pixels_below_lines);
  attributes = add_text_attribute (attributes, ATK_TEXT_ATTR_PIXELS_BELOW_LINES, value);

  value = g_strdup_printf ("%i", text_attrs->pixels_above_lines);
  attributes = add_text_attribute (attributes, ATK_TEXT_ATTR_PIXELS_ABOVE_LINES, value);

  value = g_strdup_printf ("%i", text_attrs->indent);
  attributes = add_text_attribute (attributes, ATK_TEXT_ATTR_INDENT, value);

  value = g_strdup_printf ("%i", text_attrs->left_margin);
  attributes = add_text_attribute (attributes, ATK_TEXT_ATTR_LEFT_MARGIN, value);

  value = g_strdup_printf ("%i", text_attrs->right_margin);
  attributes = add_text_attribute (attributes, ATK_TEXT_ATTR_RIGHT_MARGIN, value);

  ctk_text_attributes_unref (text_attrs);
  return attributes;
}

static gint
ctk_text_view_accessible_get_n_selections (AtkText *text)
{
  CtkWidget *widget;
  CtkTextBuffer *buffer;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return 0;

  buffer = ctk_text_view_get_buffer (CTK_TEXT_VIEW (widget));
  if (ctk_text_buffer_get_selection_bounds (buffer, NULL, NULL))
    return 1;

  return 0;
}

static gchar *
ctk_text_view_accessible_get_selection (AtkText *atk_text,
                                        gint     selection_num,
                                        gint    *start_pos,
                                        gint    *end_pos)
{
  CtkTextView *view;
  CtkWidget *widget;
  CtkTextBuffer *buffer;
  CtkTextIter start, end;
  gchar *text;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (atk_text));
  if (widget == NULL)
    return NULL;

  if (selection_num != 0)
    return NULL;

  view = CTK_TEXT_VIEW (widget);
  buffer = ctk_text_view_get_buffer (view);

  if (ctk_text_buffer_get_selection_bounds (buffer, &start, &end))
    text = ctk_text_buffer_get_text (buffer, &start, &end, FALSE);
  else
    text = NULL;

  *start_pos = ctk_text_iter_get_offset (&start);
  *end_pos = ctk_text_iter_get_offset (&end);

  return text;
}

static gboolean
ctk_text_view_accessible_add_selection (AtkText *text,
                                        gint     start_pos,
                                        gint     end_pos)
{
  CtkWidget *widget;
  CtkTextBuffer *buffer;
  CtkTextIter start, end;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return FALSE;

  buffer = ctk_text_view_get_buffer (CTK_TEXT_VIEW (widget));

  if (!ctk_text_buffer_get_selection_bounds (buffer, NULL, NULL))
    {
      ctk_text_buffer_get_iter_at_offset (buffer, &start, start_pos);
      ctk_text_buffer_get_iter_at_offset (buffer, &end, end_pos);
      ctk_text_buffer_select_range (buffer, &end, &start);

      return TRUE;
    }
  else
    return FALSE;
}

static gboolean
ctk_text_view_accessible_remove_selection (AtkText *text,
                                           gint     selection_num)
{
  CtkWidget *widget;
  CtkTextBuffer *buffer;
  CtkTextMark *insert;
  CtkTextIter iter;
  CtkTextIter start, end;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return FALSE;

  if (selection_num != 0)
     return FALSE;

  buffer = ctk_text_view_get_buffer (CTK_TEXT_VIEW (widget));

  if (ctk_text_buffer_get_selection_bounds (buffer, &start, &end))
    {
      insert = ctk_text_buffer_get_insert (buffer);
      ctk_text_buffer_get_iter_at_mark (buffer, &iter, insert);
      ctk_text_buffer_place_cursor (buffer, &iter);
      return TRUE;
    }
  else
    return FALSE;
}

static gboolean
ctk_text_view_accessible_set_selection (AtkText *text,
                                        gint     selection_num,
                                        gint     start_pos,
                                        gint     end_pos)
{
  CtkWidget *widget;
  CtkTextBuffer *buffer;
  CtkTextIter start, end;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return FALSE;

  if (selection_num != 0)
    return FALSE;

  buffer = ctk_text_view_get_buffer (CTK_TEXT_VIEW (widget));

  if (ctk_text_buffer_get_selection_bounds (buffer, &start, &end))
    {
      ctk_text_buffer_get_iter_at_offset (buffer, &start, start_pos);
      ctk_text_buffer_get_iter_at_offset (buffer, &end, end_pos);
      ctk_text_buffer_select_range (buffer, &end, &start);

      return TRUE;
    }
  else
    return FALSE;
}

static gboolean
ctk_text_view_accessible_scroll_substring_to(AtkText *text,
                                             gint start_offset,
                                             gint end_offset,
                                             AtkScrollType type)
{
  CtkTextView *view;
  CtkWidget *widget;
  CtkTextBuffer *buffer;
  CtkTextIter iter;
  gdouble xalign = -1.0, yalign = -1.0;
  gboolean use_align = TRUE;
  gint offset, rtl = 0;

  if (end_offset < start_offset)
    return FALSE;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return FALSE;

  view = CTK_TEXT_VIEW (widget);
  buffer = ctk_text_view_get_buffer (view);

  if (ctk_widget_get_direction (widget) == CTK_TEXT_DIR_RTL)
    rtl = 1;

  /*
   * Opportunistically pick which offset should be used to calculate
   * the scrolling factor.
   *
   * Considering only an extremity of the substring is good enough when
   * the selected string doesn't include line break and isn't larger than
   * the visible rectangle.
   */
  switch (type)
    {
    case ATK_SCROLL_TOP_LEFT:
      offset = (rtl) ? end_offset : start_offset;
      xalign = 0.0;
      yalign = 0.0;
      break;
    case ATK_SCROLL_BOTTOM_RIGHT:
      offset = (rtl) ? start_offset : end_offset;
      xalign = 1.0;
      yalign = 1.0;
      break;
    case ATK_SCROLL_TOP_EDGE:
      offset = start_offset;
      yalign = 0.0;
      break;
    case ATK_SCROLL_BOTTOM_EDGE:
      offset = end_offset;
      yalign = 1.0;
      break;
    case ATK_SCROLL_LEFT_EDGE:
      offset = (rtl) ? end_offset : start_offset;
      xalign = 0.0;
      break;
    case ATK_SCROLL_RIGHT_EDGE:
      offset = (rtl) ? start_offset : end_offset;
      xalign = 1.0;
      break;
    case ATK_SCROLL_ANYWHERE:
      offset = start_offset;
      use_align = FALSE;
      xalign = yalign = 0.0;
      break;
    default:
      return FALSE;
    }

  ctk_text_buffer_get_iter_at_offset (buffer, &iter, offset);

  /* Get current iter location to be able to scroll in a single direction. */
  if (use_align && (xalign == -1.0 || yalign == -1.0))
    {
      CdkRectangle rect, irect;

      ctk_text_view_get_visible_rect (view, &rect);
      ctk_text_view_get_iter_location (view, &iter, &irect);

      if (xalign == -1.0)
        xalign = ((gdouble) (irect.x - rect.x)) / (rect.width - 1);
      if (yalign == -1.0)
        yalign = ((gdouble) (irect.y - rect.y)) / (rect.height - 1);
    }

  ctk_text_view_scroll_to_iter (view, &iter, 0, use_align, xalign, yalign);

  return TRUE;
}

static void
atk_text_interface_init (AtkTextIface *iface)
{
  iface->get_text = ctk_text_view_accessible_get_text;
  iface->get_text_after_offset = ctk_text_view_accessible_get_text_after_offset;
  iface->get_text_at_offset = ctk_text_view_accessible_get_text_at_offset;
  iface->get_text_before_offset = ctk_text_view_accessible_get_text_before_offset;
  iface->get_character_at_offset = ctk_text_view_accessible_get_character_at_offset;
  iface->get_character_count = ctk_text_view_accessible_get_character_count;
  iface->get_caret_offset = ctk_text_view_accessible_get_caret_offset;
  iface->set_caret_offset = ctk_text_view_accessible_set_caret_offset;
  iface->get_offset_at_point = ctk_text_view_accessible_get_offset_at_point;
  iface->get_character_extents = ctk_text_view_accessible_get_character_extents;
  iface->get_n_selections = ctk_text_view_accessible_get_n_selections;
  iface->get_selection = ctk_text_view_accessible_get_selection;
  iface->add_selection = ctk_text_view_accessible_add_selection;
  iface->remove_selection = ctk_text_view_accessible_remove_selection;
  iface->set_selection = ctk_text_view_accessible_set_selection;
  iface->get_run_attributes = ctk_text_view_accessible_get_run_attributes;
  iface->get_default_attributes = ctk_text_view_accessible_get_default_attributes;
  iface->scroll_substring_to = ctk_text_view_accessible_scroll_substring_to;
}

/* atkeditabletext.h */

static gboolean
ctk_text_view_accessible_set_run_attributes (AtkEditableText *text,
                                             AtkAttributeSet *attributes,
                                             gint             start_offset,
                                             gint             end_offset)
{
  CtkTextView *view;
  CtkTextBuffer *buffer;
  CtkWidget *widget;
  CtkTextTag *tag;
  CtkTextIter start;
  CtkTextIter end;
  gint j;
  CdkColor *color;
  gchar** RGB_vals;
  GSList *l;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return FALSE;

  view = CTK_TEXT_VIEW (widget);
  if (!ctk_text_view_get_editable (view))
    return FALSE;

  buffer = ctk_text_view_get_buffer (view);

  if (attributes == NULL)
    return FALSE;

  ctk_text_buffer_get_iter_at_offset (buffer, &start, start_offset);
  ctk_text_buffer_get_iter_at_offset (buffer, &end, end_offset);

  tag = ctk_text_buffer_create_tag (buffer, NULL, NULL);

  for (l = attributes; l; l = l->next)
    {
      gchar *name;
      gchar *value;
      AtkAttribute *at;

      at = l->data;

      name = at->name;
      value = at->value;

      if (!strcmp (name, atk_text_attribute_get_name (ATK_TEXT_ATTR_LEFT_MARGIN)))
        g_object_set (G_OBJECT (tag), "left-margin", atoi (value), NULL);

      else if (!strcmp (name, atk_text_attribute_get_name (ATK_TEXT_ATTR_RIGHT_MARGIN)))
        g_object_set (G_OBJECT (tag), "right-margin", atoi (value), NULL);

      else if (!strcmp (name, atk_text_attribute_get_name (ATK_TEXT_ATTR_INDENT)))
        g_object_set (G_OBJECT (tag), "indent", atoi (value), NULL);

      else if (!strcmp (name, atk_text_attribute_get_name (ATK_TEXT_ATTR_PIXELS_ABOVE_LINES)))
        g_object_set (G_OBJECT (tag), "pixels-above-lines", atoi (value), NULL);

      else if (!strcmp(name, atk_text_attribute_get_name (ATK_TEXT_ATTR_PIXELS_BELOW_LINES)))
        g_object_set (G_OBJECT (tag), "pixels-below-lines", atoi (value), NULL);

      else if (!strcmp (name, atk_text_attribute_get_name (ATK_TEXT_ATTR_PIXELS_INSIDE_WRAP)))
        g_object_set (G_OBJECT (tag), "pixels-inside-wrap", atoi (value), NULL);

      else if (!strcmp (name, atk_text_attribute_get_name (ATK_TEXT_ATTR_SIZE)))
        g_object_set (G_OBJECT (tag), "size", atoi (value), NULL);

      else if (!strcmp (name, atk_text_attribute_get_name (ATK_TEXT_ATTR_RISE)))
        g_object_set (G_OBJECT (tag), "rise", atoi (value), NULL);

      else if (!strcmp (name, atk_text_attribute_get_name (ATK_TEXT_ATTR_WEIGHT)))
        g_object_set (G_OBJECT (tag), "weight", atoi (value), NULL);

      else if (!strcmp (name, atk_text_attribute_get_name (ATK_TEXT_ATTR_BG_FULL_HEIGHT)))
        {
          g_object_set (G_OBJECT (tag), "bg-full-height",
                   (strcmp (value, atk_text_attribute_get_value (ATK_TEXT_ATTR_BG_FULL_HEIGHT, 0))),
                   NULL);
        }

      else if (!strcmp (name, atk_text_attribute_get_name (ATK_TEXT_ATTR_LANGUAGE)))
        g_object_set (G_OBJECT (tag), "language", value, NULL);

      else if (!strcmp (name, atk_text_attribute_get_name (ATK_TEXT_ATTR_FAMILY_NAME)))
        g_object_set (G_OBJECT (tag), "family", value, NULL);

      else if (!strcmp (name, atk_text_attribute_get_name (ATK_TEXT_ATTR_EDITABLE)))
        {
          g_object_set (G_OBJECT (tag), "editable",
                   (strcmp (value, atk_text_attribute_get_value (ATK_TEXT_ATTR_EDITABLE, 0))),
                   NULL);
        }

      else if (!strcmp (name, atk_text_attribute_get_name (ATK_TEXT_ATTR_INVISIBLE)))
        {
          g_object_set (G_OBJECT (tag), "invisible",
                   (strcmp (value, atk_text_attribute_get_value (ATK_TEXT_ATTR_EDITABLE, 0))),
                   NULL);
        }

      else if (!strcmp (name, atk_text_attribute_get_name (ATK_TEXT_ATTR_UNDERLINE)))
        {
          for (j = 0; j < 3; j++)
            {
              if (!strcmp (value, atk_text_attribute_get_value (ATK_TEXT_ATTR_UNDERLINE, j)))
                {
                  g_object_set (G_OBJECT (tag), "underline", j, NULL);
                  break;
                }
            }
        }

      else if (!strcmp (name, atk_text_attribute_get_name (ATK_TEXT_ATTR_STRIKETHROUGH)))
        {
          g_object_set (G_OBJECT (tag), "strikethrough",
                   (strcmp (value, atk_text_attribute_get_value (ATK_TEXT_ATTR_STRIKETHROUGH, 0))),
                   NULL);
        }

      else if (!strcmp (name, atk_text_attribute_get_name (ATK_TEXT_ATTR_BG_COLOR)))
        {
          RGB_vals = g_strsplit (value, ",", 3);
          color = g_malloc (sizeof (CdkColor));
          color->red = atoi (RGB_vals[0]);
          color->green = atoi (RGB_vals[1]);
          color->blue = atoi (RGB_vals[2]);
          g_object_set (G_OBJECT (tag), "background-cdk", color, NULL);
        }
 
      else if (!strcmp (name, atk_text_attribute_get_name (ATK_TEXT_ATTR_FG_COLOR)))
        {
          RGB_vals = g_strsplit (value, ",", 3);
          color = g_malloc (sizeof (CdkColor));
          color->red = atoi (RGB_vals[0]);
          color->green = atoi (RGB_vals[1]);
          color->blue = atoi (RGB_vals[2]);
          g_object_set (G_OBJECT (tag), "foreground-cdk", color, NULL);
        }

      else if (!strcmp (name, atk_text_attribute_get_name (ATK_TEXT_ATTR_STRETCH)))
        {
          for (j = 0; j < 9; j++)
            {
              if (!strcmp (value, atk_text_attribute_get_value (ATK_TEXT_ATTR_STRETCH, j)))
                {
                  g_object_set (G_OBJECT (tag), "stretch", j, NULL);
                  break;
                }
            }
        }

      else if (!strcmp (name, atk_text_attribute_get_name (ATK_TEXT_ATTR_JUSTIFICATION)))
        {
          for (j = 0; j < 4; j++)
            {
              if (!strcmp (value, atk_text_attribute_get_value (ATK_TEXT_ATTR_JUSTIFICATION, j)))
                {
                  g_object_set (G_OBJECT (tag), "justification", j, NULL);
                  break;
                }
            }
        }

      else if (!strcmp (name, atk_text_attribute_get_name (ATK_TEXT_ATTR_DIRECTION)))
        {
          for (j = 0; j < 3; j++)
            {
              if (!strcmp (value, atk_text_attribute_get_value (ATK_TEXT_ATTR_DIRECTION, j)))
                {
                  g_object_set (G_OBJECT (tag), "direction", j, NULL);
                  break;
                }
            }
        }

      else if (!strcmp (name, atk_text_attribute_get_name (ATK_TEXT_ATTR_VARIANT)))
        {
          for (j = 0; j < 2; j++)
            {
              if (!strcmp (value, atk_text_attribute_get_value (ATK_TEXT_ATTR_VARIANT, j)))
                {
                  g_object_set (G_OBJECT (tag), "variant", j, NULL);
                  break;
                }
            }
        }

      else if (!strcmp (name, atk_text_attribute_get_name (ATK_TEXT_ATTR_WRAP_MODE)))
        {
          for (j = 0; j < 3; j++)
            {
              if (!strcmp (value, atk_text_attribute_get_value (ATK_TEXT_ATTR_WRAP_MODE, j)))
                {
                  g_object_set (G_OBJECT (tag), "wrap-mode", j, NULL);
                  break;
                }
            }
        }

      else if (!strcmp (name, atk_text_attribute_get_name (ATK_TEXT_ATTR_STYLE)))
        {
          for (j = 0; j < 3; j++)
            {
              if (!strcmp (value, atk_text_attribute_get_value (ATK_TEXT_ATTR_STYLE, j)))
                {
                  g_object_set (G_OBJECT (tag), "style", j, NULL);
                  break;
              }
            }
        }

      else
        return FALSE;
    }

  ctk_text_buffer_apply_tag (buffer, tag, &start, &end);

  return TRUE;
}

static void
ctk_text_view_accessible_set_text_contents (AtkEditableText *text,
                                            const gchar     *string)
{
  CtkTextView *view;
  CtkWidget *widget;
  CtkTextBuffer *buffer;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return;

  view = CTK_TEXT_VIEW (widget);
  if (!ctk_text_view_get_editable (view))
    return;

  buffer = ctk_text_view_get_buffer (view);
  ctk_text_buffer_set_text (buffer, string, -1);
}

static void
ctk_text_view_accessible_insert_text (AtkEditableText *text,
                                      const gchar     *string,
                                      gint             length,
                                      gint            *position)
{
  CtkTextView *view;
  CtkWidget *widget;
  CtkTextBuffer *buffer;
  CtkTextIter iter;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return;

  view = CTK_TEXT_VIEW (widget);
  if (!ctk_text_view_get_editable (view))
    return;

  buffer = ctk_text_view_get_buffer (view);
  ctk_text_buffer_get_iter_at_offset (buffer, &iter, *position);
  ctk_text_buffer_insert (buffer, &iter, string, length);
}

static void
ctk_text_view_accessible_copy_text (AtkEditableText *text,
                                    gint             start_pos,
                                    gint             end_pos)
{
  CtkWidget *widget;
  CtkTextBuffer *buffer;
  CtkTextIter start, end;
  gchar *str;
  CtkClipboard *clipboard;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return;

  buffer = ctk_text_view_get_buffer (CTK_TEXT_VIEW (widget));

  ctk_text_buffer_get_iter_at_offset (buffer, &start, start_pos);
  ctk_text_buffer_get_iter_at_offset (buffer, &end, end_pos);
  str = ctk_text_buffer_get_text (buffer, &start, &end, FALSE);

  clipboard = ctk_widget_get_clipboard (widget, GDK_SELECTION_CLIPBOARD);
  ctk_clipboard_set_text (clipboard, str, -1);
}

static void
ctk_text_view_accessible_cut_text (AtkEditableText *text,
                                   gint             start_pos,
                                   gint             end_pos)
{
  CtkTextView *view;
  CtkWidget *widget;
  CtkTextBuffer *buffer;
  CtkTextIter start, end;
  gchar *str;
  CtkClipboard *clipboard;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return;

  view = CTK_TEXT_VIEW (widget);
  if (!ctk_text_view_get_editable (view))
    return;
  buffer = ctk_text_view_get_buffer (view);

  ctk_text_buffer_get_iter_at_offset (buffer, &start, start_pos);
  ctk_text_buffer_get_iter_at_offset (buffer, &end, end_pos);
  str = ctk_text_buffer_get_text (buffer, &start, &end, FALSE);
  clipboard = ctk_widget_get_clipboard (widget, GDK_SELECTION_CLIPBOARD);
  ctk_clipboard_set_text (clipboard, str, -1);
  ctk_text_buffer_delete (buffer, &start, &end);
}

static void
ctk_text_view_accessible_delete_text (AtkEditableText *text,
                                      gint             start_pos,
                                      gint             end_pos)
{
  CtkTextView *view;
  CtkWidget *widget;
  CtkTextBuffer *buffer;
  CtkTextIter start_itr;
  CtkTextIter end_itr;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return;

  view = CTK_TEXT_VIEW (widget);
  if (!ctk_text_view_get_editable (view))
    return;
  buffer = ctk_text_view_get_buffer (view);

  ctk_text_buffer_get_iter_at_offset (buffer, &start_itr, start_pos);
  ctk_text_buffer_get_iter_at_offset (buffer, &end_itr, end_pos);
  ctk_text_buffer_delete (buffer, &start_itr, &end_itr);
}

typedef struct
{
  CtkTextBuffer* buffer;
  gint position;
} PasteData;

static void
paste_received (CtkClipboard *clipboard,
                const gchar  *text,
                gpointer      data)
{
  PasteData* paste = data;
  CtkTextIter pos_itr;

  if (text)
    {
      ctk_text_buffer_get_iter_at_offset (paste->buffer, &pos_itr, paste->position);
      ctk_text_buffer_insert (paste->buffer, &pos_itr, text, -1);
    }

  g_object_unref (paste->buffer);
}

static void
ctk_text_view_accessible_paste_text (AtkEditableText *text,
                                     gint             position)
{
  CtkTextView *view;
  CtkWidget *widget;
  CtkTextBuffer *buffer;
  PasteData paste;
  CtkClipboard *clipboard;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return;

  view = CTK_TEXT_VIEW (widget);
  if (!ctk_text_view_get_editable (view))
    return;
  buffer = ctk_text_view_get_buffer (view);

  paste.buffer = buffer;
  paste.position = position;

  g_object_ref (paste.buffer);
  clipboard = ctk_widget_get_clipboard (widget, GDK_SELECTION_CLIPBOARD);
  ctk_clipboard_request_text (clipboard, paste_received, &paste);
}

static void
atk_editable_text_interface_init (AtkEditableTextIface *iface)
{
  iface->set_text_contents = ctk_text_view_accessible_set_text_contents;
  iface->insert_text = ctk_text_view_accessible_insert_text;
  iface->copy_text = ctk_text_view_accessible_copy_text;
  iface->cut_text = ctk_text_view_accessible_cut_text;
  iface->delete_text = ctk_text_view_accessible_delete_text;
  iface->paste_text = ctk_text_view_accessible_paste_text;
  iface->set_run_attributes = ctk_text_view_accessible_set_run_attributes;
}

/* Callbacks */

static void
ctk_text_view_accessible_update_cursor (CtkTextViewAccessible *accessible,
                                        CtkTextBuffer *        buffer)
{
  int prev_insert_offset, prev_selection_bound;
  int insert_offset, selection_bound;
  CtkTextIter iter;

  prev_insert_offset = accessible->priv->insert_offset;
  prev_selection_bound = accessible->priv->selection_bound;

  ctk_text_buffer_get_iter_at_mark (buffer, &iter, ctk_text_buffer_get_insert (buffer));
  insert_offset = ctk_text_iter_get_offset (&iter);
  ctk_text_buffer_get_iter_at_mark (buffer, &iter, ctk_text_buffer_get_selection_bound (buffer));
  selection_bound = ctk_text_iter_get_offset (&iter);

  if (prev_insert_offset == insert_offset && prev_selection_bound == selection_bound)
    return;

  accessible->priv->insert_offset = insert_offset;
  accessible->priv->selection_bound = selection_bound;

  if (prev_insert_offset != insert_offset)
    g_signal_emit_by_name (accessible, "text-caret-moved", insert_offset);

  if (prev_insert_offset != prev_selection_bound || insert_offset != selection_bound)
    g_signal_emit_by_name (accessible, "text-selection-changed");
}

static void
insert_text_cb (CtkTextBuffer *buffer,
                CtkTextIter   *iter,
                gchar         *text,
                gint           len,
                gpointer       data)
{
  CtkTextViewAccessible *accessible = data;
  gint position;
  gint length;

  position = ctk_text_iter_get_offset (iter);
  length = g_utf8_strlen (text, len);
 
  g_signal_emit_by_name (accessible, "text-changed::insert", position - length, length);

  ctk_text_view_accessible_update_cursor (accessible, buffer);
}

static void
delete_range_cb (CtkTextBuffer *buffer,
                 CtkTextIter   *start,
                 CtkTextIter   *end,
                 gpointer       data)
{
  CtkTextViewAccessible *accessible = data;
  gint offset, length;

  offset = ctk_text_iter_get_offset (start);
  length = ctk_text_iter_get_offset (end) - offset;

  g_signal_emit_by_name (accessible,
                         "text-changed::delete",
                         offset,
                         length);
}

static void
delete_range_after_cb (CtkTextBuffer *buffer,
                       CtkTextIter   *start,
                       CtkTextIter   *end,
                       gpointer       data)
{
  CtkTextViewAccessible *accessible = data;

  ctk_text_view_accessible_update_cursor (accessible, buffer);
}

static void
mark_set_cb (CtkTextBuffer *buffer,
             CtkTextIter   *location,
             CtkTextMark   *mark,
             gpointer       data)
{
  CtkTextViewAccessible *accessible = data;

  /*
   * Only generate the signal for the "insert" mark, which
   * represents the cursor.
   */
  if (mark == ctk_text_buffer_get_insert (buffer))
    {
      ctk_text_view_accessible_update_cursor (accessible, buffer);
    }
  else if (mark == ctk_text_buffer_get_selection_bound (buffer))
    {
      ctk_text_view_accessible_update_cursor (accessible, buffer);
    }
}

static gint
gail_streamable_content_get_n_mime_types (AtkStreamableContent *streamable)
{
  CtkWidget *widget;
  CtkTextBuffer *buffer;
  gint n_mime_types = 0;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (streamable));
  if (widget == NULL)
    return 0;

  buffer = ctk_text_view_get_buffer (CTK_TEXT_VIEW (widget));
  if (buffer)
    {
      gint i;
      gboolean advertises_plaintext = FALSE;
      CdkAtom *atoms;

      atoms = ctk_text_buffer_get_serialize_formats (buffer, &n_mime_types);
      for (i = 0; i < n_mime_types-1; ++i)
        if (!strcmp ("text/plain", cdk_atom_name (atoms[i])))
            advertises_plaintext = TRUE;
      if (!advertises_plaintext)
        n_mime_types++;
    }

  return n_mime_types;
}

static const gchar *
gail_streamable_content_get_mime_type (AtkStreamableContent *streamable,
                                       gint                  i)
{
  CtkWidget *widget;
  CtkTextBuffer *buffer;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (streamable));
  if (widget == NULL)
    return 0;

  buffer = ctk_text_view_get_buffer (CTK_TEXT_VIEW (widget));
  if (buffer)
    {
      gint n_mime_types = 0;
      CdkAtom *atoms;

      atoms = ctk_text_buffer_get_serialize_formats (buffer, &n_mime_types);
      if (i < n_mime_types)
        return cdk_atom_name (atoms [i]);
      else if (i == n_mime_types)
        return "text/plain";
    }

  return NULL;
}

static GIOChannel *
gail_streamable_content_get_stream (AtkStreamableContent *streamable,
                                    const gchar          *mime_type)
{
  CtkWidget *widget;
  CtkTextBuffer *buffer;
  gint i, n_mime_types = 0;
  CdkAtom *atoms;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (streamable));
  if (widget == NULL)
    return 0;

  buffer = ctk_text_view_get_buffer (CTK_TEXT_VIEW (widget));
  if (!buffer)
    return NULL;

  atoms = ctk_text_buffer_get_serialize_formats (buffer, &n_mime_types);

  for (i = 0; i < n_mime_types; ++i)
    {
      if (!strcmp ("text/plain", mime_type) ||
          !strcmp (cdk_atom_name (atoms[i]), mime_type))
        {
          guint8 *cbuf;
          GError *err = NULL;
          gsize len, written;
          gchar tname[80];
          CtkTextIter start, end;
          GIOChannel *gio = NULL;
          int fd;

          ctk_text_buffer_get_iter_at_offset (buffer, &start, 0);
          ctk_text_buffer_get_iter_at_offset (buffer, &end, -1);
          if (!strcmp ("text/plain", mime_type))
            {
              cbuf = (guint8*) ctk_text_buffer_get_text (buffer, &start, &end, FALSE);
              len = strlen ((const char *) cbuf);
            }
          else
            {
              cbuf = ctk_text_buffer_serialize (buffer, buffer, atoms[i], &start, &end, &len);
            }
          g_snprintf (tname, 20, "streamXXXXXX");
          fd = g_mkstemp (tname);
          gio = g_io_channel_unix_new (fd);
          g_io_channel_set_encoding (gio, NULL, &err);
          if (!err)
            g_io_channel_write_chars (gio, (const char *) cbuf, (gssize) len, &written, &err);
          else
            g_message ("%s", err->message);
          if (!err)
            g_io_channel_seek_position (gio, 0, G_SEEK_SET, &err);
          else
            g_message ("%s", err->message);
          if (!err)
            g_io_channel_flush (gio, &err);
          else
            g_message ("%s", err->message);
          if (err)
            {
              g_message ("<error writing to stream [%s]>", tname);
              g_error_free (err);
            }
          /* make sure the file is removed on unref of the giochannel */
          else
            {
              g_unlink (tname);
              return gio;
            }
        }
    }

  return NULL;
}

static void
atk_streamable_content_interface_init (AtkStreamableContentIface *iface)
{
  iface->get_n_mime_types = gail_streamable_content_get_n_mime_types;
  iface->get_mime_type = gail_streamable_content_get_mime_type;
  iface->get_stream = gail_streamable_content_get_stream;
}

void
_ctk_text_view_accessible_set_buffer (CtkTextView   *textview,
                                      CtkTextBuffer *old_buffer)
{
  CtkTextViewAccessible *accessible;

  g_return_if_fail (CTK_IS_TEXT_VIEW (textview));
  g_return_if_fail (old_buffer == NULL || CTK_IS_TEXT_BUFFER (old_buffer));

  accessible = CTK_TEXT_VIEW_ACCESSIBLE (_ctk_widget_peek_accessible (CTK_WIDGET (textview)));
  if (accessible == NULL)
    return;

  ctk_text_view_accessible_change_buffer (accessible,
                                          old_buffer,
                                          ctk_text_view_get_buffer (textview));
}

