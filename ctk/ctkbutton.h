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
 * Modified by the CTK+ Team and others 1997-2001.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CTK_BUTTON_H__
#define __CTK_BUTTON_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkbin.h>
#include <ctk/ctkimage.h>


G_BEGIN_DECLS

#define CTK_TYPE_BUTTON                 (ctk_button_get_type ())
#define CTK_BUTTON(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_BUTTON, CtkButton))
#define CTK_BUTTON_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_BUTTON, CtkButtonClass))
#define CTK_IS_BUTTON(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_BUTTON))
#define CTK_IS_BUTTON_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_BUTTON))
#define CTK_BUTTON_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_BUTTON, CtkButtonClass))

typedef struct _CtkButton             CtkButton;
typedef struct _CtkButtonPrivate      CtkButtonPrivate;
typedef struct _CtkButtonClass        CtkButtonClass;

struct _CtkButton
{
  /*< private >*/
  CtkBin bin;

  CtkButtonPrivate *priv;
};

/**
 * CtkButtonClass:
 * @parent_class: The parent class.
 * @pressed: Signal emitted when the button is pressed. Deprecated: 2.8.
 * @released: Signal emitted when the button is released. Deprecated: 2.8.
 * @clicked: Signal emitted when the button has been activated (pressed and released).
 * @enter: Signal emitted when the pointer enters the button. Deprecated: 2.8.
 * @leave: Signal emitted when the pointer leaves the button. Deprecated: 2.8.
 * @activate: Signal that causes the button to animate press then
 *    release. Applications should never connect to this signal, but use
 *    the @clicked signal.
 */
struct _CtkButtonClass
{
  CtkBinClass        parent_class;

  /*< public >*/

  void (* pressed)  (CtkButton *button);
  void (* released) (CtkButton *button);
  void (* clicked)  (CtkButton *button);
  void (* enter)    (CtkButton *button);
  void (* leave)    (CtkButton *button);
  void (* activate) (CtkButton *button);

  /*< private >*/

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};


GDK_AVAILABLE_IN_ALL
GType          ctk_button_get_type          (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
CtkWidget*     ctk_button_new               (void);
GDK_AVAILABLE_IN_ALL
CtkWidget*     ctk_button_new_with_label    (const gchar    *label);
GDK_AVAILABLE_IN_3_10
CtkWidget*     ctk_button_new_from_icon_name (const gchar    *icon_name,
					      CtkIconSize     size);
GDK_DEPRECATED_IN_3_10_FOR(ctk_button_new_with_label)
CtkWidget*     ctk_button_new_from_stock    (const gchar    *stock_id);
GDK_AVAILABLE_IN_ALL
CtkWidget*     ctk_button_new_with_mnemonic (const gchar    *label);
GDK_AVAILABLE_IN_ALL
void           ctk_button_clicked           (CtkButton      *button);
GDK_DEPRECATED
void           ctk_button_pressed           (CtkButton      *button);
GDK_DEPRECATED
void           ctk_button_released          (CtkButton      *button);
GDK_DEPRECATED
void           ctk_button_enter             (CtkButton      *button);
GDK_DEPRECATED
void           ctk_button_leave             (CtkButton      *button);

GDK_AVAILABLE_IN_ALL
void                  ctk_button_set_relief         (CtkButton      *button,
						     CtkReliefStyle  relief);
GDK_AVAILABLE_IN_ALL
CtkReliefStyle        ctk_button_get_relief         (CtkButton      *button);
GDK_AVAILABLE_IN_ALL
void                  ctk_button_set_label          (CtkButton      *button,
						     const gchar    *label);
GDK_AVAILABLE_IN_ALL
const gchar *         ctk_button_get_label          (CtkButton      *button);
GDK_AVAILABLE_IN_ALL
void                  ctk_button_set_use_underline  (CtkButton      *button,
						     gboolean        use_underline);
GDK_AVAILABLE_IN_ALL
gboolean              ctk_button_get_use_underline  (CtkButton      *button);
GDK_DEPRECATED_IN_3_10
void                  ctk_button_set_use_stock      (CtkButton      *button,
						     gboolean        use_stock);
GDK_DEPRECATED_IN_3_10
gboolean              ctk_button_get_use_stock      (CtkButton      *button);
GDK_DEPRECATED_IN_3_20_FOR(ctk_widget_set_focus_on_click)
void                  ctk_button_set_focus_on_click (CtkButton      *button,
						     gboolean        focus_on_click);
GDK_DEPRECATED_IN_3_20_FOR(ctk_widget_get_focus_on_click)
gboolean              ctk_button_get_focus_on_click (CtkButton      *button);
GDK_DEPRECATED_IN_3_14
void                  ctk_button_set_alignment      (CtkButton      *button,
						     gfloat          xalign,
						     gfloat          yalign);
GDK_DEPRECATED_IN_3_14
void                  ctk_button_get_alignment      (CtkButton      *button,
						     gfloat         *xalign,
						     gfloat         *yalign);
GDK_AVAILABLE_IN_ALL
void                  ctk_button_set_image          (CtkButton      *button,
					             CtkWidget      *image);
GDK_AVAILABLE_IN_ALL
CtkWidget*            ctk_button_get_image          (CtkButton      *button);
GDK_AVAILABLE_IN_ALL
void                  ctk_button_set_image_position (CtkButton      *button,
						     CtkPositionType position);
GDK_AVAILABLE_IN_ALL
CtkPositionType       ctk_button_get_image_position (CtkButton      *button);
GDK_AVAILABLE_IN_3_6
void                  ctk_button_set_always_show_image (CtkButton   *button,
                                                        gboolean     always_show);
GDK_AVAILABLE_IN_3_6
gboolean              ctk_button_get_always_show_image (CtkButton   *button);

GDK_AVAILABLE_IN_ALL
GdkWindow*            ctk_button_get_event_window   (CtkButton      *button);


G_END_DECLS

#endif /* __CTK_BUTTON_H__ */
