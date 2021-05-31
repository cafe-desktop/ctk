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
#define CTK_MENU_SHELL(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_MENU_SHELL, GtkMenuShell))
#define CTK_MENU_SHELL_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_MENU_SHELL, GtkMenuShellClass))
#define CTK_IS_MENU_SHELL(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_MENU_SHELL))
#define CTK_IS_MENU_SHELL_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_MENU_SHELL))
#define CTK_MENU_SHELL_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_MENU_SHELL, GtkMenuShellClass))


typedef struct _GtkMenuShell        GtkMenuShell;
typedef struct _GtkMenuShellClass   GtkMenuShellClass;
typedef struct _GtkMenuShellPrivate GtkMenuShellPrivate;

struct _GtkMenuShell
{
  GtkContainer container;

  /*< private >*/
  GtkMenuShellPrivate *priv;
};

struct _GtkMenuShellClass
{
  GtkContainerClass parent_class;

  guint submenu_placement : 1;

  void     (*deactivate)       (GtkMenuShell *menu_shell);
  void     (*selection_done)   (GtkMenuShell *menu_shell);

  void     (*move_current)     (GtkMenuShell *menu_shell,
                                GtkMenuDirectionType direction);
  void     (*activate_current) (GtkMenuShell *menu_shell,
                                gboolean      force_hide);
  void     (*cancel)           (GtkMenuShell *menu_shell);
  void     (*select_item)      (GtkMenuShell *menu_shell,
                                GtkWidget    *menu_item);
  void     (*insert)           (GtkMenuShell *menu_shell,
                                GtkWidget    *child,
                                gint          position);
  gint     (*get_popup_delay)  (GtkMenuShell *menu_shell);
  gboolean (*move_selected)    (GtkMenuShell *menu_shell,
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
void     ctk_menu_shell_append         (GtkMenuShell *menu_shell,
                                        GtkWidget    *child);
GDK_AVAILABLE_IN_ALL
void     ctk_menu_shell_prepend        (GtkMenuShell *menu_shell,
                                        GtkWidget    *child);
GDK_AVAILABLE_IN_ALL
void     ctk_menu_shell_insert         (GtkMenuShell *menu_shell,
                                        GtkWidget    *child,
                                        gint          position);
GDK_AVAILABLE_IN_ALL
void     ctk_menu_shell_deactivate     (GtkMenuShell *menu_shell);
GDK_AVAILABLE_IN_ALL
void     ctk_menu_shell_select_item    (GtkMenuShell *menu_shell,
                                        GtkWidget    *menu_item);
GDK_AVAILABLE_IN_ALL
void     ctk_menu_shell_deselect       (GtkMenuShell *menu_shell);
GDK_AVAILABLE_IN_ALL
void     ctk_menu_shell_activate_item  (GtkMenuShell *menu_shell,
                                        GtkWidget    *menu_item,
                                        gboolean      force_deactivate);
GDK_AVAILABLE_IN_ALL
void     ctk_menu_shell_select_first   (GtkMenuShell *menu_shell,
                                        gboolean      search_sensitive);
GDK_AVAILABLE_IN_ALL
void     ctk_menu_shell_cancel         (GtkMenuShell *menu_shell);
GDK_AVAILABLE_IN_ALL
gboolean ctk_menu_shell_get_take_focus (GtkMenuShell *menu_shell);
GDK_AVAILABLE_IN_ALL
void     ctk_menu_shell_set_take_focus (GtkMenuShell *menu_shell,
                                        gboolean      take_focus);

GDK_AVAILABLE_IN_ALL
GtkWidget *ctk_menu_shell_get_selected_item (GtkMenuShell *menu_shell);
GDK_AVAILABLE_IN_ALL
GtkWidget *ctk_menu_shell_get_parent_shell  (GtkMenuShell *menu_shell);

GDK_AVAILABLE_IN_3_6
void       ctk_menu_shell_bind_model   (GtkMenuShell *menu_shell,
                                        GMenuModel   *model,
                                        const gchar  *action_namespace,
                                        gboolean      with_separators);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(GtkMenuShell, g_object_unref)

G_END_DECLS

#endif /* __CTK_MENU_SHELL_H__ */
