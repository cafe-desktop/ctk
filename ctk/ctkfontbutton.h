/* CTK - The GIMP Toolkit
 * Copyright (C) 1998 David Abilleira Freijeiro <odaf@nexo.es>
 * All rights reserved
 * Based on gnome-color-picker by Federico Mena <federico@nuclecu.unam.mx>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */
/*
 * Modified by the CTK+ Team and others 2003.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CTK_FONT_BUTTON_H__
#define __CTK_FONT_BUTTON_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkbutton.h>


G_BEGIN_DECLS

/* CtkFontButton is a button widget that allow user to select a font.
 */

#define CTK_TYPE_FONT_BUTTON             (ctk_font_button_get_type ())
#define CTK_FONT_BUTTON(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_FONT_BUTTON, CtkFontButton))
#define CTK_FONT_BUTTON_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_FONT_BUTTON, CtkFontButtonClass))
#define CTK_IS_FONT_BUTTON(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_FONT_BUTTON))
#define CTK_IS_FONT_BUTTON_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_FONT_BUTTON))
#define CTK_FONT_BUTTON_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_FONT_BUTTON, CtkFontButtonClass))

typedef struct _CtkFontButton        CtkFontButton;
typedef struct _CtkFontButtonClass   CtkFontButtonClass;
typedef struct _CtkFontButtonPrivate CtkFontButtonPrivate;

struct _CtkFontButton {
  CtkButton button;

  /*< private >*/
  CtkFontButtonPrivate *priv;
};

struct _CtkFontButtonClass {
  CtkButtonClass parent_class;

  /* font_set signal is emitted when font is chosen */
  void (* font_set) (CtkFontButton *gfp);

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};


CDK_AVAILABLE_IN_ALL
GType                 ctk_font_button_get_type       (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
CtkWidget            *ctk_font_button_new            (void);
CDK_AVAILABLE_IN_ALL
CtkWidget            *ctk_font_button_new_with_font  (const gchar   *fontname);

CDK_AVAILABLE_IN_ALL
const gchar *         ctk_font_button_get_title      (CtkFontButton *font_button);
CDK_AVAILABLE_IN_ALL
void                  ctk_font_button_set_title      (CtkFontButton *font_button,
                                                      const gchar   *title);
CDK_AVAILABLE_IN_ALL
gboolean              ctk_font_button_get_use_font   (CtkFontButton *font_button);
CDK_AVAILABLE_IN_ALL
void                  ctk_font_button_set_use_font   (CtkFontButton *font_button,
                                                      gboolean       use_font);
CDK_AVAILABLE_IN_ALL
gboolean              ctk_font_button_get_use_size   (CtkFontButton *font_button);
CDK_AVAILABLE_IN_ALL
void                  ctk_font_button_set_use_size   (CtkFontButton *font_button,
                                                      gboolean       use_size);
CDK_DEPRECATED_IN_3_22
const gchar *         ctk_font_button_get_font_name  (CtkFontButton *font_button);
CDK_DEPRECATED_IN_3_22
gboolean              ctk_font_button_set_font_name  (CtkFontButton *font_button,
                                                      const gchar   *fontname);
CDK_AVAILABLE_IN_ALL
gboolean              ctk_font_button_get_show_style (CtkFontButton *font_button);
CDK_AVAILABLE_IN_ALL
void                  ctk_font_button_set_show_style (CtkFontButton *font_button,
                                                      gboolean       show_style);
CDK_AVAILABLE_IN_ALL
gboolean              ctk_font_button_get_show_size  (CtkFontButton *font_button);
CDK_AVAILABLE_IN_ALL
void                  ctk_font_button_set_show_size  (CtkFontButton *font_button,
                                                      gboolean       show_size);

G_END_DECLS


#endif /* __CTK_FONT_BUTTON_H__ */
