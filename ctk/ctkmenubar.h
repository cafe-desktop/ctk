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

#ifndef __CTK_MENU_BAR_H__
#define __CTK_MENU_BAR_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkmenushell.h>


G_BEGIN_DECLS


#define	CTK_TYPE_MENU_BAR               (ctk_menu_bar_get_type ())
#define CTK_MENU_BAR(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_MENU_BAR, CtkMenuBar))
#define CTK_MENU_BAR_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_MENU_BAR, CtkMenuBarClass))
#define CTK_IS_MENU_BAR(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_MENU_BAR))
#define CTK_IS_MENU_BAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_MENU_BAR))
#define CTK_MENU_BAR_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_MENU_BAR, CtkMenuBarClass))

typedef struct _CtkMenuBar         CtkMenuBar;
typedef struct _CtkMenuBarPrivate  CtkMenuBarPrivate;
typedef struct _CtkMenuBarClass    CtkMenuBarClass;

struct _CtkMenuBar
{
  CtkMenuShell menu_shell;

  /*< private >*/
  CtkMenuBarPrivate *priv;
};

struct _CtkMenuBarClass
{
  CtkMenuShellClass parent_class;

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};


CDK_AVAILABLE_IN_ALL
GType      ctk_menu_bar_get_type        (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
CtkWidget* ctk_menu_bar_new             (void);
CDK_AVAILABLE_IN_3_4
CtkWidget* ctk_menu_bar_new_from_model  (GMenuModel *model);

CDK_AVAILABLE_IN_ALL
CtkPackDirection ctk_menu_bar_get_pack_direction (CtkMenuBar       *menubar);
CDK_AVAILABLE_IN_ALL
void             ctk_menu_bar_set_pack_direction (CtkMenuBar       *menubar,
						  CtkPackDirection  pack_dir);
CDK_AVAILABLE_IN_ALL
CtkPackDirection ctk_menu_bar_get_child_pack_direction (CtkMenuBar       *menubar);
CDK_AVAILABLE_IN_ALL
void             ctk_menu_bar_set_child_pack_direction (CtkMenuBar       *menubar,
							CtkPackDirection  child_pack_dir);

/* Private functions */
void _ctk_menu_bar_cycle_focus (CtkMenuBar       *menubar,
				CtkDirectionType  dir);
GList* _ctk_menu_bar_get_viewable_menu_bars (CtkWindow *window);



G_END_DECLS


#endif /* __CTK_MENU_BAR_H__ */
