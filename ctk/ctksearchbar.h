/* CTK - The GIMP Toolkit
 * Copyright (C) 2013 Red Hat, Inc.
 *
 * Authors:
 * - Bastien Nocera <bnocera@redhat.com>
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
 * Modified by the CTK+ Team and others 2013.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CTK_SEARCH_BAR_H__
#define __CTK_SEARCH_BAR_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkrevealer.h>

G_BEGIN_DECLS

#define CTK_TYPE_SEARCH_BAR                 (ctk_search_bar_get_type ())
#define CTK_SEARCH_BAR(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_SEARCH_BAR, CtkSearchBar))
#define CTK_SEARCH_BAR_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_SEARCH_BAR, CtkSearchBarClass))
#define CTK_IS_SEARCH_BAR(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_SEARCH_BAR))
#define CTK_IS_SEARCH_BAR_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_SEARCH_BAR))
#define CTK_SEARCH_BAR_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_SEARCH_BAR, CtkSearchBarClass))

typedef struct _CtkSearchBar        CtkSearchBar;
typedef struct _CtkSearchBarClass   CtkSearchBarClass;

struct _CtkSearchBar
{
  /*< private >*/
  CtkBin parent;
};

/**
 * CtkSearchBarClass:
 * @parent_class: The parent class.
 */
struct _CtkSearchBarClass
{
  CtkBinClass parent_class;

  /*< private >*/

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

GDK_AVAILABLE_IN_3_10
GType       ctk_search_bar_get_type        (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_3_10
CtkWidget*  ctk_search_bar_new             (void);

GDK_AVAILABLE_IN_3_10
void        ctk_search_bar_connect_entry   (CtkSearchBar *bar,
                                            CtkEntry     *entry);

GDK_AVAILABLE_IN_3_10
gboolean    ctk_search_bar_get_search_mode (CtkSearchBar *bar);
GDK_AVAILABLE_IN_3_10
void        ctk_search_bar_set_search_mode (CtkSearchBar *bar,
                                            gboolean      search_mode);

GDK_AVAILABLE_IN_3_10
gboolean    ctk_search_bar_get_show_close_button (CtkSearchBar *bar);
GDK_AVAILABLE_IN_3_10
void        ctk_search_bar_set_show_close_button (CtkSearchBar *bar,
                                                  gboolean      visible);

GDK_AVAILABLE_IN_3_10
gboolean    ctk_search_bar_handle_event    (CtkSearchBar *bar,
                                            GdkEvent     *event);

G_END_DECLS

#endif /* __CTK_SEARCH_BAR_H__ */
