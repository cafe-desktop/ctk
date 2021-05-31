/* CTK - The GIMP Toolkit
 * Copyright (C) 2017 Red Hat, Inc.
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

#ifndef __CTK_FONT_CHOOSER_WIDGET_PRIVATE_H__
#define __CTK_FONT_CHOOSER_WIDGET_PRIVATE_H__

#include "ctkfontchooserwidget.h"

G_BEGIN_DECLS

gboolean ctk_font_chooser_widget_handle_event (CtkWidget   *widget,
                                               GdkEventKey *event);

GAction *ctk_font_chooser_widget_get_tweak_action (CtkWidget *fontchooser);

G_END_DECLS

#endif /* __CTK_FONT_CHOOSER_WIDGET_PRIVATE_H__ */
