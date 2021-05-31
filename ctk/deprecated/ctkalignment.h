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

#ifndef __CTK_ALIGNMENT_H__
#define __CTK_ALIGNMENT_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <gtk/gtk.h> can be included directly."
#endif

#include <gtk/gtkbin.h>


G_BEGIN_DECLS

#define CTK_TYPE_ALIGNMENT                  (ctk_alignment_get_type ())
#define CTK_ALIGNMENT(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_ALIGNMENT, GtkAlignment))
#define CTK_ALIGNMENT_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_ALIGNMENT, GtkAlignmentClass))
#define CTK_IS_ALIGNMENT(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_ALIGNMENT))
#define CTK_IS_ALIGNMENT_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_ALIGNMENT))
#define CTK_ALIGNMENT_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_ALIGNMENT, GtkAlignmentClass))


typedef struct _GtkAlignment              GtkAlignment;
typedef struct _GtkAlignmentPrivate       GtkAlignmentPrivate;
typedef struct _GtkAlignmentClass         GtkAlignmentClass;

struct _GtkAlignment
{
  GtkBin bin;

  /*< private >*/
  GtkAlignmentPrivate *priv;
};

/**
 * GtkAlignmentClass:
 * @parent_class: The parent class.
 */
struct _GtkAlignmentClass
{
  GtkBinClass parent_class;

  /*< private >*/

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};


GDK_DEPRECATED_IN_3_14
GType      ctk_alignment_get_type   (void) G_GNUC_CONST;
GDK_DEPRECATED_IN_3_14
GtkWidget* ctk_alignment_new        (gfloat             xalign,
				     gfloat             yalign,
				     gfloat             xscale,
				     gfloat             yscale);
GDK_DEPRECATED_IN_3_14
void       ctk_alignment_set        (GtkAlignment      *alignment,
				     gfloat             xalign,
				     gfloat             yalign,
				     gfloat             xscale,
				     gfloat             yscale);

GDK_DEPRECATED_IN_3_14
void       ctk_alignment_set_padding (GtkAlignment      *alignment,
				      guint              padding_top,
				      guint              padding_bottom,
				      guint              padding_left,
				      guint              padding_right);

GDK_DEPRECATED_IN_3_14
void       ctk_alignment_get_padding (GtkAlignment      *alignment,
				      guint             *padding_top,
				      guint             *padding_bottom,
				      guint             *padding_left,
				      guint             *padding_right);

G_END_DECLS


#endif /* __CTK_ALIGNMENT_H__ */
