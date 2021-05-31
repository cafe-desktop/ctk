/* GTK - The GIMP Toolkit
 * Copyright (C) 2012 Benjamin Otte <otte@gnome.org>
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

#ifndef __CTK_STYLECASCADE_PRIVATE_H__
#define __CTK_STYLECASCADE_PRIVATE_H__

#include <gdk/gdk.h>
#include <gtk/gtkstyleproviderprivate.h>

G_BEGIN_DECLS

#define CTK_TYPE_STYLE_CASCADE           (_ctk_style_cascade_get_type ())
#define CTK_STYLE_CASCADE(obj)           (G_TYPE_CHECK_INSTANCE_CAST (obj, CTK_TYPE_STYLE_CASCADE, GtkStyleCascade))
#define CTK_STYLE_CASCADE_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST (cls, CTK_TYPE_STYLE_CASCADE, GtkStyleCascadeClass))
#define CTK_IS_STYLE_CASCADE(obj)        (G_TYPE_CHECK_INSTANCE_TYPE (obj, CTK_TYPE_STYLE_CASCADE))
#define CTK_IS_STYLE_CASCADE_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE (obj, CTK_TYPE_STYLE_CASCADE))
#define CTK_STYLE_CASCADE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_STYLE_CASCADE, GtkStyleCascadeClass))

typedef struct _GtkStyleCascade           GtkStyleCascade;
typedef struct _GtkStyleCascadeClass      GtkStyleCascadeClass;

struct _GtkStyleCascade
{
  GObject object;

  GtkStyleCascade *parent;
  GArray *providers;
  int scale;
};

struct _GtkStyleCascadeClass
{
  GObjectClass  parent_class;
};

GType                 _ctk_style_cascade_get_type               (void) G_GNUC_CONST;

GtkStyleCascade *     _ctk_style_cascade_new                    (void);

void                  _ctk_style_cascade_set_parent             (GtkStyleCascade     *cascade,
                                                                 GtkStyleCascade     *parent);
void                  _ctk_style_cascade_set_scale              (GtkStyleCascade     *cascade,
                                                                 int                  scale);
int                   _ctk_style_cascade_get_scale              (GtkStyleCascade     *cascade);

void                  _ctk_style_cascade_add_provider           (GtkStyleCascade     *cascade,
                                                                 GtkStyleProvider    *provider,
                                                                 guint                priority);
void                  _ctk_style_cascade_remove_provider        (GtkStyleCascade     *cascade,
                                                                 GtkStyleProvider    *provider);


G_END_DECLS

#endif /* __CTK_CSS_STYLECASCADE_PRIVATE_H__ */
