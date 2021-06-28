/* CTK - The GIMP Toolkit
 * Copyright (C) 2016, Red Hat, Inc.
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
 *
 * Author(s): Carlos Garnacho <carlosg@gnome.org>
 */

#ifndef __CTK_PAD_CONTROLLER_H__
#define __CTK_PAD_CONTROLLER_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <cdk/cdk.h>
#include <ctk/ctkeventcontroller.h>

G_BEGIN_DECLS

#define CTK_TYPE_PAD_CONTROLLER         (ctk_pad_controller_get_type ())
#define CTK_PAD_CONTROLLER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), CTK_TYPE_PAD_CONTROLLER, CtkPadController))
#define CTK_PAD_CONTROLLER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), CTK_TYPE_PAD_CONTROLLER, CtkPadControllerClass))
#define CTK_IS_PAD_CONTROLLER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CTK_TYPE_PAD_CONTROLLER))
#define CTK_IS_PAD_CONTROLLER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), CTK_TYPE_PAD_CONTROLLER))
#define CTK_PAD_CONTROLLER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), CTK_TYPE_PAD_CONTROLLER, CtkPadControllerClass))

typedef struct _CtkPadController CtkPadController;
typedef struct _CtkPadControllerClass CtkPadControllerClass;
typedef struct _CtkPadActionEntry CtkPadActionEntry;

/**
 * CtkPadActionType:
 * @CTK_PAD_ACTION_BUTTON: Action is triggered by a pad button
 * @CTK_PAD_ACTION_RING: Action is triggered by a pad ring
 * @CTK_PAD_ACTION_STRIP: Action is triggered by a pad strip
 *
 * The type of a pad action.
 */
typedef enum {
  CTK_PAD_ACTION_BUTTON,
  CTK_PAD_ACTION_RING,
  CTK_PAD_ACTION_STRIP
} CtkPadActionType;

/**
 * CtkPadActionEntry:
 * @type: the type of pad feature that will trigger this action entry.
 * @index: the 0-indexed button/ring/strip number that will trigger this action
 *   entry.
 * @mode: the mode that will trigger this action entry, or -1 for all modes.
 * @label: Human readable description of this action entry, this string should
 *   be deemed user-visible.
 * @action_name: action name that will be activated in the #GActionGroup.
 *
 * Struct defining a pad action entry.
 */
struct _CtkPadActionEntry {
  CtkPadActionType type;
  gint index;
  gint mode;
  gchar *label;
  gchar *action_name;
};

GDK_AVAILABLE_IN_3_22
GType ctk_pad_controller_get_type           (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_3_22
CtkPadController *ctk_pad_controller_new    (CtkWindow        *window,
                                             GActionGroup     *group,
                                             CdkDevice        *pad);

GDK_AVAILABLE_IN_3_22
void  ctk_pad_controller_set_action_entries (CtkPadController        *controller,
                                             const CtkPadActionEntry *entries,
                                             gint                     n_entries);
GDK_AVAILABLE_IN_3_22
void  ctk_pad_controller_set_action         (CtkPadController *controller,
                                             CtkPadActionType  type,
                                             gint              index,
                                             gint              mode,
                                             const gchar      *label,
                                             const gchar      *action_name);

G_END_DECLS

#endif /* __CTK_PAD_CONTROLLER_H__ */
