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

#include "cdkprivate-broadway.h"
#include "cdkinternals.h"
#include "cdkdisplay-broadway.h"
#include "cdkkeysprivate.h"
#include "cdkkeysyms.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <limits.h>
#include <errno.h>

typedef struct _CdkBroadwayKeymap   CdkBroadwayKeymap;
typedef struct _CdkKeymapClass CdkBroadwayKeymapClass;

#define CDK_TYPE_BROADWAY_KEYMAP          (cdk_broadway_keymap_get_type ())
#define CDK_BROADWAY_KEYMAP(object)       (G_TYPE_CHECK_INSTANCE_CAST ((object), CDK_TYPE_BROADWAY_KEYMAP, CdkBroadwayKeymap))
#define CDK_IS_BROADWAY_KEYMAP(object)    (G_TYPE_CHECK_INSTANCE_TYPE ((object), CDK_TYPE_BROADWAY_KEYMAP))

static GType cdk_broadway_keymap_get_type (void);

typedef struct _DirectionCacheEntry DirectionCacheEntry;

struct _CdkBroadwayKeymap
{
  CdkKeymap     parent_instance;
};

struct _CdkBroadwayKeymapClass
{
  CdkKeymapClass keymap_class;
};

G_DEFINE_TYPE (CdkBroadwayKeymap, cdk_broadway_keymap, CDK_TYPE_KEYMAP)

static void  cdk_broadway_keymap_finalize   (GObject           *object);

static void
cdk_broadway_keymap_init (CdkBroadwayKeymap *keymap G_GNUC_UNUSED)
{
}

static void
cdk_broadway_keymap_finalize (GObject *object)
{
  G_OBJECT_CLASS (cdk_broadway_keymap_parent_class)->finalize (object);
}

CdkKeymap*
_cdk_broadway_display_get_keymap (CdkDisplay *display)
{
  CdkBroadwayDisplay *broadway_display;

  g_return_val_if_fail (CDK_IS_DISPLAY (display), NULL);
  broadway_display = CDK_BROADWAY_DISPLAY (display);

  if (!broadway_display->keymap)
    broadway_display->keymap = g_object_new (cdk_broadway_keymap_get_type (), NULL);

  broadway_display->keymap->display = display;

  return broadway_display->keymap;
}

static PangoDirection
cdk_broadway_keymap_get_direction (CdkKeymap *keymap G_GNUC_UNUSED)
{
  return PANGO_DIRECTION_NEUTRAL;
}

static gboolean
cdk_broadway_keymap_have_bidi_layouts (CdkKeymap *keymap G_GNUC_UNUSED)
{
  return FALSE;
}

static gboolean
cdk_broadway_keymap_get_caps_lock_state (CdkKeymap *keymap G_GNUC_UNUSED)
{
  return FALSE;
}

static gboolean
cdk_broadway_keymap_get_num_lock_state (CdkKeymap *keymap G_GNUC_UNUSED)
{
  return FALSE;
}

static gboolean
cdk_broadway_keymap_get_scroll_lock_state (CdkKeymap *keymap G_GNUC_UNUSED)
{
  return FALSE;
}

static gboolean
cdk_broadway_keymap_get_entries_for_keyval (CdkKeymap     *keymap G_GNUC_UNUSED,
					    guint          keyval,
					    CdkKeymapKey **keys,
					    gint          *n_keys)
{
  if (n_keys)
    *n_keys = 1;
  if (keys)
    {
      *keys = g_new0 (CdkKeymapKey, 1);
      (*keys)->keycode = keyval;
    }

  return TRUE;
}

static gboolean
cdk_broadway_keymap_get_entries_for_keycode (CdkKeymap     *keymap G_GNUC_UNUSED,
					     guint          hardware_keycode,
					     CdkKeymapKey **keys,
					     guint        **keyvals,
					     gint          *n_entries)
{
  if (n_entries)
    *n_entries = 1;
  if (keys)
    {
      *keys = g_new0 (CdkKeymapKey, 1);
      (*keys)->keycode = hardware_keycode;
    }
  if (keyvals)
    {
      *keyvals = g_new0 (guint, 1);
      (*keyvals)[0] = hardware_keycode;
    }
  return TRUE;
}

static guint
cdk_broadway_keymap_lookup_key (CdkKeymap          *keymap G_GNUC_UNUSED,
				const CdkKeymapKey *key)
{
  return key->keycode;
}


static gboolean
cdk_broadway_keymap_translate_keyboard_state (CdkKeymap       *keymap G_GNUC_UNUSED,
					      guint            hardware_keycode,
					      CdkModifierType  state G_GNUC_UNUSED,
					      gint             group G_GNUC_UNUSED,
					      guint           *keyval,
					      gint            *effective_group,
					      gint            *level,
					      CdkModifierType *consumed_modifiers G_GNUC_UNUSED)
{
  if (keyval)
    *keyval = hardware_keycode;
  if (effective_group)
    *effective_group = 0;
  if (level)
    *level = 0;
  return TRUE;
}

static void
cdk_broadway_keymap_add_virtual_modifiers (CdkKeymap       *keymap G_GNUC_UNUSED,
					   CdkModifierType *state G_GNUC_UNUSED)
{
}

static gboolean
cdk_broadway_keymap_map_virtual_modifiers (CdkKeymap       *keymap G_GNUC_UNUSED,
					   CdkModifierType *state G_GNUC_UNUSED)
{
  return TRUE;
}

static void
cdk_broadway_keymap_class_init (CdkBroadwayKeymapClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CdkKeymapClass *keymap_class = CDK_KEYMAP_CLASS (klass);

  object_class->finalize = cdk_broadway_keymap_finalize;

  keymap_class->get_direction = cdk_broadway_keymap_get_direction;
  keymap_class->have_bidi_layouts = cdk_broadway_keymap_have_bidi_layouts;
  keymap_class->get_caps_lock_state = cdk_broadway_keymap_get_caps_lock_state;
  keymap_class->get_num_lock_state = cdk_broadway_keymap_get_num_lock_state;
  keymap_class->get_scroll_lock_state = cdk_broadway_keymap_get_scroll_lock_state;
  keymap_class->get_entries_for_keyval = cdk_broadway_keymap_get_entries_for_keyval;
  keymap_class->get_entries_for_keycode = cdk_broadway_keymap_get_entries_for_keycode;
  keymap_class->lookup_key = cdk_broadway_keymap_lookup_key;
  keymap_class->translate_keyboard_state = cdk_broadway_keymap_translate_keyboard_state;
  keymap_class->add_virtual_modifiers = cdk_broadway_keymap_add_virtual_modifiers;
  keymap_class->map_virtual_modifiers = cdk_broadway_keymap_map_virtual_modifiers;
}

