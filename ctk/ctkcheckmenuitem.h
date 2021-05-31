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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the GTK+ Team and others 1997-2001.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CTK_CHECK_MENU_ITEM_H__
#define __CTK_CHECK_MENU_ITEM_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkmenuitem.h>


G_BEGIN_DECLS

#define CTK_TYPE_CHECK_MENU_ITEM            (ctk_check_menu_item_get_type ())
#define CTK_CHECK_MENU_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_CHECK_MENU_ITEM, CtkCheckMenuItem))
#define CTK_CHECK_MENU_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_CHECK_MENU_ITEM, CtkCheckMenuItemClass))
#define CTK_IS_CHECK_MENU_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_CHECK_MENU_ITEM))
#define CTK_IS_CHECK_MENU_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_CHECK_MENU_ITEM))
#define CTK_CHECK_MENU_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_CHECK_MENU_ITEM, CtkCheckMenuItemClass))


typedef struct _CtkCheckMenuItem              CtkCheckMenuItem;
typedef struct _CtkCheckMenuItemPrivate       CtkCheckMenuItemPrivate;
typedef struct _CtkCheckMenuItemClass         CtkCheckMenuItemClass;

struct _CtkCheckMenuItem
{
  CtkMenuItem menu_item;

  /*< private >*/
  CtkCheckMenuItemPrivate *priv;
};

/**
 * CtkCheckMenuItemClass:
 * @parent_class: The parent class.
 * @toggled: Signal emitted when the state of the check box is changed.
 * @draw_indicator: Called to draw the check indicator.
 */
struct _CtkCheckMenuItemClass
{
  CtkMenuItemClass parent_class;

  /*< public >*/

  void (* toggled)	  (CtkCheckMenuItem *check_menu_item);
  void (* draw_indicator) (CtkCheckMenuItem *check_menu_item,
			   cairo_t          *cr);

  /*< private >*/

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};


GDK_AVAILABLE_IN_ALL
GType	   ctk_check_menu_item_get_type	         (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
CtkWidget* ctk_check_menu_item_new               (void);
GDK_AVAILABLE_IN_ALL
CtkWidget* ctk_check_menu_item_new_with_label    (const gchar      *label);
GDK_AVAILABLE_IN_ALL
CtkWidget* ctk_check_menu_item_new_with_mnemonic (const gchar      *label);
GDK_AVAILABLE_IN_ALL
void       ctk_check_menu_item_set_active        (CtkCheckMenuItem *check_menu_item,
						  gboolean          is_active);
GDK_AVAILABLE_IN_ALL
gboolean   ctk_check_menu_item_get_active        (CtkCheckMenuItem *check_menu_item);
GDK_AVAILABLE_IN_ALL
void       ctk_check_menu_item_toggled           (CtkCheckMenuItem *check_menu_item);
GDK_AVAILABLE_IN_ALL
void       ctk_check_menu_item_set_inconsistent  (CtkCheckMenuItem *check_menu_item,
						  gboolean          setting);
GDK_AVAILABLE_IN_ALL
gboolean   ctk_check_menu_item_get_inconsistent  (CtkCheckMenuItem *check_menu_item);
GDK_AVAILABLE_IN_ALL
void       ctk_check_menu_item_set_draw_as_radio (CtkCheckMenuItem *check_menu_item,
						  gboolean          draw_as_radio);
GDK_AVAILABLE_IN_ALL
gboolean   ctk_check_menu_item_get_draw_as_radio (CtkCheckMenuItem *check_menu_item);

G_END_DECLS

#endif /* __CTK_CHECK_MENU_ITEM_H__ */
