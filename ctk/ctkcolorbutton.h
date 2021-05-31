/*
 * CTK - The GIMP Toolkit
 * Copyright (C) 1998, 1999 Red Hat, Inc.
 * All rights reserved.
 *
 * This Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

/* Color picker button for GNOME
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 *
 * Modified by the CTK+ Team and others 2003.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CTK_COLOR_BUTTON_H__
#define __CTK_COLOR_BUTTON_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkbutton.h>

G_BEGIN_DECLS


#define CTK_TYPE_COLOR_BUTTON             (ctk_color_button_get_type ())
#define CTK_COLOR_BUTTON(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_COLOR_BUTTON, CtkColorButton))
#define CTK_COLOR_BUTTON_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_COLOR_BUTTON, CtkColorButtonClass))
#define CTK_IS_COLOR_BUTTON(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_COLOR_BUTTON))
#define CTK_IS_COLOR_BUTTON_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_COLOR_BUTTON))
#define CTK_COLOR_BUTTON_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_COLOR_BUTTON, CtkColorButtonClass))

typedef struct _CtkColorButton          CtkColorButton;
typedef struct _CtkColorButtonClass     CtkColorButtonClass;
typedef struct _CtkColorButtonPrivate   CtkColorButtonPrivate;

struct _CtkColorButton {
  CtkButton button;

  /*< private >*/
  CtkColorButtonPrivate *priv;
};

struct _CtkColorButtonClass {
  CtkButtonClass parent_class;

  void (* color_set) (CtkColorButton *cp);

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};


GDK_AVAILABLE_IN_ALL
GType        ctk_color_button_get_type      (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
CtkWidget *  ctk_color_button_new           (void);
GDK_AVAILABLE_IN_ALL
CtkWidget *  ctk_color_button_new_with_rgba (const GdkRGBA  *rgba);
GDK_AVAILABLE_IN_ALL
void         ctk_color_button_set_title     (CtkColorButton *button,
                                             const gchar    *title);
GDK_AVAILABLE_IN_ALL
const gchar *ctk_color_button_get_title     (CtkColorButton *button);

GDK_DEPRECATED_IN_3_4_FOR(ctk_color_button_new_with_rgba)
CtkWidget *ctk_color_button_new_with_color (const GdkColor *color);
GDK_DEPRECATED_IN_3_4_FOR(ctk_color_chooser_set_rgba)
void       ctk_color_button_set_color      (CtkColorButton *button,
                                            const GdkColor *color);
GDK_DEPRECATED_IN_3_4_FOR(ctk_color_chooser_get_rgba)
void       ctk_color_button_get_color      (CtkColorButton *button,
                                            GdkColor       *color);
GDK_DEPRECATED_IN_3_4_FOR(ctk_color_chooser_set_rgba)
void       ctk_color_button_set_alpha      (CtkColorButton *button,
                                            guint16         alpha);
GDK_DEPRECATED_IN_3_4_FOR(ctk_color_chooser_get_rgba)
guint16    ctk_color_button_get_alpha      (CtkColorButton *button);
GDK_DEPRECATED_IN_3_4_FOR(ctk_color_chooser_set_use_alpha)
void         ctk_color_button_set_use_alpha (CtkColorButton *button,
                                             gboolean        use_alpha);
GDK_DEPRECATED_IN_3_4_FOR(ctk_color_chooser_get_use_alpha)
gboolean     ctk_color_button_get_use_alpha (CtkColorButton *button);
GDK_DEPRECATED_IN_3_4_FOR(ctk_color_chooser_set_rgba)
void         ctk_color_button_set_rgba      (CtkColorButton *button,
                                             const GdkRGBA  *rgba);
GDK_DEPRECATED_IN_3_4_FOR(ctk_color_chooser_get_rgba)
void         ctk_color_button_get_rgba      (CtkColorButton *button,
                                             GdkRGBA        *rgba);

G_END_DECLS

#endif  /* __CTK_COLOR_BUTTON_H__ */
