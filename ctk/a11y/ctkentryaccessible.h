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

#ifndef __CTK_ENTRY_ACCESSIBLE_H__
#define __CTK_ENTRY_ACCESSIBLE_H__

#if !defined (__CTK_A11Y_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk-a11y.h> can be included directly."
#endif

#include <ctk/a11y/ctkwidgetaccessible.h>

G_BEGIN_DECLS

#define CTK_TYPE_ENTRY_ACCESSIBLE                      (ctk_entry_accessible_get_type ())
#define CTK_ENTRY_ACCESSIBLE(obj)                      (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_ENTRY_ACCESSIBLE, CtkEntryAccessible))
#define CTK_ENTRY_ACCESSIBLE_CLASS(klass)              (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_ENTRY_ACCESSIBLE, CtkEntryAccessibleClass))
#define CTK_IS_ENTRY_ACCESSIBLE(obj)                   (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_ENTRY_ACCESSIBLE))
#define CTK_IS_ENTRY_ACCESSIBLE_CLASS(klass)           (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_ENTRY_ACCESSIBLE))
#define CTK_ENTRY_ACCESSIBLE_GET_CLASS(obj)            (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_ENTRY_ACCESSIBLE, CtkEntryAccessibleClass))

typedef struct _CtkEntryAccessible        CtkEntryAccessible;
typedef struct _CtkEntryAccessibleClass   CtkEntryAccessibleClass;
typedef struct _CtkEntryAccessiblePrivate CtkEntryAccessiblePrivate;

struct _CtkEntryAccessible
{
  CtkWidgetAccessible parent;

  CtkEntryAccessiblePrivate *priv;
};

struct _CtkEntryAccessibleClass
{
  CtkWidgetAccessibleClass parent_class;
};

GDK_AVAILABLE_IN_ALL
GType ctk_entry_accessible_get_type (void);

GDK_AVAILABLE_IN_ALL
GType ctk_entry_icon_accessible_get_type (void);

G_END_DECLS

#endif /* __CTK_ENTRY_ACCESSIBLE_H__ */
