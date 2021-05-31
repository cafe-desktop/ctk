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

#ifndef __CTK_ACTION_GROUP_H__
#define __CTK_ACTION_GROUP_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/deprecated/ctkaction.h>
#include <ctk/deprecated/ctkstock.h>

G_BEGIN_DECLS

#define CTK_TYPE_ACTION_GROUP              (ctk_action_group_get_type ())
#define CTK_ACTION_GROUP(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_ACTION_GROUP, CtkActionGroup))
#define CTK_ACTION_GROUP_CLASS(vtable)     (G_TYPE_CHECK_CLASS_CAST ((vtable), CTK_TYPE_ACTION_GROUP, CtkActionGroupClass))
#define CTK_IS_ACTION_GROUP(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_ACTION_GROUP))
#define CTK_IS_ACTION_GROUP_CLASS(vtable)  (G_TYPE_CHECK_CLASS_TYPE ((vtable), CTK_TYPE_ACTION_GROUP))
#define CTK_ACTION_GROUP_GET_CLASS(inst)   (G_TYPE_INSTANCE_GET_CLASS ((inst), CTK_TYPE_ACTION_GROUP, CtkActionGroupClass))

typedef struct _CtkActionGroup        CtkActionGroup;
typedef struct _CtkActionGroupPrivate CtkActionGroupPrivate;
typedef struct _CtkActionGroupClass   CtkActionGroupClass;
typedef struct _CtkActionEntry        CtkActionEntry;
typedef struct _CtkToggleActionEntry  CtkToggleActionEntry;
typedef struct _CtkRadioActionEntry   CtkRadioActionEntry;

struct _CtkActionGroup
{
  GObject parent;

  /*< private >*/
  CtkActionGroupPrivate *priv;
};

/**
 * CtkActionGroupClass:
 * @parent_class: The parent class.
 * @get_action: Looks up an action in the action group by name.
 */
struct _CtkActionGroupClass
{
  GObjectClass parent_class;

  CtkAction *(* get_action) (CtkActionGroup *action_group,
                             const gchar    *action_name);

  /*< private >*/

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

/**
 * CtkActionEntry:
 * @name: The name of the action.
 * @stock_id: The stock id for the action, or the name of an icon from the
 *  icon theme.
 * @label: The label for the action. This field should typically be marked
 *  for translation, see ctk_action_group_set_translation_domain(). If
 *  @label is %NULL, the label of the stock item with id @stock_id is used.
 * @accelerator: The accelerator for the action, in the format understood by
 *  ctk_accelerator_parse().
 * @tooltip: The tooltip for the action. This field should typically be
 *  marked for translation, see ctk_action_group_set_translation_domain().
 * @callback: The function to call when the action is activated.
 *
 * #CtkActionEntry structs are used with ctk_action_group_add_actions() to
 * construct actions.
 *
 * Deprecated: 3.10
 */
struct _CtkActionEntry 
{
  const gchar     *name;
  const gchar     *stock_id;
  const gchar     *label;
  const gchar     *accelerator;
  const gchar     *tooltip;
  GCallback  callback;
};

/**
 * CtkToggleActionEntry:
 * @name: The name of the action.
 * @stock_id: The stock id for the action, or the name of an icon from the
 *  icon theme.
 * @label: The label for the action. This field should typically be marked
 *  for translation, see ctk_action_group_set_translation_domain().
 * @accelerator: The accelerator for the action, in the format understood by
 *  ctk_accelerator_parse().
 * @tooltip: The tooltip for the action. This field should typically be
 *  marked for translation, see ctk_action_group_set_translation_domain().
 * @callback: The function to call when the action is activated.
 * @is_active: The initial state of the toggle action.
 *
 * #CtkToggleActionEntry structs are used with
 * ctk_action_group_add_toggle_actions() to construct toggle actions.
 *
 * Deprecated: 3.10
 */
struct _CtkToggleActionEntry 
{
  const gchar     *name;
  const gchar     *stock_id;
  const gchar     *label;
  const gchar     *accelerator;
  const gchar     *tooltip;
  GCallback  callback;
  gboolean   is_active;
};

/**
 * CtkRadioActionEntry:
 * @name: The name of the action.
 * @stock_id: The stock id for the action, or the name of an icon from the
 *  icon theme.
 * @label: The label for the action. This field should typically be marked
 *  for translation, see ctk_action_group_set_translation_domain().
 * @accelerator: The accelerator for the action, in the format understood by
 *  ctk_accelerator_parse().
 * @tooltip: The tooltip for the action. This field should typically be
 *  marked for translation, see ctk_action_group_set_translation_domain().
 * @value: The value to set on the radio action. See
 *  ctk_radio_action_get_current_value().
 *
 * #CtkRadioActionEntry structs are used with
 * ctk_action_group_add_radio_actions() to construct groups of radio actions.
 *
 * Deprecated: 3.10
 */
struct _CtkRadioActionEntry 
{
  const gchar *name;
  const gchar *stock_id;
  const gchar *label;
  const gchar *accelerator;
  const gchar *tooltip;
  gint   value; 
};

GDK_DEPRECATED_IN_3_10
GType           ctk_action_group_get_type                (void) G_GNUC_CONST;
GDK_DEPRECATED_IN_3_10
CtkActionGroup *ctk_action_group_new                     (const gchar                *name);
GDK_DEPRECATED_IN_3_10
const gchar    *ctk_action_group_get_name                (CtkActionGroup             *action_group);
GDK_DEPRECATED_IN_3_10
gboolean        ctk_action_group_get_sensitive           (CtkActionGroup             *action_group);
GDK_DEPRECATED_IN_3_10
void            ctk_action_group_set_sensitive           (CtkActionGroup             *action_group,
							  gboolean                    sensitive);
GDK_DEPRECATED_IN_3_10
gboolean        ctk_action_group_get_visible             (CtkActionGroup             *action_group);
GDK_DEPRECATED_IN_3_10
void            ctk_action_group_set_visible             (CtkActionGroup             *action_group,
							  gboolean                    visible);
GDK_DEPRECATED_IN_3_10
CtkAccelGroup  *ctk_action_group_get_accel_group         (CtkActionGroup             *action_group);
GDK_DEPRECATED_IN_3_10
void            ctk_action_group_set_accel_group         (CtkActionGroup             *action_group,
                                                          CtkAccelGroup              *accel_group);

GDK_DEPRECATED_IN_3_10
CtkAction      *ctk_action_group_get_action              (CtkActionGroup             *action_group,
							  const gchar                *action_name);
GDK_DEPRECATED_IN_3_10
GList          *ctk_action_group_list_actions            (CtkActionGroup             *action_group);
GDK_DEPRECATED_IN_3_10
void            ctk_action_group_add_action              (CtkActionGroup             *action_group,
							  CtkAction                  *action);
GDK_DEPRECATED_IN_3_10
void            ctk_action_group_add_action_with_accel   (CtkActionGroup             *action_group,
							  CtkAction                  *action,
							  const gchar                *accelerator);
GDK_DEPRECATED_IN_3_10
void            ctk_action_group_remove_action           (CtkActionGroup             *action_group,
							  CtkAction                  *action);
GDK_DEPRECATED_IN_3_10
void            ctk_action_group_add_actions             (CtkActionGroup             *action_group,
							  const CtkActionEntry       *entries,
							  guint                       n_entries,
							  gpointer                    user_data);
GDK_DEPRECATED_IN_3_10
void            ctk_action_group_add_toggle_actions      (CtkActionGroup             *action_group,
							  const CtkToggleActionEntry *entries,
							  guint                       n_entries,
							  gpointer                    user_data);
GDK_DEPRECATED_IN_3_10
void            ctk_action_group_add_radio_actions       (CtkActionGroup             *action_group,
							  const CtkRadioActionEntry  *entries,
							  guint                       n_entries,
							  gint                        value,
							  GCallback                   on_change,
							  gpointer                    user_data);
GDK_DEPRECATED_IN_3_10
void            ctk_action_group_add_actions_full        (CtkActionGroup             *action_group,
							  const CtkActionEntry       *entries,
							  guint                       n_entries,
							  gpointer                    user_data,
							  GDestroyNotify              destroy);
GDK_DEPRECATED_IN_3_10
void            ctk_action_group_add_toggle_actions_full (CtkActionGroup             *action_group,
							  const CtkToggleActionEntry *entries,
							  guint                       n_entries,
							  gpointer                    user_data,
							  GDestroyNotify              destroy);
GDK_DEPRECATED_IN_3_10
void            ctk_action_group_add_radio_actions_full  (CtkActionGroup             *action_group,
							  const CtkRadioActionEntry  *entries,
							  guint                       n_entries,
							  gint                        value,
							  GCallback                   on_change,
							  gpointer                    user_data,
							  GDestroyNotify              destroy);
GDK_DEPRECATED_IN_3_10
void            ctk_action_group_set_translate_func      (CtkActionGroup             *action_group,
							  CtkTranslateFunc            func,
							  gpointer                    data,
							  GDestroyNotify              notify);
GDK_DEPRECATED_IN_3_10
void            ctk_action_group_set_translation_domain  (CtkActionGroup             *action_group,
							  const gchar                *domain);
GDK_DEPRECATED_IN_3_10
const gchar *   ctk_action_group_translate_string        (CtkActionGroup             *action_group,
  	                                                  const gchar                *string);

/* Protected for use by CtkAction */
void _ctk_action_group_emit_connect_proxy    (CtkActionGroup *action_group,
                                              CtkAction      *action,
                                              CtkWidget      *proxy);
void _ctk_action_group_emit_disconnect_proxy (CtkActionGroup *action_group,
                                              CtkAction      *action,
                                              CtkWidget      *proxy);
void _ctk_action_group_emit_pre_activate     (CtkActionGroup *action_group,
                                              CtkAction      *action);
void _ctk_action_group_emit_post_activate    (CtkActionGroup *action_group,
                                              CtkAction      *action);

G_END_DECLS

#endif  /* __CTK_ACTION_GROUP_H__ */
