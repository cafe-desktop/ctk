/* CTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * CtkLayout: Widget for scrolling of arbitrary-sized areas.
 *
 * Copyright Owen Taylor, 1998
 */

/*
 * Modified by the CTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CTK_LAYOUT_H__
#define __CTK_LAYOUT_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkcontainer.h>


G_BEGIN_DECLS

#define CTK_TYPE_LAYOUT            (ctk_layout_get_type ())
#define CTK_LAYOUT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_LAYOUT, CtkLayout))
#define CTK_LAYOUT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_LAYOUT, CtkLayoutClass))
#define CTK_IS_LAYOUT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_LAYOUT))
#define CTK_IS_LAYOUT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_LAYOUT))
#define CTK_LAYOUT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_LAYOUT, CtkLayoutClass))


typedef struct _CtkLayout              CtkLayout;
typedef struct _CtkLayoutPrivate       CtkLayoutPrivate;
typedef struct _CtkLayoutClass         CtkLayoutClass;

struct _CtkLayout
{
  CtkContainer container;

  /*< private >*/
  CtkLayoutPrivate *priv;
};

struct _CtkLayoutClass
{
  CtkContainerClass parent_class;

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

GDK_AVAILABLE_IN_ALL
GType          ctk_layout_get_type        (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
CtkWidget*     ctk_layout_new             (CtkAdjustment *hadjustment,
				           CtkAdjustment *vadjustment);
GDK_AVAILABLE_IN_ALL
CdkWindow*     ctk_layout_get_bin_window  (CtkLayout     *layout);
GDK_AVAILABLE_IN_ALL
void           ctk_layout_put             (CtkLayout     *layout,
		                           CtkWidget     *child_widget,
		                           gint           x,
		                           gint           y);

GDK_AVAILABLE_IN_ALL
void           ctk_layout_move            (CtkLayout     *layout,
		                           CtkWidget     *child_widget,
		                           gint           x,
		                           gint           y);

GDK_AVAILABLE_IN_ALL
void           ctk_layout_set_size        (CtkLayout     *layout,
			                   guint          width,
			                   guint          height);
GDK_AVAILABLE_IN_ALL
void           ctk_layout_get_size        (CtkLayout     *layout,
					   guint         *width,
					   guint         *height);

GDK_DEPRECATED_IN_3_0_FOR(ctk_scrollable_get_hadjustment)
CtkAdjustment* ctk_layout_get_hadjustment (CtkLayout     *layout);
GDK_DEPRECATED_IN_3_0_FOR(ctk_scrollable_get_vadjustment)
CtkAdjustment* ctk_layout_get_vadjustment (CtkLayout     *layout);
GDK_DEPRECATED_IN_3_0_FOR(ctk_scrollable_set_hadjustment)
void           ctk_layout_set_hadjustment (CtkLayout     *layout,
                                           CtkAdjustment *adjustment);
GDK_DEPRECATED_IN_3_0_FOR(ctk_scrollable_set_vadjustment)
void           ctk_layout_set_vadjustment (CtkLayout     *layout,
                                           CtkAdjustment *adjustment);


G_END_DECLS

#endif /* __CTK_LAYOUT_H__ */
