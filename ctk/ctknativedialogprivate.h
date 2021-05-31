/* GTK - The GIMP Toolkit
 * ctknativedialogprivate.h: Native dialog
 * Copyright (C) 2015, Red Hat, Inc.
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

#ifndef __CTK_NATIVE_DIALOG_PRIVATE_H__
#define __CTK_NATIVE_DIALOG_PRIVATE_H__

#include <ctk/ctknativedialog.h>

G_BEGIN_DECLS

void _ctk_native_dialog_emit_response (CtkNativeDialog *self,
                                       int              response_id);

G_END_DECLS

#endif /* __CTK_NATIVE_DIALOG_PRIVATE_H__ */
