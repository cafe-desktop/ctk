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
 * Modified by the GTK+ Team and others 1997-2001.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 */

#ifndef __CTK_ARROW_H__
#define __CTK_ARROW_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <gtk/gtk.h> can be included directly."
#endif

#include <gtk/deprecated/gtkmisc.h>


G_BEGIN_DECLS


#define CTK_TYPE_ARROW                  (ctk_arrow_get_type ())
#define CTK_ARROW(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_ARROW, GtkArrow))
#define CTK_ARROW_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_ARROW, GtkArrowClass))
#define CTK_IS_ARROW(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_ARROW))
#define CTK_IS_ARROW_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_ARROW))
#define CTK_ARROW_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_ARROW, GtkArrowClass))

typedef struct _GtkArrow              GtkArrow;
typedef struct _GtkArrowPrivate       GtkArrowPrivate;
typedef struct _GtkArrowClass         GtkArrowClass;

struct _GtkArrow
{
  GtkMisc misc;

  /*< private >*/
  GtkArrowPrivate *priv;
};

struct _GtkArrowClass
{
  GtkMiscClass parent_class;

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};


GDK_DEPRECATED_IN_3_14
GType      ctk_arrow_get_type   (void) G_GNUC_CONST;
GDK_DEPRECATED_IN_3_14
GtkWidget* ctk_arrow_new        (GtkArrowType   arrow_type,
				 GtkShadowType  shadow_type);
GDK_DEPRECATED_IN_3_14
void       ctk_arrow_set        (GtkArrow      *arrow,
				 GtkArrowType   arrow_type,
				 GtkShadowType  shadow_type);


G_END_DECLS


#endif /* __CTK_ARROW_H__ */