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

/*
 * Modified by the CTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CTK_MENU_ITEM_H__
#define __CTK_MENU_ITEM_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkbin.h>


G_BEGIN_DECLS

#define CTK_TYPE_MENU_ITEM            (ctk_menu_item_get_type ())
#define CTK_MENU_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_MENU_ITEM, CtkMenuItem))
#define CTK_MENU_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_MENU_ITEM, CtkMenuItemClass))
#define CTK_IS_MENU_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_MENU_ITEM))
#define CTK_IS_MENU_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_MENU_ITEM))
#define CTK_MENU_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_MENU_ITEM, CtkMenuItemClass))


typedef struct _CtkMenuItem        CtkMenuItem;
typedef struct _CtkMenuItemClass   CtkMenuItemClass;
typedef struct _CtkMenuItemPrivate CtkMenuItemPrivate;

struct _CtkMenuItem
{
  CtkBin bin;

  /*< private >*/
  CtkMenuItemPrivate *priv;
};

/**
 * CtkMenuItemClass:
 * @parent_class: The parent class.
 * @hide_on_activate: If %TRUE, then we should always
 *    hide the menu when the %CtkMenuItem is activated. Otherwise,
 *    it is up to the caller.
 * @activate: Signal emitted when the item is activated.
 * @activate_item: Signal emitted when the item is activated, but also
 *    if the menu item has a submenu.
 * @toggle_size_request: 
 * @toggle_size_allocate: 
 * @set_label: Sets @text on the #CtkMenuItem label
 * @get_label: Gets @text from the #CtkMenuItem label
 * @select: Signal emitted when the item is selected.
 * @deselect: Signal emitted when the item is deselected.
 */
struct _CtkMenuItemClass
{
  CtkBinClass parent_class;

  /*< public >*/

  /* If the following flag is true, then we should always
   * hide the menu when the MenuItem is activated. Otherwise,
   * it is up to the caller. For instance, when navigating
   * a menu with the keyboard, <Space> doesn't hide, but
   * <Return> does.
   */
  guint hide_on_activate : 1;

  void (* activate)             (CtkMenuItem *menu_item);
  void (* activate_item)        (CtkMenuItem *menu_item);
  void (* toggle_size_request)  (CtkMenuItem *menu_item,
                                 gint        *requisition);
  void (* toggle_size_allocate) (CtkMenuItem *menu_item,
                                 gint         allocation);
  void (* set_label)            (CtkMenuItem *menu_item,
                                 const gchar *label);
  const gchar * (* get_label)   (CtkMenuItem *menu_item);

  void (* select)               (CtkMenuItem *menu_item);
  void (* deselect)             (CtkMenuItem *menu_item);

  /*< private >*/

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};


GDK_AVAILABLE_IN_ALL
GType      ctk_menu_item_get_type             (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
CtkWidget* ctk_menu_item_new                  (void);
GDK_AVAILABLE_IN_ALL
CtkWidget* ctk_menu_item_new_with_label       (const gchar         *label);
GDK_AVAILABLE_IN_ALL
CtkWidget* ctk_menu_item_new_with_mnemonic    (const gchar         *label);
GDK_AVAILABLE_IN_ALL
void       ctk_menu_item_set_submenu          (CtkMenuItem         *menu_item,
                                               CtkWidget           *submenu);
GDK_AVAILABLE_IN_ALL
CtkWidget* ctk_menu_item_get_submenu          (CtkMenuItem         *menu_item);
GDK_AVAILABLE_IN_ALL
void       ctk_menu_item_select               (CtkMenuItem         *menu_item);
GDK_AVAILABLE_IN_ALL
void       ctk_menu_item_deselect             (CtkMenuItem         *menu_item);
GDK_AVAILABLE_IN_ALL
void       ctk_menu_item_activate             (CtkMenuItem         *menu_item);
GDK_AVAILABLE_IN_ALL
void       ctk_menu_item_toggle_size_request  (CtkMenuItem         *menu_item,
                                               gint                *requisition);
GDK_AVAILABLE_IN_ALL
void       ctk_menu_item_toggle_size_allocate (CtkMenuItem         *menu_item,
                                               gint                 allocation);
GDK_DEPRECATED_IN_3_2
void       ctk_menu_item_set_right_justified  (CtkMenuItem         *menu_item,
                                               gboolean             right_justified);
GDK_DEPRECATED_IN_3_2
gboolean   ctk_menu_item_get_right_justified  (CtkMenuItem         *menu_item);
GDK_AVAILABLE_IN_ALL
void          ctk_menu_item_set_accel_path    (CtkMenuItem         *menu_item,
                                               const gchar         *accel_path);
GDK_AVAILABLE_IN_ALL
const gchar * ctk_menu_item_get_accel_path    (CtkMenuItem    *menu_item);

GDK_AVAILABLE_IN_ALL
void          ctk_menu_item_set_label         (CtkMenuItem         *menu_item,
                                               const gchar         *label);
GDK_AVAILABLE_IN_ALL
const gchar * ctk_menu_item_get_label         (CtkMenuItem         *menu_item);

GDK_AVAILABLE_IN_ALL
void       ctk_menu_item_set_use_underline    (CtkMenuItem         *menu_item,
                                               gboolean             setting);
GDK_AVAILABLE_IN_ALL
gboolean   ctk_menu_item_get_use_underline    (CtkMenuItem         *menu_item);

GDK_AVAILABLE_IN_ALL
void       ctk_menu_item_set_reserve_indicator (CtkMenuItem        *menu_item,
                                                gboolean            reserve);
GDK_AVAILABLE_IN_ALL
gboolean   ctk_menu_item_get_reserve_indicator (CtkMenuItem        *menu_item);

G_END_DECLS

#endif /* __CTK_MENU_ITEM_H__ */
