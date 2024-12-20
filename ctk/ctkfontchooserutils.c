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
 * Based on ctkfilechooserutils.c:
 *	Copyright (C) 2003 Red Hat, Inc.
 */

#include "config.h"

#include "ctkfontchooserutils.h"

static CtkFontChooser *
get_delegate (CtkFontChooser *receiver)
{
  return g_object_get_qdata (G_OBJECT (receiver),
                             CTK_FONT_CHOOSER_DELEGATE_QUARK);
}

static PangoFontFamily *
delegate_get_font_family (CtkFontChooser *chooser)
{
  return ctk_font_chooser_get_font_family (get_delegate (chooser));
}

static PangoFontFace *
delegate_get_font_face (CtkFontChooser *chooser)
{
  return ctk_font_chooser_get_font_face (get_delegate (chooser));
}

static int
delegate_get_font_size (CtkFontChooser *chooser)
{
  return ctk_font_chooser_get_font_size (get_delegate (chooser));
}

static void
delegate_set_filter_func (CtkFontChooser    *chooser,
                          CtkFontFilterFunc  filter_func,
                          gpointer           filter_data,
                          GDestroyNotify     data_destroy)
{
  ctk_font_chooser_set_filter_func (get_delegate (chooser),
                                    filter_func,
                                    filter_data,
                                    data_destroy);
}

static void
delegate_set_font_map (CtkFontChooser *chooser,
                       PangoFontMap   *map)
{
  ctk_font_chooser_set_font_map (get_delegate (chooser), map);
}

static PangoFontMap *
delegate_get_font_map (CtkFontChooser *chooser)
{
  return ctk_font_chooser_get_font_map (get_delegate (chooser));
}

static void
delegate_notify (GObject    *object,
                 GParamSpec *pspec,
                 gpointer    user_data)
{
  gpointer iface;

  iface = g_type_interface_peek (g_type_class_peek (G_OBJECT_TYPE (object)),
                                 CTK_TYPE_FONT_CHOOSER);
  if (g_object_interface_find_property (iface, pspec->name))
    g_object_notify_by_pspec (user_data, pspec);
}

static void
delegate_font_activated (CtkFontChooser *receiver G_GNUC_UNUSED,
                         const gchar    *fontname,
                         CtkFontChooser *delegate)
{
  _ctk_font_chooser_font_activated (delegate, fontname);
}

GQuark
_ctk_font_chooser_delegate_get_quark (void)
{
  static GQuark quark = 0;

  if (G_UNLIKELY (quark == 0))
    quark = g_quark_from_static_string ("ctk-font-chooser-delegate");

  return quark;
}

/**
 * _ctk_font_chooser_install_properties:
 * @klass: the class structure for a type deriving from #GObject
 *
 * Installs the necessary properties for a class implementing
 * #CtkFontChooser. A #CtkParamSpecOverride property is installed
 * for each property, using the values from the #CtkFontChooserProp
 * enumeration. The caller must make sure itself that the enumeration
 * values don’t collide with some other property values they
 * are using.
 */
void
_ctk_font_chooser_install_properties (GObjectClass *klass)
{
  g_object_class_override_property (klass,
                                    CTK_FONT_CHOOSER_PROP_FONT,
                                    "font");
  g_object_class_override_property (klass,
                                    CTK_FONT_CHOOSER_PROP_FONT_DESC,
                                    "font-desc");
  g_object_class_override_property (klass,
                                    CTK_FONT_CHOOSER_PROP_PREVIEW_TEXT,
                                    "preview-text");
  g_object_class_override_property (klass,
                                    CTK_FONT_CHOOSER_PROP_SHOW_PREVIEW_ENTRY,
                                    "show-preview-entry");
  g_object_class_override_property (klass,
                                    CTK_FONT_CHOOSER_PROP_LEVEL,
                                    "level");
  g_object_class_override_property (klass,
                                    CTK_FONT_CHOOSER_PROP_FONT_FEATURES,
                                    "font-features");
  g_object_class_override_property (klass,
                                    CTK_FONT_CHOOSER_PROP_LANGUAGE,
                                    "language");
}

/**
 * _ctk_font_chooser_delegate_iface_init:
 * @iface: a #CtkFontChooserIface
 *
 * An interface-initialization function for use in cases where
 * an object is simply delegating the methods, signals of
 * the #CtkFontChooser interface to another object.
 * _ctk_font_chooser_set_delegate() must be called on each
 * instance of the object so that the delegate object can
 * be found.
 */
void
_ctk_font_chooser_delegate_iface_init (CtkFontChooserIface *iface)
{
  iface->get_font_family = delegate_get_font_family;
  iface->get_font_face = delegate_get_font_face;
  iface->get_font_size = delegate_get_font_size;
  iface->set_filter_func = delegate_set_filter_func;
  iface->set_font_map = delegate_set_font_map;
  iface->get_font_map = delegate_get_font_map;
}

/**
 * _ctk_font_chooser_set_delegate:
 * @receiver: a #GObject implementing #CtkFontChooser
 * @delegate: another #GObject implementing #CtkFontChooser
 *
 * Establishes that calls on @receiver for #CtkFontChooser
 * methods should be delegated to @delegate, and that
 * #CtkFontChooser signals emitted on @delegate should be
 * forwarded to @receiver. Must be used in conjunction with
 * _ctk_font_chooser_delegate_iface_init().
 */
void
_ctk_font_chooser_set_delegate (CtkFontChooser *receiver,
                                CtkFontChooser *delegate)
{
  g_return_if_fail (CTK_IS_FONT_CHOOSER (receiver));
  g_return_if_fail (CTK_IS_FONT_CHOOSER (delegate));
  
  g_object_set_qdata (G_OBJECT (receiver),
                      CTK_FONT_CHOOSER_DELEGATE_QUARK,
  		      delegate);
  
  g_signal_connect (delegate, "notify",
  		    G_CALLBACK (delegate_notify), receiver);
  g_signal_connect (delegate, "font-activated",
  		    G_CALLBACK (delegate_font_activated), receiver);
}
