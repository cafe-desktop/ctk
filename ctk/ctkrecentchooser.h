/* GTK - The GIMP Toolkit
 * ctkrecentchooser.h - Abstract interface for recent file selectors GUIs
 *
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

#ifndef __CTK_RECENT_CHOOSER_H__
#define __CTK_RECENT_CHOOSER_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkwidget.h>
#include <ctk/ctkrecentmanager.h>
#include <ctk/ctkrecentfilter.h>

G_BEGIN_DECLS

#define CTK_TYPE_RECENT_CHOOSER			(ctk_recent_chooser_get_type ())
#define CTK_RECENT_CHOOSER(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_RECENT_CHOOSER, GtkRecentChooser))
#define CTK_IS_RECENT_CHOOSER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_RECENT_CHOOSER))
#define CTK_RECENT_CHOOSER_GET_IFACE(inst)	(G_TYPE_INSTANCE_GET_INTERFACE ((inst), CTK_TYPE_RECENT_CHOOSER, GtkRecentChooserIface))

/**
 * GtkRecentSortType:
 * @CTK_RECENT_SORT_NONE: Do not sort the returned list of recently used
 *   resources.
 * @CTK_RECENT_SORT_MRU: Sort the returned list with the most recently used
 *   items first.
 * @CTK_RECENT_SORT_LRU: Sort the returned list with the least recently used
 *   items first.
 * @CTK_RECENT_SORT_CUSTOM: Sort the returned list using a custom sorting
 *   function passed using ctk_recent_chooser_set_sort_func().
 *
 * Used to specify the sorting method to be applyed to the recently
 * used resource list.
 *
 * Since: 2.10
 */
typedef enum
{
  CTK_RECENT_SORT_NONE = 0,
  CTK_RECENT_SORT_MRU,
  CTK_RECENT_SORT_LRU,
  CTK_RECENT_SORT_CUSTOM
} GtkRecentSortType;

typedef gint (*GtkRecentSortFunc) (GtkRecentInfo *a,
				   GtkRecentInfo *b,
				   gpointer       user_data);


typedef struct _GtkRecentChooser      GtkRecentChooser; /* dummy */
typedef struct _GtkRecentChooserIface GtkRecentChooserIface;

/**
 * CTK_RECENT_CHOOSER_ERROR:
 *
 * Used to get the #GError quark for #GtkRecentChooser errors.
 *
 * Since: 2.10
 */
#define CTK_RECENT_CHOOSER_ERROR	(ctk_recent_chooser_error_quark ())

/**
 * GtkRecentChooserError:
 * @CTK_RECENT_CHOOSER_ERROR_NOT_FOUND: Indicates that a file does not exist
 * @CTK_RECENT_CHOOSER_ERROR_INVALID_URI: Indicates a malformed URI
 *
 * These identify the various errors that can occur while calling
 * #GtkRecentChooser functions.
 *
 * Since: 2.10
 */
typedef enum
{
  CTK_RECENT_CHOOSER_ERROR_NOT_FOUND,
  CTK_RECENT_CHOOSER_ERROR_INVALID_URI
} GtkRecentChooserError;

GDK_AVAILABLE_IN_ALL
GQuark  ctk_recent_chooser_error_quark (void);


/**
 * GtkRecentChooserIface:
 * @set_current_uri: Sets uri as the current URI for chooser.
 * @get_current_uri: Gets the URI currently selected by chooser.
 * @select_uri: Selects uri inside chooser.
 * @unselect_uri: Unselects uri inside chooser.
 * @select_all: Selects all the items inside chooser, if the chooser
 *    supports multiple selection.
 * @unselect_all: Unselects all the items inside chooser.
 * @get_items: Gets the list of recently used resources in form of
 *    #GtkRecentInfo objects.
 * @get_recent_manager: Gets the #GtkRecentManager used by chooser.
 * @add_filter: Adds filter to the list of #GtkRecentFilter objects
 *    held by chooser.
 * @remove_filter: Removes filter from the list of #GtkRecentFilter
 *    objects held by chooser.
 * @list_filters: Gets the #GtkRecentFilter objects held by chooser.
 * @set_sort_func: Sets the comparison function used when sorting to
 *    be sort_func.
 * @item_activated: Signal emitted when the user “activates” a recent
 *    item in the recent chooser.
 * @selection_changed: Signal emitted when there is a change in the
 *    set of selected recently used resources.
 */
struct _GtkRecentChooserIface
{
  /*< private >*/
  GTypeInterface base_iface;

  /*< public >*/

  /*
   * Methods
   */
  gboolean          (* set_current_uri)    (GtkRecentChooser  *chooser,
  					    const gchar       *uri,
  					    GError           **error);
  gchar *           (* get_current_uri)    (GtkRecentChooser  *chooser);
  gboolean          (* select_uri)         (GtkRecentChooser  *chooser,
  					    const gchar       *uri,
  					    GError           **error);
  void              (* unselect_uri)       (GtkRecentChooser  *chooser,
                                            const gchar       *uri);
  void              (* select_all)         (GtkRecentChooser  *chooser);
  void              (* unselect_all)       (GtkRecentChooser  *chooser);
  GList *           (* get_items)          (GtkRecentChooser  *chooser);
  GtkRecentManager *(* get_recent_manager) (GtkRecentChooser  *chooser);
  void              (* add_filter)         (GtkRecentChooser  *chooser,
  					    GtkRecentFilter   *filter);
  void              (* remove_filter)      (GtkRecentChooser  *chooser,
  					    GtkRecentFilter   *filter);
  GSList *          (* list_filters)       (GtkRecentChooser  *chooser);
  void              (* set_sort_func)      (GtkRecentChooser  *chooser,
  					    GtkRecentSortFunc  sort_func,
  					    gpointer           sort_data,
  					    GDestroyNotify     data_destroy);

  /*
   * Signals
   */
  void		    (* item_activated)     (GtkRecentChooser  *chooser);
  void		    (* selection_changed)  (GtkRecentChooser  *chooser);
};

GDK_AVAILABLE_IN_ALL
GType   ctk_recent_chooser_get_type    (void) G_GNUC_CONST;

/*
 * Configuration
 */
GDK_AVAILABLE_IN_ALL
void              ctk_recent_chooser_set_show_private    (GtkRecentChooser  *chooser,
							  gboolean           show_private);
GDK_AVAILABLE_IN_ALL
gboolean          ctk_recent_chooser_get_show_private    (GtkRecentChooser  *chooser);
GDK_AVAILABLE_IN_ALL
void              ctk_recent_chooser_set_show_not_found  (GtkRecentChooser  *chooser,
							  gboolean           show_not_found);
GDK_AVAILABLE_IN_ALL
gboolean          ctk_recent_chooser_get_show_not_found  (GtkRecentChooser  *chooser);
GDK_AVAILABLE_IN_ALL
void              ctk_recent_chooser_set_select_multiple (GtkRecentChooser  *chooser,
							  gboolean           select_multiple);
GDK_AVAILABLE_IN_ALL
gboolean          ctk_recent_chooser_get_select_multiple (GtkRecentChooser  *chooser);
GDK_AVAILABLE_IN_ALL
void              ctk_recent_chooser_set_limit           (GtkRecentChooser  *chooser,
							  gint               limit);
GDK_AVAILABLE_IN_ALL
gint              ctk_recent_chooser_get_limit           (GtkRecentChooser  *chooser);
GDK_AVAILABLE_IN_ALL
void              ctk_recent_chooser_set_local_only      (GtkRecentChooser  *chooser,
							  gboolean           local_only);
GDK_AVAILABLE_IN_ALL
gboolean          ctk_recent_chooser_get_local_only      (GtkRecentChooser  *chooser);
GDK_AVAILABLE_IN_ALL
void              ctk_recent_chooser_set_show_tips       (GtkRecentChooser  *chooser,
							  gboolean           show_tips);
GDK_AVAILABLE_IN_ALL
gboolean          ctk_recent_chooser_get_show_tips       (GtkRecentChooser  *chooser);
GDK_AVAILABLE_IN_ALL
void              ctk_recent_chooser_set_show_icons      (GtkRecentChooser  *chooser,
							  gboolean           show_icons);
GDK_AVAILABLE_IN_ALL
gboolean          ctk_recent_chooser_get_show_icons      (GtkRecentChooser  *chooser);
GDK_AVAILABLE_IN_ALL
void              ctk_recent_chooser_set_sort_type       (GtkRecentChooser  *chooser,
							  GtkRecentSortType  sort_type);
GDK_AVAILABLE_IN_ALL
GtkRecentSortType ctk_recent_chooser_get_sort_type       (GtkRecentChooser  *chooser);
GDK_AVAILABLE_IN_ALL
void              ctk_recent_chooser_set_sort_func       (GtkRecentChooser  *chooser,
							  GtkRecentSortFunc  sort_func,
							  gpointer           sort_data,
							  GDestroyNotify     data_destroy);

/*
 * Items handling
 */
GDK_AVAILABLE_IN_ALL
gboolean       ctk_recent_chooser_set_current_uri  (GtkRecentChooser  *chooser,
						    const gchar       *uri,
						    GError           **error);
GDK_AVAILABLE_IN_ALL
gchar *        ctk_recent_chooser_get_current_uri  (GtkRecentChooser  *chooser);
GDK_AVAILABLE_IN_ALL
GtkRecentInfo *ctk_recent_chooser_get_current_item (GtkRecentChooser  *chooser);
GDK_AVAILABLE_IN_ALL
gboolean       ctk_recent_chooser_select_uri       (GtkRecentChooser  *chooser,
						    const gchar       *uri,
						    GError           **error);
GDK_AVAILABLE_IN_ALL
void           ctk_recent_chooser_unselect_uri     (GtkRecentChooser  *chooser,
					            const gchar       *uri);
GDK_AVAILABLE_IN_ALL
void           ctk_recent_chooser_select_all       (GtkRecentChooser  *chooser);
GDK_AVAILABLE_IN_ALL
void           ctk_recent_chooser_unselect_all     (GtkRecentChooser  *chooser);
GDK_AVAILABLE_IN_ALL
GList *        ctk_recent_chooser_get_items        (GtkRecentChooser  *chooser);
GDK_AVAILABLE_IN_ALL
gchar **       ctk_recent_chooser_get_uris         (GtkRecentChooser  *chooser,
						    gsize             *length);

/*
 * Filters
 */
GDK_AVAILABLE_IN_ALL
void 		 ctk_recent_chooser_add_filter    (GtkRecentChooser *chooser,
			 			   GtkRecentFilter  *filter);
GDK_AVAILABLE_IN_ALL
void 		 ctk_recent_chooser_remove_filter (GtkRecentChooser *chooser,
						   GtkRecentFilter  *filter);
GDK_AVAILABLE_IN_ALL
GSList * 	 ctk_recent_chooser_list_filters  (GtkRecentChooser *chooser);
GDK_AVAILABLE_IN_ALL
void 		 ctk_recent_chooser_set_filter    (GtkRecentChooser *chooser,
						   GtkRecentFilter  *filter);
GDK_AVAILABLE_IN_ALL
GtkRecentFilter *ctk_recent_chooser_get_filter    (GtkRecentChooser *chooser);


G_END_DECLS

#endif /* __CTK_RECENT_CHOOSER_H__ */
