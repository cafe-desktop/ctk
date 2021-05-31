/* GTK - The GIMP Toolkit
 * Copyright (C) 2012 Red Hat, Inc.
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

#ifndef __CTK_COLOR_PLANE_H__
#define __CTK_COLOR_PLANE_H__

#include <gtk/gtkdrawingarea.h>
#include <gtk/gtktypes.h>

G_BEGIN_DECLS

#define CTK_TYPE_COLOR_PLANE            (ctk_color_plane_get_type ())
#define CTK_COLOR_PLANE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_COLOR_PLANE, GtkColorPlane))
#define CTK_COLOR_PLANE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_COLOR_PLANE, GtkColorPlaneClass))
#define CTK_IS_COLOR_PLANE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_COLOR_PLANE))
#define CTK_IS_COLOR_PLANE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_COLOR_PLANE))
#define CTK_COLOR_PLANE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_COLOR_PLANE, GtkColorPlaneClass))


typedef struct _GtkColorPlane         GtkColorPlane;
typedef struct _GtkColorPlaneClass    GtkColorPlaneClass;
typedef struct _GtkColorPlanePrivate  GtkColorPlanePrivate;

struct _GtkColorPlane
{
  GtkDrawingArea parent_instance;

  GtkColorPlanePrivate *priv;
};

struct _GtkColorPlaneClass
{
  GtkDrawingAreaClass parent_class;

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};


GType       ctk_color_plane_get_type (void) G_GNUC_CONST;
GtkWidget * ctk_color_plane_new      (GtkAdjustment *h_adj,
                                      GtkAdjustment *s_adj,
                                      GtkAdjustment *v_adj);

G_END_DECLS

#endif /* __CTK_COLOR_PLANE_H__ */
