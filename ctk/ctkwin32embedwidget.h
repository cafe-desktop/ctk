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
 * Modified by the GTK+ Team and others 1997-2006.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CTK_WIN32_EMBED_WIDGET_H__
#define __CTK_WIN32_EMBED_WIDGET_H__


#include <ctk/ctkwindow.h>
#include "win32/gdkwin32.h"


G_BEGIN_DECLS

#define CTK_TYPE_WIN32_EMBED_WIDGET            (ctk_win32_embed_widget_get_type ())
#define CTK_WIN32_EMBED_WIDGET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_WIN32_EMBED_WIDGET, GtkWin32EmbedWidget))
#define CTK_WIN32_EMBED_WIDGET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_WIN32_EMBED_WIDGET, GtkWin32EmbedWidgetClass))
#define CTK_IS_WIN32_EMBED_WIDGET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_WIN32_EMBED_WIDGET))
#define CTK_IS_WIN32_EMBED_WIDGET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_WIN32_EMBED_WIDGET))
#define CTK_WIN32_EMBED_WIDGET_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_WIN32_EMBED_WIDGET, GtkWin32EmbedWidgetClass))


typedef struct _GtkWin32EmbedWidget        GtkWin32EmbedWidget;
typedef struct _GtkWin32EmbedWidgetClass   GtkWin32EmbedWidgetClass;


struct _GtkWin32EmbedWidget
{
  GtkWindow window;

  GdkWindow *parent_window;
  gpointer old_window_procedure;
};

struct _GtkWin32EmbedWidgetClass
{
  GtkWindowClass parent_class;

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};


GType      ctk_win32_embed_widget_get_type (void) G_GNUC_CONST;
GtkWidget* _ctk_win32_embed_widget_new              (HWND parent);
BOOL       _ctk_win32_embed_widget_dialog_procedure (GtkWin32EmbedWidget *embed_widget,
						     HWND wnd, UINT message, WPARAM wparam, LPARAM lparam);


G_END_DECLS

#endif /* __CTK_WIN32_EMBED_WIDGET_H__ */
