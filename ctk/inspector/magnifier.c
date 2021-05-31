/*
 * Copyright (c) 2014 Red Hat, Inc.
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
#include <glib/gi18n-lib.h>

#include "magnifier.h"

#include "ctkmagnifierprivate.h"

#include "ctklabel.h"
#include "ctkadjustment.h"

enum
{
  PROP_0,
  PROP_ADJUSTMENT
};

struct _GtkInspectorMagnifierPrivate
{
  GtkWidget *object;
  GtkWidget *magnifier;
  GtkAdjustment *adjustment;
};

G_DEFINE_TYPE_WITH_PRIVATE (GtkInspectorMagnifier, ctk_inspector_magnifier, CTK_TYPE_BOX)

static void
ctk_inspector_magnifier_init (GtkInspectorMagnifier *sl)
{
  sl->priv = ctk_inspector_magnifier_get_instance_private (sl);
  ctk_widget_init_template (CTK_WIDGET (sl));
}

void
ctk_inspector_magnifier_set_object (GtkInspectorMagnifier *sl,
                                    GObject              *object)
{
  sl->priv->object = NULL;

  if (!CTK_IS_WIDGET (object) || !ctk_widget_is_visible (CTK_WIDGET (object)))
    {
      ctk_widget_hide (CTK_WIDGET (sl));
      _ctk_magnifier_set_inspected (CTK_MAGNIFIER (sl->priv->magnifier), NULL);
      return;
    }

  ctk_widget_show (CTK_WIDGET (sl));

  sl->priv->object = CTK_WIDGET (object);

  _ctk_magnifier_set_inspected (CTK_MAGNIFIER (sl->priv->magnifier), CTK_WIDGET (object));
  _ctk_magnifier_set_coords (CTK_MAGNIFIER (sl->priv->magnifier), 0, 0);
}

static void
get_property (GObject    *object,
              guint       param_id,
              GValue     *value,
              GParamSpec *pspec)
{
  GtkInspectorMagnifier *sl = CTK_INSPECTOR_MAGNIFIER (object);

  switch (param_id)
    {
    case PROP_ADJUSTMENT:
      g_value_take_object (value, sl->priv->adjustment);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
set_property (GObject      *object,
              guint         param_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GtkInspectorMagnifier *sl = CTK_INSPECTOR_MAGNIFIER (object);

  switch (param_id)
    {
    case PROP_ADJUSTMENT:
      sl->priv->adjustment = g_value_get_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
constructed (GObject *object)
{
  GtkInspectorMagnifier *sl = CTK_INSPECTOR_MAGNIFIER (object);

  g_object_bind_property (sl->priv->adjustment, "value",
                          sl->priv->magnifier, "magnification",
                          G_BINDING_SYNC_CREATE);
}

static void
ctk_inspector_magnifier_class_init (GtkInspectorMagnifierClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);

  object_class->get_property = get_property;
  object_class->set_property = set_property;
  object_class->constructed = constructed;

  g_object_class_install_property (object_class, PROP_ADJUSTMENT,
      g_param_spec_object ("adjustment", NULL, NULL,
                           CTK_TYPE_ADJUSTMENT, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  ctk_widget_class_set_template_from_resource (widget_class, "/org/ctk/libctk/inspector/magnifier.ui");
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorMagnifier, magnifier);
}

// vim: set et sw=2 ts=2:
