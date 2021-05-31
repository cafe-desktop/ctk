/* GTK - The GIMP Toolkit
 * ctksizegroup.h:
 * Copyright (C) 2000 Red Hat Software
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

#ifndef __CTK_SIZE_GROUP_H__
#define __CTK_SIZE_GROUP_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkwidget.h>

G_BEGIN_DECLS

#define CTK_TYPE_SIZE_GROUP            (ctk_size_group_get_type ())
#define CTK_SIZE_GROUP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_SIZE_GROUP, CtkSizeGroup))
#define CTK_SIZE_GROUP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_SIZE_GROUP, CtkSizeGroupClass))
#define CTK_IS_SIZE_GROUP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_SIZE_GROUP))
#define CTK_IS_SIZE_GROUP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_SIZE_GROUP))
#define CTK_SIZE_GROUP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_SIZE_GROUP, CtkSizeGroupClass))


typedef struct _CtkSizeGroup              CtkSizeGroup;
typedef struct _CtkSizeGroupPrivate       CtkSizeGroupPrivate;
typedef struct _CtkSizeGroupClass         CtkSizeGroupClass;

struct _CtkSizeGroup
{
  GObject parent_instance;

  /*< private >*/
  CtkSizeGroupPrivate *priv;
};

struct _CtkSizeGroupClass
{
  GObjectClass parent_class;

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

GDK_AVAILABLE_IN_ALL
GType            ctk_size_group_get_type      (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
CtkSizeGroup *   ctk_size_group_new           (CtkSizeGroupMode  mode);
GDK_AVAILABLE_IN_ALL
void             ctk_size_group_set_mode      (CtkSizeGroup     *size_group,
					       CtkSizeGroupMode  mode);
GDK_AVAILABLE_IN_ALL
CtkSizeGroupMode ctk_size_group_get_mode      (CtkSizeGroup     *size_group);
GDK_DEPRECATED_IN_3_22
void             ctk_size_group_set_ignore_hidden (CtkSizeGroup *size_group,
						   gboolean      ignore_hidden);
GDK_DEPRECATED_IN_3_22
gboolean         ctk_size_group_get_ignore_hidden (CtkSizeGroup *size_group);
GDK_AVAILABLE_IN_ALL
void             ctk_size_group_add_widget    (CtkSizeGroup     *size_group,
					       CtkWidget        *widget);
GDK_AVAILABLE_IN_ALL
void             ctk_size_group_remove_widget (CtkSizeGroup     *size_group,
					       CtkWidget        *widget);
GDK_AVAILABLE_IN_ALL
GSList *         ctk_size_group_get_widgets   (CtkSizeGroup     *size_group);

G_END_DECLS

#endif /* __CTK_SIZE_GROUP_H__ */
