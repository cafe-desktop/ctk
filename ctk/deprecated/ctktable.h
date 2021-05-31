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

#ifndef __CTK_TABLE_H__
#define __CTK_TABLE_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkcontainer.h>

G_BEGIN_DECLS

#define CTK_TYPE_TABLE			(ctk_table_get_type ())
#define CTK_TABLE(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_TABLE, CtkTable))
#define CTK_TABLE_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_TABLE, CtkTableClass))
#define CTK_IS_TABLE(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_TABLE))
#define CTK_IS_TABLE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_TABLE))
#define CTK_TABLE_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_TABLE, CtkTableClass))


typedef struct _CtkTable              CtkTable;
typedef struct _CtkTablePrivate       CtkTablePrivate;
typedef struct _CtkTableClass         CtkTableClass;
typedef struct _CtkTableChild         CtkTableChild;
typedef struct _CtkTableRowCol        CtkTableRowCol;

struct _CtkTable
{
  CtkContainer container;

  /*< private >*/
  CtkTablePrivate *priv;
};

struct _CtkTableClass
{
  CtkContainerClass parent_class;

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

struct _CtkTableChild
{
  CtkWidget *widget;
  guint16 left_attach;
  guint16 right_attach;
  guint16 top_attach;
  guint16 bottom_attach;
  guint16 xpadding;
  guint16 ypadding;
  guint xexpand : 1;
  guint yexpand : 1;
  guint xshrink : 1;
  guint yshrink : 1;
  guint xfill : 1;
  guint yfill : 1;
};

struct _CtkTableRowCol
{
  guint16 requisition;
  guint16 allocation;
  guint16 spacing;
  guint need_expand : 1;
  guint need_shrink : 1;
  guint expand : 1;
  guint shrink : 1;
  guint empty : 1;
};

/**
 * CtkAttachOptions:
 * @CTK_EXPAND: the widget should expand to take up any extra space in its
 * container that has been allocated.
 * @CTK_SHRINK: the widget should shrink as and when possible.
 * @CTK_FILL: the widget should fill the space allocated to it.
 *
 * Denotes the expansion properties that a widget will have when it (or its
 * parent) is resized.
 */
typedef enum
{
  CTK_EXPAND = 1 << 0,
  CTK_SHRINK = 1 << 1,
  CTK_FILL   = 1 << 2
} CtkAttachOptions;


GDK_DEPRECATED_IN_3_4
GType	   ctk_table_get_type	      (void) G_GNUC_CONST;
GDK_DEPRECATED_IN_3_4_FOR(CtkGrid)
CtkWidget* ctk_table_new	      (guint		rows,
				       guint		columns,
				       gboolean		homogeneous);
GDK_DEPRECATED_IN_3_4_FOR(CtkGrid)
void	   ctk_table_resize	      (CtkTable	       *table,
				       guint            rows,
				       guint            columns);
GDK_DEPRECATED_IN_3_4_FOR(CtkGrid)
void	   ctk_table_attach	      (CtkTable	       *table,
				       CtkWidget       *child,
				       guint		left_attach,
				       guint		right_attach,
				       guint		top_attach,
				       guint		bottom_attach,
				       CtkAttachOptions xoptions,
				       CtkAttachOptions yoptions,
				       guint		xpadding,
				       guint		ypadding);
GDK_DEPRECATED_IN_3_4_FOR(CtkGrid)
void	   ctk_table_attach_defaults  (CtkTable	       *table,
				       CtkWidget       *widget,
				       guint		left_attach,
				       guint		right_attach,
				       guint		top_attach,
				       guint		bottom_attach);
GDK_DEPRECATED_IN_3_4_FOR(CtkGrid)
void	   ctk_table_set_row_spacing  (CtkTable	       *table,
				       guint		row,
				       guint		spacing);
GDK_DEPRECATED_IN_3_4_FOR(CtkGrid)
guint      ctk_table_get_row_spacing  (CtkTable        *table,
				       guint            row);
GDK_DEPRECATED_IN_3_4_FOR(CtkGrid)
void	   ctk_table_set_col_spacing  (CtkTable	       *table,
				       guint		column,
				       guint		spacing);
GDK_DEPRECATED_IN_3_4_FOR(CtkGrid)
guint      ctk_table_get_col_spacing  (CtkTable        *table,
				       guint            column);
GDK_DEPRECATED_IN_3_4_FOR(CtkGrid)
void	   ctk_table_set_row_spacings (CtkTable	       *table,
				       guint		spacing);
GDK_DEPRECATED_IN_3_4_FOR(CtkGrid)
guint      ctk_table_get_default_row_spacing (CtkTable        *table);
GDK_DEPRECATED_IN_3_4_FOR(CtkGrid)
void	   ctk_table_set_col_spacings (CtkTable	       *table,
				       guint		spacing);
GDK_DEPRECATED_IN_3_4_FOR(CtkGrid)
guint      ctk_table_get_default_col_spacing (CtkTable        *table);
GDK_DEPRECATED_IN_3_4_FOR(CtkGrid)
void	   ctk_table_set_homogeneous  (CtkTable	       *table,
				       gboolean		homogeneous);
GDK_DEPRECATED_IN_3_4_FOR(CtkGrid)
gboolean   ctk_table_get_homogeneous  (CtkTable        *table);
GDK_DEPRECATED_IN_3_4_FOR(CtkGrid)
void       ctk_table_get_size         (CtkTable        *table,
                                       guint           *rows,
                                       guint           *columns);

G_END_DECLS

#endif /* __CTK_TABLE_H__ */
