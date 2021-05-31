/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* CtkPlacesSidebar - sidebar widget for places in the filesystem
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * This code comes from Nautilus, GNOME’s file manager.
 *
 * Authors : Mr Jamie McCracken (jamiemcc at blueyonder dot co dot uk)
 *           Federico Mena Quintero <federico@gnome.org>
 */

#ifndef __CTK_PLACES_SIDEBAR_H__
#define __CTK_PLACES_SIDEBAR_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkwidget.h>

G_BEGIN_DECLS

#define CTK_TYPE_PLACES_SIDEBAR			(ctk_places_sidebar_get_type ())
#define CTK_PLACES_SIDEBAR(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_PLACES_SIDEBAR, CtkPlacesSidebar))
#define CTK_PLACES_SIDEBAR_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_PLACES_SIDEBAR, CtkPlacesSidebarClass))
#define CTK_IS_PLACES_SIDEBAR(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_PLACES_SIDEBAR))
#define CTK_IS_PLACES_SIDEBAR_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_PLACES_SIDEBAR))
#define CTK_PLACES_SIDEBAR_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_PLACES_SIDEBAR, CtkPlacesSidebarClass))

typedef struct _CtkPlacesSidebar CtkPlacesSidebar;
typedef struct _CtkPlacesSidebarClass CtkPlacesSidebarClass;

/**
 * CtkPlacesOpenFlags:
 * @CTK_PLACES_OPEN_NORMAL: This is the default mode that #CtkPlacesSidebar uses if no other flags
 *  are specified.  It indicates that the calling application should open the selected location
 *  in the normal way, for example, in the folder view beside the sidebar.
 * @CTK_PLACES_OPEN_NEW_TAB: When passed to ctk_places_sidebar_set_open_flags(), this indicates
 *  that the application can open folders selected from the sidebar in new tabs.  This value
 *  will be passed to the #CtkPlacesSidebar::open-location signal when the user selects
 *  that a location be opened in a new tab instead of in the standard fashion.
 * @CTK_PLACES_OPEN_NEW_WINDOW: Similar to @CTK_PLACES_OPEN_NEW_TAB, but indicates that the application
 *  can open folders in new windows.
 *
 * These flags serve two purposes.  First, the application can call ctk_places_sidebar_set_open_flags()
 * using these flags as a bitmask.  This tells the sidebar that the application is able to open
 * folders selected from the sidebar in various ways, for example, in new tabs or in new windows in
 * addition to the normal mode.
 *
 * Second, when one of these values gets passed back to the application in the
 * #CtkPlacesSidebar::open-location signal, it means that the application should
 * open the selected location in the normal way, in a new tab, or in a new
 * window.  The sidebar takes care of determining the desired way to open the location,
 * based on the modifier keys that the user is pressing at the time the selection is made.
 *
 * If the application never calls ctk_places_sidebar_set_open_flags(), then the sidebar will only
 * use #CTK_PLACES_OPEN_NORMAL in the #CtkPlacesSidebar::open-location signal.  This is the
 * default mode of operation.
 */
typedef enum {
  CTK_PLACES_OPEN_NORMAL     = 1 << 0,
  CTK_PLACES_OPEN_NEW_TAB    = 1 << 1,
  CTK_PLACES_OPEN_NEW_WINDOW = 1 << 2
} CtkPlacesOpenFlags;

GDK_AVAILABLE_IN_3_10
GType              ctk_places_sidebar_get_type                   (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_3_10
CtkWidget *        ctk_places_sidebar_new                        (void);

GDK_AVAILABLE_IN_3_10
CtkPlacesOpenFlags ctk_places_sidebar_get_open_flags             (CtkPlacesSidebar   *sidebar);
GDK_AVAILABLE_IN_3_10
void               ctk_places_sidebar_set_open_flags             (CtkPlacesSidebar   *sidebar,
                                                                  CtkPlacesOpenFlags  flags);

GDK_AVAILABLE_IN_3_10
GFile *            ctk_places_sidebar_get_location               (CtkPlacesSidebar   *sidebar);
GDK_AVAILABLE_IN_3_10
void               ctk_places_sidebar_set_location               (CtkPlacesSidebar   *sidebar,
                                                                  GFile              *location);

GDK_AVAILABLE_IN_3_18
gboolean           ctk_places_sidebar_get_show_recent            (CtkPlacesSidebar   *sidebar);
GDK_AVAILABLE_IN_3_18
void               ctk_places_sidebar_set_show_recent            (CtkPlacesSidebar   *sidebar,
                                                                  gboolean            show_recent);

GDK_AVAILABLE_IN_3_10
gboolean           ctk_places_sidebar_get_show_desktop           (CtkPlacesSidebar   *sidebar);
GDK_AVAILABLE_IN_3_10
void               ctk_places_sidebar_set_show_desktop           (CtkPlacesSidebar   *sidebar,
                                                                  gboolean            show_desktop);

GDK_DEPRECATED_IN_3_18
gboolean           ctk_places_sidebar_get_show_connect_to_server (CtkPlacesSidebar   *sidebar);
GDK_DEPRECATED_IN_3_18
void               ctk_places_sidebar_set_show_connect_to_server (CtkPlacesSidebar   *sidebar,
                                                                  gboolean            show_connect_to_server);
GDK_AVAILABLE_IN_3_14
gboolean           ctk_places_sidebar_get_show_enter_location    (CtkPlacesSidebar   *sidebar);
GDK_AVAILABLE_IN_3_14
void               ctk_places_sidebar_set_show_enter_location    (CtkPlacesSidebar   *sidebar,
                                                                  gboolean            show_enter_location);

GDK_AVAILABLE_IN_3_12
void                 ctk_places_sidebar_set_local_only           (CtkPlacesSidebar   *sidebar,
                                                                  gboolean            local_only);
GDK_AVAILABLE_IN_3_12
gboolean             ctk_places_sidebar_get_local_only           (CtkPlacesSidebar   *sidebar);


GDK_AVAILABLE_IN_3_10
void               ctk_places_sidebar_add_shortcut               (CtkPlacesSidebar   *sidebar,
                                                                  GFile              *location);
GDK_AVAILABLE_IN_3_10
void               ctk_places_sidebar_remove_shortcut            (CtkPlacesSidebar   *sidebar,
                                                                  GFile              *location);
GDK_AVAILABLE_IN_3_10
GSList *           ctk_places_sidebar_list_shortcuts             (CtkPlacesSidebar   *sidebar);

GDK_AVAILABLE_IN_3_10
GFile *            ctk_places_sidebar_get_nth_bookmark           (CtkPlacesSidebar   *sidebar,
                                                                  gint                n);
GDK_AVAILABLE_IN_3_18
void               ctk_places_sidebar_set_drop_targets_visible   (CtkPlacesSidebar   *sidebar,
                                                                  gboolean            visible,
                                                                  GdkDragContext     *context);
GDK_AVAILABLE_IN_3_18
gboolean           ctk_places_sidebar_get_show_trash             (CtkPlacesSidebar   *sidebar);
GDK_AVAILABLE_IN_3_18
void               ctk_places_sidebar_set_show_trash             (CtkPlacesSidebar   *sidebar,
                                                                  gboolean            show_trash);

GDK_AVAILABLE_IN_3_18
void                 ctk_places_sidebar_set_show_other_locations (CtkPlacesSidebar   *sidebar,
                                                                  gboolean            show_other_locations);
GDK_AVAILABLE_IN_3_18
gboolean             ctk_places_sidebar_get_show_other_locations (CtkPlacesSidebar   *sidebar);

GDK_AVAILABLE_IN_3_22
void                 ctk_places_sidebar_set_show_starred_location (CtkPlacesSidebar   *sidebar,
                                                                   gboolean            show_starred_location);
GDK_AVAILABLE_IN_3_22
gboolean             ctk_places_sidebar_get_show_starred_location (CtkPlacesSidebar   *sidebar);
G_END_DECLS

#endif /* __CTK_PLACES_SIDEBAR_H__ */
