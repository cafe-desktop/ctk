/* CTK - The GIMP Toolkit
 *
 * Copyright (C) 2010 Javier Jardón
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */


/* The contents of a selection are returned in a CtkSelectionData
 * structure. selection/target identify the request.  type specifies
 * the type of the return; if length < 0, and the data should be
 * ignored. This structure has object semantics - no fields should be
 * modified directly, they should not be created directly, and
 * pointers to them should not be stored beyond the duration of a
 * callback. (If the last is changed, we’ll need to add reference
 * counting.) The time field gives the timestamp at which the data was
 * sent.
 */

#ifndef __CTK_SELECTION_PRIVATE_H__
#define __CTK_SELECTION_PRIVATE_H__

#include "ctkselection.h"

G_BEGIN_DECLS

struct _CtkSelectionData
{
  /*< private >*/
  CdkAtom       selection;
  CdkAtom       target;
  CdkAtom       type;
  gint          format;
  guchar       *data;
  gint          length;
  CdkDisplay   *display;
};

struct _CtkTargetList
{
  /*< private >*/
  GList *list;
  guint ref_count;
};

gboolean _ctk_selection_clear           (CtkWidget         *widget,
                                         CdkEventSelection *event);
gboolean _ctk_selection_request         (CtkWidget         *widget,
                                         CdkEventSelection *event);
gboolean _ctk_selection_incr_event      (CdkWindow         *window,
                                         CdkEventProperty  *event);
gboolean _ctk_selection_notify          (CtkWidget         *widget,
                                         CdkEventSelection *event);
gboolean _ctk_selection_property_notify (CtkWidget         *widget,
                                         CdkEventProperty  *event);

G_END_DECLS

#endif /* __CTK_SELECTION_PRIVATE_H__ */
