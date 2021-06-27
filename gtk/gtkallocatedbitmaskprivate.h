/*
 * Copyright © 2011 Red Hat Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Authors: Benjamin Otte <otte@gnome.org>
 */

#ifndef __CTK_ALLOCATED_BITMASK_PRIVATE_H__
#define __CTK_ALLOCATED_BITMASK_PRIVATE_H__

#include <glib.h>

G_BEGIN_DECLS

typedef struct _GtkBitmask GtkBitmask;

#define _ctk_bitmask_to_bits(mask) \
  (GPOINTER_TO_SIZE (mask) >> ((gsize) 1))

#define _ctk_bitmask_from_bits(bits) \
  GSIZE_TO_POINTER ((((gsize) (bits)) << 1) | 1)

#define _ctk_bitmask_is_allocated(mask) \
  (!(GPOINTER_TO_SIZE (mask) & 1))

#define CTK_BITMASK_N_DIRECT_BITS (sizeof (gsize) * 8 - 1)


GtkBitmask *   _ctk_allocated_bitmask_copy              (const GtkBitmask  *mask);
void           _ctk_allocated_bitmask_free              (GtkBitmask        *mask);

void           _ctk_allocated_bitmask_print             (const GtkBitmask  *mask,
                                                         GString           *string);

GtkBitmask *   _ctk_allocated_bitmask_intersect         (GtkBitmask        *mask,
                                                         const GtkBitmask  *other) G_GNUC_WARN_UNUSED_RESULT;
GtkBitmask *   _ctk_allocated_bitmask_union             (GtkBitmask        *mask,
                                                         const GtkBitmask  *other) G_GNUC_WARN_UNUSED_RESULT;
GtkBitmask *   _ctk_allocated_bitmask_subtract          (GtkBitmask        *mask,
                                                         const GtkBitmask  *other) G_GNUC_WARN_UNUSED_RESULT;

gboolean       _ctk_allocated_bitmask_get               (const GtkBitmask  *mask,
                                                         guint              index_);
GtkBitmask *   _ctk_allocated_bitmask_set               (GtkBitmask        *mask,
                                                         guint              index_,
                                                         gboolean           value) G_GNUC_WARN_UNUSED_RESULT;

GtkBitmask *   _ctk_allocated_bitmask_invert_range      (GtkBitmask        *mask,
                                                         guint              start,
                                                         guint              end) G_GNUC_WARN_UNUSED_RESULT;

gboolean       _ctk_allocated_bitmask_equals            (const GtkBitmask  *mask,
                                                         const GtkBitmask  *other);
gboolean       _ctk_allocated_bitmask_intersects        (const GtkBitmask  *mask,
                                                         const GtkBitmask  *other);

G_END_DECLS

#endif /* __CTK_ALLOCATED_BITMASK_PRIVATE_H__ */
