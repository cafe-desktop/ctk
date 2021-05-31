/*
 * Copyright © 2015 Red Hat Inc.
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
 *
 * Authors: Benjamin Otte <otte@gnome.org>
 */

#ifndef __CTK_BOX_GADGET_PRIVATE_H__
#define __CTK_BOX_GADGET_PRIVATE_H__

#include "ctk/ctkcssgadgetprivate.h"
#include "ctk/ctkenums.h"

G_BEGIN_DECLS

#define CTK_TYPE_BOX_GADGET           (ctk_box_gadget_get_type ())
#define CTK_BOX_GADGET(obj)           (G_TYPE_CHECK_INSTANCE_CAST (obj, CTK_TYPE_BOX_GADGET, CtkBoxGadget))
#define CTK_BOX_GADGET_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST (cls, CTK_TYPE_BOX_GADGET, CtkBoxGadgetClass))
#define CTK_IS_BOX_GADGET(obj)        (G_TYPE_CHECK_INSTANCE_TYPE (obj, CTK_TYPE_BOX_GADGET))
#define CTK_IS_BOX_GADGET_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE (obj, CTK_TYPE_BOX_GADGET))
#define CTK_BOX_GADGET_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_BOX_GADGET, CtkBoxGadgetClass))

typedef struct _CtkBoxGadget           CtkBoxGadget;
typedef struct _CtkBoxGadgetClass      CtkBoxGadgetClass;

struct _CtkBoxGadget
{
  CtkCssGadget parent;
};

struct _CtkBoxGadgetClass
{
  CtkCssGadgetClass  parent_class;
};

GType                   ctk_box_gadget_get_type                 (void) G_GNUC_CONST;

CtkCssGadget *          ctk_box_gadget_new                      (const char             *name,
                                                                 CtkWidget              *owner,
                                                                 CtkCssGadget           *parent,
                                                                 CtkCssGadget           *next_sibling);
CtkCssGadget *          ctk_box_gadget_new_for_node             (CtkCssNode             *node,
                                                                 CtkWidget              *owner);

void                    ctk_box_gadget_set_orientation          (CtkBoxGadget           *gadget,
                                                                 CtkOrientation          orientation);
void                    ctk_box_gadget_set_draw_focus           (CtkBoxGadget           *gadget,
                                                                 gboolean                draw_focus);
void                    ctk_box_gadget_set_draw_reverse         (CtkBoxGadget           *gadget,
                                                                 gboolean                draw_reverse);
void                    ctk_box_gadget_set_allocate_reverse     (CtkBoxGadget           *gadget,
                                                                 gboolean                allocate_reverse);

void                    ctk_box_gadget_set_align_reverse        (CtkBoxGadget           *gadget,
                                                                 gboolean                align_reverse);
void                    ctk_box_gadget_insert_widget            (CtkBoxGadget           *gadget,
                                                                 int                     pos,
                                                                 CtkWidget              *widget);
void                    ctk_box_gadget_remove_widget            (CtkBoxGadget           *gadget,
                                                                 CtkWidget              *widget);
void                    ctk_box_gadget_insert_gadget            (CtkBoxGadget           *gadget,
                                                                 int                     pos,
                                                                 CtkCssGadget           *cssgadget,
                                                                 gboolean                expand,
                                                                 CtkAlign                align);
void                    ctk_box_gadget_insert_gadget_before     (CtkBoxGadget           *gadget,
                                                                 CtkCssGadget           *sibling,
                                                                 CtkCssGadget           *cssgadget,
                                                                 gboolean                expand,
                                                                 CtkAlign                align);
void                    ctk_box_gadget_insert_gadget_after      (CtkBoxGadget           *gadget,
                                                                 CtkCssGadget           *sibling,
                                                                 CtkCssGadget           *cssgadget,
                                                                 gboolean                expand,
                                                                 CtkAlign                align);

void                    ctk_box_gadget_remove_gadget            (CtkBoxGadget           *gadget,
                                                                 CtkCssGadget           *cssgadget);
void                    ctk_box_gadget_reverse_children         (CtkBoxGadget           *gadget);

void                    ctk_box_gadget_set_gadget_expand        (CtkBoxGadget           *gadget,
                                                                 GObject                *object,
                                                                 gboolean                expand);
void                    ctk_box_gadget_set_gadget_align         (CtkBoxGadget           *gadget,
                                                                 GObject                *object,
                                                                 CtkAlign                align);

G_END_DECLS

#endif /* __CTK_BOX_GADGET_PRIVATE_H__ */
