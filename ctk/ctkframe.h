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

#ifndef __CTK_FRAME_H__
#define __CTK_FRAME_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkbin.h>


G_BEGIN_DECLS


#define CTK_TYPE_FRAME                  (ctk_frame_get_type ())
#define CTK_FRAME(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_FRAME, CtkFrame))
#define CTK_FRAME_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_FRAME, CtkFrameClass))
#define CTK_IS_FRAME(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_FRAME))
#define CTK_IS_FRAME_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_FRAME))
#define CTK_FRAME_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_FRAME, CtkFrameClass))

typedef struct _CtkFrame              CtkFrame;
typedef struct _CtkFramePrivate       CtkFramePrivate;
typedef struct _CtkFrameClass         CtkFrameClass;

struct _CtkFrame
{
  CtkBin bin;

  /*< private >*/
  CtkFramePrivate *priv;
};

/**
 * CtkFrameClass:
 * @parent_class: The parent class.
 * @compute_child_allocation:
 */
struct _CtkFrameClass
{
  CtkBinClass parent_class;

  /*< public >*/

  void (*compute_child_allocation) (CtkFrame *frame,
                                    CtkAllocation *allocation);

  /*< private >*/

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};


CDK_AVAILABLE_IN_ALL
GType      ctk_frame_get_type         (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
CtkWidget* ctk_frame_new              (const gchar   *label);

CDK_AVAILABLE_IN_ALL
void          ctk_frame_set_label (CtkFrame    *frame,
                                   const gchar *label);
CDK_AVAILABLE_IN_ALL
const gchar * ctk_frame_get_label (CtkFrame    *frame);

CDK_AVAILABLE_IN_ALL
void       ctk_frame_set_label_widget (CtkFrame      *frame,
				       CtkWidget     *label_widget);
CDK_AVAILABLE_IN_ALL
CtkWidget *ctk_frame_get_label_widget (CtkFrame      *frame);
CDK_AVAILABLE_IN_ALL
void       ctk_frame_set_label_align  (CtkFrame      *frame,
				       gfloat         xalign,
				       gfloat         yalign);
CDK_AVAILABLE_IN_ALL
void       ctk_frame_get_label_align  (CtkFrame      *frame,
				       gfloat        *xalign,
				       gfloat        *yalign);
CDK_AVAILABLE_IN_ALL
void       ctk_frame_set_shadow_type  (CtkFrame      *frame,
				       CtkShadowType  type);
CDK_AVAILABLE_IN_ALL
CtkShadowType ctk_frame_get_shadow_type (CtkFrame    *frame);


G_END_DECLS


#endif /* __CTK_FRAME_H__ */
