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

#ifndef __CTK_HSEPARATOR_H__
#define __CTK_HSEPARATOR_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <gtk/gtk.h> can be included directly."
#endif

#include <gtk/gtkseparator.h>

G_BEGIN_DECLS

#define CTK_TYPE_HSEPARATOR                  (ctk_hseparator_get_type ())
#define CTK_HSEPARATOR(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_HSEPARATOR, GtkHSeparator))
#define CTK_HSEPARATOR_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_HSEPARATOR, GtkHSeparatorClass))
#define CTK_IS_HSEPARATOR(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_HSEPARATOR))
#define CTK_IS_HSEPARATOR_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_HSEPARATOR))
#define CTK_HSEPARATOR_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_HSEPARATOR, GtkHSeparatorClass))


typedef struct _GtkHSeparator       GtkHSeparator;
typedef struct _GtkHSeparatorClass  GtkHSeparatorClass;

struct _GtkHSeparator
{
  GtkSeparator separator;
};

struct _GtkHSeparatorClass
{
  GtkSeparatorClass parent_class;
};


GDK_DEPRECATED_IN_3_2
GType      ctk_hseparator_get_type (void) G_GNUC_CONST;
GDK_DEPRECATED_IN_3_2_FOR(ctk_separator_new)
GtkWidget* ctk_hseparator_new      (void);

G_END_DECLS

#endif /* __CTK_HSEPARATOR_H__ */
