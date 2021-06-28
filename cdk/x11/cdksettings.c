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
 * Modified by the CTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

static const struct {
  const char *xname, *cdkname;
} cdk_settings_map[] = {
  {"Net/DoubleClickTime",     "ctk-double-click-time"},
  {"Net/DoubleClickDistance", "ctk-double-click-distance"},
  {"Net/DndDragThreshold",    "ctk-dnd-drag-threshold"},
  {"Net/CursorBlink",         "ctk-cursor-blink"},
  {"Net/CursorBlinkTime",     "ctk-cursor-blink-time"},
  {"Net/ThemeName",           "ctk-theme-name"},
  {"Net/IconThemeName",       "ctk-icon-theme-name"},
  {"Ctk/ColorPalette",        "ctk-color-palette"},
  {"Ctk/FontName",            "ctk-font-name"},
  {"Ctk/KeyThemeName",        "ctk-key-theme-name"},
  {"Ctk/Modules",             "ctk-modules"},
  {"Ctk/ButtonImages",        "ctk-button-images"},
  {"Ctk/MenuImages",          "ctk-menu-images"},
  {"Ctk/CursorThemeName",     "ctk-cursor-theme-name"},
  {"Ctk/CursorThemeSize",     "ctk-cursor-theme-size"},
  {"Ctk/ColorScheme",         "ctk-color-scheme"},
  {"Ctk/EnableAnimations",    "ctk-enable-animations"},
  {"Xft/Antialias",           "ctk-xft-antialias"},
  {"Xft/Hinting",             "ctk-xft-hinting"},
  {"Xft/HintStyle",           "ctk-xft-hintstyle"},
  {"Xft/RGBA",                "ctk-xft-rgba"},
  {"Xft/DPI",                 "ctk-xft-dpi"},
  {"Ctk/EnableAccels",        "ctk-enable-accels"},
  {"Ctk/ScrolledWindowPlacement", "ctk-scrolled-window-placement"},
  {"Ctk/IMModule",            "ctk-im-module"},
  {"Fontconfig/Timestamp",    "ctk-fontconfig-timestamp"},
  {"Net/SoundThemeName",      "ctk-sound-theme-name"},
  {"Net/EnableInputFeedbackSounds", "ctk-enable-input-feedback-sounds"},
  {"Net/EnableEventSounds",   "ctk-enable-event-sounds"},
  {"Ctk/CursorBlinkTimeout",  "ctk-cursor-blink-timeout"},
  {"Ctk/ShellShowsAppMenu",   "ctk-shell-shows-app-menu"},
  {"Ctk/ShellShowsMenubar",   "ctk-shell-shows-menubar"},
  {"Ctk/ShellShowsDesktop",   "ctk-shell-shows-desktop"},
  {"Ctk/SessionBusId",        "ctk-session-bus-id"},
  {"Ctk/DecorationLayout",    "ctk-decoration-layout"},
  {"Ctk/TitlebarDoubleClick", "ctk-titlebar-double-click"},
  {"Ctk/TitlebarMiddleClick", "ctk-titlebar-middle-click"},
  {"Ctk/TitlebarRightClick", "ctk-titlebar-right-click"},
  {"Ctk/DialogsUseHeader",    "ctk-dialogs-use-header"},
  {"Ctk/EnablePrimaryPaste",  "ctk-enable-primary-paste"},
  {"Ctk/PrimaryButtonWarpsSlider", "ctk-primary-button-warps-slider"},
  {"Ctk/RecentFilesMaxAge",   "ctk-recent-files-max-age"},
  {"Ctk/RecentFilesEnabled",  "ctk-recent-files-enabled"},
  {"Ctk/KeynavUseCaret",      "ctk-keynav-use-caret"},
  {"Ctk/OverlayScrolling",    "ctk-overlay-scrolling"},

  /* These are here in order to be recognized, but are not sent to
     ctk as they are handled internally by cdk: */
  {"Cdk/WindowScalingFactor", "cdk-window-scaling-factor"},
  {"Cdk/UnscaledDPI",         "cdk-unscaled-dpi"}
};

static const char *
cdk_from_xsettings_name (const char *xname)
{
  static GHashTable *hash = NULL;
  guint i;

  if (G_UNLIKELY (hash == NULL))
  {
    hash = g_hash_table_new (g_str_hash, g_str_equal);

    for (i = 0; i < G_N_ELEMENTS (cdk_settings_map); i++)
      g_hash_table_insert (hash, (gpointer)cdk_settings_map[i].xname,
                                 (gpointer)cdk_settings_map[i].cdkname);
  }

  return g_hash_table_lookup (hash, xname);
}

