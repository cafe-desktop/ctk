/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * ctkorientable.c
 * Copyright (C) 2008 Imendio AB
 * Contact: Michael Natterer <mitch@imendio.com>
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

#include "ctkorientableprivate.h"
#include "ctkprivate.h"
#include "ctktypebuiltins.h"
#include "ctkintl.h"


/**
 * SECTION:ctkorientable
 * @Short_description: An interface for flippable widgets
 * @Title: GtkOrientable
 *
 * The #GtkOrientable interface is implemented by all widgets that can be
 * oriented horizontally or vertically. Historically, such widgets have been
 * realized as subclasses of a common base class (e.g #GtkBox/#GtkHBox/#GtkVBox
 * or #GtkScale/#GtkHScale/#GtkVScale). #GtkOrientable is more flexible in that
 * it allows the orientation to be changed at runtime, allowing the widgets
 * to “flip”.
 *
 * #GtkOrientable was introduced in GTK+ 2.16.
 */


typedef GtkOrientableIface GtkOrientableInterface;
G_DEFINE_INTERFACE (GtkOrientable, ctk_orientable, G_TYPE_OBJECT)

static void
ctk_orientable_default_init (GtkOrientableInterface *iface)
{
  /**
   * GtkOrientable:orientation:
   *
   * The orientation of the orientable.
   *
   * Since: 2.16
   **/
  g_object_interface_install_property (iface,
                                       g_param_spec_enum ("orientation",
                                                          P_("Orientation"),
                                                          P_("The orientation of the orientable"),
                                                          CTK_TYPE_ORIENTATION,
                                                          CTK_ORIENTATION_HORIZONTAL,
                                                          CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));
}

/**
 * ctk_orientable_set_orientation:
 * @orientable: a #GtkOrientable
 * @orientation: the orientable’s new orientation.
 *
 * Sets the orientation of the @orientable.
 *
 * Since: 2.16
 **/
void
ctk_orientable_set_orientation (GtkOrientable  *orientable,
                                GtkOrientation  orientation)
{
  g_return_if_fail (CTK_IS_ORIENTABLE (orientable));

  g_object_set (orientable,
                "orientation", orientation,
                NULL);

  if (CTK_IS_WIDGET (orientable))
    _ctk_orientable_set_style_classes (orientable);
}

/**
 * ctk_orientable_get_orientation:
 * @orientable: a #GtkOrientable
 *
 * Retrieves the orientation of the @orientable.
 *
 * Returns: the orientation of the @orientable.
 *
 * Since: 2.16
 **/
GtkOrientation
ctk_orientable_get_orientation (GtkOrientable *orientable)
{
  GtkOrientation orientation;

  g_return_val_if_fail (CTK_IS_ORIENTABLE (orientable),
                        CTK_ORIENTATION_HORIZONTAL);

  g_object_get (orientable,
                "orientation", &orientation,
                NULL);

  return orientation;
}

void
_ctk_orientable_set_style_classes (GtkOrientable *orientable)
{
  GtkStyleContext *context;
  GtkOrientation orientation;

  g_return_if_fail (CTK_IS_ORIENTABLE (orientable));
  g_return_if_fail (CTK_IS_WIDGET (orientable));

  context = ctk_widget_get_style_context (CTK_WIDGET (orientable));
  orientation = ctk_orientable_get_orientation (orientable);

  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    {
      ctk_style_context_add_class (context, CTK_STYLE_CLASS_HORIZONTAL);
      ctk_style_context_remove_class (context, CTK_STYLE_CLASS_VERTICAL);
    }
  else
    {
      ctk_style_context_add_class (context, CTK_STYLE_CLASS_VERTICAL);
      ctk_style_context_remove_class (context, CTK_STYLE_CLASS_HORIZONTAL);
    }
}
