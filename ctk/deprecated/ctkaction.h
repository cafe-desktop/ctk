/*
 * CTK - The GIMP Toolkit
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
 * Modified by the CTK+ Team and others 2003.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CTK_ACTION_H__
#define __CTK_ACTION_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkwidget.h>

G_BEGIN_DECLS

#define CTK_TYPE_ACTION            (ctk_action_get_type ())
#define CTK_ACTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_ACTION, CtkAction))
#define CTK_ACTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_ACTION, CtkActionClass))
#define CTK_IS_ACTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_ACTION))
#define CTK_IS_ACTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_ACTION))
#define CTK_ACTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), CTK_TYPE_ACTION, CtkActionClass))

typedef struct _CtkAction      CtkAction;
typedef struct _CtkActionClass CtkActionClass;
typedef struct _CtkActionPrivate CtkActionPrivate;

struct _CtkAction
{
  GObject object;

  /*< private >*/
  CtkActionPrivate *private_data;
};

/**
 * CtkActionClass:
 * @parent_class: The parent class.
 * @activate: Signal emitted when the action is activated.
 */
struct _CtkActionClass
{
  GObjectClass parent_class;

  /*< public >*/

  /* activation signal */
  void       (* activate)           (CtkAction    *action);

  /*< private >*/

  GType      menu_item_type;
  GType      toolbar_item_type;

  /* widget creation routines (not signals) */
  CtkWidget *(* create_menu_item)   (CtkAction *action);
  CtkWidget *(* create_tool_item)   (CtkAction *action);
  void       (* connect_proxy)      (CtkAction *action,
				     CtkWidget *proxy);
  void       (* disconnect_proxy)   (CtkAction *action,
				     CtkWidget *proxy);

  CtkWidget *(* create_menu)        (CtkAction *action);

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

CDK_DEPRECATED_IN_3_10
GType        ctk_action_get_type               (void) G_GNUC_CONST;
CDK_DEPRECATED_IN_3_10
CtkAction   *ctk_action_new                    (const gchar *name,
						const gchar *label,
						const gchar *tooltip,
						const gchar *stock_id);
CDK_DEPRECATED_IN_3_10
const gchar* ctk_action_get_name               (CtkAction     *action);
CDK_DEPRECATED_IN_3_10
gboolean     ctk_action_is_sensitive           (CtkAction     *action);
CDK_DEPRECATED_IN_3_10
gboolean     ctk_action_get_sensitive          (CtkAction     *action);
CDK_DEPRECATED_IN_3_10
void         ctk_action_set_sensitive          (CtkAction     *action,
						gboolean       sensitive);
CDK_DEPRECATED_IN_3_10
gboolean     ctk_action_is_visible             (CtkAction     *action);
CDK_DEPRECATED_IN_3_10
gboolean     ctk_action_get_visible            (CtkAction     *action);
CDK_DEPRECATED_IN_3_10
void         ctk_action_set_visible            (CtkAction     *action,
						gboolean       visible);
CDK_DEPRECATED_IN_3_10
void         ctk_action_activate               (CtkAction     *action);
CDK_DEPRECATED_IN_3_10
CtkWidget *  ctk_action_create_icon            (CtkAction     *action,
						CtkIconSize    icon_size);
CDK_DEPRECATED_IN_3_10
CtkWidget *  ctk_action_create_menu_item       (CtkAction     *action);
CDK_DEPRECATED_IN_3_10
CtkWidget *  ctk_action_create_tool_item       (CtkAction     *action);
CDK_DEPRECATED_IN_3_10
CtkWidget *  ctk_action_create_menu            (CtkAction     *action);
CDK_DEPRECATED_IN_3_10
GSList *     ctk_action_get_proxies            (CtkAction     *action);
CDK_DEPRECATED_IN_3_10
void         ctk_action_connect_accelerator    (CtkAction     *action);
CDK_DEPRECATED_IN_3_10
void         ctk_action_disconnect_accelerator (CtkAction     *action);
CDK_DEPRECATED_IN_3_10
const gchar *ctk_action_get_accel_path         (CtkAction     *action);
CDK_DEPRECATED_IN_3_10
GClosure    *ctk_action_get_accel_closure      (CtkAction     *action);
CDK_DEPRECATED_IN_3_10
void         ctk_action_block_activate         (CtkAction     *action);
CDK_DEPRECATED_IN_3_10
void         ctk_action_unblock_activate       (CtkAction     *action);

void         _ctk_action_add_to_proxy_list     (CtkAction     *action,
						CtkWidget     *proxy);
void         _ctk_action_remove_from_proxy_list(CtkAction     *action,
						CtkWidget     *proxy);

/* protected ... for use by child actions */
void         _ctk_action_emit_activate         (CtkAction     *action);

/* protected ... for use by action groups */
CDK_DEPRECATED_IN_3_10
void         ctk_action_set_accel_path         (CtkAction     *action,
						const gchar   *accel_path);
CDK_DEPRECATED_IN_3_10
void         ctk_action_set_accel_group        (CtkAction     *action,
						CtkAccelGroup *accel_group);
void         _ctk_action_sync_menu_visible     (CtkAction     *action,
						CtkWidget     *proxy,
						gboolean       empty);

CDK_DEPRECATED_IN_3_10
void                  ctk_action_set_label              (CtkAction   *action,
                                                         const gchar *label);
CDK_DEPRECATED_IN_3_10
const gchar *         ctk_action_get_label              (CtkAction   *action);
CDK_DEPRECATED_IN_3_10
void                  ctk_action_set_short_label        (CtkAction   *action,
                                                         const gchar *short_label);
CDK_DEPRECATED_IN_3_10
const gchar *         ctk_action_get_short_label        (CtkAction   *action);
CDK_DEPRECATED_IN_3_10
void                  ctk_action_set_tooltip            (CtkAction   *action,
                                                         const gchar *tooltip);
CDK_DEPRECATED_IN_3_10
const gchar *         ctk_action_get_tooltip            (CtkAction   *action);
CDK_DEPRECATED_IN_3_10
void                  ctk_action_set_stock_id           (CtkAction   *action,
                                                         const gchar *stock_id);
CDK_DEPRECATED_IN_3_10
const gchar *         ctk_action_get_stock_id           (CtkAction   *action);
CDK_DEPRECATED_IN_3_10
void                  ctk_action_set_gicon              (CtkAction   *action,
                                                         GIcon       *icon);
CDK_DEPRECATED_IN_3_10
GIcon                *ctk_action_get_gicon              (CtkAction   *action);
CDK_DEPRECATED_IN_3_10
void                  ctk_action_set_icon_name          (CtkAction   *action,
                                                         const gchar *icon_name);
CDK_DEPRECATED_IN_3_10
const gchar *         ctk_action_get_icon_name          (CtkAction   *action);
CDK_DEPRECATED_IN_3_10
void                  ctk_action_set_visible_horizontal (CtkAction   *action,
                                                         gboolean     visible_horizontal);
CDK_DEPRECATED_IN_3_10
gboolean              ctk_action_get_visible_horizontal (CtkAction   *action);
CDK_DEPRECATED_IN_3_10
void                  ctk_action_set_visible_vertical   (CtkAction   *action,
                                                         gboolean     visible_vertical);
CDK_DEPRECATED_IN_3_10
gboolean              ctk_action_get_visible_vertical   (CtkAction   *action);
CDK_DEPRECATED_IN_3_10
void                  ctk_action_set_is_important       (CtkAction   *action,
                                                         gboolean     is_important);
CDK_DEPRECATED_IN_3_10
gboolean              ctk_action_get_is_important       (CtkAction   *action);
CDK_DEPRECATED_IN_3_10
void                  ctk_action_set_always_show_image  (CtkAction   *action,
                                                         gboolean     always_show);
CDK_DEPRECATED_IN_3_10
gboolean              ctk_action_get_always_show_image  (CtkAction   *action);


G_END_DECLS

#endif  /* __CTK_ACTION_H__ */
