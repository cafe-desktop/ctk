/* CTK - The GIMP Toolkit
 * ctkfilechooserembed.h: Abstract sizing interface for file selector implementations
 * Copyright (C) 2004, Red Hat, Inc.
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

#ifndef __CTK_FILE_CHOOSER_EMBED_H__
#define __CTK_FILE_CHOOSER_EMBED_H__

#include <ctk/ctkwidget.h>

G_BEGIN_DECLS

#define CTK_TYPE_FILE_CHOOSER_EMBED             (_ctk_file_chooser_embed_get_type ())
#define CTK_FILE_CHOOSER_EMBED(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_FILE_CHOOSER_EMBED, CtkFileChooserEmbed))
#define CTK_IS_FILE_CHOOSER_EMBED(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_FILE_CHOOSER_EMBED))
#define CTK_FILE_CHOOSER_EMBED_GET_IFACE(obj)   (G_TYPE_INSTANCE_GET_INTERFACE ((obj), CTK_TYPE_FILE_CHOOSER_EMBED, CtkFileChooserEmbedIface))

typedef struct _CtkFileChooserEmbed      CtkFileChooserEmbed;
typedef struct _CtkFileChooserEmbedIface CtkFileChooserEmbedIface;


struct _CtkFileChooserEmbedIface
{
  GTypeInterface base_iface;

  /* Methods
   */
  void (*get_default_size)        (CtkFileChooserEmbed *chooser_embed,
				   gint                *default_width,
				   gint                *default_height);

  gboolean (*should_respond)      (CtkFileChooserEmbed *chooser_embed);

  void (*initial_focus)           (CtkFileChooserEmbed *chooser_embed);
  /* Signals
   */
  void (*default_size_changed)    (CtkFileChooserEmbed *chooser_embed);
  void (*response_requested)      (CtkFileChooserEmbed *chooser_embed);
};

GType _ctk_file_chooser_embed_get_type (void) G_GNUC_CONST;

void  _ctk_file_chooser_embed_get_default_size    (CtkFileChooserEmbed *chooser_embed,
						   gint                *default_width,
						   gint                *default_height);
gboolean _ctk_file_chooser_embed_should_respond (CtkFileChooserEmbed *chooser_embed);

void _ctk_file_chooser_embed_initial_focus (CtkFileChooserEmbed *chooser_embed);

void _ctk_file_chooser_embed_delegate_iface_init  (CtkFileChooserEmbedIface *iface);
void _ctk_file_chooser_embed_set_delegate         (CtkFileChooserEmbed *receiver,
						   CtkFileChooserEmbed *delegate);

G_END_DECLS

#endif /* __CTK_FILE_CHOOSER_EMBED_H__ */
