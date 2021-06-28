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

#ifndef __CTK_CHECK_BUTTON_H__
#define __CTK_CHECK_BUTTON_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctktogglebutton.h>


G_BEGIN_DECLS

#define CTK_TYPE_CHECK_BUTTON                  (ctk_check_button_get_type ())
#define CTK_CHECK_BUTTON(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_CHECK_BUTTON, CtkCheckButton))
#define CTK_CHECK_BUTTON_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_CHECK_BUTTON, CtkCheckButtonClass))
#define CTK_IS_CHECK_BUTTON(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_CHECK_BUTTON))
#define CTK_IS_CHECK_BUTTON_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_CHECK_BUTTON))
#define CTK_CHECK_BUTTON_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_CHECK_BUTTON, CtkCheckButtonClass))


typedef struct _CtkCheckButton       CtkCheckButton;
typedef struct _CtkCheckButtonClass  CtkCheckButtonClass;

struct _CtkCheckButton
{
  CtkToggleButton toggle_button;
};

struct _CtkCheckButtonClass
{
  CtkToggleButtonClass parent_class;

  void (* draw_indicator) (CtkCheckButton *check_button,
			   cairo_t        *cr);

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};


CDK_AVAILABLE_IN_ALL
GType      ctk_check_button_get_type       (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
CtkWidget* ctk_check_button_new               (void);
CDK_AVAILABLE_IN_ALL
CtkWidget* ctk_check_button_new_with_label    (const gchar *label);
CDK_AVAILABLE_IN_ALL
CtkWidget* ctk_check_button_new_with_mnemonic (const gchar *label);

void _ctk_check_button_get_props (CtkCheckButton *check_button,
				  gint           *indicator_size,
				  gint           *indicator_spacing);

G_END_DECLS

#endif /* __CTK_CHECK_BUTTON_H__ */
