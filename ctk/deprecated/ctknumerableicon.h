/*
 * gtknumerableicon.h: an emblemed icon with number emblems
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
 * Authors: Cosimo Cecchi <cosimoc@redhat.com>
 */

#ifndef __CTK_NUMERABLE_ICON_H__
#define __CTK_NUMERABLE_ICON_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <gtk/gtk.h> can be included directly."
#endif

#include <gio/gio.h>
#include <gtk/gtkstylecontext.h>

G_BEGIN_DECLS

#define CTK_TYPE_NUMERABLE_ICON                  (ctk_numerable_icon_get_type ())
#define CTK_NUMERABLE_ICON(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_NUMERABLE_ICON, GtkNumerableIcon))
#define CTK_NUMERABLE_ICON_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_NUMERABLE_ICON, GtkNumerableIconClass))
#define CTK_IS_NUMERABLE_ICON(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_NUMERABLE_ICON))
#define CTK_IS_NUMERABLE_ICON_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_NUMERABLE_ICON))
#define CTK_NUMERABLE_ICON_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_NUMERABLE_ICON, GtkNumerableIconClass))

typedef struct _GtkNumerableIcon        GtkNumerableIcon;
typedef struct _GtkNumerableIconClass   GtkNumerableIconClass;
typedef struct _GtkNumerableIconPrivate GtkNumerableIconPrivate;

struct _GtkNumerableIcon {
  GEmblemedIcon parent;

  /*< private >*/
  GtkNumerableIconPrivate *priv;
};

struct _GtkNumerableIconClass {
  GEmblemedIconClass parent_class;

  /* padding for future class expansion */
  gpointer padding[16];
};

GDK_DEPRECATED_IN_3_14
GType             ctk_numerable_icon_get_type                 (void) G_GNUC_CONST;

GDK_DEPRECATED_IN_3_14
GIcon *           ctk_numerable_icon_new                      (GIcon            *base_icon);
GDK_DEPRECATED_IN_3_14
GIcon *           ctk_numerable_icon_new_with_style_context   (GIcon            *base_icon,
                                                               GtkStyleContext  *context);

GDK_DEPRECATED_IN_3_14
GtkStyleContext * ctk_numerable_icon_get_style_context        (GtkNumerableIcon *self);
GDK_DEPRECATED_IN_3_14
void              ctk_numerable_icon_set_style_context        (GtkNumerableIcon *self,
                                                               GtkStyleContext  *style);

GDK_DEPRECATED_IN_3_14
gint              ctk_numerable_icon_get_count                (GtkNumerableIcon *self);
GDK_DEPRECATED_IN_3_14
void              ctk_numerable_icon_set_count                (GtkNumerableIcon *self,
                                                               gint count);

GDK_DEPRECATED_IN_3_14
const gchar *     ctk_numerable_icon_get_label                (GtkNumerableIcon *self);
GDK_DEPRECATED_IN_3_14
void              ctk_numerable_icon_set_label                (GtkNumerableIcon *self,
                                                               const gchar      *label);

GDK_DEPRECATED_IN_3_14
void              ctk_numerable_icon_set_background_gicon     (GtkNumerableIcon *self,
                                                               GIcon            *icon);
GDK_DEPRECATED_IN_3_14
GIcon *           ctk_numerable_icon_get_background_gicon     (GtkNumerableIcon *self);

GDK_DEPRECATED_IN_3_14
void              ctk_numerable_icon_set_background_icon_name (GtkNumerableIcon *self,
                                                               const gchar      *icon_name);
GDK_DEPRECATED_IN_3_14
const gchar *     ctk_numerable_icon_get_background_icon_name (GtkNumerableIcon *self);

G_END_DECLS

#endif /* __CTK_NUMERABLE_ICON_H__ */
