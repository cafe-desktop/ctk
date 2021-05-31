/*
 * Copyright (c) 2013 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef __CTK_HEADER_BAR_H__
#define __CTK_HEADER_BAR_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkcontainer.h>

G_BEGIN_DECLS

#define CTK_TYPE_HEADER_BAR            (ctk_header_bar_get_type ())
#define CTK_HEADER_BAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_HEADER_BAR, CtkHeaderBar))
#define CTK_HEADER_BAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_HEADER_BAR, CtkHeaderBarClass))
#define CTK_IS_HEADER_BAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_HEADER_BAR))
#define CTK_IS_HEADER_BAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_HEADER_BAR))
#define CTK_HEADER_BAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_HEADER_BAR, CtkHeaderBarClass))

typedef struct _CtkHeaderBar              CtkHeaderBar;
typedef struct _CtkHeaderBarPrivate       CtkHeaderBarPrivate;
typedef struct _CtkHeaderBarClass         CtkHeaderBarClass;

struct _CtkHeaderBar
{
  CtkContainer container;
};

struct _CtkHeaderBarClass
{
  CtkContainerClass parent_class;

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

GDK_AVAILABLE_IN_3_10
GType        ctk_header_bar_get_type          (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_3_10
CtkWidget   *ctk_header_bar_new               (void);
GDK_AVAILABLE_IN_3_10
void         ctk_header_bar_set_title         (CtkHeaderBar *bar,
                                               const gchar  *title);
GDK_AVAILABLE_IN_3_10
const gchar *ctk_header_bar_get_title         (CtkHeaderBar *bar);
GDK_AVAILABLE_IN_3_10
void         ctk_header_bar_set_subtitle      (CtkHeaderBar *bar,
                                               const gchar  *subtitle);
GDK_AVAILABLE_IN_3_10
const gchar *ctk_header_bar_get_subtitle      (CtkHeaderBar *bar);


GDK_AVAILABLE_IN_3_10
void         ctk_header_bar_set_custom_title  (CtkHeaderBar *bar,
                                               CtkWidget    *title_widget);
GDK_AVAILABLE_IN_3_10
CtkWidget   *ctk_header_bar_get_custom_title  (CtkHeaderBar *bar);
GDK_AVAILABLE_IN_3_10
void         ctk_header_bar_pack_start        (CtkHeaderBar *bar,
                                               CtkWidget    *child);
GDK_AVAILABLE_IN_3_10
void         ctk_header_bar_pack_end          (CtkHeaderBar *bar,
                                               CtkWidget    *child);

GDK_AVAILABLE_IN_3_10
gboolean     ctk_header_bar_get_show_close_button (CtkHeaderBar *bar);

GDK_AVAILABLE_IN_3_10
void         ctk_header_bar_set_show_close_button (CtkHeaderBar *bar,
                                                   gboolean      setting);

GDK_AVAILABLE_IN_3_12
void         ctk_header_bar_set_has_subtitle (CtkHeaderBar *bar,
                                              gboolean      setting);
GDK_AVAILABLE_IN_3_12
gboolean     ctk_header_bar_get_has_subtitle (CtkHeaderBar *bar);

GDK_AVAILABLE_IN_3_12
void         ctk_header_bar_set_decoration_layout (CtkHeaderBar *bar,
                                                   const gchar  *layout);
GDK_AVAILABLE_IN_3_12
const gchar *ctk_header_bar_get_decoration_layout (CtkHeaderBar *bar);

G_END_DECLS

#endif /* __CTK_HEADER_BAR_H__ */
