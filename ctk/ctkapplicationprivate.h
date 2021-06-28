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

#include "ctkapplicationwindow.h"
#include "ctkwindowprivate.h"

#include "ctkactionmuxer.h"
#include "ctkapplicationaccelsprivate.h"

G_BEGIN_DECLS

void                    ctk_application_window_set_id                   (CtkApplicationWindow     *window,
                                                                         guint                     id);
GActionGroup *          ctk_application_window_get_action_group         (CtkApplicationWindow     *window);
void                    ctk_application_handle_window_realize           (CtkApplication           *application,
                                                                         CtkWindow                *window);
void                    ctk_application_handle_window_map               (CtkApplication           *application,
                                                                         CtkWindow                *window);
CtkActionMuxer *        ctk_application_get_parent_muxer_for_window     (CtkWindow                *window);

CtkActionMuxer *        ctk_application_get_action_muxer                (CtkApplication           *application);
void                    ctk_application_insert_action_group             (CtkApplication           *application,
                                                                         const gchar              *name,
                                                                         GActionGroup             *action_group);

CtkApplicationAccels *  ctk_application_get_application_accels          (CtkApplication           *application);

void                    ctk_application_set_screensaver_active          (CtkApplication           *application,
                                                                         gboolean                  active);

#define CTK_TYPE_APPLICATION_IMPL                           (ctk_application_impl_get_type ())
#define CTK_APPLICATION_IMPL_CLASS(class)                   (G_TYPE_CHECK_CLASS_CAST ((class),                     \
                                                             CTK_TYPE_APPLICATION_IMPL,                            \
                                                             CtkApplicationImplClass))
#define CTK_APPLICATION_IMPL_GET_CLASS(obj)                 (G_TYPE_INSTANCE_GET_CLASS ((obj),                     \
                                                             CTK_TYPE_APPLICATION_IMPL,                            \
                                                             CtkApplicationImplClass))

typedef struct
{
  GObject parent_instance;
  CtkApplication *application;
  CdkDisplay *display;
} CtkApplicationImpl;

typedef struct
{
  GObjectClass parent_class;

  void        (* startup)                   (CtkApplicationImpl          *impl,
                                             gboolean                     register_session);
  void        (* shutdown)                  (CtkApplicationImpl          *impl);

  void        (* before_emit)               (CtkApplicationImpl          *impl,
                                             GVariant                    *platform_data);

  void        (* window_added)              (CtkApplicationImpl          *impl,
                                             CtkWindow                   *window);
  void        (* window_removed)            (CtkApplicationImpl          *impl,
                                             CtkWindow                   *window);
  void        (* active_window_changed)     (CtkApplicationImpl          *impl,
                                             CtkWindow                   *window);
  void        (* handle_window_realize)     (CtkApplicationImpl          *impl,
                                             CtkWindow                   *window);
  void        (* handle_window_map)         (CtkApplicationImpl          *impl,
                                             CtkWindow                   *window);

  void        (* set_app_menu)              (CtkApplicationImpl          *impl,
                                             GMenuModel                  *app_menu);
  void        (* set_menubar)               (CtkApplicationImpl          *impl,
                                             GMenuModel                  *menubar);

  guint       (* inhibit)                   (CtkApplicationImpl          *impl,
                                             CtkWindow                   *window,
                                             CtkApplicationInhibitFlags   flags,
                                             const gchar                 *reason);
  void        (* uninhibit)                 (CtkApplicationImpl          *impl,
                                             guint                        cookie);
  gboolean    (* is_inhibited)              (CtkApplicationImpl          *impl,
                                             CtkApplicationInhibitFlags   flags);

  gboolean    (* prefers_app_menu)          (CtkApplicationImpl          *impl);


} CtkApplicationImplClass;

#define CTK_TYPE_APPLICATION_IMPL_DBUS                      (ctk_application_impl_dbus_get_type ())
#define CTK_APPLICATION_IMPL_DBUS_CLASS(class)              (G_TYPE_CHECK_CLASS_CAST ((class),                     \
                                                             CTK_TYPE_APPLICATION_IMPL_DBUS,                       \
                                                             CtkApplicationImplDBusClass))
#define CTK_APPLICATION_IMPL_DBUS_GET_CLASS(obj)            (G_TYPE_INSTANCE_GET_CLASS ((obj),                     \
                                                             CTK_TYPE_APPLICATION_IMPL_DBUS,                       \
                                                             CtkApplicationImplDBusClass))

typedef struct
{
  CtkApplicationImpl impl;

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
} CtkApplicationImplDBus;

typedef struct
{
  CtkApplicationImplClass parent_class;

  /* returns floating */
  GVariant *  (* get_window_system_id)      (CtkApplicationImplDBus      *dbus,
                                             CtkWindow                   *window);
} CtkApplicationImplDBusClass;

GType                   ctk_application_impl_get_type                   (void);
GType                   ctk_application_impl_dbus_get_type              (void);
GType                   ctk_application_impl_x11_get_type               (void);
GType                   ctk_application_impl_wayland_get_type           (void);
GType                   ctk_application_impl_quartz_get_type            (void);

CtkApplicationImpl *    ctk_application_impl_new                        (CtkApplication              *application,
                                                                         CdkDisplay                  *display);
void                    ctk_application_impl_startup                    (CtkApplicationImpl          *impl,
                                                                         gboolean                     register_sesion);
void                    ctk_application_impl_shutdown                   (CtkApplicationImpl          *impl);
void                    ctk_application_impl_before_emit                (CtkApplicationImpl          *impl,
                                                                         GVariant                    *platform_data);
void                    ctk_application_impl_window_added               (CtkApplicationImpl          *impl,
                                                                         CtkWindow                   *window);
void                    ctk_application_impl_window_removed             (CtkApplicationImpl          *impl,
                                                                         CtkWindow                   *window);
void                    ctk_application_impl_active_window_changed      (CtkApplicationImpl          *impl,
                                                                         CtkWindow                   *window);
void                    ctk_application_impl_handle_window_realize      (CtkApplicationImpl          *impl,
                                                                         CtkWindow                   *window);
void                    ctk_application_impl_handle_window_map          (CtkApplicationImpl          *impl,
                                                                         CtkWindow                   *window);
void                    ctk_application_impl_set_app_menu               (CtkApplicationImpl          *impl,
                                                                         GMenuModel                  *app_menu);
void                    ctk_application_impl_set_menubar                (CtkApplicationImpl          *impl,
                                                                         GMenuModel                  *menubar);
guint                   ctk_application_impl_inhibit                    (CtkApplicationImpl          *impl,
                                                                         CtkWindow                   *window,
                                                                         CtkApplicationInhibitFlags   flags,
                                                                         const gchar                 *reason);
void                    ctk_application_impl_uninhibit                  (CtkApplicationImpl          *impl,
                                                                         guint                        cookie);
gboolean                ctk_application_impl_is_inhibited               (CtkApplicationImpl          *impl,
                                                                         CtkApplicationInhibitFlags   flags);

gchar *                 ctk_application_impl_dbus_get_window_path       (CtkApplicationImplDBus      *dbus,
                                                                         CtkWindow                   *window);
gboolean                ctk_application_impl_prefers_app_menu           (CtkApplicationImpl          *impl);


void                    ctk_application_impl_quartz_setup_menu          (GMenuModel                  *model,
                                                                         CtkActionMuxer              *muxer);

G_END_DECLS

#endif /* __CTK_APPLICATION_PRIVATE_H__ */
