/* GDK - The GIMP Drawing Kit
 * Copyright (C) 2010 Red Hat, Inc
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

#ifndef __GDK_KEYS_PRIVATE_H__
#define __GDK_KEYS_PRIVATE_H__

#include "cdkkeys.h"

G_BEGIN_DECLS

#define GDK_KEYMAP_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GDK_TYPE_KEYMAP, CdkKeymapClass))
#define GDK_IS_KEYMAP_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GDK_TYPE_KEYMAP))
#define GDK_KEYMAP_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GDK_TYPE_KEYMAP, CdkKeymapClass))

typedef struct _CdkKeymapClass CdkKeymapClass;

struct _CdkKeymapClass
{
  GObjectClass parent_class;

  PangoDirection (* get_direction)      (CdkKeymap *keymap);
  gboolean (* have_bidi_layouts)        (CdkKeymap *keymap);
  gboolean (* get_caps_lock_state)      (CdkKeymap *keymap);
  gboolean (* get_num_lock_state)       (CdkKeymap *keymap);
  gboolean (* get_scroll_lock_state)    (CdkKeymap *keymap);
  gboolean (* get_entries_for_keyval)   (CdkKeymap     *keymap,
                                         guint          keyval,
                                         CdkKeymapKey **keys,
                                         gint          *n_keys);
  gboolean (* get_entries_for_keycode)  (CdkKeymap     *keymap,
                                         guint          hardware_keycode,
                                         CdkKeymapKey **keys,
                                         guint        **keyvals,
                                         gint          *n_entries);
  guint (* lookup_key)                  (CdkKeymap          *keymap,
                                         const CdkKeymapKey *key);
  gboolean (* translate_keyboard_state) (CdkKeymap       *keymap,
                                         guint            hardware_keycode,
                                         CdkModifierType  state,
                                         gint             group,
                                         guint           *keyval,
                                         gint            *effective_group,
                                         gint            *level,
                                         CdkModifierType *consumed_modifiers);
  void (* add_virtual_modifiers)        (CdkKeymap       *keymap,
                                         CdkModifierType *state);
  gboolean (* map_virtual_modifiers)    (CdkKeymap       *keymap,
                                         CdkModifierType *state);
  CdkModifierType (*get_modifier_mask)  (CdkKeymap         *keymap,
                                         CdkModifierIntent  intent);
  guint (* get_modifier_state)          (CdkKeymap *keymap);


  /* Signals */
  void (*direction_changed) (CdkKeymap *keymap);
  void (*keys_changed)      (CdkKeymap *keymap);
  void (*state_changed)     (CdkKeymap *keymap);
};

struct _CdkKeymap
{
  GObject     parent_instance;
  CdkDisplay *display;
};

G_END_DECLS

#endif
