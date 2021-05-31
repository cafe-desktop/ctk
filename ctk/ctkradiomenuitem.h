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
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 */

#ifndef __CTK_RADIO_MENU_ITEM_H__
#define __CTK_RADIO_MENU_ITEM_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <gtk/gtk.h> can be included directly."
#endif

#include <gtk/gtkcheckmenuitem.h>


G_BEGIN_DECLS

#define CTK_TYPE_RADIO_MENU_ITEM	      (ctk_radio_menu_item_get_type ())
#define CTK_RADIO_MENU_ITEM(obj)	      (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_RADIO_MENU_ITEM, GtkRadioMenuItem))
#define CTK_RADIO_MENU_ITEM_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_RADIO_MENU_ITEM, GtkRadioMenuItemClass))
#define CTK_IS_RADIO_MENU_ITEM(obj)	      (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_RADIO_MENU_ITEM))
#define CTK_IS_RADIO_MENU_ITEM_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_RADIO_MENU_ITEM))
#define CTK_RADIO_MENU_ITEM_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_RADIO_MENU_ITEM, GtkRadioMenuItemClass))


typedef struct _GtkRadioMenuItem              GtkRadioMenuItem;
typedef struct _GtkRadioMenuItemPrivate       GtkRadioMenuItemPrivate;
typedef struct _GtkRadioMenuItemClass         GtkRadioMenuItemClass;

struct _GtkRadioMenuItem
{
  GtkCheckMenuItem check_menu_item;

  /*< private >*/
  GtkRadioMenuItemPrivate *priv;
};

struct _GtkRadioMenuItemClass
{
  GtkCheckMenuItemClass parent_class;

  /* Signals */
  void (*group_changed) (GtkRadioMenuItem *radio_menu_item);

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};


GDK_AVAILABLE_IN_ALL
GType      ctk_radio_menu_item_get_type	         (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
GtkWidget* ctk_radio_menu_item_new                           (GSList           *group);
GDK_AVAILABLE_IN_ALL
GtkWidget* ctk_radio_menu_item_new_with_label                (GSList           *group,
							      const gchar      *label);
GDK_AVAILABLE_IN_ALL
GtkWidget* ctk_radio_menu_item_new_with_mnemonic             (GSList           *group,
							      const gchar      *label);
GDK_AVAILABLE_IN_ALL
GtkWidget* ctk_radio_menu_item_new_from_widget               (GtkRadioMenuItem *group);
GDK_AVAILABLE_IN_ALL
GtkWidget *ctk_radio_menu_item_new_with_mnemonic_from_widget (GtkRadioMenuItem *group,
							      const gchar      *label);
GDK_AVAILABLE_IN_ALL
GtkWidget *ctk_radio_menu_item_new_with_label_from_widget    (GtkRadioMenuItem *group,
							      const gchar      *label);
GDK_AVAILABLE_IN_ALL
GSList*    ctk_radio_menu_item_get_group                     (GtkRadioMenuItem *radio_menu_item);
GDK_AVAILABLE_IN_ALL
void       ctk_radio_menu_item_set_group                     (GtkRadioMenuItem *radio_menu_item,
							      GSList           *group);

GDK_AVAILABLE_IN_3_18
void       ctk_radio_menu_item_join_group                    (GtkRadioMenuItem *radio_menu_item,
                                                              GtkRadioMenuItem *group_source);

G_END_DECLS

#endif /* __CTK_RADIO_MENU_ITEM_H__ */