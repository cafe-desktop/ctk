/* testfilechooser.c
 * Copyright (C) 2003  Red Hat, Inc.
 * Author: Owen Taylor
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
 */
#include "config.h"

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <time.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <ctk/ctk.h>

#ifdef G_OS_WIN32
#  include <io.h>
#  define localtime_r(t,b) *(b) = *localtime (t)
#  ifndef S_ISREG
#    define S_ISREG(m) ((m) & _S_IFREG)
#  endif
#endif

#if 0
static CtkWidget *preview_label;
static CtkWidget *preview_image;
#endif
static CtkFileChooserAction action;

static void
print_current_folder (CtkFileChooser *chooser)
{
  gchar *uri;

  uri = ctk_file_chooser_get_current_folder_uri (chooser);
  g_print ("Current folder changed :\n  %s\n", uri ? uri : "(null)");
  g_free (uri);
}

static void
print_selected (CtkFileChooser *chooser)
{
  GSList *uris = ctk_file_chooser_get_uris (chooser);
  GSList *tmp_list;

  g_print ("Selection changed :\n");
  for (tmp_list = uris; tmp_list; tmp_list = tmp_list->next)
    {
      gchar *uri = tmp_list->data;
      g_print ("  %s\n", uri);
      g_free (uri);
    }
  g_print ("\n");
  g_slist_free (uris);
}

static void
response_cb (CtkDialog *dialog,
	     gint       response_id)
{
  if (response_id == CTK_RESPONSE_OK)
    {
      GSList *list;

      list = ctk_file_chooser_get_uris (CTK_FILE_CHOOSER (dialog));

      if (list)
	{
	  GSList *l;

	  g_print ("Selected files:\n");

	  for (l = list; l; l = l->next)
	    {
	      g_print ("%s\n", (char *) l->data);
	      g_free (l->data);
	    }

	  g_slist_free (list);
	}
      else
	g_print ("No selected files\n");
    }
  else
    g_print ("Dialog was closed\n");

  ctk_main_quit ();
}

static gboolean
no_backup_files_filter (const CtkFileFilterInfo *filter_info,
			gpointer                 data)
{
  gsize len = filter_info->display_name ? strlen (filter_info->display_name) : 0;
  if (len > 0 && filter_info->display_name[len - 1] == '~')
    return 0;
  else
    return 1;
}

static void
filter_changed (CtkFileChooserDialog *dialog,
		gpointer              data)
{
  g_print ("file filter changed\n");
}

#include <stdio.h>
#include <errno.h>
#define _(s) (s)

static void
size_prepared_cb (GdkPixbufLoader *loader,
		  int              width,
		  int              height,
		  int             *data)
{
	int des_width = data[0];
	int des_height = data[1];

	if (des_height >= height && des_width >= width) {
		/* Nothing */
	} else if ((double)height * des_width > (double)width * des_height) {
		width = 0.5 + (double)width * des_height / (double)height;
		height = des_height;
	} else {
		height = 0.5 + (double)height * des_width / (double)width;
		width = des_width;
	}

	gdk_pixbuf_loader_set_size (loader, width, height);
}

GdkPixbuf *
my_new_from_file_at_size (const char *filename,
			  int         width,
			  int         height,
			  GError    **error)
{
	GdkPixbufLoader *loader;
	GdkPixbuf       *pixbuf;
	int              info[2];
	struct stat st;

	guchar buffer [4096];
	int length;
	FILE *f;

	g_return_val_if_fail (filename != NULL, NULL);
        g_return_val_if_fail (width > 0 && height > 0, NULL);

	if (stat (filename, &st) != 0) {
                int errsv = errno;

		g_set_error (error,
			     G_FILE_ERROR,
			     g_file_error_from_errno (errsv),
			     _("Could not get information for file '%s': %s"),
			     filename, g_strerror (errsv));
		return NULL;
	}

	if (!S_ISREG (st.st_mode))
		return NULL;

	f = fopen (filename, "rb");
	if (!f) {
                int errsv = errno;

                g_set_error (error,
                             G_FILE_ERROR,
                             g_file_error_from_errno (errsv),
                             _("Failed to open file '%s': %s"),
                             filename, g_strerror (errsv));
		return NULL;
        }

	loader = gdk_pixbuf_loader_new ();
#ifdef DONT_PRESERVE_ASPECT
	gdk_pixbuf_loader_set_size (loader, width, height);
#else
	info[0] = width;
	info[1] = height;
	g_signal_connect (loader, "size-prepared", G_CALLBACK (size_prepared_cb), info);
#endif

	while (!feof (f)) {
		length = fread (buffer, 1, sizeof (buffer), f);
		if (length > 0)
			if (!gdk_pixbuf_loader_write (loader, buffer, length, error)) {
			        gdk_pixbuf_loader_close (loader, NULL);
				fclose (f);
				g_object_unref (loader);
				return NULL;
			}
	}

	fclose (f);

	g_assert (*error == NULL);
	if (!gdk_pixbuf_loader_close (loader, error)) {
		g_object_unref (loader);
		return NULL;
	}

	pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);

	if (!pixbuf) {
		g_object_unref (loader);

		/* did the loader set an error? */
		if (*error != NULL)
			return NULL;

		g_set_error (error,
                             CDK_PIXBUF_ERROR,
                             CDK_PIXBUF_ERROR_FAILED,
                             _("Failed to load image '%s': reason not known, probably a corrupt image file"),
                             filename);
		return NULL;
	}

	g_object_ref (pixbuf);

	g_object_unref (loader);

	return pixbuf;
}

#if 0
static char *
format_time (time_t t)
{
  gchar buf[128];
  struct tm tm_buf;
  time_t now = time (NULL);
  const char *format;

  if (abs (now - t) < 24*60*60)
    format = "%X";
  else
    format = "%x";

  localtime_r (&t, &tm_buf);
  if (strftime (buf, sizeof (buf), format, &tm_buf) == 0)
    return g_strdup ("<unknown>");
  else
    return g_strdup (buf);
}

static char *
format_size (gint64 size)
{
  if (size < (gint64)1024)
    return g_strdup_printf ("%d bytes", (gint)size);
  else if (size < (gint64)1024*1024)
    return g_strdup_printf ("%.1f K", size / (1024.));
  else if (size < (gint64)1024*1024*1024)
    return g_strdup_printf ("%.1f M", size / (1024.*1024.));
  else
    return g_strdup_printf ("%.1f G", size / (1024.*1024.*1024.));
}

static void
update_preview_cb (CtkFileChooser *chooser)
{
  gchar *filename = ctk_file_chooser_get_preview_filename (chooser);
  gboolean have_preview = FALSE;

  if (filename)
    {
      GdkPixbuf *pixbuf;
      GError *error = NULL;

      pixbuf = my_new_from_file_at_size (filename, 128, 128, &error);
      if (pixbuf)
	{
	  ctk_image_set_from_pixbuf (CTK_IMAGE (preview_image), pixbuf);
	  g_object_unref (pixbuf);
	  ctk_widget_show (preview_image);
	  ctk_widget_hide (preview_label);
	  have_preview = TRUE;
	}
      else
	{
	  struct stat buf;
	  if (stat (filename, &buf) == 0)
	    {
	      gchar *preview_text;
	      gchar *size_str;
	      gchar *modified_time;

	      size_str = format_size (buf.st_size);
	      modified_time = format_time (buf.st_mtime);

	      preview_text = g_strdup_printf ("<i>Modified:</i>\t%s\n"
					      "<i>Size:</i>\t%s\n",
					      modified_time,
					      size_str);
	      ctk_label_set_markup (CTK_LABEL (preview_label), preview_text);
	      g_free (modified_time);
	      g_free (size_str);
	      g_free (preview_text);

	      ctk_widget_hide (preview_image);
	      ctk_widget_show (preview_label);
	      have_preview = TRUE;
	    }
	}

      g_free (filename);

      if (error)
	g_error_free (error);
    }

  ctk_file_chooser_set_preview_widget_active (chooser, have_preview);
}
#endif

static void
set_current_folder (CtkFileChooser *chooser,
		    const char     *name)
{
  if (!ctk_file_chooser_set_current_folder (chooser, name))
    {
      CtkWidget *dialog;

      dialog = ctk_message_dialog_new (CTK_WINDOW (chooser),
				       CTK_DIALOG_MODAL | CTK_DIALOG_DESTROY_WITH_PARENT,
				       CTK_MESSAGE_ERROR,
				       CTK_BUTTONS_CLOSE,
				       "Could not set the folder to %s",
				       name);
      ctk_dialog_run (CTK_DIALOG (dialog));
      ctk_widget_destroy (dialog);
    }
}

static void
set_folder_nonexistent_cb (CtkButton      *button,
			   CtkFileChooser *chooser)
{
  set_current_folder (chooser, "/nonexistent");
}

static void
set_folder_existing_nonexistent_cb (CtkButton      *button,
				    CtkFileChooser *chooser)
{
  set_current_folder (chooser, "/usr/nonexistent");
}

static void
set_filename (CtkFileChooser *chooser,
	      const char     *name)
{
  if (!ctk_file_chooser_set_filename (chooser, name))
    {
      CtkWidget *dialog;

      dialog = ctk_message_dialog_new (CTK_WINDOW (chooser),
				       CTK_DIALOG_MODAL | CTK_DIALOG_DESTROY_WITH_PARENT,
				       CTK_MESSAGE_ERROR,
				       CTK_BUTTONS_CLOSE,
				       "Could not select %s",
				       name);
      ctk_dialog_run (CTK_DIALOG (dialog));
      ctk_widget_destroy (dialog);
    }
}

static void
set_filename_nonexistent_cb (CtkButton      *button,
			     CtkFileChooser *chooser)
{
  set_filename (chooser, "/nonexistent");
}

static void
set_filename_existing_nonexistent_cb (CtkButton      *button,
				      CtkFileChooser *chooser)
{
  set_filename (chooser, "/usr/nonexistent");
}

static void
get_selection_cb (CtkButton      *button,
		  CtkFileChooser *chooser)
{
  GSList *selection;

  selection = ctk_file_chooser_get_uris (chooser);

  g_print ("Selection: ");

  if (selection == NULL)
    g_print ("empty\n");
  else
    {
      GSList *l;
      
      for (l = selection; l; l = l->next)
	{
	  char *uri = l->data;

	  g_print ("%s\n", uri);

	  if (l->next)
	    g_print ("           ");
	}
    }

  g_slist_free_full (selection, g_free);
}

static void
get_current_name_cb (CtkButton      *button,
		     CtkFileChooser *chooser)
{
  char *name;

  name = ctk_file_chooser_get_current_name (chooser);
  g_print ("Current name: %s\n", name ? name : "NULL");
  g_free (name);
}

static void
unmap_and_remap_cb (CtkButton *button,
		    CtkFileChooser *chooser)
{
  ctk_widget_hide (CTK_WIDGET (chooser));
  ctk_widget_show (CTK_WIDGET (chooser));
}

static void
kill_dependent (CtkWindow *win, CtkWidget *dep)
{
  ctk_widget_destroy (dep);
  g_object_unref (dep);
}

static void
notify_multiple_cb (CtkWidget  *dialog,
		    GParamSpec *pspec,
		    CtkWidget  *button)
{
  gboolean multiple;

  multiple = ctk_file_chooser_get_select_multiple (CTK_FILE_CHOOSER (dialog));

  ctk_widget_set_sensitive (button, multiple);
}

static CtkFileChooserConfirmation
confirm_overwrite_cb (CtkFileChooser *chooser,
		      gpointer        data)
{
  CtkWidget *dialog;
  CtkWidget *button;
  int response;
  CtkFileChooserConfirmation conf;

  dialog = ctk_message_dialog_new (CTK_WINDOW (ctk_widget_get_toplevel (CTK_WIDGET (chooser))),
				   CTK_DIALOG_MODAL | CTK_DIALOG_DESTROY_WITH_PARENT,
				   CTK_MESSAGE_QUESTION,
				   CTK_BUTTONS_NONE,
				   "What do you want to do?");

  button = ctk_button_new_with_label ("Use the stock confirmation dialog");
  ctk_widget_show (button);
  ctk_dialog_add_action_widget (CTK_DIALOG (dialog), button, 1);

  button = ctk_button_new_with_label ("Type a new file name");
  ctk_widget_show (button);
  ctk_dialog_add_action_widget (CTK_DIALOG (dialog), button, 2);

  button = ctk_button_new_with_label ("Accept the file name");
  ctk_widget_show (button);
  ctk_dialog_add_action_widget (CTK_DIALOG (dialog), button, 3);

  response = ctk_dialog_run (CTK_DIALOG (dialog));

  switch (response)
    {
    case 1:
      conf = CTK_FILE_CHOOSER_CONFIRMATION_CONFIRM;
      break;

    case 3:
      conf = CTK_FILE_CHOOSER_CONFIRMATION_ACCEPT_FILENAME;
      break;

    default:
      conf = CTK_FILE_CHOOSER_CONFIRMATION_SELECT_AGAIN;
      break;
    }

  ctk_widget_destroy (dialog);

  return conf;
}

int
main (int argc, char **argv)
{
  CtkWidget *control_window;
  CtkWidget *vbbox;
  CtkWidget *button;
  CtkWidget *dialog;
  CtkWidget *extra;
  CtkFileFilter *filter;
  gboolean force_rtl = FALSE;
  gboolean multiple = FALSE;
  gboolean local_only = FALSE;
  char *action_arg = NULL;
  char *initial_filename = NULL;
  char *initial_folder = NULL;
  GError *error = NULL;
  GOptionEntry options[] = {
    { "action", 'a', 0, G_OPTION_ARG_STRING, &action_arg, "Filechooser action", "ACTION" },
    { "multiple", 'm', 0, G_OPTION_ARG_NONE, &multiple, "Select multiple", NULL },
    { "local-only", 'l', 0, G_OPTION_ARG_NONE, &local_only, "Local only", NULL },
    { "right-to-left", 'r', 0, G_OPTION_ARG_NONE, &force_rtl, "Force right-to-left layout.", NULL },
    { "initial-filename", 'f', 0, G_OPTION_ARG_FILENAME, &initial_filename, "Initial filename to select", "FILENAME" },
    { "initial-folder", 'F', 0, G_OPTION_ARG_FILENAME, &initial_folder, "Initial folder to show", "FILENAME" },
    { NULL }
  };

  if (!ctk_init_with_args (&argc, &argv, "", options, NULL, &error))
    {
      g_print ("Failed to parse args: %s\n", error->message);
      g_error_free (error);
      return 1;
    }

  if (initial_filename && initial_folder)
    {
      g_print ("Only one of --initial-filename and --initial-folder may be specified");
      return 1;
    }

  if (force_rtl)
    ctk_widget_set_default_direction (CTK_TEXT_DIR_RTL);

  action = CTK_FILE_CHOOSER_ACTION_OPEN;

  if (action_arg != NULL)
    {
      if (! strcmp ("open", action_arg))
	action = CTK_FILE_CHOOSER_ACTION_OPEN;
      else if (! strcmp ("save", action_arg))
	action = CTK_FILE_CHOOSER_ACTION_SAVE;
      else if (! strcmp ("select_folder", action_arg))
	action = CTK_FILE_CHOOSER_ACTION_SELECT_FOLDER;
      else if (! strcmp ("create_folder", action_arg))
	action = CTK_FILE_CHOOSER_ACTION_CREATE_FOLDER;
      else
	{
	  g_print ("--action must be one of \"open\", \"save\", \"select_folder\", \"create_folder\"\n");
	  return 1;
	}

      g_free (action_arg);
    }

  dialog = g_object_new (CTK_TYPE_FILE_CHOOSER_DIALOG,
			 "action", action,
			 "select-multiple", multiple,
                         "local-only", local_only,
			 NULL);

  switch (action)
    {
    case CTK_FILE_CHOOSER_ACTION_OPEN:
    case CTK_FILE_CHOOSER_ACTION_SELECT_FOLDER:
      ctk_window_set_title (CTK_WINDOW (dialog), "Select a file");
      ctk_dialog_add_buttons (CTK_DIALOG (dialog),
			      _("_Cancel"), CTK_RESPONSE_CANCEL,
			      _("_Open"), CTK_RESPONSE_OK,
			      NULL);
      break;
    case CTK_FILE_CHOOSER_ACTION_SAVE:
    case CTK_FILE_CHOOSER_ACTION_CREATE_FOLDER:
      ctk_window_set_title (CTK_WINDOW (dialog), "Save a file");
      ctk_dialog_add_buttons (CTK_DIALOG (dialog),
			      _("_Cancel"), CTK_RESPONSE_CANCEL,
			      _("_Save"), CTK_RESPONSE_OK,
			      NULL);
      break;
    }
  ctk_dialog_set_default_response (CTK_DIALOG (dialog), CTK_RESPONSE_OK);

  g_signal_connect (dialog, "selection-changed",
		    G_CALLBACK (print_selected), NULL);
  g_signal_connect (dialog, "current-folder-changed",
		    G_CALLBACK (print_current_folder), NULL);
  g_signal_connect (dialog, "response",
		    G_CALLBACK (response_cb), NULL);
  g_signal_connect (dialog, "confirm-overwrite",
		    G_CALLBACK (confirm_overwrite_cb), NULL);

  /* Filters */
  filter = ctk_file_filter_new ();
  ctk_file_filter_set_name (filter, "All Files");
  ctk_file_filter_add_pattern (filter, "*");
  ctk_file_chooser_add_filter (CTK_FILE_CHOOSER (dialog), filter);

  /* Make this filter the default */
  ctk_file_chooser_set_filter (CTK_FILE_CHOOSER (dialog), filter);

  filter = ctk_file_filter_new ();
  ctk_file_filter_set_name (filter, "No backup files");
  ctk_file_filter_add_custom (filter, CTK_FILE_FILTER_DISPLAY_NAME,
			      no_backup_files_filter, NULL, NULL);
  ctk_file_filter_add_mime_type (filter, "image/png");
  ctk_file_chooser_add_filter (CTK_FILE_CHOOSER (dialog), filter);

  filter = ctk_file_filter_new ();
  ctk_file_filter_set_name (filter, "Starts with D");
  ctk_file_filter_add_pattern (filter, "D*");
  ctk_file_chooser_add_filter (CTK_FILE_CHOOSER (dialog), filter);

  g_signal_connect (dialog, "notify::filter",
		    G_CALLBACK (filter_changed), NULL);

  filter = ctk_file_filter_new ();
  ctk_file_filter_set_name (filter, "PNG and JPEG");
  ctk_file_filter_add_mime_type (filter, "image/jpeg");
  ctk_file_filter_add_mime_type (filter, "image/png");
  ctk_file_chooser_add_filter (CTK_FILE_CHOOSER (dialog), filter);

  filter = ctk_file_filter_new ();
  ctk_file_filter_set_name (filter, "Images");
  ctk_file_filter_add_pixbuf_formats (filter);
  ctk_file_chooser_add_filter (CTK_FILE_CHOOSER (dialog), filter);

#if 0
  /* Preview widget */
  /* THIS IS A TERRIBLE PREVIEW WIDGET, AND SHOULD NOT BE COPIED AT ALL.
   */
  preview_vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  ctk_file_chooser_set_preview_widget (CTK_FILE_CHOOSER (dialog), preview_vbox);

  preview_label = ctk_label_new (NULL);
  ctk_box_pack_start (CTK_BOX (preview_vbox), preview_label, TRUE, TRUE, 0);
  g_object_set (preview_label, "margin", 6, NULL);

  preview_image = ctk_image_new ();
  ctk_box_pack_start (CTK_BOX (preview_vbox), preview_image, TRUE, TRUE, 0);
  g_object_set (preview_image, "margin", 6, NULL);

  update_preview_cb (CTK_FILE_CHOOSER (dialog));
  g_signal_connect (dialog, "update-preview",
		    G_CALLBACK (update_preview_cb), NULL);
#endif

  /* Extra widget */

  extra = ctk_check_button_new_with_mnemonic ("Lar_t whoever asks about this button");
  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (extra), TRUE);
  ctk_file_chooser_set_extra_widget (CTK_FILE_CHOOSER (dialog), extra);

  /* Shortcuts */

  ctk_file_chooser_add_shortcut_folder_uri (CTK_FILE_CHOOSER (dialog),
					    "file:///usr/share/pixmaps",
					    NULL);
  ctk_file_chooser_add_shortcut_folder (CTK_FILE_CHOOSER (dialog),
					g_get_user_special_dir (G_USER_DIRECTORY_MUSIC),
					NULL);

  /* Initial filename or folder */

  if (initial_filename)
    set_filename (CTK_FILE_CHOOSER (dialog), initial_filename);

  if (initial_folder)
    set_current_folder (CTK_FILE_CHOOSER (dialog), initial_folder);

  /* show_all() to reveal bugs in composite widget handling */
  ctk_widget_show_all (dialog);

  /* Extra controls for manipulating the test environment
   */

  control_window = ctk_window_new (CTK_WINDOW_TOPLEVEL);

  vbbox = ctk_button_box_new (CTK_ORIENTATION_VERTICAL);
  ctk_container_add (CTK_CONTAINER (control_window), vbbox);

  button = ctk_button_new_with_mnemonic ("_Select all");
  ctk_widget_set_sensitive (button, multiple);
  ctk_container_add (CTK_CONTAINER (vbbox), button);
  g_signal_connect_swapped (button, "clicked",
			    G_CALLBACK (ctk_file_chooser_select_all), dialog);
  g_signal_connect (dialog, "notify::select-multiple",
		    G_CALLBACK (notify_multiple_cb), button);

  button = ctk_button_new_with_mnemonic ("_Unselect all");
  ctk_container_add (CTK_CONTAINER (vbbox), button);
  g_signal_connect_swapped (button, "clicked",
			    G_CALLBACK (ctk_file_chooser_unselect_all), dialog);

  button = ctk_button_new_with_label ("set_current_folder (\"/nonexistent\")");
  ctk_container_add (CTK_CONTAINER (vbbox), button);
  g_signal_connect (button, "clicked",
		    G_CALLBACK (set_folder_nonexistent_cb), dialog);

  button = ctk_button_new_with_label ("set_current_folder (\"/usr/nonexistent\")");
  ctk_container_add (CTK_CONTAINER (vbbox), button);
  g_signal_connect (button, "clicked",
		    G_CALLBACK (set_folder_existing_nonexistent_cb), dialog);

  button = ctk_button_new_with_label ("set_filename (\"/nonexistent\")");
  ctk_container_add (CTK_CONTAINER (vbbox), button);
  g_signal_connect (button, "clicked",
		    G_CALLBACK (set_filename_nonexistent_cb), dialog);

  button = ctk_button_new_with_label ("set_filename (\"/usr/nonexistent\")");
  ctk_container_add (CTK_CONTAINER (vbbox), button);
  g_signal_connect (button, "clicked",
		    G_CALLBACK (set_filename_existing_nonexistent_cb), dialog);

  button = ctk_button_new_with_label ("Get selection");
  ctk_container_add (CTK_CONTAINER (vbbox), button);
  g_signal_connect (button, "clicked",
		    G_CALLBACK (get_selection_cb), dialog);

  button = ctk_button_new_with_label ("Get current name");
  ctk_container_add (CTK_CONTAINER (vbbox), button);
  g_signal_connect (button, "clicked",
		    G_CALLBACK (get_current_name_cb), dialog);

  button = ctk_button_new_with_label ("Unmap and remap");
  ctk_container_add (CTK_CONTAINER (vbbox), button);
  g_signal_connect (button, "clicked",
		    G_CALLBACK (unmap_and_remap_cb), dialog);

  ctk_widget_show_all (control_window);

  g_object_ref (control_window);
  g_signal_connect (dialog, "destroy",
		    G_CALLBACK (kill_dependent), control_window);

  /* We need to hold a ref until we have destroyed the widgets, just in case
   * someone else destroys them.  We explicitly destroy windows to catch leaks.
   */
  g_object_ref (dialog);
  ctk_main ();
  ctk_widget_destroy (dialog);
  g_object_unref (dialog);

  return 0;
}
