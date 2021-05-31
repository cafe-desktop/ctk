/* ctkemojichooser.h: An Emoji chooser widget
 * Copyright 2017, Red Hat, Inc.
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

#pragma once

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkwidget.h>

G_BEGIN_DECLS

#define CTK_TYPE_EMOJI_CHOOSER                 (ctk_emoji_chooser_get_type ())
#define CTK_EMOJI_CHOOSER(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_EMOJI_CHOOSER, CtkEmojiChooser))
#define CTK_EMOJI_CHOOSER_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_EMOJI_CHOOSER, CtkEmojiChooserClass))
#define CTK_IS_EMOJI_CHOOSER(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_EMOJI_CHOOSER))
#define CTK_IS_EMOJI_CHOOSER_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_EMOJI_CHOOSER))
#define CTK_EMOJI_CHOOSER_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_EMOJI_CHOOSER, CtkEmojiChooserClass))

typedef struct _CtkEmojiChooser      CtkEmojiChooser;
typedef struct _CtkEmojiChooserClass CtkEmojiChooserClass;

GType      ctk_emoji_chooser_get_type (void) G_GNUC_CONST;
CtkWidget *ctk_emoji_chooser_new      (void);

G_END_DECLS
