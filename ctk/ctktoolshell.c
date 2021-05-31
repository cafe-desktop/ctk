/* ctktoolshell.c
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
#include "ctktoolshell.h"
#include "ctkwidget.h"
#include "ctkintl.h"


/**
 * SECTION:ctktoolshell
 * @Short_description: Interface for containers containing CtkToolItem widgets
 * @Title: CtkToolShell
 * @see_also: #CtkToolbar, #CtkToolItem
 *
 * The #CtkToolShell interface allows container widgets to provide additional
 * information when embedding #CtkToolItem widgets.
 */

/**
 * CtkToolShell:
 *
 * Dummy structure for accessing instances of #CtkToolShellIface.
 */


typedef CtkToolShellIface CtkToolShellInterface;
G_DEFINE_INTERFACE (CtkToolShell, ctk_tool_shell, CTK_TYPE_WIDGET);

static CtkReliefStyle ctk_tool_shell_real_get_relief_style (CtkToolShell *shell);
static CtkOrientation ctk_tool_shell_real_get_text_orientation (CtkToolShell *shell);
static gfloat ctk_tool_shell_real_get_text_alignment (CtkToolShell *shell);
static PangoEllipsizeMode ctk_tool_shell_real_get_ellipsize_mode (CtkToolShell *shell);

static void
ctk_tool_shell_default_init (CtkToolShellInterface *iface)
{
  iface->get_relief_style = ctk_tool_shell_real_get_relief_style;
  iface->get_text_orientation = ctk_tool_shell_real_get_text_orientation;
  iface->get_text_alignment = ctk_tool_shell_real_get_text_alignment;
  iface->get_ellipsize_mode = ctk_tool_shell_real_get_ellipsize_mode;
}

static CtkReliefStyle
ctk_tool_shell_real_get_relief_style (CtkToolShell *shell)
{
  return CTK_RELIEF_NONE;
}

static CtkOrientation
ctk_tool_shell_real_get_text_orientation (CtkToolShell *shell)
{
  return CTK_ORIENTATION_HORIZONTAL;
}

static gfloat
ctk_tool_shell_real_get_text_alignment (CtkToolShell *shell)
{
  return 0.5f;
}

static PangoEllipsizeMode
ctk_tool_shell_real_get_ellipsize_mode (CtkToolShell *shell)
{
  return PANGO_ELLIPSIZE_NONE;
}


/**
 * ctk_tool_shell_get_icon_size:
 * @shell: a #CtkToolShell
 *
 * Retrieves the icon size for the tool shell. Tool items must not call this
 * function directly, but rely on ctk_tool_item_get_icon_size() instead.
 *
 * Returns: (type int): the current size (#CtkIconSize) for icons of @shell
 *
 * Since: 2.14
 **/
CtkIconSize
ctk_tool_shell_get_icon_size (CtkToolShell *shell)
{
  return CTK_TOOL_SHELL_GET_IFACE (shell)->get_icon_size (shell);
}

/**
 * ctk_tool_shell_get_orientation:
 * @shell: a #CtkToolShell
 *
 * Retrieves the current orientation for the tool shell. Tool items must not
 * call this function directly, but rely on ctk_tool_item_get_orientation()
 * instead.
 *
 * Returns: the current orientation of @shell
 *
 * Since: 2.14
 **/
CtkOrientation
ctk_tool_shell_get_orientation (CtkToolShell *shell)
{
  return CTK_TOOL_SHELL_GET_IFACE (shell)->get_orientation (shell);
}

/**
 * ctk_tool_shell_get_style:
 * @shell: a #CtkToolShell
 *
 * Retrieves whether the tool shell has text, icons, or both. Tool items must
 * not call this function directly, but rely on ctk_tool_item_get_toolbar_style()
 * instead.
 *
 * Returns: the current style of @shell
 *
 * Since: 2.14
 **/
CtkToolbarStyle
ctk_tool_shell_get_style (CtkToolShell *shell)
{
  return CTK_TOOL_SHELL_GET_IFACE (shell)->get_style (shell);
}

/**
 * ctk_tool_shell_get_relief_style:
 * @shell: a #CtkToolShell
 *
 * Returns the relief style of buttons on @shell. Tool items must not call this
 * function directly, but rely on ctk_tool_item_get_relief_style() instead.
 *
 * Returns: The relief style of buttons on @shell.
 *
 * Since: 2.14
 **/
CtkReliefStyle
ctk_tool_shell_get_relief_style (CtkToolShell *shell)
{
  CtkToolShellIface *iface = CTK_TOOL_SHELL_GET_IFACE (shell);

  return iface->get_relief_style (shell);
}

/**
 * ctk_tool_shell_rebuild_menu:
 * @shell: a #CtkToolShell
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
ctk_tool_shell_rebuild_menu (CtkToolShell *shell)
{
  CtkToolShellIface *iface = CTK_TOOL_SHELL_GET_IFACE (shell);

  if (iface->rebuild_menu)
    iface->rebuild_menu (shell);
}

/**
 * ctk_tool_shell_get_text_orientation:
 * @shell: a #CtkToolShell
 *
 * Retrieves the current text orientation for the tool shell. Tool items must not
 * call this function directly, but rely on ctk_tool_item_get_text_orientation()
 * instead.
 *
 * Returns: the current text orientation of @shell
 *
 * Since: 2.20
 **/
CtkOrientation
ctk_tool_shell_get_text_orientation (CtkToolShell *shell)
{
  CtkToolShellIface *iface = CTK_TOOL_SHELL_GET_IFACE (shell);

  return iface->get_text_orientation (shell);
}

/**
 * ctk_tool_shell_get_text_alignment:
 * @shell: a #CtkToolShell
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
ctk_tool_shell_get_text_alignment (CtkToolShell *shell)
{
  CtkToolShellIface *iface = CTK_TOOL_SHELL_GET_IFACE (shell);

  return iface->get_text_alignment (shell);
}

/**
 * ctk_tool_shell_get_ellipsize_mode:
 * @shell: a #CtkToolShell
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
ctk_tool_shell_get_ellipsize_mode (CtkToolShell *shell)
{
  CtkToolShellIface *iface = CTK_TOOL_SHELL_GET_IFACE (shell);

  return iface->get_ellipsize_mode (shell);
}

/**
 * ctk_tool_shell_get_text_size_group:
 * @shell: a #CtkToolShell
 *
 * Retrieves the current text size group for the tool shell. Tool items must not
 * call this function directly, but rely on ctk_tool_item_get_text_size_group()
 * instead.
 *
 * Returns: (transfer none): the current text size group of @shell
 *
 * Since: 2.20
 **/
CtkSizeGroup *
ctk_tool_shell_get_text_size_group (CtkToolShell *shell)
{
  CtkToolShellIface *iface = CTK_TOOL_SHELL_GET_IFACE (shell);

  if (iface->get_text_size_group)
    return CTK_TOOL_SHELL_GET_IFACE (shell)->get_text_size_group (shell);

  return NULL;
}
