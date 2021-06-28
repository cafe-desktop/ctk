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
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkwidget.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define CTK_TYPE_APPLICATION            (ctk_application_get_type ())
#define CTK_APPLICATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_APPLICATION, CtkApplication))
#define CTK_APPLICATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_APPLICATION, CtkApplicationClass))
#define CTK_IS_APPLICATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_APPLICATION))
#define CTK_IS_APPLICATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_APPLICATION))
#define CTK_APPLICATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_APPLICATION, CtkApplicationClass))

typedef struct _CtkApplication        CtkApplication;
typedef struct _CtkApplicationClass   CtkApplicationClass;
typedef struct _CtkApplicationPrivate CtkApplicationPrivate;

struct _CtkApplication
{
  GApplication parent;

  /*< private >*/
  CtkApplicationPrivate *priv;
};

/**
 * CtkApplicationClass:
 * @parent_class: The parent class.
 * @window_added: Signal emitted when a #CtkWindow is added to
 *    application through ctk_application_add_window().
 * @window_removed: Signal emitted when a #CtkWindow is removed from
 *    application, either as a side-effect of being destroyed or
 *    explicitly through ctk_application_remove_window().
 */
struct _CtkApplicationClass
{
  GApplicationClass parent_class;

  /*< public >*/

  void (*window_added)   (CtkApplication *application,
                          CtkWindow      *window);
  void (*window_removed) (CtkApplication *application,
                          CtkWindow      *window);

  /*< private >*/
  gpointer padding[12];
};

CDK_AVAILABLE_IN_ALL
GType            ctk_application_get_type      (void) G_GNUC_CONST;

CDK_AVAILABLE_IN_ALL
CtkApplication * ctk_application_new           (const gchar       *application_id,
                                                GApplicationFlags  flags);

CDK_AVAILABLE_IN_ALL
void             ctk_application_add_window    (CtkApplication    *application,
                                                CtkWindow         *window);

CDK_AVAILABLE_IN_ALL
void             ctk_application_remove_window (CtkApplication    *application,
                                                CtkWindow         *window);
CDK_AVAILABLE_IN_ALL
GList *          ctk_application_get_windows   (CtkApplication    *application);

CDK_AVAILABLE_IN_3_4
GMenuModel *     ctk_application_get_app_menu  (CtkApplication    *application);
CDK_AVAILABLE_IN_3_4
void             ctk_application_set_app_menu  (CtkApplication    *application,
                                                GMenuModel        *app_menu);

CDK_AVAILABLE_IN_3_4
GMenuModel *     ctk_application_get_menubar   (CtkApplication    *application);
CDK_AVAILABLE_IN_3_4
void             ctk_application_set_menubar   (CtkApplication    *application,
                                                GMenuModel        *menubar);

CDK_DEPRECATED_IN_3_14_FOR(ctk_application_set_accels_for_action)
void             ctk_application_add_accelerator    (CtkApplication  *application,
                                                     const gchar     *accelerator,
                                                     const gchar     *action_name,
                                                     GVariant        *parameter);

CDK_DEPRECATED_IN_3_14_FOR(ctk_application_set_accels_for_action)
void             ctk_application_remove_accelerator (CtkApplication *application,
                                                     const gchar    *action_name,
                                                     GVariant       *parameter);

typedef enum
{
  CTK_APPLICATION_INHIBIT_LOGOUT  = (1 << 0),
  CTK_APPLICATION_INHIBIT_SWITCH  = (1 << 1),
  CTK_APPLICATION_INHIBIT_SUSPEND = (1 << 2),
  CTK_APPLICATION_INHIBIT_IDLE    = (1 << 3)
} CtkApplicationInhibitFlags;

CDK_AVAILABLE_IN_3_4
guint            ctk_application_inhibit            (CtkApplication             *application,
                                                     CtkWindow                  *window,
                                                     CtkApplicationInhibitFlags  flags,
                                                     const gchar                *reason);
CDK_AVAILABLE_IN_3_4
void             ctk_application_uninhibit          (CtkApplication             *application,
                                                     guint                       cookie);
CDK_AVAILABLE_IN_3_4
gboolean         ctk_application_is_inhibited       (CtkApplication             *application,
                                                     CtkApplicationInhibitFlags  flags);

CDK_AVAILABLE_IN_3_6
CtkWindow *      ctk_application_get_window_by_id   (CtkApplication             *application,
                                                     guint                       id);

CDK_AVAILABLE_IN_3_6
CtkWindow *      ctk_application_get_active_window  (CtkApplication             *application);

CDK_AVAILABLE_IN_3_12
gchar **         ctk_application_list_action_descriptions        (CtkApplication       *application);

CDK_AVAILABLE_IN_3_12
gchar **         ctk_application_get_accels_for_action           (CtkApplication       *application,
                                                                  const gchar          *detailed_action_name);
CDK_AVAILABLE_IN_3_14
gchar **         ctk_application_get_actions_for_accel           (CtkApplication       *application,
                                                                  const gchar          *accel);


CDK_AVAILABLE_IN_3_12
void             ctk_application_set_accels_for_action           (CtkApplication       *application,
                                                                  const gchar          *detailed_action_name,
                                                                  const gchar * const  *accels);

CDK_AVAILABLE_IN_3_14
gboolean         ctk_application_prefers_app_menu                (CtkApplication       *application);

CDK_AVAILABLE_IN_3_14
GMenu *          ctk_application_get_menu_by_id                  (CtkApplication       *application,
                                                                  const gchar          *id);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkApplication, g_object_unref)

G_END_DECLS

#endif /* __CTK_APPLICATION_H__ */
