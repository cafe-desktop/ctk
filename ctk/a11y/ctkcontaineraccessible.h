/* GTK+ - accessibility implementations
 * Copyright 2001, 2002, 2003 Sun Microsystems Inc.
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

#ifndef __CTK_CONTAINER_ACCESSIBLE_H__
#define __CTK_CONTAINER_ACCESSIBLE_H__

#if !defined (__CTK_A11Y_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk-a11y.h> can be included directly."
#endif

#include <ctk/ctk.h>
#include <ctk/a11y/ctkwidgetaccessible.h>

G_BEGIN_DECLS

#define CTK_TYPE_CONTAINER_ACCESSIBLE                  (ctk_container_accessible_get_type ())
#define CTK_CONTAINER_ACCESSIBLE(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_CONTAINER_ACCESSIBLE, CtkContainerAccessible))
#define CTK_CONTAINER_ACCESSIBLE_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_CONTAINER_ACCESSIBLE, CtkContainerAccessibleClass))
#define CTK_IS_CONTAINER_ACCESSIBLE(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_CONTAINER_ACCESSIBLE))
#define CTK_IS_CONTAINER_ACCESSIBLE_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_CONTAINER_ACCESSIBLE))
#define CTK_CONTAINER_ACCESSIBLE_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_CONTAINER_ACCESSIBLE, CtkContainerAccessibleClass))

typedef struct _CtkContainerAccessible        CtkContainerAccessible;
typedef struct _CtkContainerAccessibleClass   CtkContainerAccessibleClass;
typedef struct _CtkContainerAccessiblePrivate CtkContainerAccessiblePrivate;

struct _CtkContainerAccessible
{
  CtkWidgetAccessible parent;

  CtkContainerAccessiblePrivate *priv;
};

struct _CtkContainerAccessibleClass
{
  CtkWidgetAccessibleClass parent_class;

  gint (*add_ctk)    (CtkContainer *container,
                      CtkWidget    *widget,
                      gpointer     data);
  gint (*remove_ctk) (CtkContainer *container,
                      CtkWidget    *widget,
                      gpointer     data);
};

GDK_AVAILABLE_IN_ALL
GType ctk_container_accessible_get_type (void);

G_END_DECLS

#endif /* __CTK_CONTAINER_ACCESSIBLE_H__ */
