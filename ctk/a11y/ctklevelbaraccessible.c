/* CAIL - The GNOME Accessibility Implementation Library
 * Copyright 2001, 2002, 2003 Sun Microsystems Inc.
 * Copyright 2013 SUSE LLC.
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
#include "ctklevelbaraccessible.h"


static void atk_value_interface_init (AtkValueIface *iface);

G_DEFINE_TYPE_WITH_CODE (CtkLevelBarAccessible, ctk_level_bar_accessible, CTK_TYPE_WIDGET_ACCESSIBLE,
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_VALUE, atk_value_interface_init))

static void
ctk_level_bar_accessible_initialize (AtkObject *obj,
                                       gpointer  data)
{
  ATK_OBJECT_CLASS (ctk_level_bar_accessible_parent_class)->initialize (obj, data);

  obj->role = ATK_ROLE_LEVEL_BAR;
}

static void
ctk_level_bar_accessible_notify_ctk (GObject    *obj,
                                       GParamSpec *pspec)
{
  CtkWidget *widget = CTK_WIDGET (obj);
  CtkLevelBarAccessible *level_bar = CTK_LEVEL_BAR_ACCESSIBLE (ctk_widget_get_accessible (widget));

  if (strcmp (pspec->name, "value") == 0)
    {
      g_object_notify (G_OBJECT (level_bar), "accessible-value");
    }
  else
    CTK_WIDGET_ACCESSIBLE_CLASS (ctk_level_bar_accessible_parent_class)->notify_ctk (obj, pspec);
}



static void
ctk_level_bar_accessible_class_init (CtkLevelBarAccessibleClass *klass)
{
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);
  CtkWidgetAccessibleClass *widget_class = (CtkWidgetAccessibleClass*)klass;

  widget_class->notify_ctk = ctk_level_bar_accessible_notify_ctk;

  class->initialize = ctk_level_bar_accessible_initialize;
}

static void
ctk_level_bar_accessible_init (CtkLevelBarAccessible *button G_GNUC_UNUSED)
{
}

static void
ctk_level_bar_accessible_get_current_value (AtkValue *obj,
                                            GValue   *value)
{
  CtkWidget *widget;
  CtkLevelBar *level_bar;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  level_bar = CTK_LEVEL_BAR (widget);

  memset (value,  0, sizeof (GValue));
  g_value_init (value, G_TYPE_DOUBLE);
  g_value_set_double (value, ctk_level_bar_get_value (level_bar));
}

static void
ctk_level_bar_accessible_get_maximum_value (AtkValue *obj,
                                            GValue   *value)
{
  CtkWidget *widget;
  CtkLevelBar *level_bar;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  level_bar = CTK_LEVEL_BAR (widget);

  memset (value,  0, sizeof (GValue));
  g_value_init (value, G_TYPE_DOUBLE);
  g_value_set_double (value, ctk_level_bar_get_max_value (level_bar));
}

static void
ctk_level_bar_accessible_get_minimum_value (AtkValue *obj,
                                            GValue   *value)
{
  CtkWidget *widget;
  CtkLevelBar *level_bar;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  level_bar = CTK_LEVEL_BAR (widget);

  memset (value,  0, sizeof (GValue));
  g_value_init (value, G_TYPE_DOUBLE);
  g_value_set_double (value, ctk_level_bar_get_min_value (level_bar));
}

static gboolean
ctk_level_bar_accessible_set_current_value (AtkValue     *obj,
                                            const GValue *value)
{
  CtkWidget *widget;
  CtkLevelBar *level_bar;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  level_bar = CTK_LEVEL_BAR (widget);

  ctk_level_bar_set_value (level_bar, g_value_get_double (value));

  return TRUE;
}

static void
ctk_level_bar_accessible_get_value_and_text (AtkValue  *obj,
                                             gdouble   *value,
                                             gchar    **text)
{
  CtkWidget *widget;
  CtkLevelBar *level_bar;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  level_bar = CTK_LEVEL_BAR (widget);

  *value = ctk_level_bar_get_value (level_bar);
  *text = NULL;
}

static AtkRange *
ctk_level_bar_accessible_get_range (AtkValue *obj)
{
  CtkWidget *widget;
  CtkLevelBar *level_bar;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  level_bar = CTK_LEVEL_BAR (widget);

  return atk_range_new (ctk_level_bar_get_min_value (level_bar),
                        ctk_level_bar_get_max_value (level_bar),
                        NULL);
}

static void
ctk_level_bar_accessible_set_value (AtkValue      *obj,
                                    const gdouble  value)
{
  CtkWidget *widget;
  CtkLevelBar *level_bar;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  level_bar = CTK_LEVEL_BAR (widget);

  ctk_level_bar_set_value (level_bar, value);
}

static void
atk_value_interface_init (AtkValueIface *iface)
{
  iface->get_current_value = ctk_level_bar_accessible_get_current_value;
  iface->get_maximum_value = ctk_level_bar_accessible_get_maximum_value;
  iface->get_minimum_value = ctk_level_bar_accessible_get_minimum_value;
  iface->set_current_value = ctk_level_bar_accessible_set_current_value;

  iface->get_value_and_text = ctk_level_bar_accessible_get_value_and_text;
  iface->get_range = ctk_level_bar_accessible_get_range;
  iface->set_value = ctk_level_bar_accessible_set_value;
}
