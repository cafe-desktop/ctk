/* GTK+ - accessibility implementations
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
#include "ctkpanedaccessible.h"

static void atk_value_interface_init (AtkValueIface *iface);

G_DEFINE_TYPE_WITH_CODE (GtkPanedAccessible, ctk_paned_accessible, CTK_TYPE_CONTAINER_ACCESSIBLE,
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_VALUE, atk_value_interface_init))

static void
ctk_paned_accessible_size_allocate_ctk (GtkWidget     *widget,
                                        GtkAllocation *allocation)
{
  AtkObject *obj = ctk_widget_get_accessible (widget);

  g_object_notify (G_OBJECT (obj), "accessible-value");
}

static void
ctk_paned_accessible_initialize (AtkObject *obj,
                                 gpointer   data)
{
  ATK_OBJECT_CLASS (ctk_paned_accessible_parent_class)->initialize (obj, data);

  g_signal_connect (data, "size-allocate",
                    G_CALLBACK (ctk_paned_accessible_size_allocate_ctk), NULL);

  obj->role = ATK_ROLE_SPLIT_PANE;
}

static void
ctk_paned_accessible_class_init (GtkPanedAccessibleClass *klass)
{
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);

  class->initialize = ctk_paned_accessible_initialize;
}

static void
ctk_paned_accessible_init (GtkPanedAccessible *paned)
{
}

static void
ctk_paned_accessible_get_current_value (AtkValue *obj,
                                        GValue   *value)
{
  GtkWidget* widget;
  gint current_value;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  if (widget == NULL)
    return;

  current_value = ctk_paned_get_position (CTK_PANED (widget));
  memset (value,  0, sizeof (GValue));
  g_value_init (value, G_TYPE_INT);
  g_value_set_int (value, current_value);
}

static void
ctk_paned_accessible_get_maximum_value (AtkValue *obj,
                                        GValue   *value)
{
  GtkWidget* widget;
  gint maximum_value;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  if (widget == NULL)
    return;

  g_object_get (CTK_PANED (widget),
                "max-position", &maximum_value,
                NULL);
  memset (value,  0, sizeof (GValue));
  g_value_init (value, G_TYPE_INT);
  g_value_set_int (value, maximum_value);
}

static void
ctk_paned_accessible_get_minimum_value (AtkValue *obj,
                                        GValue   *value)
{
  GtkWidget* widget;
  gint minimum_value;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  if (widget == NULL)
    return;

  g_object_get (CTK_PANED (widget),
                "min-position", &minimum_value,
                NULL);
  memset (value,  0, sizeof (GValue));
  g_value_init (value, G_TYPE_INT);
  g_value_set_int (value, minimum_value);
}

/* Calling atk_value_set_current_value() is no guarantee that the value
 * is acceptable; it is necessary to listen for accessible-value signals
 * and check whether the current value has been changed or check what the
 * maximum and minimum values are.
 */
static gboolean
ctk_paned_accessible_set_current_value (AtkValue     *obj,
                                        const GValue *value)
{
  GtkWidget* widget;
  gint new_value;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  if (widget == NULL)
    return FALSE;

  if (G_VALUE_HOLDS_INT (value))
    {
      new_value = g_value_get_int (value);
      ctk_paned_set_position (CTK_PANED (widget), new_value);

      return TRUE;
    }
  else
    return FALSE;
}

static void
ctk_paned_accessible_get_value_and_text (AtkValue  *obj,
                                         gdouble   *value,
                                         gchar    **text)
{
  GtkWidget *widget;
  GtkPaned *paned;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  paned = CTK_PANED (widget);

  *value = ctk_paned_get_position (paned);
  *text = NULL;
}

static AtkRange *
ctk_paned_accessible_get_range (AtkValue *obj)
{
  GtkWidget *widget;
  gint minimum_value;
  gint maximum_value;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));

  g_object_get (widget,
                "min-position", &minimum_value,
                "max-position", &maximum_value,
                NULL);

  return atk_range_new (minimum_value, maximum_value, NULL);
}

static void
ctk_paned_accessible_set_value (AtkValue      *obj,
                                const gdouble  value)
{
  GtkWidget *widget;
  GtkPaned *paned;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  paned = CTK_PANED (widget);

  ctk_paned_set_position (paned, (gint)(value + 0.5));
}

static void
atk_value_interface_init (AtkValueIface *iface)
{
  iface->get_current_value = ctk_paned_accessible_get_current_value;
  iface->get_maximum_value = ctk_paned_accessible_get_maximum_value;
  iface->get_minimum_value = ctk_paned_accessible_get_minimum_value;
  iface->set_current_value = ctk_paned_accessible_set_current_value;

  iface->get_value_and_text = ctk_paned_accessible_get_value_and_text;
  iface->get_range = ctk_paned_accessible_get_range;
  iface->set_value = ctk_paned_accessible_set_value;
}
