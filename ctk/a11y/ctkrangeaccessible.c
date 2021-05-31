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
#include "ctkrangeaccessible.h"

struct _CtkRangeAccessiblePrivate
{
  CtkAdjustment *adjustment;
};

static void atk_value_interface_init  (AtkValueIface  *iface);

G_DEFINE_TYPE_WITH_CODE (CtkRangeAccessible, ctk_range_accessible, CTK_TYPE_WIDGET_ACCESSIBLE,
                         G_ADD_PRIVATE (CtkRangeAccessible)
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_VALUE, atk_value_interface_init))

static void
ctk_range_accessible_value_changed (CtkAdjustment *adjustment,
                                    gpointer       data)
{
  g_object_notify (G_OBJECT (data), "accessible-value");
}

static void
ctk_range_accessible_widget_set (CtkAccessible *accessible)
{
  CtkRangeAccessiblePrivate *priv = CTK_RANGE_ACCESSIBLE (accessible)->priv;
  CtkWidget *range;
  CtkAdjustment *adj;

  range = ctk_accessible_get_widget (accessible);
  adj = ctk_range_get_adjustment (CTK_RANGE (range));
  if (adj)
    {
      priv->adjustment = adj;
      g_object_ref (priv->adjustment);
      g_signal_connect (priv->adjustment, "value-changed",
                        G_CALLBACK (ctk_range_accessible_value_changed),
                        accessible);
    }
}

static void
ctk_range_accessible_widget_unset (CtkAccessible *accessible)
{
  CtkRangeAccessiblePrivate *priv = CTK_RANGE_ACCESSIBLE (accessible)->priv;

  if (priv->adjustment)
    {
      g_signal_handlers_disconnect_by_func (priv->adjustment,
                                            G_CALLBACK (ctk_range_accessible_value_changed),
                                            accessible);
      g_object_unref (priv->adjustment);
      priv->adjustment = NULL;
    }
}

static void
ctk_range_accessible_initialize (AtkObject *obj,
                                 gpointer   data)
{
  ATK_OBJECT_CLASS (ctk_range_accessible_parent_class)->initialize (obj, data);
  obj->role = ATK_ROLE_SLIDER;
}

static void
ctk_range_accessible_notify_ctk (GObject    *obj,
                                 GParamSpec *pspec)
{
  CtkWidget *widget = CTK_WIDGET (obj);
  AtkObject *range;

  if (strcmp (pspec->name, "adjustment") == 0)
    {
      range = ctk_widget_get_accessible (widget);
      ctk_range_accessible_widget_unset (CTK_ACCESSIBLE (range));
      ctk_range_accessible_widget_set (CTK_ACCESSIBLE (range));
    }
  else
    CTK_WIDGET_ACCESSIBLE_CLASS (ctk_range_accessible_parent_class)->notify_ctk (obj, pspec);
}


static void
ctk_range_accessible_class_init (CtkRangeAccessibleClass *klass)
{
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);
  CtkAccessibleClass *accessible_class = (CtkAccessibleClass*)klass;
  CtkWidgetAccessibleClass *widget_class = (CtkWidgetAccessibleClass*)klass;

  class->initialize = ctk_range_accessible_initialize;

  accessible_class->widget_set = ctk_range_accessible_widget_set;
  accessible_class->widget_unset = ctk_range_accessible_widget_unset;

  widget_class->notify_ctk = ctk_range_accessible_notify_ctk;
}

static void
ctk_range_accessible_init (CtkRangeAccessible *range)
{
  range->priv = ctk_range_accessible_get_instance_private (range);
}

static void
ctk_range_accessible_get_current_value (AtkValue *obj,
                                        GValue   *value)
{
  CtkWidget *widget;
  CtkAdjustment *adjustment;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  adjustment = ctk_range_get_adjustment (CTK_RANGE (widget));
  if (adjustment == NULL)
    return;

  memset (value,  0, sizeof (GValue));
  g_value_init (value, G_TYPE_DOUBLE);
  g_value_set_double (value, ctk_adjustment_get_value (adjustment));
}

static void
ctk_range_accessible_get_maximum_value (AtkValue *obj,
                                        GValue   *value)
{
  CtkWidget *widget;
  CtkAdjustment *adjustment;
  gdouble max;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  adjustment = ctk_range_get_adjustment (CTK_RANGE (widget));
  if (adjustment == NULL)
    return;

  max = ctk_adjustment_get_upper (adjustment)
        - ctk_adjustment_get_page_size (adjustment);

  if (ctk_range_get_restrict_to_fill_level (CTK_RANGE (widget)))
    max = MIN (max, ctk_range_get_fill_level (CTK_RANGE (widget)));

  memset (value,  0, sizeof (GValue));
  g_value_init (value, G_TYPE_DOUBLE);
  g_value_set_double (value, max);
}

static void
ctk_range_accessible_get_minimum_value (AtkValue *obj,
                                        GValue   *value)
{
  CtkWidget *widget;
  CtkAdjustment *adjustment;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  adjustment = ctk_range_get_adjustment (CTK_RANGE (widget));
  if (adjustment == NULL)
    return;

  memset (value,  0, sizeof (GValue));
  g_value_init (value, G_TYPE_DOUBLE);
  g_value_set_double (value, ctk_adjustment_get_lower (adjustment));
}

static void
ctk_range_accessible_get_minimum_increment (AtkValue *obj,
                                            GValue   *value)
{
  CtkWidget *widget;
  CtkAdjustment *adjustment;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  adjustment = ctk_range_get_adjustment (CTK_RANGE (widget));
  if (adjustment == NULL)
    return;

  memset (value,  0, sizeof (GValue));
  g_value_init (value, G_TYPE_DOUBLE);
  g_value_set_double (value, ctk_adjustment_get_minimum_increment (adjustment));
}

static gboolean
ctk_range_accessible_set_current_value (AtkValue     *obj,
                                        const GValue *value)
{
 CtkWidget *widget;
  CtkAdjustment *adjustment;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  adjustment = ctk_range_get_adjustment (CTK_RANGE (widget));
  if (adjustment == NULL)
    return FALSE;

  ctk_adjustment_set_value (adjustment, g_value_get_double (value));

  return TRUE;
}

static void
ctk_range_accessible_get_value_and_text (AtkValue  *obj,
                                         gdouble   *value,
                                         gchar    **text)
{
  CtkWidget *widget;
  CtkAdjustment *adjustment;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  adjustment = ctk_range_get_adjustment (CTK_RANGE (widget));
  if (adjustment == NULL)
    return;

  *value = ctk_adjustment_get_value (adjustment);
  *text = NULL;
}

static AtkRange *
ctk_range_accessible_get_range (AtkValue *obj)
{
  CtkWidget *widget;
  CtkAdjustment *adjustment;
  gdouble min, max;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  adjustment = ctk_range_get_adjustment (CTK_RANGE (widget));
  if (adjustment == NULL)
    return NULL;

  min = ctk_adjustment_get_lower (adjustment);
  max = ctk_adjustment_get_upper (adjustment)
        - ctk_adjustment_get_page_size (adjustment);

  if (ctk_range_get_restrict_to_fill_level (CTK_RANGE (widget)))
    max = MIN (max, ctk_range_get_fill_level (CTK_RANGE (widget)));

  return atk_range_new (min, max, NULL);
}

static void
ctk_range_accessible_set_value (AtkValue      *obj,
                                const gdouble  value)
{
  CtkWidget *widget;
  CtkAdjustment *adjustment;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  adjustment = ctk_range_get_adjustment (CTK_RANGE (widget));
  if (adjustment == NULL)
    return;

  ctk_adjustment_set_value (adjustment, value);
}

static gdouble
ctk_range_accessible_get_increment (AtkValue *obj)
{
  CtkWidget *widget;
  CtkAdjustment *adjustment;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  adjustment = ctk_range_get_adjustment (CTK_RANGE (widget));
  if (adjustment == NULL)
    return 0;

  return ctk_adjustment_get_minimum_increment (adjustment);
}

static void
atk_value_interface_init (AtkValueIface *iface)
{
  iface->get_current_value = ctk_range_accessible_get_current_value;
  iface->get_maximum_value = ctk_range_accessible_get_maximum_value;
  iface->get_minimum_value = ctk_range_accessible_get_minimum_value;
  iface->get_minimum_increment = ctk_range_accessible_get_minimum_increment;
  iface->set_current_value = ctk_range_accessible_set_current_value;

  iface->get_value_and_text = ctk_range_accessible_get_value_and_text;
  iface->get_range = ctk_range_accessible_get_range;
  iface->set_value = ctk_range_accessible_set_value;
  iface->get_increment = ctk_range_accessible_get_increment;
}
