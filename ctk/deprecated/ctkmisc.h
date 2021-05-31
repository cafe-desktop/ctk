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
 * GTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CTK_MISC_H__
#define __CTK_MISC_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkwidget.h>


G_BEGIN_DECLS

#define CTK_TYPE_MISC		       (ctk_misc_get_type ())
#define CTK_MISC(obj)		       (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_MISC, CtkMisc))
#define CTK_MISC_CLASS(klass)	       (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_MISC, CtkMiscClass))
#define CTK_IS_MISC(obj)	       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_MISC))
#define CTK_IS_MISC_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_MISC))
#define CTK_MISC_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_MISC, CtkMiscClass))


typedef struct _CtkMisc              CtkMisc;
typedef struct _CtkMiscPrivate       CtkMiscPrivate;
typedef struct _CtkMiscClass         CtkMiscClass;

struct _CtkMisc
{
  CtkWidget widget;

  /*< private >*/
  CtkMiscPrivate *priv;
};

struct _CtkMiscClass
{
  CtkWidgetClass parent_class;

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

GDK_DEPRECATED_IN_3_14
GType   ctk_misc_get_type      (void) G_GNUC_CONST;
GDK_DEPRECATED_IN_3_14
void	ctk_misc_set_alignment (CtkMisc *misc,
				gfloat	 xalign,
				gfloat	 yalign);
GDK_DEPRECATED_IN_3_14
void    ctk_misc_get_alignment (CtkMisc *misc,
				gfloat  *xalign,
				gfloat  *yalign);
GDK_DEPRECATED_IN_3_14
void	ctk_misc_set_padding   (CtkMisc *misc,
				gint	 xpad,
				gint	 ypad);
GDK_DEPRECATED_IN_3_14
void    ctk_misc_get_padding   (CtkMisc *misc,
				gint    *xpad,
				gint    *ypad);

void   _ctk_misc_get_padding_and_border	(CtkMisc   *misc,
					 CtkBorder *border);

G_END_DECLS

#endif /* __CTK_MISC_H__ */
