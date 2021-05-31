/*
 * GTK - The GIMP Toolkit
 * Copyright (C) 1998, 1999 Red Hat, Inc.
 * All rights reserved.
 *
 * This Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Author: James Henstridge <james@daa.com.au>
 *
 * Modified by the GTK+ Team and others 2003.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CTK_UI_MANAGER_H__
#define __CTK_UI_MANAGER_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkaccelgroup.h>
#include <ctk/ctkwidget.h>
#include <ctk/deprecated/ctkaction.h>
#include <ctk/deprecated/ctkactiongroup.h>

G_BEGIN_DECLS

#define CTK_TYPE_UI_MANAGER            (ctk_ui_manager_get_type ())
#define CTK_UI_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_UI_MANAGER, CtkUIManager))
#define CTK_UI_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_UI_MANAGER, CtkUIManagerClass))
#define CTK_IS_UI_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_UI_MANAGER))
#define CTK_IS_UI_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_UI_MANAGER))
#define CTK_UI_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), CTK_TYPE_UI_MANAGER, CtkUIManagerClass))

typedef struct _CtkUIManager      CtkUIManager;
typedef struct _CtkUIManagerClass CtkUIManagerClass;
typedef struct _CtkUIManagerPrivate CtkUIManagerPrivate;


struct _CtkUIManager {
  GObject parent;

  /*< private >*/
  CtkUIManagerPrivate *private_data;
};

struct _CtkUIManagerClass {
  GObjectClass parent_class;

  /* Signals */
  void (* add_widget)       (CtkUIManager *manager,
                             CtkWidget    *widget);
  void (* actions_changed)  (CtkUIManager *manager);
  void (* connect_proxy)    (CtkUIManager *manager,
			     CtkAction    *action,
			     CtkWidget    *proxy);
  void (* disconnect_proxy) (CtkUIManager *manager,
			     CtkAction    *action,
			     CtkWidget    *proxy);
  void (* pre_activate)     (CtkUIManager *manager,
			     CtkAction    *action);
  void (* post_activate)    (CtkUIManager *manager,
			     CtkAction    *action);

  /* Virtual functions */
  CtkWidget * (* get_widget) (CtkUIManager *manager,
                              const gchar  *path);
  CtkAction * (* get_action) (CtkUIManager *manager,
                              const gchar  *path);

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

/**
 * CtkUIManagerItemType:
 * @CTK_UI_MANAGER_AUTO: Pick the type of the UI element according to context.
 * @CTK_UI_MANAGER_MENUBAR: Create a menubar.
 * @CTK_UI_MANAGER_MENU: Create a menu.
 * @CTK_UI_MANAGER_TOOLBAR: Create a toolbar.
 * @CTK_UI_MANAGER_PLACEHOLDER: Insert a placeholder.
 * @CTK_UI_MANAGER_POPUP: Create a popup menu.
 * @CTK_UI_MANAGER_MENUITEM: Create a menuitem.
 * @CTK_UI_MANAGER_TOOLITEM: Create a toolitem.
 * @CTK_UI_MANAGER_SEPARATOR: Create a separator.
 * @CTK_UI_MANAGER_ACCELERATOR: Install an accelerator.
 * @CTK_UI_MANAGER_POPUP_WITH_ACCELS: Same as %CTK_UI_MANAGER_POPUP, but the
 *   actions’ accelerators are shown.
 *
 * These enumeration values are used by ctk_ui_manager_add_ui() to determine
 * what UI element to create.
 *
 * Deprecated: 3.10
 */
typedef enum {
  CTK_UI_MANAGER_AUTO              = 0,
  CTK_UI_MANAGER_MENUBAR           = 1 << 0,
  CTK_UI_MANAGER_MENU              = 1 << 1,
  CTK_UI_MANAGER_TOOLBAR           = 1 << 2,
  CTK_UI_MANAGER_PLACEHOLDER       = 1 << 3,
  CTK_UI_MANAGER_POPUP             = 1 << 4,
  CTK_UI_MANAGER_MENUITEM          = 1 << 5,
  CTK_UI_MANAGER_TOOLITEM          = 1 << 6,
  CTK_UI_MANAGER_SEPARATOR         = 1 << 7,
  CTK_UI_MANAGER_ACCELERATOR       = 1 << 8,
  CTK_UI_MANAGER_POPUP_WITH_ACCELS = 1 << 9
} CtkUIManagerItemType;

GDK_DEPRECATED_IN_3_10
GType          ctk_ui_manager_get_type            (void) G_GNUC_CONST;
GDK_DEPRECATED_IN_3_10
CtkUIManager  *ctk_ui_manager_new                 (void);
GDK_DEPRECATED_IN_3_4
void           ctk_ui_manager_set_add_tearoffs    (CtkUIManager          *manager,
                                                   gboolean               add_tearoffs);
GDK_DEPRECATED_IN_3_4
gboolean       ctk_ui_manager_get_add_tearoffs    (CtkUIManager          *manager);

GDK_DEPRECATED_IN_3_10
void           ctk_ui_manager_insert_action_group (CtkUIManager          *manager,
						   CtkActionGroup        *action_group,
						   gint                   pos);
GDK_DEPRECATED_IN_3_10
void           ctk_ui_manager_remove_action_group (CtkUIManager          *manager,
						   CtkActionGroup        *action_group);
GDK_DEPRECATED_IN_3_10
GList         *ctk_ui_manager_get_action_groups   (CtkUIManager          *manager);
GDK_DEPRECATED_IN_3_10
CtkAccelGroup *ctk_ui_manager_get_accel_group     (CtkUIManager          *manager);
GDK_DEPRECATED_IN_3_10
CtkWidget     *ctk_ui_manager_get_widget          (CtkUIManager          *manager,
						   const gchar           *path);
GDK_DEPRECATED_IN_3_10
GSList        *ctk_ui_manager_get_toplevels       (CtkUIManager          *manager,
						   CtkUIManagerItemType   types);
GDK_DEPRECATED_IN_3_10
CtkAction     *ctk_ui_manager_get_action          (CtkUIManager          *manager,
						   const gchar           *path);
GDK_DEPRECATED_IN_3_10
guint          ctk_ui_manager_add_ui_from_string  (CtkUIManager          *manager,
						   const gchar           *buffer,
						   gssize                 length,
						   GError               **error);
GDK_DEPRECATED_IN_3_10
guint          ctk_ui_manager_add_ui_from_file    (CtkUIManager          *manager,
						   const gchar           *filename,
						   GError               **error);
GDK_DEPRECATED_IN_3_10
guint          ctk_ui_manager_add_ui_from_resource(CtkUIManager          *manager,
						   const gchar           *resource_path,
						   GError               **error);
GDK_DEPRECATED_IN_3_10
void           ctk_ui_manager_add_ui              (CtkUIManager          *manager,
						   guint                  merge_id,
						   const gchar           *path,
						   const gchar           *name,
						   const gchar           *action,
						   CtkUIManagerItemType   type,
						   gboolean               top);
GDK_DEPRECATED_IN_3_10
void           ctk_ui_manager_remove_ui           (CtkUIManager          *manager,
						   guint                  merge_id);
GDK_DEPRECATED_IN_3_10
gchar         *ctk_ui_manager_get_ui              (CtkUIManager          *manager);
GDK_DEPRECATED_IN_3_10
void           ctk_ui_manager_ensure_update       (CtkUIManager          *manager);
GDK_DEPRECATED_IN_3_10
guint          ctk_ui_manager_new_merge_id        (CtkUIManager          *manager);

G_END_DECLS

#endif /* __CTK_UI_MANAGER_H__ */
