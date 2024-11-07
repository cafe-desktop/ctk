/* templates.c
 * Copyright (C) 2013 Openismus GmbH
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Tristan Van Berkom <tristanvb@openismus.com>
 */
#include <ctk/ctk.h>

#ifdef HAVE_UNIX_PRINT_WIDGETS
#  include <ctk/ctkunixprint.h>
#endif

static gboolean
main_loop_quit_cb (gpointer data G_GNUC_UNUSED)
{
  ctk_main_quit ();

  return FALSE;
}

static void
test_dialog_basic (void)
{
  CtkWidget *dialog;

  dialog = ctk_dialog_new();
  g_assert (CTK_IS_DIALOG (dialog));

  g_assert (ctk_dialog_get_action_area (CTK_DIALOG (dialog)) != NULL);

  g_assert (ctk_dialog_get_content_area (CTK_DIALOG (dialog)) != NULL);

  ctk_widget_destroy (dialog);
}

static void
test_dialog_override_property (void)
{
  CtkWidget *dialog;

  dialog = g_object_new (CTK_TYPE_DIALOG,
			 "type-hint", CDK_WINDOW_TYPE_HINT_UTILITY,
			 NULL);
  g_assert (CTK_IS_DIALOG (dialog));
  g_assert (ctk_window_get_type_hint (CTK_WINDOW (dialog)) == CDK_WINDOW_TYPE_HINT_UTILITY);

  ctk_widget_destroy (dialog);
}

static void
test_message_dialog_basic (void)
{
  CtkWidget *dialog;

  dialog = ctk_message_dialog_new (NULL, 0,
				   CTK_MESSAGE_INFO,
				   CTK_BUTTONS_CLOSE,
				   "Do it hard !");
  g_assert (CTK_IS_DIALOG (dialog));
  ctk_widget_destroy (dialog);
}

static void
test_about_dialog_basic (void)
{
  CtkWidget *dialog;

  dialog = ctk_about_dialog_new ();
  g_assert (CTK_IS_ABOUT_DIALOG (dialog));
  ctk_widget_destroy (dialog);
}

static void
test_info_bar_basic (void)
{
  CtkWidget *infobar;

  infobar = ctk_info_bar_new ();
  g_assert (CTK_IS_INFO_BAR (infobar));
  ctk_widget_destroy (infobar);
}

static void
test_lock_button_basic (void)
{
  CtkWidget *button;
  GPermission *permission;

  permission = g_simple_permission_new (TRUE);
  button = ctk_lock_button_new (permission);
  g_assert (CTK_IS_LOCK_BUTTON (button));
  ctk_widget_destroy (button);
  g_object_unref (permission);
}

static void
test_assistant_basic (void)
{
  CtkWidget *widget;

  widget = ctk_assistant_new ();
  g_assert (CTK_IS_ASSISTANT (widget));
  ctk_widget_destroy (widget);
}

static void
test_scale_button_basic (void)
{
  CtkWidget *widget;

  widget = ctk_scale_button_new (CTK_ICON_SIZE_MENU,
				 0, 100, 10, NULL);
  g_assert (CTK_IS_SCALE_BUTTON (widget));
  ctk_widget_destroy (widget);
}

static void
test_volume_button_basic (void)
{
  CtkWidget *widget;

  widget = ctk_volume_button_new ();
  g_assert (CTK_IS_VOLUME_BUTTON (widget));
  ctk_widget_destroy (widget);
}

static void
test_statusbar_basic (void)
{
  CtkWidget *widget;

  widget = ctk_statusbar_new ();
  g_assert (CTK_IS_STATUSBAR (widget));
  ctk_widget_destroy (widget);
}

static void
test_search_bar_basic (void)
{
  CtkWidget *widget;

  widget = ctk_search_bar_new ();
  g_assert (CTK_IS_SEARCH_BAR (widget));
  ctk_widget_destroy (widget);
}

static void
test_action_bar_basic (void)
{
  CtkWidget *widget;

  widget = ctk_action_bar_new ();
  g_assert (CTK_IS_ACTION_BAR (widget));
  ctk_widget_destroy (widget);
}

static void
test_app_chooser_widget_basic (void)
{
  CtkWidget *widget;

  widget = ctk_app_chooser_widget_new (NULL);
  g_assert (CTK_IS_APP_CHOOSER_WIDGET (widget));
  ctk_widget_destroy (widget);
}

static void
test_app_chooser_dialog_basic (void)
{
  CtkWidget *widget;

  widget = ctk_app_chooser_dialog_new_for_content_type (NULL, 0, "text/plain");
  g_assert (CTK_IS_APP_CHOOSER_DIALOG (widget));

  /* CtkAppChooserDialog bug, if destroyed before spinning 
   * the main context then app_chooser_online_get_default_ready_cb()
   * will be eventually called and segfault.
   */
  g_timeout_add (500, main_loop_quit_cb, NULL);
  ctk_main();
  ctk_widget_destroy (widget);
}

static void
test_color_chooser_dialog_basic (void)
{
  CtkWidget *widget;

  /* This test also tests the internal CtkColorEditor widget */
  widget = ctk_color_chooser_dialog_new (NULL, NULL);
  g_assert (CTK_IS_COLOR_CHOOSER_DIALOG (widget));
  ctk_widget_destroy (widget);
}

/* Avoid warnings from GVFS-RemoteVolumeMonitor */
static gboolean
ignore_gvfs_warning (const gchar    *log_domain,
		     GLogLevelFlags  log_level G_GNUC_UNUSED,
		     const gchar    *message G_GNUC_UNUSED,
		     gpointer        user_data G_GNUC_UNUSED)
{
  if (g_strcmp0 (log_domain, "GVFS-RemoteVolumeMonitor") == 0)
    return FALSE;

  return TRUE;
}

static void
test_file_chooser_widget_basic (void)
{
  CtkWidget *widget;

  /* This test also tests the internal CtkPathBar widget */
  g_test_log_set_fatal_handler (ignore_gvfs_warning, NULL);

  widget = ctk_file_chooser_widget_new (CTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
  g_assert (CTK_IS_FILE_CHOOSER_WIDGET (widget));

  /* XXX BUG:
   *
   * Spin the mainloop for a bit, this allows the file operations
   * to complete, CtkFileChooserWidget has a bug where it leaks
   * CtkTreeRowReferences to the internal shortcuts_model
   *
   * Since we assert all automated children are finalized we
   * can catch this
   */
  g_timeout_add (100, main_loop_quit_cb, NULL);
  ctk_main();

  ctk_widget_destroy (widget);
}

static void
test_file_chooser_dialog_basic (void)
{
  CtkWidget *widget;

  g_test_log_set_fatal_handler (ignore_gvfs_warning, NULL);

  widget = ctk_file_chooser_dialog_new ("The Dialog", NULL,
					CTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
					"_OK", CTK_RESPONSE_OK,
					NULL);

  g_assert (CTK_IS_FILE_CHOOSER_DIALOG (widget));
  g_timeout_add (100, main_loop_quit_cb, NULL);
  ctk_main();

  ctk_widget_destroy (widget);
}

static void
test_file_chooser_button_basic (void)
{
  CtkWidget *widget;

  g_test_log_set_fatal_handler (ignore_gvfs_warning, NULL);

  widget = ctk_file_chooser_button_new ("Choose a file !", CTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
  g_assert (CTK_IS_FILE_CHOOSER_BUTTON (widget));
  g_timeout_add (100, main_loop_quit_cb, NULL);
  ctk_main();

  ctk_widget_destroy (widget);
}

static void
test_font_button_basic (void)
{
  CtkWidget *widget;

  widget = ctk_font_button_new ();
  g_assert (CTK_IS_FONT_BUTTON (widget));
  ctk_widget_destroy (widget);
}

static void
test_font_chooser_widget_basic (void)
{
  CtkWidget *widget;

  widget = ctk_font_chooser_widget_new ();
  g_assert (CTK_IS_FONT_CHOOSER_WIDGET (widget));
  ctk_widget_destroy (widget);
}

static void
test_font_chooser_dialog_basic (void)
{
  CtkWidget *widget;

  widget = ctk_font_chooser_dialog_new ("Choose a font !", NULL);
  g_assert (CTK_IS_FONT_CHOOSER_DIALOG (widget));
  ctk_widget_destroy (widget);
}

static void
test_recent_chooser_widget_basic (void)
{
  CtkWidget *widget;

  widget = ctk_recent_chooser_widget_new ();
  g_assert (CTK_IS_RECENT_CHOOSER_WIDGET (widget));
  ctk_widget_destroy (widget);
}

#ifdef HAVE_UNIX_PRINT_WIDGETS
static void
test_page_setup_unix_dialog_basic (void)
{
  CtkWidget *widget;

  widget = ctk_page_setup_unix_dialog_new ("Setup your Page !", NULL);
  g_assert (CTK_IS_PAGE_SETUP_UNIX_DIALOG (widget));
  ctk_widget_destroy (widget);
}

static void
test_print_unix_dialog_basic (void)
{
  CtkWidget *widget;

  widget = ctk_print_unix_dialog_new ("Go Print !", NULL);
  g_assert (CTK_IS_PRINT_UNIX_DIALOG (widget));
  ctk_widget_destroy (widget);
}
#endif

int
main (int argc, char **argv)
{
  gchar *schema_dir;

  /* These must be set before before ctk_test_init */
  g_setenv ("GIO_USE_VFS", "local", TRUE);
  g_setenv ("GSETTINGS_BACKEND", "memory", TRUE);

  /* initialize test program */
  ctk_test_init (&argc, &argv);

  /* g_test_build_filename must be called after ctk_test_init */
  schema_dir = g_test_build_filename (G_TEST_BUILT, "", NULL);
  if (g_getenv ("CTK_TEST_MESON") == NULL)
    g_setenv ("GSETTINGS_SCHEMA_DIR", schema_dir, TRUE);

  /* This environment variable cooperates with ctk_widget_destroy()
   * to assert that all automated compoenents are properly finalized
   * when a given composite widget is destroyed.
   */
  g_assert (g_setenv ("CTK_WIDGET_ASSERT_COMPONENTS", "1", TRUE));

  g_test_add_func ("/Template/CtkDialog/Basic", test_dialog_basic);
  g_test_add_func ("/Template/CtkDialog/OverrideProperty", test_dialog_override_property);
  g_test_add_func ("/Template/CtkMessageDialog/Basic", test_message_dialog_basic);
  g_test_add_func ("/Template/CtkAboutDialog/Basic", test_about_dialog_basic);
  g_test_add_func ("/Template/CtkInfoBar/Basic", test_info_bar_basic);
  g_test_add_func ("/Template/CtkLockButton/Basic", test_lock_button_basic);
  g_test_add_func ("/Template/CtkAssistant/Basic", test_assistant_basic);
  g_test_add_func ("/Template/CtkScaleButton/Basic", test_scale_button_basic);
  g_test_add_func ("/Template/CtkVolumeButton/Basic", test_volume_button_basic);
  g_test_add_func ("/Template/CtkStatusBar/Basic", test_statusbar_basic);
  g_test_add_func ("/Template/CtkSearchBar/Basic", test_search_bar_basic);
  g_test_add_func ("/Template/CtkActionBar/Basic", test_action_bar_basic);
  g_test_add_func ("/Template/CtkAppChooserWidget/Basic", test_app_chooser_widget_basic);
  g_test_add_func ("/Template/CtkAppChooserDialog/Basic", test_app_chooser_dialog_basic);
  g_test_add_func ("/Template/CtkColorChooserDialog/Basic", test_color_chooser_dialog_basic);
  g_test_add_func ("/Template/CtkFileChooserWidget/Basic", test_file_chooser_widget_basic);
  g_test_add_func ("/Template/CtkFileChooserDialog/Basic", test_file_chooser_dialog_basic);
  g_test_add_func ("/Template/CtkFileChooserButton/Basic", test_file_chooser_button_basic);
  g_test_add_func ("/Template/CtkFontButton/Basic", test_font_button_basic);
  g_test_add_func ("/Template/CtkFontChooserWidget/Basic", test_font_chooser_widget_basic);
  g_test_add_func ("/Template/CtkFontChooserDialog/Basic", test_font_chooser_dialog_basic);
  g_test_add_func ("/Template/CtkRecentChooserWidget/Basic", test_recent_chooser_widget_basic);

#ifdef HAVE_UNIX_PRINT_WIDGETS
  g_test_add_func ("/Template/UnixPrint/CtkPageSetupUnixDialog/Basic", test_page_setup_unix_dialog_basic);
  g_test_add_func ("/Template/UnixPrint/CtkPrintUnixDialog/Basic", test_print_unix_dialog_basic);
#endif

  g_free (schema_dir);

  return g_test_run();
}
