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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GTK_ACCESSIBILITY_MISC_H__
#define __GTK_ACCESSIBILITY_MISC_H__

#include <atk/atk.h>

G_BEGIN_DECLS

#define GTK_TYPE_MISC_IMPL (_ctk_misc_impl_get_type ())

typedef struct _GtkMiscImpl      GtkMiscImpl;
typedef struct _GtkMiscImplClass GtkMiscImplClass;

struct _GtkMiscImpl
{
  AtkMisc parent;
};

struct _GtkMiscImplClass
{
  AtkMiscClass parent_class;
};

GType _ctk_misc_impl_get_type (void);

G_END_DECLS

#endif /* __GTK_ACCESSIBILITY_MISC_H__ */
