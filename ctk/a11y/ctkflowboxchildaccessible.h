/*
 * Copyright (C) 2013 Red Hat, Inc.
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

#ifndef __CTK_FLOW_BOX_CHILD_ACCESSIBLE_H__
#define __CTK_FLOW_BOX_CHILD_ACCESSIBLE_H__

#if !defined (__CTK_A11Y_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk-a11y.h> can be included directly."
#endif

#include <ctk/ctk.h>
#include <ctk/a11y/ctkcontaineraccessible.h>

G_BEGIN_DECLS

#define CTK_TYPE_FLOW_BOX_CHILD_ACCESSIBLE               (ctk_flow_box_child_accessible_get_type ())
#define CTK_FLOW_BOX_CHILD_ACCESSIBLE(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_FLOW_BOX_CHILD_ACCESSIBLE, CtkFlowBoxChildAccessible))
#define CTK_FLOW_BOX_CHILD_ACCESSIBLE_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_FLOW_BOX_CHILD_ACCESSIBLE, CtkFlowBoxChildAccessibleClass))
#define CTK_IS_FLOW_BOX_CHILD_ACCESSIBLE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_FLOW_BOX_CHILD_ACCESSIBLE))
#define CTK_IS_FLOW_BOX_CHILD_ACCESSIBLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_FLOW_BOX_CHILD_ACCESSIBLE))
#define CTK_FLOW_BOX_CHILD_ACCESSIBLE_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_FLOW_BOX_CHILD_ACCESSIBLE, CtkFlowBoxChildAccessibleClass))

typedef struct _CtkFlowBoxChildAccessible        CtkFlowBoxChildAccessible;
typedef struct _CtkFlowBoxChildAccessibleClass   CtkFlowBoxChildAccessibleClass;

struct _CtkFlowBoxChildAccessible
{
  CtkContainerAccessible parent;
};

struct _CtkFlowBoxChildAccessibleClass
{
  CtkContainerAccessibleClass parent_class;
};

GDK_AVAILABLE_IN_3_12
GType ctk_flow_box_child_accessible_get_type (void);

G_END_DECLS

#endif /* __CTK_FLOW_BOX_CHILD_ACCESSIBLE_H__ */
