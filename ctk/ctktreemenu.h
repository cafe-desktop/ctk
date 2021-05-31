/* ctktreemenu.h
 *
 * Copyright (C) 2010 Openismus GmbH
 *
 * Authors:
 *      Tristan Van Berkom <tristanvb@openismus.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __CTK_TREE_MENU_H__
#define __CTK_TREE_MENU_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkmenu.h>
#include <ctk/ctktreemodel.h>
#include <ctk/ctktreeview.h>
#include <ctk/ctkcellarea.h>

G_BEGIN_DECLS

#define CTK_TYPE_TREE_MENU		  (_ctk_tree_menu_get_type ())
#define CTK_TREE_MENU(obj)		  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_TREE_MENU, GtkTreeMenu))
#define CTK_TREE_MENU_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_TREE_MENU, GtkTreeMenuClass))
#define CTK_IS_TREE_MENU(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_TREE_MENU))
#define CTK_IS_TREE_MENU_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_TREE_MENU))
#define CTK_TREE_MENU_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_TREE_MENU, GtkTreeMenuClass))

typedef struct _GtkTreeMenu              GtkTreeMenu;
typedef struct _GtkTreeMenuClass         GtkTreeMenuClass;
typedef struct _GtkTreeMenuPrivate       GtkTreeMenuPrivate;

struct _GtkTreeMenu
{
  GtkMenu parent_instance;

  /*< private >*/
  GtkTreeMenuPrivate *priv;
};

struct _GtkTreeMenuClass
{
  GtkMenuClass parent_class;

  /*< private >*/
  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
  void (*_ctk_reserved5) (void);
  void (*_ctk_reserved6) (void);
};

GType                 _ctk_tree_menu_get_type                       (void) G_GNUC_CONST;

GtkWidget            *_ctk_tree_menu_new                            (void);
GtkWidget            *_ctk_tree_menu_new_with_area                  (GtkCellArea         *area);
GtkWidget            *_ctk_tree_menu_new_full                       (GtkCellArea         *area,
                                                                     GtkTreeModel        *model,
                                                                     GtkTreePath         *root);
void                  _ctk_tree_menu_set_model                      (GtkTreeMenu         *menu,
                                                                     GtkTreeModel        *model);
GtkTreeModel         *_ctk_tree_menu_get_model                      (GtkTreeMenu         *menu);
void                  _ctk_tree_menu_set_root                       (GtkTreeMenu         *menu,
                                                                     GtkTreePath         *path);
GtkTreePath          *_ctk_tree_menu_get_root                       (GtkTreeMenu         *menu);
gboolean              _ctk_tree_menu_get_tearoff                    (GtkTreeMenu         *menu);
void                  _ctk_tree_menu_set_tearoff                    (GtkTreeMenu         *menu,
                                                                     gboolean             tearoff);
gint                  _ctk_tree_menu_get_wrap_width                 (GtkTreeMenu         *menu);
void                  _ctk_tree_menu_set_wrap_width                 (GtkTreeMenu         *menu,
                                                                     gint                 width);
gint                  _ctk_tree_menu_get_row_span_column            (GtkTreeMenu         *menu);
void                  _ctk_tree_menu_set_row_span_column            (GtkTreeMenu         *menu,
                                                                     gint                 row_span);
gint                  _ctk_tree_menu_get_column_span_column         (GtkTreeMenu         *menu);
void                  _ctk_tree_menu_set_column_span_column         (GtkTreeMenu         *menu,
                                                                     gint                 column_span);

GtkTreeViewRowSeparatorFunc _ctk_tree_menu_get_row_separator_func   (GtkTreeMenu          *menu);
void                        _ctk_tree_menu_set_row_separator_func   (GtkTreeMenu          *menu,
                                                                     GtkTreeViewRowSeparatorFunc func,
                                                                     gpointer              data,
                                                                     GDestroyNotify        destroy);

G_END_DECLS

#endif /* __CTK_TREE_MENU_H__ */
