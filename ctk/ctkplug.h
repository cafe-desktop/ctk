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

#ifndef __CTK_PLUG_H__
#define __CTK_PLUG_H__

#if !defined (__GTKX_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <gtk/gtkx.h> can be included directly."
#endif

#include <gdk/gdk.h>

#ifdef GDK_WINDOWING_X11

#include <gdk/gdkx.h>

#include <gtk/gtksocket.h>
#include <gtk/gtkwindow.h>

G_BEGIN_DECLS

#define CTK_TYPE_PLUG            (ctk_plug_get_type ())
#define CTK_PLUG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_PLUG, GtkPlug))
#define CTK_PLUG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_PLUG, GtkPlugClass))
#define CTK_IS_PLUG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_PLUG))
#define CTK_IS_PLUG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_PLUG))
#define CTK_PLUG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_PLUG, GtkPlugClass))


typedef struct _GtkPlug        GtkPlug;
typedef struct _GtkPlugPrivate GtkPlugPrivate;
typedef struct _GtkPlugClass   GtkPlugClass;


struct _GtkPlug
{
  GtkWindow window;

  GtkPlugPrivate *priv;
};

struct _GtkPlugClass
{
  GtkWindowClass parent_class;

  void (*embedded) (GtkPlug *plug);

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

GDK_AVAILABLE_IN_ALL
GType      ctk_plug_get_type              (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
void       ctk_plug_construct             (GtkPlug    *plug,
                                           Window      socket_id);
GDK_AVAILABLE_IN_ALL
GtkWidget *ctk_plug_new                   (Window      socket_id);

GDK_AVAILABLE_IN_ALL
void       ctk_plug_construct_for_display (GtkPlug    *plug,
                                           GdkDisplay *display,
                                           Window      socket_id);
GDK_AVAILABLE_IN_ALL
GtkWidget *ctk_plug_new_for_display       (GdkDisplay *display,
                                           Window      socket_id);
GDK_AVAILABLE_IN_ALL
Window     ctk_plug_get_id                (GtkPlug    *plug);
GDK_AVAILABLE_IN_ALL
gboolean   ctk_plug_get_embedded          (GtkPlug    *plug);
GDK_AVAILABLE_IN_ALL
GdkWindow *ctk_plug_get_socket_window     (GtkPlug    *plug);

G_END_DECLS

#endif /* GDK_WINDOWING_X11 */

#endif /* __CTK_PLUG_H__ */
