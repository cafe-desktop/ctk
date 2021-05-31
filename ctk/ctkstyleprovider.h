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

#ifndef __CTK_STYLE_PROVIDER_H__
#define __CTK_STYLE_PROVIDER_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <glib-object.h>
#include <ctk/ctkenums.h>
#include <ctk/deprecated/ctkiconfactory.h>
#include <ctk/deprecated/ctkstyleproperties.h>
#include <ctk/ctktypes.h>

G_BEGIN_DECLS

#define CTK_TYPE_STYLE_PROVIDER          (ctk_style_provider_get_type ())
#define CTK_STYLE_PROVIDER(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), CTK_TYPE_STYLE_PROVIDER, CtkStyleProvider))
#define CTK_IS_STYLE_PROVIDER(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), CTK_TYPE_STYLE_PROVIDER))
#define CTK_STYLE_PROVIDER_GET_IFACE(o)  (G_TYPE_INSTANCE_GET_INTERFACE ((o), CTK_TYPE_STYLE_PROVIDER, CtkStyleProviderIface))

/**
 * CTK_STYLE_PROVIDER_PRIORITY_FALLBACK:
 *
 * The priority used for default style information
 * that is used in the absence of themes.
 *
 * Note that this is not very useful for providing default
 * styling for custom style classes - themes are likely to
 * override styling provided at this priority with
 * catch-all `* {...}` rules.
 */
#define CTK_STYLE_PROVIDER_PRIORITY_FALLBACK      1

/**
 * CTK_STYLE_PROVIDER_PRIORITY_THEME:
 *
 * The priority used for style information provided
 * by themes.
 */
#define CTK_STYLE_PROVIDER_PRIORITY_THEME     200

/**
 * CTK_STYLE_PROVIDER_PRIORITY_SETTINGS:
 *
 * The priority used for style information provided
 * via #CtkSettings.
 *
 * This priority is higher than #CTK_STYLE_PROVIDER_PRIORITY_THEME
 * to let settings override themes.
 */
#define CTK_STYLE_PROVIDER_PRIORITY_SETTINGS    400

/**
 * CTK_STYLE_PROVIDER_PRIORITY_APPLICATION:
 *
 * A priority that can be used when adding a #CtkStyleProvider
 * for application-specific style information.
 */
#define CTK_STYLE_PROVIDER_PRIORITY_APPLICATION 600

/**
 * CTK_STYLE_PROVIDER_PRIORITY_USER:
 *
 * The priority used for the style information from
 * `XDG_CONFIG_HOME/ctk-3.0/ctk.css`.
 *
 * You should not use priorities higher than this, to
 * give the user the last word.
 */
#define CTK_STYLE_PROVIDER_PRIORITY_USER        800

typedef struct _CtkStyleProviderIface CtkStyleProviderIface;
typedef struct _CtkStyleProvider CtkStyleProvider; /* dummy typedef */

/**
 * CtkStyleProviderIface:
 * @get_style: Gets a set of style information that applies to a widget path.
 * @get_style_property: Gets the value of a widget style property that applies to a widget path.
 * @get_icon_factory: Gets the icon factory that applies to a widget path.
 */
struct _CtkStyleProviderIface
{
  /*< private >*/
  GTypeInterface g_iface;

  /*< public >*/

  CtkStyleProperties * (* get_style) (CtkStyleProvider *provider,
                                      CtkWidgetPath    *path);

  gboolean (* get_style_property) (CtkStyleProvider *provider,
                                   CtkWidgetPath    *path,
                                   CtkStateFlags     state,
                                   GParamSpec       *pspec,
                                   GValue           *value);

  CtkIconFactory * (* get_icon_factory) (CtkStyleProvider *provider,
					 CtkWidgetPath    *path);
};

GDK_AVAILABLE_IN_ALL
GType ctk_style_provider_get_type (void) G_GNUC_CONST;

GDK_DEPRECATED_IN_3_8
CtkStyleProperties *ctk_style_provider_get_style (CtkStyleProvider *provider,
                                                  CtkWidgetPath    *path);

GDK_AVAILABLE_IN_ALL
gboolean ctk_style_provider_get_style_property (CtkStyleProvider *provider,
                                                CtkWidgetPath    *path,
                                                CtkStateFlags     state,
                                                GParamSpec       *pspec,
                                                GValue           *value);

GDK_DEPRECATED_IN_3_8_FOR(NULL)
CtkIconFactory * ctk_style_provider_get_icon_factory (CtkStyleProvider *provider,
						      CtkWidgetPath    *path);

G_END_DECLS

#endif /* __CTK_STYLE_PROVIDER_H__ */
