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
#define CTK_TREE_MENU(obj)		  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_TREE_MENU, CtkTreeMenu))
#define CTK_TREE_MENU_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_TREE_MENU, CtkTreeMenuClass))
#define CTK_IS_TREE_MENU(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_TREE_MENU))
#define CTK_IS_TREE_MENU_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_TREE_MENU))
#define CTK_TREE_MENU_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_TREE_MENU, CtkTreeMenuClass))

typedef struct _CtkTreeMenu              CtkTreeMenu;
typedef struct _CtkTreeMenuClass         CtkTreeMenuClass;
typedef struct _CtkTreeMenuPrivate       CtkTreeMenuPrivate;

struct _CtkTreeMenu
{
  CtkMenu parent_instance;

  /*< private >*/
  CtkTreeMenuPrivate *priv;
};

struct _CtkTreeMenuClass
{
  CtkMenuClass parent_class;

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

CtkWidget            *_ctk_tree_menu_new                            (void);
CtkWidget            *_ctk_tree_menu_new_with_area                  (CtkCellArea         *area);
CtkWidget            *_ctk_tree_menu_new_full                       (CtkCellArea         *area,
                                                                     CtkTreeModel        *model,
                                                                     CtkTreePath         *root);
void                  _ctk_tree_menu_set_model                      (CtkTreeMenu         *menu,
                                                                     CtkTreeModel        *model);
CtkTreeModel         *_ctk_tree_menu_get_model                      (CtkTreeMenu         *menu);
void                  _ctk_tree_menu_set_root                       (CtkTreeMenu         *menu,
                                                                     CtkTreePath         *path);
CtkTreePath          *_ctk_tree_menu_get_root                       (CtkTreeMenu         *menu);
gboolean              _ctk_tree_menu_get_tearoff                    (CtkTreeMenu         *menu);
void                  _ctk_tree_menu_set_tearoff                    (CtkTreeMenu         *menu,
                                                                     gboolean             tearoff);
gint                  _ctk_tree_menu_get_wrap_width                 (CtkTreeMenu         *menu);
void                  _ctk_tree_menu_set_wrap_width                 (CtkTreeMenu         *menu,
                                                                     gint                 width);
gint                  _ctk_tree_menu_get_row_span_column            (CtkTreeMenu         *menu);
void                  _ctk_tree_menu_set_row_span_column            (CtkTreeMenu         *menu,
                                                                     gint                 row_span);
gint                  _ctk_tree_menu_get_column_span_column         (CtkTreeMenu         *menu);
void                  _ctk_tree_menu_set_column_span_column         (CtkTreeMenu         *menu,
                                                                     gint                 column_span);

CtkTreeViewRowSeparatorFunc _ctk_tree_menu_get_row_separator_func   (CtkTreeMenu          *menu);
void                        _ctk_tree_menu_set_row_separator_func   (CtkTreeMenu          *menu,
                                                                     CtkTreeViewRowSeparatorFunc func,
                                                                     gpointer              data,
                                                                     GDestroyNotify        destroy);

G_END_DECLS

#endif /* __CTK_TREE_MENU_H__ */
