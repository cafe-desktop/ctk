/* GTK - The GIMP Toolkit
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
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 */

#ifndef __CTK_BUTTON_BOX_H__
#define __CTK_BUTTON_BOX_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <gtk/gtk.h> can be included directly."
#endif

#include <gtk/gtkbox.h>


G_BEGIN_DECLS  

#define CTK_TYPE_BUTTON_BOX             (ctk_button_box_get_type ())
#define CTK_BUTTON_BOX(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_BUTTON_BOX, GtkButtonBox))
#define CTK_BUTTON_BOX_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_BUTTON_BOX, GtkButtonBoxClass))
#define CTK_IS_BUTTON_BOX(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_BUTTON_BOX))
#define CTK_IS_BUTTON_BOX_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_BUTTON_BOX))
#define CTK_BUTTON_BOX_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_BUTTON_BOX, GtkButtonBoxClass))


typedef struct _GtkButtonBox              GtkButtonBox;
typedef struct _GtkButtonBoxPrivate       GtkButtonBoxPrivate;
typedef struct _GtkButtonBoxClass         GtkButtonBoxClass;

struct _GtkButtonBox
{
  GtkBox box;

  /*< private >*/
  GtkButtonBoxPrivate *priv;
};

/**
 * GtkButtonBoxClass:
 * @parent_class: The parent class.
 */
struct _GtkButtonBoxClass
{
  GtkBoxClass parent_class;

  /*< private >*/

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};


/**
 * GtkButtonBoxStyle:
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
 * Used to dictate the style that a #GtkButtonBox uses to layout the buttons it
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
} GtkButtonBoxStyle;


GDK_AVAILABLE_IN_ALL
GType             ctk_button_box_get_type            (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
GtkWidget       * ctk_button_box_new                 (GtkOrientation     orientation);
GDK_AVAILABLE_IN_ALL
GtkButtonBoxStyle ctk_button_box_get_layout          (GtkButtonBox      *widget);
GDK_AVAILABLE_IN_ALL
void              ctk_button_box_set_layout          (GtkButtonBox      *widget,
                                                      GtkButtonBoxStyle  layout_style);
GDK_AVAILABLE_IN_ALL
gboolean          ctk_button_box_get_child_secondary (GtkButtonBox      *widget,
                                                      GtkWidget         *child);
GDK_AVAILABLE_IN_ALL
void              ctk_button_box_set_child_secondary (GtkButtonBox      *widget,
                                                      GtkWidget         *child,
                                                      gboolean           is_secondary);
GDK_AVAILABLE_IN_3_2
gboolean          ctk_button_box_get_child_non_homogeneous (GtkButtonBox *widget,
                                                            GtkWidget    *child);
GDK_AVAILABLE_IN_3_2
void              ctk_button_box_set_child_non_homogeneous (GtkButtonBox *widget,
                                                            GtkWidget    *child,
                                                            gboolean      non_homogeneous);


G_END_DECLS

#endif /* __CTK_BUTTON_BOX_H__ */
