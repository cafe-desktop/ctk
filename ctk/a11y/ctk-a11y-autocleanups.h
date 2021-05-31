/* CTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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

#if !defined (__CTK_A11Y_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk-a11y.h> can be included directly."
#endif

#ifndef __GI_SCANNER__

G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkArrowAccessible, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkBooleanCellAccessible, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkButtonAccessible, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkCellAccessible, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkCellAccessibleParent, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkCheckMenuItemAccessible, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkComboBoxAccessible, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkContainerAccessible, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkContainerCellAccessible, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkEntryAccessible, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkExpanderAccessible, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkFlowBoxAccessible, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkFlowBoxChildAccessible, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkFrameAccessible, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkIconViewAccessible, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkImageAccessible, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkImageCellAccessible, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkLabelAccessible, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkLevelBarAccessible, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkLinkButtonAccessible, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkListBoxAccessible, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkListBoxRowAccessible, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkLockButtonAccessible, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkMenuAccessible, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkMenuButtonAccessible, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkMenuItemAccessible, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkMenuShellAccessible, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkNotebookAccessible, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkNotebookPageAccessible, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkPanedAccessible, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkPopoverAccessible, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkProgressBarAccessible, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkRadioButtonAccessible, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkRadioMenuItemAccessible, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkRangeAccessible, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkRendererCellAccessible, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkScaleAccessible, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkScaleButtonAccessible, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkScrolledWindowAccessible, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkSpinButtonAccessible, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkSpinnerAccessible, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkStatusbarAccessible, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkSwitchAccessible, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkTextCellAccessible, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkTextViewAccessible, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkToggleButtonAccessible, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkToplevelAccessible, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkTreeViewAccessible, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkWidgetAccessible, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkWindowAccessible, g_object_unref)

#endif
