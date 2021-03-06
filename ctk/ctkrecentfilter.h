/* CTK - The GIMP Toolkit
 * ctkrecentfilter.h - Filter object for recently used resources
 * Copyright (C) 2006, Emmanuele Bassi
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

#ifndef __CTK_RECENT_FILTER_H__
#define __CTK_RECENT_FILTER_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <glib-object.h>
#include <cdk/cdk.h>

G_BEGIN_DECLS

#define CTK_TYPE_RECENT_FILTER		(ctk_recent_filter_get_type ())
#define CTK_RECENT_FILTER(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_RECENT_FILTER, CtkRecentFilter))
#define CTK_IS_RECENT_FILTER(obj)	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_RECENT_FILTER))

typedef struct _CtkRecentFilter		CtkRecentFilter;
typedef struct _CtkRecentFilterInfo	CtkRecentFilterInfo;

/**
 * CtkRecentFilterFlags:
 * @CTK_RECENT_FILTER_URI: the URI of the file being tested
 * @CTK_RECENT_FILTER_DISPLAY_NAME: the string that will be used to
 *  display the file in the recent chooser
 * @CTK_RECENT_FILTER_MIME_TYPE: the mime type of the file
 * @CTK_RECENT_FILTER_APPLICATION: the list of applications that have
 *  registered the file
 * @CTK_RECENT_FILTER_GROUP: the groups to which the file belongs to
 * @CTK_RECENT_FILTER_AGE: the number of days elapsed since the file
 *  has been registered
 *
 * These flags indicate what parts of a #CtkRecentFilterInfo struct
 * are filled or need to be filled.
 */
typedef enum {
  CTK_RECENT_FILTER_URI          = 1 << 0,
  CTK_RECENT_FILTER_DISPLAY_NAME = 1 << 1,
  CTK_RECENT_FILTER_MIME_TYPE    = 1 << 2,
  CTK_RECENT_FILTER_APPLICATION  = 1 << 3,
  CTK_RECENT_FILTER_GROUP        = 1 << 4,
  CTK_RECENT_FILTER_AGE          = 1 << 5
} CtkRecentFilterFlags;

/**
 * CtkRecentFilterFunc:
 * @filter_info: a #CtkRecentFilterInfo that is filled according
 *  to the @needed flags passed to ctk_recent_filter_add_custom()
 * @user_data: user data passed to ctk_recent_filter_add_custom()
 *
 * The type of function that is used with custom filters,
 * see ctk_recent_filter_add_custom().
 *
 * Returns: %TRUE if the file should be displayed
 */
typedef gboolean (*CtkRecentFilterFunc) (const CtkRecentFilterInfo *filter_info,
					 gpointer                   user_data);


/**
 * CtkRecentFilterInfo:
 * @contains: #CtkRecentFilterFlags to indicate which fields are set.
 * @uri: (nullable): The URI of the file being tested.
 * @display_name: (nullable): The string that will be used to display
 *    the file in the recent chooser.
 * @mime_type: (nullable): MIME type of the file.
 * @applications: (nullable) (array zero-terminated=1): The list of
 *    applications that have registered the file.
 * @groups: (nullable) (array zero-terminated=1): The groups to which
 *    the file belongs to.
 * @age: The number of days elapsed since the file has been
 *    registered.
 *
 * A CtkRecentFilterInfo struct is used
 * to pass information about the tested file to ctk_recent_filter_filter().
 */
struct _CtkRecentFilterInfo
{
  CtkRecentFilterFlags contains;

  const gchar *uri;
  const gchar *display_name;
  const gchar *mime_type;
  const gchar **applications;
  const gchar **groups;

  gint age;
};

CDK_AVAILABLE_IN_ALL
GType                 ctk_recent_filter_get_type (void) G_GNUC_CONST;

CDK_AVAILABLE_IN_ALL
CtkRecentFilter *     ctk_recent_filter_new      (void);
CDK_AVAILABLE_IN_ALL
void                  ctk_recent_filter_set_name (CtkRecentFilter *filter,
						  const gchar     *name);
CDK_AVAILABLE_IN_ALL
const gchar *         ctk_recent_filter_get_name (CtkRecentFilter *filter);

CDK_AVAILABLE_IN_ALL
void ctk_recent_filter_add_mime_type      (CtkRecentFilter      *filter,
					   const gchar          *mime_type);
CDK_AVAILABLE_IN_ALL
void ctk_recent_filter_add_pattern        (CtkRecentFilter      *filter,
					   const gchar          *pattern);
CDK_AVAILABLE_IN_ALL
void ctk_recent_filter_add_pixbuf_formats (CtkRecentFilter      *filter);
CDK_AVAILABLE_IN_ALL
void ctk_recent_filter_add_application    (CtkRecentFilter      *filter,
					   const gchar          *application);
CDK_AVAILABLE_IN_ALL
void ctk_recent_filter_add_group          (CtkRecentFilter      *filter,
					   const gchar          *group);
CDK_AVAILABLE_IN_ALL
void ctk_recent_filter_add_age            (CtkRecentFilter      *filter,
					   gint                  days);
CDK_AVAILABLE_IN_ALL
void ctk_recent_filter_add_custom         (CtkRecentFilter      *filter,
					   CtkRecentFilterFlags  needed,
					   CtkRecentFilterFunc   func,
					   gpointer              data,
					   GDestroyNotify        data_destroy);

CDK_AVAILABLE_IN_ALL
CtkRecentFilterFlags ctk_recent_filter_get_needed (CtkRecentFilter           *filter);
CDK_AVAILABLE_IN_ALL
gboolean             ctk_recent_filter_filter     (CtkRecentFilter           *filter,
						   const CtkRecentFilterInfo *filter_info);

G_END_DECLS

#endif /* ! __CTK_RECENT_FILTER_H__ */
