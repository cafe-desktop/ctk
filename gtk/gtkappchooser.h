/*
 * gtkappchooser.h: app-chooser interface
 *
 * Copyright (C) 2010 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Cosimo Cecchi <ccecchi@redhat.com>
 */

#ifndef __CTK_APP_CHOOSER_H__
#define __CTK_APP_CHOOSER_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <gtk/gtk.h> can be included directly."
#endif

#include <glib.h>
#include <gio/gio.h>
#include <gdk/gdk.h>

G_BEGIN_DECLS

#define CTK_TYPE_APP_CHOOSER    (ctk_app_chooser_get_type ())
#define CTK_APP_CHOOSER(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_APP_CHOOSER, GtkAppChooser))
#define CTK_IS_APP_CHOOSER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_APP_CHOOSER))

typedef struct _GtkAppChooser GtkAppChooser;

GDK_AVAILABLE_IN_ALL
GType      ctk_app_chooser_get_type         (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
GAppInfo * ctk_app_chooser_get_app_info     (GtkAppChooser *self);
GDK_AVAILABLE_IN_ALL
gchar *    ctk_app_chooser_get_content_type (GtkAppChooser *self);
GDK_AVAILABLE_IN_ALL
void       ctk_app_chooser_refresh          (GtkAppChooser *self);

G_END_DECLS

#endif /* __CTK_APP_CHOOSER_H__ */

