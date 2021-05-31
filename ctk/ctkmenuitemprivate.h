/* GTK - The GIMP Toolkit
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

#ifndef __CTK_MENU_ITEM_PRIVATE_H__
#define __CTK_MENU_ITEM_PRIVATE_H__

#include <ctk/ctkmenuitem.h>
#include <ctk/deprecated/ctkaction.h>
#include <ctk/ctkactionhelper.h>
#include <ctk/ctkcssgadgetprivate.h>
#include <ctk/ctkcssnodeprivate.h>

G_BEGIN_DECLS

struct _CtkMenuItemPrivate
{
  CtkWidget *submenu;
  GdkWindow *event_window;

  guint16 toggle_size;
  guint16 accelerator_width;

  guint timer;

  const char *accel_path;

  CtkAction *action;
  CtkActionHelper *action_helper;

  CtkCssGadget *gadget;
  CtkCssGadget *arrow_gadget;

  guint submenu_placement      : 1;
  guint submenu_direction      : 1;
  guint right_justify          : 1;
  guint from_menubar           : 1;
  guint use_action_appearance  : 1;
  guint reserve_indicator      : 1;
};

CtkCssGadget * _ctk_menu_item_get_gadget     (CtkMenuItem   *menu_item);
void     _ctk_menu_item_refresh_accel_path   (CtkMenuItem   *menu_item,
                                              const gchar   *prefix,
                                              CtkAccelGroup *accel_group,
                                              gboolean       group_changed);
gboolean _ctk_menu_item_is_selectable        (CtkWidget     *menu_item);
void     _ctk_menu_item_popup_submenu        (CtkWidget     *menu_item,
                                              gboolean       with_delay);
void     _ctk_menu_item_popdown_submenu      (CtkWidget     *menu_item);
void	  _ctk_menu_item_refresh_accel_path  (CtkMenuItem   *menu_item,
					      const gchar   *prefix,
					      CtkAccelGroup *accel_group,
					      gboolean	     group_changed);
gboolean  _ctk_menu_item_is_selectable       (CtkWidget     *menu_item);
void      _ctk_menu_item_popup_submenu       (CtkWidget     *menu_item,
					      gboolean       with_delay);
void      _ctk_menu_item_popdown_submenu     (CtkWidget     *menu_item);

G_END_DECLS

#endif /* __CTK_MENU_ITEM_PRIVATE_H__ */
