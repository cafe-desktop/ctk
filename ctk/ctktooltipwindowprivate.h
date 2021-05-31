/* GTK - The GIMP Toolkit
 * Copyright 2015  Emmanuele Bassi 
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

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CTK_TOOLTIP_WINDOW_PRIVATE_H__
#define __CTK_TOOLTIP_WINDOW_PRIVATE_H__

#include <gio/gio.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <ctk/ctkwindow.h>

G_BEGIN_DECLS

#define CTK_TYPE_TOOLTIP_WINDOW (ctk_tooltip_window_get_type ())

G_DECLARE_FINAL_TYPE (CtkTooltipWindow, ctk_tooltip_window, GTK, TOOLTIP_WINDOW, CtkWindow)

CtkWidget *     ctk_tooltip_window_new                          (void);

void            ctk_tooltip_window_set_label_markup             (CtkTooltipWindow *window,
                                                                 const char       *markup);
void            ctk_tooltip_window_set_label_text               (CtkTooltipWindow *window,
                                                                 const char       *text);
void            ctk_tooltip_window_set_image_icon               (CtkTooltipWindow *window,
                                                                 GdkPixbuf        *pixbuf);
void            ctk_tooltip_window_set_image_icon_from_stock    (CtkTooltipWindow *window,
                                                                 const char       *stock_id,
                                                                 CtkIconSize       icon_size);
void            ctk_tooltip_window_set_image_icon_from_name     (CtkTooltipWindow *window,
                                                                 const char       *icon_name,
                                                                 CtkIconSize       icon_size);
void            ctk_tooltip_window_set_image_icon_from_gicon    (CtkTooltipWindow *window,
                                                                 GIcon            *gicon,
                                                                 CtkIconSize       icon_size);
void            ctk_tooltip_window_set_custom_widget            (CtkTooltipWindow *window,
                                                                 CtkWidget        *custom_widget);

G_END_DECLS

#endif /* __CTK_TOOLTIP_WINDOW_PRIVATE_H__ */
