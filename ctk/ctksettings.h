/* GTK - The GIMP Toolkit
 * Copyright (C) 2000 Red Hat, Inc.
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

#ifndef __CTK_SETTINGS_H__
#define __CTK_SETTINGS_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <gdk/gdk.h>
#include <ctk/ctktypes.h>

G_BEGIN_DECLS


/* -- type macros --- */
#define CTK_TYPE_SETTINGS             (ctk_settings_get_type ())
#define CTK_SETTINGS(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_SETTINGS, CtkSettings))
#define CTK_SETTINGS_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_SETTINGS, CtkSettingsClass))
#define CTK_IS_SETTINGS(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_SETTINGS))
#define CTK_IS_SETTINGS_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_SETTINGS))
#define CTK_SETTINGS_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_SETTINGS, CtkSettingsClass))


/* --- typedefs --- */
typedef struct _CtkSettingsPrivate CtkSettingsPrivate;
typedef struct _CtkSettingsClass CtkSettingsClass;
typedef struct _CtkSettingsValue CtkSettingsValue;


/* --- structures --- */
struct _CtkSettings
{
  GObject parent_instance;

  /*< private >*/
  CtkSettingsPrivate *priv;
};

struct _CtkSettingsClass
{
  GObjectClass parent_class;

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

/**
 * CtkSettingsValue:
 * @origin: Origin should be something like “filename:linenumber” for
 *    rc files, or e.g. “XProperty” for other sources.
 * @value: Valid types are LONG, DOUBLE and STRING corresponding to
 *    the token parsed, or a GSTRING holding an unparsed statement
 */
struct _CtkSettingsValue
{
  /* origin should be something like "filename:linenumber" for rc files,
   * or e.g. "XProperty" for other sources
   */
  gchar *origin;

  /* valid types are LONG, DOUBLE and STRING corresponding to the token parsed,
   * or a GSTRING holding an unparsed statement
   */
  GValue value;
};


/* --- functions --- */
GDK_AVAILABLE_IN_ALL
GType           ctk_settings_get_type                (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
CtkSettings*    ctk_settings_get_default             (void);
GDK_AVAILABLE_IN_ALL
CtkSettings*    ctk_settings_get_for_screen          (GdkScreen *screen);

GDK_DEPRECATED_IN_3_16
void            ctk_settings_install_property        (GParamSpec         *pspec);
GDK_DEPRECATED_IN_3_16
void            ctk_settings_install_property_parser (GParamSpec         *pspec,
                                                      CtkRcPropertyParser parser);

/* --- precoded parsing functions --- */
GDK_AVAILABLE_IN_ALL
gboolean ctk_rc_property_parse_color       (const GParamSpec *pspec,
                                            const GString    *gstring,
                                            GValue           *property_value);
GDK_AVAILABLE_IN_ALL
gboolean ctk_rc_property_parse_enum        (const GParamSpec *pspec,
                                            const GString    *gstring,
                                            GValue           *property_value);
GDK_AVAILABLE_IN_ALL
gboolean ctk_rc_property_parse_flags       (const GParamSpec *pspec,
                                            const GString    *gstring,
                                            GValue           *property_value);
GDK_AVAILABLE_IN_ALL
gboolean ctk_rc_property_parse_requisition (const GParamSpec *pspec,
                                            const GString    *gstring,
                                            GValue           *property_value);
GDK_AVAILABLE_IN_ALL
gboolean ctk_rc_property_parse_border      (const GParamSpec *pspec,
                                            const GString    *gstring,
                                            GValue           *property_value);

GDK_DEPRECATED_IN_3_16
void     ctk_settings_set_property_value   (CtkSettings            *settings,
                                            const gchar            *name,
                                            const CtkSettingsValue *svalue);
GDK_DEPRECATED_IN_3_16
void     ctk_settings_set_string_property  (CtkSettings            *settings,
                                            const gchar            *name,
                                            const gchar            *v_string,
                                            const gchar            *origin);
GDK_DEPRECATED_IN_3_16
void     ctk_settings_set_long_property    (CtkSettings            *settings,
                                            const gchar            *name,
                                            glong                   v_long,
                                            const gchar            *origin);
GDK_DEPRECATED_IN_3_16
void     ctk_settings_set_double_property  (CtkSettings            *settings,
                                            const gchar            *name,
                                            gdouble                 v_double,
                                            const gchar            *origin);

GDK_AVAILABLE_IN_3_20
void     ctk_settings_reset_property       (CtkSettings            *settings,
                                            const gchar            *name);

G_END_DECLS

#endif /* __CTK_SETTINGS_H__ */
