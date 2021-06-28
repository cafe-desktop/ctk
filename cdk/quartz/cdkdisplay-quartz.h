/*
 * cdkdisplay-quartz.h
 *
 * Copyright 2017 Tom Schoonjans 
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GDK_QUARTZ_DISPLAY__
#define __GDK_QUARTZ_DISPLAY__

#include "cdkdisplayprivate.h"
#include "cdkkeys.h"
#include "cdkwindow.h"
#include "cdkinternals.h"
#include "cdkmain.h"
#include "cdkinternal-quartz.h"

G_BEGIN_DECLS


struct _CdkQuartzDisplay
{
  CdkDisplay parent_instance;
  NSRect geometry; /* In AppKit coordinates. */
  NSSize size; /* Aggregate size of displays in millimeters. */
  GPtrArray *monitors;
};

struct _CdkQuartzDisplayClass
{
  CdkDisplayClass parent_class;
};

/* Display methods - events */
void     _cdk_quartz_display_queue_events (CdkDisplay *display);
gboolean _cdk_quartz_display_has_pending  (CdkDisplay *display);

void       _cdk_quartz_display_event_data_copy (CdkDisplay     *display,
                                                const CdkEvent *src,
                                                CdkEvent       *dst);
void       _cdk_quartz_display_event_data_free (CdkDisplay     *display,
                                                CdkEvent       *event);

/* Display methods - cursor */
CdkCursor *_cdk_quartz_display_get_cursor_for_type     (CdkDisplay      *display,
                                                        CdkCursorType    type);
CdkCursor *_cdk_quartz_display_get_cursor_for_name     (CdkDisplay      *display,
                                                        const gchar     *name);
CdkCursor *_cdk_quartz_display_get_cursor_for_surface  (CdkDisplay      *display,
                                                        cairo_surface_t *surface,
                                                        gdouble          x,
                                                        gdouble          y);
gboolean   _cdk_quartz_display_supports_cursor_alpha   (CdkDisplay    *display);
gboolean   _cdk_quartz_display_supports_cursor_color   (CdkDisplay    *display);
void       _cdk_quartz_display_get_default_cursor_size (CdkDisplay *display,
                                                        guint      *width,
                                                        guint      *height);
void       _cdk_quartz_display_get_maximal_cursor_size (CdkDisplay *display,
                                                        guint      *width,
                                                        guint      *height);

/* Display methods - window */
void       _cdk_quartz_display_before_process_all_updates (CdkDisplay *display);
void       _cdk_quartz_display_after_process_all_updates  (CdkDisplay *display);
void       _cdk_quartz_display_create_window_impl (CdkDisplay    *display,
                                                   CdkWindow     *window,
                                                   CdkWindow     *real_parent,
                                                   CdkScreen     *screen,
                                                   CdkEventMask   event_mask,
                                                   CdkWindowAttr *attributes,
                                                   gint           attributes_mask);

/* Display methods - keymap */
CdkKeymap * _cdk_quartz_display_get_keymap (CdkDisplay *display);

/* Display methods - selection */
gboolean    _cdk_quartz_display_set_selection_owner (CdkDisplay *display,
                                                     CdkWindow  *owner,
                                                     CdkAtom     selection,
                                                     guint32     time,
                                                     gboolean    send_event);
CdkWindow * _cdk_quartz_display_get_selection_owner (CdkDisplay *display,
                                                     CdkAtom     selection);
gint        _cdk_quartz_display_get_selection_property (CdkDisplay     *display,
                                                        CdkWindow      *requestor,
                                                        guchar        **data,
                                                        CdkAtom        *ret_type,
                                                        gint           *ret_format);
void        _cdk_quartz_display_convert_selection      (CdkDisplay     *display,
                                                        CdkWindow      *requestor,
                                                        CdkAtom         selection,
                                                        CdkAtom         target,
                                                        guint32         time);
gint        _cdk_quartz_display_text_property_to_utf8_list (CdkDisplay     *display,
                                                            CdkAtom         encoding,
                                                            gint            format,
                                                            const guchar   *text,
                                                            gint            length,
                                                            gchar        ***list);
gchar *     _cdk_quartz_display_utf8_to_string_target      (CdkDisplay     *displayt,
                                                            const gchar    *str);
G_END_DECLS

#endif  /* __GDK_QUARTZ_DISPLAY__ */
