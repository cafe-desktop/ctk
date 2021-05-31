/*
 * ctknumerableicon.h: an emblemed icon with number emblems
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
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <gio/gio.h>
#include <ctk/ctkstylecontext.h>

G_BEGIN_DECLS

#define CTK_TYPE_NUMERABLE_ICON                  (ctk_numerable_icon_get_type ())
#define CTK_NUMERABLE_ICON(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_NUMERABLE_ICON, CtkNumerableIcon))
#define CTK_NUMERABLE_ICON_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_NUMERABLE_ICON, CtkNumerableIconClass))
#define CTK_IS_NUMERABLE_ICON(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_NUMERABLE_ICON))
#define CTK_IS_NUMERABLE_ICON_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_NUMERABLE_ICON))
#define CTK_NUMERABLE_ICON_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_NUMERABLE_ICON, CtkNumerableIconClass))

typedef struct _CtkNumerableIcon        CtkNumerableIcon;
typedef struct _CtkNumerableIconClass   CtkNumerableIconClass;
typedef struct _CtkNumerableIconPrivate CtkNumerableIconPrivate;

struct _CtkNumerableIcon {
  GEmblemedIcon parent;

  /*< private >*/
  CtkNumerableIconPrivate *priv;
};

struct _CtkNumerableIconClass {
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
                                                               CtkStyleContext  *context);

GDK_DEPRECATED_IN_3_14
CtkStyleContext * ctk_numerable_icon_get_style_context        (CtkNumerableIcon *self);
GDK_DEPRECATED_IN_3_14
void              ctk_numerable_icon_set_style_context        (CtkNumerableIcon *self,
                                                               CtkStyleContext  *style);

GDK_DEPRECATED_IN_3_14
gint              ctk_numerable_icon_get_count                (CtkNumerableIcon *self);
GDK_DEPRECATED_IN_3_14
void              ctk_numerable_icon_set_count                (CtkNumerableIcon *self,
                                                               gint count);

GDK_DEPRECATED_IN_3_14
const gchar *     ctk_numerable_icon_get_label                (CtkNumerableIcon *self);
GDK_DEPRECATED_IN_3_14
void              ctk_numerable_icon_set_label                (CtkNumerableIcon *self,
                                                               const gchar      *label);

GDK_DEPRECATED_IN_3_14
void              ctk_numerable_icon_set_background_gicon     (CtkNumerableIcon *self,
                                                               GIcon            *icon);
GDK_DEPRECATED_IN_3_14
GIcon *           ctk_numerable_icon_get_background_gicon     (CtkNumerableIcon *self);

GDK_DEPRECATED_IN_3_14
void              ctk_numerable_icon_set_background_icon_name (CtkNumerableIcon *self,
                                                               const gchar      *icon_name);
GDK_DEPRECATED_IN_3_14
const gchar *     ctk_numerable_icon_get_background_icon_name (CtkNumerableIcon *self);

G_END_DECLS

#endif /* __CTK_NUMERABLE_ICON_H__ */
