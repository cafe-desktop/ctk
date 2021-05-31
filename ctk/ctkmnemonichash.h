/* ctkmnemonichash.h: Sets of mnemonics with cycling
 *
 * GTK - The GIMP Toolkit
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

#ifndef __CTK_MNEMONIC_HASH_H__
#define __CTK_MNEMONIC_HASH_H__

#include <gdk/gdk.h>
#include <ctk/ctkwidget.h>

G_BEGIN_DECLS

typedef struct _CtkMnemnonicHash CtkMnemonicHash;

typedef void (*CtkMnemonicHashForeach) (guint      keyval,
					GSList    *targets,
					gpointer   data);

CtkMnemonicHash *_ctk_mnemonic_hash_new      (void);
void             _ctk_mnemonic_hash_free     (CtkMnemonicHash        *mnemonic_hash);
void             _ctk_mnemonic_hash_add      (CtkMnemonicHash        *mnemonic_hash,
					      guint                   keyval,
					      CtkWidget              *target);
void             _ctk_mnemonic_hash_remove   (CtkMnemonicHash        *mnemonic_hash,
					      guint                   keyval,
					      CtkWidget              *target);
gboolean         _ctk_mnemonic_hash_activate (CtkMnemonicHash        *mnemonic_hash,
					      guint                   keyval);
GSList *         _ctk_mnemonic_hash_lookup   (CtkMnemonicHash        *mnemonic_hash,
					      guint                   keyval);
void             _ctk_mnemonic_hash_foreach  (CtkMnemonicHash        *mnemonic_hash,
					      CtkMnemonicHashForeach  func,
					      gpointer                func_data);

G_END_DECLS

#endif /* __CTK_MNEMONIC_HASH_H__ */
