/*
 * Copyright (c) 2013 - 2014 Red Hat, Inc.
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

#ifndef __CTK_ACTION_BAR_H__
#define __CTK_ACTION_BAR_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkbin.h>

G_BEGIN_DECLS

#define CTK_TYPE_ACTION_BAR            (ctk_action_bar_get_type ())
#define CTK_ACTION_BAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_ACTION_BAR, CtkActionBar))
#define CTK_ACTION_BAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_ACTION_BAR, CtkActionBarClass))
#define CTK_IS_ACTION_BAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_ACTION_BAR))
#define CTK_IS_ACTION_BAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_ACTION_BAR))
#define CTK_ACTION_BAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_ACTION_BAR, CtkActionBarClass))

typedef struct _CtkActionBar              CtkActionBar;
typedef struct _CtkActionBarPrivate       CtkActionBarPrivate;
typedef struct _CtkActionBarClass         CtkActionBarClass;

struct _CtkActionBar
{
  /*< private >*/
  CtkBin bin;
};

struct _CtkActionBarClass
{
  /*< private >*/
  CtkBinClass parent_class;

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

GDK_AVAILABLE_IN_3_12
GType        ctk_action_bar_get_type          (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_3_12
CtkWidget   *ctk_action_bar_new               (void);
GDK_AVAILABLE_IN_3_12
CtkWidget   *ctk_action_bar_get_center_widget (CtkActionBar *action_bar);
GDK_AVAILABLE_IN_3_12
void         ctk_action_bar_set_center_widget (CtkActionBar *action_bar,
                                               CtkWidget    *center_widget);

GDK_AVAILABLE_IN_3_12
void         ctk_action_bar_pack_start        (CtkActionBar *action_bar,
                                               CtkWidget    *child);
GDK_AVAILABLE_IN_3_12
void         ctk_action_bar_pack_end          (CtkActionBar *action_bar,
                                               CtkWidget    *child);
G_END_DECLS

#endif /* __CTK_ACTION_BAR_H__ */
