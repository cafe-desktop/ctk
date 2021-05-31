/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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

#ifndef __CTK_SCROLLBAR_H__
#define __CTK_SCROLLBAR_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkrange.h>


G_BEGIN_DECLS

#define CTK_TYPE_SCROLLBAR            (ctk_scrollbar_get_type ())
#define CTK_SCROLLBAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_SCROLLBAR, GtkScrollbar))
#define CTK_SCROLLBAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_SCROLLBAR, GtkScrollbarClass))
#define CTK_IS_SCROLLBAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_SCROLLBAR))
#define CTK_IS_SCROLLBAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_SCROLLBAR))
#define CTK_SCROLLBAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_SCROLLBAR, GtkScrollbarClass))


typedef struct _GtkScrollbar        GtkScrollbar;
typedef struct _GtkScrollbarClass   GtkScrollbarClass;

struct _GtkScrollbar
{
  GtkRange range;
};

struct _GtkScrollbarClass
{
  GtkRangeClass parent_class;

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};


GDK_AVAILABLE_IN_ALL
GType       ctk_scrollbar_get_type (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
GtkWidget * ctk_scrollbar_new      (GtkOrientation  orientation,
                                    GtkAdjustment  *adjustment);

G_END_DECLS

#endif /* __CTK_SCROLLBAR_H__ */
