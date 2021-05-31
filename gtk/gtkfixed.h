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

#ifndef __CTK_FIXED_H__
#define __CTK_FIXED_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <gtk/gtk.h> can be included directly."
#endif

#include <gtk/gtkcontainer.h>


G_BEGIN_DECLS

#define CTK_TYPE_FIXED                  (ctk_fixed_get_type ())
#define CTK_FIXED(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_FIXED, GtkFixed))
#define CTK_FIXED_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_FIXED, GtkFixedClass))
#define CTK_IS_FIXED(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_FIXED))
#define CTK_IS_FIXED_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_FIXED))
#define CTK_FIXED_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_FIXED, GtkFixedClass))

typedef struct _GtkFixed              GtkFixed;
typedef struct _GtkFixedPrivate       GtkFixedPrivate;
typedef struct _GtkFixedClass         GtkFixedClass;
typedef struct _GtkFixedChild         GtkFixedChild;

struct _GtkFixed
{
  GtkContainer container;

  /*< private >*/
  GtkFixedPrivate *priv;
};

struct _GtkFixedClass
{
  GtkContainerClass parent_class;

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

struct _GtkFixedChild
{
  GtkWidget *widget;
  gint x;
  gint y;
};


GDK_AVAILABLE_IN_ALL
GType      ctk_fixed_get_type          (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
GtkWidget* ctk_fixed_new               (void);
GDK_AVAILABLE_IN_ALL
void       ctk_fixed_put               (GtkFixed       *fixed,
                                        GtkWidget      *widget,
                                        gint            x,
                                        gint            y);
GDK_AVAILABLE_IN_ALL
void       ctk_fixed_move              (GtkFixed       *fixed,
                                        GtkWidget      *widget,
                                        gint            x,
                                        gint            y);


G_END_DECLS

#endif /* __CTK_FIXED_H__ */
