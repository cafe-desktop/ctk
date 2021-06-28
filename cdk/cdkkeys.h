/* GDK - The GIMP Drawing Kit
 * Copyright (C) 2000 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
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

#ifndef __GDK_KEYS_H__
#define __GDK_KEYS_H__

#if !defined (__GDK_H_INSIDE__) && !defined (GDK_COMPILATION)
#error "Only <cdk/cdk.h> can be included directly."
#endif

#include <cdk/cdkversionmacros.h>
#include <cdk/cdktypes.h>

G_BEGIN_DECLS


typedef struct _CdkKeymapKey CdkKeymapKey;

/**
 * CdkKeymapKey:
 * @keycode: the hardware keycode. This is an identifying number for a
 *   physical key.
 * @group: indicates movement in a horizontal direction. Usually groups are used
 *   for two different languages. In group 0, a key might have two English
 *   characters, and in group 1 it might have two Hebrew characters. The Hebrew
 *   characters will be printed on the key next to the English characters.
 * @level: indicates which symbol on the key will be used, in a vertical direction.
 *   So on a standard US keyboard, the key with the number “1” on it also has the
 *   exclamation point ("!") character on it. The level indicates whether to use
 *   the “1” or the “!” symbol. The letter keys are considered to have a lowercase
 *   letter at level 0, and an uppercase letter at level 1, though only the
 *   uppercase letter is printed.
 *
 * A #CdkKeymapKey is a hardware key that can be mapped to a keyval.
 */
struct _CdkKeymapKey
{
  guint keycode;
  gint  group;
  gint  level;
};


#define GDK_TYPE_KEYMAP              (cdk_keymap_get_type ())
#define GDK_KEYMAP(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_KEYMAP, CdkKeymap))
#define GDK_IS_KEYMAP(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_KEYMAP))

/**
 * CdkKeymap:
 *
 * A #CdkKeymap defines the translation from keyboard state
 * (including a hardware key, a modifier mask, and active keyboard group)
 * to a keyval. This translation has two phases. The first phase is
 * to determine the effective keyboard group and level for the keyboard
 * state; the second phase is to look up the keycode/group/level triplet
 * in the keymap and see what keyval it corresponds to.
 */

GDK_AVAILABLE_IN_ALL
GType cdk_keymap_get_type (void) G_GNUC_CONST;

GDK_DEPRECATED_IN_3_22_FOR(cdk_keymap_get_for_display)
CdkKeymap* cdk_keymap_get_default     (void);
GDK_AVAILABLE_IN_ALL
CdkKeymap* cdk_keymap_get_for_display (CdkDisplay *display);

GDK_AVAILABLE_IN_ALL
guint          cdk_keymap_lookup_key               (CdkKeymap           *keymap,
						    const CdkKeymapKey  *key);
GDK_AVAILABLE_IN_ALL
gboolean       cdk_keymap_translate_keyboard_state (CdkKeymap           *keymap,
						    guint                hardware_keycode,
						    CdkModifierType      state,
						    gint                 group,
						    guint               *keyval,
						    gint                *effective_group,
						    gint                *level,
						    CdkModifierType     *consumed_modifiers);
GDK_AVAILABLE_IN_ALL
gboolean       cdk_keymap_get_entries_for_keyval   (CdkKeymap           *keymap,
						    guint                keyval,
						    CdkKeymapKey       **keys,
						    gint                *n_keys);
GDK_AVAILABLE_IN_ALL
gboolean       cdk_keymap_get_entries_for_keycode  (CdkKeymap           *keymap,
						    guint                hardware_keycode,
						    CdkKeymapKey       **keys,
						    guint              **keyvals,
						    gint                *n_entries);

GDK_AVAILABLE_IN_ALL
PangoDirection cdk_keymap_get_direction            (CdkKeymap           *keymap);
GDK_AVAILABLE_IN_ALL
gboolean       cdk_keymap_have_bidi_layouts        (CdkKeymap           *keymap);
GDK_AVAILABLE_IN_ALL
gboolean       cdk_keymap_get_caps_lock_state      (CdkKeymap           *keymap);
GDK_AVAILABLE_IN_ALL
gboolean       cdk_keymap_get_num_lock_state       (CdkKeymap           *keymap);
GDK_AVAILABLE_IN_3_18
gboolean       cdk_keymap_get_scroll_lock_state    (CdkKeymap           *keymap);
GDK_AVAILABLE_IN_3_4
guint          cdk_keymap_get_modifier_state       (CdkKeymap           *keymap);
GDK_AVAILABLE_IN_ALL
void           cdk_keymap_add_virtual_modifiers    (CdkKeymap           *keymap,
                                                    CdkModifierType     *state);
GDK_AVAILABLE_IN_ALL
gboolean       cdk_keymap_map_virtual_modifiers    (CdkKeymap           *keymap,
                                                    CdkModifierType     *state);
GDK_AVAILABLE_IN_3_4
CdkModifierType cdk_keymap_get_modifier_mask       (CdkKeymap           *keymap,
                                                    CdkModifierIntent    intent);


/* Key values
 */
GDK_AVAILABLE_IN_ALL
gchar*   cdk_keyval_name         (guint        keyval) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
guint    cdk_keyval_from_name    (const gchar *keyval_name);
GDK_AVAILABLE_IN_ALL
void     cdk_keyval_convert_case (guint        symbol,
				  guint       *lower,
				  guint       *upper);
GDK_AVAILABLE_IN_ALL
guint    cdk_keyval_to_upper     (guint        keyval) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
guint    cdk_keyval_to_lower     (guint        keyval) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
gboolean cdk_keyval_is_upper     (guint        keyval) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
gboolean cdk_keyval_is_lower     (guint        keyval) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
guint32  cdk_keyval_to_unicode   (guint        keyval) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
guint    cdk_unicode_to_keyval   (guint32      wc) G_GNUC_CONST;


G_END_DECLS

#endif /* __GDK_KEYS_H__ */
