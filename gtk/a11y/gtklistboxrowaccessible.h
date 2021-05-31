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

#ifndef __CTK_LIST_BOX_ROW_ACCESSIBLE_H__
#define __CTK_LIST_BOX_ROW_ACCESSIBLE_H__

#if !defined (__CTK_A11Y_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <gtk/gtk-a11y.h> can be included directly."
#endif

#include <gtk/gtk.h>
#include <gtk/a11y/gtkcontaineraccessible.h>

G_BEGIN_DECLS

#define CTK_TYPE_LIST_BOX_ROW_ACCESSIBLE               (ctk_list_box_row_accessible_get_type ())
#define CTK_LIST_BOX_ROW_ACCESSIBLE(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_LIST_BOX_ROW_ACCESSIBLE, GtkListBoxRowAccessible))
#define CTK_LIST_BOX_ROW_ACCESSIBLE_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_LIST_BOX_ROW_ACCESSIBLE, GtkListBoxRowAccessibleClass))
#define CTK_IS_LIST_BOX_ROW_ACCESSIBLE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_LIST_BOX_ROW_ACCESSIBLE))
#define CTK_IS_LIST_BOX_ROW_ACCESSIBLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_LIST_BOX_ROW_ACCESSIBLE))
#define CTK_LIST_BOX_ROW_ACCESSIBLE_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_LIST_BOX_ROW_ACCESSIBLE, GtkListBoxRowAccessibleClass))

typedef struct _GtkListBoxRowAccessible        GtkListBoxRowAccessible;
typedef struct _GtkListBoxRowAccessibleClass   GtkListBoxRowAccessibleClass;

struct _GtkListBoxRowAccessible
{
  GtkContainerAccessible parent;
};

struct _GtkListBoxRowAccessibleClass
{
  GtkContainerAccessibleClass parent_class;
};

GDK_AVAILABLE_IN_3_10
GType ctk_list_box_row_accessible_get_type (void);

G_END_DECLS

#endif /* __CTK_LIST_BOX_ROW_ACCESSIBLE_H__ */
