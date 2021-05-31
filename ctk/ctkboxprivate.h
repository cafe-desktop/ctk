/* GTK - The GIMP Toolkit
 *
 * Copyright (C) 2011 Javier Jardón
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
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __CTK_BOX_PRIVATE_H__
#define __CTK_BOX_PRIVATE_H__

#include "ctkbox.h"
#include "ctkcssgadgetprivate.h"

G_BEGIN_DECLS


void        _ctk_box_set_old_defaults   (CtkBox         *box);
gboolean    _ctk_box_get_spacing_set    (CtkBox         *box);
void        _ctk_box_set_spacing_set    (CtkBox         *box,
                                         gboolean        spacing_set);
GList      *_ctk_box_get_children       (CtkBox         *box);

CtkCssGadget *ctk_box_get_gadget (CtkBox *box);


G_END_DECLS

#endif /* __CTK_BOX_PRIVATE_H__ */
