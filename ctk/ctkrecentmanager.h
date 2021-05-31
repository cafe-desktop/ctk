/* GTK - The GIMP Toolkit
 * ctkrecentmanager.h: a manager for the recently used resources
 *
 * Copyright (C) 2006 Emmanuele Bassi
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

#ifndef __CTK_RECENT_MANAGER_H__
#define __CTK_RECENT_MANAGER_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdk.h>
#include <time.h>

G_BEGIN_DECLS

#define CTK_TYPE_RECENT_INFO			(ctk_recent_info_get_type ())

#define CTK_TYPE_RECENT_MANAGER			(ctk_recent_manager_get_type ())
#define CTK_RECENT_MANAGER(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_RECENT_MANAGER, CtkRecentManager))
#define CTK_IS_RECENT_MANAGER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_RECENT_MANAGER))
#define CTK_RECENT_MANAGER_CLASS(klass) 	(G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_RECENT_MANAGER, CtkRecentManagerClass))
#define CTK_IS_RECENT_MANAGER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_RECENT_MANAGER))
#define CTK_RECENT_MANAGER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_RECENT_MANAGER, CtkRecentManagerClass))

typedef struct _CtkRecentInfo		CtkRecentInfo;
typedef struct _CtkRecentData		CtkRecentData;
typedef struct _CtkRecentManager	CtkRecentManager;
typedef struct _CtkRecentManagerClass	CtkRecentManagerClass;
typedef struct _CtkRecentManagerPrivate CtkRecentManagerPrivate;

/**
 * CtkRecentData:
 * @display_name: a UTF-8 encoded string, containing the name of the recently
 *   used resource to be displayed, or %NULL;
 * @description: a UTF-8 encoded string, containing a short description of
 *   the resource, or %NULL;
 * @mime_type: the MIME type of the resource;
 * @app_name: the name of the application that is registering this recently
 *   used resource;
 * @app_exec: command line used to launch this resource; may contain the
 *   “\%f” and “\%u” escape characters which will be expanded
 *   to the resource file path and URI respectively when the command line
 *   is retrieved;
 * @groups: (array zero-terminated=1): a vector of strings containing
 *   groups names;
 * @is_private: whether this resource should be displayed only by the
 *   applications that have registered it or not.
 *
 * Meta-data to be passed to ctk_recent_manager_add_full() when
 * registering a recently used resource.
 **/
struct _CtkRecentData
{
  gchar *display_name;
  gchar *description;

  gchar *mime_type;

  gchar *app_name;
  gchar *app_exec;

  gchar **groups;

  gboolean is_private;
};

/**
 * CtkRecentManager:
 *
 * #CtkRecentManager-struct contains only private data
 * and should be accessed using the provided API.
 *
 * Since: 2.10
 */
struct _CtkRecentManager
{
  /*< private >*/
  GObject parent_instance;

  CtkRecentManagerPrivate *priv;
};

/**
 * CtkRecentManagerClass:
 *
 * #CtkRecentManagerClass contains only private data.
 *
 * Since: 2.10
 */
struct _CtkRecentManagerClass
{
  /*< private >*/
  GObjectClass parent_class;

  void (*changed) (CtkRecentManager *manager);

  /* padding for future expansion */
  void (*_ctk_recent1) (void);
  void (*_ctk_recent2) (void);
  void (*_ctk_recent3) (void);
  void (*_ctk_recent4) (void);
};

/**
 * CtkRecentManagerError:
 * @CTK_RECENT_MANAGER_ERROR_NOT_FOUND: the URI specified does not exists in
 *   the recently used resources list.
 * @CTK_RECENT_MANAGER_ERROR_INVALID_URI: the URI specified is not valid.
 * @CTK_RECENT_MANAGER_ERROR_INVALID_ENCODING: the supplied string is not
 *   UTF-8 encoded.
 * @CTK_RECENT_MANAGER_ERROR_NOT_REGISTERED: no application has registered
 *   the specified item.
 * @CTK_RECENT_MANAGER_ERROR_READ: failure while reading the recently used
 *   resources file.
 * @CTK_RECENT_MANAGER_ERROR_WRITE: failure while writing the recently used
 *   resources file.
 * @CTK_RECENT_MANAGER_ERROR_UNKNOWN: unspecified error.
 *
 * Error codes for #CtkRecentManager operations
 *
 * Since: 2.10
 */
typedef enum
{
  CTK_RECENT_MANAGER_ERROR_NOT_FOUND,
  CTK_RECENT_MANAGER_ERROR_INVALID_URI,
  CTK_RECENT_MANAGER_ERROR_INVALID_ENCODING,
  CTK_RECENT_MANAGER_ERROR_NOT_REGISTERED,
  CTK_RECENT_MANAGER_ERROR_READ,
  CTK_RECENT_MANAGER_ERROR_WRITE,
  CTK_RECENT_MANAGER_ERROR_UNKNOWN
} CtkRecentManagerError;

/**
 * CTK_RECENT_MANAGER_ERROR:
 *
 * The #GError domain for #CtkRecentManager errors.
 *
 * Since: 2.10
 */
#define CTK_RECENT_MANAGER_ERROR	(ctk_recent_manager_error_quark ())
GDK_AVAILABLE_IN_ALL
GQuark 	ctk_recent_manager_error_quark (void);


GDK_AVAILABLE_IN_ALL
GType 		  ctk_recent_manager_get_type       (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
CtkRecentManager *ctk_recent_manager_new            (void);
GDK_AVAILABLE_IN_ALL
CtkRecentManager *ctk_recent_manager_get_default    (void);

GDK_AVAILABLE_IN_ALL
gboolean          ctk_recent_manager_add_item       (CtkRecentManager     *manager,
						     const gchar          *uri);
GDK_AVAILABLE_IN_ALL
gboolean          ctk_recent_manager_add_full       (CtkRecentManager     *manager,
						     const gchar          *uri,
						     const CtkRecentData  *recent_data);
GDK_AVAILABLE_IN_ALL
gboolean          ctk_recent_manager_remove_item    (CtkRecentManager     *manager,
						     const gchar          *uri,
						     GError              **error);
GDK_AVAILABLE_IN_ALL
CtkRecentInfo *   ctk_recent_manager_lookup_item    (CtkRecentManager     *manager,
						     const gchar          *uri,
						     GError              **error);
GDK_AVAILABLE_IN_ALL
gboolean          ctk_recent_manager_has_item       (CtkRecentManager     *manager,
						     const gchar          *uri);
GDK_AVAILABLE_IN_ALL
gboolean          ctk_recent_manager_move_item      (CtkRecentManager     *manager,
						     const gchar          *uri,
						     const gchar          *new_uri,
						     GError              **error);
GDK_AVAILABLE_IN_ALL
GList *           ctk_recent_manager_get_items      (CtkRecentManager     *manager);
GDK_AVAILABLE_IN_ALL
gint              ctk_recent_manager_purge_items    (CtkRecentManager     *manager,
						     GError              **error);


GDK_AVAILABLE_IN_ALL
GType	              ctk_recent_info_get_type             (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
CtkRecentInfo *       ctk_recent_info_ref                  (CtkRecentInfo  *info);
GDK_AVAILABLE_IN_ALL
void                  ctk_recent_info_unref                (CtkRecentInfo  *info);

GDK_AVAILABLE_IN_ALL
const gchar *         ctk_recent_info_get_uri              (CtkRecentInfo  *info);
GDK_AVAILABLE_IN_ALL
const gchar *         ctk_recent_info_get_display_name     (CtkRecentInfo  *info);
GDK_AVAILABLE_IN_ALL
const gchar *         ctk_recent_info_get_description      (CtkRecentInfo  *info);
GDK_AVAILABLE_IN_ALL
const gchar *         ctk_recent_info_get_mime_type        (CtkRecentInfo  *info);
GDK_AVAILABLE_IN_ALL
time_t                ctk_recent_info_get_added            (CtkRecentInfo  *info);
GDK_AVAILABLE_IN_ALL
time_t                ctk_recent_info_get_modified         (CtkRecentInfo  *info);
GDK_AVAILABLE_IN_ALL
time_t                ctk_recent_info_get_visited          (CtkRecentInfo  *info);
GDK_AVAILABLE_IN_ALL
gboolean              ctk_recent_info_get_private_hint     (CtkRecentInfo  *info);
GDK_AVAILABLE_IN_ALL
gboolean              ctk_recent_info_get_application_info (CtkRecentInfo  *info,
							    const gchar    *app_name,
							    const gchar   **app_exec,
							    guint          *count,
							    time_t         *time_);
GDK_AVAILABLE_IN_ALL
GAppInfo *            ctk_recent_info_create_app_info      (CtkRecentInfo  *info,
                                                            const gchar    *app_name,
                                                            GError        **error);
GDK_AVAILABLE_IN_ALL
gchar **              ctk_recent_info_get_applications     (CtkRecentInfo  *info,
							    gsize          *length) G_GNUC_MALLOC;
GDK_AVAILABLE_IN_ALL
gchar *               ctk_recent_info_last_application     (CtkRecentInfo  *info) G_GNUC_MALLOC;
GDK_AVAILABLE_IN_ALL
gboolean              ctk_recent_info_has_application      (CtkRecentInfo  *info,
							    const gchar    *app_name);
GDK_AVAILABLE_IN_ALL
gchar **              ctk_recent_info_get_groups           (CtkRecentInfo  *info,
							    gsize          *length) G_GNUC_MALLOC;
GDK_AVAILABLE_IN_ALL
gboolean              ctk_recent_info_has_group            (CtkRecentInfo  *info,
							    const gchar    *group_name);
GDK_AVAILABLE_IN_ALL
GdkPixbuf *           ctk_recent_info_get_icon             (CtkRecentInfo  *info,
							    gint            size);
GDK_AVAILABLE_IN_ALL
GIcon *               ctk_recent_info_get_gicon            (CtkRecentInfo  *info);
GDK_AVAILABLE_IN_ALL
gchar *               ctk_recent_info_get_short_name       (CtkRecentInfo  *info) G_GNUC_MALLOC;
GDK_AVAILABLE_IN_ALL
gchar *               ctk_recent_info_get_uri_display      (CtkRecentInfo  *info) G_GNUC_MALLOC;
GDK_AVAILABLE_IN_ALL
gint                  ctk_recent_info_get_age              (CtkRecentInfo  *info);
GDK_AVAILABLE_IN_ALL
gboolean              ctk_recent_info_is_local             (CtkRecentInfo  *info);
GDK_AVAILABLE_IN_ALL
gboolean              ctk_recent_info_exists               (CtkRecentInfo  *info);
GDK_AVAILABLE_IN_ALL
gboolean              ctk_recent_info_match                (CtkRecentInfo  *info_a,
							    CtkRecentInfo  *info_b);

/* private */
void _ctk_recent_manager_sync (void);

G_END_DECLS

#endif /* __CTK_RECENT_MANAGER_H__ */
