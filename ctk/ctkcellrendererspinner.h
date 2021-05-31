/* GTK - The GIMP Toolkit
 *
 * Copyright (C) 2009 Matthias Clasen <mclasen@redhat.com>
 * Copyright (C) 2008 Richard Hughes <richard@hughsie.com>
 * Copyright (C) 2009 Bastien Nocera <hadess@hadess.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.         See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __CTK_CELL_RENDERER_SPINNER_H__
#define __CTK_CELL_RENDERER_SPINNER_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkcellrenderer.h>

G_BEGIN_DECLS

#define CTK_TYPE_CELL_RENDERER_SPINNER            (ctk_cell_renderer_spinner_get_type ())
#define CTK_CELL_RENDERER_SPINNER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_CELL_RENDERER_SPINNER, GtkCellRendererSpinner))
#define CTK_CELL_RENDERER_SPINNER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_CELL_RENDERER_SPINNER, GtkCellRendererSpinnerClass))
#define CTK_IS_CELL_RENDERER_SPINNER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_CELL_RENDERER_SPINNER))
#define CTK_IS_CELL_RENDERER_SPINNER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_CELL_RENDERER_SPINNER))
#define CTK_CELL_RENDERER_SPINNER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_CELL_RENDERER_SPINNER, GtkCellRendererSpinnerClass))

typedef struct _GtkCellRendererSpinner        GtkCellRendererSpinner;
typedef struct _GtkCellRendererSpinnerClass   GtkCellRendererSpinnerClass;
typedef struct _GtkCellRendererSpinnerPrivate GtkCellRendererSpinnerPrivate;

struct _GtkCellRendererSpinner
{
  GtkCellRenderer                parent;

  /*< private >*/
  GtkCellRendererSpinnerPrivate *priv;
};

struct _GtkCellRendererSpinnerClass
{
  GtkCellRendererClass parent_class;

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

GDK_AVAILABLE_IN_ALL
GType            ctk_cell_renderer_spinner_get_type (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
GtkCellRenderer *ctk_cell_renderer_spinner_new      (void);

G_END_DECLS

#endif /* __CTK_CELL_RENDERER_SPINNER_H__ */
