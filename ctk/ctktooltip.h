/* ctktooltip.h
 *
 * Copyright (C) 2006-2007 Imendio AB
 * Contact: Kristian Rietveld <kris@imendio.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __CTK_TOOLTIP_H__
#define __CTK_TOOLTIP_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkwindow.h>

G_BEGIN_DECLS

#define CTK_TYPE_TOOLTIP                 (ctk_tooltip_get_type ())
#define CTK_TOOLTIP(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_TOOLTIP, CtkTooltip))
#define CTK_IS_TOOLTIP(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_TOOLTIP))

CDK_AVAILABLE_IN_ALL
GType ctk_tooltip_get_type (void);

CDK_AVAILABLE_IN_ALL
void ctk_tooltip_set_markup              (CtkTooltip         *tooltip,
                                          const gchar        *markup);
CDK_AVAILABLE_IN_ALL
void ctk_tooltip_set_text                (CtkTooltip         *tooltip,
                                          const gchar        *text);
CDK_AVAILABLE_IN_ALL
void ctk_tooltip_set_icon                (CtkTooltip         *tooltip,
                                          CdkPixbuf          *pixbuf);
CDK_DEPRECATED_IN_3_10_FOR(ctk_tooltip_set_icon_from_icon_name)
void ctk_tooltip_set_icon_from_stock     (CtkTooltip         *tooltip,
                                          const gchar        *stock_id,
                                          CtkIconSize         size);
CDK_AVAILABLE_IN_ALL
void ctk_tooltip_set_icon_from_icon_name (CtkTooltip         *tooltip,
				          const gchar        *icon_name,
				          CtkIconSize         size);
CDK_AVAILABLE_IN_ALL
void ctk_tooltip_set_icon_from_gicon     (CtkTooltip         *tooltip,
					  GIcon              *gicon,
					  CtkIconSize         size);
CDK_AVAILABLE_IN_ALL
void ctk_tooltip_set_custom	         (CtkTooltip         *tooltip,
                                          CtkWidget          *custom_widget);

CDK_AVAILABLE_IN_ALL
void ctk_tooltip_set_tip_area            (CtkTooltip         *tooltip,
                                          const CdkRectangle *rect);

CDK_AVAILABLE_IN_ALL
void ctk_tooltip_trigger_tooltip_query   (CdkDisplay         *display);


G_END_DECLS

#endif /* __CTK_TOOLTIP_H__ */
