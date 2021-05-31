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
#include <gtk/gtk.h>
#include "gtkspinbuttonaccessible.h"

struct _GtkSpinButtonAccessiblePrivate
{
  GtkAdjustment *adjustment;
};

static void atk_value_interface_init (AtkValueIface *iface);

G_DEFINE_TYPE_WITH_CODE (GtkSpinButtonAccessible, ctk_spin_button_accessible, GTK_TYPE_ENTRY_ACCESSIBLE,
                         G_ADD_PRIVATE (GtkSpinButtonAccessible)
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_VALUE, atk_value_interface_init))

static void
ctk_spin_button_accessible_value_changed (GtkAdjustment *adjustment,
                                          gpointer       data)
{
  g_object_notify (G_OBJECT (data), "accessible-value");
}

static void
ctk_spin_button_accessible_widget_set (GtkAccessible *accessible)
{
  GtkSpinButtonAccessiblePrivate *priv = GTK_SPIN_BUTTON_ACCESSIBLE (accessible)->priv;
  GtkWidget *spin;
  GtkAdjustment *adj;

  spin = ctk_accessible_get_widget (accessible);
  adj = ctk_spin_button_get_adjustment (GTK_SPIN_BUTTON (spin));
  if (adj)
    {
      priv->adjustment = adj;
      g_object_ref (priv->adjustment);
      g_signal_connect (priv->adjustment, "value-changed",
                        G_CALLBACK (ctk_spin_button_accessible_value_changed),
                        accessible);
    }
}

static void
ctk_spin_button_accessible_widget_unset (GtkAccessible *accessible)
{
  GtkSpinButtonAccessiblePrivate *priv = GTK_SPIN_BUTTON_ACCESSIBLE (accessible)->priv;

  if (priv->adjustment)
    {
      g_signal_handlers_disconnect_by_func (priv->adjustment,
                                            G_CALLBACK (ctk_spin_button_accessible_value_changed),
                                            accessible);
      g_object_unref (priv->adjustment);
      priv->adjustment = NULL;
    }
}

static void
ctk_spin_button_accessible_initialize (AtkObject *obj,
                                       gpointer  data)
{
  ATK_OBJECT_CLASS (ctk_spin_button_accessible_parent_class)->initialize (obj, data);
  obj->role = ATK_ROLE_SPIN_BUTTON;
}

static void
ctk_spin_button_accessible_notify_gtk (GObject    *obj,
                                       GParamSpec *pspec)
{
  GtkWidget *widget = GTK_WIDGET (obj);
  AtkObject *spin;

  if (strcmp (pspec->name, "adjustment") == 0)
    {
      spin = ctk_widget_get_accessible (widget);
      ctk_spin_button_accessible_widget_unset (GTK_ACCESSIBLE (spin));
      ctk_spin_button_accessible_widget_set (GTK_ACCESSIBLE (spin));

    }
  else
    GTK_WIDGET_ACCESSIBLE_CLASS (ctk_spin_button_accessible_parent_class)->notify_gtk (obj, pspec);
}

static void
ctk_spin_button_accessible_class_init (GtkSpinButtonAccessibleClass *klass)
{
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);
  GtkAccessibleClass *accessible_class = (GtkAccessibleClass*)klass;
  GtkWidgetAccessibleClass *widget_class = (GtkWidgetAccessibleClass*)klass;

  class->initialize = ctk_spin_button_accessible_initialize;

  accessible_class->widget_set = ctk_spin_button_accessible_widget_set;
  accessible_class->widget_unset = ctk_spin_button_accessible_widget_unset;

  widget_class->notify_gtk = ctk_spin_button_accessible_notify_gtk;
}

static void
ctk_spin_button_accessible_init (GtkSpinButtonAccessible *button)
{
  button->priv = ctk_spin_button_accessible_get_instance_private (button);
}

static void
ctk_spin_button_accessible_get_current_value (AtkValue *obj,
                                              GValue   *value)
{
  GtkWidget *widget;
  GtkAdjustment *adjustment;

  widget = ctk_accessible_get_widget (GTK_ACCESSIBLE (obj));
  adjustment = ctk_spin_button_get_adjustment (GTK_SPIN_BUTTON (widget));
  if (adjustment == NULL)
    return;

  memset (value,  0, sizeof (GValue));
  g_value_init (value, G_TYPE_DOUBLE);
  g_value_set_double (value, ctk_adjustment_get_value (adjustment));
}

static void
ctk_spin_button_accessible_get_maximum_value (AtkValue *obj,
                                              GValue   *value)
{
  GtkWidget *widget;
  GtkAdjustment *adjustment;

  widget = ctk_accessible_get_widget (GTK_ACCESSIBLE (obj));
  adjustment = ctk_spin_button_get_adjustment (GTK_SPIN_BUTTON (widget));
  if (adjustment == NULL)
    return;

  memset (value,  0, sizeof (GValue));
  g_value_init (value, G_TYPE_DOUBLE);
  g_value_set_double (value, ctk_adjustment_get_upper (adjustment));
}

static void
ctk_spin_button_accessible_get_minimum_value (AtkValue *obj,
                                              GValue   *value)
{
  GtkWidget *widget;
  GtkAdjustment *adjustment;

  widget = ctk_accessible_get_widget (GTK_ACCESSIBLE (obj));
  adjustment = ctk_spin_button_get_adjustment (GTK_SPIN_BUTTON (widget));
  if (adjustment == NULL)
    return;

  memset (value,  0, sizeof (GValue));
  g_value_init (value, G_TYPE_DOUBLE);
  g_value_set_double (value, ctk_adjustment_get_lower (adjustment));
}

static void
ctk_spin_button_accessible_get_minimum_increment (AtkValue *obj,
                                                  GValue   *value)
{
  GtkWidget *widget;
  GtkAdjustment *adjustment;

  widget = ctk_accessible_get_widget (GTK_ACCESSIBLE (obj));
  adjustment = ctk_spin_button_get_adjustment (GTK_SPIN_BUTTON (widget));
  if (adjustment == NULL)
    return;

  memset (value,  0, sizeof (GValue));
  g_value_init (value, G_TYPE_DOUBLE);
  g_value_set_double (value, ctk_adjustment_get_minimum_increment (adjustment));
}

static gboolean
ctk_spin_button_accessible_set_current_value (AtkValue     *obj,
                                              const GValue *value)
{
  GtkWidget *widget;
  GtkAdjustment *adjustment;

  widget = ctk_accessible_get_widget (GTK_ACCESSIBLE (obj));
  adjustment = ctk_spin_button_get_adjustment (GTK_SPIN_BUTTON (widget));
  if (adjustment == NULL)
    return FALSE;

  ctk_adjustment_set_value (adjustment, g_value_get_double (value));

  return TRUE;
}

static void
ctk_spin_button_accessible_get_value_and_text (AtkValue  *obj,
                                         gdouble   *value,
                                         gchar    **text)
{
  GtkWidget *widget;
  GtkAdjustment *adjustment;

  widget = ctk_accessible_get_widget (GTK_ACCESSIBLE (obj));
  adjustment = ctk_spin_button_get_adjustment (GTK_SPIN_BUTTON (widget));
  if (adjustment == NULL)
    return;

  *value = ctk_adjustment_get_value (adjustment);
  *text = NULL;
}

static AtkRange *
ctk_spin_button_accessible_get_range (AtkValue *obj)
{
  GtkWidget *widget;
  GtkAdjustment *adjustment;

  widget = ctk_accessible_get_widget (GTK_ACCESSIBLE (obj));
  adjustment = ctk_spin_button_get_adjustment (GTK_SPIN_BUTTON (widget));
  if (adjustment == NULL)
    return NULL;

  return atk_range_new (ctk_adjustment_get_lower (adjustment),
                        ctk_adjustment_get_upper (adjustment),
                        NULL);
}

static void
ctk_spin_button_accessible_set_value (AtkValue      *obj,
                                const gdouble  value)
{
  GtkWidget *widget;
  GtkAdjustment *adjustment;

  widget = ctk_accessible_get_widget (GTK_ACCESSIBLE (obj));
  adjustment = ctk_spin_button_get_adjustment (GTK_SPIN_BUTTON (widget));
  if (adjustment == NULL)
    return;

  ctk_adjustment_set_value (adjustment, value);
}

static gdouble
ctk_spin_button_accessible_get_increment (AtkValue *obj)
{
  GtkWidget *widget;
  GtkAdjustment *adjustment;

  widget = ctk_accessible_get_widget (GTK_ACCESSIBLE (obj));
  adjustment = ctk_spin_button_get_adjustment (GTK_SPIN_BUTTON (widget));
  if (adjustment == NULL)
    return 0;

  return ctk_adjustment_get_minimum_increment (adjustment);
}

static void
atk_value_interface_init (AtkValueIface *iface)
{
  iface->get_current_value = ctk_spin_button_accessible_get_current_value;
  iface->get_maximum_value = ctk_spin_button_accessible_get_maximum_value;
  iface->get_minimum_value = ctk_spin_button_accessible_get_minimum_value;
  iface->get_minimum_increment = ctk_spin_button_accessible_get_minimum_increment;
  iface->set_current_value = ctk_spin_button_accessible_set_current_value;

  iface->get_value_and_text = ctk_spin_button_accessible_get_value_and_text;
  iface->get_range = ctk_spin_button_accessible_get_range;
  iface->set_value = ctk_spin_button_accessible_set_value;
  iface->get_increment = ctk_spin_button_accessible_get_increment;
}
