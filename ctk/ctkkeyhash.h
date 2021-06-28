/* ctkkeyhash.h: Keymap aware matching of key bindings
 *
 * CTK - The GIMP Toolkit
 * Copyright (C) 2002, Red Hat Inc.
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

#ifndef __CTK_KEY_HASH_H__
#define __CTK_KEY_HASH_H__

#include <cdk/cdk.h>

G_BEGIN_DECLS

typedef struct _CtkKeyHash CtkKeyHash;

CtkKeyHash *_ctk_key_hash_new           (CdkKeymap       *keymap,
					 GDestroyNotify   item_destroy_notify);
void        _ctk_key_hash_add_entry     (CtkKeyHash      *key_hash,
					 guint            keyval,
					 CdkModifierType  modifiers,
					 gpointer         value);
void        _ctk_key_hash_remove_entry  (CtkKeyHash      *key_hash,
					 gpointer         value);
GSList *    _ctk_key_hash_lookup        (CtkKeyHash      *key_hash,
					 guint16          hardware_keycode,
					 CdkModifierType  state,
					 CdkModifierType  mask,
					 gint             group);
GSList *    _ctk_key_hash_lookup_keyval (CtkKeyHash      *key_hash,
					 guint            keyval,
					 CdkModifierType  modifiers);
void        _ctk_key_hash_free          (CtkKeyHash      *key_hash);

G_END_DECLS

#endif /* __CTK_KEY_HASH_H__ */
