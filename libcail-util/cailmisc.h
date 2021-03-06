/* CAIL - The GNOME Accessibility Implementation Library
 * Copyright 2001 Sun Microsystems Inc.
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

#ifndef __CAIL_MISC_H__
#define __CAIL_MISC_H__

#include <glib-object.h>
#include <ctk/ctk.h>
#include <pango/pango.h>

G_BEGIN_DECLS

CDK_AVAILABLE_IN_ALL
AtkAttributeSet* cail_misc_add_attribute          (AtkAttributeSet   *attrib_set,
                                                   AtkTextAttribute   attr,
                                                   gchar             *value);
CDK_AVAILABLE_IN_ALL
AtkAttributeSet* cail_misc_layout_get_run_attributes
                                                  (AtkAttributeSet   *attrib_set,
                                                   PangoLayout       *layout,
                                                   const gchar       *text,
                                                   gint              offset,
                                                   gint              *start_offset,
                                                   gint              *end_offset);

CDK_AVAILABLE_IN_ALL
AtkAttributeSet* cail_misc_get_default_attributes (AtkAttributeSet   *attrib_set,
                                                   PangoLayout       *layout,
                                                   CtkWidget         *widget);

CDK_AVAILABLE_IN_ALL
void             cail_misc_get_extents_from_pango_rectangle
                                                  (CtkWidget         *widget,
                                                   PangoRectangle    *char_rect,
                                                   gint              x_layout,
                                                   gint              y_layout,
                                                   gint              *x,
                    		                   gint              *y,
                                                   gint              *width,
                                                   gint              *height,
                                                   AtkCoordType      coords);

CDK_AVAILABLE_IN_ALL
gint             cail_misc_get_index_at_point_in_layout
                                                  (CtkWidget         *widget,
                                                   PangoLayout       *layout, 
                                                   gint              x_layout,
                                                   gint              y_layout,
                                                   gint              x,
                                                   gint              y,
                                                   AtkCoordType      coords);

CDK_AVAILABLE_IN_ALL
void		 cail_misc_get_origins            (CtkWidget         *widget,
                                                   gint              *x_window,
					           gint              *y_window,
					           gint              *x_toplevel,
					           gint              *y_toplevel);

CDK_AVAILABLE_IN_ALL
AtkAttributeSet* cail_misc_buffer_get_run_attributes
                                                  (CtkTextBuffer     *buffer,
                                                   gint              offset,
                                                   gint              *start_offset,
                                                   gint              *end_offset);

G_END_DECLS

#endif /*__CAIL_MISC_H__ */
