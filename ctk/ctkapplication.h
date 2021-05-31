/*
 * Copyright © 2010 Codethink Limited
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

#ifndef __CTK_APPLICATION_H__
#define __CTK_APPLICATION_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <gtk/gtk.h> can be included directly."
#endif

#include <gtk/gtkwidget.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define CTK_TYPE_APPLICATION            (ctk_application_get_type ())
#define CTK_APPLICATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_APPLICATION, GtkApplication))
#define CTK_APPLICATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_APPLICATION, GtkApplicationClass))
#define CTK_IS_APPLICATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_APPLICATION))
#define CTK_IS_APPLICATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_APPLICATION))
#define CTK_APPLICATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_APPLICATION, GtkApplicationClass))

typedef struct _GtkApplication        GtkApplication;
typedef struct _GtkApplicationClass   GtkApplicationClass;
typedef struct _GtkApplicationPrivate GtkApplicationPrivate;

struct _GtkApplication
{
  GApplication parent;

  /*< private >*/
  GtkApplicationPrivate *priv;
};

/**
 * GtkApplicationClass:
 * @parent_class: The parent class.
 * @window_added: Signal emitted when a #GtkWindow is added to
 *    application through ctk_application_add_window().
 * @window_removed: Signal emitted when a #GtkWindow is removed from
 *    application, either as a side-effect of being destroyed or
 *    explicitly through ctk_application_remove_window().
 */
struct _GtkApplicationClass
{
  GApplicationClass parent_class;

  /*< public >*/

  void (*window_added)   (GtkApplication *application,
                          GtkWindow      *window);
  void (*window_removed) (GtkApplication *application,
                          GtkWindow      *window);

  /*< private >*/
  gpointer padding[12];
};

GDK_AVAILABLE_IN_ALL
GType            ctk_application_get_type      (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
GtkApplication * ctk_application_new           (const gchar       *application_id,
                                                GApplicationFlags  flags);

GDK_AVAILABLE_IN_ALL
void             ctk_application_add_window    (GtkApplication    *application,
                                                GtkWindow         *window);

GDK_AVAILABLE_IN_ALL
void             ctk_application_remove_window (GtkApplication    *application,
                                                GtkWindow         *window);
GDK_AVAILABLE_IN_ALL
GList *          ctk_application_get_windows   (GtkApplication    *application);

GDK_AVAILABLE_IN_3_4
GMenuModel *     ctk_application_get_app_menu  (GtkApplication    *application);
GDK_AVAILABLE_IN_3_4
void             ctk_application_set_app_menu  (GtkApplication    *application,
                                                GMenuModel        *app_menu);

GDK_AVAILABLE_IN_3_4
GMenuModel *     ctk_application_get_menubar   (GtkApplication    *application);
GDK_AVAILABLE_IN_3_4
void             ctk_application_set_menubar   (GtkApplication    *application,
                                                GMenuModel        *menubar);

GDK_DEPRECATED_IN_3_14_FOR(ctk_application_set_accels_for_action)
void             ctk_application_add_accelerator    (GtkApplication  *application,
                                                     const gchar     *accelerator,
                                                     const gchar     *action_name,
                                                     GVariant        *parameter);

GDK_DEPRECATED_IN_3_14_FOR(ctk_application_set_accels_for_action)
void             ctk_application_remove_accelerator (GtkApplication *application,
                                                     const gchar    *action_name,
                                                     GVariant       *parameter);

typedef enum
{
  CTK_APPLICATION_INHIBIT_LOGOUT  = (1 << 0),
  CTK_APPLICATION_INHIBIT_SWITCH  = (1 << 1),
  CTK_APPLICATION_INHIBIT_SUSPEND = (1 << 2),
  CTK_APPLICATION_INHIBIT_IDLE    = (1 << 3)
} GtkApplicationInhibitFlags;

GDK_AVAILABLE_IN_3_4
guint            ctk_application_inhibit            (GtkApplication             *application,
                                                     GtkWindow                  *window,
                                                     GtkApplicationInhibitFlags  flags,
                                                     const gchar                *reason);
GDK_AVAILABLE_IN_3_4
void             ctk_application_uninhibit          (GtkApplication             *application,
                                                     guint                       cookie);
GDK_AVAILABLE_IN_3_4
gboolean         ctk_application_is_inhibited       (GtkApplication             *application,
                                                     GtkApplicationInhibitFlags  flags);

GDK_AVAILABLE_IN_3_6
GtkWindow *      ctk_application_get_window_by_id   (GtkApplication             *application,
                                                     guint                       id);

GDK_AVAILABLE_IN_3_6
GtkWindow *      ctk_application_get_active_window  (GtkApplication             *application);

GDK_AVAILABLE_IN_3_12
gchar **         ctk_application_list_action_descriptions        (GtkApplication       *application);

GDK_AVAILABLE_IN_3_12
gchar **         ctk_application_get_accels_for_action           (GtkApplication       *application,
                                                                  const gchar          *detailed_action_name);
GDK_AVAILABLE_IN_3_14
gchar **         ctk_application_get_actions_for_accel           (GtkApplication       *application,
                                                                  const gchar          *accel);


GDK_AVAILABLE_IN_3_12
void             ctk_application_set_accels_for_action           (GtkApplication       *application,
                                                                  const gchar          *detailed_action_name,
                                                                  const gchar * const  *accels);

GDK_AVAILABLE_IN_3_14
gboolean         ctk_application_prefers_app_menu                (GtkApplication       *application);

GDK_AVAILABLE_IN_3_14
GMenu *          ctk_application_get_menu_by_id                  (GtkApplication       *application,
                                                                  const gchar          *id);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(GtkApplication, g_object_unref)

G_END_DECLS

#endif /* __CTK_APPLICATION_H__ */
