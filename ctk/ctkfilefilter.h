/* CTK - The GIMP Toolkit
 * ctkfilefilter.h: Filters for selecting a file subset
 * Copyright (C) 2003, Red Hat, Inc.
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

#ifndef __CTK_FILE_FILTER_H__
#define __CTK_FILE_FILTER_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <glib-object.h>
#include <cdk/cdk.h>

G_BEGIN_DECLS

#define CTK_TYPE_FILE_FILTER              (ctk_file_filter_get_type ())
#define CTK_FILE_FILTER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_FILE_FILTER, CtkFileFilter))
#define CTK_IS_FILE_FILTER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_FILE_FILTER))

typedef struct _CtkFileFilter     CtkFileFilter;
typedef struct _CtkFileFilterInfo CtkFileFilterInfo;

/**
 * CtkFileFilterFlags:
 * @CTK_FILE_FILTER_FILENAME: the filename of the file being tested
 * @CTK_FILE_FILTER_URI: the URI for the file being tested
 * @CTK_FILE_FILTER_DISPLAY_NAME: the string that will be used to 
 *   display the file in the file chooser
 * @CTK_FILE_FILTER_MIME_TYPE: the mime type of the file
 * 
 * These flags indicate what parts of a #CtkFileFilterInfo struct
 * are filled or need to be filled. 
 */
typedef enum {
  CTK_FILE_FILTER_FILENAME     = 1 << 0,
  CTK_FILE_FILTER_URI          = 1 << 1,
  CTK_FILE_FILTER_DISPLAY_NAME = 1 << 2,
  CTK_FILE_FILTER_MIME_TYPE    = 1 << 3
} CtkFileFilterFlags;

/**
 * CtkFileFilterFunc:
 * @filter_info: a #CtkFileFilterInfo that is filled according
 *   to the @needed flags passed to ctk_file_filter_add_custom()
 * @data: (closure): user data passed to ctk_file_filter_add_custom()
 *
 * The type of function that is used with custom filters, see
 * ctk_file_filter_add_custom().
 *
 * Returns: %TRUE if the file should be displayed
 */
typedef gboolean (*CtkFileFilterFunc) (const CtkFileFilterInfo *filter_info,
				       gpointer                 data);

/**
 * CtkFileFilterInfo:
 * @contains: Flags indicating which of the following fields need
 *   are filled
 * @filename: the filename of the file being tested
 * @uri: the URI for the file being tested
 * @display_name: the string that will be used to display the file
 *   in the file chooser
 * @mime_type: the mime type of the file
 *
 * A #CtkFileFilterInfo-struct is used to pass information about the
 * tested file to ctk_file_filter_filter().
 */
struct _CtkFileFilterInfo
{
  CtkFileFilterFlags contains;

  const gchar *filename;
  const gchar *uri;
  const gchar *display_name;
  const gchar *mime_type;
};

CDK_AVAILABLE_IN_ALL
GType ctk_file_filter_get_type (void) G_GNUC_CONST;

CDK_AVAILABLE_IN_ALL
CtkFileFilter *       ctk_file_filter_new      (void);
CDK_AVAILABLE_IN_ALL
void                  ctk_file_filter_set_name (CtkFileFilter *filter,
						const gchar   *name);
CDK_AVAILABLE_IN_ALL
const gchar *         ctk_file_filter_get_name (CtkFileFilter *filter);

CDK_AVAILABLE_IN_ALL
void ctk_file_filter_add_mime_type      (CtkFileFilter      *filter,
					 const gchar        *mime_type);
CDK_AVAILABLE_IN_ALL
void ctk_file_filter_add_pattern        (CtkFileFilter      *filter,
					 const gchar        *pattern);
CDK_AVAILABLE_IN_ALL
void ctk_file_filter_add_pixbuf_formats (CtkFileFilter      *filter);
CDK_AVAILABLE_IN_ALL
void ctk_file_filter_add_custom         (CtkFileFilter      *filter,
					 CtkFileFilterFlags  needed,
					 CtkFileFilterFunc   func,
					 gpointer            data,
					 GDestroyNotify      notify);

CDK_AVAILABLE_IN_ALL
CtkFileFilterFlags ctk_file_filter_get_needed (CtkFileFilter           *filter);
CDK_AVAILABLE_IN_ALL
gboolean           ctk_file_filter_filter     (CtkFileFilter           *filter,
					       const CtkFileFilterInfo *filter_info);

CDK_AVAILABLE_IN_3_22
GVariant      *ctk_file_filter_to_gvariant       (CtkFileFilter *filter);
CDK_AVAILABLE_IN_3_22
CtkFileFilter *ctk_file_filter_new_from_gvariant (GVariant      *variant);

G_END_DECLS

#endif /* __CTK_FILE_FILTER_H__ */
