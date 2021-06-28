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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
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

#ifndef __CTK_BOX_H__
#define __CTK_BOX_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkcontainer.h>


G_BEGIN_DECLS


#define CTK_TYPE_BOX            (ctk_box_get_type ())
#define CTK_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_BOX, CtkBox))
#define CTK_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_BOX, CtkBoxClass))
#define CTK_IS_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_BOX))
#define CTK_IS_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_BOX))
#define CTK_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_BOX, CtkBoxClass))


typedef struct _CtkBox              CtkBox;
typedef struct _CtkBoxPrivate       CtkBoxPrivate;
typedef struct _CtkBoxClass         CtkBoxClass;

struct _CtkBox
{
  CtkContainer container;

  /*< private >*/
  CtkBoxPrivate *priv;
};

/**
 * CtkBoxClass:
 * @parent_class: The parent class.
 */
struct _CtkBoxClass
{
  CtkContainerClass parent_class;

  /*< private >*/

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};


CDK_AVAILABLE_IN_ALL
GType       ctk_box_get_type            (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
CtkWidget*  ctk_box_new                 (CtkOrientation  orientation,
                                         gint            spacing);

CDK_AVAILABLE_IN_ALL
void        ctk_box_pack_start          (CtkBox         *box,
                                         CtkWidget      *child,
                                         gboolean        expand,
                                         gboolean        fill,
                                         guint           padding);
CDK_AVAILABLE_IN_ALL
void        ctk_box_pack_end            (CtkBox         *box,
                                         CtkWidget      *child,
                                         gboolean        expand,
                                         gboolean        fill,
                                         guint           padding);

CDK_AVAILABLE_IN_ALL
void        ctk_box_set_homogeneous     (CtkBox         *box,
                                         gboolean        homogeneous);
CDK_AVAILABLE_IN_ALL
gboolean    ctk_box_get_homogeneous     (CtkBox         *box);
CDK_AVAILABLE_IN_ALL
void        ctk_box_set_spacing         (CtkBox         *box,
                                         gint            spacing);
CDK_AVAILABLE_IN_ALL
gint        ctk_box_get_spacing         (CtkBox         *box);
CDK_AVAILABLE_IN_3_10
void        ctk_box_set_baseline_position (CtkBox             *box,
					   CtkBaselinePosition position);
CDK_AVAILABLE_IN_3_10
CtkBaselinePosition ctk_box_get_baseline_position (CtkBox         *box);

CDK_AVAILABLE_IN_ALL
void        ctk_box_reorder_child       (CtkBox         *box,
                                         CtkWidget      *child,
                                         gint            position);

CDK_AVAILABLE_IN_ALL
void        ctk_box_query_child_packing (CtkBox         *box,
                                         CtkWidget      *child,
                                         gboolean       *expand,
                                         gboolean       *fill,
                                         guint          *padding,
                                         CtkPackType    *pack_type);
CDK_AVAILABLE_IN_ALL
void        ctk_box_set_child_packing   (CtkBox         *box,
                                         CtkWidget      *child,
                                         gboolean        expand,
                                         gboolean        fill,
                                         guint           padding,
                                         CtkPackType     pack_type);

CDK_AVAILABLE_IN_3_12
void        ctk_box_set_center_widget   (CtkBox         *box,
                                         CtkWidget      *widget);
CDK_AVAILABLE_IN_3_12
CtkWidget  *ctk_box_get_center_widget   (CtkBox         *box);

G_END_DECLS

#endif /* __CTK_BOX_H__ */
