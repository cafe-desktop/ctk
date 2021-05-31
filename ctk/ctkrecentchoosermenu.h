/* GTK - The GIMP Toolkit
 * ctkrecentchoosermenu.h - Recently used items menu widget
 * Copyright (C) 2006, Emmanuele Bassi
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

#ifndef __CTK_RECENT_CHOOSER_MENU_H__
#define __CTK_RECENT_CHOOSER_MENU_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkmenu.h>
#include <ctk/ctkrecentchooser.h>

G_BEGIN_DECLS

#define CTK_TYPE_RECENT_CHOOSER_MENU		(ctk_recent_chooser_menu_get_type ())
#define CTK_RECENT_CHOOSER_MENU(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_RECENT_CHOOSER_MENU, GtkRecentChooserMenu))
#define CTK_IS_RECENT_CHOOSER_MENU(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_RECENT_CHOOSER_MENU))
#define CTK_RECENT_CHOOSER_MENU_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_RECENT_CHOOSER_MENU, GtkRecentChooserMenuClass))
#define CTK_IS_RECENT_CHOOSER_MENU_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_RECENT_CHOOSER_MENU))
#define CTK_RECENT_CHOOSER_MENU_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_RECENT_CHOOSER_MENU, GtkRecentChooserMenuClass))

typedef struct _GtkRecentChooserMenu		GtkRecentChooserMenu;
typedef struct _GtkRecentChooserMenuClass	GtkRecentChooserMenuClass;
typedef struct _GtkRecentChooserMenuPrivate	GtkRecentChooserMenuPrivate;

struct _GtkRecentChooserMenu
{
  GtkMenu parent_instance;

  /*< private >*/
  GtkRecentChooserMenuPrivate *priv;
};

struct _GtkRecentChooserMenuClass
{
  GtkMenuClass parent_class;

  /* padding for future expansion */
  void (* ctk_recent1) (void);
  void (* ctk_recent2) (void);
  void (* ctk_recent3) (void);
  void (* ctk_recent4) (void);
};

GDK_AVAILABLE_IN_ALL
GType      ctk_recent_chooser_menu_get_type         (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
GtkWidget *ctk_recent_chooser_menu_new              (void);
GDK_AVAILABLE_IN_ALL
GtkWidget *ctk_recent_chooser_menu_new_for_manager  (GtkRecentManager     *manager);

GDK_AVAILABLE_IN_ALL
gboolean   ctk_recent_chooser_menu_get_show_numbers (GtkRecentChooserMenu *menu);
GDK_AVAILABLE_IN_ALL
void       ctk_recent_chooser_menu_set_show_numbers (GtkRecentChooserMenu *menu,
						     gboolean              show_numbers);

G_END_DECLS

#endif /* ! __CTK_RECENT_CHOOSER_MENU_H__ */
