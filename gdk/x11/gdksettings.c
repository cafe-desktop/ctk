/* GDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

static const struct {
  const char *xname, *gdkname;
} gdk_settings_map[] = {
  {"Net/DoubleClickTime",     "ctk-double-click-time"},
  {"Net/DoubleClickDistance", "ctk-double-click-distance"},
  {"Net/DndDragThreshold",    "ctk-dnd-drag-threshold"},
  {"Net/CursorBlink",         "ctk-cursor-blink"},
  {"Net/CursorBlinkTime",     "ctk-cursor-blink-time"},
  {"Net/ThemeName",           "ctk-theme-name"},
  {"Net/IconThemeName",       "ctk-icon-theme-name"},
  {"Gtk/ColorPalette",        "ctk-color-palette"},
  {"Gtk/FontName",            "ctk-font-name"},
  {"Gtk/KeyThemeName",        "ctk-key-theme-name"},
  {"Gtk/Modules",             "ctk-modules"},
  {"Gtk/ButtonImages",        "ctk-button-images"},
  {"Gtk/MenuImages",          "ctk-menu-images"},
  {"Gtk/CursorThemeName",     "ctk-cursor-theme-name"},
  {"Gtk/CursorThemeSize",     "ctk-cursor-theme-size"},
  {"Gtk/ColorScheme",         "ctk-color-scheme"},
  {"Gtk/EnableAnimations",    "ctk-enable-animations"},
  {"Xft/Antialias",           "ctk-xft-antialias"},
  {"Xft/Hinting",             "ctk-xft-hinting"},
  {"Xft/HintStyle",           "ctk-xft-hintstyle"},
  {"Xft/RGBA",                "ctk-xft-rgba"},
  {"Xft/DPI",                 "ctk-xft-dpi"},
  {"Gtk/EnableAccels",        "ctk-enable-accels"},
  {"Gtk/ScrolledWindowPlacement", "ctk-scrolled-window-placement"},
  {"Gtk/IMModule",            "ctk-im-module"},
  {"Fontconfig/Timestamp",    "ctk-fontconfig-timestamp"},
  {"Net/SoundThemeName",      "ctk-sound-theme-name"},
  {"Net/EnableInputFeedbackSounds", "ctk-enable-input-feedback-sounds"},
  {"Net/EnableEventSounds",   "ctk-enable-event-sounds"},
  {"Gtk/CursorBlinkTimeout",  "ctk-cursor-blink-timeout"},
  {"Gtk/ShellShowsAppMenu",   "ctk-shell-shows-app-menu"},
  {"Gtk/ShellShowsMenubar",   "ctk-shell-shows-menubar"},
  {"Gtk/ShellShowsDesktop",   "ctk-shell-shows-desktop"},
  {"Gtk/SessionBusId",        "ctk-session-bus-id"},
  {"Gtk/DecorationLayout",    "ctk-decoration-layout"},
  {"Gtk/TitlebarDoubleClick", "ctk-titlebar-double-click"},
  {"Gtk/TitlebarMiddleClick", "ctk-titlebar-middle-click"},
  {"Gtk/TitlebarRightClick", "ctk-titlebar-right-click"},
  {"Gtk/DialogsUseHeader",    "ctk-dialogs-use-header"},
  {"Gtk/EnablePrimaryPaste",  "ctk-enable-primary-paste"},
  {"Gtk/PrimaryButtonWarpsSlider", "ctk-primary-button-warps-slider"},
  {"Gtk/RecentFilesMaxAge",   "ctk-recent-files-max-age"},
  {"Gtk/RecentFilesEnabled",  "ctk-recent-files-enabled"},
  {"Gtk/KeynavUseCaret",      "ctk-keynav-use-caret"},
  {"Gtk/OverlayScrolling",    "ctk-overlay-scrolling"},

  /* These are here in order to be recognized, but are not sent to
     ctk as they are handled internally by gdk: */
  {"Gdk/WindowScalingFactor", "gdk-window-scaling-factor"},
  {"Gdk/UnscaledDPI",         "gdk-unscaled-dpi"}
};

static const char *
gdk_from_xsettings_name (const char *xname)
{
  static GHashTable *hash = NULL;
  guint i;

  if (G_UNLIKELY (hash == NULL))
  {
    hash = g_hash_table_new (g_str_hash, g_str_equal);

    for (i = 0; i < G_N_ELEMENTS (gdk_settings_map); i++)
      g_hash_table_insert (hash, (gpointer)gdk_settings_map[i].xname,
                                 (gpointer)gdk_settings_map[i].gdkname);
  }

  return g_hash_table_lookup (hash, xname);
}

