/* CDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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

/*
 * Modified by the CTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CDK_PROPERTY_H__
#define __CDK_PROPERTY_H__

#if !defined (__CDK_H_INSIDE__) && !defined (CDK_COMPILATION)
#error "Only <cdk/cdk.h> can be included directly."
#endif

#include <cdk/cdktypes.h>
#include <cdk/cdkversionmacros.h>

G_BEGIN_DECLS


/**
 * CdkPropMode:
 * @CDK_PROP_MODE_REPLACE: the new data replaces the existing data.
 * @CDK_PROP_MODE_PREPEND: the new data is prepended to the existing data.
 * @CDK_PROP_MODE_APPEND: the new data is appended to the existing data.
 *
 * Describes how existing data is combined with new data when
 * using cdk_property_change().
 */
typedef enum
{
  CDK_PROP_MODE_REPLACE,
  CDK_PROP_MODE_PREPEND,
  CDK_PROP_MODE_APPEND
} CdkPropMode;


CDK_AVAILABLE_IN_ALL
CdkAtom cdk_atom_intern (const gchar *atom_name,
                         gboolean     only_if_exists);
CDK_AVAILABLE_IN_ALL
CdkAtom cdk_atom_intern_static_string (const gchar *atom_name);
CDK_AVAILABLE_IN_ALL
gchar*  cdk_atom_name   (CdkAtom      atom);


CDK_AVAILABLE_IN_ALL
gboolean cdk_property_get    (CdkWindow     *window,
                              CdkAtom        property,
                              CdkAtom        type,
                              gulong         offset,
                              gulong         length,
                              gint           pdelete,
                              CdkAtom       *actual_property_type,
                              gint          *actual_format,
                              gint          *actual_length,
                              guchar       **data);
CDK_AVAILABLE_IN_ALL
void     cdk_property_change (CdkWindow     *window,
                              CdkAtom        property,
                              CdkAtom        type,
                              gint           format,
                              CdkPropMode    mode,
                              const guchar  *data,
                              gint           nelements);
CDK_AVAILABLE_IN_ALL
void     cdk_property_delete (CdkWindow     *window,
                              CdkAtom        property);

CDK_AVAILABLE_IN_ALL
gint   cdk_text_property_to_utf8_list_for_display (CdkDisplay     *display,
                                                   CdkAtom         encoding,
                                                   gint            format,
                                                   const guchar   *text,
                                                   gint            length,
                                                   gchar        ***list);

CDK_AVAILABLE_IN_ALL
gchar *cdk_utf8_to_string_target                  (const gchar    *str);

G_END_DECLS

#endif /* __CDK_PROPERTY_H__ */
