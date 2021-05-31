/*
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
 *
 * Authors: Cody Russell <crussell@canonical.com>
 *          Alexander Larsson <alexl@redhat.com>
 */

#ifndef __CTK_OFFSCREEN_WINDOW_H__
#define __CTK_OFFSCREEN_WINDOW_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkwindow.h>

G_BEGIN_DECLS

#define CTK_TYPE_OFFSCREEN_WINDOW         (ctk_offscreen_window_get_type ())
#define CTK_OFFSCREEN_WINDOW(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), CTK_TYPE_OFFSCREEN_WINDOW, GtkOffscreenWindow))
#define CTK_OFFSCREEN_WINDOW_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), CTK_TYPE_OFFSCREEN_WINDOW, GtkOffscreenWindowClass))
#define CTK_IS_OFFSCREEN_WINDOW(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CTK_TYPE_OFFSCREEN_WINDOW))
#define CTK_IS_OFFSCREEN_WINDOW_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), CTK_TYPE_OFFSCREEN_WINDOW))
#define CTK_OFFSCREEN_WINDOW_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), CTK_TYPE_OFFSCREEN_WINDOW, GtkOffscreenWindowClass))

typedef struct _GtkOffscreenWindow      GtkOffscreenWindow;
typedef struct _GtkOffscreenWindowClass GtkOffscreenWindowClass;

struct _GtkOffscreenWindow
{
  GtkWindow parent_object;
};

/**
 * GtkOffscreenWindowClass:
 * @parent_class: The parent class.
 */
struct _GtkOffscreenWindowClass
{
  GtkWindowClass parent_class;

  /*< private >*/

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

GDK_AVAILABLE_IN_ALL
GType            ctk_offscreen_window_get_type    (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
GtkWidget       *ctk_offscreen_window_new         (void);
GDK_AVAILABLE_IN_ALL
cairo_surface_t *ctk_offscreen_window_get_surface (GtkOffscreenWindow *offscreen);
GDK_AVAILABLE_IN_ALL
GdkPixbuf       *ctk_offscreen_window_get_pixbuf  (GtkOffscreenWindow *offscreen);

G_END_DECLS

#endif /* __CTK_OFFSCREEN_WINDOW_H__ */
