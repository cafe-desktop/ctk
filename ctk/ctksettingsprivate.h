/* CTK - The GIMP Toolkit
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

#ifndef __CTK_SETTINGS_PRIVATE_H__
#define __CTK_SETTINGS_PRIVATE_H__

#include <ctk/ctksettings.h>
#include "ctkstylecascadeprivate.h"

G_BEGIN_DECLS

#define DEFAULT_THEME_NAME      "Adwaita"
#define DEFAULT_ICON_THEME      "Adwaita"

void                _ctk_settings_set_property_value_from_rc (CtkSettings            *settings,
                                                              const gchar            *name,
                                                              const CtkSettingsValue *svalue);
void                _ctk_settings_reset_rc_values            (CtkSettings            *settings);

void                _ctk_settings_handle_event               (GdkEventSetting        *event);
CtkRcPropertyParser _ctk_rc_property_parser_from_type        (GType                   type);
gboolean            _ctk_settings_parse_convert              (CtkRcPropertyParser     parser,
                                                              const GValue           *src_value,
                                                              GParamSpec             *pspec,
                                                              GValue                 *dest_value);
GdkScreen          *_ctk_settings_get_screen                 (CtkSettings            *settings);
CtkStyleCascade    *_ctk_settings_get_style_cascade          (CtkSettings            *settings,
                                                              gint                    scale);

typedef enum
{
  CTK_SETTINGS_SOURCE_DEFAULT,
  CTK_SETTINGS_SOURCE_THEME,
  CTK_SETTINGS_SOURCE_XSETTING,
  CTK_SETTINGS_SOURCE_APPLICATION
} CtkSettingsSource;

CtkSettingsSource  _ctk_settings_get_setting_source (CtkSettings *settings,
                                                     const gchar *name);

gboolean ctk_settings_get_enable_animations  (CtkSettings *settings);
gint     ctk_settings_get_dnd_drag_threshold (CtkSettings *settings);
const gchar *ctk_settings_get_font_family    (CtkSettings *settings);
gint         ctk_settings_get_font_size      (CtkSettings *settings);
gboolean     ctk_settings_get_font_size_is_absolute (CtkSettings *settings);

G_END_DECLS

#endif /* __CTK_SETTINGS_PRIVATE_H__ */
