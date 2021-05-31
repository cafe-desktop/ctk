/* GTK+ - accessibility implementations
 * Copyright 2001 Sun Microsystems Inc.
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
#include "ctkrenderercellaccessible.h"
#include "ctkintl.h"

struct _CtkRendererCellAccessiblePrivate
{
  CtkCellRenderer *renderer;
};

enum {
  PROP_0,
  PROP_RENDERER
};

G_DEFINE_TYPE_WITH_PRIVATE (CtkRendererCellAccessible, ctk_renderer_cell_accessible, CTK_TYPE_CELL_ACCESSIBLE)

static void
ctk_renderer_cell_accessible_set_property (GObject         *object,
                                           guint            prop_id,
                                           const GValue    *value,
                                           GParamSpec      *pspec)
{
  CtkRendererCellAccessible *accessible = CTK_RENDERER_CELL_ACCESSIBLE (object);

  switch (prop_id)
    {
    case PROP_RENDERER:
      accessible->priv->renderer = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_renderer_cell_accessible_get_property (GObject         *object,
                                           guint            prop_id,
                                           GValue          *value,
                                           GParamSpec      *pspec)
{
  CtkRendererCellAccessible *accessible = CTK_RENDERER_CELL_ACCESSIBLE (object);

  switch (prop_id)
    {
    case PROP_RENDERER:
      g_value_set_object (value, accessible->priv->renderer);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_renderer_cell_accessible_finalize (GObject *object)
{
  CtkRendererCellAccessible *renderer_cell = CTK_RENDERER_CELL_ACCESSIBLE (object);

  if (renderer_cell->priv->renderer)
    g_object_unref (renderer_cell->priv->renderer);

  G_OBJECT_CLASS (ctk_renderer_cell_accessible_parent_class)->finalize (object);
}

static void
ctk_renderer_cell_accessible_class_init (CtkRendererCellAccessibleClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->get_property = ctk_renderer_cell_accessible_get_property;
  gobject_class->set_property = ctk_renderer_cell_accessible_set_property;
  gobject_class->finalize = ctk_renderer_cell_accessible_finalize;

  g_object_class_install_property (gobject_class,
				   PROP_RENDERER,
				   g_param_spec_object ("renderer",
							P_("Cell renderer"),
							P_("The cell renderer represented by this accessible"),
							CTK_TYPE_CELL_RENDERER,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
ctk_renderer_cell_accessible_init (CtkRendererCellAccessible *renderer_cell)
{
  renderer_cell->priv = ctk_renderer_cell_accessible_get_instance_private (renderer_cell);
}

AtkObject *
ctk_renderer_cell_accessible_new (CtkCellRenderer *renderer)
{
  AtkObject *object;

  g_return_val_if_fail (CTK_IS_CELL_RENDERER (renderer), NULL);

  object = g_object_new (_ctk_cell_renderer_get_accessible_type (renderer),
                         "renderer", renderer,
                         NULL);

  atk_object_set_role (object, ATK_ROLE_TABLE_CELL);

  return object;
}
