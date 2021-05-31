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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 */

#ifndef __CTK_VBOX_H__
#define __CTK_VBOX_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <gtk/gtk.h> can be included directly."
#endif

#include <gtk/gtkbox.h>

G_BEGIN_DECLS

#define CTK_TYPE_VBOX		 (ctk_vbox_get_type ())
#define CTK_VBOX(obj)		 (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_VBOX, GtkVBox))
#define CTK_VBOX_CLASS(klass)	 (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_VBOX, GtkVBoxClass))
#define CTK_IS_VBOX(obj)	 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_VBOX))
#define CTK_IS_VBOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_VBOX))
#define CTK_VBOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_VBOX, GtkVBoxClass))


typedef struct _GtkVBox	      GtkVBox;
typedef struct _GtkVBoxClass  GtkVBoxClass;

struct _GtkVBox
{
  GtkBox box;
};

struct _GtkVBoxClass
{
  GtkBoxClass parent_class;
};


GDK_DEPRECATED_IN_3_2
GType       ctk_vbox_get_type (void) G_GNUC_CONST;
GDK_DEPRECATED_IN_3_2_FOR(ctk_box_new)
GtkWidget * ctk_vbox_new      (gboolean homogeneous,
                               gint     spacing);

G_END_DECLS

#endif /* __CTK_VBOX_H__ */
