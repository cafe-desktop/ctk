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

#include "ctkheaderbaraccessible.h"

#include "ctkcontainerprivate.h"

G_DEFINE_TYPE (CtkHeaderBarAccessible, ctk_header_bar_accessible, CTK_TYPE_CONTAINER_ACCESSIBLE)

static void
count_widget (CtkWidget *widget G_GNUC_UNUSED,
              gint      *count)
{
  (*count)++;
}

static gint
ctk_header_bar_accessible_get_n_children (AtkObject* obj)
{
  CtkWidget *widget;
  gint count = 0;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  if (widget == NULL)
    return 0;

  ctk_container_forall (CTK_CONTAINER (widget), (CtkCallback) count_widget, &count);
  return count;
}

static AtkObject *
ctk_header_bar_accessible_ref_child (AtkObject *obj,
                                     gint       i)
{
  GList *children, *tmp_list;
  AtkObject  *accessible;
  CtkWidget *widget;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  if (widget == NULL)
    return NULL;

  children = ctk_container_get_all_children (CTK_CONTAINER (widget));
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

static void
ctk_header_bar_accessible_class_init (CtkHeaderBarAccessibleClass *klass)
{
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);

  class->get_n_children = ctk_header_bar_accessible_get_n_children;
  class->ref_child = ctk_header_bar_accessible_ref_child;
}

static void
ctk_header_bar_accessible_init (CtkHeaderBarAccessible *header_bar G_GNUC_UNUSED)
{
}

