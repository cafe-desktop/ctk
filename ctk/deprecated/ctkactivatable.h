/* CTK - The GIMP Toolkit
 * Copyright (C) 2008 Tristan Van Berkom <tristan.van.berkom@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __CTK_ACTIVATABLE_H__
#define __CTK_ACTIVATABLE_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/deprecated/ctkaction.h>

G_BEGIN_DECLS

#define CTK_TYPE_ACTIVATABLE            (ctk_activatable_get_type ())
#define CTK_ACTIVATABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_ACTIVATABLE, CtkActivatable))
#define CTK_ACTIVATABLE_CLASS(obj)      (G_TYPE_CHECK_CLASS_CAST ((obj), CTK_TYPE_ACTIVATABLE, CtkActivatableIface))
#define CTK_IS_ACTIVATABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_ACTIVATABLE))
#define CTK_ACTIVATABLE_GET_IFACE(obj)  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), CTK_TYPE_ACTIVATABLE, CtkActivatableIface))


typedef struct _CtkActivatable      CtkActivatable; /* Dummy typedef */
typedef struct _CtkActivatableIface CtkActivatableIface;


/**
 * CtkActivatableIface:
 * @update: Called to update the activatable when its related action’s properties change.
 * You must check the #CtkActivatable:use-action-appearance property only apply action
 * properties that are meant to effect the appearance accordingly.
 * @sync_action_properties: Called to update the activatable completely, this is called internally when
 * #CtkActivatable:related-action property is set or unset and by the implementor when
 * #CtkActivatable:use-action-appearance changes.
 *
 * > This method can be called with a %NULL action at times.
 *
 * Since: 2.16
 *
 * Deprecated: 3.10
 */

struct _CtkActivatableIface
{
  /*< private >*/
  GTypeInterface g_iface;

  /*< public >*/

  /* virtual table */
  void   (* update)                   (CtkActivatable *activatable,
		                       CtkAction      *action,
		                       const gchar    *property_name);
  void   (* sync_action_properties)   (CtkActivatable *activatable,
		                       CtkAction      *action);
};


CDK_DEPRECATED_IN_3_10
GType      ctk_activatable_get_type                   (void) G_GNUC_CONST;

CDK_DEPRECATED_IN_3_10
void       ctk_activatable_sync_action_properties     (CtkActivatable *activatable,
						       CtkAction      *action);

CDK_DEPRECATED_IN_3_10
void       ctk_activatable_set_related_action         (CtkActivatable *activatable,
						       CtkAction      *action);
CDK_DEPRECATED_IN_3_10
CtkAction *ctk_activatable_get_related_action         (CtkActivatable *activatable);

CDK_DEPRECATED_IN_3_10
void       ctk_activatable_set_use_action_appearance  (CtkActivatable *activatable,
						       gboolean        use_appearance);
CDK_DEPRECATED_IN_3_10
gboolean   ctk_activatable_get_use_action_appearance  (CtkActivatable *activatable);

/* For use in activatable implementations */
CDK_DEPRECATED_IN_3_10
void       ctk_activatable_do_set_related_action      (CtkActivatable *activatable,
						       CtkAction      *action);

G_END_DECLS

#endif /* __CTK_ACTIVATABLE_H__ */
