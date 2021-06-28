/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
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

#ifndef __CTK_NOTEBOOK_H__
#define __CTK_NOTEBOOK_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkcontainer.h>


G_BEGIN_DECLS

#define CTK_TYPE_NOTEBOOK                  (ctk_notebook_get_type ())
#define CTK_NOTEBOOK(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_NOTEBOOK, CtkNotebook))
#define CTK_NOTEBOOK_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_NOTEBOOK, CtkNotebookClass))
#define CTK_IS_NOTEBOOK(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_NOTEBOOK))
#define CTK_IS_NOTEBOOK_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_NOTEBOOK))
#define CTK_NOTEBOOK_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_NOTEBOOK, CtkNotebookClass))


typedef enum
{
  CTK_NOTEBOOK_TAB_FIRST,
  CTK_NOTEBOOK_TAB_LAST
} CtkNotebookTab;

typedef struct _CtkNotebook              CtkNotebook;
typedef struct _CtkNotebookPrivate       CtkNotebookPrivate;
typedef struct _CtkNotebookClass         CtkNotebookClass;

struct _CtkNotebook
{
  /*< private >*/
  CtkContainer container;

  CtkNotebookPrivate *priv;
};

struct _CtkNotebookClass
{
  CtkContainerClass parent_class;

  void (* switch_page)       (CtkNotebook     *notebook,
                              CtkWidget       *page,
			      guint            page_num);

  /* Action signals for keybindings */
  gboolean (* select_page)     (CtkNotebook       *notebook,
                                gboolean           move_focus);
  gboolean (* focus_tab)       (CtkNotebook       *notebook,
                                CtkNotebookTab     type);
  gboolean (* change_current_page) (CtkNotebook   *notebook,
                                gint               offset);
  void (* move_focus_out)      (CtkNotebook       *notebook,
				CtkDirectionType   direction);
  gboolean (* reorder_tab)     (CtkNotebook       *notebook,
				CtkDirectionType   direction,
				gboolean           move_to_last);

  /* More vfuncs */
  gint (* insert_page)         (CtkNotebook       *notebook,
			        CtkWidget         *child,
				CtkWidget         *tab_label,
				CtkWidget         *menu_label,
				gint               position);

  CtkNotebook * (* create_window) (CtkNotebook       *notebook,
                                   CtkWidget         *page,
                                   gint               x,
                                   gint               y);

  void (* page_reordered)      (CtkNotebook     *notebook,
                                CtkWidget       *child,
                                guint            page_num);

  void (* page_removed)        (CtkNotebook     *notebook,
                                CtkWidget       *child,
                                guint            page_num);

  void (* page_added)          (CtkNotebook     *notebook,
                                CtkWidget       *child,
                                guint            page_num);

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
  void (*_ctk_reserved5) (void);
  void (*_ctk_reserved6) (void);
  void (*_ctk_reserved7) (void);
  void (*_ctk_reserved8) (void);
};

/***********************************************************
 *           Creation, insertion, deletion                 *
 ***********************************************************/

CDK_AVAILABLE_IN_ALL
GType   ctk_notebook_get_type       (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
CtkWidget * ctk_notebook_new        (void);
CDK_AVAILABLE_IN_ALL
gint ctk_notebook_append_page       (CtkNotebook *notebook,
				     CtkWidget   *child,
				     CtkWidget   *tab_label);
CDK_AVAILABLE_IN_ALL
gint ctk_notebook_append_page_menu  (CtkNotebook *notebook,
				     CtkWidget   *child,
				     CtkWidget   *tab_label,
				     CtkWidget   *menu_label);
CDK_AVAILABLE_IN_ALL
gint ctk_notebook_prepend_page      (CtkNotebook *notebook,
				     CtkWidget   *child,
				     CtkWidget   *tab_label);
CDK_AVAILABLE_IN_ALL
gint ctk_notebook_prepend_page_menu (CtkNotebook *notebook,
				     CtkWidget   *child,
				     CtkWidget   *tab_label,
				     CtkWidget   *menu_label);
CDK_AVAILABLE_IN_ALL
gint ctk_notebook_insert_page       (CtkNotebook *notebook,
				     CtkWidget   *child,
				     CtkWidget   *tab_label,
				     gint         position);
CDK_AVAILABLE_IN_ALL
gint ctk_notebook_insert_page_menu  (CtkNotebook *notebook,
				     CtkWidget   *child,
				     CtkWidget   *tab_label,
				     CtkWidget   *menu_label,
				     gint         position);
CDK_AVAILABLE_IN_ALL
void ctk_notebook_remove_page       (CtkNotebook *notebook,
				     gint         page_num);

/***********************************************************
 *           Tabs drag and drop                            *
 ***********************************************************/

CDK_AVAILABLE_IN_ALL
void         ctk_notebook_set_group_name (CtkNotebook *notebook,
                                          const gchar *group_name);
CDK_AVAILABLE_IN_ALL
const gchar *ctk_notebook_get_group_name (CtkNotebook *notebook);



/***********************************************************
 *            query, set current NotebookPage              *
 ***********************************************************/

CDK_AVAILABLE_IN_ALL
gint       ctk_notebook_get_current_page (CtkNotebook *notebook);
CDK_AVAILABLE_IN_ALL
CtkWidget* ctk_notebook_get_nth_page     (CtkNotebook *notebook,
					  gint         page_num);
CDK_AVAILABLE_IN_ALL
gint       ctk_notebook_get_n_pages      (CtkNotebook *notebook);
CDK_AVAILABLE_IN_ALL
gint       ctk_notebook_page_num         (CtkNotebook *notebook,
					  CtkWidget   *child);
CDK_AVAILABLE_IN_ALL
void       ctk_notebook_set_current_page (CtkNotebook *notebook,
					  gint         page_num);
CDK_AVAILABLE_IN_ALL
void       ctk_notebook_next_page        (CtkNotebook *notebook);
CDK_AVAILABLE_IN_ALL
void       ctk_notebook_prev_page        (CtkNotebook *notebook);

/***********************************************************
 *            set Notebook, NotebookTab style              *
 ***********************************************************/

CDK_AVAILABLE_IN_ALL
void     ctk_notebook_set_show_border      (CtkNotebook     *notebook,
					    gboolean         show_border);
CDK_AVAILABLE_IN_ALL
gboolean ctk_notebook_get_show_border      (CtkNotebook     *notebook);
CDK_AVAILABLE_IN_ALL
void     ctk_notebook_set_show_tabs        (CtkNotebook     *notebook,
					    gboolean         show_tabs);
CDK_AVAILABLE_IN_ALL
gboolean ctk_notebook_get_show_tabs        (CtkNotebook     *notebook);
CDK_AVAILABLE_IN_ALL
void     ctk_notebook_set_tab_pos          (CtkNotebook     *notebook,
				            CtkPositionType  pos);
CDK_AVAILABLE_IN_ALL
CtkPositionType ctk_notebook_get_tab_pos   (CtkNotebook     *notebook);
CDK_AVAILABLE_IN_ALL
void     ctk_notebook_set_scrollable       (CtkNotebook     *notebook,
					    gboolean         scrollable);
CDK_AVAILABLE_IN_ALL
gboolean ctk_notebook_get_scrollable       (CtkNotebook     *notebook);
CDK_DEPRECATED_IN_3_4
guint16  ctk_notebook_get_tab_hborder      (CtkNotebook     *notebook);
CDK_DEPRECATED_IN_3_4
guint16  ctk_notebook_get_tab_vborder      (CtkNotebook     *notebook);

/***********************************************************
 *               enable/disable PopupMenu                  *
 ***********************************************************/

CDK_AVAILABLE_IN_ALL
void ctk_notebook_popup_enable  (CtkNotebook *notebook);
CDK_AVAILABLE_IN_ALL
void ctk_notebook_popup_disable (CtkNotebook *notebook);

/***********************************************************
 *             query/set NotebookPage Properties           *
 ***********************************************************/

CDK_AVAILABLE_IN_ALL
CtkWidget * ctk_notebook_get_tab_label    (CtkNotebook *notebook,
					   CtkWidget   *child);
CDK_AVAILABLE_IN_ALL
void ctk_notebook_set_tab_label           (CtkNotebook *notebook,
					   CtkWidget   *child,
					   CtkWidget   *tab_label);
CDK_AVAILABLE_IN_ALL
void          ctk_notebook_set_tab_label_text (CtkNotebook *notebook,
                                               CtkWidget   *child,
                                               const gchar *tab_text);
CDK_AVAILABLE_IN_ALL
const gchar * ctk_notebook_get_tab_label_text (CtkNotebook *notebook,
                                               CtkWidget   *child);
CDK_AVAILABLE_IN_ALL
CtkWidget * ctk_notebook_get_menu_label   (CtkNotebook *notebook,
					   CtkWidget   *child);
CDK_AVAILABLE_IN_ALL
void ctk_notebook_set_menu_label          (CtkNotebook *notebook,
					   CtkWidget   *child,
					   CtkWidget   *menu_label);
CDK_AVAILABLE_IN_ALL
void          ctk_notebook_set_menu_label_text (CtkNotebook *notebook,
                                                CtkWidget   *child,
                                                const gchar *menu_text);
CDK_AVAILABLE_IN_ALL
const gchar * ctk_notebook_get_menu_label_text (CtkNotebook *notebook,
							CtkWidget   *child);
CDK_AVAILABLE_IN_ALL
void ctk_notebook_reorder_child           (CtkNotebook *notebook,
					   CtkWidget   *child,
					   gint         position);
CDK_AVAILABLE_IN_ALL
gboolean ctk_notebook_get_tab_reorderable (CtkNotebook *notebook,
					   CtkWidget   *child);
CDK_AVAILABLE_IN_ALL
void ctk_notebook_set_tab_reorderable     (CtkNotebook *notebook,
					   CtkWidget   *child,
					   gboolean     reorderable);
CDK_AVAILABLE_IN_ALL
gboolean ctk_notebook_get_tab_detachable  (CtkNotebook *notebook,
					   CtkWidget   *child);
CDK_AVAILABLE_IN_ALL
void ctk_notebook_set_tab_detachable      (CtkNotebook *notebook,
					   CtkWidget   *child,
					   gboolean     detachable);
CDK_AVAILABLE_IN_3_16
void ctk_notebook_detach_tab              (CtkNotebook *notebook,
                                           CtkWidget   *child);

CDK_AVAILABLE_IN_ALL
CtkWidget* ctk_notebook_get_action_widget (CtkNotebook *notebook,
                                           CtkPackType  pack_type);
CDK_AVAILABLE_IN_ALL
void       ctk_notebook_set_action_widget (CtkNotebook *notebook,
                                           CtkWidget   *widget,
                                           CtkPackType  pack_type);

G_END_DECLS

#endif /* __CTK_NOTEBOOK_H__ */
