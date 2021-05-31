/* gtktoolshell.c
 * Copyright (C) 2007  Openismus GmbH
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author:
 *   Mathias Hasselmann
 */

#include "config.h"
#include "gtktoolshell.h"
#include "gtkwidget.h"
#include "gtkintl.h"


/**
 * SECTION:gtktoolshell
 * @Short_description: Interface for containers containing GtkToolItem widgets
 * @Title: GtkToolShell
 * @see_also: #GtkToolbar, #GtkToolItem
 *
 * The #GtkToolShell interface allows container widgets to provide additional
 * information when embedding #GtkToolItem widgets.
 */

/**
 * GtkToolShell:
 *
 * Dummy structure for accessing instances of #GtkToolShellIface.
 */


typedef GtkToolShellIface GtkToolShellInterface;
G_DEFINE_INTERFACE (GtkToolShell, ctk_tool_shell, GTK_TYPE_WIDGET);

static GtkReliefStyle ctk_tool_shell_real_get_relief_style (GtkToolShell *shell);
static GtkOrientation ctk_tool_shell_real_get_text_orientation (GtkToolShell *shell);
static gfloat ctk_tool_shell_real_get_text_alignment (GtkToolShell *shell);
static PangoEllipsizeMode ctk_tool_shell_real_get_ellipsize_mode (GtkToolShell *shell);

static void
ctk_tool_shell_default_init (GtkToolShellInterface *iface)
{
  iface->get_relief_style = ctk_tool_shell_real_get_relief_style;
  iface->get_text_orientation = ctk_tool_shell_real_get_text_orientation;
  iface->get_text_alignment = ctk_tool_shell_real_get_text_alignment;
  iface->get_ellipsize_mode = ctk_tool_shell_real_get_ellipsize_mode;
}

static GtkReliefStyle
ctk_tool_shell_real_get_relief_style (GtkToolShell *shell)
{
  return GTK_RELIEF_NONE;
}

static GtkOrientation
ctk_tool_shell_real_get_text_orientation (GtkToolShell *shell)
{
  return GTK_ORIENTATION_HORIZONTAL;
}

static gfloat
ctk_tool_shell_real_get_text_alignment (GtkToolShell *shell)
{
  return 0.5f;
}

static PangoEllipsizeMode
ctk_tool_shell_real_get_ellipsize_mode (GtkToolShell *shell)
{
  return PANGO_ELLIPSIZE_NONE;
}


/**
 * ctk_tool_shell_get_icon_size:
 * @shell: a #GtkToolShell
 *
 * Retrieves the icon size for the tool shell. Tool items must not call this
 * function directly, but rely on ctk_tool_item_get_icon_size() instead.
 *
 * Returns: (type int): the current size (#GtkIconSize) for icons of @shell
 *
 * Since: 2.14
 **/
GtkIconSize
ctk_tool_shell_get_icon_size (GtkToolShell *shell)
{
  return GTK_TOOL_SHELL_GET_IFACE (shell)->get_icon_size (shell);
}

/**
 * ctk_tool_shell_get_orientation:
 * @shell: a #GtkToolShell
 *
 * Retrieves the current orientation for the tool shell. Tool items must not
 * call this function directly, but rely on ctk_tool_item_get_orientation()
 * instead.
 *
 * Returns: the current orientation of @shell
 *
 * Since: 2.14
 **/
GtkOrientation
ctk_tool_shell_get_orientation (GtkToolShell *shell)
{
  return GTK_TOOL_SHELL_GET_IFACE (shell)->get_orientation (shell);
}

/**
 * ctk_tool_shell_get_style:
 * @shell: a #GtkToolShell
 *
 * Retrieves whether the tool shell has text, icons, or both. Tool items must
 * not call this function directly, but rely on ctk_tool_item_get_toolbar_style()
 * instead.
 *
 * Returns: the current style of @shell
 *
 * Since: 2.14
 **/
GtkToolbarStyle
ctk_tool_shell_get_style (GtkToolShell *shell)
{
  return GTK_TOOL_SHELL_GET_IFACE (shell)->get_style (shell);
}

/**
 * ctk_tool_shell_get_relief_style:
 * @shell: a #GtkToolShell
 *
 * Returns the relief style of buttons on @shell. Tool items must not call this
 * function directly, but rely on ctk_tool_item_get_relief_style() instead.
 *
 * Returns: The relief style of buttons on @shell.
 *
 * Since: 2.14
 **/
GtkReliefStyle
ctk_tool_shell_get_relief_style (GtkToolShell *shell)
{
  GtkToolShellIface *iface = GTK_TOOL_SHELL_GET_IFACE (shell);

  return iface->get_relief_style (shell);
}

/**
 * ctk_tool_shell_rebuild_menu:
 * @shell: a #GtkToolShell
 *
 * Calling this function signals the tool shell that the overflow menu item for
 * tool items have changed. If there is an overflow menu and if it is visible
 * when this function it called, the menu will be rebuilt.
 *
 * Tool items must not call this function directly, but rely on
 * ctk_tool_item_rebuild_menu() instead.
 *
 * Since: 2.14
 **/
void
ctk_tool_shell_rebuild_menu (GtkToolShell *shell)
{
  GtkToolShellIface *iface = GTK_TOOL_SHELL_GET_IFACE (shell);

  if (iface->rebuild_menu)
    iface->rebuild_menu (shell);
}

/**
 * ctk_tool_shell_get_text_orientation:
 * @shell: a #GtkToolShell
 *
 * Retrieves the current text orientation for the tool shell. Tool items must not
 * call this function directly, but rely on ctk_tool_item_get_text_orientation()
 * instead.
 *
 * Returns: the current text orientation of @shell
 *
 * Since: 2.20
 **/
GtkOrientation
ctk_tool_shell_get_text_orientation (GtkToolShell *shell)
{
  GtkToolShellIface *iface = GTK_TOOL_SHELL_GET_IFACE (shell);

  return iface->get_text_orientation (shell);
}

/**
 * ctk_tool_shell_get_text_alignment:
 * @shell: a #GtkToolShell
 *
 * Retrieves the current text alignment for the tool shell. Tool items must not
 * call this function directly, but rely on ctk_tool_item_get_text_alignment()
 * instead.
 *
 * Returns: the current text alignment of @shell
 *
 * Since: 2.20
 **/
gfloat
ctk_tool_shell_get_text_alignment (GtkToolShell *shell)
{
  GtkToolShellIface *iface = GTK_TOOL_SHELL_GET_IFACE (shell);

  return iface->get_text_alignment (shell);
}

/**
 * ctk_tool_shell_get_ellipsize_mode:
 * @shell: a #GtkToolShell
 *
 * Retrieves the current ellipsize mode for the tool shell. Tool items must not
 * call this function directly, but rely on ctk_tool_item_get_ellipsize_mode()
 * instead.
 *
 * Returns: the current ellipsize mode of @shell
 *
 * Since: 2.20
 **/
PangoEllipsizeMode
ctk_tool_shell_get_ellipsize_mode (GtkToolShell *shell)
{
  GtkToolShellIface *iface = GTK_TOOL_SHELL_GET_IFACE (shell);

  return iface->get_ellipsize_mode (shell);
}

/**
 * ctk_tool_shell_get_text_size_group:
 * @shell: a #GtkToolShell
 *
 * Retrieves the current text size group for the tool shell. Tool items must not
 * call this function directly, but rely on ctk_tool_item_get_text_size_group()
 * instead.
 *
 * Returns: (transfer none): the current text size group of @shell
 *
 * Since: 2.20
 **/
GtkSizeGroup *
ctk_tool_shell_get_text_size_group (GtkToolShell *shell)
{
  GtkToolShellIface *iface = GTK_TOOL_SHELL_GET_IFACE (shell);

  if (iface->get_text_size_group)
    return GTK_TOOL_SHELL_GET_IFACE (shell)->get_text_size_group (shell);

  return NULL;
}
