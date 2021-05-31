/*
 * Copyright © 2011, 2013 Canonical Limited
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the licence, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Ryan Lortie <desrt@desrt.ca>
 */


#ifndef __CTK_APPLICATION_PRIVATE_H__
#define __CTK_APPLICATION_PRIVATE_H__

#include "gtkapplicationwindow.h"
#include "gtkwindowprivate.h"

#include "gtkactionmuxer.h"
#include "gtkapplicationaccelsprivate.h"

G_BEGIN_DECLS

void                    ctk_application_window_set_id                   (GtkApplicationWindow     *window,
                                                                         guint                     id);
GActionGroup *          ctk_application_window_get_action_group         (GtkApplicationWindow     *window);
void                    ctk_application_handle_window_realize           (GtkApplication           *application,
                                                                         GtkWindow                *window);
void                    ctk_application_handle_window_map               (GtkApplication           *application,
                                                                         GtkWindow                *window);
GtkActionMuxer *        ctk_application_get_parent_muxer_for_window     (GtkWindow                *window);

GtkActionMuxer *        ctk_application_get_action_muxer                (GtkApplication           *application);
void                    ctk_application_insert_action_group             (GtkApplication           *application,
                                                                         const gchar              *name,
                                                                         GActionGroup             *action_group);

GtkApplicationAccels *  ctk_application_get_application_accels          (GtkApplication           *application);

void                    ctk_application_set_screensaver_active          (GtkApplication           *application,
                                                                         gboolean                  active);

#define CTK_TYPE_APPLICATION_IMPL                           (ctk_application_impl_get_type ())
#define CTK_APPLICATION_IMPL_CLASS(class)                   (G_TYPE_CHECK_CLASS_CAST ((class),                     \
                                                             CTK_TYPE_APPLICATION_IMPL,                            \
                                                             GtkApplicationImplClass))
#define CTK_APPLICATION_IMPL_GET_CLASS(obj)                 (G_TYPE_INSTANCE_GET_CLASS ((obj),                     \
                                                             CTK_TYPE_APPLICATION_IMPL,                            \
                                                             GtkApplicationImplClass))

typedef struct
{
  GObject parent_instance;
  GtkApplication *application;
  GdkDisplay *display;
} GtkApplicationImpl;

typedef struct
{
  GObjectClass parent_class;

  void        (* startup)                   (GtkApplicationImpl          *impl,
                                             gboolean                     register_session);
  void        (* shutdown)                  (GtkApplicationImpl          *impl);

  void        (* before_emit)               (GtkApplicationImpl          *impl,
                                             GVariant                    *platform_data);

  void        (* window_added)              (GtkApplicationImpl          *impl,
                                             GtkWindow                   *window);
  void        (* window_removed)            (GtkApplicationImpl          *impl,
                                             GtkWindow                   *window);
  void        (* active_window_changed)     (GtkApplicationImpl          *impl,
                                             GtkWindow                   *window);
  void        (* handle_window_realize)     (GtkApplicationImpl          *impl,
                                             GtkWindow                   *window);
  void        (* handle_window_map)         (GtkApplicationImpl          *impl,
                                             GtkWindow                   *window);

  void        (* set_app_menu)              (GtkApplicationImpl          *impl,
                                             GMenuModel                  *app_menu);
  void        (* set_menubar)               (GtkApplicationImpl          *impl,
                                             GMenuModel                  *menubar);

  guint       (* inhibit)                   (GtkApplicationImpl          *impl,
                                             GtkWindow                   *window,
                                             GtkApplicationInhibitFlags   flags,
                                             const gchar                 *reason);
  void        (* uninhibit)                 (GtkApplicationImpl          *impl,
                                             guint                        cookie);
  gboolean    (* is_inhibited)              (GtkApplicationImpl          *impl,
                                             GtkApplicationInhibitFlags   flags);

  gboolean    (* prefers_app_menu)          (GtkApplicationImpl          *impl);


} GtkApplicationImplClass;

#define CTK_TYPE_APPLICATION_IMPL_DBUS                      (ctk_application_impl_dbus_get_type ())
#define CTK_APPLICATION_IMPL_DBUS_CLASS(class)              (G_TYPE_CHECK_CLASS_CAST ((class),                     \
                                                             CTK_TYPE_APPLICATION_IMPL_DBUS,                       \
                                                             GtkApplicationImplDBusClass))
#define CTK_APPLICATION_IMPL_DBUS_GET_CLASS(obj)            (G_TYPE_INSTANCE_GET_CLASS ((obj),                     \
                                                             CTK_TYPE_APPLICATION_IMPL_DBUS,                       \
                                                             GtkApplicationImplDBusClass))

typedef struct
{
  GtkApplicationImpl impl;

  GDBusConnection *session;

  const gchar     *application_id;
  const gchar     *unique_name;
  const gchar     *object_path;

  gchar           *app_menu_path;
  guint            app_menu_id;

  gchar           *menubar_path;
  guint            menubar_id;

  /* Session management... */
  GDBusProxy      *sm_proxy;
  GDBusProxy      *client_proxy;
  gchar           *client_path;
  GDBusProxy      *ss_proxy;

  /* Portal support */
  GDBusProxy      *inhibit_proxy;
  GSList *inhibit_handles;
  guint            state_changed_handler;
  char *           session_id;
  guint            session_state;
} GtkApplicationImplDBus;

typedef struct
{
  GtkApplicationImplClass parent_class;

  /* returns floating */
  GVariant *  (* get_window_system_id)      (GtkApplicationImplDBus      *dbus,
                                             GtkWindow                   *window);
} GtkApplicationImplDBusClass;

GType                   ctk_application_impl_get_type                   (void);
GType                   ctk_application_impl_dbus_get_type              (void);
GType                   ctk_application_impl_x11_get_type               (void);
GType                   ctk_application_impl_wayland_get_type           (void);
GType                   ctk_application_impl_quartz_get_type            (void);

GtkApplicationImpl *    ctk_application_impl_new                        (GtkApplication              *application,
                                                                         GdkDisplay                  *display);
void                    ctk_application_impl_startup                    (GtkApplicationImpl          *impl,
                                                                         gboolean                     register_sesion);
void                    ctk_application_impl_shutdown                   (GtkApplicationImpl          *impl);
void                    ctk_application_impl_before_emit                (GtkApplicationImpl          *impl,
                                                                         GVariant                    *platform_data);
void                    ctk_application_impl_window_added               (GtkApplicationImpl          *impl,
                                                                         GtkWindow                   *window);
void                    ctk_application_impl_window_removed             (GtkApplicationImpl          *impl,
                                                                         GtkWindow                   *window);
void                    ctk_application_impl_active_window_changed      (GtkApplicationImpl          *impl,
                                                                         GtkWindow                   *window);
void                    ctk_application_impl_handle_window_realize      (GtkApplicationImpl          *impl,
                                                                         GtkWindow                   *window);
void                    ctk_application_impl_handle_window_map          (GtkApplicationImpl          *impl,
                                                                         GtkWindow                   *window);
void                    ctk_application_impl_set_app_menu               (GtkApplicationImpl          *impl,
                                                                         GMenuModel                  *app_menu);
void                    ctk_application_impl_set_menubar                (GtkApplicationImpl          *impl,
                                                                         GMenuModel                  *menubar);
guint                   ctk_application_impl_inhibit                    (GtkApplicationImpl          *impl,
                                                                         GtkWindow                   *window,
                                                                         GtkApplicationInhibitFlags   flags,
                                                                         const gchar                 *reason);
void                    ctk_application_impl_uninhibit                  (GtkApplicationImpl          *impl,
                                                                         guint                        cookie);
gboolean                ctk_application_impl_is_inhibited               (GtkApplicationImpl          *impl,
                                                                         GtkApplicationInhibitFlags   flags);

gchar *                 ctk_application_impl_dbus_get_window_path       (GtkApplicationImplDBus      *dbus,
                                                                         GtkWindow                   *window);
gboolean                ctk_application_impl_prefers_app_menu           (GtkApplicationImpl          *impl);


void                    ctk_application_impl_quartz_setup_menu          (GMenuModel                  *model,
                                                                         GtkActionMuxer              *muxer);

G_END_DECLS

#endif /* __CTK_APPLICATION_PRIVATE_H__ */
