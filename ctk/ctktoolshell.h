/* CTK - The GIMP Toolkit
 * Copyright (C) 2007  Openismus GmbH
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
 *
 * Author:
 *   Mathias Hasselmann
 */

#ifndef __CTK_TOOL_SHELL_H__
#define __CTK_TOOL_SHELL_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkenums.h>
#include <pango/pango.h>
#include <ctk/ctksizegroup.h>


G_BEGIN_DECLS

#define CTK_TYPE_TOOL_SHELL            (ctk_tool_shell_get_type ())
#define CTK_TOOL_SHELL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_TOOL_SHELL, CtkToolShell))
#define CTK_IS_TOOL_SHELL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_TOOL_SHELL))
#define CTK_TOOL_SHELL_GET_IFACE(obj)  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), CTK_TYPE_TOOL_SHELL, CtkToolShellIface))

typedef struct _CtkToolShell           CtkToolShell; /* dummy typedef */
typedef struct _CtkToolShellIface      CtkToolShellIface;

/**
 * CtkToolShellIface:
 * @get_icon_size:        mandatory implementation of ctk_tool_shell_get_icon_size().
 * @get_orientation:      mandatory implementation of ctk_tool_shell_get_orientation().
 * @get_style:            mandatory implementation of ctk_tool_shell_get_style().
 * @get_relief_style:     optional implementation of ctk_tool_shell_get_relief_style().
 * @rebuild_menu:         optional implementation of ctk_tool_shell_rebuild_menu().
 * @get_text_orientation: optional implementation of ctk_tool_shell_get_text_orientation().
 * @get_text_alignment:   optional implementation of ctk_tool_shell_get_text_alignment().
 * @get_ellipsize_mode:   optional implementation of ctk_tool_shell_get_ellipsize_mode().
 * @get_text_size_group:  optional implementation of ctk_tool_shell_get_text_size_group().
 *
 * Virtual function table for the #CtkToolShell interface.
 */
struct _CtkToolShellIface
{
  /*< private >*/
  GTypeInterface g_iface;

  /*< public >*/
  CtkIconSize        (*get_icon_size)        (CtkToolShell *shell);
  CtkOrientation     (*get_orientation)      (CtkToolShell *shell);
  CtkToolbarStyle    (*get_style)            (CtkToolShell *shell);
  CtkReliefStyle     (*get_relief_style)     (CtkToolShell *shell);
  void               (*rebuild_menu)         (CtkToolShell *shell);
  CtkOrientation     (*get_text_orientation) (CtkToolShell *shell);
  gfloat             (*get_text_alignment)   (CtkToolShell *shell);
  PangoEllipsizeMode (*get_ellipsize_mode)   (CtkToolShell *shell);
  CtkSizeGroup *     (*get_text_size_group)  (CtkToolShell *shell);
};

CDK_AVAILABLE_IN_ALL
GType              ctk_tool_shell_get_type             (void) G_GNUC_CONST;

CDK_AVAILABLE_IN_ALL
CtkIconSize        ctk_tool_shell_get_icon_size        (CtkToolShell *shell);
CDK_AVAILABLE_IN_ALL
CtkOrientation     ctk_tool_shell_get_orientation      (CtkToolShell *shell);
CDK_AVAILABLE_IN_ALL
CtkToolbarStyle    ctk_tool_shell_get_style            (CtkToolShell *shell);
CDK_AVAILABLE_IN_ALL
CtkReliefStyle     ctk_tool_shell_get_relief_style     (CtkToolShell *shell);
CDK_AVAILABLE_IN_ALL
void               ctk_tool_shell_rebuild_menu         (CtkToolShell *shell);
CDK_AVAILABLE_IN_ALL
CtkOrientation     ctk_tool_shell_get_text_orientation (CtkToolShell *shell);
CDK_AVAILABLE_IN_ALL
gfloat             ctk_tool_shell_get_text_alignment   (CtkToolShell *shell);
CDK_AVAILABLE_IN_ALL
PangoEllipsizeMode ctk_tool_shell_get_ellipsize_mode   (CtkToolShell *shell);
CDK_AVAILABLE_IN_ALL
CtkSizeGroup *     ctk_tool_shell_get_text_size_group  (CtkToolShell *shell);

G_END_DECLS

#endif /* __CTK_TOOL_SHELL_H__ */
