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
 * GTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CTK_TOGGLE_BUTTON_H__
#define __CTK_TOGGLE_BUTTON_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkbutton.h>


G_BEGIN_DECLS

#define CTK_TYPE_TOGGLE_BUTTON                  (ctk_toggle_button_get_type ())
#define CTK_TOGGLE_BUTTON(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_TOGGLE_BUTTON, CtkToggleButton))
#define CTK_TOGGLE_BUTTON_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_TOGGLE_BUTTON, CtkToggleButtonClass))
#define CTK_IS_TOGGLE_BUTTON(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_TOGGLE_BUTTON))
#define CTK_IS_TOGGLE_BUTTON_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_TOGGLE_BUTTON))
#define CTK_TOGGLE_BUTTON_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_TOGGLE_BUTTON, CtkToggleButtonClass))

typedef struct _CtkToggleButton              CtkToggleButton;
typedef struct _CtkToggleButtonPrivate       CtkToggleButtonPrivate;
typedef struct _CtkToggleButtonClass         CtkToggleButtonClass;

struct _CtkToggleButton
{
  /*< private >*/
  CtkButton button;

  CtkToggleButtonPrivate *priv;
};

struct _CtkToggleButtonClass
{
  CtkButtonClass parent_class;

  void (* toggled) (CtkToggleButton *toggle_button);

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};


GDK_AVAILABLE_IN_ALL
GType      ctk_toggle_button_get_type          (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
CtkWidget* ctk_toggle_button_new               (void);
GDK_AVAILABLE_IN_ALL
CtkWidget* ctk_toggle_button_new_with_label    (const gchar     *label);
GDK_AVAILABLE_IN_ALL
CtkWidget* ctk_toggle_button_new_with_mnemonic (const gchar     *label);
GDK_AVAILABLE_IN_ALL
void       ctk_toggle_button_set_mode          (CtkToggleButton *toggle_button,
                                                gboolean         draw_indicator);
GDK_AVAILABLE_IN_ALL
gboolean   ctk_toggle_button_get_mode          (CtkToggleButton *toggle_button);
GDK_AVAILABLE_IN_ALL
void       ctk_toggle_button_set_active        (CtkToggleButton *toggle_button,
                                                gboolean         is_active);
GDK_AVAILABLE_IN_ALL
gboolean   ctk_toggle_button_get_active        (CtkToggleButton *toggle_button);
GDK_AVAILABLE_IN_ALL
void       ctk_toggle_button_toggled           (CtkToggleButton *toggle_button);
GDK_AVAILABLE_IN_ALL
void       ctk_toggle_button_set_inconsistent  (CtkToggleButton *toggle_button,
                                                gboolean         setting);
GDK_AVAILABLE_IN_ALL
gboolean   ctk_toggle_button_get_inconsistent  (CtkToggleButton *toggle_button);


G_END_DECLS

#endif /* __CTK_TOGGLE_BUTTON_H__ */
