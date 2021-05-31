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
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Benjamin Otte <otte@gnome.org>
 */

#ifndef __CTK_BITMASK_PRIVATE_H__
#define __CTK_BITMASK_PRIVATE_H__

#include <glib.h>
#include "ctkallocatedbitmaskprivate.h"

G_BEGIN_DECLS

static inline CtkBitmask *      _ctk_bitmask_new                  (void);
static inline CtkBitmask *      _ctk_bitmask_copy                 (const CtkBitmask  *mask);
static inline void              _ctk_bitmask_free                 (CtkBitmask        *mask);

static inline char *            _ctk_bitmask_to_string            (const CtkBitmask  *mask);
static inline void              _ctk_bitmask_print                (const CtkBitmask  *mask,
                                                                   GString           *string);

static inline CtkBitmask *      _ctk_bitmask_intersect            (CtkBitmask        *mask,
                                                                   const CtkBitmask  *other) G_GNUC_WARN_UNUSED_RESULT;
static inline CtkBitmask *      _ctk_bitmask_union                (CtkBitmask        *mask,
                                                                   const CtkBitmask  *other) G_GNUC_WARN_UNUSED_RESULT;
static inline CtkBitmask *      _ctk_bitmask_subtract             (CtkBitmask        *mask,
                                                                   const CtkBitmask  *other) G_GNUC_WARN_UNUSED_RESULT;

static inline gboolean          _ctk_bitmask_get                  (const CtkBitmask  *mask,
                                                                   guint              index_);
static inline CtkBitmask *      _ctk_bitmask_set                  (CtkBitmask        *mask,
                                                                   guint              index_,
                                                                   gboolean           value) G_GNUC_WARN_UNUSED_RESULT;

static inline CtkBitmask *      _ctk_bitmask_invert_range         (CtkBitmask        *mask,
                                                                   guint              start,
                                                                   guint              end) G_GNUC_WARN_UNUSED_RESULT;

static inline gboolean          _ctk_bitmask_is_empty             (const CtkBitmask  *mask);
static inline gboolean          _ctk_bitmask_equals               (const CtkBitmask  *mask,
                                                                   const CtkBitmask  *other);
static inline gboolean          _ctk_bitmask_intersects           (const CtkBitmask  *mask,
                                                                   const CtkBitmask  *other);


/* This is the actual implementation of the functions declared above.
 * We put it in a separate file so people don’t get scared from looking at this
 * file when reading source code.
 */
#include "ctkbitmaskprivateimpl.h"


G_END_DECLS

#endif /* __CTK_BITMASK_PRIVATE_H__ */
