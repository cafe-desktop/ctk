/* CTK+ - accessibility implementations
 * Copyright 2002 Sun Microsystems Inc.
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
 */

#include "config.h"

#include <ctk/ctk.h>
#include "ctkradiomenuitemaccessible.h"

struct _CtkRadioMenuItemAccessiblePrivate
{
  GSList *old_group;
};

G_DEFINE_TYPE_WITH_PRIVATE (CtkRadioMenuItemAccessible,
                            ctk_radio_menu_item_accessible,
                            CTK_TYPE_CHECK_MENU_ITEM_ACCESSIBLE)


static AtkRelationSet *
ctk_radio_menu_item_accessible_ref_relation_set (AtkObject *obj)
{
  CtkWidget *widget;
  AtkRelationSet *relation_set;
  GSList *list;
  CtkRadioMenuItemAccessible *radio_menu_item;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  if (widget == NULL)
    return NULL;

  radio_menu_item = CTK_RADIO_MENU_ITEM_ACCESSIBLE (obj);

  relation_set = ATK_OBJECT_CLASS (ctk_radio_menu_item_accessible_parent_class)->ref_relation_set (obj);

  /* If the radio menu_item's group has changed remove the relation */
  list = ctk_radio_menu_item_get_group (CTK_RADIO_MENU_ITEM (widget));

  if (radio_menu_item->priv->old_group != list)
    {
      AtkRelation *relation;

      relation = atk_relation_set_get_relation_by_type (relation_set, ATK_RELATION_MEMBER_OF);
      atk_relation_set_remove (relation_set, relation);
    }

  if (!atk_relation_set_contains (relation_set, ATK_RELATION_MEMBER_OF))
    {
      /* Get the members of the menu_item group */
      radio_menu_item->priv->old_group = list;
      if (list)
        {
          AtkObject **accessible_array;
          guint list_length;
          AtkRelation* relation;
          gint i = 0;

          list_length = g_slist_length (list);
          accessible_array = g_new (AtkObject *, list_length);
          while (list != NULL)
            {
              CtkWidget* list_item = list->data;

              accessible_array[i++] = ctk_widget_get_accessible (list_item);

              list = list->next;
            }
          relation = atk_relation_new (accessible_array, list_length,
                                       ATK_RELATION_MEMBER_OF);
          g_free (accessible_array);

          atk_relation_set_add (relation_set, relation);

          /* Unref the relation so that it is not leaked */
          g_object_unref (relation);
        }
    }

  return relation_set;
}

static void
ctk_radio_menu_item_accessible_initialize (AtkObject *obj,
                                              gpointer   data)
{
  ATK_OBJECT_CLASS (ctk_radio_menu_item_accessible_parent_class)->initialize (obj, data);

  obj->role = ATK_ROLE_RADIO_MENU_ITEM;
}

static void
ctk_radio_menu_item_accessible_class_init (CtkRadioMenuItemAccessibleClass *klass)
{
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);

  class->ref_relation_set = ctk_radio_menu_item_accessible_ref_relation_set;
  class->initialize = ctk_radio_menu_item_accessible_initialize;
}

static void
ctk_radio_menu_item_accessible_init (CtkRadioMenuItemAccessible *radio_menu_item)
{
  radio_menu_item->priv = ctk_radio_menu_item_accessible_get_instance_private (radio_menu_item);
}
