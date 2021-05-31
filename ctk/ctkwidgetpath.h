/* GTK - The GIMP Toolkit
 * Copyright (C) 2010 Carlos Garnacho <carlosg@gnome.org>
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

#ifndef __CTK_WIDGET_PATH_H__
#define __CTK_WIDGET_PATH_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <glib-object.h>
#include <gdk/gdk.h>
#include <ctk/ctkenums.h>
#include <ctk/ctktypes.h>

G_BEGIN_DECLS

#define CTK_TYPE_WIDGET_PATH (ctk_widget_path_get_type ())

GDK_AVAILABLE_IN_ALL
GType           ctk_widget_path_get_type            (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
CtkWidgetPath * ctk_widget_path_new                 (void);

GDK_AVAILABLE_IN_ALL
CtkWidgetPath * ctk_widget_path_copy                (const CtkWidgetPath *path);
GDK_AVAILABLE_IN_3_2
CtkWidgetPath * ctk_widget_path_ref                 (CtkWidgetPath       *path);
GDK_AVAILABLE_IN_3_2
void            ctk_widget_path_unref               (CtkWidgetPath       *path);
GDK_AVAILABLE_IN_ALL
void            ctk_widget_path_free                (CtkWidgetPath       *path);

GDK_AVAILABLE_IN_3_2
char *          ctk_widget_path_to_string           (const CtkWidgetPath *path);
GDK_AVAILABLE_IN_ALL
gint            ctk_widget_path_length              (const CtkWidgetPath *path);

GDK_AVAILABLE_IN_ALL
gint            ctk_widget_path_append_type         (CtkWidgetPath       *path,
                                                     GType                type);
GDK_AVAILABLE_IN_ALL
void            ctk_widget_path_prepend_type        (CtkWidgetPath       *path,
                                                     GType                type);
GDK_AVAILABLE_IN_3_2
gint            ctk_widget_path_append_with_siblings(CtkWidgetPath       *path,
                                                     CtkWidgetPath       *siblings,
                                                     guint                sibling_index);
/* ctk_widget_path_append_for_widget() is declared in ctkwidget.c */
GDK_AVAILABLE_IN_3_2
gint            ctk_widget_path_append_for_widget   (CtkWidgetPath       *path,
                                                     CtkWidget           *widget);

GDK_AVAILABLE_IN_ALL
GType               ctk_widget_path_iter_get_object_type  (const CtkWidgetPath *path,
                                                           gint                 pos);
GDK_AVAILABLE_IN_ALL
void                ctk_widget_path_iter_set_object_type  (CtkWidgetPath       *path,
                                                           gint                 pos,
                                                           GType                type);
GDK_AVAILABLE_IN_3_20
const char *        ctk_widget_path_iter_get_object_name  (const CtkWidgetPath *path,
                                                           gint                 pos);
GDK_AVAILABLE_IN_3_20
void                ctk_widget_path_iter_set_object_name  (CtkWidgetPath       *path,
                                                           gint                 pos,
                                                           const char          *name);
GDK_AVAILABLE_IN_ALL
const CtkWidgetPath *
                    ctk_widget_path_iter_get_siblings     (const CtkWidgetPath *path,
                                                           gint                 pos);
GDK_AVAILABLE_IN_ALL
guint               ctk_widget_path_iter_get_sibling_index(const CtkWidgetPath *path,
                                                           gint                 pos);

GDK_AVAILABLE_IN_ALL
const gchar *          ctk_widget_path_iter_get_name  (const CtkWidgetPath *path,
                                                       gint                 pos);
GDK_AVAILABLE_IN_ALL
void                   ctk_widget_path_iter_set_name  (CtkWidgetPath       *path,
                                                       gint                 pos,
                                                       const gchar         *name);
GDK_AVAILABLE_IN_ALL
gboolean               ctk_widget_path_iter_has_name  (const CtkWidgetPath *path,
                                                       gint                 pos,
                                                       const gchar         *name);
GDK_AVAILABLE_IN_ALL
gboolean               ctk_widget_path_iter_has_qname (const CtkWidgetPath *path,
                                                       gint                 pos,
                                                       GQuark               qname);
GDK_AVAILABLE_IN_3_14
CtkStateFlags          ctk_widget_path_iter_get_state (const CtkWidgetPath *path,
                                                       gint                 pos);
GDK_AVAILABLE_IN_3_14
void                   ctk_widget_path_iter_set_state (CtkWidgetPath       *path,
                                                       gint                 pos,
                                                       CtkStateFlags        state);

GDK_AVAILABLE_IN_ALL
void     ctk_widget_path_iter_add_class     (CtkWidgetPath       *path,
                                             gint                 pos,
                                             const gchar         *name);
GDK_AVAILABLE_IN_ALL
void     ctk_widget_path_iter_remove_class  (CtkWidgetPath       *path,
                                             gint                 pos,
                                             const gchar         *name);
GDK_AVAILABLE_IN_ALL
void     ctk_widget_path_iter_clear_classes (CtkWidgetPath       *path,
                                             gint                 pos);
GDK_AVAILABLE_IN_ALL
GSList * ctk_widget_path_iter_list_classes  (const CtkWidgetPath *path,
                                             gint                 pos);
GDK_AVAILABLE_IN_ALL
gboolean ctk_widget_path_iter_has_class     (const CtkWidgetPath *path,
                                             gint                 pos,
                                             const gchar         *name);
GDK_AVAILABLE_IN_ALL
gboolean ctk_widget_path_iter_has_qclass    (const CtkWidgetPath *path,
                                             gint                 pos,
                                             GQuark               qname);

GDK_DEPRECATED_IN_3_14
void     ctk_widget_path_iter_add_region    (CtkWidgetPath      *path,
                                             gint                pos,
                                             const gchar        *name,
                                             CtkRegionFlags     flags);
GDK_DEPRECATED_IN_3_14
void     ctk_widget_path_iter_remove_region (CtkWidgetPath      *path,
                                             gint                pos,
                                             const gchar        *name);
GDK_DEPRECATED_IN_3_14
void     ctk_widget_path_iter_clear_regions (CtkWidgetPath      *path,
                                             gint                pos);

GDK_DEPRECATED_IN_3_14
GSList * ctk_widget_path_iter_list_regions  (const CtkWidgetPath *path,
                                             gint                 pos);

GDK_DEPRECATED_IN_3_14
gboolean ctk_widget_path_iter_has_region    (const CtkWidgetPath *path,
                                             gint                 pos,
                                             const gchar         *name,
                                             CtkRegionFlags      *flags);
GDK_DEPRECATED_IN_3_14
gboolean ctk_widget_path_iter_has_qregion   (const CtkWidgetPath *path,
                                             gint                 pos,
                                             GQuark               qname,
                                             CtkRegionFlags      *flags);

GDK_AVAILABLE_IN_ALL
GType           ctk_widget_path_get_object_type (const CtkWidgetPath *path);

GDK_AVAILABLE_IN_ALL
gboolean        ctk_widget_path_is_type    (const CtkWidgetPath *path,
                                            GType                type);
GDK_AVAILABLE_IN_ALL
gboolean        ctk_widget_path_has_parent (const CtkWidgetPath *path,
                                            GType                type);


G_END_DECLS

#endif /* __CTK_WIDGET_PATH_H__ */
