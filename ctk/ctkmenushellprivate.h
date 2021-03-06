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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __CTK_MENU_SHELL_PRIVATE_H__
#define __CTK_MENU_SHELL_PRIVATE_H__


#include <ctk/ctkmenushell.h>
#include <ctk/ctkmnemonichash.h>
#include <ctk/ctkkeyhash.h>
#include <ctk/ctkmenutracker.h>

G_BEGIN_DECLS

/* Placement of submenus */
typedef enum
{
  CTK_TOP_BOTTOM,
  CTK_LEFT_RIGHT
} CtkSubmenuPlacement;

struct _CtkMenuShellPrivate
{
  GList *children;

  CtkWidget *active_menu_item; /* This is not an "active" menu item
                                * (there is no such thing) but rather,
                                * the selected menu item in that MenuShell,
                                * if there is one.
                                */
  CtkWidget *parent_menu_shell;
  CtkMenuTracker *tracker;    // if bound to a GMenuModel

  guint button;
  guint32 activate_time;

  guint active               : 1;
  guint have_grab            : 1;
  guint have_xgrab           : 1;
  guint ignore_enter         : 1;
  guint keyboard_mode        : 1;

  guint take_focus           : 1;
  guint activated_submenu    : 1;
  guint in_unselectable_item : 1; /* This flag is a crutch to keep
                                   * mnemonics in the same menu if
                                   * the user moves the mouse over
                                   * an unselectable menuitem.
                                   */

  guint selection_done_coming_soon : 1; /* Set TRUE when a selection-done
                                         * signal is coming soon (when checked
                                         * from inside of a "hide" handler).
                                         */
  CtkMnemonicHash *mnemonic_hash;
  CtkKeyHash *key_hash;

  CdkDevice *grab_pointer;
};

void        _ctk_menu_shell_select_last      (CtkMenuShell *menu_shell,
                                              gboolean      search_sensitive);
gint        _ctk_menu_shell_get_popup_delay  (CtkMenuShell *menu_shell);
void        _ctk_menu_shell_set_grab_device  (CtkMenuShell *menu_shell,
                                              CdkDevice    *device);
CdkDevice *_ctk_menu_shell_get_grab_device   (CtkMenuShell *menu_shell);

void       _ctk_menu_shell_add_mnemonic      (CtkMenuShell *menu_shell,
                                              guint         keyval,
                                              CtkWidget    *target);
void       _ctk_menu_shell_remove_mnemonic   (CtkMenuShell *menu_shell,
                                              guint         keyval,
                                              CtkWidget    *target);

void       _ctk_menu_shell_update_mnemonics  (CtkMenuShell *menu_shell);
void       _ctk_menu_shell_set_keyboard_mode (CtkMenuShell *menu_shell,
                                              gboolean      keyboard_mode);
gboolean   _ctk_menu_shell_get_keyboard_mode (CtkMenuShell *menu_shell);


G_END_DECLS

#endif  /* __CTK_MENU_SHELL_PRIVATE_H__ */
