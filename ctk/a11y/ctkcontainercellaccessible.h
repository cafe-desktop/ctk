/* CTK+ - accessibility implementations
 * Copyright 2001 Sun Microsystems Inc.
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

#ifndef __CTK_CONTAINER_CELL_ACCESSIBLE_H__
#define __CTK_CONTAINER_CELL_ACCESSIBLE_H__

#if !defined (__CTK_A11Y_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk-a11y.h> can be included directly."
#endif

#include <atk/atk.h>
#include <ctk/a11y/ctkcellaccessible.h>

G_BEGIN_DECLS

#define CTK_TYPE_CONTAINER_CELL_ACCESSIBLE            (ctk_container_cell_accessible_get_type ())
#define CTK_CONTAINER_CELL_ACCESSIBLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_CONTAINER_CELL_ACCESSIBLE, CtkContainerCellAccessible))
#define CTK_CONTAINER_CELL_ACCESSIBLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_CONTAINER_CELL_ACCESSIBLE, CtkContainerCellAccessibleClass))
#define CTK_IS_CONTAINER_CELL_ACCESSIBLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_CONTAINER_CELL_ACCESSIBLE))
#define CTK_IS_CONTAINER_CELL_ACCESSIBLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_CONTAINER_CELL_ACCESSIBLE))
#define CTK_CONTAINER_CELL_ACCESSIBLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_CONTAINER_CELL_ACCESSIBLE, CtkContainerCellAccessibleClass))

typedef struct _CtkContainerCellAccessible        CtkContainerCellAccessible;
typedef struct _CtkContainerCellAccessibleClass   CtkContainerCellAccessibleClass;
typedef struct _CtkContainerCellAccessiblePrivate CtkContainerCellAccessiblePrivate;

struct _CtkContainerCellAccessible
{
  CtkCellAccessible parent;

  CtkContainerCellAccessiblePrivate *priv;
};

struct _CtkContainerCellAccessibleClass
{
  CtkCellAccessibleClass parent_class;
};

CDK_AVAILABLE_IN_ALL
GType                       ctk_container_cell_accessible_get_type     (void);

CDK_AVAILABLE_IN_ALL
CtkContainerCellAccessible *ctk_container_cell_accessible_new          (void);
CDK_AVAILABLE_IN_ALL
void                        ctk_container_cell_accessible_add_child    (CtkContainerCellAccessible *container,
                                                                        CtkCellAccessible          *child);
CDK_AVAILABLE_IN_ALL
void                        ctk_container_cell_accessible_remove_child (CtkContainerCellAccessible *container,
                                                                        CtkCellAccessible          *child);
CDK_AVAILABLE_IN_ALL
GList                      *ctk_container_cell_accessible_get_children  (CtkContainerCellAccessible *container);

G_END_DECLS

#endif /* __CTK_CONTAINER_CELL_ACCESSIBLE_H__ */
