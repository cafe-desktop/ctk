/* CTK+ - accessibility implementations
 * Copyright 2019 Samuel Thibault <sthibault@hypra.fr>
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

#ifndef __CTK_PLUG_ACCESSIBLE_H__
#define __CTK_PLUG_ACCESSIBLE_H__

#if !defined (__CTK_A11Y_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk-a11y.h> can be included directly."
#endif

#include <ctk/a11y/ctkwindowaccessible.h>

#if ATK_CHECK_VERSION(2,35,1)

#define CTK_HAVE_ATK_PLUG_SET_CHILD

G_BEGIN_DECLS

#define CTK_TYPE_PLUG_ACCESSIBLE                         (ctk_plug_accessible_get_type ())
#define CTK_PLUG_ACCESSIBLE(obj)                         (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_PLUG_ACCESSIBLE, CtkPlugAccessible))
#define CTK_PLUG_ACCESSIBLE_CLASS(klass)                 (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_PLUG_ACCESSIBLE, CtkPlugAccessibleClass))
#define CTK_IS_PLUG_ACCESSIBLE(obj)                      (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_PLUG_ACCESSIBLE))
#define CTK_IS_PLUG_ACCESSIBLE_CLASS(klass)              (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_PLUG_ACCESSIBLE))
#define CTK_PLUG_ACCESSIBLE_GET_CLASS(obj)               (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_PLUG_ACCESSIBLE, CtkPlugAccessibleClass))

typedef struct _CtkPlugAccessible        CtkPlugAccessible;
typedef struct _CtkPlugAccessibleClass   CtkPlugAccessibleClass;
typedef struct _CtkPlugAccessiblePrivate CtkPlugAccessiblePrivate;

struct _CtkPlugAccessible
{
  CtkWindowAccessible parent;

  CtkPlugAccessiblePrivate *priv;
};

struct _CtkPlugAccessibleClass
{
  CtkWindowAccessibleClass parent_class;
};

GDK_AVAILABLE_IN_ALL
GType ctk_plug_accessible_get_type (void);

GDK_AVAILABLE_IN_ALL
gchar *ctk_plug_accessible_get_id (CtkPlugAccessible *plug);

G_END_DECLS

#endif /* ATK_CHECK_VERSION(2,35,1) */

#endif /* __CTK_PLUG_ACCESSIBLE_H__ */
