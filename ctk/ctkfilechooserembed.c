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

#include "config.h"
#include "ctkfilechooserembed.h"
#include "ctkmarshalers.h"
#include "ctkintl.h"

static void ctk_file_chooser_embed_class_init (gpointer g_iface);
static void delegate_get_default_size         (CtkFileChooserEmbed *chooser_embed,
					       gint                *default_width,
					       gint                *default_height);
static gboolean delegate_should_respond       (CtkFileChooserEmbed *chooser_embed);
static void delegate_initial_focus            (CtkFileChooserEmbed *chooser_embed);
static void delegate_default_size_changed     (CtkFileChooserEmbed *chooser_embed,
					       gpointer             data);
static void delegate_response_requested       (CtkFileChooserEmbed *chooser_embed,
					       gpointer             data);

static CtkFileChooserEmbed *
get_delegate (CtkFileChooserEmbed *receiver)
{
  return g_object_get_data (G_OBJECT (receiver), "ctk-file-chooser-embed-delegate");
}

/**
 * _ctk_file_chooser_embed_delegate_iface_init:
 * @iface: a #CtkFileChoserEmbedIface structure
 * 
 * An interface-initialization function for use in cases where an object is
 * simply delegating the methods, signals of the #CtkFileChooserEmbed interface
 * to another object.  _ctk_file_chooser_embed_set_delegate() must be called on
 * each instance of the object so that the delegate object can be found.
 **/
void
_ctk_file_chooser_embed_delegate_iface_init (CtkFileChooserEmbedIface *iface)
{
  iface->get_default_size = delegate_get_default_size;
  iface->should_respond = delegate_should_respond;
  iface->initial_focus = delegate_initial_focus;
}

/**
 * _ctk_file_chooser_embed_set_delegate:
 * @receiver: a GOobject implementing #CtkFileChooserEmbed
 * @delegate: another GObject implementing #CtkFileChooserEmbed
 *
 * Establishes that calls on @receiver for #CtkFileChooser methods should be
 * delegated to @delegate, and that #CtkFileChooser signals emitted on @delegate
 * should be forwarded to @receiver. Must be used in conjunction with
 * _ctk_file_chooser_embed_delegate_iface_init().
 **/
void
_ctk_file_chooser_embed_set_delegate (CtkFileChooserEmbed *receiver,
				      CtkFileChooserEmbed *delegate)
{
  g_return_if_fail (CTK_IS_FILE_CHOOSER_EMBED (receiver));
  g_return_if_fail (CTK_IS_FILE_CHOOSER_EMBED (delegate));
  
  g_object_set_data (G_OBJECT (receiver), I_("ctk-file-chooser-embed-delegate"), delegate);

  g_signal_connect (delegate, "default-size-changed",
		    G_CALLBACK (delegate_default_size_changed), receiver);
  g_signal_connect (delegate, "response-requested",
		    G_CALLBACK (delegate_response_requested), receiver);
}



static void
delegate_get_default_size (CtkFileChooserEmbed *chooser_embed,
			   gint                *default_width,
			   gint                *default_height)
{
  _ctk_file_chooser_embed_get_default_size (get_delegate (chooser_embed), default_width, default_height);
}

static gboolean
delegate_should_respond (CtkFileChooserEmbed *chooser_embed)
{
  return _ctk_file_chooser_embed_should_respond (get_delegate (chooser_embed));
}

static void
delegate_initial_focus (CtkFileChooserEmbed *chooser_embed)
{
  _ctk_file_chooser_embed_initial_focus (get_delegate (chooser_embed));
}

static void
delegate_default_size_changed (CtkFileChooserEmbed *chooser_embed G_GNUC_UNUSED,
			       gpointer             data)
{
  g_signal_emit_by_name (data, "default-size-changed");
}

static void
delegate_response_requested (CtkFileChooserEmbed *chooser_embed G_GNUC_UNUSED,
			     gpointer             data)
{
  g_signal_emit_by_name (data, "response-requested");
}


/* publicly callable functions */

GType
_ctk_file_chooser_embed_get_type (void)
{
  static GType file_chooser_embed_type = 0;

  if (!file_chooser_embed_type)
    {
      const GTypeInfo file_chooser_embed_info =
      {
	.class_size = sizeof (CtkFileChooserEmbedIface),
	.class_init = (GClassInitFunc)ctk_file_chooser_embed_class_init
      };

      file_chooser_embed_type = g_type_register_static (G_TYPE_INTERFACE,
							I_("CtkFileChooserEmbed"),
							&file_chooser_embed_info, 0);

      g_type_interface_add_prerequisite (file_chooser_embed_type, CTK_TYPE_WIDGET);
    }

  return file_chooser_embed_type;
}

static void
ctk_file_chooser_embed_class_init (gpointer g_iface)
{
  GType iface_type = G_TYPE_FROM_INTERFACE (g_iface);

  g_signal_new (I_("default-size-changed"),
		iface_type,
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (CtkFileChooserEmbedIface, default_size_changed),
		NULL, NULL,
		NULL,
		G_TYPE_NONE, 0);
  g_signal_new (I_("response-requested"),
		iface_type,
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (CtkFileChooserEmbedIface, response_requested),
		NULL, NULL,
		NULL,
		G_TYPE_NONE, 0);
}

void
_ctk_file_chooser_embed_get_default_size (CtkFileChooserEmbed *chooser_embed,
					 gint                *default_width,
					 gint                *default_height)
{
  g_return_if_fail (CTK_IS_FILE_CHOOSER_EMBED (chooser_embed));
  g_return_if_fail (default_width != NULL);
  g_return_if_fail (default_height != NULL);

  CTK_FILE_CHOOSER_EMBED_GET_IFACE (chooser_embed)->get_default_size (chooser_embed, default_width, default_height);
}

gboolean
_ctk_file_chooser_embed_should_respond (CtkFileChooserEmbed *chooser_embed)
{
  g_return_val_if_fail (CTK_IS_FILE_CHOOSER_EMBED (chooser_embed), FALSE);

  return CTK_FILE_CHOOSER_EMBED_GET_IFACE (chooser_embed)->should_respond (chooser_embed);
}

void
_ctk_file_chooser_embed_initial_focus (CtkFileChooserEmbed *chooser_embed)
{
  g_return_if_fail (CTK_IS_FILE_CHOOSER_EMBED (chooser_embed));

  CTK_FILE_CHOOSER_EMBED_GET_IFACE (chooser_embed)->initial_focus (chooser_embed);
}
