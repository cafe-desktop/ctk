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

#include <string.h>
#include <ctk/ctk.h>
#include "ctkstatusbaraccessible.h"


G_DEFINE_TYPE (CtkStatusbarAccessible, ctk_statusbar_accessible, CTK_TYPE_CONTAINER_ACCESSIBLE)

static void
text_changed (CtkStatusbar *statusbar G_GNUC_UNUSED,
              guint         context_id G_GNUC_UNUSED,
              const gchar  *text G_GNUC_UNUSED,
              AtkObject    *obj)
{
  if (!obj->name)
    g_object_notify (G_OBJECT (obj), "accessible-name");
  g_signal_emit_by_name (obj, "visible-data-changed");
}

static void
ctk_statusbar_accessible_initialize (AtkObject *obj,
                                     gpointer   data)
{
  CtkWidget *statusbar = data;

  ATK_OBJECT_CLASS (ctk_statusbar_accessible_parent_class)->initialize (obj, data);

  g_signal_connect_after (statusbar, "text-pushed",
                          G_CALLBACK (text_changed), obj);
  g_signal_connect_after (statusbar, "text-popped",
                          G_CALLBACK (text_changed), obj);

  obj->role = ATK_ROLE_STATUSBAR;
}

static CtkWidget *
find_label_child (CtkContainer *container)
{
  GList *children, *tmp_list;
  CtkWidget *child;

  children = ctk_container_get_children (container);

  child = NULL;
  for (tmp_list = children; tmp_list != NULL; tmp_list = tmp_list->next)
    {
      if (CTK_IS_LABEL (tmp_list->data))
        {
          child = CTK_WIDGET (tmp_list->data);
          break;
        }
      else if (CTK_IS_CONTAINER (tmp_list->data))
        {
          child = find_label_child (CTK_CONTAINER (tmp_list->data));
          if (child)
            break;
        }
    }
  g_list_free (children);

  return child;
}

static CtkWidget *
get_label_from_statusbar (CtkStatusbar *statusbar)
{
  CtkWidget *box;

  box = ctk_statusbar_get_message_area (statusbar);

  return find_label_child (CTK_CONTAINER (box));
}

static const gchar *
ctk_statusbar_accessible_get_name (AtkObject *obj)
{
  const gchar *name;
  CtkWidget *widget;
  CtkWidget *label;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  if (widget == NULL)
    return NULL;

  name = ATK_OBJECT_CLASS (ctk_statusbar_accessible_parent_class)->get_name (obj);
  if (name != NULL)
    return name;

  label = get_label_from_statusbar (CTK_STATUSBAR (widget));
  if (CTK_IS_LABEL (label))
    return ctk_label_get_label (CTK_LABEL (label));

  return NULL;
}

static gint
ctk_statusbar_accessible_get_n_children (AtkObject *obj G_GNUC_UNUSED)
{
  return 0;
}

static AtkObject*
ctk_statusbar_accessible_ref_child (AtkObject *obj G_GNUC_UNUSED,
                                    gint       i G_GNUC_UNUSED)
{
  return NULL;
}

static void
ctk_statusbar_accessible_class_init (CtkStatusbarAccessibleClass *klass)
{
  AtkObjectClass  *class = ATK_OBJECT_CLASS (klass);
  CtkContainerAccessibleClass *container_class = (CtkContainerAccessibleClass*)klass;

  class->get_name = ctk_statusbar_accessible_get_name;
  class->get_n_children = ctk_statusbar_accessible_get_n_children;
  class->ref_child = ctk_statusbar_accessible_ref_child;
  class->initialize = ctk_statusbar_accessible_initialize;
  /*
   * As we report the statusbar as having no children
   * we are not interested in add and remove signals
   */
  container_class->add_ctk = NULL;
  container_class->remove_ctk = NULL;
}

static void
ctk_statusbar_accessible_init (CtkStatusbarAccessible *bar G_GNUC_UNUSED)
{
}
