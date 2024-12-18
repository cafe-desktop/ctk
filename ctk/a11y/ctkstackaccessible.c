/* CTK+ - accessibility implementations
 * Copyright (C) 2016  Timm Bäder <mail@baedert.org>
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

#include <string.h>
#include <ctk/ctk.h>
#include "ctkstackaccessible.h"
#include "ctkwidgetprivate.h"


G_DEFINE_TYPE (CtkStackAccessible, ctk_stack_accessible, CTK_TYPE_CONTAINER_ACCESSIBLE)

static AtkObject*
ctk_stack_accessible_ref_child (AtkObject *obj,
                                int        i)
{
  CtkWidget *stack = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  CtkWidget *visible_child;

  if (stack == NULL)
    return NULL;

  if (i != 0)
    return NULL;

  visible_child = ctk_stack_get_visible_child (CTK_STACK (stack));

  if (visible_child == NULL)
    return NULL;

  return g_object_ref (ctk_widget_get_accessible (visible_child));
}

static int
ctk_stack_accessible_get_n_children (AtkObject *obj)
{
  CtkWidget *stack = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));

  if (stack == NULL)
    return 0;

  if (ctk_stack_get_visible_child (CTK_STACK (stack)))
    return 1;

  return 0;
}

static void
ctk_stack_accessible_class_init (CtkStackAccessibleClass *klass)
{
  AtkObjectClass *class                        = ATK_OBJECT_CLASS (klass);
  CtkContainerAccessibleClass *container_class = (CtkContainerAccessibleClass*)klass;

  class->get_n_children = ctk_stack_accessible_get_n_children;
  class->ref_child      = ctk_stack_accessible_ref_child;
  /*
   * As we report the stack as having only the visible child,
   * we are not interested in add and remove signals
   */
  container_class->add_ctk    = NULL;
  container_class->remove_ctk = NULL;
}

static void
ctk_stack_accessible_init (CtkStackAccessible *bar G_GNUC_UNUSED) {}


void
ctk_stack_accessible_update_visible_child (CtkStack  *stack,
                                           CtkWidget *old_visible_child,
                                           CtkWidget *new_visible_child)
{
  AtkObject *stack_accessible = _ctk_widget_peek_accessible (CTK_WIDGET (stack));

  if (stack_accessible == NULL)
    return;

  if (old_visible_child)
    {
      AtkObject *accessible = ctk_widget_get_accessible (old_visible_child);
      g_object_notify (G_OBJECT (accessible), "accessible-parent");
      g_signal_emit_by_name (stack_accessible, "children-changed::remove", 0, accessible, NULL);
    }

  if (new_visible_child)
    {
      AtkObject *accessible = ctk_widget_get_accessible (new_visible_child);
      g_object_notify (G_OBJECT (accessible), "accessible-parent");
      g_signal_emit_by_name (stack_accessible, "children-changed::add", 0, accessible, NULL);
    }
}



