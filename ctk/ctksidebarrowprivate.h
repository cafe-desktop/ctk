/* ctksidebarrowprivate.h
 *
 * Copyright (C) 2015 Carlos Soriano <csoriano@gnome.org>
 *
 * This file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef CTK_SIDEBAR_ROW_PRIVATE_H
#define CTK_SIDEBAR_ROW_PRIVATE_H

#include <glib.h>
#include "ctklistbox.h"

G_BEGIN_DECLS

#define CTK_TYPE_SIDEBAR_ROW             (ctk_sidebar_row_get_type())
#define CTK_SIDEBAR_ROW(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_SIDEBAR_ROW, GtkSidebarRow))
#define CTK_SIDEBAR_ROW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_SIDEBAR_ROW, GtkSidebarRowClass))
#define CTK_IS_SIDEBAR_ROW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_SIDEBAR_ROW))
#define CTK_IS_SIDEBAR_ROW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_SIDEBAR_ROW))
#define CTK_SIDEBAR_ROW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_SIDEBAR_ROW, GtkSidebarRowClass))

typedef struct _GtkSidebarRow GtkSidebarRow;
typedef struct _GtkSidebarRowClass GtkSidebarRowClass;

struct _GtkSidebarRowClass
{
  GtkListBoxRowClass parent;
};

GType      ctk_sidebar_row_get_type   (void) G_GNUC_CONST;

GtkSidebarRow *ctk_sidebar_row_new    (void);
GtkSidebarRow *ctk_sidebar_row_clone  (GtkSidebarRow *self);

/* Use these methods instead of ctk_widget_hide/show to use an animation */
void           ctk_sidebar_row_hide   (GtkSidebarRow *self,
                                       gboolean       inmediate);
void           ctk_sidebar_row_reveal (GtkSidebarRow *self);

GtkWidget     *ctk_sidebar_row_get_eject_button (GtkSidebarRow *self);
GtkWidget     *ctk_sidebar_row_get_event_box    (GtkSidebarRow *self);
void           ctk_sidebar_row_set_start_icon   (GtkSidebarRow *self,
                                                 GIcon         *icon);
void           ctk_sidebar_row_set_end_icon     (GtkSidebarRow *self,
                                                 GIcon         *icon);
void           ctk_sidebar_row_set_busy         (GtkSidebarRow *row,
                                                 gboolean       is_busy);

G_END_DECLS

#endif /* CTK_SIDEBAR_ROW_PRIVATE_H */
