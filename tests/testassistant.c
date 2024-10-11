/* 
 * CTK - The GIMP Toolkit
 * Copyright (C) 1999  Red Hat, Inc.
 * Copyright (C) 2002  Anders Carlsson <andersca@gnu.org>
 * Copyright (C) 2003  Matthias Clasen <mclasen@redhat.com>
 * Copyright (C) 2005  Carlos Garnacho Parro <carlosg@gnome.org>
 *
 * All rights reserved.
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

#include <ctk/ctk.h>

static CtkWidget*
get_test_page (const gchar *text)
{
  return ctk_label_new (text);
}

typedef struct {
  CtkAssistant *assistant;
  CtkWidget    *page;
} PageData;

static void
complete_cb (CtkWidget *check, 
	     gpointer   data)
{
  PageData *pdata = data;
  gboolean complete;

  complete = ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (check));

  ctk_assistant_set_page_complete (pdata->assistant,
				   pdata->page,
				   complete);
}
	     
static CtkWidget *
add_completion_test_page (CtkWidget   *assistant,
			  const gchar *text, 
			  gboolean     visible,
			  gboolean     complete)
{
  CtkWidget *page;
  CtkWidget *check;
  PageData *pdata;

  page = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  check = ctk_check_button_new_with_label ("Complete");

  ctk_container_add (CTK_CONTAINER (page), ctk_label_new (text));
  ctk_container_add (CTK_CONTAINER (page), check);
  
  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (check), complete);

  pdata = g_new (PageData, 1);
  pdata->assistant = CTK_ASSISTANT (assistant);
  pdata->page = page;
  g_signal_connect (G_OBJECT (check), "toggled", 
		    G_CALLBACK (complete_cb), pdata);


  if (visible)
    ctk_widget_show_all (page);

  ctk_assistant_append_page (CTK_ASSISTANT (assistant), page);
  ctk_assistant_set_page_title (CTK_ASSISTANT (assistant), page, text);
  ctk_assistant_set_page_complete (CTK_ASSISTANT (assistant), page, complete);

  return page;
}

static void
cancel_callback (CtkWidget *widget)
{
  g_print ("cancel\n");

  ctk_widget_hide (widget);
}

static void
close_callback (CtkWidget *widget)
{
  g_print ("close\n");

  ctk_widget_hide (widget);
}

static void
apply_callback (CtkWidget *widget G_GNUC_UNUSED)
{
  g_print ("apply\n");
}

static gboolean
progress_timeout (CtkWidget *assistant)
{
  CtkWidget *progress;
  gint current_page;
  gdouble value;

  current_page = ctk_assistant_get_current_page (CTK_ASSISTANT (assistant));
  progress = ctk_assistant_get_nth_page (CTK_ASSISTANT (assistant), current_page);

  value  = ctk_progress_bar_get_fraction (CTK_PROGRESS_BAR (progress));
  value += 0.1;
  ctk_progress_bar_set_fraction (CTK_PROGRESS_BAR (progress), value);

  if (value >= 1.0)
    {
      ctk_assistant_set_page_complete (CTK_ASSISTANT (assistant), progress, TRUE);
      return FALSE;
    }

  return TRUE;
}

static void
prepare_callback (CtkWidget *widget, CtkWidget *page)
{
  if (CTK_IS_LABEL (page))
    g_print ("prepare: %s\n", ctk_label_get_text (CTK_LABEL (page)));
  else if (ctk_assistant_get_page_type (CTK_ASSISTANT (widget), page) == CTK_ASSISTANT_PAGE_PROGRESS)
    {
      ctk_assistant_set_page_complete (CTK_ASSISTANT (widget), page, FALSE);
      ctk_progress_bar_set_fraction (CTK_PROGRESS_BAR (page), 0.0);
      cdk_threads_add_timeout (300, (GSourceFunc) progress_timeout, widget);
    }
  else
    g_print ("prepare: %d\n", ctk_assistant_get_current_page (CTK_ASSISTANT (widget)));
}

static void
create_simple_assistant (CtkWidget *widget G_GNUC_UNUSED)
{
  static CtkWidget *assistant = NULL;

  if (!assistant)
    {
      CtkWidget *page;

      assistant = ctk_assistant_new ();
      ctk_window_set_default_size (CTK_WINDOW (assistant), 400, 300);

      g_signal_connect (G_OBJECT (assistant), "cancel",
			G_CALLBACK (cancel_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "close",
			G_CALLBACK (close_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "apply",
			G_CALLBACK (apply_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "prepare",
			G_CALLBACK (prepare_callback), NULL);

      page = get_test_page ("Page 1");
      ctk_widget_show (page);
      ctk_assistant_append_page (CTK_ASSISTANT (assistant), page);
      ctk_assistant_set_page_title (CTK_ASSISTANT (assistant), page, "Page 1");
      ctk_assistant_set_page_complete (CTK_ASSISTANT (assistant), page, TRUE);

      page = get_test_page ("Page 2");
      ctk_widget_show (page);
      ctk_assistant_append_page (CTK_ASSISTANT (assistant), page);
      ctk_assistant_set_page_title (CTK_ASSISTANT (assistant), page, "Page 2");
      ctk_assistant_set_page_type  (CTK_ASSISTANT (assistant), page, CTK_ASSISTANT_PAGE_CONFIRM);
      ctk_assistant_set_page_complete (CTK_ASSISTANT (assistant), page, TRUE);
    }

  if (!ctk_widget_get_visible (assistant))
    ctk_widget_show (assistant);
  else
    {
      ctk_widget_destroy (assistant);
      assistant = NULL;
    }
}

static void
create_anonymous_assistant (CtkWidget *widget G_GNUC_UNUSED)
{
  static CtkWidget *assistant = NULL;

  if (!assistant)
    {
      CtkWidget *page;

      assistant = ctk_assistant_new ();
      ctk_window_set_default_size (CTK_WINDOW (assistant), 400, 300);

      g_signal_connect (G_OBJECT (assistant), "cancel",
			G_CALLBACK (cancel_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "close",
			G_CALLBACK (close_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "apply",
			G_CALLBACK (apply_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "prepare",
			G_CALLBACK (prepare_callback), NULL);

      page = get_test_page ("Page 1");
      ctk_widget_show (page);
      ctk_assistant_append_page (CTK_ASSISTANT (assistant), page);
      ctk_assistant_set_page_complete (CTK_ASSISTANT (assistant), page, TRUE);

      page = get_test_page ("Page 2");
      ctk_widget_show (page);
      ctk_assistant_append_page (CTK_ASSISTANT (assistant), page);
      ctk_assistant_set_page_type  (CTK_ASSISTANT (assistant), page, CTK_ASSISTANT_PAGE_CONFIRM);
      ctk_assistant_set_page_complete (CTK_ASSISTANT (assistant), page, TRUE);
    }

  if (!ctk_widget_get_visible (assistant))
    ctk_widget_show (assistant);
  else
    {
      ctk_widget_destroy (assistant);
      assistant = NULL;
    }
}

static void
visible_cb (CtkWidget *check, 
	    gpointer   data)
{
  CtkWidget *page = data;
  gboolean visible;

  visible = ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (check));

  g_object_set (G_OBJECT (page), "visible", visible, NULL);
}

static void
create_generous_assistant (CtkWidget *widget G_GNUC_UNUSED)
{
  static CtkWidget *assistant = NULL;

  if (!assistant)
    {
      CtkWidget *page, *next, *check;
      PageData  *pdata;

      assistant = ctk_assistant_new ();
      ctk_window_set_default_size (CTK_WINDOW (assistant), 400, 300);

      g_signal_connect (G_OBJECT (assistant), "cancel",
			G_CALLBACK (cancel_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "close",
			G_CALLBACK (close_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "apply",
			G_CALLBACK (apply_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "prepare",
			G_CALLBACK (prepare_callback), NULL);

      page = get_test_page ("Introduction");
      ctk_widget_show (page);
      ctk_assistant_append_page (CTK_ASSISTANT (assistant), page);
      ctk_assistant_set_page_title (CTK_ASSISTANT (assistant), page, "Introduction");
      ctk_assistant_set_page_type  (CTK_ASSISTANT (assistant), page, CTK_ASSISTANT_PAGE_INTRO);
      ctk_assistant_set_page_complete (CTK_ASSISTANT (assistant), page, TRUE);

      page = add_completion_test_page (assistant, "Content", TRUE, FALSE);
      next = add_completion_test_page (assistant, "More Content", TRUE, TRUE);

      check = ctk_check_button_new_with_label ("Next page visible");
      ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (check), TRUE);
      g_signal_connect (G_OBJECT (check), "toggled", 
			G_CALLBACK (visible_cb), next);
      ctk_widget_show (check);
      ctk_container_add (CTK_CONTAINER (page), check);
      
      add_completion_test_page (assistant, "Even More Content", TRUE, TRUE);

      page = get_test_page ("Confirmation");
      ctk_widget_show (page);
      ctk_assistant_append_page (CTK_ASSISTANT (assistant), page);
      ctk_assistant_set_page_title (CTK_ASSISTANT (assistant), page, "Confirmation");
      ctk_assistant_set_page_type  (CTK_ASSISTANT (assistant), page, CTK_ASSISTANT_PAGE_CONFIRM);
      ctk_assistant_set_page_complete (CTK_ASSISTANT (assistant), page, TRUE);

      page = ctk_progress_bar_new ();
      ctk_widget_set_halign (page, CTK_ALIGN_FILL);
      ctk_widget_set_valign (page, CTK_ALIGN_CENTER);
      ctk_widget_set_margin_start (page, 20);
      ctk_widget_set_margin_end (page, 20);
      ctk_widget_show_all (page);
      ctk_assistant_append_page (CTK_ASSISTANT (assistant), page);
      ctk_assistant_set_page_title (CTK_ASSISTANT (assistant), page, "Progress");
      ctk_assistant_set_page_type  (CTK_ASSISTANT (assistant), page, CTK_ASSISTANT_PAGE_PROGRESS);

      page = ctk_check_button_new_with_label ("Summary complete");
      ctk_widget_show (page);
      ctk_assistant_append_page (CTK_ASSISTANT (assistant), page);
      ctk_assistant_set_page_title (CTK_ASSISTANT (assistant), page, "Summary");
      ctk_assistant_set_page_type  (CTK_ASSISTANT (assistant), page, CTK_ASSISTANT_PAGE_SUMMARY);

      ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (page),
                                    ctk_assistant_get_page_complete (CTK_ASSISTANT (assistant),
                                                                     page));

      pdata = g_new (PageData, 1);
      pdata->assistant = CTK_ASSISTANT (assistant);
      pdata->page = page;
      g_signal_connect (page, "toggled",
                      G_CALLBACK (complete_cb), pdata);
    }

  if (!ctk_widget_get_visible (assistant))
    ctk_widget_show (assistant);
  else
    {
      ctk_widget_destroy (assistant);
      assistant = NULL;
    }
}

static gchar selected_branch = 'A';

static void
select_branch (CtkWidget *widget G_GNUC_UNUSED,
	       gchar      branch)
{
  selected_branch = branch;
}

static gint
nonlinear_assistant_forward_page (gint     current_page,
				  gpointer data G_GNUC_UNUSED)
{
  switch (current_page)
    {
    case 0:
      if (selected_branch == 'A')
	return 1;
      else
	return 2;
    case 1:
    case 2:
      return 3;
    default:
      return -1;
    }
}

static void
create_nonlinear_assistant (CtkWidget *widget G_GNUC_UNUSED)
{
  static CtkWidget *assistant = NULL;

  if (!assistant)
    {
      CtkWidget *page, *button;

      assistant = ctk_assistant_new ();
      ctk_window_set_default_size (CTK_WINDOW (assistant), 400, 300);

      g_signal_connect (G_OBJECT (assistant), "cancel",
			G_CALLBACK (cancel_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "close",
			G_CALLBACK (close_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "apply",
			G_CALLBACK (apply_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "prepare",
			G_CALLBACK (prepare_callback), NULL);

      ctk_assistant_set_forward_page_func (CTK_ASSISTANT (assistant),
					   nonlinear_assistant_forward_page,
					   NULL, NULL);

      page = ctk_box_new (CTK_ORIENTATION_VERTICAL, 6);

      button = ctk_radio_button_new_with_label (NULL, "branch A");
      ctk_box_pack_start (CTK_BOX (page), button, FALSE, FALSE, 0);
      g_signal_connect (G_OBJECT (button), "toggled", G_CALLBACK (select_branch), GINT_TO_POINTER ('A'));
      ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (button), TRUE);
      
      button = ctk_radio_button_new_with_label (ctk_radio_button_get_group (CTK_RADIO_BUTTON (button)),
						"branch B");
      ctk_box_pack_start (CTK_BOX (page), button, FALSE, FALSE, 0);
      g_signal_connect (G_OBJECT (button), "toggled", G_CALLBACK (select_branch), GINT_TO_POINTER ('B'));

      ctk_widget_show_all (page);
      ctk_assistant_append_page (CTK_ASSISTANT (assistant), page);
      ctk_assistant_set_page_title (CTK_ASSISTANT (assistant), page, "Page 1");
      ctk_assistant_set_page_complete (CTK_ASSISTANT (assistant), page, TRUE);
      
      page = get_test_page ("Page 2A");
      ctk_widget_show (page);
      ctk_assistant_append_page (CTK_ASSISTANT (assistant), page);
      ctk_assistant_set_page_title (CTK_ASSISTANT (assistant), page, "Page 2");
      ctk_assistant_set_page_complete (CTK_ASSISTANT (assistant), page, TRUE);

      page = get_test_page ("Page 2B");
      ctk_widget_show (page);
      ctk_assistant_append_page (CTK_ASSISTANT (assistant), page);
      ctk_assistant_set_page_title (CTK_ASSISTANT (assistant), page, "Page 2");
      ctk_assistant_set_page_complete (CTK_ASSISTANT (assistant), page, TRUE);

      page = get_test_page ("Confirmation");
      ctk_widget_show (page);
      ctk_assistant_append_page (CTK_ASSISTANT (assistant), page);
      ctk_assistant_set_page_title (CTK_ASSISTANT (assistant), page, "Confirmation");
      ctk_assistant_set_page_type  (CTK_ASSISTANT (assistant), page, CTK_ASSISTANT_PAGE_CONFIRM);
      ctk_assistant_set_page_complete (CTK_ASSISTANT (assistant), page, TRUE);
    }

  if (!ctk_widget_get_visible (assistant))
    ctk_widget_show (assistant);
  else
    {
      ctk_widget_destroy (assistant);
      assistant = NULL;
    }
}

static gint
looping_assistant_forward_page (gint current_page, gpointer data)
{
  switch (current_page)
    {
    case 0:
      return 1;
    case 1:
      return 2;
    case 2:
      return 3;
    case 3:
      {
	CtkAssistant *assistant;
	CtkWidget *page;

	assistant = (CtkAssistant*) data;
	page = ctk_assistant_get_nth_page (assistant, current_page);

	if (ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (page)))
	  return 0;
	else
	  return 4;
      }
    case 4:
    default:
      return -1;
    }
}

static void
create_looping_assistant (CtkWidget *widget G_GNUC_UNUSED)
{
  static CtkWidget *assistant = NULL;

  if (!assistant)
    {
      CtkWidget *page;

      assistant = ctk_assistant_new ();
      ctk_window_set_default_size (CTK_WINDOW (assistant), 400, 300);

      g_signal_connect (G_OBJECT (assistant), "cancel",
			G_CALLBACK (cancel_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "close",
			G_CALLBACK (close_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "apply",
			G_CALLBACK (apply_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "prepare",
			G_CALLBACK (prepare_callback), NULL);

      ctk_assistant_set_forward_page_func (CTK_ASSISTANT (assistant),
					   looping_assistant_forward_page,
					   assistant, NULL);

      page = get_test_page ("Introduction");
      ctk_widget_show (page);
      ctk_assistant_append_page (CTK_ASSISTANT (assistant), page);
      ctk_assistant_set_page_title (CTK_ASSISTANT (assistant), page, "Introduction");
      ctk_assistant_set_page_type  (CTK_ASSISTANT (assistant), page, CTK_ASSISTANT_PAGE_INTRO);
      ctk_assistant_set_page_complete (CTK_ASSISTANT (assistant), page, TRUE);

      page = get_test_page ("Content");
      ctk_widget_show (page);
      ctk_assistant_append_page (CTK_ASSISTANT (assistant), page);
      ctk_assistant_set_page_title (CTK_ASSISTANT (assistant), page, "Content");
      ctk_assistant_set_page_complete (CTK_ASSISTANT (assistant), page, TRUE);

      page = get_test_page ("More content");
      ctk_widget_show (page);
      ctk_assistant_append_page (CTK_ASSISTANT (assistant), page);
      ctk_assistant_set_page_title (CTK_ASSISTANT (assistant), page, "More content");
      ctk_assistant_set_page_complete (CTK_ASSISTANT (assistant), page, TRUE);

      page = ctk_check_button_new_with_label ("Loop?");
      ctk_widget_show (page);
      ctk_assistant_append_page (CTK_ASSISTANT (assistant), page);
      ctk_assistant_set_page_title (CTK_ASSISTANT (assistant), page, "Loop?");
      ctk_assistant_set_page_complete (CTK_ASSISTANT (assistant), page, TRUE);
      
      page = get_test_page ("Confirmation");
      ctk_widget_show (page);
      ctk_assistant_append_page (CTK_ASSISTANT (assistant), page);
      ctk_assistant_set_page_title (CTK_ASSISTANT (assistant), page, "Confirmation");
      ctk_assistant_set_page_type  (CTK_ASSISTANT (assistant), page, CTK_ASSISTANT_PAGE_CONFIRM);
      ctk_assistant_set_page_complete (CTK_ASSISTANT (assistant), page, TRUE);
    }

  if (!ctk_widget_get_visible (assistant))
    ctk_widget_show (assistant);
  else
    {
      ctk_widget_destroy (assistant);
      assistant = NULL;
    }
}

static void
toggle_invisible (CtkButton    *button G_GNUC_UNUSED,
		  CtkAssistant *assistant)
{
  CtkWidget *page;

  page = ctk_assistant_get_nth_page (assistant, 1);

  ctk_widget_set_visible (page, !ctk_widget_get_visible (page));
}

static void
create_full_featured_assistant (CtkWidget *widget G_GNUC_UNUSED)
{
  static CtkWidget *assistant = NULL;

  if (!assistant)
    {
      CtkWidget *page, *button;

      assistant = ctk_assistant_new ();
      ctk_window_set_default_size (CTK_WINDOW (assistant), 400, 300);

      button = ctk_button_new_with_label ("_Stop");
      ctk_button_set_use_underline (CTK_BUTTON (button), TRUE);
      ctk_widget_show (button);
      ctk_assistant_add_action_widget (CTK_ASSISTANT (assistant), button);
      g_signal_connect (button, "clicked",
                        G_CALLBACK (toggle_invisible), assistant);

      g_signal_connect (G_OBJECT (assistant), "cancel",
			G_CALLBACK (cancel_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "close",
			G_CALLBACK (close_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "apply",
			G_CALLBACK (apply_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "prepare",
			G_CALLBACK (prepare_callback), NULL);

      page = get_test_page ("Page 1");
      ctk_widget_show (page);
      ctk_assistant_append_page (CTK_ASSISTANT (assistant), page);
      ctk_assistant_set_page_title (CTK_ASSISTANT (assistant), page, "Page 1");
      ctk_assistant_set_page_complete (CTK_ASSISTANT (assistant), page, TRUE);

      page = get_test_page ("Invisible page");
      ctk_assistant_append_page (CTK_ASSISTANT (assistant), page);
      ctk_assistant_set_page_title (CTK_ASSISTANT (assistant), page, "Page 2");
      ctk_assistant_set_page_complete (CTK_ASSISTANT (assistant), page, TRUE);

      page = ctk_file_chooser_widget_new (CTK_FILE_CHOOSER_ACTION_OPEN);
      ctk_widget_show (page);
      ctk_assistant_append_page (CTK_ASSISTANT (assistant), page);
      ctk_assistant_set_page_title (CTK_ASSISTANT (assistant), page, "Filechooser");
      ctk_assistant_set_page_complete (CTK_ASSISTANT (assistant), page, TRUE);
      ctk_assistant_set_page_has_padding (CTK_ASSISTANT (assistant), page, FALSE);

      page = get_test_page ("Page 3");
      ctk_widget_show (page);
      ctk_assistant_append_page (CTK_ASSISTANT (assistant), page);
      ctk_assistant_set_page_title (CTK_ASSISTANT (assistant), page, "Page 3");
      ctk_assistant_set_page_type  (CTK_ASSISTANT (assistant), page, CTK_ASSISTANT_PAGE_CONFIRM);
      ctk_assistant_set_page_complete (CTK_ASSISTANT (assistant), page, TRUE);
    }

  if (!ctk_widget_get_visible (assistant))
    ctk_widget_show (assistant);
  else
    {
      ctk_widget_destroy (assistant);
      assistant = NULL;
    }
}

static void
flip_pages (CtkButton    *button G_GNUC_UNUSED,
	    CtkAssistant *assistant)
{
  CtkWidget *page;
  gchar *title;

  page = ctk_assistant_get_nth_page (assistant, 1);

  g_object_ref (page);

  title = g_strdup (ctk_assistant_get_page_title (assistant, page));

  ctk_assistant_remove_page (assistant, 1);
  ctk_assistant_insert_page (assistant, page, 2);

  ctk_widget_show_all (page);
  ctk_assistant_set_page_title (CTK_ASSISTANT (assistant), page, title);
  ctk_assistant_set_page_complete (CTK_ASSISTANT (assistant), page, TRUE);

  g_object_unref (page);
  g_free (title);
}


static void
create_page_flipping_assistant (CtkWidget *widget G_GNUC_UNUSED)
{
  static CtkWidget *assistant = NULL;

  if (!assistant)
    {
      CtkWidget *page, *button;

      assistant = ctk_assistant_new ();
      ctk_window_set_default_size (CTK_WINDOW (assistant), 400, 300);

      button = ctk_button_new_with_label ("_Flip");
      ctk_button_set_use_underline (CTK_BUTTON (button), TRUE);
      ctk_widget_show (button);
      ctk_assistant_add_action_widget (CTK_ASSISTANT (assistant), button);
      g_signal_connect (button, "clicked",
                        G_CALLBACK (flip_pages), assistant);

      g_signal_connect (G_OBJECT (assistant), "cancel",
			G_CALLBACK (cancel_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "close",
			G_CALLBACK (close_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "apply",
			G_CALLBACK (apply_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "prepare",
			G_CALLBACK (prepare_callback), NULL);

      page = get_test_page ("Page 1");
      ctk_widget_show (page);
      ctk_assistant_append_page (CTK_ASSISTANT (assistant), page);
      ctk_assistant_set_page_title (CTK_ASSISTANT (assistant), page, "Page 1");
      ctk_assistant_set_page_complete (CTK_ASSISTANT (assistant), page, TRUE);

      page = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
      ctk_box_pack_start (CTK_BOX (page),
                          get_test_page ("Page 2"),
                          TRUE,
                          TRUE,
                          0);
      ctk_widget_show_all (page);
      ctk_assistant_append_page (CTK_ASSISTANT (assistant), page);
      ctk_assistant_set_page_title (CTK_ASSISTANT (assistant), page, "Page 2");
      ctk_assistant_set_page_complete (CTK_ASSISTANT (assistant), page, TRUE);

      page = get_test_page ("Page 3");
      ctk_widget_show (page);
      ctk_assistant_append_page (CTK_ASSISTANT (assistant), page);
      ctk_assistant_set_page_title (CTK_ASSISTANT (assistant), page, "Page 3");
      ctk_assistant_set_page_complete (CTK_ASSISTANT (assistant), page, TRUE);

      page = get_test_page ("Summary");
      ctk_widget_show (page);
      ctk_assistant_append_page (CTK_ASSISTANT (assistant), page);
      ctk_assistant_set_page_title (CTK_ASSISTANT (assistant), page, "Summary");
      ctk_assistant_set_page_type  (CTK_ASSISTANT (assistant), page, CTK_ASSISTANT_PAGE_SUMMARY);
      ctk_assistant_set_page_complete (CTK_ASSISTANT (assistant), page, TRUE);
    }

  if (!ctk_widget_get_visible (assistant))
    ctk_widget_show (assistant);
  else
    {
      ctk_widget_destroy (assistant);
      assistant = NULL;
    }
}

struct {
  gchar *text;
  void  (*func) (CtkWidget *widget);
} buttons[] =
  {
    { "simple assistant",        create_simple_assistant },
    { "anonymous assistant",        create_anonymous_assistant },
    { "generous assistant",      create_generous_assistant },
    { "nonlinear assistant",     create_nonlinear_assistant },
    { "looping assistant",       create_looping_assistant },
    { "full featured assistant", create_full_featured_assistant },
    { "page-flipping assistant", create_page_flipping_assistant },
  };

int
main (int argc, gchar *argv[])
{
  CtkWidget *window, *box;
  gint i;

  ctk_init (&argc, &argv);

  if (g_getenv ("RTL"))
    ctk_widget_set_default_direction (CTK_TEXT_DIR_RTL);

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);

  g_signal_connect (G_OBJECT (window), "destroy",
		    G_CALLBACK (ctk_main_quit), NULL);
  g_signal_connect (G_OBJECT (window), "delete-event",
		    G_CALLBACK (ctk_false), NULL);

  box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 6);
  ctk_container_add (CTK_CONTAINER (window), box);

  for (i = 0; i < G_N_ELEMENTS (buttons); i++)
    {
      CtkWidget *button;

      button = ctk_button_new_with_label (buttons[i].text);

      if (buttons[i].func)
	g_signal_connect (G_OBJECT (button), "clicked",
			  G_CALLBACK (buttons[i].func), NULL);

      ctk_box_pack_start (CTK_BOX (box), button, TRUE, TRUE, 0);
    }

  ctk_widget_show_all (window);
  ctk_main ();

  return 0;
}
