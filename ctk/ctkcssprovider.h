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

#ifndef __CTK_CSS_PROVIDER_H__
#define __CTK_CSS_PROVIDER_H__

#include <gio/gio.h>
#include <ctk/ctkcsssection.h>

G_BEGIN_DECLS

#define CTK_TYPE_CSS_PROVIDER         (ctk_css_provider_get_type ())
#define CTK_CSS_PROVIDER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), CTK_TYPE_CSS_PROVIDER, CtkCssProvider))
#define CTK_CSS_PROVIDER_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST    ((c), CTK_TYPE_CSS_PROVIDER, CtkCssProviderClass))
#define CTK_IS_CSS_PROVIDER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CTK_TYPE_CSS_PROVIDER))
#define CTK_IS_CSS_PROVIDER_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE    ((c), CTK_TYPE_CSS_PROVIDER))
#define CTK_CSS_PROVIDER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS  ((o), CTK_TYPE_CSS_PROVIDER, CtkCssProviderClass))

/**
 * CTK_CSS_PROVIDER_ERROR:
 *
 * Domain for #CtkCssProvider errors.
 */
#define CTK_CSS_PROVIDER_ERROR (ctk_css_provider_error_quark ())

/**
 * CtkCssProviderError:
 * @CTK_CSS_PROVIDER_ERROR_FAILED: Failed.
 * @CTK_CSS_PROVIDER_ERROR_SYNTAX: Syntax error.
 * @CTK_CSS_PROVIDER_ERROR_IMPORT: Import error.
 * @CTK_CSS_PROVIDER_ERROR_NAME: Name error.
 * @CTK_CSS_PROVIDER_ERROR_DEPRECATED: Deprecation error.
 * @CTK_CSS_PROVIDER_ERROR_UNKNOWN_VALUE: Unknown value.
 *
 * Error codes for %CTK_CSS_PROVIDER_ERROR.
 */
typedef enum
{
  CTK_CSS_PROVIDER_ERROR_FAILED,
  CTK_CSS_PROVIDER_ERROR_SYNTAX,
  CTK_CSS_PROVIDER_ERROR_IMPORT,
  CTK_CSS_PROVIDER_ERROR_NAME,
  CTK_CSS_PROVIDER_ERROR_DEPRECATED,
  CTK_CSS_PROVIDER_ERROR_UNKNOWN_VALUE
} CtkCssProviderError;

CDK_AVAILABLE_IN_ALL
GQuark ctk_css_provider_error_quark (void);

typedef struct _CtkCssProvider CtkCssProvider;
typedef struct _CtkCssProviderClass CtkCssProviderClass;
typedef struct _CtkCssProviderPrivate CtkCssProviderPrivate;

struct _CtkCssProvider
{
  GObject parent_instance;
  CtkCssProviderPrivate *priv;
};

struct _CtkCssProviderClass
{
  GObjectClass parent_class;

  void (* parsing_error)                        (CtkCssProvider  *provider,
                                                 CtkCssSection   *section,
                                                 const GError *   error);

  /* Padding for future expansion */
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

CDK_AVAILABLE_IN_ALL
GType ctk_css_provider_get_type (void) G_GNUC_CONST;

CDK_AVAILABLE_IN_ALL
CtkCssProvider * ctk_css_provider_new (void);

CDK_AVAILABLE_IN_3_2
char *           ctk_css_provider_to_string      (CtkCssProvider  *provider);

CDK_AVAILABLE_IN_ALL
gboolean         ctk_css_provider_load_from_data (CtkCssProvider  *css_provider,
                                                  const gchar     *data,
                                                  gssize           length,
                                                  GError         **error);
CDK_AVAILABLE_IN_ALL
gboolean         ctk_css_provider_load_from_file (CtkCssProvider  *css_provider,
                                                  GFile           *file,
                                                  GError         **error);
CDK_AVAILABLE_IN_ALL
gboolean         ctk_css_provider_load_from_path (CtkCssProvider  *css_provider,
                                                  const gchar     *path,
                                                  GError         **error);

CDK_AVAILABLE_IN_3_16
void             ctk_css_provider_load_from_resource (CtkCssProvider *css_provider,
                                                      const gchar    *resource_path);

CDK_DEPRECATED_FOR(ctk_css_provider_new)
CtkCssProvider * ctk_css_provider_get_default (void);

CDK_AVAILABLE_IN_ALL
CtkCssProvider * ctk_css_provider_get_named (const gchar *name,
                                             const gchar *variant);

G_END_DECLS

#endif /* __CTK_CSS_PROVIDER_H__ */
