/* CTK - The GIMP Toolkit
 * Copyright (C) 2000 Red Hat Software
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

#ifndef __CTK_IM_CONTEXT_SIMPLE_PRIVATE_H__
#define __CTK_IM_CONTEXT_SIMPLE_PRIVATE_H__

#include <glib.h>

#include "cdk/cdkkeysyms.h"

G_BEGIN_DECLS

extern const CtkComposeTableCompact ctk_compose_table_compact;

gboolean ctk_check_algorithmically (const guint16                *compose_buffer,
                                    gint                          n_compose,
                                    gunichar                     *output);
gboolean ctk_check_compact_table   (const CtkComposeTableCompact *table,
                                    guint16                      *compose_buffer,
                                    gint                          n_compose,
                                    gboolean                     *compose_finish,
                                    gboolean                     *compose_match,
                                    gunichar                     *output_char);

G_END_DECLS


#endif /* __CTK_IM_CONTEXT_SIMPLE_PRIVATE_H__ */
