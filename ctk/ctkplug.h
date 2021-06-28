/* CTK - The GIMP Toolkit
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
 * Modified by the CTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CTK_PLUG_H__
#define __CTK_PLUG_H__

#if !defined (__CTKX_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctkx.h> can be included directly."
#endif

#include <cdk/cdk.h>

#ifdef GDK_WINDOWING_X11

#include <cdk/cdkx.h>

#include <ctk/ctksocket.h>
#include <ctk/ctkwindow.h>

G_BEGIN_DECLS

#define CTK_TYPE_PLUG            (ctk_plug_get_type ())
#define CTK_PLUG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_PLUG, CtkPlug))
#define CTK_PLUG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_PLUG, CtkPlugClass))
#define CTK_IS_PLUG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_PLUG))
#define CTK_IS_PLUG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_PLUG))
#define CTK_PLUG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_PLUG, CtkPlugClass))


typedef struct _CtkPlug        CtkPlug;
typedef struct _CtkPlugPrivate CtkPlugPrivate;
typedef struct _CtkPlugClass   CtkPlugClass;


struct _CtkPlug
{
  CtkWindow window;

  CtkPlugPrivate *priv;
};

struct _CtkPlugClass
{
  CtkWindowClass parent_class;

  void (*embedded) (CtkPlug *plug);

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

GDK_AVAILABLE_IN_ALL
GType      ctk_plug_get_type              (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
void       ctk_plug_construct             (CtkPlug    *plug,
                                           Window      socket_id);
GDK_AVAILABLE_IN_ALL
CtkWidget *ctk_plug_new                   (Window      socket_id);

GDK_AVAILABLE_IN_ALL
void       ctk_plug_construct_for_display (CtkPlug    *plug,
                                           GdkDisplay *display,
                                           Window      socket_id);
GDK_AVAILABLE_IN_ALL
CtkWidget *ctk_plug_new_for_display       (GdkDisplay *display,
                                           Window      socket_id);
GDK_AVAILABLE_IN_ALL
Window     ctk_plug_get_id                (CtkPlug    *plug);
GDK_AVAILABLE_IN_ALL
gboolean   ctk_plug_get_embedded          (CtkPlug    *plug);
GDK_AVAILABLE_IN_ALL
GdkWindow *ctk_plug_get_socket_window     (CtkPlug    *plug);

G_END_DECLS

#endif /* GDK_WINDOWING_X11 */

#endif /* __CTK_PLUG_H__ */
