/* GTK - The GIMP Toolkit
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
 * Modified by the GTK+ Team and others 2013.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 */

#ifndef __CTK_SEARCH_BAR_H__
#define __CTK_SEARCH_BAR_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <gtk/gtk.h> can be included directly."
#endif

#include <gtk/gtkrevealer.h>

G_BEGIN_DECLS

#define CTK_TYPE_SEARCH_BAR                 (ctk_search_bar_get_type ())
#define CTK_SEARCH_BAR(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_SEARCH_BAR, GtkSearchBar))
#define CTK_SEARCH_BAR_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_SEARCH_BAR, GtkSearchBarClass))
#define CTK_IS_SEARCH_BAR(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_SEARCH_BAR))
#define CTK_IS_SEARCH_BAR_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_SEARCH_BAR))
#define CTK_SEARCH_BAR_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_SEARCH_BAR, GtkSearchBarClass))

typedef struct _GtkSearchBar        GtkSearchBar;
typedef struct _GtkSearchBarClass   GtkSearchBarClass;

struct _GtkSearchBar
{
  /*< private >*/
  GtkBin parent;
};

/**
 * GtkSearchBarClass:
 * @parent_class: The parent class.
 */
struct _GtkSearchBarClass
{
  GtkBinClass parent_class;

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
GtkWidget*  ctk_search_bar_new             (void);

GDK_AVAILABLE_IN_3_10
void        ctk_search_bar_connect_entry   (GtkSearchBar *bar,
                                            GtkEntry     *entry);

GDK_AVAILABLE_IN_3_10
gboolean    ctk_search_bar_get_search_mode (GtkSearchBar *bar);
GDK_AVAILABLE_IN_3_10
void        ctk_search_bar_set_search_mode (GtkSearchBar *bar,
                                            gboolean      search_mode);

GDK_AVAILABLE_IN_3_10
gboolean    ctk_search_bar_get_show_close_button (GtkSearchBar *bar);
GDK_AVAILABLE_IN_3_10
void        ctk_search_bar_set_show_close_button (GtkSearchBar *bar,
                                                  gboolean      visible);

GDK_AVAILABLE_IN_3_10
gboolean    ctk_search_bar_handle_event    (GtkSearchBar *bar,
                                            GdkEvent     *event);

G_END_DECLS

#endif /* __CTK_SEARCH_BAR_H__ */
