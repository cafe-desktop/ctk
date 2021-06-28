/* ctkgladecatalog.c
 *
 * Copyright (C) 2013 Openismus GmbH
 *
 * Authors:
 *      Tristan Van Berkom <tristanvb@openismus.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */


#include "config.h"

#include "ctkpathbar.h"
#include "ctkplacesviewprivate.h"
#include "ctkcolorswatchprivate.h"
#include "ctkcolorplaneprivate.h"
#include "ctkcolorscaleprivate.h"
#include "ctkcoloreditorprivate.h"

#ifdef G_OS_UNIX
#  include "ctkprinteroptionwidget.h"
#endif

_CDK_EXTERN
void ctk_glade_catalog_init (const gchar *catalog_name);

/* This function is referred to in ctk/glade/ctk-private-widgets.xml
 * and is used to ensure the private types for use in Glade while
 * editing UI files that define CTK+’s various composite widget classes.
 */
void
ctk_glade_catalog_init (const gchar *catalog_name)
{
  g_type_ensure (CTK_TYPE_PATH_BAR);
  g_type_ensure (CTK_TYPE_PLACES_VIEW);
  g_type_ensure (CTK_TYPE_COLOR_SWATCH);
  g_type_ensure (CTK_TYPE_COLOR_PLANE);
  g_type_ensure (CTK_TYPE_COLOR_SCALE);
  g_type_ensure (CTK_TYPE_COLOR_EDITOR);

#ifdef G_OS_UNIX
  g_type_ensure (CTK_TYPE_PRINTER_OPTION_WIDGET);
#endif
}
