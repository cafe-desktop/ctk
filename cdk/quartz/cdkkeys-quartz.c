/* cdkkeys-quartz.c
 *
 * Copyright (C) 2000 Red Hat, Inc.
 * Copyright (C) 2005 Imendio AB
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
/* Some parts of this code come from quartzKeyboard.c,
 * from the Apple X11 Server.
 *
 * Copyright (c) 2003 Apple Computer, Inc. All rights reserved.
 *
 *  Permission is hereby granted, free of charge, to any person
 *  obtaining a copy of this software and associated documentation files
 *  (the "Software"), to deal in the Software without restriction,
 *  including without limitation the rights to use, copy, modify, merge,
 *  publish, distribute, sublicense, and/or sell copies of the Software,
 *  and to permit persons to whom the Software is furnished to do so,
 *  subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be
 *  included in all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *  NONINFRINGEMENT.  IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT
 *  HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *  DEALINGS IN THE SOFTWARE.
 *
 *  Except as contained in this notice, the name(s) of the above
 *  copyright holders shall not be used in advertising or otherwise to
 *  promote the sale, use or other dealings in this Software without
 *  prior written authorization.
 */

#include "config.h"

#include <Carbon/Carbon.h>
#include <AppKit/NSEvent.h>
#include "cdk.h"
#include "cdkquartzkeys.h"
#include "cdkkeysprivate.h"
#include "cdkkeysyms.h"
#include "cdkkeys-quartz.h"
#include "cdkinternal-quartz.h"

#define NUM_KEYCODES 128
#define KEYVALS_PER_KEYCODE 4

static CdkKeymap *default_keymap = NULL;

struct _CdkQuartzKeymap
{
  CdkKeymap keymap;
};

struct _CdkQuartzKeymapClass
{
  CdkKeymapClass keymap_class;
};

G_DEFINE_TYPE (CdkQuartzKeymap, cdk_quartz_keymap, CDK_TYPE_KEYMAP)

CdkKeymap *
_cdk_quartz_display_get_keymap (CdkDisplay *display)
{
  if (default_keymap == NULL)
    default_keymap = g_object_new (cdk_quartz_keymap_get_type (), NULL);

  return default_keymap;
}

/* This is a table of all keyvals. Each keycode gets KEYVALS_PER_KEYCODE entries.
 * TThere is 1 keyval per modifier (Nothing, Shift, Alt, Shift+Alt);
 */
static guint *keyval_array = NULL;

const static struct {
  guint keycode;
  guint keyval;
  unsigned int modmask; /* So we can tell when a mod key is pressed/released */
} modifier_keys[] = {
  {  54, CDK_KEY_Meta_R,    CDK_QUARTZ_COMMAND_KEY_MASK },
  {  55, CDK_KEY_Meta_L,    CDK_QUARTZ_COMMAND_KEY_MASK },
  {  56, CDK_KEY_Shift_L,   CDK_QUARTZ_SHIFT_KEY_MASK },
  {  57, CDK_KEY_Caps_Lock, CDK_QUARTZ_ALPHA_SHIFT_KEY_MASK },
  {  58, CDK_KEY_Alt_L,     CDK_QUARTZ_ALTERNATE_KEY_MASK },
  {  59, CDK_KEY_Control_L, CDK_QUARTZ_CONTROL_KEY_MASK },
  {  60, CDK_KEY_Shift_R,   CDK_QUARTZ_SHIFT_KEY_MASK },
  {  61, CDK_KEY_Alt_R,     CDK_QUARTZ_ALTERNATE_KEY_MASK },
  {  62, CDK_KEY_Control_R, CDK_QUARTZ_CONTROL_KEY_MASK }
};

const static struct {
  guint keycode;
  guint keyval;
} function_keys[] = {
  { 122, CDK_KEY_F1 },
  { 120, CDK_KEY_F2 },
  {  99, CDK_KEY_F3 },
  { 118, CDK_KEY_F4 },
  {  96, CDK_KEY_F5 },
  {  97, CDK_KEY_F6 },
  {  98, CDK_KEY_F7 },
  { 100, CDK_KEY_F8 },
  { 101, CDK_KEY_F9 },
  { 109, CDK_KEY_F10 },
  { 103, CDK_KEY_F11 },
  { 111, CDK_KEY_F12 },
  { 105, CDK_KEY_F13 },
  { 107, CDK_KEY_F14 },
  { 113, CDK_KEY_F15 },
  { 106, CDK_KEY_F16 }
};

const static struct {
  guint keycode;
  guint normal_keyval, keypad_keyval;
} known_numeric_keys[] = {
  { 65, CDK_KEY_period, CDK_KEY_KP_Decimal },
  { 67, CDK_KEY_asterisk, CDK_KEY_KP_Multiply },
  { 69, CDK_KEY_plus, CDK_KEY_KP_Add },
  { 75, CDK_KEY_slash, CDK_KEY_KP_Divide },
  { 76, CDK_KEY_Return, CDK_KEY_KP_Enter },
  { 78, CDK_KEY_minus, CDK_KEY_KP_Subtract },
  { 81, CDK_KEY_equal, CDK_KEY_KP_Equal },
  { 82, CDK_KEY_0, CDK_KEY_KP_0 },
  { 83, CDK_KEY_1, CDK_KEY_KP_1 },
  { 84, CDK_KEY_2, CDK_KEY_KP_2 },
  { 85, CDK_KEY_3, CDK_KEY_KP_3 },
  { 86, CDK_KEY_4, CDK_KEY_KP_4 },
  { 87, CDK_KEY_5, CDK_KEY_KP_5 },
  { 88, CDK_KEY_6, CDK_KEY_KP_6 },
  { 89, CDK_KEY_7, CDK_KEY_KP_7 },
  { 91, CDK_KEY_8, CDK_KEY_KP_8 },
  { 92, CDK_KEY_9, CDK_KEY_KP_9 }
};

/* These values aren't covered by cdk_unicode_to_keyval */
const static struct {
  gunichar ucs_value;
  guint keyval;
} special_ucs_table [] = {
  { 0x0001, CDK_KEY_Home },
  { 0x0003, CDK_KEY_Return },
  { 0x0004, CDK_KEY_End },
  { 0x0008, CDK_KEY_BackSpace },
  { 0x0009, CDK_KEY_Tab },
  { 0x000b, CDK_KEY_Page_Up },
  { 0x000c, CDK_KEY_Page_Down },
  { 0x000d, CDK_KEY_Return },
  { 0x001b, CDK_KEY_Escape },
  { 0x001c, CDK_KEY_Left },
  { 0x001d, CDK_KEY_Right },
  { 0x001e, CDK_KEY_Up },
  { 0x001f, CDK_KEY_Down },
  { 0x007f, CDK_KEY_Delete },
  { 0xf027, CDK_KEY_dead_acute },
  { 0xf060, CDK_KEY_dead_grave },
  { 0xf300, CDK_KEY_dead_grave },
  { 0xf0b4, CDK_KEY_dead_acute },
  { 0xf301, CDK_KEY_dead_acute },
  { 0xf385, CDK_KEY_dead_acute },
  { 0xf05e, CDK_KEY_dead_circumflex },
  { 0xf2c6, CDK_KEY_dead_circumflex },
  { 0xf302, CDK_KEY_dead_circumflex },
  { 0xf07e, CDK_KEY_dead_tilde },
  { 0xf2dc, CDK_KEY_dead_tilde },
  { 0xf303, CDK_KEY_dead_tilde },
  { 0xf342, CDK_KEY_dead_perispomeni },
  { 0xf0af, CDK_KEY_dead_macron },
  { 0xf304, CDK_KEY_dead_macron },
  { 0xf2d8, CDK_KEY_dead_breve },
  { 0xf306, CDK_KEY_dead_breve },
  { 0xf2d9, CDK_KEY_dead_abovedot },
  { 0xf307, CDK_KEY_dead_abovedot },
  { 0xf0a8, CDK_KEY_dead_diaeresis },
  { 0xf308, CDK_KEY_dead_diaeresis },
  { 0xf2da, CDK_KEY_dead_abovering },
  { 0xf30A, CDK_KEY_dead_abovering },
  { 0xf022, CDK_KEY_dead_doubleacute },
  { 0xf2dd, CDK_KEY_dead_doubleacute },
  { 0xf30B, CDK_KEY_dead_doubleacute },
  { 0xf2c7, CDK_KEY_dead_caron },
  { 0xf30C, CDK_KEY_dead_caron },
  { 0xf0be, CDK_KEY_dead_cedilla },
  { 0xf327, CDK_KEY_dead_cedilla },
  { 0xf2db, CDK_KEY_dead_ogonek },
  { 0xf328, CDK_KEY_dead_ogonek },
  { 0xfe5d, CDK_KEY_dead_iota },
  { 0xf323, CDK_KEY_dead_belowdot },
  { 0xf309, CDK_KEY_dead_hook },
  { 0xf31B, CDK_KEY_dead_horn },
  { 0xf02d, CDK_KEY_dead_stroke },
  { 0xf335, CDK_KEY_dead_stroke },
  { 0xf336, CDK_KEY_dead_stroke },
  { 0xf313, CDK_KEY_dead_abovecomma },
  /*  { 0xf313, CDK_KEY_dead_psili }, */
  { 0xf314, CDK_KEY_dead_abovereversedcomma },
  /*  { 0xf314, CDK_KEY_dead_dasia }, */
  { 0xf30F, CDK_KEY_dead_doublegrave },
  { 0xf325, CDK_KEY_dead_belowring },
  { 0xf2cd, CDK_KEY_dead_belowmacron },
  { 0xf331, CDK_KEY_dead_belowmacron },
  { 0xf32D, CDK_KEY_dead_belowcircumflex },
  { 0xf330, CDK_KEY_dead_belowtilde },
  { 0xf32E, CDK_KEY_dead_belowbreve },
  { 0xf324, CDK_KEY_dead_belowdiaeresis },
  { 0xf311, CDK_KEY_dead_invertedbreve },
  { 0xf02c, CDK_KEY_dead_belowcomma },
  { 0xf326, CDK_KEY_dead_belowcomma }
};

static void
update_keymap (void)
{
  const void *chr_data = NULL;
  guint *p;
  int i;

  /* Note: we could check only if building against the 10.5 SDK instead, but
   * that would make non-xml layouts not work in 32-bit which would be a quite
   * bad regression. This way, old unsupported layouts will just not work in
   * 64-bit.
   */
#ifdef __LP64__
  TISInputSourceRef new_layout = TISCopyCurrentKeyboardLayoutInputSource ();
  CFDataRef layout_data_ref;

#else
  KeyboardLayoutRef new_layout;
  KeyboardLayoutKind layout_kind;

  KLGetCurrentKeyboardLayout (&new_layout);
#endif

  g_free (keyval_array);
  keyval_array = g_new0 (guint, NUM_KEYCODES * KEYVALS_PER_KEYCODE);

#ifdef __LP64__
  layout_data_ref = (CFDataRef) TISGetInputSourceProperty
    (new_layout, kTISPropertyUnicodeKeyLayoutData);

  if (layout_data_ref)
    chr_data = CFDataGetBytePtr (layout_data_ref);

  if (chr_data == NULL)
    {
      g_error ("cannot get keyboard layout data");
      return;
    }
#else

  /* Get the layout kind */
  KLGetKeyboardLayoutProperty (new_layout, kKLKind, (const void **)&layout_kind);

  /* 8-bit-only keyabord layout */
  if (layout_kind == kKLKCHRKind)
    {
      /* Get chr data */
      KLGetKeyboardLayoutProperty (new_layout, kKLKCHRData, (const void **)&chr_data);

      for (i = 0; i < NUM_KEYCODES; i++)
        {
          int j;
          UInt32 modifiers[] = {0, shiftKey, optionKey, shiftKey | optionKey};

          p = keyval_array + i * KEYVALS_PER_KEYCODE;

          for (j = 0; j < KEYVALS_PER_KEYCODE; j++)
            {
              UInt32 c, state = 0;
              UInt16 key_code;
              UniChar uc;

              key_code = modifiers[j] | i;
              c = KeyTranslate (chr_data, key_code, &state);

              if (state != 0)
                {
                  UInt32 state2 = 0;
                  c = KeyTranslate (chr_data, key_code | 128, &state2);
                }

              if (c != 0 && c != 0x10)
                {
                  int k;
                  gboolean found = FALSE;

                  /* FIXME: some keyboard layouts (e.g. Russian) use a
                   * different 8-bit character set. We should check
                   * for this. Not a serious problem, because most
                   * (all?) of these layouts also have a uchr version.
                   */
                  uc = macroman2ucs (c);

                  for (k = 0; k < G_N_ELEMENTS (special_ucs_table); k++)
                    {
                      if (special_ucs_table[k].ucs_value == uc)
                        {
                          p[j] = special_ucs_table[k].keyval;
                          found = TRUE;
                          break;
                        }
                    }

                  /* Special-case shift-tab since CTK+ expects
                   * CDK_KEY_ISO_Left_Tab for that.
                   */
                  if (found && p[j] == CDK_KEY_Tab && modifiers[j] == shiftKey)
                    p[j] = CDK_KEY_ISO_Left_Tab;

                  if (!found)
                    p[j] = cdk_unicode_to_keyval (uc);
                }
            }

          if (p[3] == p[2])
            p[3] = 0;
          if (p[2] == p[1])
            p[2] = 0;
          if (p[1] == p[0])
            p[1] = 0;
          if (p[0] == p[2] &&
              p[1] == p[3])
            p[2] = p[3] = 0;
        }
    }
  /* unicode keyboard layout */
  else if (layout_kind == kKLKCHRuchrKind || layout_kind == kKLuchrKind)
    {
      /* Get chr data */
      KLGetKeyboardLayoutProperty (new_layout, kKLuchrData, (const void **)&chr_data);
#endif

      for (i = 0; i < NUM_KEYCODES; i++)
        {
          int j;
          UInt32 modifiers[] = {0, shiftKey, optionKey, shiftKey | optionKey};
          UniChar chars[4];
          UniCharCount nChars;

          p = keyval_array + i * KEYVALS_PER_KEYCODE;

          for (j = 0; j < KEYVALS_PER_KEYCODE; j++)
            {
              UInt32 state = 0;
              OSStatus err;
              UInt16 key_code;
              UniChar uc;

              key_code = modifiers[j] | i;
              err = UCKeyTranslate (chr_data, i, kUCKeyActionDisplay,
                                    (modifiers[j] >> 8) & 0xFF,
                                    LMGetKbdType(),
                                    0,
                                    &state, 4, &nChars, chars);

              /* FIXME: Theoretically, we can get multiple UTF-16
               * values; we should convert them to proper unicode and
               * figure out whether there are really keyboard layouts
               * that give us more than one character for one
               * keypress.
               */
              if (err == noErr && nChars == 1)
                {
                  int k;
                  gboolean found = FALSE;

                  /* A few <Shift><Option>keys return two characters,
                   * the first of which is U+00a0, which isn't
                   * interesting; so we return the second. More
                   * sophisticated handling is the job of a
                   * CtkIMContext.
                   *
                   * If state isn't zero, it means that it's a dead
                   * key of some sort. Some of those are enumerated in
                   * the special_ucs_table with the high nibble set to
                   * f to push it into the private use range. Here we
                   * do the same.
                   */
                  if (state != 0)
                    chars[nChars - 1] |= 0xf000;
                  uc = chars[nChars - 1];

                  for (k = 0; k < G_N_ELEMENTS (special_ucs_table); k++)
                    {
                      if (special_ucs_table[k].ucs_value == uc)
                        {
                          p[j] = special_ucs_table[k].keyval;
                          found = TRUE;
                          break;
                        }
                    }

                  /* Special-case shift-tab since CTK+ expects
                   * CDK_KEY_ISO_Left_Tab for that.
                   */
                  if (found && p[j] == CDK_KEY_Tab && modifiers[j] == shiftKey)
                    p[j] = CDK_KEY_ISO_Left_Tab;

                  if (!found)
                    p[j] = cdk_unicode_to_keyval (uc);
                }
            }

          if (p[3] == p[2])
            p[3] = 0;
          if (p[2] == p[1])
            p[2] = 0;
          if (p[1] == p[0])
            p[1] = 0;
          if (p[0] == p[2] &&
              p[1] == p[3])
            p[2] = p[3] = 0;
        }
#ifndef __LP64__
    }
  else
    {
      g_error ("unknown type of keyboard layout (neither KCHR nor uchr)"
               " - not supported right now");
    }
#endif

  for (i = 0; i < G_N_ELEMENTS (modifier_keys); i++)
    {
      p = keyval_array + modifier_keys[i].keycode * KEYVALS_PER_KEYCODE;

      if (p[0] == 0 && p[1] == 0 &&
          p[2] == 0 && p[3] == 0)
        p[0] = modifier_keys[i].keyval;
    }

  for (i = 0; i < G_N_ELEMENTS (function_keys); i++)
    {
      p = keyval_array + function_keys[i].keycode * KEYVALS_PER_KEYCODE;

      p[0] = function_keys[i].keyval;
      p[1] = p[2] = p[3] = 0;
    }

  for (i = 0; i < G_N_ELEMENTS (known_numeric_keys); i++)
    {
      p = keyval_array + known_numeric_keys[i].keycode * KEYVALS_PER_KEYCODE;

      if (p[0] == known_numeric_keys[i].normal_keyval)
        p[0] = known_numeric_keys[i].keypad_keyval;
    }

  if (default_keymap != NULL)
    g_signal_emit_by_name (default_keymap, "keys-changed");
}

static PangoDirection
cdk_quartz_keymap_get_direction (CdkKeymap *keymap)
{
  return PANGO_DIRECTION_NEUTRAL;
}

static gboolean
cdk_quartz_keymap_have_bidi_layouts (CdkKeymap *keymap)
{
  /* FIXME: Can we implement this? */
  return FALSE;
}

static gboolean
cdk_quartz_keymap_get_caps_lock_state (CdkKeymap *keymap)
{
  /* FIXME: Implement this. */
  return FALSE;
}

static gboolean
cdk_quartz_keymap_get_num_lock_state (CdkKeymap *keymap)
{
  /* FIXME: Implement this. */
  return FALSE;
}

static gboolean
cdk_quartz_keymap_get_scroll_lock_state (CdkKeymap *keymap)
{
  /* FIXME: Implement this. */
  return FALSE;
}

static gboolean
cdk_quartz_keymap_get_entries_for_keyval (CdkKeymap     *keymap,
                                          guint          keyval,
                                          CdkKeymapKey **keys,
                                          gint          *n_keys)
{
  GArray *keys_array;
  int i;

  *n_keys = 0;
  keys_array = g_array_new (FALSE, FALSE, sizeof (CdkKeymapKey));

  for (i = 0; i < NUM_KEYCODES * KEYVALS_PER_KEYCODE; i++)
    {
      CdkKeymapKey key;

      if (keyval_array[i] != keyval)
	continue;

      (*n_keys)++;

      key.keycode = i / KEYVALS_PER_KEYCODE;
      key.group = (i % KEYVALS_PER_KEYCODE) >= 2;
      key.level = i % 2;

      g_array_append_val (keys_array, key);
    }

  *keys = (CdkKeymapKey *)g_array_free (keys_array, FALSE);
  
  return *n_keys > 0;;
}

static gboolean
cdk_quartz_keymap_get_entries_for_keycode (CdkKeymap     *keymap,
                                           guint          hardware_keycode,
                                           CdkKeymapKey **keys,
                                           guint        **keyvals,
                                           gint          *n_entries)
{
  GArray *keys_array, *keyvals_array;
  int i;
  guint *p;

  *n_entries = 0;

  if (hardware_keycode > NUM_KEYCODES)
    return FALSE;

  if (keys)
    keys_array = g_array_new (FALSE, FALSE, sizeof (CdkKeymapKey));
  else
    keys_array = NULL;

  if (keyvals)
    keyvals_array = g_array_new (FALSE, FALSE, sizeof (guint));
  else
    keyvals_array = NULL;

  p = keyval_array + hardware_keycode * KEYVALS_PER_KEYCODE;
  
  for (i = 0; i < KEYVALS_PER_KEYCODE; i++)
    {
      if (!p[i])
	continue;

      (*n_entries)++;
      
      if (keyvals_array)
	g_array_append_val (keyvals_array, p[i]);

      if (keys_array)
	{
	  CdkKeymapKey key;

	  key.keycode = hardware_keycode;
	  key.group = i >= 2;
	  key.level = i % 2;

	  g_array_append_val (keys_array, key);
	}
    }
  
  if (keys)
    *keys = (CdkKeymapKey *)g_array_free (keys_array, FALSE);

  if (keyvals)
    *keyvals = (guint *)g_array_free (keyvals_array, FALSE);

  return *n_entries > 0;
}

#define GET_KEYVAL(keycode, group, level) (keyval_array[(keycode * KEYVALS_PER_KEYCODE + group * 2 + level)])

static guint
cdk_quartz_keymap_lookup_key (CdkKeymap          *keymap,
                              const CdkKeymapKey *key)
{
  return GET_KEYVAL (key->keycode, key->group, key->level);
}

static guint
translate_keysym (guint           hardware_keycode,
		  gint            group,
		  CdkModifierType state,
		  gint           *effective_group,
		  gint           *effective_level)
{
  gint level;
  guint tmp_keyval;

  level = (state & CDK_SHIFT_MASK) ? 1 : 0;

  if (!(GET_KEYVAL (hardware_keycode, group, 0) || GET_KEYVAL (hardware_keycode, group, 1)) &&
      (GET_KEYVAL (hardware_keycode, 0, 0) || GET_KEYVAL (hardware_keycode, 0, 1)))
    group = 0;

  if (!GET_KEYVAL (hardware_keycode, group, level) &&
      GET_KEYVAL (hardware_keycode, group, 0))
    level = 0;

  tmp_keyval = GET_KEYVAL (hardware_keycode, group, level);

  if (state & CDK_LOCK_MASK)
    {
      guint upper = cdk_keyval_to_upper (tmp_keyval);
      if (upper != tmp_keyval)
        tmp_keyval = upper;
    }

  if (effective_group)
    *effective_group = group;
  if (effective_level)
    *effective_level = level;

  return tmp_keyval;
}

static gboolean
cdk_quartz_keymap_translate_keyboard_state (CdkKeymap       *keymap,
                                            guint            hardware_keycode,
                                            CdkModifierType  state,
                                            gint             group,
                                            guint           *keyval,
                                            gint            *effective_group,
                                            gint            *level,
                                            CdkModifierType *consumed_modifiers)
{
  guint tmp_keyval;
  CdkModifierType bit;

  if (keyval)
    *keyval = 0;
  if (effective_group)
    *effective_group = 0;
  if (level)
    *level = 0;
  if (consumed_modifiers)
    *consumed_modifiers = 0;

  if (hardware_keycode < 0 || hardware_keycode >= NUM_KEYCODES)
    return FALSE;

  tmp_keyval = translate_keysym (hardware_keycode, group, state, level, effective_group);

  /* Check if modifiers modify the keyval */
  if (consumed_modifiers)
    {
      guint tmp_modifiers = (state & CDK_MODIFIER_MASK);

      for (bit = 1; bit <= tmp_modifiers; bit <<= 1)
        {
          if ((bit & tmp_modifiers) &&
              translate_keysym (hardware_keycode, group, state & ~bit,
                                NULL, NULL) == tmp_keyval)
            tmp_modifiers &= ~bit;
        }

      *consumed_modifiers = tmp_modifiers;
    }

  if (keyval)
    *keyval = tmp_keyval; 

  return TRUE;
}

static void
cdk_quartz_keymap_add_virtual_modifiers (CdkKeymap       *keymap,
                                         CdkModifierType *state)
{
  if (*state & CDK_MOD2_MASK)
    *state |= CDK_META_MASK;
}

static gboolean
cdk_quartz_keymap_map_virtual_modifiers (CdkKeymap       *keymap,
                                         CdkModifierType *state)
{
  if (*state & CDK_META_MASK)
    *state |= CDK_MOD2_MASK;

  return TRUE;
}

static CdkModifierType
cdk_quartz_keymap_get_modifier_mask (CdkKeymap         *keymap,
                                     CdkModifierIntent  intent)
{
  switch (intent)
    {
    case CDK_MODIFIER_INTENT_PRIMARY_ACCELERATOR:
      return CDK_MOD2_MASK;

    case CDK_MODIFIER_INTENT_CONTEXT_MENU:
      return CDK_CONTROL_MASK;

    case CDK_MODIFIER_INTENT_EXTEND_SELECTION:
      return CDK_SHIFT_MASK;

    case CDK_MODIFIER_INTENT_MODIFY_SELECTION:
      return CDK_MOD2_MASK;

    case CDK_MODIFIER_INTENT_NO_TEXT_INPUT:
      return CDK_MOD2_MASK | CDK_CONTROL_MASK;

    case CDK_MODIFIER_INTENT_SHIFT_GROUP:
      return CDK_MOD1_MASK;

    case CDK_MODIFIER_INTENT_DEFAULT_MOD_MASK:
      return (CDK_SHIFT_MASK   | CDK_CONTROL_MASK | CDK_MOD1_MASK    |
	      CDK_MOD2_MASK    | CDK_SUPER_MASK   | CDK_HYPER_MASK   |
	      CDK_META_MASK);

    default:
      g_return_val_if_reached (0);
    }
}

/* What sort of key event is this? Returns one of
 * CDK_KEY_PRESS, CDK_KEY_RELEASE, CDK_NOTHING (should be ignored)
 */
CdkEventType
_cdk_quartz_keys_event_type (NSEvent *event)
{
  unsigned short keycode;
  unsigned int flags;
  int i;
  
  switch ([event type])
    {
    case CDK_QUARTZ_KEY_DOWN:
      return CDK_KEY_PRESS;
    case CDK_QUARTZ_KEY_UP:
      return CDK_KEY_RELEASE;
    case CDK_QUARTZ_FLAGS_CHANGED:
      break;
    default:
      g_assert_not_reached ();
    }
  
  /* For flags-changed events, we have to find the special key that caused the
   * event, and see if it's in the modifier mask. */
  keycode = [event keyCode];
  flags = [event modifierFlags];
  
  for (i = 0; i < G_N_ELEMENTS (modifier_keys); i++)
    {
      if (modifier_keys[i].keycode == keycode)
	{
	  if (flags & modifier_keys[i].modmask)
	    return CDK_KEY_PRESS;
	  else
	    return CDK_KEY_RELEASE;
	}
    }
  
  /* Some keypresses (eg: Expose' activations) seem to trigger flags-changed
   * events for no good reason. Ignore them! */
  return CDK_NOTHING;
}

gboolean
_cdk_quartz_keys_is_modifier (guint keycode)
{
  gint i;
  
  for (i = 0; i < G_N_ELEMENTS (modifier_keys); i++)
    {
      if (modifier_keys[i].modmask == 0)
	break;

      if (modifier_keys[i].keycode == keycode)
	return TRUE;
    }

  return FALSE;
}

static void
input_sources_changed_notification (CFNotificationCenterRef  center,
                                    void                    *observer,
                                    CFStringRef              name,
                                    const void              *object,
                                    CFDictionaryRef          userInfo)
{
  update_keymap ();
}

static void
cdk_quartz_keymap_init (CdkQuartzKeymap *keymap)
{
  update_keymap ();
  CFNotificationCenterAddObserver (CFNotificationCenterGetDistributedCenter (),
                                   keymap,
                                   input_sources_changed_notification,
                                   CFSTR ("AppleSelectedInputSourcesChangedNotification"),
                                   NULL,
                                   CFNotificationSuspensionBehaviorDeliverImmediately);
}

static void
cdk_quartz_keymap_finalize (GObject *object)
{
  CFNotificationCenterRemoveObserver (CFNotificationCenterGetDistributedCenter (),
                                      object,
                                      CFSTR ("AppleSelectedInputSourcesChangedNotification"),
                                      NULL);

  G_OBJECT_CLASS (cdk_quartz_keymap_parent_class)->finalize (object);
}

static void
cdk_quartz_keymap_class_init (CdkQuartzKeymapClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CdkKeymapClass *keymap_class = CDK_KEYMAP_CLASS (klass);

  object_class->finalize = cdk_quartz_keymap_finalize;

  keymap_class->get_direction = cdk_quartz_keymap_get_direction;
  keymap_class->have_bidi_layouts = cdk_quartz_keymap_have_bidi_layouts;
  keymap_class->get_caps_lock_state = cdk_quartz_keymap_get_caps_lock_state;
  keymap_class->get_num_lock_state = cdk_quartz_keymap_get_num_lock_state;
  keymap_class->get_scroll_lock_state = cdk_quartz_keymap_get_scroll_lock_state;
  keymap_class->get_entries_for_keyval = cdk_quartz_keymap_get_entries_for_keyval;
  keymap_class->get_entries_for_keycode = cdk_quartz_keymap_get_entries_for_keycode;
  keymap_class->lookup_key = cdk_quartz_keymap_lookup_key;
  keymap_class->translate_keyboard_state = cdk_quartz_keymap_translate_keyboard_state;
  keymap_class->add_virtual_modifiers = cdk_quartz_keymap_add_virtual_modifiers;
  keymap_class->map_virtual_modifiers = cdk_quartz_keymap_map_virtual_modifiers;
  keymap_class->get_modifier_mask = cdk_quartz_keymap_get_modifier_mask;
}
