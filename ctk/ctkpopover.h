/* GTK - The GIMP Toolkit
 * Copyright © 2013 Carlos Garnacho <carlosg@gnome.org>
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

#ifndef __CTK_POPOVER_H__
#define __CTK_POPOVER_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkwindow.h>

G_BEGIN_DECLS

#define CTK_TYPE_POPOVER           (ctk_popover_get_type ())
#define CTK_POPOVER(o)             (G_TYPE_CHECK_INSTANCE_CAST ((o), CTK_TYPE_POPOVER, CtkPopover))
#define CTK_POPOVER_CLASS(c)       (G_TYPE_CHECK_CLASS_CAST ((c), CTK_TYPE_POPOVER, CtkPopoverClass))
#define CTK_IS_POPOVER(o)          (G_TYPE_CHECK_INSTANCE_TYPE ((o), CTK_TYPE_POPOVER))
#define CTK_IS_POPOVER_CLASS(o)    (G_TYPE_CHECK_CLASS_TYPE ((o), CTK_TYPE_POPOVER))
#define CTK_POPOVER_GET_CLASS(o)   (G_TYPE_INSTANCE_GET_CLASS ((o), CTK_TYPE_POPOVER, CtkPopoverClass))

typedef struct _CtkPopover CtkPopover;
typedef struct _CtkPopoverClass CtkPopoverClass;
typedef struct _CtkPopoverPrivate CtkPopoverPrivate;

struct _CtkPopover
{
  CtkBin parent_instance;

  /*< private >*/

  CtkPopoverPrivate *priv;
};

struct _CtkPopoverClass
{
  CtkBinClass parent_class;

  void (* closed) (CtkPopover *popover);

  /*< private >*/

  /* Padding for future expansion */
  gpointer reserved[10];
};

GDK_AVAILABLE_IN_3_12
GType           ctk_popover_get_type        (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_3_12
CtkWidget *     ctk_popover_new             (CtkWidget             *relative_to);

GDK_AVAILABLE_IN_3_12
CtkWidget *     ctk_popover_new_from_model  (CtkWidget             *relative_to,
                                             GMenuModel            *model);

GDK_AVAILABLE_IN_3_12
void            ctk_popover_set_relative_to (CtkPopover            *popover,
                                             CtkWidget             *relative_to);
GDK_AVAILABLE_IN_3_12
CtkWidget *     ctk_popover_get_relative_to (CtkPopover            *popover);

GDK_AVAILABLE_IN_3_12
void            ctk_popover_set_pointing_to (CtkPopover            *popover,
                                             const GdkRectangle    *rect);
GDK_AVAILABLE_IN_3_12
gboolean        ctk_popover_get_pointing_to (CtkPopover            *popover,
                                             GdkRectangle          *rect);
GDK_AVAILABLE_IN_3_12
void            ctk_popover_set_position    (CtkPopover            *popover,
                                             CtkPositionType        position);
GDK_AVAILABLE_IN_3_12
CtkPositionType ctk_popover_get_position    (CtkPopover            *popover);

GDK_AVAILABLE_IN_3_12
void            ctk_popover_set_modal       (CtkPopover            *popover,
                                             gboolean               modal);
GDK_AVAILABLE_IN_3_12
gboolean        ctk_popover_get_modal       (CtkPopover            *popover);

GDK_AVAILABLE_IN_3_12
void            ctk_popover_bind_model      (CtkPopover            *popover,
                                             GMenuModel            *model,
                                             const gchar           *action_namespace);

GDK_DEPRECATED_IN_3_22
void            ctk_popover_set_transitions_enabled (CtkPopover *popover,
                                                     gboolean    transitions_enabled);
GDK_DEPRECATED_IN_3_22
gboolean        ctk_popover_get_transitions_enabled (CtkPopover *popover);

GDK_AVAILABLE_IN_3_18
void            ctk_popover_set_default_widget (CtkPopover *popover,
                                                CtkWidget  *widget);
GDK_AVAILABLE_IN_3_18
CtkWidget *     ctk_popover_get_default_widget (CtkPopover *popover);

GDK_AVAILABLE_IN_3_20
void                 ctk_popover_set_constrain_to (CtkPopover           *popover,
                                                   CtkPopoverConstraint  constraint);

GDK_AVAILABLE_IN_3_20
CtkPopoverConstraint ctk_popover_get_constrain_to (CtkPopover           *popover);

GDK_AVAILABLE_IN_3_22
void                 ctk_popover_popup            (CtkPopover *popover);

GDK_AVAILABLE_IN_3_22
void                 ctk_popover_popdown          (CtkPopover *popover);


G_END_DECLS

#endif /* __CTK_POPOVER_H__ */
