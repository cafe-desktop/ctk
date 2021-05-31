/* CTK+ - accessibility implementations
 * Copyright 2001, 2002, 2003 Sun Microsystems Inc.
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

#include "ctkcontaineraccessible.h"
#include "ctkcontaineraccessibleprivate.h"

#include <ctk/ctk.h>

#include "ctkwidgetprivate.h"

struct _CtkContainerAccessiblePrivate
{
  GList *children;
};

G_DEFINE_TYPE_WITH_PRIVATE (CtkContainerAccessible, ctk_container_accessible, CTK_TYPE_WIDGET_ACCESSIBLE)

static void
count_widget (CtkWidget *widget,
              gint      *count)
{
  (*count)++;
}

static gint
ctk_container_accessible_get_n_children (AtkObject* obj)
{
  CtkWidget *widget;
  gint count = 0;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  if (widget == NULL)
    return 0;

  ctk_container_foreach (CTK_CONTAINER (widget), (CtkCallback) count_widget, &count);
  return count;
}

static AtkObject *
ctk_container_accessible_ref_child (AtkObject *obj,
                                    gint       i)
{
  GList *children, *tmp_list;
  AtkObject  *accessible;
  CtkWidget *widget;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  if (widget == NULL)
    return NULL;

  children = ctk_container_get_children (CTK_CONTAINER (widget));
  tmp_list = g_list_nth (children, i);
  if (!tmp_list)
    {
      g_list_free (children);
      return NULL;
    }
  accessible = ctk_widget_get_accessible (CTK_WIDGET (tmp_list->data));

  g_list_free (children);
  g_object_ref (accessible);

  return accessible;
}

void
_ctk_container_accessible_add (CtkWidget *parent,
                               CtkWidget *child)
{
  CtkContainerAccessible *accessible;
  CtkContainerAccessibleClass *klass;
  AtkObject *obj;

  obj = _ctk_widget_peek_accessible (CTK_WIDGET (parent));
  if (!CTK_IS_CONTAINER_ACCESSIBLE (obj))
    return;

  accessible = CTK_CONTAINER_ACCESSIBLE (obj);
  klass = CTK_CONTAINER_ACCESSIBLE_GET_CLASS (accessible);

  if (klass->add_ctk)
    klass->add_ctk (CTK_CONTAINER (parent), child, obj);
}

void
_ctk_container_accessible_remove (CtkWidget *parent,
                                  CtkWidget *child)
{
  CtkContainerAccessible *accessible;
  CtkContainerAccessibleClass *klass;
  AtkObject *obj;

  obj = _ctk_widget_peek_accessible (CTK_WIDGET (parent));
  if (!CTK_IS_CONTAINER_ACCESSIBLE (obj))
    return;

  accessible = CTK_CONTAINER_ACCESSIBLE (obj);
  klass = CTK_CONTAINER_ACCESSIBLE_GET_CLASS (accessible);

  if (klass->remove_ctk)
    klass->remove_ctk (CTK_CONTAINER (parent), child, obj);
}

static gint
ctk_container_accessible_real_add_ctk (CtkContainer *container,
                                       CtkWidget    *widget,
                                       gpointer      data)
{
  AtkObject *atk_parent;
  AtkObject *atk_child;
  CtkContainerAccessible *accessible;
  gint index;

  atk_parent = ATK_OBJECT (data);
  atk_child = ctk_widget_get_accessible (widget);
  accessible = CTK_CONTAINER_ACCESSIBLE (atk_parent);

  g_list_free (accessible->priv->children);
  accessible->priv->children = ctk_container_get_children (container);
  index = g_list_index (accessible->priv->children, widget);
  _ctk_container_accessible_add_child (accessible, atk_child, index);

  return 1;
}

static gint
ctk_container_accessible_real_remove_ctk (CtkContainer *container,
                                          CtkWidget    *widget,
                                          gpointer      data)
{
  AtkObject* atk_parent;
  AtkObject *atk_child;
  CtkContainerAccessible *accessible;
  gint index;

  atk_parent = ATK_OBJECT (data);
  atk_child = _ctk_widget_peek_accessible (widget);
  if (atk_child == NULL)
    return 1;
  accessible = CTK_CONTAINER_ACCESSIBLE (atk_parent);

  index = g_list_index (accessible->priv->children, widget);
  g_list_free (accessible->priv->children);
  accessible->priv->children = ctk_container_get_children (container);
  if (index >= 0)
    _ctk_container_accessible_remove_child (accessible, atk_child, index);

  return 1;
}

static void
ctk_container_accessible_real_initialize (AtkObject *obj,
                                          gpointer   data)
{
  CtkContainerAccessible *accessible = CTK_CONTAINER_ACCESSIBLE (obj);

  ATK_OBJECT_CLASS (ctk_container_accessible_parent_class)->initialize (obj, data);

  accessible->priv->children = ctk_container_get_children (CTK_CONTAINER (data));

  obj->role = ATK_ROLE_PANEL;
}

static void
ctk_container_accessible_finalize (GObject *object)
{
  CtkContainerAccessible *accessible = CTK_CONTAINER_ACCESSIBLE (object);

  g_list_free (accessible->priv->children);

  G_OBJECT_CLASS (ctk_container_accessible_parent_class)->finalize (object);
}

static void
ctk_container_accessible_class_init (CtkContainerAccessibleClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);

  gobject_class->finalize = ctk_container_accessible_finalize;

  class->get_n_children = ctk_container_accessible_get_n_children;
  class->ref_child = ctk_container_accessible_ref_child;
  class->initialize = ctk_container_accessible_real_initialize;

  klass->add_ctk = ctk_container_accessible_real_add_ctk;
  klass->remove_ctk = ctk_container_accessible_real_remove_ctk;
}

static void
ctk_container_accessible_init (CtkContainerAccessible *container)
{
  container->priv = ctk_container_accessible_get_instance_private (container);
}

void
_ctk_container_accessible_add_child (CtkContainerAccessible *accessible,
                                     AtkObject              *child,
                                     gint                    index)
{
  g_object_notify (G_OBJECT (child), "accessible-parent");
  g_signal_emit_by_name (accessible, "children-changed::add", index, child, NULL);
}

void
_ctk_container_accessible_remove_child (CtkContainerAccessible *accessible,
                                        AtkObject              *child,
                                        gint                    index)
{
  g_object_notify (G_OBJECT (child), "accessible-parent");
  g_signal_emit_by_name (accessible, "children-changed::remove", index, child, NULL);
}
