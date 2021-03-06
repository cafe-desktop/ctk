/* CDK - The GIMP Drawing Kit
 * Copyright (C) 2009 Carlos Garnacho <carlosg@gnome.org>
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

#ifndef __CDK_EVENT_TRANSLATOR_H__
#define __CDK_EVENT_TRANSLATOR_H__

#include "cdktypes.h"
#include "cdkdisplay.h"

#include <X11/Xlib.h>

G_BEGIN_DECLS

#define CDK_TYPE_EVENT_TRANSLATOR         (_cdk_x11_event_translator_get_type ())
#define CDK_EVENT_TRANSLATOR(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), CDK_TYPE_EVENT_TRANSLATOR, CdkEventTranslator))
#define CDK_IS_EVENT_TRANSLATOR(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CDK_TYPE_EVENT_TRANSLATOR))
#define CDK_EVENT_TRANSLATOR_GET_IFACE(o) (G_TYPE_INSTANCE_GET_INTERFACE  ((o), CDK_TYPE_EVENT_TRANSLATOR, CdkEventTranslatorIface))

typedef struct _CdkEventTranslatorIface CdkEventTranslatorIface;
typedef struct _CdkEventTranslator CdkEventTranslator; /* Dummy typedef */

struct _CdkEventTranslatorIface
{
  GTypeInterface iface;

  /* VMethods */
  gboolean (* translate_event) (CdkEventTranslator *translator,
                                CdkDisplay         *display,
                                CdkEvent           *event,
                                XEvent             *xevent);

  CdkEventMask (* get_handled_events)   (CdkEventTranslator *translator);
  void         (* select_window_events) (CdkEventTranslator *translator,
                                         Window              window,
                                         CdkEventMask        event_mask);
  CdkWindow *  (* get_window)           (CdkEventTranslator *translator,
                                         XEvent             *xevent);
};

GType      _cdk_x11_event_translator_get_type (void) G_GNUC_CONST;

CdkEvent * _cdk_x11_event_translator_translate (CdkEventTranslator *translator,
                                               CdkDisplay         *display,
                                               XEvent             *xevent);
CdkEventMask _cdk_x11_event_translator_get_handled_events   (CdkEventTranslator *translator);
void         _cdk_x11_event_translator_select_window_events (CdkEventTranslator *translator,
                                                             Window              window,
                                                             CdkEventMask        event_mask);
CdkWindow *  _cdk_x11_event_translator_get_window           (CdkEventTranslator *translator,
                                                             CdkDisplay         *display,
                                                             XEvent             *xevent);

G_END_DECLS

#endif /* __CDK_EVENT_TRANSLATOR_H__ */
