/* ctkfontchooserutils.h - Private utility functions for implementing a
 *                           CtkFontChooser interface
 *
 * Copyright (C) 2006 Emmanuele Bassi
 *
 * All rights reserved
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
 *
 * Based on ctkfilechooserutils.h:
 *	Copyright (C) 2003 Red Hat, Inc.
 */
 
#ifndef __CTK_FONT_CHOOSER_UTILS_H__
#define __CTK_FONT_CHOOSER_UTILS_H__

#include "ctkfontchooserprivate.h"

G_BEGIN_DECLS

#define CTK_FONT_CHOOSER_DELEGATE_QUARK	(_ctk_font_chooser_delegate_get_quark ())

typedef enum {
  CTK_FONT_CHOOSER_PROP_FIRST           = 0x4000,
  CTK_FONT_CHOOSER_PROP_FONT,
  CTK_FONT_CHOOSER_PROP_FONT_DESC,
  CTK_FONT_CHOOSER_PROP_PREVIEW_TEXT,
  CTK_FONT_CHOOSER_PROP_SHOW_PREVIEW_ENTRY,
  CTK_FONT_CHOOSER_PROP_LEVEL,
  CTK_FONT_CHOOSER_PROP_FONT_FEATURES,
  CTK_FONT_CHOOSER_PROP_LANGUAGE,
  CTK_FONT_CHOOSER_PROP_LAST
} CtkFontChooserProp;

void   _ctk_font_chooser_install_properties  (GObjectClass          *klass);
void   _ctk_font_chooser_delegate_iface_init (CtkFontChooserIface *iface);
void   _ctk_font_chooser_set_delegate        (CtkFontChooser      *receiver,
                                              CtkFontChooser      *delegate);

GQuark _ctk_font_chooser_delegate_get_quark  (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __CTK_FONT_CHOOSER_UTILS_H__ */
