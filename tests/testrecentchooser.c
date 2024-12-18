/* testrecentchooser.c
 * Copyright (C) 2006  Emmanuele Bassi.
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
# include <io.h>
# define localtime_r(t,b) *(b) = localtime (t)
# ifndef S_ISREG
#  define S_ISREG(m) ((m) & _S_IFREG)
# endif
#endif

static void
print_current_item (CtkRecentChooser *chooser)
{
  gchar *uri;

  uri = ctk_recent_chooser_get_current_uri (chooser);
  g_print ("Current item changed :\n  %s\n", uri ? uri : "null");
  g_free (uri);
}

static void
print_selected (CtkRecentChooser *chooser)
{
  gsize uris_len, i;
  gchar **uris = ctk_recent_chooser_get_uris (chooser, &uris_len);

  g_print ("Selection changed :\n");
  for (i = 0; i < uris_len; i++)
    g_print ("  %s\n", uris[i]);
  g_print ("\n");

  g_strfreev (uris);
}

static void
response_cb (CtkDialog *dialog G_GNUC_UNUSED,
	     gint       response_id)
{
  if (response_id == CTK_RESPONSE_OK)
    {
    }
  else
    g_print ("Dialog was closed\n");

  ctk_main_quit ();
}

static void
filter_changed (CtkRecentChooserDialog *dialog G_GNUC_UNUSED,
		gpointer                data G_GNUC_UNUSED)
{
  g_print ("recent filter changed\n");
}

static void
notify_multiple_cb (CtkWidget  *dialog,
		    GParamSpec *pspec G_GNUC_UNUSED,
		    CtkWidget  *button)
{
  gboolean multiple;

  multiple = ctk_recent_chooser_get_select_multiple (CTK_RECENT_CHOOSER (dialog));

  ctk_widget_set_sensitive (button, multiple);
}

static void
kill_dependent (CtkWindow *win G_GNUC_UNUSED,
		CtkWidget *dep)
{
  ctk_widget_destroy (dep);
  g_object_unref (dep);
}

int
main (int   argc,
      char *argv[])
{
  CtkWidget *control_window;
  CtkWidget *vbbox;
  CtkWidget *button;
  CtkWidget *dialog;
  CtkRecentFilter *filter;
  gint i;
  gboolean multiple = FALSE;
  
  ctk_init (&argc, &argv);

  /* to test rtl layout, set RTL=1 in the environment */
  if (g_getenv ("RTL"))
    ctk_widget_set_default_direction (CTK_TEXT_DIR_RTL);

  for (i = 1; i < argc; i++)
    {
      if (!strcmp ("--multiple", argv[i]))
	multiple = TRUE;
    }

  dialog = g_object_new (CTK_TYPE_RECENT_CHOOSER_DIALOG,
		         "select-multiple", multiple,
                         "show-tips", TRUE,
                         "show-icons", TRUE,
			 NULL);
  ctk_window_set_title (CTK_WINDOW (dialog), "Select a file");
  ctk_dialog_add_buttons (CTK_DIALOG (dialog),
		  	  "_Cancel", CTK_RESPONSE_CANCEL,
			  "_Open", CTK_RESPONSE_OK,
			  NULL);
  
  ctk_dialog_set_default_response (CTK_DIALOG (dialog), CTK_RESPONSE_OK);

  g_signal_connect (dialog, "item-activated",
		    G_CALLBACK (print_current_item), NULL);
  g_signal_connect (dialog, "selection-changed",
		    G_CALLBACK (print_selected), NULL);
  g_signal_connect (dialog, "response",
		    G_CALLBACK (response_cb), NULL);
  
  /* filters */
  filter = ctk_recent_filter_new ();
  ctk_recent_filter_set_name (filter, "All Files");
  ctk_recent_filter_add_pattern (filter, "*");
  ctk_recent_chooser_add_filter (CTK_RECENT_CHOOSER (dialog), filter);

  filter = ctk_recent_filter_new ();
  ctk_recent_filter_set_name (filter, "Only PDF Files");
  ctk_recent_filter_add_mime_type (filter, "application/pdf");
  ctk_recent_chooser_add_filter (CTK_RECENT_CHOOSER (dialog), filter);

  g_signal_connect (dialog, "notify::filter",
		    G_CALLBACK (filter_changed), NULL);

  ctk_recent_chooser_set_filter (CTK_RECENT_CHOOSER (dialog), filter);

  filter = ctk_recent_filter_new ();
  ctk_recent_filter_set_name (filter, "PNG and JPEG");
  ctk_recent_filter_add_mime_type (filter, "image/png");
  ctk_recent_filter_add_mime_type (filter, "image/jpeg");
  ctk_recent_chooser_add_filter (CTK_RECENT_CHOOSER (dialog), filter);

  ctk_widget_show_all (dialog);

  control_window = ctk_window_new (CTK_WINDOW_TOPLEVEL);

  vbbox = ctk_button_box_new (CTK_ORIENTATION_VERTICAL);
  ctk_container_add (CTK_CONTAINER (control_window), vbbox);

  button = ctk_button_new_with_mnemonic ("_Select all");
  ctk_widget_set_sensitive (button, multiple);
  ctk_container_add (CTK_CONTAINER (vbbox), button);
  g_signal_connect_swapped (button, "clicked",
		            G_CALLBACK (ctk_recent_chooser_select_all), dialog);
  g_signal_connect (dialog, "notify::select-multiple",
		    G_CALLBACK (notify_multiple_cb), button);

  button = ctk_button_new_with_mnemonic ("_Unselect all");
  ctk_container_add (CTK_CONTAINER (vbbox), button);
  g_signal_connect_swapped (button, "clicked",
		            G_CALLBACK (ctk_recent_chooser_unselect_all), dialog);

  ctk_widget_show_all (control_window);
  
  g_object_ref (control_window);
  g_signal_connect (dialog, "destroy",
		    G_CALLBACK (kill_dependent), control_window);
  
  g_object_ref (dialog);
  ctk_main ();
  ctk_widget_destroy (dialog);
  g_object_unref (dialog);

  return 0;
}
