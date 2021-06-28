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

#ifndef __CTK_BUTTON_BOX_H__
#define __CTK_BUTTON_BOX_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkbox.h>


G_BEGIN_DECLS  

#define CTK_TYPE_BUTTON_BOX             (ctk_button_box_get_type ())
#define CTK_BUTTON_BOX(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_BUTTON_BOX, CtkButtonBox))
#define CTK_BUTTON_BOX_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_BUTTON_BOX, CtkButtonBoxClass))
#define CTK_IS_BUTTON_BOX(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_BUTTON_BOX))
#define CTK_IS_BUTTON_BOX_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_BUTTON_BOX))
#define CTK_BUTTON_BOX_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_BUTTON_BOX, CtkButtonBoxClass))


typedef struct _CtkButtonBox              CtkButtonBox;
typedef struct _CtkButtonBoxPrivate       CtkButtonBoxPrivate;
typedef struct _CtkButtonBoxClass         CtkButtonBoxClass;

struct _CtkButtonBox
{
  CtkBox box;

  /*< private >*/
  CtkButtonBoxPrivate *priv;
};

/**
 * CtkButtonBoxClass:
 * @parent_class: The parent class.
 */
struct _CtkButtonBoxClass
{
  CtkBoxClass parent_class;

  /*< private >*/

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};


/**
 * CtkButtonBoxStyle:
 * @CTK_BUTTONBOX_SPREAD: Buttons are evenly spread across the box.
 * @CTK_BUTTONBOX_EDGE: Buttons are placed at the edges of the box.
 * @CTK_BUTTONBOX_START: Buttons are grouped towards the start of the box,
 *   (on the left for a HBox, or the top for a VBox).
 * @CTK_BUTTONBOX_END: Buttons are grouped towards the end of the box,
 *   (on the right for a HBox, or the bottom for a VBox).
 * @CTK_BUTTONBOX_CENTER: Buttons are centered in the box. Since 2.12.
 * @CTK_BUTTONBOX_EXPAND: Buttons expand to fill the box. This entails giving
 *   buttons a "linked" appearance, making button sizes homogeneous, and
 *   setting spacing to 0 (same as calling ctk_box_set_homogeneous() and
 *   ctk_box_set_spacing() manually). Since 3.12.
 *
 * Used to dictate the style that a #CtkButtonBox uses to layout the buttons it
 * contains.
 */
typedef enum
{
  CTK_BUTTONBOX_SPREAD = 1,
  CTK_BUTTONBOX_EDGE,
  CTK_BUTTONBOX_START,
  CTK_BUTTONBOX_END,
  CTK_BUTTONBOX_CENTER,
  CTK_BUTTONBOX_EXPAND
} CtkButtonBoxStyle;


CDK_AVAILABLE_IN_ALL
GType             ctk_button_box_get_type            (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
CtkWidget       * ctk_button_box_new                 (CtkOrientation     orientation);
CDK_AVAILABLE_IN_ALL
CtkButtonBoxStyle ctk_button_box_get_layout          (CtkButtonBox      *widget);
CDK_AVAILABLE_IN_ALL
void              ctk_button_box_set_layout          (CtkButtonBox      *widget,
                                                      CtkButtonBoxStyle  layout_style);
CDK_AVAILABLE_IN_ALL
gboolean          ctk_button_box_get_child_secondary (CtkButtonBox      *widget,
                                                      CtkWidget         *child);
CDK_AVAILABLE_IN_ALL
void              ctk_button_box_set_child_secondary (CtkButtonBox      *widget,
                                                      CtkWidget         *child,
                                                      gboolean           is_secondary);
CDK_AVAILABLE_IN_3_2
gboolean          ctk_button_box_get_child_non_homogeneous (CtkButtonBox *widget,
                                                            CtkWidget    *child);
CDK_AVAILABLE_IN_3_2
void              ctk_button_box_set_child_non_homogeneous (CtkButtonBox *widget,
                                                            CtkWidget    *child,
                                                            gboolean      non_homogeneous);


G_END_DECLS

#endif /* __CTK_BUTTON_BOX_H__ */
