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

#ifndef __CDK_X11_EVENT_SOURCE_H__
#define __CDK_X11_EVENT_SOURCE_H__

#include "cdkeventtranslator.h"

G_BEGIN_DECLS

typedef struct _CdkEventSource CdkEventSource;

G_GNUC_INTERNAL
GSource * cdk_x11_event_source_new            (CdkDisplay *display);

G_GNUC_INTERNAL
void      cdk_x11_event_source_add_translator (CdkEventSource  *source,
                                               CdkEventTranslator *translator);

G_GNUC_INTERNAL
void      cdk_x11_event_source_select_events  (CdkEventSource *source,
                                               Window          window,
                                               CdkEventMask    event_mask,
                                               unsigned int    extra_x_mask);


G_END_DECLS

#endif /* __CDK_X11_EVENT_SOURCE_H__ */
