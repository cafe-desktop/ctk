/* testmultidisplay.c
 * Copyright (C) 2001 Sun Microsystems Inc.
 * Author: Erwann Chenede <erwann.chenede@sun.com>
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
#include <gtk/gtk.h>

gchar *screen2_name = NULL;

typedef struct
{
  GtkEntry *e1;
  GtkEntry *e2;
}
MyDoubleGtkEntry;

void
get_screen_response (GtkDialog *dialog,
		     gint       response_id,
		     GtkEntry  *entry)
{
  if (response_id == CTK_RESPONSE_DELETE_EVENT)
    return;
  g_free (screen2_name);
  screen2_name = g_strdup (ctk_entry_get_text (entry));
}

void
entry_dialog_response (GtkDialog        *dialog,
		       gint              response_id,
		       MyDoubleGtkEntry *de)
{
  if (response_id == CTK_RESPONSE_APPLY)
    ctk_entry_set_text (de->e2, ctk_entry_get_text (de->e1));
  else
    ctk_main_quit ();
}

void
make_selection_dialog (GdkScreen * screen,
		       GtkWidget * entry,
		       GtkWidget * other_entry)
{
  GtkWidget *window, *vbox;
  GtkWidget *content_area;
  MyDoubleGtkEntry *double_entry = g_new (MyDoubleGtkEntry, 1);
  double_entry->e1 = CTK_ENTRY (entry);
  double_entry->e2 = CTK_ENTRY (other_entry);

  if (!screen)
    screen = gdk_screen_get_default ();

  window = g_object_new (CTK_TYPE_DIALOG,
			   "screen", screen,
			   "user_data", NULL,
			   "type", CTK_WINDOW_TOPLEVEL,
			   "title", "MultiDisplay Cut & Paste",
			   "border_width", 10, NULL);
  g_signal_connect (window, "destroy",
		    G_CALLBACK (ctk_main_quit), NULL);

  content_area = ctk_dialog_get_content_area (CTK_DIALOG (window));

  vbox = g_object_new (CTK_TYPE_BOX,
			 "border_width", 5,
                         "orientation", CTK_ORIENTATION_VERTICAL,
			 NULL);
  ctk_box_pack_start (CTK_BOX (content_area), vbox, FALSE, FALSE, 0);

  ctk_box_pack_start (CTK_BOX (vbox), entry, FALSE, FALSE, 0);
  ctk_widget_grab_focus (entry);

  ctk_dialog_add_buttons (CTK_DIALOG (window),
			  "_Apply", CTK_RESPONSE_APPLY,
			  "_Quit", CTK_RESPONSE_DELETE_EVENT,
			  NULL);
  ctk_dialog_set_default_response (CTK_DIALOG (window), CTK_RESPONSE_APPLY);

  g_signal_connect (window, "response",
		    G_CALLBACK (entry_dialog_response), double_entry);

  ctk_widget_show_all (window);
}

int
main (int argc, char *argv[])
{
  GtkWidget *dialog, *display_entry, *dialog_label;
  GtkWidget *entry, *entry2;
  GtkWidget *content_area;
  GdkDisplay *dpy2;
  GdkScreen *scr2 = NULL;	/* Quiet GCC */
  gboolean correct_second_display = FALSE;

  ctk_init (&argc, &argv);

  if (argc == 2)
    screen2_name = g_strdup (argv[1]);
  
  /* Get the second display information */

  dialog = ctk_dialog_new_with_buttons ("Second Display Selection",
					NULL,
					CTK_DIALOG_MODAL,
					"_OK",
					CTK_RESPONSE_OK, NULL);

  ctk_dialog_set_default_response (CTK_DIALOG (dialog), CTK_RESPONSE_OK);
  display_entry = ctk_entry_new ();
  ctk_entry_set_activates_default (CTK_ENTRY (display_entry), TRUE);
  dialog_label =
    ctk_label_new ("Please enter the name of\nthe second display\n");

  content_area = ctk_dialog_get_content_area (CTK_DIALOG (dialog));

  ctk_container_add (CTK_CONTAINER (content_area), dialog_label);
  ctk_container_add (CTK_CONTAINER (content_area), display_entry);
  g_signal_connect (dialog, "response",
		    G_CALLBACK (get_screen_response), display_entry);

  ctk_widget_grab_focus (display_entry);
  ctk_widget_show_all (ctk_bin_get_child (CTK_BIN (dialog)));
  
  while (!correct_second_display)
    {
      if (screen2_name)
	{
	  if (!g_ascii_strcasecmp (screen2_name, ""))
	    g_printerr ("No display name, reverting to default display\n");
	  
	  dpy2 = gdk_display_open (screen2_name);
	  if (dpy2)
	    {
	      scr2 = gdk_display_get_default_screen (dpy2);
	      correct_second_display = TRUE;
	    }
	  else
	    {
	      char *error_msg =
		g_strdup_printf  ("Can't open display :\n\t%s\nplease try another one\n",
				  screen2_name);
	      ctk_label_set_text (CTK_LABEL (dialog_label), error_msg);
	      g_free (error_msg);
	    }
       }

      if (!correct_second_display)
	ctk_dialog_run (CTK_DIALOG (dialog));
    }
  
  ctk_widget_destroy (dialog);

  entry = g_object_new (CTK_TYPE_ENTRY,
			  "activates_default", TRUE,
			  "visible", TRUE,
			  NULL);
  entry2 = g_object_new (CTK_TYPE_ENTRY,
			   "activates_default", TRUE,
			   "visible", TRUE,
			   NULL);

  /* for default display */
  make_selection_dialog (NULL, entry2, entry);
  /* for selected display */
  make_selection_dialog (scr2, entry, entry2);
  ctk_main ();

  return 0;
}
