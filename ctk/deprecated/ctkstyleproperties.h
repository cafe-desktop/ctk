/* CTK - The GIMP Toolkit
 * Copyright (C) 2010 Carlos Garnacho <carlosg@gnome.org>
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

#ifndef __CTK_STYLE_PROPERTIES_H__
#define __CTK_STYLE_PROPERTIES_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <glib-object.h>
#include <gdk/gdk.h>
#include <ctk/ctkenums.h>

G_BEGIN_DECLS

#define CTK_TYPE_STYLE_PROPERTIES         (ctk_style_properties_get_type ())
#define CTK_STYLE_PROPERTIES(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), CTK_TYPE_STYLE_PROPERTIES, CtkStyleProperties))
#define CTK_STYLE_PROPERTIES_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST    ((c), CTK_TYPE_STYLE_PROPERTIES, CtkStylePropertiesClass))
#define CTK_IS_STYLE_PROPERTIES(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CTK_TYPE_STYLE_PROPERTIES))
#define CTK_IS_STYLE_PROPERTIES_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE    ((c), CTK_TYPE_STYLE_PROPERTIES))
#define CTK_STYLE_PROPERTIES_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS  ((o), CTK_TYPE_STYLE_PROPERTIES, CtkStylePropertiesClass))

typedef struct _CtkStyleProperties CtkStyleProperties;
typedef struct _CtkStylePropertiesClass CtkStylePropertiesClass;
typedef struct _CtkStylePropertiesPrivate CtkStylePropertiesPrivate;

typedef struct _CtkSymbolicColor CtkSymbolicColor;
typedef struct _CtkGradient CtkGradient;

struct _CtkStyleProperties
{
  /*< private >*/
  GObject parent_object;
  CtkStylePropertiesPrivate *priv;
};

struct _CtkStylePropertiesClass
{
  /*< private >*/
  GObjectClass parent_class;

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

typedef gboolean (* CtkStylePropertyParser) (const gchar  *string,
                                             GValue       *value,
                                             GError      **error);

GDK_DEPRECATED_IN_3_16
GType ctk_style_properties_get_type (void) G_GNUC_CONST;

/* Next 2 are implemented in ctkcsscustomproperty.c */
GDK_DEPRECATED_IN_3_8
void     ctk_style_properties_register_property (CtkStylePropertyParser  parse_func,
                                                 GParamSpec             *pspec);
GDK_DEPRECATED_IN_3_8
gboolean ctk_style_properties_lookup_property   (const gchar             *property_name,
                                                 CtkStylePropertyParser  *parse_func,
                                                 GParamSpec             **pspec);

GDK_DEPRECATED_IN_3_16
CtkStyleProperties * ctk_style_properties_new (void);

GDK_DEPRECATED_IN_3_8
void               ctk_style_properties_map_color    (CtkStyleProperties *props,
                                                      const gchar        *name,
                                                      CtkSymbolicColor   *color);
GDK_DEPRECATED_IN_3_8
CtkSymbolicColor * ctk_style_properties_lookup_color (CtkStyleProperties *props,
                                                      const gchar        *name);

GDK_DEPRECATED_IN_3_16
void     ctk_style_properties_set_property (CtkStyleProperties *props,
                                            const gchar        *property,
                                            CtkStateFlags       state,
                                            const GValue       *value);
GDK_DEPRECATED_IN_3_16
void     ctk_style_properties_set_valist   (CtkStyleProperties *props,
                                            CtkStateFlags       state,
                                            va_list             args);
GDK_DEPRECATED_IN_3_16
void     ctk_style_properties_set          (CtkStyleProperties *props,
                                            CtkStateFlags       state,
                                            ...) G_GNUC_NULL_TERMINATED;

GDK_DEPRECATED_IN_3_16
gboolean ctk_style_properties_get_property (CtkStyleProperties *props,
                                            const gchar        *property,
                                            CtkStateFlags       state,
                                            GValue             *value);
GDK_DEPRECATED_IN_3_16
void     ctk_style_properties_get_valist   (CtkStyleProperties *props,
                                            CtkStateFlags       state,
                                            va_list             args);
GDK_DEPRECATED_IN_3_16
void     ctk_style_properties_get          (CtkStyleProperties *props,
                                            CtkStateFlags       state,
                                            ...) G_GNUC_NULL_TERMINATED;

GDK_DEPRECATED_IN_3_16
void     ctk_style_properties_unset_property (CtkStyleProperties *props,
                                              const gchar        *property,
                                              CtkStateFlags       state);

GDK_DEPRECATED_IN_3_16
void     ctk_style_properties_clear          (CtkStyleProperties  *props);

GDK_DEPRECATED_IN_3_16
void     ctk_style_properties_merge          (CtkStyleProperties       *props,
                                              const CtkStyleProperties *props_to_merge,
                                              gboolean                  replace);

G_END_DECLS

#endif /* __CTK_STYLE_PROPERTIES_H__ */
