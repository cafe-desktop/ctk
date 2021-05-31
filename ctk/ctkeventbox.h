/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CTK_EVENT_BOX_H__
#define __CTK_EVENT_BOX_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkbin.h>


G_BEGIN_DECLS

#define CTK_TYPE_EVENT_BOX              (ctk_event_box_get_type ())
#define CTK_EVENT_BOX(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_EVENT_BOX, CtkEventBox))
#define CTK_EVENT_BOX_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_EVENT_BOX, CtkEventBoxClass))
#define CTK_IS_EVENT_BOX(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_EVENT_BOX))
#define CTK_IS_EVENT_BOX_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_EVENT_BOX))
#define CTK_EVENT_BOX_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_EVENT_BOX, CtkEventBoxClass))

typedef struct _CtkEventBox        CtkEventBox;
typedef struct _CtkEventBoxClass   CtkEventBoxClass;
typedef struct _CtkEventBoxPrivate CtkEventBoxPrivate;

struct _CtkEventBox
{
  CtkBin bin;

  /*< private >*/
  CtkEventBoxPrivate *priv;
};

/**
 * CtkEventBoxClass:
 * @parent_class: The parent class.
 */
struct _CtkEventBoxClass
{
  CtkBinClass parent_class;

  /*< private >*/

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

GDK_AVAILABLE_IN_ALL
GType      ctk_event_box_get_type           (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
CtkWidget* ctk_event_box_new                (void);
GDK_AVAILABLE_IN_ALL
gboolean   ctk_event_box_get_visible_window (CtkEventBox *event_box);
GDK_AVAILABLE_IN_ALL
void       ctk_event_box_set_visible_window (CtkEventBox *event_box,
                                             gboolean     visible_window);
GDK_AVAILABLE_IN_ALL
gboolean   ctk_event_box_get_above_child    (CtkEventBox *event_box);
GDK_AVAILABLE_IN_ALL
void       ctk_event_box_set_above_child    (CtkEventBox *event_box,
                                             gboolean     above_child);

G_END_DECLS

#endif /* __CTK_EVENT_BOX_H__ */
