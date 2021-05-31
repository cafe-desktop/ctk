/* ctkcellrenderertext.h
 * Copyright (C) 2000  Red Hat, Inc.,  Jonathan Blandford <jrb@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __CTK_CELL_RENDERER_TEXT_H__
#define __CTK_CELL_RENDERER_TEXT_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkcellrenderer.h>


G_BEGIN_DECLS


#define CTK_TYPE_CELL_RENDERER_TEXT		(ctk_cell_renderer_text_get_type ())
#define CTK_CELL_RENDERER_TEXT(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_CELL_RENDERER_TEXT, CtkCellRendererText))
#define CTK_CELL_RENDERER_TEXT_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_CELL_RENDERER_TEXT, CtkCellRendererTextClass))
#define CTK_IS_CELL_RENDERER_TEXT(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_CELL_RENDERER_TEXT))
#define CTK_IS_CELL_RENDERER_TEXT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_CELL_RENDERER_TEXT))
#define CTK_CELL_RENDERER_TEXT_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_CELL_RENDERER_TEXT, CtkCellRendererTextClass))

typedef struct _CtkCellRendererText              CtkCellRendererText;
typedef struct _CtkCellRendererTextPrivate       CtkCellRendererTextPrivate;
typedef struct _CtkCellRendererTextClass         CtkCellRendererTextClass;

struct _CtkCellRendererText
{
  CtkCellRenderer parent;

  /*< private >*/
  CtkCellRendererTextPrivate *priv;
};

struct _CtkCellRendererTextClass
{
  CtkCellRendererClass parent_class;

  void (* edited) (CtkCellRendererText *cell_renderer_text,
		   const gchar         *path,
		   const gchar         *new_text);

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

GDK_AVAILABLE_IN_ALL
GType            ctk_cell_renderer_text_get_type (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
CtkCellRenderer *ctk_cell_renderer_text_new      (void);

GDK_AVAILABLE_IN_ALL
void             ctk_cell_renderer_text_set_fixed_height_from_font (CtkCellRendererText *renderer,
								    gint                 number_of_rows);


G_END_DECLS

#endif /* __CTK_CELL_RENDERER_TEXT_H__ */
