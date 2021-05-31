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

#include "gtkbox.h"
#include "gtkcssgadgetprivate.h"

G_BEGIN_DECLS


void        _ctk_box_set_old_defaults   (GtkBox         *box);
gboolean    _ctk_box_get_spacing_set    (GtkBox         *box);
void        _ctk_box_set_spacing_set    (GtkBox         *box,
                                         gboolean        spacing_set);
GList      *_ctk_box_get_children       (GtkBox         *box);

GtkCssGadget *ctk_box_get_gadget (GtkBox *box);


G_END_DECLS

#endif /* __CTK_BOX_PRIVATE_H__ */