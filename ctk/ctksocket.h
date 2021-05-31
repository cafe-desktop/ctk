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

#ifndef __CTK_SOCKET_H__
#define __CTK_SOCKET_H__

#if !defined (__GTKX_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <gtk/gtkx.h> can be included directly."
#endif

#ifdef GDK_WINDOWING_X11

#include <gdk/gdkx.h>

#include <gtk/gtkcontainer.h>

G_BEGIN_DECLS

#define CTK_TYPE_SOCKET            (ctk_socket_get_type ())
#define CTK_SOCKET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_SOCKET, GtkSocket))
#define CTK_SOCKET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_SOCKET, GtkSocketClass))
#define CTK_IS_SOCKET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_SOCKET))
#define CTK_IS_SOCKET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_SOCKET))
#define CTK_SOCKET_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_SOCKET, GtkSocketClass))


typedef struct _GtkSocket        GtkSocket;
typedef struct _GtkSocketClass   GtkSocketClass;
typedef struct _GtkSocketPrivate GtkSocketPrivate;

struct _GtkSocket
{
  GtkContainer container;

  GtkSocketPrivate *priv;
};

struct _GtkSocketClass
{
  GtkContainerClass parent_class;

  void     (*plug_added)   (GtkSocket *socket_);
  gboolean (*plug_removed) (GtkSocket *socket_);

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

GDK_AVAILABLE_IN_ALL
GType      ctk_socket_get_type        (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
GtkWidget *ctk_socket_new             (void);
GDK_AVAILABLE_IN_ALL
void       ctk_socket_add_id          (GtkSocket *socket_,
                                       Window     window);
GDK_AVAILABLE_IN_ALL
Window     ctk_socket_get_id          (GtkSocket *socket_);
GDK_AVAILABLE_IN_ALL
GdkWindow *ctk_socket_get_plug_window (GtkSocket *socket_);

G_END_DECLS

#endif /* GDK_WINDOWING_X11 */

#endif /* __CTK_SOCKET_H__ */