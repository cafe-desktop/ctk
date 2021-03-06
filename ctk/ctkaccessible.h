/* CTK - The GIMP Toolkit
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

#ifndef __CTK_ACCESSIBLE_H__
#define __CTK_ACCESSIBLE_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <atk/atk.h>
#include <ctk/ctkwidget.h>

G_BEGIN_DECLS

#define CTK_TYPE_ACCESSIBLE                  (ctk_accessible_get_type ())
#define CTK_ACCESSIBLE(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_ACCESSIBLE, CtkAccessible))
#define CTK_ACCESSIBLE_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_ACCESSIBLE, CtkAccessibleClass))
#define CTK_IS_ACCESSIBLE(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_ACCESSIBLE))
#define CTK_IS_ACCESSIBLE_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_ACCESSIBLE))
#define CTK_ACCESSIBLE_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_ACCESSIBLE, CtkAccessibleClass))

typedef struct _CtkAccessible        CtkAccessible;
typedef struct _CtkAccessiblePrivate CtkAccessiblePrivate;
typedef struct _CtkAccessibleClass   CtkAccessibleClass;

struct _CtkAccessible
{
  AtkObject parent;

  /*< private >*/
  CtkAccessiblePrivate *priv;
};

struct _CtkAccessibleClass
{
  AtkObjectClass parent_class;

  void (*connect_widget_destroyed) (CtkAccessible *accessible);

  void (*widget_set)               (CtkAccessible *accessible);
  void (*widget_unset)             (CtkAccessible *accessible);
  /* Padding for future expansion */
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

CDK_AVAILABLE_IN_ALL
GType      ctk_accessible_get_type                 (void) G_GNUC_CONST;

CDK_AVAILABLE_IN_ALL
void       ctk_accessible_set_widget               (CtkAccessible *accessible,
                                                    CtkWidget     *widget);
CDK_AVAILABLE_IN_ALL
CtkWidget *ctk_accessible_get_widget               (CtkAccessible *accessible);

CDK_DEPRECATED_IN_3_4_FOR(ctk_accessible_set_widget)
void       ctk_accessible_connect_widget_destroyed (CtkAccessible *accessible);

G_END_DECLS

#endif /* __CTK_ACCESSIBLE_H__ */


