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
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 */

#ifndef __CTK_SEPARATOR_H__
#define __CTK_SEPARATOR_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <gtk/gtk.h> can be included directly."
#endif

#include <gtk/gtkwidget.h>


G_BEGIN_DECLS

#define CTK_TYPE_SEPARATOR                  (ctk_separator_get_type ())
#define CTK_SEPARATOR(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_SEPARATOR, GtkSeparator))
#define CTK_SEPARATOR_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_SEPARATOR, GtkSeparatorClass))
#define CTK_IS_SEPARATOR(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_SEPARATOR))
#define CTK_IS_SEPARATOR_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_SEPARATOR))
#define CTK_SEPARATOR_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_SEPARATOR, GtkSeparatorClass))


typedef struct _GtkSeparator              GtkSeparator;
typedef struct _GtkSeparatorPrivate       GtkSeparatorPrivate;
typedef struct _GtkSeparatorClass         GtkSeparatorClass;

struct _GtkSeparator
{
  GtkWidget widget;

  GtkSeparatorPrivate *priv;
};

struct _GtkSeparatorClass
{
  GtkWidgetClass parent_class;

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};


GDK_AVAILABLE_IN_ALL
GType       ctk_separator_get_type (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
GtkWidget * ctk_separator_new      (GtkOrientation orientation);


G_END_DECLS

#endif /* __CTK_SEPARATOR_H__ */
