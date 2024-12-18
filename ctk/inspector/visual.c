/*
 * Copyright (c) 2014 Red Hat, Inc.
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

#include "config.h"
#include <glib/gi18n-lib.h>

#include "visual.h"

#include "ctkadjustment.h"
#include "ctkbox.h"
#include "ctkcomboboxtext.h"
#include "ctkdebug.h"
#include "ctkprivate.h"
#include "ctksettings.h"
#include "ctkswitch.h"
#include "ctkscale.h"
#include "ctkwindow.h"
#include "ctkcssproviderprivate.h"
#include "ctkversion.h"

#include "fallback-c89.c"

#ifdef CDK_WINDOWING_X11
#include "x11/cdkx.h"
#endif
#ifdef CDK_WINDOWING_WAYLAND
#include "wayland/cdkwayland.h"
#endif

#include "cdk/cdk-private.h"

#define EPSILON               1e-10

struct _CtkInspectorVisualPrivate
{
  CtkWidget *visual_box;
  CtkWidget *theme_combo;
  CtkWidget *dark_switch;
  CtkWidget *icon_combo;
  CtkWidget *cursor_combo;
  CtkWidget *cursor_size_spin;
  CtkWidget *direction_combo;
  CtkWidget *font_button;
  CtkWidget *hidpi_spin;
  CtkWidget *animation_switch;
  CtkWidget *font_scale_entry;
  CtkAdjustment *font_scale_adjustment;
  CtkAdjustment *scale_adjustment;
  CtkAdjustment *slowdown_adjustment;
  CtkWidget *slowdown_entry;
  CtkAdjustment *cursor_size_adjustment;

  CtkWidget *debug_box;
  CtkWidget *rendering_mode_combo;
  CtkWidget *updates_switch;
  CtkWidget *baselines_switch;
  CtkWidget *layout_switch;
  CtkWidget *touchscreen_switch;

  CtkWidget *gl_box;
  CtkWidget *gl_combo;
  CtkWidget *software_gl_switch;
  CtkWidget *software_surface_switch;
  CtkWidget *texture_rectangle_switch;

  CtkAdjustment *focus_adjustment;
};

G_DEFINE_TYPE_WITH_PRIVATE (CtkInspectorVisual, ctk_inspector_visual, CTK_TYPE_SCROLLED_WINDOW)

static void
fix_direction_recurse (CtkWidget *widget,
                       gpointer   data)
{
  CtkTextDirection dir = GPOINTER_TO_INT (data);

  g_object_ref (widget);

  ctk_widget_set_direction (widget, dir);
  if (CTK_IS_CONTAINER (widget))
    ctk_container_forall (CTK_CONTAINER (widget), fix_direction_recurse, data);

  g_object_unref (widget);
}

static CtkTextDirection initial_direction;

static void
fix_direction (CtkWidget *iw)
{
  fix_direction_recurse (iw, GINT_TO_POINTER (initial_direction));
}

static void
direction_changed (CtkComboBox *combo)
{
  CtkWidget *iw;
  const gchar *direction;

  iw = ctk_widget_get_toplevel (CTK_WIDGET (combo));
  fix_direction (iw);

  direction = ctk_combo_box_get_active_id (combo);
  if (g_strcmp0 (direction, "ltr") == 0)
    ctk_widget_set_default_direction (CTK_TEXT_DIR_LTR);
  else
    ctk_widget_set_default_direction (CTK_TEXT_DIR_RTL);
}

static void
init_direction (CtkInspectorVisual *vis)
{
  const gchar *direction;

  initial_direction = ctk_widget_get_default_direction ();
  if (initial_direction == CTK_TEXT_DIR_LTR)
    direction = "ltr";
  else
    direction = "rtl";
  ctk_combo_box_set_active_id (CTK_COMBO_BOX (vis->priv->direction_combo), direction);
}

static void
redraw_everything (void)
{
  GList *toplevels;
  toplevels = ctk_window_list_toplevels ();
  g_list_foreach (toplevels, (GFunc) ctk_widget_queue_draw, NULL);
  g_list_free (toplevels);
}

static double
get_font_scale (CtkInspectorVisual *vis G_GNUC_UNUSED)
{
#ifdef CDK_WINDOWING_X11
  if (CDK_IS_X11_DISPLAY (cdk_display_get_default ()))
    {
      int dpi_int;

      g_object_get (ctk_settings_get_default (),
                    "ctk-xft-dpi", &dpi_int,
                    NULL);

      return dpi_int / (96.0 * 1024.0);
    }
#endif
#ifdef CDK_WINDOWING_WAYLAND
  if (CDK_IS_WAYLAND_DISPLAY (cdk_display_get_default ()))
    {
      int dpi_int;

      g_object_get (ctk_settings_get_default (),
                    "ctk-xft-dpi", &dpi_int,
                    NULL);

      return dpi_int / (96.0 * 1024.0);
    }
#endif

  return 1.0;
}

static void
update_font_scale (CtkInspectorVisual *vis,
                   gdouble             factor,
                   gboolean            update_adjustment,
                   gboolean            update_entry)
{
  g_object_set (ctk_settings_get_default (),
                "ctk-xft-dpi", (gint)(factor * 96 * 1024),
                NULL);

  if (update_adjustment)
    ctk_adjustment_set_value (vis->priv->font_scale_adjustment, factor);

  if (update_entry)
    {
      gchar *str = g_strdup_printf ("%0.2f", factor);

      ctk_entry_set_text (CTK_ENTRY (vis->priv->font_scale_entry), str);
      g_free (str);
    }
}

static void
font_scale_adjustment_changed (CtkAdjustment      *adjustment,
                               CtkInspectorVisual *vis)
{
  gdouble factor;

  factor = ctk_adjustment_get_value (adjustment);
  update_font_scale (vis, factor, FALSE, TRUE);
}

static void
font_scale_entry_activated (CtkEntry           *entry,
                            CtkInspectorVisual *vis)
{
  gdouble factor;
  gchar *err = NULL;

  factor = g_strtod (ctk_entry_get_text (entry), &err);
  if (err != NULL)
    update_font_scale (vis, factor, TRUE, FALSE);
}

static void
updates_activate (CtkSwitch *sw)
{
  gboolean updates;

  updates = ctk_switch_get_active (sw);
  CDK_PRIVATE_CALL (cdk_display_set_debug_updates) (cdk_display_get_default (), updates);
  redraw_everything ();
}

static void
init_updates (CtkInspectorVisual *vis)
{
  gboolean updates;

  updates = CDK_PRIVATE_CALL (cdk_display_get_debug_updates) (cdk_display_get_default ());
  ctk_switch_set_active (CTK_SWITCH (vis->priv->updates_switch), updates);
}

static void
baselines_activate (CtkSwitch *sw)
{
  guint flags;

  flags = ctk_get_debug_flags ();

  if (ctk_switch_get_active (sw))
    flags |= CTK_DEBUG_BASELINES;
  else
    flags &= ~CTK_DEBUG_BASELINES;

  ctk_set_debug_flags (flags);
  redraw_everything ();
}

static void
layout_activate (CtkSwitch *sw)
{
  guint flags;

  flags = ctk_get_debug_flags ();

  if (ctk_switch_get_active (sw))
    flags |= CTK_DEBUG_LAYOUT;
  else
    flags &= ~CTK_DEBUG_LAYOUT;

  ctk_set_debug_flags (flags);
  redraw_everything ();
}

static void
pixelcache_activate (CtkSwitch *sw)
{
  guint flags;

  flags = ctk_get_debug_flags ();

  if (ctk_switch_get_active (sw))
    flags |= CTK_DEBUG_PIXEL_CACHE;
  else
    flags &= ~CTK_DEBUG_PIXEL_CACHE;

  ctk_set_debug_flags (flags);
  /* FIXME: this doesn't work, because it is redrawing
   * _from_ the cache. We need to recurse over the tree
   * and invalidate the pixel cache of every widget that
   * has one.
   */
  redraw_everything ();
}

static void
widget_resize_activate (CtkSwitch *sw)
{
  guint flags = ctk_get_debug_flags ();

  if (ctk_switch_get_active (sw))
    flags |= CTK_DEBUG_RESIZE;
  else
    flags &= ~CTK_DEBUG_RESIZE;

  ctk_set_debug_flags (flags);
}

static void
fill_ctk (const gchar *path,
          GHashTable  *t)
{
  const gchar *dir_entry;
  GDir *dir = g_dir_open (path, 0, NULL);

  if (!dir)
    return;

#if (CTK_MINOR_VERSION % 2)
#define MINOR (CTK_MINOR_VERSION + 1)
#else
#define MINOR CTK_MINOR_VERSION
#endif

  /* Keep this in sync with _ctk_css_find_theme_dir() in ctkcssprovider.c */
  while ((dir_entry = g_dir_read_name (dir)))
    {
      gint i;
      gboolean found = FALSE;

      for (i = MINOR; !found && i >= 0; i = i - 2)
        {
          gchar *filename, *subsubdir;

          if (i < 14)
            i = 0;

          subsubdir = g_strdup_printf ("ctk-3.%d", i);
          filename = g_build_filename (path, dir_entry, subsubdir, "ctk.css", NULL);
          g_free (subsubdir);

          if (g_file_test (filename, G_FILE_TEST_IS_REGULAR) &&
              !g_hash_table_contains (t, dir_entry))
            {
              found = TRUE;
              g_hash_table_add (t, g_strdup (dir_entry));
            }

          g_free (filename);
        }
    }

#undef MINOR

  g_dir_close (dir);
}

static gchar*
get_data_path (const gchar *subdir)
{
  gchar *base_datadir, *full_datadir;
#if defined (CDK_WINDOWING_WIN32) || defined (CDK_WINDOWING_QUARTZ)
  base_datadir = g_strdup (_ctk_get_datadir ());
#else
  base_datadir = g_strdup (CTK_DATADIR);
#endif
  full_datadir = g_build_filename (base_datadir, subdir, NULL);
  g_free (base_datadir);
  return full_datadir;
}

static void
init_theme (CtkInspectorVisual *vis)
{
  GHashTable *t;
  GHashTableIter iter;
  gchar *theme, *path;
  gchar **builtin_themes;
  GList *list, *l;
  guint i;
  const gchar * const *dirs;

  t = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
  /* Builtin themes */
  builtin_themes = g_resources_enumerate_children ("/org/ctk/libctk/theme", 0, NULL);
  for (i = 0; builtin_themes[i] != NULL; i++)
    {
      if (g_str_has_suffix (builtin_themes[i], "/"))
        g_hash_table_add (t, g_strndup (builtin_themes[i], strlen (builtin_themes[i]) - 1));
    }
  g_strfreev (builtin_themes);

  path = _ctk_get_theme_dir ();
  fill_ctk (path, t);
  g_free (path);

  path = g_build_filename (g_get_user_data_dir (), "themes", NULL);
  fill_ctk (path, t);
  g_free (path);

  path = g_build_filename (g_get_home_dir (), ".themes", NULL);
  fill_ctk (path, t);
  g_free (path);

  dirs = g_get_system_data_dirs ();
  for (i = 0; dirs[i]; i++)
    {
      path = g_build_filename (dirs[i], "themes", NULL);
      fill_ctk (path, t);
      g_free (path);
    }

  list = NULL;
  g_hash_table_iter_init (&iter, t);
  while (g_hash_table_iter_next (&iter, (gpointer *)&theme, NULL))
    list = g_list_insert_sorted (list, theme, (GCompareFunc)strcmp);

  for (l = list; l; l = l->next)
    {
      theme = l->data;
      ctk_combo_box_text_append (CTK_COMBO_BOX_TEXT (vis->priv->theme_combo), theme, theme);
    }

  g_list_free (list);
  g_hash_table_destroy (t);

  g_object_bind_property (ctk_settings_get_default (), "ctk-theme-name",
                          vis->priv->theme_combo, "active-id",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

  if (g_getenv ("CTK_THEME") != NULL)
    {
      /* theme is hardcoded, nothing we can do */
      ctk_widget_set_sensitive (vis->priv->theme_combo, FALSE);
      ctk_widget_set_tooltip_text (vis->priv->theme_combo , _("Theme is hardcoded by CTK_THEME"));
    }
}

static void
init_dark (CtkInspectorVisual *vis)
{
  g_object_bind_property (ctk_settings_get_default (), "ctk-application-prefer-dark-theme",
                          vis->priv->dark_switch, "active",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

  if (g_getenv ("CTK_THEME") != NULL)
    {
      /* theme is hardcoded, nothing we can do */
      ctk_widget_set_sensitive (vis->priv->dark_switch, FALSE);
      ctk_widget_set_tooltip_text (vis->priv->dark_switch, _("Theme is hardcoded by CTK_THEME"));
    }
}

static void
fill_icons (const gchar *path,
            GHashTable  *t)
{
  const gchar *dir_entry;
  GDir *dir;

  dir = g_dir_open (path, 0, NULL);
  if (!dir)
    return;

  while ((dir_entry = g_dir_read_name (dir)))
    {
      gchar *filename = g_build_filename (path, dir_entry, "index.theme", NULL);

      if (g_file_test (filename, G_FILE_TEST_IS_REGULAR) &&
          g_strcmp0 (dir_entry, "hicolor") != 0 &&
          !g_hash_table_contains (t, dir_entry))
        g_hash_table_add (t, g_strdup (dir_entry));

      g_free (filename);
    }

  g_dir_close (dir);
}

static void
init_icons (CtkInspectorVisual *vis)
{
  GHashTable *t;
  GHashTableIter iter;
  gchar *theme, *path;
  GList *list, *l;

  t = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

  path = get_data_path ("icons");
  fill_icons (path, t);
  g_free (path);

  path = g_build_filename (g_get_user_data_dir (), "icons", NULL);
  fill_icons (path, t);
  g_free (path);

  list = NULL;
  g_hash_table_iter_init (&iter, t);
  while (g_hash_table_iter_next (&iter, (gpointer *)&theme, NULL))
    list = g_list_insert_sorted (list, theme, (GCompareFunc)strcmp);

  for (l = list; l; l = l->next)
    {
      theme = l->data;
      ctk_combo_box_text_append (CTK_COMBO_BOX_TEXT (vis->priv->icon_combo), theme, theme);
    }

  g_hash_table_destroy (t);
  g_list_free (list);

  g_object_bind_property (ctk_settings_get_default (), "ctk-icon-theme-name",
                          vis->priv->icon_combo, "active-id",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
}

static void
fill_cursors (const gchar *path,
              GHashTable  *t)
{
  const gchar *dir_entry;
  GDir *dir;

  dir = g_dir_open (path, 0, NULL);
  if (!dir)
    return;

  while ((dir_entry = g_dir_read_name (dir)))
    {
      gchar *filename = g_build_filename (path, dir_entry, "cursors", NULL);

      if (g_file_test (filename, G_FILE_TEST_IS_DIR) &&
          !g_hash_table_contains (t, dir_entry))
        g_hash_table_add (t, g_strdup (dir_entry));

      g_free (filename);
    }

  g_dir_close (dir);
}

static void
init_cursors (CtkInspectorVisual *vis)
{
  GHashTable *t;
  GHashTableIter iter;
  gchar *theme, *path;
  GList *list, *l;

  t = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

  path = get_data_path ("icons");
  fill_cursors (path, t);
  g_free (path);

  path = g_build_filename (g_get_user_data_dir (), "icons", NULL);
  fill_cursors (path, t);
  g_free (path);

  list = NULL;
  g_hash_table_iter_init (&iter, t);
  while (g_hash_table_iter_next (&iter, (gpointer *)&theme, NULL))
    list = g_list_insert_sorted (list, theme, (GCompareFunc)strcmp);

  for (l = list; l; l = l->next)
    {
      theme = l->data;
      ctk_combo_box_text_append (CTK_COMBO_BOX_TEXT (vis->priv->cursor_combo), theme, theme);
    }

  g_hash_table_destroy (t);
  g_list_free (list);

  g_object_bind_property (ctk_settings_get_default (), "ctk-cursor-theme-name",
                          vis->priv->cursor_combo, "active-id",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
}

static void
cursor_size_changed (CtkAdjustment      *adjustment,
                     CtkInspectorVisual *vis G_GNUC_UNUSED)
{
  gint size;

  size = ctk_adjustment_get_value (adjustment);
  g_object_set (ctk_settings_get_default (), "ctk-cursor-theme-size", size, NULL);
}

static void
init_cursor_size (CtkInspectorVisual *vis)
{
  gint size;

  g_object_get (ctk_settings_get_default (), "ctk-cursor-theme-size", &size, NULL);
  if (size == 0)
    size = cdk_display_get_default_cursor_size (cdk_display_get_default ());

  ctk_adjustment_set_value (vis->priv->scale_adjustment, (gdouble)size);
  g_signal_connect (vis->priv->cursor_size_adjustment, "value-changed",
                    G_CALLBACK (cursor_size_changed), vis);
}

static void
init_font (CtkInspectorVisual *vis)
{
  g_object_bind_property (ctk_settings_get_default (), "ctk-font-name",
                          vis->priv->font_button, "font-name",
                          G_BINDING_BIDIRECTIONAL|G_BINDING_SYNC_CREATE);
}

static void
init_font_scale (CtkInspectorVisual *vis)
{
  double scale;

  scale = get_font_scale (vis);
  update_font_scale (vis, scale, TRUE, TRUE);
  g_signal_connect (vis->priv->font_scale_adjustment, "value-changed",
                    G_CALLBACK (font_scale_adjustment_changed), vis);
  g_signal_connect (vis->priv->font_scale_entry, "activate",
                    G_CALLBACK (font_scale_entry_activated), vis);
}

#if defined (CDK_WINDOWING_X11)
static void
scale_changed (CtkAdjustment      *adjustment,
               CtkInspectorVisual *vis G_GNUC_UNUSED)
{
  CdkDisplay *display;
  gint scale;

  scale = ctk_adjustment_get_value (adjustment);
  display = cdk_display_get_default ();
  cdk_x11_display_set_window_scale (display, scale);
}
#endif

static void
init_scale (CtkInspectorVisual *vis)
{
#if defined (CDK_WINDOWING_X11)
  CdkScreen *screen;

  screen = cdk_screen_get_default ();
  if (CDK_IS_X11_SCREEN (screen))
    {
      gdouble scale;

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      scale = cdk_screen_get_monitor_scale_factor (screen, 0);
G_GNUC_END_IGNORE_DEPRECATIONS
      ctk_adjustment_set_value (vis->priv->scale_adjustment, scale);
      g_signal_connect (vis->priv->scale_adjustment, "value-changed",
                        G_CALLBACK (scale_changed), vis);
    }
  else
#endif
    {
      ctk_adjustment_set_value (vis->priv->scale_adjustment, 1);
      ctk_widget_set_sensitive (vis->priv->hidpi_spin, FALSE);
      ctk_widget_set_tooltip_text (vis->priv->hidpi_spin,
                                   _("Backend does not support window scaling"));
    }
}

static void
init_animation (CtkInspectorVisual *vis)
{
  g_object_bind_property (ctk_settings_get_default (), "ctk-enable-animations",
                          vis->priv->animation_switch, "active",
                          G_BINDING_BIDIRECTIONAL|G_BINDING_SYNC_CREATE);
}

static void
update_slowdown (CtkInspectorVisual *vis,
                 gdouble slowdown,
                 gboolean update_adjustment,
                 gboolean update_entry)
{
  _ctk_set_slowdown (slowdown);

  if (update_adjustment)
    ctk_adjustment_set_value (vis->priv->slowdown_adjustment,
                              log2 (slowdown));

  if (update_entry)
    {
      gchar *str = g_strdup_printf ("%0.*f", 2, slowdown);

      ctk_entry_set_text (CTK_ENTRY (vis->priv->slowdown_entry), str);
      g_free (str);
    }
}

static void
slowdown_adjustment_changed (CtkAdjustment *adjustment,
                             CtkInspectorVisual *vis)
{
  gdouble value = ctk_adjustment_get_value (adjustment);
  gdouble previous = CLAMP (log2 (_ctk_get_slowdown ()),
                            ctk_adjustment_get_lower (adjustment),
                            ctk_adjustment_get_upper (adjustment));

  if (fabs (value - previous) > EPSILON)
    update_slowdown (vis, exp2 (value), FALSE, TRUE);
}

static void
slowdown_entry_activated (CtkEntry *entry,
                          CtkInspectorVisual *vis)
{
  gdouble slowdown;
  gchar *err = NULL;

  slowdown = g_strtod (ctk_entry_get_text (entry), &err);
  if (err != NULL)
    update_slowdown (vis, slowdown, TRUE, FALSE);
}

static void
init_slowdown (CtkInspectorVisual *vis)
{
  update_slowdown (vis, _ctk_get_slowdown (), TRUE, TRUE);
  g_signal_connect (vis->priv->slowdown_adjustment, "value-changed",
                    G_CALLBACK (slowdown_adjustment_changed), vis);
  g_signal_connect (vis->priv->slowdown_entry, "activate",
                    G_CALLBACK (slowdown_entry_activated), vis);
}

static void
update_touchscreen (CtkSwitch *sw)
{
  CtkDebugFlag flags;

  flags = ctk_get_debug_flags ();

  if (ctk_switch_get_active (sw))
    flags |= CTK_DEBUG_TOUCHSCREEN;
  else
    flags &= ~CTK_DEBUG_TOUCHSCREEN;

  ctk_set_debug_flags (flags);
}

static void
init_touchscreen (CtkInspectorVisual *vis)
{
  ctk_switch_set_active (CTK_SWITCH (vis->priv->touchscreen_switch), (ctk_get_debug_flags () & CTK_DEBUG_TOUCHSCREEN) != 0);
  g_signal_connect (vis->priv->touchscreen_switch, "notify::active",
                    G_CALLBACK (update_touchscreen), NULL);

  if (g_getenv ("CTK_TEST_TOUCHSCREEN") != 0)
    {
      /* hardcoded, nothing we can do */
      ctk_switch_set_active (CTK_SWITCH (vis->priv->touchscreen_switch), TRUE);
      ctk_widget_set_sensitive (vis->priv->touchscreen_switch, FALSE);
      ctk_widget_set_tooltip_text (vis->priv->touchscreen_switch, _("Setting is hardcoded by CTK_TEST_TOUCHSCREEN"));
    }
}

static gboolean
keynav_failed (CtkWidget *widget, CtkDirectionType direction, CtkInspectorVisual *vis)
{
  CtkWidget *next;
  gdouble value, lower, upper, page;

  if (direction == CTK_DIR_DOWN &&
      widget == vis->priv->visual_box)
    next = vis->priv->debug_box;
  else if (direction == CTK_DIR_DOWN &&
      widget == vis->priv->debug_box)
    next = vis->priv->gl_box;
  else if (direction == CTK_DIR_UP &&
           widget == vis->priv->debug_box)
    next = vis->priv->visual_box;
  else if (direction == CTK_DIR_UP &&
           widget == vis->priv->gl_box)
    next = vis->priv->debug_box;
  else
    next = NULL;

  if (next)
    {
      ctk_widget_child_focus (next, direction);
      return TRUE;
    }

  value = ctk_adjustment_get_value (vis->priv->focus_adjustment);
  lower = ctk_adjustment_get_lower (vis->priv->focus_adjustment);
  upper = ctk_adjustment_get_upper (vis->priv->focus_adjustment);
  page  = ctk_adjustment_get_page_size (vis->priv->focus_adjustment);

  if (direction == CTK_DIR_UP && value > lower)
    {
      ctk_adjustment_set_value (vis->priv->focus_adjustment, lower);
      return TRUE;
    }
  else if (direction == CTK_DIR_DOWN && value < upper - page)
    {
      ctk_adjustment_set_value (vis->priv->focus_adjustment, upper - page);
      return TRUE;
    }

  return FALSE;
}

static void
init_gl (CtkInspectorVisual *vis)
{
  CdkGLFlags flags;

  flags = CDK_PRIVATE_CALL (cdk_gl_get_flags) ();

  if (flags & CDK_GL_ALWAYS)
    ctk_combo_box_set_active_id (CTK_COMBO_BOX (vis->priv->gl_combo), "always");
  else if (flags & CDK_GL_DISABLE)
    ctk_combo_box_set_active_id (CTK_COMBO_BOX (vis->priv->gl_combo), "disable");
  else
    ctk_combo_box_set_active_id (CTK_COMBO_BOX (vis->priv->gl_combo), "maybe");
  ctk_widget_set_sensitive (vis->priv->gl_combo, FALSE);
  ctk_widget_set_tooltip_text (vis->priv->gl_combo,
                               _("Not settable at runtime.\nUse CDK_GL=always or CDK_GL=disable instead"));

  ctk_switch_set_active (CTK_SWITCH (vis->priv->software_gl_switch),
                         flags & CDK_GL_SOFTWARE_DRAW_GL);
  ctk_switch_set_active (CTK_SWITCH (vis->priv->software_surface_switch),
                         flags & CDK_GL_SOFTWARE_DRAW_SURFACE);
  ctk_switch_set_active (CTK_SWITCH (vis->priv->texture_rectangle_switch),
                         flags & CDK_GL_TEXTURE_RECTANGLE);

  if (flags & CDK_GL_DISABLE)
    {
      ctk_widget_set_sensitive (vis->priv->software_gl_switch, FALSE);
      ctk_widget_set_sensitive (vis->priv->software_surface_switch, FALSE);
      ctk_widget_set_sensitive (vis->priv->texture_rectangle_switch, FALSE);
      ctk_widget_set_tooltip_text (vis->priv->software_gl_switch, _("GL rendering is disabled"));
      ctk_widget_set_tooltip_text (vis->priv->software_surface_switch, _("GL rendering is disabled"));
      ctk_widget_set_tooltip_text (vis->priv->texture_rectangle_switch, _("GL rendering is disabled"));
    }
}

static void
init_rendering_mode (CtkInspectorVisual *vis)
{
  CdkRenderingMode mode;

  mode = CDK_PRIVATE_CALL (cdk_display_get_rendering_mode) (cdk_display_get_default ());
  ctk_combo_box_set_active (CTK_COMBO_BOX (vis->priv->rendering_mode_combo), mode);
}

static void
rendering_mode_changed (CtkComboBox        *c,
                        CtkInspectorVisual *vis G_GNUC_UNUSED)
{
  CdkRenderingMode mode;

  mode = ctk_combo_box_get_active (c);
  CDK_PRIVATE_CALL (cdk_display_set_rendering_mode) (cdk_display_get_default (), mode);
}

static void
update_gl_flag (CtkSwitch  *sw,
                CdkGLFlags  flag)
{
  CdkGLFlags flags;

  flags = CDK_PRIVATE_CALL (cdk_gl_get_flags) ();

  if (ctk_switch_get_active (sw))
    flags |= flag;
  else
    flags &= ~flag;

  CDK_PRIVATE_CALL (cdk_gl_set_flags) (flags);
}

static void
software_gl_activate (CtkSwitch *sw)
{
  update_gl_flag (sw, CDK_GL_SOFTWARE_DRAW_GL);
}

static void
software_surface_activate (CtkSwitch *sw)
{
  update_gl_flag (sw, CDK_GL_SOFTWARE_DRAW_SURFACE);
}

static void
texture_rectangle_activate (CtkSwitch *sw)
{
  update_gl_flag (sw, CDK_GL_TEXTURE_RECTANGLE);
}

static void
ctk_inspector_visual_init (CtkInspectorVisual *vis)
{
  vis->priv = ctk_inspector_visual_get_instance_private (vis);
  ctk_widget_init_template (CTK_WIDGET (vis));
  init_direction (vis);
  init_theme (vis);
  init_dark (vis);
  init_icons (vis);
  init_cursors (vis);
  init_cursor_size (vis);
  init_font (vis);
  init_font_scale (vis);
  init_scale (vis);
  init_rendering_mode (vis);
  init_updates (vis);
  init_animation (vis);
  init_slowdown (vis);
  init_touchscreen (vis);
  init_gl (vis);
}

static void
ctk_inspector_visual_constructed (GObject *object)
{
  CtkInspectorVisual *vis = CTK_INSPECTOR_VISUAL (object);

  G_OBJECT_CLASS (ctk_inspector_visual_parent_class)->constructed (object);

  vis->priv->focus_adjustment = ctk_scrolled_window_get_vadjustment (CTK_SCROLLED_WINDOW (vis));
  ctk_container_set_focus_vadjustment (CTK_CONTAINER (ctk_bin_get_child (CTK_BIN (vis))),
                                       vis->priv->focus_adjustment);

   g_signal_connect (vis->priv->visual_box, "keynav-failed", G_CALLBACK (keynav_failed), vis);
   g_signal_connect (vis->priv->debug_box, "keynav-failed", G_CALLBACK (keynav_failed), vis);
   g_signal_connect (vis->priv->gl_box, "keynav-failed", G_CALLBACK (keynav_failed), vis);
}

static void
ctk_inspector_visual_class_init (CtkInspectorVisualClass *klass)
{
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = ctk_inspector_visual_constructed;

  ctk_widget_class_set_template_from_resource (widget_class, "/org/ctk/libctk/inspector/visual.ui");
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorVisual, rendering_mode_combo);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorVisual, updates_switch);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorVisual, direction_combo);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorVisual, baselines_switch);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorVisual, layout_switch);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorVisual, theme_combo);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorVisual, dark_switch);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorVisual, cursor_combo);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorVisual, cursor_size_spin);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorVisual, cursor_size_adjustment);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorVisual, icon_combo);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorVisual, hidpi_spin);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorVisual, scale_adjustment);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorVisual, animation_switch);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorVisual, slowdown_adjustment);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorVisual, slowdown_entry);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorVisual, touchscreen_switch);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorVisual, visual_box);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorVisual, debug_box);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorVisual, font_button);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorVisual, gl_box);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorVisual, gl_combo);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorVisual, software_gl_switch);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorVisual, software_surface_switch);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorVisual, texture_rectangle_switch);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorVisual, font_scale_entry);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorVisual, font_scale_adjustment);

  ctk_widget_class_bind_template_callback (widget_class, updates_activate);
  ctk_widget_class_bind_template_callback (widget_class, direction_changed);
  ctk_widget_class_bind_template_callback (widget_class, rendering_mode_changed);
  ctk_widget_class_bind_template_callback (widget_class, baselines_activate);
  ctk_widget_class_bind_template_callback (widget_class, layout_activate);
  ctk_widget_class_bind_template_callback (widget_class, pixelcache_activate);
  ctk_widget_class_bind_template_callback (widget_class, widget_resize_activate);
  ctk_widget_class_bind_template_callback (widget_class, software_gl_activate);
  ctk_widget_class_bind_template_callback (widget_class, software_surface_activate);
  ctk_widget_class_bind_template_callback (widget_class, texture_rectangle_activate);
}
