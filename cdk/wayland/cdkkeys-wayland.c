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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <sys/mman.h>

#include "cdk.h"
#include "cdkwayland.h"

#include "cdkprivate-wayland.h"
#include "cdkinternals.h"
#include "cdkkeysprivate.h"

#include <xkbcommon/xkbcommon.h>

typedef struct _CdkWaylandKeymap          CdkWaylandKeymap;
typedef struct _CdkWaylandKeymapClass     CdkWaylandKeymapClass;

struct _CdkWaylandKeymap
{
  CdkKeymap parent_instance;

  struct xkb_keymap *xkb_keymap;
  struct xkb_state *xkb_state;

  PangoDirection *direction;
  gboolean bidi;
};

struct _CdkWaylandKeymapClass
{
  CdkKeymapClass parent_class;
};

#define CDK_TYPE_WAYLAND_KEYMAP          (_cdk_wayland_keymap_get_type ())
#define CDK_WAYLAND_KEYMAP(object)       (G_TYPE_CHECK_INSTANCE_CAST ((object), CDK_TYPE_WAYLAND_KEYMAP, CdkWaylandKeymap))
#define CDK_IS_WAYLAND_KEYMAP(object)    (G_TYPE_CHECK_INSTANCE_TYPE ((object), CDK_TYPE_WAYLAND_KEYMAP))

GType _cdk_wayland_keymap_get_type (void);

G_DEFINE_TYPE (CdkWaylandKeymap, _cdk_wayland_keymap, CDK_TYPE_KEYMAP)

static void
cdk_wayland_keymap_finalize (GObject *object)
{
  CdkWaylandKeymap *keymap = CDK_WAYLAND_KEYMAP (object);

  xkb_keymap_unref (keymap->xkb_keymap);
  xkb_state_unref (keymap->xkb_state);
  g_free (keymap->direction);

  G_OBJECT_CLASS (_cdk_wayland_keymap_parent_class)->finalize (object);
}

static PangoDirection
cdk_wayland_keymap_get_direction (CdkKeymap *keymap)
{
  CdkWaylandKeymap *keymap_wayland = CDK_WAYLAND_KEYMAP (keymap);
  gint i;

  for (i = 0; i < xkb_keymap_num_layouts (keymap_wayland->xkb_keymap); i++)
    {
      if (xkb_state_layout_index_is_active (keymap_wayland->xkb_state, i, XKB_STATE_LAYOUT_EFFECTIVE))
        return keymap_wayland->direction[i];
    }

  return PANGO_DIRECTION_NEUTRAL;
}

static gboolean
cdk_wayland_keymap_have_bidi_layouts (CdkKeymap *keymap)
{
  CdkWaylandKeymap *keymap_wayland = CDK_WAYLAND_KEYMAP (keymap);

  return keymap_wayland->bidi;
}

static gboolean
cdk_wayland_keymap_get_caps_lock_state (CdkKeymap *keymap)
{
  return xkb_state_led_name_is_active (CDK_WAYLAND_KEYMAP (keymap)->xkb_state,
                                       XKB_LED_NAME_CAPS);
}

static gboolean
cdk_wayland_keymap_get_num_lock_state (CdkKeymap *keymap)
{
  return xkb_state_led_name_is_active (CDK_WAYLAND_KEYMAP (keymap)->xkb_state,
                                       XKB_LED_NAME_NUM);
}

static gboolean
cdk_wayland_keymap_get_scroll_lock_state (CdkKeymap *keymap)
{
  return xkb_state_led_name_is_active (CDK_WAYLAND_KEYMAP (keymap)->xkb_state,
                                       XKB_LED_NAME_SCROLL);
}

static gboolean
cdk_wayland_keymap_get_entries_for_keyval (CdkKeymap     *keymap,
					   guint          keyval,
					   CdkKeymapKey **keys,
					   gint          *n_keys)
{
  struct xkb_keymap *xkb_keymap = CDK_WAYLAND_KEYMAP (keymap)->xkb_keymap;
  GArray *retval;
  guint keycode;
  xkb_keycode_t min_keycode, max_keycode;

  retval = g_array_new (FALSE, FALSE, sizeof (CdkKeymapKey));

  min_keycode = xkb_keymap_min_keycode (xkb_keymap);
  max_keycode = xkb_keymap_max_keycode (xkb_keymap);
  for (keycode = min_keycode; keycode < max_keycode; keycode++)
    {
      gint num_layouts, layout;
      num_layouts = xkb_keymap_num_layouts_for_key (xkb_keymap, keycode);
      for (layout = 0; layout < num_layouts; layout++)
        {
          gint num_levels, level;
          num_levels = xkb_keymap_num_levels_for_key (xkb_keymap, keycode, layout);
          for (level = 0; level < num_levels; level++)
            {
              const xkb_keysym_t *syms;
              gint num_syms, sym;
              num_syms = xkb_keymap_key_get_syms_by_level (xkb_keymap, keycode, layout, level, &syms);
              for (sym = 0; sym < num_syms; sym++)
                {
                  if (syms[sym] == keyval)
                    {
                      CdkKeymapKey key;

                      key.keycode = keycode;
                      key.group = layout;
                      key.level = level;

                      g_array_append_val (retval, key);
                    }
                }
            }
        }
    }

  *n_keys = retval->len;
  *keys = (CdkKeymapKey*) g_array_free (retval, FALSE);

  return TRUE;
}

static gboolean
cdk_wayland_keymap_get_entries_for_keycode (CdkKeymap     *keymap,
					    guint          hardware_keycode,
					    CdkKeymapKey **keys,
					    guint        **keyvals,
					    gint          *n_entries)
{
  struct xkb_keymap *xkb_keymap = CDK_WAYLAND_KEYMAP (keymap)->xkb_keymap;
  gint num_layouts, layout;
  gint num_entries;
  gint i;

  num_layouts = xkb_keymap_num_layouts_for_key (xkb_keymap, hardware_keycode);

  num_entries = 0;
  for (layout = 0; layout < num_layouts; layout++)
    num_entries += xkb_keymap_num_levels_for_key (xkb_keymap, hardware_keycode,  layout);

 if (n_entries)
    *n_entries = num_entries;
  if (keys)
    *keys = g_new0 (CdkKeymapKey, num_entries);
  if (keyvals)
    *keyvals = g_new0 (guint, num_entries);

  i = 0;
  for (layout = 0; layout < num_layouts; layout++)
    {
      gint num_levels, level;
      num_levels = xkb_keymap_num_levels_for_key (xkb_keymap, hardware_keycode, layout);
      for (level = 0; level < num_levels; level++)
        {
          const xkb_keysym_t *syms;
          int num_syms;
          num_syms = xkb_keymap_key_get_syms_by_level (xkb_keymap, hardware_keycode, layout, 0, &syms);
          if (keys)
            {
              (*keys)[i].keycode = hardware_keycode;
              (*keys)[i].group = layout;
              (*keys)[i].level = level;
            }
          if (keyvals && num_syms > 0)
            (*keyvals)[i] = syms[0];

          i++;
        }
    }

  return num_entries > 0;
}

static guint
cdk_wayland_keymap_lookup_key (CdkKeymap          *keymap,
			       const CdkKeymapKey *key)
{
  struct xkb_keymap *xkb_keymap = CDK_WAYLAND_KEYMAP (keymap)->xkb_keymap;
  const xkb_keysym_t *syms;
  int num_syms;

  num_syms = xkb_keymap_key_get_syms_by_level (xkb_keymap,
                                               key->keycode,
                                               key->group,
                                               key->level,
                                               &syms);
  if (num_syms > 0)
    return syms[0];
  else
    return XKB_KEY_NoSymbol;
}

static guint32
get_xkb_modifiers (struct xkb_keymap *xkb_keymap,
                   CdkModifierType    state)
{
  guint32 mods = 0;

  if (state & CDK_SHIFT_MASK)
    mods |= 1 << xkb_keymap_mod_get_index (xkb_keymap, XKB_MOD_NAME_SHIFT);
  if (state & CDK_LOCK_MASK)
    mods |= 1 << xkb_keymap_mod_get_index (xkb_keymap, XKB_MOD_NAME_CAPS);
  if (state & CDK_CONTROL_MASK)
    mods |= 1 << xkb_keymap_mod_get_index (xkb_keymap, XKB_MOD_NAME_CTRL);
  if (state & CDK_MOD1_MASK)
    mods |= 1 << xkb_keymap_mod_get_index (xkb_keymap, XKB_MOD_NAME_ALT);
  if (state & CDK_MOD2_MASK)
    mods |= 1 << xkb_keymap_mod_get_index (xkb_keymap, XKB_MOD_NAME_NUM);
  if (state & CDK_MOD3_MASK)
    mods |= 1 << xkb_keymap_mod_get_index (xkb_keymap, "Mod3");
  if (state & CDK_MOD4_MASK)
    mods |= 1 << xkb_keymap_mod_get_index (xkb_keymap, XKB_MOD_NAME_LOGO);
  if (state & CDK_MOD5_MASK)
    mods |= 1 << xkb_keymap_mod_get_index (xkb_keymap, "Mod5");
  if (state & CDK_SUPER_MASK)
    mods |= 1 << xkb_keymap_mod_get_index (xkb_keymap, "Super");
  if (state & CDK_HYPER_MASK)
    mods |= 1 << xkb_keymap_mod_get_index (xkb_keymap, "Hyper");
  if (state & CDK_META_MASK)
    mods |= 1 << xkb_keymap_mod_get_index (xkb_keymap, "Meta");

  return mods;
}

static CdkModifierType
get_cdk_modifiers (struct xkb_keymap *xkb_keymap,
                   guint32            mods)
{
  CdkModifierType state = 0;

  if (mods & (1 << xkb_keymap_mod_get_index (xkb_keymap, XKB_MOD_NAME_SHIFT)))
    state |= CDK_SHIFT_MASK;
  if (mods & (1 << xkb_keymap_mod_get_index (xkb_keymap, XKB_MOD_NAME_CAPS)))
    state |= CDK_LOCK_MASK;
  if (mods & (1 << xkb_keymap_mod_get_index (xkb_keymap, XKB_MOD_NAME_CTRL)))
    state |= CDK_CONTROL_MASK;
  if (mods & (1 << xkb_keymap_mod_get_index (xkb_keymap, XKB_MOD_NAME_ALT)))
    state |= CDK_MOD1_MASK;
  if (mods & (1 << xkb_keymap_mod_get_index (xkb_keymap, XKB_MOD_NAME_NUM)))
    state |= CDK_MOD2_MASK;
  if (mods & (1 << xkb_keymap_mod_get_index (xkb_keymap, "Mod3")))
    state |= CDK_MOD3_MASK;
  if (mods & (1 << xkb_keymap_mod_get_index (xkb_keymap, XKB_MOD_NAME_LOGO)))
    state |= CDK_MOD4_MASK;
  if (mods & (1 << xkb_keymap_mod_get_index (xkb_keymap, "Mod5")))
    state |= CDK_MOD5_MASK;
  if (mods & (1 << xkb_keymap_mod_get_index (xkb_keymap, "Super")))
    state |= CDK_SUPER_MASK;
  if (mods & (1 << xkb_keymap_mod_get_index (xkb_keymap, "Hyper")))
    state |= CDK_HYPER_MASK;
  /* Ctk+ treats MOD1 as a synonym for Alt, and does not expect it to
   * be mapped around, so we should avoid adding CDK_META_MASK if MOD1
   * is already included to avoid confusing ctk+ and applications that
   * rely on that behavior.
   */
  if (mods & (1 << xkb_keymap_mod_get_index (xkb_keymap, "Meta")) &&
      (state & CDK_MOD1_MASK) == 0)
    state |= CDK_META_MASK;

  return state;
}

static gboolean
cdk_wayland_keymap_translate_keyboard_state (CdkKeymap       *keymap,
					     guint            hardware_keycode,
					     CdkModifierType  state,
					     gint             group,
					     guint           *keyval,
					     gint            *effective_group,
					     gint            *effective_level,
					     CdkModifierType *consumed_modifiers)
{
  struct xkb_keymap *xkb_keymap;
  struct xkb_state *xkb_state;
  guint32 modifiers;
  guint32 consumed;
  xkb_layout_index_t layout;
  xkb_level_index_t level;
  xkb_keysym_t sym;

  g_return_val_if_fail (keymap == NULL || CDK_IS_KEYMAP (keymap), FALSE);
  g_return_val_if_fail (group < 4, FALSE);

  xkb_keymap = CDK_WAYLAND_KEYMAP (keymap)->xkb_keymap;

  modifiers = get_xkb_modifiers (xkb_keymap, state);

  xkb_state = xkb_state_new (xkb_keymap);

  xkb_state_update_mask (xkb_state, modifiers, 0, 0, group, 0, 0);

  layout = xkb_state_key_get_layout (xkb_state, hardware_keycode);
  level = xkb_state_key_get_level (xkb_state, hardware_keycode, layout);
  sym = xkb_state_key_get_one_sym (xkb_state, hardware_keycode);
  consumed = modifiers & ~xkb_state_mod_mask_remove_consumed (xkb_state, hardware_keycode, modifiers);

  xkb_state_unref (xkb_state);

  if (keyval)
    *keyval = sym;
  if (effective_group)
    *effective_group = layout;
  if (effective_level)
    *effective_level = level;
  if (consumed_modifiers)
    *consumed_modifiers = get_cdk_modifiers (xkb_keymap, consumed);

  return (sym != XKB_KEY_NoSymbol);
}

static guint
cdk_wayland_keymap_get_modifier_state (CdkKeymap *keymap)
{
  struct xkb_keymap *xkb_keymap = CDK_WAYLAND_KEYMAP (keymap)->xkb_keymap;
  struct xkb_state *xkb_state = CDK_WAYLAND_KEYMAP (keymap)->xkb_state;
  xkb_mod_mask_t mods;

  mods = xkb_state_serialize_mods (xkb_state, XKB_STATE_MODS_EFFECTIVE);

  return get_cdk_modifiers (xkb_keymap, mods);
}

static void
cdk_wayland_keymap_add_virtual_modifiers (CdkKeymap       *keymap,
					  CdkModifierType *state)
{
  struct xkb_keymap *xkb_keymap;
  struct xkb_state *xkb_state;
  xkb_mod_index_t idx;
  uint32_t mods, real;
  struct { const char *name; CdkModifierType mask; } vmods[] = {
    { "Super", CDK_SUPER_MASK },
    { "Hyper", CDK_HYPER_MASK },
    { "Meta", CDK_META_MASK },
    { NULL, 0 }
  };
  int i;

  xkb_keymap = CDK_WAYLAND_KEYMAP (keymap)->xkb_keymap;
  mods = get_xkb_modifiers (xkb_keymap, *state);

  xkb_state = xkb_state_new (xkb_keymap);

  for (i = 0; vmods[i].name; i++)
    {
      idx = xkb_keymap_mod_get_index (xkb_keymap, vmods[i].name);
      if (idx == XKB_MOD_INVALID)
        continue;

      xkb_state_update_mask (xkb_state, 1 << idx, 0, 0, 0, 0, 0);
      real = xkb_state_serialize_mods (xkb_state, XKB_STATE_MODS_EFFECTIVE);
      real &= 0xf0; /* ignore mapping to Lock, Shift, Control, Mod1 */
      if (mods & real)
        *state |= vmods[i].mask;
      xkb_state_update_mask (xkb_state, 0, 0, 0, 0, 0, 0);
    }

  xkb_state_unref (xkb_state);
}

static gboolean
cdk_wayland_keymap_map_virtual_modifiers (CdkKeymap       *keymap,
					  CdkModifierType *state)
{
  struct xkb_keymap *xkb_keymap;
  struct xkb_state *xkb_state;
  uint32_t mods, mapped;
  gboolean ret = TRUE;

  xkb_keymap = CDK_WAYLAND_KEYMAP (keymap)->xkb_keymap;
  mods = get_xkb_modifiers (xkb_keymap, *state);

  xkb_state = xkb_state_new (xkb_keymap);
  xkb_state_update_mask (xkb_state, mods & ~0xff, 0, 0, 0, 0, 0);
  mapped = xkb_state_serialize_mods (xkb_state, XKB_STATE_MODS_EFFECTIVE);
  if ((mapped & mods & 0xff) != 0)
    ret = FALSE;
  *state |= get_cdk_modifiers (xkb_keymap, mapped);

  xkb_state_unref (xkb_state);

  return ret;
}

static void
_cdk_wayland_keymap_class_init (CdkWaylandKeymapClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CdkKeymapClass *keymap_class = CDK_KEYMAP_CLASS (klass);

  object_class->finalize = cdk_wayland_keymap_finalize;

  keymap_class->get_direction = cdk_wayland_keymap_get_direction;
  keymap_class->have_bidi_layouts = cdk_wayland_keymap_have_bidi_layouts;
  keymap_class->get_caps_lock_state = cdk_wayland_keymap_get_caps_lock_state;
  keymap_class->get_num_lock_state = cdk_wayland_keymap_get_num_lock_state;
  keymap_class->get_scroll_lock_state = cdk_wayland_keymap_get_scroll_lock_state;
  keymap_class->get_entries_for_keyval = cdk_wayland_keymap_get_entries_for_keyval;
  keymap_class->get_entries_for_keycode = cdk_wayland_keymap_get_entries_for_keycode;
  keymap_class->lookup_key = cdk_wayland_keymap_lookup_key;
  keymap_class->translate_keyboard_state = cdk_wayland_keymap_translate_keyboard_state;
  keymap_class->get_modifier_state = cdk_wayland_keymap_get_modifier_state;
  keymap_class->add_virtual_modifiers = cdk_wayland_keymap_add_virtual_modifiers;
  keymap_class->map_virtual_modifiers = cdk_wayland_keymap_map_virtual_modifiers;
}

static void
_cdk_wayland_keymap_init (CdkWaylandKeymap *keymap)
{
}

static void
update_direction (CdkWaylandKeymap *keymap)
{
  gint num_layouts;
  gint i;
  gint *rtl;
  xkb_keycode_t min_keycode, max_keycode;
  guint key;
  gboolean have_rtl, have_ltr;

  num_layouts = xkb_keymap_num_layouts (keymap->xkb_keymap);

  keymap->direction = g_renew (PangoDirection, keymap->direction, num_layouts);
  rtl = g_newa (gint, num_layouts);
  for (i = 0; i < num_layouts; i++)
    rtl[i] = 0;

  min_keycode = xkb_keymap_min_keycode (keymap->xkb_keymap);
  max_keycode = xkb_keymap_max_keycode (keymap->xkb_keymap);
  for (key = min_keycode; key < max_keycode; key++)
    {
       gint layouts, layout;

       layouts = xkb_keymap_num_layouts_for_key (keymap->xkb_keymap, key);
       for (layout = 0; layout < layouts; layout++)
         {
           const xkb_keysym_t *syms;
           gint num_syms;
           gint sym;

           num_syms = xkb_keymap_key_get_syms_by_level (keymap->xkb_keymap, key, layout, 0, &syms);
           for (sym = 0; sym < num_syms; sym++)
             {
               PangoDirection dir;

               dir = cdk_unichar_direction (xkb_keysym_to_utf32 (syms[sym]));
               switch (dir)
                 {
                 case PANGO_DIRECTION_RTL:
                   rtl[layout]++;
                   break;
                 case PANGO_DIRECTION_LTR:
                   rtl[layout]--;
                   break;
                 default:
                   break;
                 }
             }
         }
    }

  have_rtl = have_ltr = FALSE;
  for (i = 0; i < num_layouts; i++)
    {
      if (rtl[i] > 0)
        {
          keymap->direction[i] = PANGO_DIRECTION_RTL;
          have_rtl = TRUE;
        }
      else
        {
          keymap->direction[i] = PANGO_DIRECTION_LTR;
          have_ltr = TRUE;
        }
    }

  if (have_rtl && have_ltr)
    keymap->bidi = TRUE;
}

CdkKeymap *
_cdk_wayland_keymap_new (void)
{
  CdkWaylandKeymap *keymap;
  struct xkb_context *context;
  struct xkb_rule_names names;

  keymap = g_object_new (_cdk_wayland_keymap_get_type(), NULL);

  context = xkb_context_new (0);

  names.rules = "evdev";
  names.model = "pc105";
  names.layout = "us";
  names.variant = "";
  names.options = "";
  keymap->xkb_keymap = xkb_keymap_new_from_names (context, &names, 0);
  keymap->xkb_state = xkb_state_new (keymap->xkb_keymap);
  xkb_context_unref (context);

  update_direction (keymap);

  return CDK_KEYMAP (keymap);
}

#ifdef G_ENABLE_DEBUG
static void
print_modifiers (struct xkb_keymap *keymap)
{
  int i, j;
  uint32_t real;
  struct xkb_state *state;

  g_print ("modifiers:\n");
  for (i = 0; i < xkb_keymap_num_mods (keymap); i++)
    g_print ("%s ", xkb_keymap_mod_get_name (keymap, i));
  g_print ("\n\n");

  g_print ("modifier mapping\n");
  state = xkb_state_new (keymap);
  for (i = 0; i < 8; i++)
    {
      gboolean need_arrow = TRUE;
      g_print ("%s ", xkb_keymap_mod_get_name (keymap, i));
      for (j = 8; j < xkb_keymap_num_mods (keymap); j++)
        {
          xkb_state_update_mask (state, 1 << j, 0, 0, 0, 0, 0);
          real = xkb_state_serialize_mods (state, XKB_STATE_MODS_EFFECTIVE);
          if (real & (1 << i))
            {
              if (need_arrow)
                {
                  g_print ("-> ");
                  need_arrow = FALSE;
                }
              g_print ("%s ", xkb_keymap_mod_get_name (keymap, j));
            }
        }
      g_print ("\n");
    }

  xkb_state_unref (state);
}
#endif

void
_cdk_wayland_keymap_update_from_fd (CdkKeymap *keymap,
                                    uint32_t   format,
                                    uint32_t   fd,
                                    uint32_t   size)
{
  CdkWaylandKeymap *keymap_wayland = CDK_WAYLAND_KEYMAP (keymap);
  struct xkb_context *context;
  struct xkb_keymap *xkb_keymap;
  char *map_str;

  context = xkb_context_new (0);

  map_str = mmap (NULL, size, PROT_READ, MAP_SHARED, fd, 0);
  if (map_str == MAP_FAILED)
    {
      close(fd);
      return;
    }

  CDK_NOTE(INPUT, g_print ("keymap:\n%s\n", map_str));

  xkb_keymap = xkb_keymap_new_from_string (context, map_str, format, 0);
  munmap (map_str, size);
  close (fd);

  if (!xkb_keymap)
    {
      g_warning ("Got invalid keymap from compositor, keeping previous/default one");
      xkb_context_unref (context);
      return;
    }

  CDK_NOTE(INPUT, print_modifiers (xkb_keymap));

  xkb_keymap_unref (keymap_wayland->xkb_keymap);
  keymap_wayland->xkb_keymap = xkb_keymap;

  xkb_state_unref (keymap_wayland->xkb_state);
  keymap_wayland->xkb_state = xkb_state_new (keymap_wayland->xkb_keymap);

  xkb_context_unref (context);

  update_direction (keymap_wayland);
}

struct xkb_keymap *
_cdk_wayland_keymap_get_xkb_keymap (CdkKeymap *keymap)
{
  return CDK_WAYLAND_KEYMAP (keymap)->xkb_keymap;
}

struct xkb_state *
_cdk_wayland_keymap_get_xkb_state (CdkKeymap *keymap)
{
  return CDK_WAYLAND_KEYMAP (keymap)->xkb_state;
}

gboolean
_cdk_wayland_keymap_key_is_modifier (CdkKeymap *keymap,
                                     guint      keycode)
{
  struct xkb_keymap *xkb_keymap = CDK_WAYLAND_KEYMAP (keymap)->xkb_keymap;
  struct xkb_state *xkb_state;
  gboolean is_modifier;

  is_modifier = FALSE;

  xkb_state = xkb_state_new (xkb_keymap);

  if (xkb_state_update_key (xkb_state, keycode, XKB_KEY_DOWN) & XKB_STATE_MODS_EFFECTIVE)
    is_modifier = TRUE;

  xkb_state_unref (xkb_state);

  return is_modifier;
}
