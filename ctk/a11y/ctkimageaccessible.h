/* GTK+ - accessibility implementations
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

#ifndef __CTK_IMAGE_ACCESSIBLE_H__
#define __CTK_IMAGE_ACCESSIBLE_H__

#if !defined (__CTK_A11Y_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk-a11y.h> can be included directly."
#endif

#include <ctk/a11y/ctkwidgetaccessible.h>

G_BEGIN_DECLS

#define CTK_TYPE_IMAGE_ACCESSIBLE                      (ctk_image_accessible_get_type ())
#define CTK_IMAGE_ACCESSIBLE(obj)                      (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_IMAGE_ACCESSIBLE, CtkImageAccessible))
#define CTK_IMAGE_ACCESSIBLE_CLASS(klass)              (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_IMAGE_ACCESSIBLE, CtkImageAccessibleClass))
#define CTK_IS_IMAGE_ACCESSIBLE(obj)                   (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_IMAGE_ACCESSIBLE))
#define CTK_IS_IMAGE_ACCESSIBLE_CLASS(klass)           (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_IMAGE_ACCESSIBLE))
#define CTK_IMAGE_ACCESSIBLE_GET_CLASS(obj)            (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_IMAGE_ACCESSIBLE, CtkImageAccessibleClass))

typedef struct _CtkImageAccessible        CtkImageAccessible;
typedef struct _CtkImageAccessibleClass   CtkImageAccessibleClass;
typedef struct _CtkImageAccessiblePrivate CtkImageAccessiblePrivate;

struct _CtkImageAccessible
{
  CtkWidgetAccessible parent;

  CtkImageAccessiblePrivate *priv;
};

struct _CtkImageAccessibleClass
{
  CtkWidgetAccessibleClass parent_class;
};

GDK_AVAILABLE_IN_ALL
GType ctk_image_accessible_get_type (void);

G_END_DECLS

#endif /* __CTK_IMAGE_ACCESSIBLE_H__ */
