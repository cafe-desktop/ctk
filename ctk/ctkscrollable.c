/* ctkscrollable.c
 * Copyright (C) 2008 Tadej Borovšak <tadeboro@gmail.com>
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

/**
 * SECTION:ctkscrollable
 * @Short_Description: An interface for scrollable widgets
 * @Title: CtkScrollable
 *
 * #CtkScrollable is an interface that is implemented by widgets with native
 * scrolling ability.
 *
 * To implement this interface you should override the
 * #CtkScrollable:hadjustment and #CtkScrollable:vadjustment properties.
 *
 * ## Creating a scrollable widget
 *
 * All scrollable widgets should do the following.
 *
 * - When a parent widget sets the scrollable child widget’s adjustments,
 *   the widget should populate the adjustments’
 *   #CtkAdjustment:lower, #CtkAdjustment:upper,
 *   #CtkAdjustment:step-increment, #CtkAdjustment:page-increment and
 *   #CtkAdjustment:page-size properties and connect to the
 *   #CtkAdjustment::value-changed signal.
 *
 * - Because its preferred size is the size for a fully expanded widget,
 *   the scrollable widget must be able to cope with underallocations.
 *   This means that it must accept any value passed to its
 *   #CtkWidgetClass.size_allocate() function.
 *
 * - When the parent allocates space to the scrollable child widget,
 *   the widget should update the adjustments’ properties with new values.
 *
 * - When any of the adjustments emits the #CtkAdjustment::value-changed signal,
 *   the scrollable widget should scroll its contents.
 */

#include "config.h"

#include "ctkscrollable.h"

#include "ctkadjustment.h"
#include "ctkprivate.h"
#include "ctktypebuiltins.h"
#include "ctkintl.h"

G_DEFINE_INTERFACE (CtkScrollable, ctk_scrollable, G_TYPE_OBJECT)

static void
ctk_scrollable_default_init (CtkScrollableInterface *iface)
{
  GParamSpec *pspec;

  /**
   * CtkScrollable:hadjustment:
   *
   * Horizontal #CtkAdjustment of the scrollable widget. This adjustment is
   * shared between the scrollable widget and its parent.
   *
   * Since: 3.0
   */
  pspec = g_param_spec_object ("hadjustment",
                               P_("Horizontal adjustment"),
                               P_("Horizontal adjustment that is shared "
                                  "between the scrollable widget and its "
                                  "controller"),
                               CTK_TYPE_ADJUSTMENT,
                               CTK_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_interface_install_property (iface, pspec);

  /**
   * CtkScrollable:vadjustment:
   *
   * Verical #CtkAdjustment of the scrollable widget. This adjustment is shared
   * between the scrollable widget and its parent.
   *
   * Since: 3.0
   */
  pspec = g_param_spec_object ("vadjustment",
                               P_("Vertical adjustment"),
                               P_("Vertical adjustment that is shared "
                                  "between the scrollable widget and its "
                                  "controller"),
                               CTK_TYPE_ADJUSTMENT,
                               CTK_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_interface_install_property (iface, pspec);

  /**
   * CtkScrollable:hscroll-policy:
   *
   * Determines whether horizontal scrolling should start once the scrollable
   * widget is allocated less than its minimum width or less than its natural width.
   *
   * Since: 3.0
   */
  pspec = g_param_spec_enum ("hscroll-policy",
			     P_("Horizontal Scrollable Policy"),
			     P_("How the size of the content should be determined"),
			     CTK_TYPE_SCROLLABLE_POLICY,
			     CTK_SCROLL_MINIMUM,
			     CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);
  g_object_interface_install_property (iface, pspec);

  /**
   * CtkScrollable:vscroll-policy:
   *
   * Determines whether vertical scrolling should start once the scrollable
   * widget is allocated less than its minimum height or less than its natural height.
   *
   * Since: 3.0
   */
  pspec = g_param_spec_enum ("vscroll-policy",
			     P_("Vertical Scrollable Policy"),
			     P_("How the size of the content should be determined"),
			     CTK_TYPE_SCROLLABLE_POLICY,
			     CTK_SCROLL_MINIMUM,
			     CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);
  g_object_interface_install_property (iface, pspec);
}

/**
 * ctk_scrollable_get_hadjustment:
 * @scrollable: a #CtkScrollable
 *
 * Retrieves the #CtkAdjustment used for horizontal scrolling.
 *
 * Returns: (transfer none): horizontal #CtkAdjustment.
 *
 * Since: 3.0
 **/
CtkAdjustment *
ctk_scrollable_get_hadjustment (CtkScrollable *scrollable)
{
  CtkAdjustment *adj = NULL;

  g_return_val_if_fail (CTK_IS_SCROLLABLE (scrollable), NULL);

  g_object_get (scrollable, "hadjustment", &adj, NULL);

  /* Horrid hack; g_object_get() returns a new reference but
   * that contradicts the memory management conventions
   * for accessors.
   */
  if (adj)
    g_object_unref (adj);

  return adj;
}

/**
 * ctk_scrollable_set_hadjustment:
 * @scrollable: a #CtkScrollable
 * @hadjustment: (allow-none): a #CtkAdjustment
 *
 * Sets the horizontal adjustment of the #CtkScrollable.
 *
 * Since: 3.0
 **/
void
ctk_scrollable_set_hadjustment (CtkScrollable *scrollable,
                                CtkAdjustment *hadjustment)
{
  g_return_if_fail (CTK_IS_SCROLLABLE (scrollable));
  g_return_if_fail (hadjustment == NULL || CTK_IS_ADJUSTMENT (hadjustment));

  g_object_set (scrollable, "hadjustment", hadjustment, NULL);
}

/**
 * ctk_scrollable_get_vadjustment:
 * @scrollable: a #CtkScrollable
 *
 * Retrieves the #CtkAdjustment used for vertical scrolling.
 *
 * Returns: (transfer none): vertical #CtkAdjustment.
 *
 * Since: 3.0
 **/
CtkAdjustment *
ctk_scrollable_get_vadjustment (CtkScrollable *scrollable)
{
  CtkAdjustment *adj = NULL;

  g_return_val_if_fail (CTK_IS_SCROLLABLE (scrollable), NULL);

  g_object_get (scrollable, "vadjustment", &adj, NULL);

  /* Horrid hack; g_object_get() returns a new reference but
   * that contradicts the memory management conventions
   * for accessors.
   */
  if (adj)
    g_object_unref (adj);

  return adj;
}

/**
 * ctk_scrollable_set_vadjustment:
 * @scrollable: a #CtkScrollable
 * @vadjustment: (allow-none): a #CtkAdjustment
 *
 * Sets the vertical adjustment of the #CtkScrollable.
 *
 * Since: 3.0
 **/
void
ctk_scrollable_set_vadjustment (CtkScrollable *scrollable,
                                CtkAdjustment *vadjustment)
{
  g_return_if_fail (CTK_IS_SCROLLABLE (scrollable));
  g_return_if_fail (vadjustment == NULL || CTK_IS_ADJUSTMENT (vadjustment));

  g_object_set (scrollable, "vadjustment", vadjustment, NULL);
}


/**
 * ctk_scrollable_get_hscroll_policy:
 * @scrollable: a #CtkScrollable
 *
 * Gets the horizontal #CtkScrollablePolicy.
 *
 * Returns: The horizontal #CtkScrollablePolicy.
 *
 * Since: 3.0
 **/
CtkScrollablePolicy
ctk_scrollable_get_hscroll_policy (CtkScrollable *scrollable)
{
  CtkScrollablePolicy policy;

  g_return_val_if_fail (CTK_IS_SCROLLABLE (scrollable), CTK_SCROLL_MINIMUM);

  g_object_get (scrollable, "hscroll-policy", &policy, NULL);

  return policy;
}

/**
 * ctk_scrollable_set_hscroll_policy:
 * @scrollable: a #CtkScrollable
 * @policy: the horizontal #CtkScrollablePolicy
 *
 * Sets the #CtkScrollablePolicy to determine whether
 * horizontal scrolling should start below the minimum width or
 * below the natural width.
 *
 * Since: 3.0
 **/
void
ctk_scrollable_set_hscroll_policy (CtkScrollable       *scrollable,
				   CtkScrollablePolicy  policy)
{
  g_return_if_fail (CTK_IS_SCROLLABLE (scrollable));

  g_object_set (scrollable, "hscroll-policy", policy, NULL);
}

/**
 * ctk_scrollable_get_vscroll_policy:
 * @scrollable: a #CtkScrollable
 *
 * Gets the vertical #CtkScrollablePolicy.
 *
 * Returns: The vertical #CtkScrollablePolicy.
 *
 * Since: 3.0
 **/
CtkScrollablePolicy
ctk_scrollable_get_vscroll_policy (CtkScrollable *scrollable)
{
  CtkScrollablePolicy policy;

  g_return_val_if_fail (CTK_IS_SCROLLABLE (scrollable), CTK_SCROLL_MINIMUM);

  g_object_get (scrollable, "vscroll-policy", &policy, NULL);

  return policy;
}

/**
 * ctk_scrollable_set_vscroll_policy:
 * @scrollable: a #CtkScrollable
 * @policy: the vertical #CtkScrollablePolicy
 *
 * Sets the #CtkScrollablePolicy to determine whether
 * vertical scrolling should start below the minimum height or
 * below the natural height.
 *
 * Since: 3.0
 **/
void
ctk_scrollable_set_vscroll_policy (CtkScrollable       *scrollable,
				   CtkScrollablePolicy  policy)
{
  g_return_if_fail (CTK_IS_SCROLLABLE (scrollable));

  g_object_set (scrollable, "vscroll-policy", policy, NULL);
}

/**
 * ctk_scrollable_get_border:
 * @scrollable: a #CtkScrollable
 * @border: (out caller-allocates): return location for the results
 *
 * Returns the size of a non-scrolling border around the
 * outside of the scrollable. An example for this would
 * be treeview headers. CTK+ can use this information to
 * display overlayed graphics, like the overshoot indication,
 * at the right position.
 *
 * Returns: %TRUE if @border has been set
 *
 * Since: 3.16
 */
gboolean
ctk_scrollable_get_border (CtkScrollable *scrollable,
                           CtkBorder     *border)
{
  g_return_val_if_fail (CTK_IS_SCROLLABLE (scrollable), FALSE);
  g_return_val_if_fail (border != NULL, FALSE);

  if (CTK_SCROLLABLE_GET_IFACE (scrollable)->get_border)
    return CTK_SCROLLABLE_GET_IFACE (scrollable)->get_border (scrollable, border);

  return FALSE;
}
