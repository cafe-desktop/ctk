/* ctkshortcutlabelprivate.h
 *
 * Copyright (C) 2015 Christian Hergert <christian@hergert.me>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __CTK_SHORTCUT_LABEL_H__
#define __CTK_SHORTCUT_LABEL_H__

#include <ctk/ctkbox.h>

G_BEGIN_DECLS

#define CTK_TYPE_SHORTCUT_LABEL (ctk_shortcut_label_get_type())
#define CTK_SHORTCUT_LABEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_SHORTCUT_LABEL, CtkShortcutLabel))
#define CTK_SHORTCUT_LABEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_SHORTCUT_LABEL, CtkShortcutLabelClass))
#define CTK_IS_SHORTCUT_LABEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_SHORTCUT_LABEL))
#define CTK_IS_SHORTCUT_LABEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_SHORTCUT_LABEL))
#define CTK_SHORTCUT_LABEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_SHORTCUT_LABEL, CtkShortcutLabelClass))


typedef struct _CtkShortcutLabel      CtkShortcutLabel;
typedef struct _CtkShortcutLabelClass CtkShortcutLabelClass;

CDK_AVAILABLE_IN_3_22
GType        ctk_shortcut_label_get_type        (void) G_GNUC_CONST;

CDK_AVAILABLE_IN_3_22
CtkWidget   *ctk_shortcut_label_new             (const gchar      *accelerator);

CDK_AVAILABLE_IN_3_22
const gchar *ctk_shortcut_label_get_accelerator (CtkShortcutLabel *self);

CDK_AVAILABLE_IN_3_22
void         ctk_shortcut_label_set_accelerator (CtkShortcutLabel *self,
                                                 const gchar      *accelerator);

CDK_AVAILABLE_IN_3_22
const gchar *ctk_shortcut_label_get_disabled_text (CtkShortcutLabel *self);

CDK_AVAILABLE_IN_3_22
void         ctk_shortcut_label_set_disabled_text (CtkShortcutLabel *self,
                                                   const gchar      *disabled_text);

G_END_DECLS

#endif /* __CTK_SHORTCUT_LABEL_H__ */
