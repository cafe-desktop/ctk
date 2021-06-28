/* CDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * Copyright (C) 1998-2002 Tor Lillqvist
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
#include <string.h>
#include <stdlib.h>
#include <glib/gprintf.h>
#include <pango/pangowin32.h>

#include "cdkscreen.h"
#include "cdkproperty.h"
#include "cdkselection.h"
#include "cdkprivate-win32.h"
#include "cdkmonitor-win32.h"
#include "cdkwin32.h"

CdkAtom
_cdk_win32_display_manager_atom_intern (CdkDisplayManager *manager,
					const gchar *atom_name,
					gint         only_if_exists)
{
  ATOM win32_atom;
  CdkAtom retval;
  static GHashTable *atom_hash = NULL;

  if (!atom_hash)
    atom_hash = g_hash_table_new (g_str_hash, g_str_equal);

  retval = g_hash_table_lookup (atom_hash, atom_name);
  if (!retval)
    {
      if (strcmp (atom_name, "PRIMARY") == 0)
	retval = CDK_SELECTION_PRIMARY;
      else if (strcmp (atom_name, "SECONDARY") == 0)
	retval = CDK_SELECTION_SECONDARY;
      else if (strcmp (atom_name, "CLIPBOARD") == 0)
	retval = CDK_SELECTION_CLIPBOARD;
      else if (strcmp (atom_name, "ATOM") == 0)
	retval = CDK_SELECTION_TYPE_ATOM;
      else if (strcmp (atom_name, "BITMAP") == 0)
	retval = CDK_SELECTION_TYPE_BITMAP;
      else if (strcmp (atom_name, "COLORMAP") == 0)
	retval = CDK_SELECTION_TYPE_COLORMAP;
      else if (strcmp (atom_name, "DRAWABLE") == 0)
	retval = CDK_SELECTION_TYPE_DRAWABLE;
      else if (strcmp (atom_name, "INTEGER") == 0)
	retval = CDK_SELECTION_TYPE_INTEGER;
      else if (strcmp (atom_name, "PIXMAP") == 0)
	retval = CDK_SELECTION_TYPE_PIXMAP;
      else if (strcmp (atom_name, "WINDOW") == 0)
	retval = CDK_SELECTION_TYPE_WINDOW;
      else if (strcmp (atom_name, "STRING") == 0)
	retval = CDK_SELECTION_TYPE_STRING;
      else
	{
	  win32_atom = GlobalAddAtom (atom_name);
	  retval = GUINT_TO_POINTER ((guint) win32_atom);
	}
      g_hash_table_insert (atom_hash,
			   g_strdup (atom_name),
			   retval);
    }

  return retval;
}

gchar *
_cdk_win32_display_manager_get_atom_name (CdkDisplayManager *manager,
					  CdkAtom            atom)
{
  ATOM win32_atom;
  gchar name[256];

  if (CDK_NONE == atom) return g_strdup ("<none>");
  else if (CDK_SELECTION_PRIMARY == atom) return g_strdup ("PRIMARY");
  else if (CDK_SELECTION_SECONDARY == atom) return g_strdup ("SECONDARY");
  else if (CDK_SELECTION_CLIPBOARD == atom) return g_strdup ("CLIPBOARD");
  else if (CDK_SELECTION_TYPE_ATOM == atom) return g_strdup ("ATOM");
  else if (CDK_SELECTION_TYPE_BITMAP == atom) return g_strdup ("BITMAP");
  else if (CDK_SELECTION_TYPE_COLORMAP == atom) return g_strdup ("COLORMAP");
  else if (CDK_SELECTION_TYPE_DRAWABLE == atom) return g_strdup ("DRAWABLE");
  else if (CDK_SELECTION_TYPE_INTEGER == atom) return g_strdup ("INTEGER");
  else if (CDK_SELECTION_TYPE_PIXMAP == atom) return g_strdup ("PIXMAP");
  else if (CDK_SELECTION_TYPE_WINDOW == atom) return g_strdup ("WINDOW");
  else if (CDK_SELECTION_TYPE_STRING == atom) return g_strdup ("STRING");

  win32_atom = GPOINTER_TO_UINT (atom);

  if (win32_atom < 0xC000)
    return g_strdup_printf ("#%p", atom);
  else if (GlobalGetAtomName (win32_atom, name, sizeof (name)) == 0)
    return NULL;
  return g_strdup (name);
}

gint
_cdk_win32_window_get_property (CdkWindow   *window,
		  CdkAtom      property,
		  CdkAtom      type,
		  gulong       offset,
		  gulong       length,
		  gint         pdelete,
		  CdkAtom     *actual_property_type,
		  gint        *actual_format_type,
		  gint        *actual_length,
		  guchar     **data)
{
  g_return_val_if_fail (window != NULL, FALSE);
  g_return_val_if_fail (CDK_IS_WINDOW (window), FALSE);

  if (CDK_WINDOW_DESTROYED (window))
    return FALSE;

  g_warning ("cdk_property_get: Not implemented");

  return FALSE;
}

void
_cdk_win32_window_change_property (CdkWindow         *window,
                                   CdkAtom            property,
                                   CdkAtom            type,
                                   gint               format,
                                   CdkPropMode        mode,
                                   const guchar      *data,
                                   gint               nelements)
{
  CdkWin32Selection *win32_sel = _cdk_win32_selection_get ();

  g_return_if_fail (window != NULL);
  g_return_if_fail (CDK_IS_WINDOW (window));

  if (CDK_WINDOW_DESTROYED (window))
    return;

  CDK_NOTE (DND, {
      gchar *prop_name = cdk_atom_name (property);
      gchar *type_name = cdk_atom_name (type);
      gchar *datastring = _cdk_win32_data_to_string (data, MIN (10, format*nelements/8));

      g_print ("cdk_property_change: %p %s %s %s %d*%d bits: %s\n",
	       CDK_WINDOW_HWND (window),
	       prop_name,
	       type_name,
	       (mode == CDK_PROP_MODE_REPLACE ? "REPLACE" :
		(mode == CDK_PROP_MODE_PREPEND ? "PREPEND" :
		 (mode == CDK_PROP_MODE_APPEND ? "APPEND" :
		  "???"))),
	       format, nelements,
	       datastring);
      g_free (datastring);
      g_free (prop_name);
      g_free (type_name);
    });

#ifndef G_DISABLE_CHECKS
  /* We should never come here for these types */
  if (G_UNLIKELY (type == _cdk_win32_selection_atom (CDK_WIN32_ATOM_INDEX_COMPOUND_TEXT) ||
                  type == _cdk_win32_selection_atom (CDK_WIN32_ATOM_INDEX_SAVE_TARGETS)))
    {
      g_return_if_fail_warning (G_LOG_DOMAIN,
                                G_STRFUNC,
                                "change_property called with a bad type");
      return;
    }
#endif

  if (property == _cdk_win32_selection_atom (CDK_WIN32_ATOM_INDEX_CDK_SELECTION) ||
      property == _cdk_win32_selection_atom (CDK_WIN32_ATOM_INDEX_OLE2_DND) ||
      property == _cdk_win32_selection_atom (CDK_WIN32_ATOM_INDEX_LOCAL_DND_SELECTION))
    {
      _cdk_win32_selection_property_change (win32_sel,
                                            window,
                                            property,
                                            type,
                                            format,
                                            mode,
                                            data,
                                            nelements);
    }
  else
    g_warning ("cdk_property_change: General case not implemented");
}

void
_cdk_win32_window_delete_property (CdkWindow *window,
				   CdkAtom    property)
{
  gchar *prop_name;

  g_return_if_fail (window != NULL);
  g_return_if_fail (CDK_IS_WINDOW (window));

  CDK_NOTE (DND, {
      prop_name = cdk_atom_name (property);

      g_print ("cdk_property_delete: %p %s\n",
	       CDK_WINDOW_HWND (window),
	       prop_name);
      g_free (prop_name);
    });

  if (property == _cdk_win32_selection_atom (CDK_WIN32_ATOM_INDEX_CDK_SELECTION) ||
      property == _cdk_win32_selection_atom (CDK_WIN32_ATOM_INDEX_OLE2_DND))
    _cdk_selection_property_delete (window);
  else if (property == _cdk_win32_selection_atom (CDK_WIN32_ATOM_INDEX_WM_TRANSIENT_FOR))
    {
      CdkScreen *screen;

      screen = cdk_window_get_screen (window);
      cdk_window_set_transient_for (window, cdk_screen_get_root_window (screen));
    }
  else
    {
      prop_name = cdk_atom_name (property);
      g_warning ("cdk_property_delete: General case (%s) not implemented",
		 prop_name);
      g_free (prop_name);
    }
}

static gchar*
_get_system_font_name (HDC hdc)
{
  NONCLIENTMETRICSW ncm;
  PangoFontDescription *font_desc;
  gchar *result, *font_desc_string;
  int logpixelsy;
  gint font_size;

  ncm.cbSize = sizeof(NONCLIENTMETRICSW);
  if (!SystemParametersInfoW (SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0))
    return NULL;

  logpixelsy = GetDeviceCaps (hdc, LOGPIXELSY);
  font_desc = pango_win32_font_description_from_logfontw (&ncm.lfMessageFont);
  font_desc_string = pango_font_description_to_string (font_desc);
  pango_font_description_free (font_desc);

  /* https://docs.microsoft.com/en-us/windows/desktop/api/wingdi/ns-wingdi-taglogfonta */
  font_size = -MulDiv (ncm.lfMessageFont.lfHeight, 72, logpixelsy);
  result = g_strdup_printf ("%s %d", font_desc_string, font_size);
  g_free (font_desc_string);

  return result;
}

/*
  For reference, from cdk/x11/cdksettings.c:

  "Net/DoubleClickTime\0"     "ctk-double-click-time\0"
  "Net/DoubleClickDistance\0" "ctk-double-click-distance\0"
  "Net/DndDragThreshold\0"    "ctk-dnd-drag-threshold\0"
  "Net/CursorBlink\0"         "ctk-cursor-blink\0"
  "Net/CursorBlinkTime\0"     "ctk-cursor-blink-time\0"
  "Net/ThemeName\0"           "ctk-theme-name\0"
  "Net/IconThemeName\0"       "ctk-icon-theme-name\0"
  "Ctk/ColorPalette\0"        "ctk-color-palette\0"
  "Ctk/FontName\0"            "ctk-font-name\0"
  "Ctk/KeyThemeName\0"        "ctk-key-theme-name\0"
  "Ctk/Modules\0"             "ctk-modules\0"
  "Ctk/CursorBlinkTimeout\0"  "ctk-cursor-blink-timeout\0"
  "Ctk/CursorThemeName\0"     "ctk-cursor-theme-name\0"
  "Ctk/CursorThemeSize\0"     "ctk-cursor-theme-size\0"
  "Ctk/ColorScheme\0"         "ctk-color-scheme\0"
  "Ctk/EnableAnimations\0"    "ctk-enable-animations\0"
  "Xft/Antialias\0"           "ctk-xft-antialias\0"
  "Xft/Hinting\0"             "ctk-xft-hinting\0"
  "Xft/HintStyle\0"           "ctk-xft-hintstyle\0"
  "Xft/RGBA\0"                "ctk-xft-rgba\0"
  "Xft/DPI\0"                 "ctk-xft-dpi\0"
  "Ctk/EnableAccels\0"        "ctk-enable-accels\0"
  "Ctk/ScrolledWindowPlacement\0" "ctk-scrolled-window-placement\0"
  "Ctk/IMModule\0"            "ctk-im-module\0"
  "Fontconfig/Timestamp\0"    "ctk-fontconfig-timestamp\0"
  "Net/SoundThemeName\0"      "ctk-sound-theme-name\0"
  "Net/EnableInputFeedbackSounds\0" "ctk-enable-input-feedback-sounds\0"
  "Net/EnableEventSounds\0"  "ctk-enable-event-sounds\0";

  More, from various places in ctk sources:

  ctk-entry-select-on-focus
  ctk-split-cursor

*/
gboolean
_cdk_win32_screen_get_setting (CdkScreen   *screen,
                        const gchar *name,
                        GValue      *value)
{
  g_return_val_if_fail (CDK_IS_SCREEN (screen), FALSE);

  /*
   * XXX : if these values get changed through the Windoze UI the
   *       respective cdk_events are not generated yet.
   */
  if (strcmp ("ctk-double-click-time", name) == 0)
    {
      gint i = GetDoubleClickTime ();
      CDK_NOTE(MISC, g_print("cdk_screen_get_setting(\"%s\") : %d\n", name, i));
      g_value_set_int (value, i);
      return TRUE;
    }
  else if (strcmp ("ctk-double-click-distance", name) == 0)
    {
      gint i = MAX(GetSystemMetrics (SM_CXDOUBLECLK), GetSystemMetrics (SM_CYDOUBLECLK));
      CDK_NOTE(MISC, g_print("cdk_screen_get_setting(\"%s\") : %d\n", name, i));
      g_value_set_int (value, i);
      return TRUE;
    }
  else if (strcmp ("ctk-dnd-drag-threshold", name) == 0)
    {
      gint i = MAX(GetSystemMetrics (SM_CXDRAG), GetSystemMetrics (SM_CYDRAG));
      CDK_NOTE(MISC, g_print("cdk_screen_get_setting(\"%s\") : %d\n", name, i));
      g_value_set_int (value, i);
      return TRUE;
    }
  else if (strcmp ("ctk-split-cursor", name) == 0)
    {
      CDK_NOTE(MISC, g_print("cdk_screen_get_setting(\"%s\") : FALSE\n", name));
      g_value_set_boolean (value, FALSE);
      return TRUE;
    }
  else if (strcmp ("ctk-alternative-button-order", name) == 0)
    {
      CDK_NOTE(MISC, g_print("cdk_screen_get_setting(\"%s\") : TRUE\n", name));
      g_value_set_boolean (value, TRUE);
      return TRUE;
    }
  else if (strcmp ("ctk-alternative-sort-arrows", name) == 0)
    {
      CDK_NOTE(MISC, g_print("cdk_screen_get_setting(\"%s\") : TRUE\n", name));
      g_value_set_boolean (value, TRUE);
      return TRUE;
    }
  else if (strcmp ("ctk-shell-shows-desktop", name) == 0)
    {
      CDK_NOTE(MISC, g_print("cdk_screen_get_setting(\"%s\") : TRUE\n", name));
      g_value_set_boolean (value, TRUE);
      return TRUE;
    }
  else if (strcmp ("ctk-xft-hinting", name) == 0)
    {
      CDK_NOTE(MISC, g_print ("cdk_screen_get_setting(\"%s\") : 1\n", name));
      g_value_set_int (value, 1);
      return TRUE;
    }
  else if (strcmp ("ctk-xft-antialias", name) == 0)
    {
      BOOL val = TRUE;
      SystemParametersInfoW (SPI_GETFONTSMOOTHING, 0, &val, 0);
      g_value_set_int (value, val ? 1 : 0);

      CDK_NOTE(MISC, g_print ("cdk_screen_get_setting(\"%s\") : %u\n", name, val));
      return TRUE;
    }
  else if (strcmp ("ctk-xft-hintstyle", name) == 0)
    {
      g_value_set_static_string (value, "hintfull");
      CDK_NOTE(MISC, g_print ("cdk_screen_get_setting(\"%s\") : %s\n", name, g_value_get_string (value)));
      return TRUE;
    }
  else if (strcmp ("ctk-xft-rgba", name) == 0)
    {
      const gchar *rgb_value;
      CdkMonitor *monitor;
      monitor = cdk_display_get_monitor (cdk_display_get_default (), 0);
      rgb_value = _cdk_win32_monitor_get_pixel_structure (monitor);
      g_value_set_static_string (value, rgb_value);

      CDK_NOTE(MISC, g_print ("cdk_screen_get_setting(\"%s\") : %s\n", name, g_value_get_string (value)));
      return TRUE;
    }
  else if (strcmp ("ctk-font-name", name) == 0)
    {
      gchar *font_name = _get_system_font_name (_cdk_display_hdc);

      if (font_name)
        {
          /* The pango font fallback list got fixed during 1.43, before that
           * using anything but "Segoe UI" would lead to a poor glyph coverage */
          if (pango_version_check (1, 43, 0) != NULL &&
              g_ascii_strncasecmp (font_name, "Segoe UI", strlen ("Segoe UI")) != 0)
            {
              g_free (font_name);
              return FALSE;
            }

          CDK_NOTE(MISC, g_print("cdk_screen_get_setting(\"%s\") : %s\n", name, font_name));
          g_value_take_string (value, font_name);
          return TRUE;
        }
      else
        {
          g_warning ("cdk_screen_get_setting: Detecting the system font failed");
          return FALSE;
        }
    }
  else if (strcmp ("ctk-im-module", name) == 0)
    {
      if (_cdk_input_locale_is_ime)
        g_value_set_static_string (value, "ime");
      else
        g_value_set_static_string (value, "");

      return TRUE;
    }

  return FALSE;
}
