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

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CTK_MENU_SHELL_H__
#define __CTK_MENU_SHELL_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkcontainer.h>

G_BEGIN_DECLS

#define CTK_TYPE_MENU_SHELL             (ctk_menu_shell_get_type ())
#define CTK_MENU_SHELL(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_MENU_SHELL, CtkMenuShell))
#define CTK_MENU_SHELL_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_MENU_SHELL, CtkMenuShellClass))
#define CTK_IS_MENU_SHELL(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_MENU_SHELL))
#define CTK_IS_MENU_SHELL_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_MENU_SHELL))
#define CTK_MENU_SHELL_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_MENU_SHELL, CtkMenuShellClass))


typedef struct _CtkMenuShell        CtkMenuShell;
typedef struct _CtkMenuShellClass   CtkMenuShellClass;
typedef struct _CtkMenuShellPrivate CtkMenuShellPrivate;

struct _CtkMenuShell
{
  CtkContainer container;

  /*< private >*/
  CtkMenuShellPrivate *priv;
};

struct _CtkMenuShellClass
{
  CtkContainerClass parent_class;

  guint submenu_placement : 1;

  void     (*deactivate)       (CtkMenuShell *menu_shell);
  void     (*selection_done)   (CtkMenuShell *menu_shell);

  void     (*move_current)     (CtkMenuShell *menu_shell,
                                CtkMenuDirectionType direction);
  void     (*activate_current) (CtkMenuShell *menu_shell,
                                gboolean      force_hide);
  void     (*cancel)           (CtkMenuShell *menu_shell);
  void     (*select_item)      (CtkMenuShell *menu_shell,
                                CtkWidget    *menu_item);
  void     (*insert)           (CtkMenuShell *menu_shell,
                                CtkWidget    *child,
                                gint          position);
  gint     (*get_popup_delay)  (CtkMenuShell *menu_shell);
  gboolean (*move_selected)    (CtkMenuShell *menu_shell,
                                gint          distance);

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};


GDK_AVAILABLE_IN_ALL
GType    ctk_menu_shell_get_type       (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
void     ctk_menu_shell_append         (CtkMenuShell *menu_shell,
                                        CtkWidget    *child);
GDK_AVAILABLE_IN_ALL
void     ctk_menu_shell_prepend        (CtkMenuShell *menu_shell,
                                        CtkWidget    *child);
GDK_AVAILABLE_IN_ALL
void     ctk_menu_shell_insert         (CtkMenuShell *menu_shell,
                                        CtkWidget    *child,
                                        gint          position);
GDK_AVAILABLE_IN_ALL
void     ctk_menu_shell_deactivate     (CtkMenuShell *menu_shell);
GDK_AVAILABLE_IN_ALL
void     ctk_menu_shell_select_item    (CtkMenuShell *menu_shell,
                                        CtkWidget    *menu_item);
GDK_AVAILABLE_IN_ALL
void     ctk_menu_shell_deselect       (CtkMenuShell *menu_shell);
GDK_AVAILABLE_IN_ALL
void     ctk_menu_shell_activate_item  (CtkMenuShell *menu_shell,
                                        CtkWidget    *menu_item,
                                        gboolean      force_deactivate);
GDK_AVAILABLE_IN_ALL
void     ctk_menu_shell_select_first   (CtkMenuShell *menu_shell,
                                        gboolean      search_sensitive);
GDK_AVAILABLE_IN_ALL
void     ctk_menu_shell_cancel         (CtkMenuShell *menu_shell);
GDK_AVAILABLE_IN_ALL
gboolean ctk_menu_shell_get_take_focus (CtkMenuShell *menu_shell);
GDK_AVAILABLE_IN_ALL
void     ctk_menu_shell_set_take_focus (CtkMenuShell *menu_shell,
                                        gboolean      take_focus);

GDK_AVAILABLE_IN_ALL
CtkWidget *ctk_menu_shell_get_selected_item (CtkMenuShell *menu_shell);
GDK_AVAILABLE_IN_ALL
CtkWidget *ctk_menu_shell_get_parent_shell  (CtkMenuShell *menu_shell);

GDK_AVAILABLE_IN_3_6
void       ctk_menu_shell_bind_model   (CtkMenuShell *menu_shell,
                                        GMenuModel   *model,
                                        const gchar  *action_namespace,
                                        gboolean      with_separators);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkMenuShell, g_object_unref)

G_END_DECLS

#endif /* __CTK_MENU_SHELL_H__ */
