/* CTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * Copyright (C) 1998 Elliot Lee
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

/* The CtkHandleBox is to allow widgets to be dragged in and out of
 * their parents.
 */

#ifndef __CTK_HANDLE_BOX_H__
#define __CTK_HANDLE_BOX_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkbin.h>

G_BEGIN_DECLS

#define CTK_TYPE_HANDLE_BOX            (ctk_handle_box_get_type ())
#define CTK_HANDLE_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_HANDLE_BOX, CtkHandleBox))
#define CTK_HANDLE_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_HANDLE_BOX, CtkHandleBoxClass))
#define CTK_IS_HANDLE_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_HANDLE_BOX))
#define CTK_IS_HANDLE_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_HANDLE_BOX))
#define CTK_HANDLE_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_HANDLE_BOX, CtkHandleBoxClass))

typedef struct _CtkHandleBox              CtkHandleBox;
typedef struct _CtkHandleBoxPrivate       CtkHandleBoxPrivate;
typedef struct _CtkHandleBoxClass         CtkHandleBoxClass;

struct _CtkHandleBox
{
  CtkBin bin;

  /*< private >*/
  CtkHandleBoxPrivate *priv;
};

/**
 * CtkHandleBoxClass:
 * @parent_class: The parent class.
 * @child_attached: Signal emitted when the contents of the handlebox
 *    are reattached to the main window. Deprecated: 3.4.
 * @child_detached: Signal emitted when the contents of the handlebox
 *    are detached from the main window. Deprecated: 3.4.
 */
struct _CtkHandleBoxClass
{
  CtkBinClass parent_class;

  void	(*child_attached)	(CtkHandleBox	*handle_box,
				 CtkWidget	*child);
  void	(*child_detached)	(CtkHandleBox	*handle_box,
				 CtkWidget	*child);

  /*< private >*/

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};


CDK_DEPRECATED_IN_3_4
GType         ctk_handle_box_get_type             (void) G_GNUC_CONST;
CDK_DEPRECATED_IN_3_4
CtkWidget*    ctk_handle_box_new                  (void);
CDK_DEPRECATED_IN_3_4
void          ctk_handle_box_set_shadow_type      (CtkHandleBox    *handle_box,
                                                   CtkShadowType    type);
CDK_DEPRECATED_IN_3_4
CtkShadowType ctk_handle_box_get_shadow_type      (CtkHandleBox    *handle_box);
CDK_DEPRECATED_IN_3_4
void          ctk_handle_box_set_handle_position  (CtkHandleBox    *handle_box,
					           CtkPositionType  position);
CDK_DEPRECATED_IN_3_4
CtkPositionType ctk_handle_box_get_handle_position(CtkHandleBox    *handle_box);
CDK_DEPRECATED_IN_3_4
void          ctk_handle_box_set_snap_edge        (CtkHandleBox    *handle_box,
						   CtkPositionType  edge);
CDK_DEPRECATED_IN_3_4
CtkPositionType ctk_handle_box_get_snap_edge      (CtkHandleBox    *handle_box);
CDK_DEPRECATED_IN_3_4
gboolean      ctk_handle_box_get_child_detached   (CtkHandleBox    *handle_box);

G_END_DECLS

#endif /* __CTK_HANDLE_BOX_H__ */
