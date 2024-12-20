/* CDK - The GIMP Drawing Kit
 * Copyright (C) 2000 Red Hat, Inc.
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

#include "config.h"

#include "cdkkeysyms.h"
#include "cdkkeysprivate.h"
#include "cdkdisplay.h"
#include "cdkdisplaymanagerprivate.h"


/**
 * SECTION:keys
 * @Short_description: Functions for manipulating keyboard codes
 * @Title: Key Values
 *
 * Key values are the codes which are sent whenever a key is pressed or released.
 * They appear in the #CdkEventKey.keyval field of the
 * #CdkEventKey structure, which is passed to signal handlers for the
 * #CtkWidget::key-press-event and #CtkWidget::key-release-event signals.
 * The complete list of key values can be found in the
 * `cdk/cdkkeysyms.h` header file.
 *
 * Key values are regularly updated from the upstream X.org X11 implementation,
 * so new values are added regularly. They will be prefixed with CDK_KEY_ rather
 * than XF86XK_ or XK_ (for older symbols).
 *
 * Key values can be converted into a string representation using
 * cdk_keyval_name(). The reverse function, converting a string to a key value,
 * is provided by cdk_keyval_from_name().
 *
 * The case of key values can be determined using cdk_keyval_is_upper() and
 * cdk_keyval_is_lower(). Key values can be converted to upper or lower case
 * using cdk_keyval_to_upper() and cdk_keyval_to_lower().
 *
 * When it makes sense, key values can be converted to and from
 * Unicode characters with cdk_keyval_to_unicode() and cdk_unicode_to_keyval().
 *
 * # Groups # {#key-group-explanation}
 *
 * One #CdkKeymap object exists for each user display. cdk_keymap_get_default()
 * returns the #CdkKeymap for the default display; to obtain keymaps for other
 * displays, use cdk_keymap_get_for_display(). A keymap
 * is a mapping from #CdkKeymapKey to key values. You can think of a #CdkKeymapKey
 * as a representation of a symbol printed on a physical keyboard key. That is, it
 * contains three pieces of information. First, it contains the hardware keycode;
 * this is an identifying number for a physical key. Second, it contains the
 * “level” of the key. The level indicates which symbol on the
 * key will be used, in a vertical direction. So on a standard US keyboard, the key
 * with the number “1“ on it also has the exclamation point (”!”) character on
 * it. The level indicates whether to use the “1” or the “!” symbol.  The letter
 * keys are considered to have a lowercase letter at level 0, and an uppercase
 * letter at level 1, though only the uppercase letter is printed.  Third, the
 * #CdkKeymapKey contains a group; groups are not used on standard US keyboards,
 * but are used in many other countries. On a keyboard with groups, there can be 3
 * or 4 symbols printed on a single key. The group indicates movement in a
 * horizontal direction. Usually groups are used for two different languages.  In
 * group 0, a key might have two English characters, and in group 1 it might have
 * two Hebrew characters. The Hebrew characters will be printed on the key next to
 * the English characters.
 *
 * In order to use a keymap to interpret a key event, it’s necessary to first
 * convert the keyboard state into an effective group and level. This is done via a
 * set of rules that varies widely according to type of keyboard and user
 * configuration. The function cdk_keymap_translate_keyboard_state() accepts a
 * keyboard state -- consisting of hardware keycode pressed, active modifiers, and
 * active group -- applies the appropriate rules, and returns the group/level to be
 * used to index the keymap, along with the modifiers which did not affect the
 * group and level. i.e. it returns “unconsumed modifiers.” The keyboard group may
 * differ from the effective group used for keymap lookups because some keys don't
 * have multiple groups - e.g. the Enter key is always in group 0 regardless of
 * keyboard state.
 *
 * Note that cdk_keymap_translate_keyboard_state() also returns the keyval, i.e. it
 * goes ahead and performs the keymap lookup in addition to telling you which
 * effective group/level values were used for the lookup. #CdkEventKey already
 * contains this keyval, however, so you don’t normally need to call
 * cdk_keymap_translate_keyboard_state() just to get the keyval.
 */


enum {
  DIRECTION_CHANGED,
  KEYS_CHANGED,
  STATE_CHANGED,
  LAST_SIGNAL
};


static CdkModifierType cdk_keymap_real_get_modifier_mask (CdkKeymap         *keymap,
                                                          CdkModifierIntent  intent);


static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (CdkKeymap, cdk_keymap, G_TYPE_OBJECT)

static void
cdk_keymap_class_init (CdkKeymapClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  klass->get_modifier_mask = cdk_keymap_real_get_modifier_mask;

  /**
   * CdkKeymap::direction-changed:
   * @keymap: the object on which the signal is emitted
   *
   * The ::direction-changed signal gets emitted when the direction of
   * the keymap changes.
   *
   * Since: 2.0
   */
  signals[DIRECTION_CHANGED] =
    g_signal_new (g_intern_static_string ("direction-changed"),
		  G_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CdkKeymapClass, direction_changed),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE,
		  0);
  /**
   * CdkKeymap::keys-changed:
   * @keymap: the object on which the signal is emitted
   *
   * The ::keys-changed signal is emitted when the mapping represented by
   * @keymap changes.
   *
   * Since: 2.2
   */
  signals[KEYS_CHANGED] =
    g_signal_new (g_intern_static_string ("keys-changed"),
		  G_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CdkKeymapClass, keys_changed),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE,
		  0);

  /**
   * CdkKeymap::state-changed:
   * @keymap: the object on which the signal is emitted
   *
   * The ::state-changed signal is emitted when the state of the
   * keyboard changes, e.g when Caps Lock is turned on or off.
   * See cdk_keymap_get_caps_lock_state().
   *
   * Since: 2.16
   */
  signals[STATE_CHANGED] =
    g_signal_new (g_intern_static_string ("state_changed"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CdkKeymapClass, state_changed),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  0);
}

static void
cdk_keymap_init (CdkKeymap *keymap G_GNUC_UNUSED)
{
}

/* Other key-handling stuff
 */

/**
 * cdk_keyval_to_upper:
 * @keyval: a key value.
 *
 * Converts a key value to upper case, if applicable.
 *
 * Returns: the upper case form of @keyval, or @keyval itself if it is already
 *   in upper case or it is not subject to case conversion.
 */
guint
cdk_keyval_to_upper (guint keyval)
{
  guint result;

  cdk_keyval_convert_case (keyval, NULL, &result);

  return result;
}

/**
 * cdk_keyval_to_lower:
 * @keyval: a key value.
 *
 * Converts a key value to lower case, if applicable.
 *
 * Returns: the lower case form of @keyval, or @keyval itself if it is already
 *  in lower case or it is not subject to case conversion.
 */
guint
cdk_keyval_to_lower (guint keyval)
{
  guint result;

  cdk_keyval_convert_case (keyval, &result, NULL);

  return result;
}

/**
 * cdk_keyval_is_upper:
 * @keyval: a key value.
 *
 * Returns %TRUE if the given key value is in upper case.
 *
 * Returns: %TRUE if @keyval is in upper case, or if @keyval is not subject to
 *  case conversion.
 */
gboolean
cdk_keyval_is_upper (guint keyval)
{
  if (keyval)
    {
      guint upper_val = 0;

      cdk_keyval_convert_case (keyval, NULL, &upper_val);
      return upper_val == keyval;
    }
  return FALSE;
}

/**
 * cdk_keyval_is_lower:
 * @keyval: a key value.
 *
 * Returns %TRUE if the given key value is in lower case.
 *
 * Returns: %TRUE if @keyval is in lower case, or if @keyval is not
 *   subject to case conversion.
 */
gboolean
cdk_keyval_is_lower (guint keyval)
{
  if (keyval)
    {
      guint lower_val = 0;

      cdk_keyval_convert_case (keyval, &lower_val, NULL);
      return lower_val == keyval;
    }
  return FALSE;
}

/**
 * cdk_keymap_get_default:
 *
 * Returns the #CdkKeymap attached to the default display.
 *
 * Returns: (transfer none): the #CdkKeymap attached to the default display.
 *
 * Deprecated: 3.22: Use cdk_keymap_get_for_display() instead
 */
CdkKeymap*
cdk_keymap_get_default (void)
{
  return cdk_keymap_get_for_display (cdk_display_get_default ());
}

/**
 * cdk_keymap_get_direction:
 * @keymap: a #CdkKeymap
 *
 * Returns the direction of effective layout of the keymap.
 *
 * Returns: %PANGO_DIRECTION_LTR or %PANGO_DIRECTION_RTL
 *   if it can determine the direction. %PANGO_DIRECTION_NEUTRAL
 *   otherwise.
 **/
PangoDirection
cdk_keymap_get_direction (CdkKeymap *keymap)
{
  g_return_val_if_fail (CDK_IS_KEYMAP (keymap), PANGO_DIRECTION_LTR);

  return CDK_KEYMAP_GET_CLASS (keymap)->get_direction (keymap);
}

/**
 * cdk_keymap_have_bidi_layouts:
 * @keymap: a #CdkKeymap
 *
 * Determines if keyboard layouts for both right-to-left and left-to-right
 * languages are in use.
 *
 * Returns: %TRUE if there are layouts in both directions, %FALSE otherwise
 *
 * Since: 2.12
 **/
gboolean
cdk_keymap_have_bidi_layouts (CdkKeymap *keymap)
{
  g_return_val_if_fail (CDK_IS_KEYMAP (keymap), FALSE);

  return CDK_KEYMAP_GET_CLASS (keymap)->have_bidi_layouts (keymap);
}

/**
 * cdk_keymap_get_caps_lock_state:
 * @keymap: a #CdkKeymap
 *
 * Returns whether the Caps Lock modifer is locked.
 *
 * Returns: %TRUE if Caps Lock is on
 *
 * Since: 2.16
 */
gboolean
cdk_keymap_get_caps_lock_state (CdkKeymap *keymap)
{
  g_return_val_if_fail (CDK_IS_KEYMAP (keymap), FALSE);

  return CDK_KEYMAP_GET_CLASS (keymap)->get_caps_lock_state (keymap);
}

/**
 * cdk_keymap_get_num_lock_state:
 * @keymap: a #CdkKeymap
 *
 * Returns whether the Num Lock modifer is locked.
 *
 * Returns: %TRUE if Num Lock is on
 *
 * Since: 3.0
 */
gboolean
cdk_keymap_get_num_lock_state (CdkKeymap *keymap)
{
  g_return_val_if_fail (CDK_IS_KEYMAP (keymap), FALSE);

  return CDK_KEYMAP_GET_CLASS (keymap)->get_num_lock_state (keymap);
}

/**
 * cdk_keymap_get_scroll_lock_state:
 * @keymap: a #CdkKeymap
 *
 * Returns whether the Scroll Lock modifer is locked.
 *
 * Returns: %TRUE if Scroll Lock is on
 *
 * Since: 3.18
 */
gboolean
cdk_keymap_get_scroll_lock_state (CdkKeymap *keymap)
{
  g_return_val_if_fail (CDK_IS_KEYMAP (keymap), FALSE);

  return CDK_KEYMAP_GET_CLASS (keymap)->get_scroll_lock_state (keymap);
}

/**
 * cdk_keymap_get_modifier_state:
 * @keymap: a #CdkKeymap
 *
 * Returns the current modifier state.
 *
 * Returns: the current modifier state.
 *
 * Since: 3.4
 */
guint
cdk_keymap_get_modifier_state (CdkKeymap *keymap)
{
  g_return_val_if_fail (CDK_IS_KEYMAP (keymap), FALSE);

  if (CDK_KEYMAP_GET_CLASS (keymap)->get_modifier_state)
    return CDK_KEYMAP_GET_CLASS (keymap)->get_modifier_state (keymap);

  return 0;
}

/**
 * cdk_keymap_get_entries_for_keyval:
 * @keymap: a #CdkKeymap
 * @keyval: a keyval, such as %CDK_KEY_a, %CDK_KEY_Up, %CDK_KEY_Return, etc.
 * @keys: (out) (array length=n_keys) (transfer full): return location
 *     for an array of #CdkKeymapKey
 * @n_keys: return location for number of elements in returned array
 *
 * Obtains a list of keycode/group/level combinations that will
 * generate @keyval. Groups and levels are two kinds of keyboard mode;
 * in general, the level determines whether the top or bottom symbol
 * on a key is used, and the group determines whether the left or
 * right symbol is used. On US keyboards, the shift key changes the
 * keyboard level, and there are no groups. A group switch key might
 * convert a keyboard between Hebrew to English modes, for example.
 * #CdkEventKey contains a %group field that indicates the active
 * keyboard group. The level is computed from the modifier mask.
 * The returned array should be freed
 * with g_free().
 *
 * Returns: %TRUE if keys were found and returned
 **/
gboolean
cdk_keymap_get_entries_for_keyval (CdkKeymap     *keymap,
                                   guint          keyval,
                                   CdkKeymapKey **keys,
                                   gint          *n_keys)
{
  g_return_val_if_fail (CDK_IS_KEYMAP (keymap), FALSE);
  g_return_val_if_fail (keys != NULL, FALSE);
  g_return_val_if_fail (n_keys != NULL, FALSE);
  g_return_val_if_fail (keyval != 0, FALSE);

  return CDK_KEYMAP_GET_CLASS (keymap)->get_entries_for_keyval (keymap, keyval,
                                                                keys, n_keys);
}

/**
 * cdk_keymap_get_entries_for_keycode:
 * @keymap: a #CdkKeymap
 * @hardware_keycode: a keycode
 * @keys: (out) (array length=n_entries) (transfer full) (optional): return
 *     location for array of #CdkKeymapKey, or %NULL
 * @keyvals: (out) (array length=n_entries) (transfer full) (optional): return
 *     location for array of keyvals, or %NULL
 * @n_entries: length of @keys and @keyvals
 *
 * Returns the keyvals bound to @hardware_keycode.
 * The Nth #CdkKeymapKey in @keys is bound to the Nth
 * keyval in @keyvals. Free the returned arrays with g_free().
 * When a keycode is pressed by the user, the keyval from
 * this list of entries is selected by considering the effective
 * keyboard group and level. See cdk_keymap_translate_keyboard_state().
 *
 * Returns: %TRUE if there were any entries
 **/
gboolean
cdk_keymap_get_entries_for_keycode (CdkKeymap     *keymap,
                                    guint          hardware_keycode,
                                    CdkKeymapKey **keys,
                                    guint        **keyvals,
                                    gint          *n_entries)
{
  g_return_val_if_fail (CDK_IS_KEYMAP (keymap), FALSE);
  g_return_val_if_fail (n_entries != NULL, FALSE);

  return CDK_KEYMAP_GET_CLASS (keymap)->get_entries_for_keycode (keymap, hardware_keycode,
                                                                 keys, keyvals, n_entries);
}

/**
 * cdk_keymap_lookup_key:
 * @keymap: a #CdkKeymap
 * @key: a #CdkKeymapKey with keycode, group, and level initialized
 *
 * Looks up the keyval mapped to a keycode/group/level triplet.
 * If no keyval is bound to @key, returns 0. For normal user input,
 * you want to use cdk_keymap_translate_keyboard_state() instead of
 * this function, since the effective group/level may not be
 * the same as the current keyboard state.
 *
 * Returns: a keyval, or 0 if none was mapped to the given @key
 **/
guint
cdk_keymap_lookup_key (CdkKeymap          *keymap,
                       const CdkKeymapKey *key)
{
  g_return_val_if_fail (CDK_IS_KEYMAP (keymap), 0);
  g_return_val_if_fail (key != NULL, 0);

  return CDK_KEYMAP_GET_CLASS (keymap)->lookup_key (keymap, key);
}

/**
 * cdk_keymap_translate_keyboard_state:
 * @keymap: a #CdkKeymap
 * @hardware_keycode: a keycode
 * @state: a modifier state
 * @group: active keyboard group
 * @keyval: (out) (allow-none): return location for keyval, or %NULL
 * @effective_group: (out) (allow-none): return location for effective
 *     group, or %NULL
 * @level: (out) (allow-none): return location for level, or %NULL
 * @consumed_modifiers: (out) (allow-none): return location for modifiers
 *     that were used to determine the group or level, or %NULL
 *
 * Translates the contents of a #CdkEventKey into a keyval, effective
 * group, and level. Modifiers that affected the translation and
 * are thus unavailable for application use are returned in
 * @consumed_modifiers.
 * See [Groups][key-group-explanation] for an explanation of
 * groups and levels. The @effective_group is the group that was
 * actually used for the translation; some keys such as Enter are not
 * affected by the active keyboard group. The @level is derived from
 * @state. For convenience, #CdkEventKey already contains the translated
 * keyval, so this function isn’t as useful as you might think.
 *
 * @consumed_modifiers gives modifiers that should be masked outfrom @state
 * when comparing this key press to a hot key. For instance, on a US keyboard,
 * the `plus` symbol is shifted, so when comparing a key press to a
 * `<Control>plus` accelerator `<Shift>` should be masked out.
 *
 * |[<!-- language="C" -->
 * // We want to ignore irrelevant modifiers like ScrollLock
 * #define ALL_ACCELS_MASK (CDK_CONTROL_MASK | CDK_SHIFT_MASK | CDK_MOD1_MASK)
 * cdk_keymap_translate_keyboard_state (keymap, event->hardware_keycode,
 *                                      event->state, event->group,
 *                                      &keyval, NULL, NULL, &consumed);
 * if (keyval == CDK_PLUS &&
 *     (event->state & ~consumed & ALL_ACCELS_MASK) == CDK_CONTROL_MASK)
 *   // Control was pressed
 * ]|
 * 
 * An older interpretation @consumed_modifiers was that it contained
 * all modifiers that might affect the translation of the key;
 * this allowed accelerators to be stored with irrelevant consumed
 * modifiers, by doing:
 * |[<!-- language="C" -->
 * // XXX Don’t do this XXX
 * if (keyval == accel_keyval &&
 *     (event->state & ~consumed & ALL_ACCELS_MASK) == (accel_mods & ~consumed))
 *   // Accelerator was pressed
 * ]|
 *
 * However, this did not work if multi-modifier combinations were
 * used in the keymap, since, for instance, `<Control>` would be
 * masked out even if only `<Control><Alt>` was used in the keymap.
 * To support this usage as well as well as possible, all single
 * modifier combinations that could affect the key for any combination
 * of modifiers will be returned in @consumed_modifiers; multi-modifier
 * combinations are returned only when actually found in @state. When
 * you store accelerators, you should always store them with consumed
 * modifiers removed. Store `<Control>plus`, not `<Control><Shift>plus`,
 *
 * Returns: %TRUE if there was a keyval bound to the keycode/state/group
 **/
gboolean
cdk_keymap_translate_keyboard_state (CdkKeymap       *keymap,
                                     guint            hardware_keycode,
                                     CdkModifierType  state,
                                     gint             group,
                                     guint           *keyval,
                                     gint            *effective_group,
                                     gint            *level,
                                     CdkModifierType *consumed_modifiers)
{
  g_return_val_if_fail (CDK_IS_KEYMAP (keymap), FALSE);

  return CDK_KEYMAP_GET_CLASS (keymap)->translate_keyboard_state (keymap,
								  hardware_keycode,
								  state,
								  group,
								  keyval,
								  effective_group,
								  level,
								  consumed_modifiers);
}

/**
 * cdk_keymap_add_virtual_modifiers:
 * @keymap: a #CdkKeymap
 * @state: (inout): pointer to the modifier mask to change
 *
 * Maps the non-virtual modifiers (i.e Mod2, Mod3, ...) which are set
 * in @state to the virtual modifiers (i.e. Super, Hyper and Meta) and
 * set the corresponding bits in @state.
 *
 * CDK already does this before delivering key events, but for
 * compatibility reasons, it only sets the first virtual modifier
 * it finds, whereas this function sets all matching virtual modifiers.
 *
 * This function is useful when matching key events against
 * accelerators.
 *
 * Since: 2.20
 */
void
cdk_keymap_add_virtual_modifiers (CdkKeymap       *keymap,
			          CdkModifierType *state)
{
  g_return_if_fail (CDK_IS_KEYMAP (keymap));

  CDK_KEYMAP_GET_CLASS (keymap)->add_virtual_modifiers (keymap, state);
}

/**
 * cdk_keymap_map_virtual_modifiers:
 * @keymap: a #CdkKeymap
 * @state: (inout): pointer to the modifier state to map
 *
 * Maps the virtual modifiers (i.e. Super, Hyper and Meta) which
 * are set in @state to their non-virtual counterparts (i.e. Mod2,
 * Mod3,...) and set the corresponding bits in @state.
 *
 * This function is useful when matching key events against
 * accelerators.
 *
 * Returns: %FALSE if two virtual modifiers were mapped to the
 *     same non-virtual modifier. Note that %FALSE is also returned
 *     if a virtual modifier is mapped to a non-virtual modifier that
 *     was already set in @state.
 *
 * Since: 2.20
 */
gboolean
cdk_keymap_map_virtual_modifiers (CdkKeymap       *keymap,
                                  CdkModifierType *state)
{
  g_return_val_if_fail (CDK_IS_KEYMAP (keymap), FALSE);

  return CDK_KEYMAP_GET_CLASS(keymap)->map_virtual_modifiers (keymap, state);
}

static CdkModifierType
cdk_keymap_real_get_modifier_mask (CdkKeymap         *keymap G_GNUC_UNUSED,
                                   CdkModifierIntent  intent)
{
  switch (intent)
    {
    case CDK_MODIFIER_INTENT_PRIMARY_ACCELERATOR:
      return CDK_CONTROL_MASK;

    case CDK_MODIFIER_INTENT_CONTEXT_MENU:
      return 0;

    case CDK_MODIFIER_INTENT_EXTEND_SELECTION:
      return CDK_SHIFT_MASK;

    case CDK_MODIFIER_INTENT_MODIFY_SELECTION:
      return CDK_CONTROL_MASK;

    case CDK_MODIFIER_INTENT_NO_TEXT_INPUT:
      return CDK_MOD1_MASK | CDK_CONTROL_MASK;

    case CDK_MODIFIER_INTENT_SHIFT_GROUP:
      return 0;

    case CDK_MODIFIER_INTENT_DEFAULT_MOD_MASK:
      return (CDK_SHIFT_MASK   | CDK_CONTROL_MASK | CDK_MOD1_MASK    |
	      CDK_SUPER_MASK   | CDK_HYPER_MASK   | CDK_META_MASK);

    default:
      g_return_val_if_reached (0);
    }
}

/**
 * cdk_keymap_get_modifier_mask:
 * @keymap: a #CdkKeymap
 * @intent: the use case for the modifier mask
 *
 * Returns the modifier mask the @keymap’s windowing system backend
 * uses for a particular purpose.
 *
 * Note that this function always returns real hardware modifiers, not
 * virtual ones (e.g. it will return #CDK_MOD1_MASK rather than
 * #CDK_META_MASK if the backend maps MOD1 to META), so there are use
 * cases where the return value of this function has to be transformed
 * by cdk_keymap_add_virtual_modifiers() in order to contain the
 * expected result.
 *
 * Returns: the modifier mask used for @intent.
 *
 * Since: 3.4
 **/
CdkModifierType
cdk_keymap_get_modifier_mask (CdkKeymap         *keymap,
                              CdkModifierIntent  intent)
{
  g_return_val_if_fail (CDK_IS_KEYMAP (keymap), 0);

  return CDK_KEYMAP_GET_CLASS (keymap)->get_modifier_mask (keymap, intent);
}

#include "cdkkeynames.c"

/**
 * cdk_keyval_name:
 * @keyval: a key value
 *
 * Converts a key value into a symbolic name.
 *
 * The names are the same as those in the
 * `cdk/cdkkeysyms.h` header file
 * but without the leading “CDK_KEY_”.
 *
 * Returns: (nullable) (transfer none): a string containing the name
 *     of the key, or %NULL if @keyval is not a valid key. The string
 *     should not be modified.
 */
gchar *
cdk_keyval_name (guint keyval)
{
  return _cdk_keyval_name (keyval);
}

/**
 * cdk_keyval_from_name:
 * @keyval_name: a key name
 *
 * Converts a key name to a key value.
 *
 * The names are the same as those in the
 * `cdk/cdkkeysyms.h` header file
 * but without the leading “CDK_KEY_”.
 *
 * Returns: the corresponding key value, or %CDK_KEY_VoidSymbol
 *     if the key name is not a valid key
 */
guint
cdk_keyval_from_name (const gchar *keyval_name)
{
  return _cdk_keyval_from_name (keyval_name);
}

/**
 * cdk_keyval_convert_case:
 * @symbol: a keyval
 * @lower: (out): return location for lowercase version of @symbol
 * @upper: (out): return location for uppercase version of @symbol
 *
 * Obtains the upper- and lower-case versions of the keyval @symbol.
 * Examples of keyvals are #CDK_KEY_a, #CDK_KEY_Enter, #CDK_KEY_F1, etc.
 */
void
cdk_keyval_convert_case (guint symbol,
                         guint *lower,
                         guint *upper)
{
  guint xlower, xupper;

  xlower = symbol;
  xupper = symbol;

  /* Check for directly encoded 24-bit UCS characters: */
  if ((symbol & 0xff000000) == 0x01000000)
    {
      if (lower)
        *lower = cdk_unicode_to_keyval (g_unichar_tolower (symbol & 0x00ffffff));
      if (upper)
        *upper = cdk_unicode_to_keyval (g_unichar_toupper (symbol & 0x00ffffff));
      return;
    }

  switch (symbol >> 8)
    {
    case 0: /* Latin 1 */
      if ((symbol >= CDK_KEY_A) && (symbol <= CDK_KEY_Z))
        xlower += (CDK_KEY_a - CDK_KEY_A);
      else if ((symbol >= CDK_KEY_a) && (symbol <= CDK_KEY_z))
        xupper -= (CDK_KEY_a - CDK_KEY_A);
      else if ((symbol >= CDK_KEY_Agrave) && (symbol <= CDK_KEY_Odiaeresis))
        xlower += (CDK_KEY_agrave - CDK_KEY_Agrave);
      else if ((symbol >= CDK_KEY_agrave) && (symbol <= CDK_KEY_odiaeresis))
        xupper -= (CDK_KEY_agrave - CDK_KEY_Agrave);
      else if ((symbol >= CDK_KEY_Ooblique) && (symbol <= CDK_KEY_Thorn))
        xlower += (CDK_KEY_oslash - CDK_KEY_Ooblique);
      else if ((symbol >= CDK_KEY_oslash) && (symbol <= CDK_KEY_thorn))
        xupper -= (CDK_KEY_oslash - CDK_KEY_Ooblique);
      break;

    case 1: /* Latin 2 */
      /* Assume the KeySym is a legal value (ignore discontinuities) */
      if (symbol == CDK_KEY_Aogonek)
        xlower = CDK_KEY_aogonek;
      else if (symbol >= CDK_KEY_Lstroke && symbol <= CDK_KEY_Sacute)
        xlower += (CDK_KEY_lstroke - CDK_KEY_Lstroke);
      else if (symbol >= CDK_KEY_Scaron && symbol <= CDK_KEY_Zacute)
        xlower += (CDK_KEY_scaron - CDK_KEY_Scaron);
      else if (symbol >= CDK_KEY_Zcaron && symbol <= CDK_KEY_Zabovedot)
        xlower += (CDK_KEY_zcaron - CDK_KEY_Zcaron);
      else if (symbol == CDK_KEY_aogonek)
        xupper = CDK_KEY_Aogonek;
      else if (symbol >= CDK_KEY_lstroke && symbol <= CDK_KEY_sacute)
        xupper -= (CDK_KEY_lstroke - CDK_KEY_Lstroke);
      else if (symbol >= CDK_KEY_scaron && symbol <= CDK_KEY_zacute)
        xupper -= (CDK_KEY_scaron - CDK_KEY_Scaron);
      else if (symbol >= CDK_KEY_zcaron && symbol <= CDK_KEY_zabovedot)
        xupper -= (CDK_KEY_zcaron - CDK_KEY_Zcaron);
      else if (symbol >= CDK_KEY_Racute && symbol <= CDK_KEY_Tcedilla)
        xlower += (CDK_KEY_racute - CDK_KEY_Racute);
      else if (symbol >= CDK_KEY_racute && symbol <= CDK_KEY_tcedilla)
        xupper -= (CDK_KEY_racute - CDK_KEY_Racute);
      break;

    case 2: /* Latin 3 */
      /* Assume the KeySym is a legal value (ignore discontinuities) */
      if (symbol >= CDK_KEY_Hstroke && symbol <= CDK_KEY_Hcircumflex)
        xlower += (CDK_KEY_hstroke - CDK_KEY_Hstroke);
      else if (symbol >= CDK_KEY_Gbreve && symbol <= CDK_KEY_Jcircumflex)
        xlower += (CDK_KEY_gbreve - CDK_KEY_Gbreve);
      else if (symbol >= CDK_KEY_hstroke && symbol <= CDK_KEY_hcircumflex)
        xupper -= (CDK_KEY_hstroke - CDK_KEY_Hstroke);
      else if (symbol >= CDK_KEY_gbreve && symbol <= CDK_KEY_jcircumflex)
        xupper -= (CDK_KEY_gbreve - CDK_KEY_Gbreve);
      else if (symbol >= CDK_KEY_Cabovedot && symbol <= CDK_KEY_Scircumflex)
        xlower += (CDK_KEY_cabovedot - CDK_KEY_Cabovedot);
      else if (symbol >= CDK_KEY_cabovedot && symbol <= CDK_KEY_scircumflex)
        xupper -= (CDK_KEY_cabovedot - CDK_KEY_Cabovedot);
      break;

    case 3: /* Latin 4 */
      /* Assume the KeySym is a legal value (ignore discontinuities) */
      if (symbol >= CDK_KEY_Rcedilla && symbol <= CDK_KEY_Tslash)
        xlower += (CDK_KEY_rcedilla - CDK_KEY_Rcedilla);
      else if (symbol >= CDK_KEY_rcedilla && symbol <= CDK_KEY_tslash)
        xupper -= (CDK_KEY_rcedilla - CDK_KEY_Rcedilla);
      else if (symbol == CDK_KEY_ENG)
        xlower = CDK_KEY_eng;
      else if (symbol == CDK_KEY_eng)
        xupper = CDK_KEY_ENG;
      else if (symbol >= CDK_KEY_Amacron && symbol <= CDK_KEY_Umacron)
        xlower += (CDK_KEY_amacron - CDK_KEY_Amacron);
      else if (symbol >= CDK_KEY_amacron && symbol <= CDK_KEY_umacron)
        xupper -= (CDK_KEY_amacron - CDK_KEY_Amacron);
      break;

    case 6: /* Cyrillic */
      /* Assume the KeySym is a legal value (ignore discontinuities) */
      if (symbol >= CDK_KEY_Serbian_DJE && symbol <= CDK_KEY_Serbian_DZE)
        xlower -= (CDK_KEY_Serbian_DJE - CDK_KEY_Serbian_dje);
      else if (symbol >= CDK_KEY_Serbian_dje && symbol <= CDK_KEY_Serbian_dze)
        xupper += (CDK_KEY_Serbian_DJE - CDK_KEY_Serbian_dje);
      else if (symbol >= CDK_KEY_Cyrillic_YU && symbol <= CDK_KEY_Cyrillic_HARDSIGN)
        xlower -= (CDK_KEY_Cyrillic_YU - CDK_KEY_Cyrillic_yu);
      else if (symbol >= CDK_KEY_Cyrillic_yu && symbol <= CDK_KEY_Cyrillic_hardsign)
        xupper += (CDK_KEY_Cyrillic_YU - CDK_KEY_Cyrillic_yu);
      break;

    case 7: /* Greek */
      /* Assume the KeySym is a legal value (ignore discontinuities) */
      if (symbol >= CDK_KEY_Greek_ALPHAaccent && symbol <= CDK_KEY_Greek_OMEGAaccent)
        xlower += (CDK_KEY_Greek_alphaaccent - CDK_KEY_Greek_ALPHAaccent);
      else if (symbol >= CDK_KEY_Greek_alphaaccent && symbol <= CDK_KEY_Greek_omegaaccent &&
               symbol != CDK_KEY_Greek_iotaaccentdieresis &&
               symbol != CDK_KEY_Greek_upsilonaccentdieresis)
        xupper -= (CDK_KEY_Greek_alphaaccent - CDK_KEY_Greek_ALPHAaccent);
      else if (symbol >= CDK_KEY_Greek_ALPHA && symbol <= CDK_KEY_Greek_OMEGA)
        xlower += (CDK_KEY_Greek_alpha - CDK_KEY_Greek_ALPHA);
      else if (symbol >= CDK_KEY_Greek_alpha && symbol <= CDK_KEY_Greek_omega &&
               symbol != CDK_KEY_Greek_finalsmallsigma)
        xupper -= (CDK_KEY_Greek_alpha - CDK_KEY_Greek_ALPHA);
      break;
    }

  if (lower)
    *lower = xlower;
  if (upper)
    *upper = xupper;
}
