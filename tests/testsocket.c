/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 2 -*- */
/* testsocket.c
 * Copyright (C) 2001 Red Hat, Inc
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
#include <ctk/ctk.h>
#include <ctk/ctkx.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

int n_children = 0;

GSList *sockets = NULL;

CtkWidget *window;
CtkWidget *box;

typedef struct 
{
  CtkWidget *box;
  CtkWidget *frame;
  CtkWidget *socket;
} Socket;

extern guint32 create_child_plug (guint32  xid,
				  gboolean local);

static void
quit_cb (gpointer        callback_data,
	 guint           callback_action,
	 CtkWidget      *widget)
{
  CtkWidget *message_dialog = ctk_message_dialog_new (CTK_WINDOW (window), 0,
						      CTK_MESSAGE_QUESTION,
						      CTK_BUTTONS_YES_NO,
						      "Really Quit?");
  ctk_dialog_set_default_response (CTK_DIALOG (message_dialog), CTK_RESPONSE_NO);

  if (ctk_dialog_run (CTK_DIALOG (message_dialog)) == CTK_RESPONSE_YES)
    ctk_widget_destroy (window);

  ctk_widget_destroy (message_dialog);
}

static void
socket_destroyed (CtkWidget *widget,
		  Socket    *socket)
{
  sockets = g_slist_remove (sockets, socket);
  g_free (socket);
}

static void
plug_added (CtkWidget *widget,
	    Socket    *socket)
{
  g_print ("Plug added to socket\n");
  
  ctk_widget_show (socket->socket);
  ctk_widget_hide (socket->frame);
}

static gboolean
plug_removed (CtkWidget *widget,
	      Socket    *socket)
{
  g_print ("Plug removed from socket\n");
  
  ctk_widget_hide (socket->socket);
  ctk_widget_show (socket->frame);
  
  return TRUE;
}

static Socket *
create_socket (void)
{
  CtkWidget *label;
  
  Socket *socket = g_new (Socket, 1);
  
  socket->box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);

  socket->socket = ctk_socket_new ();
  
  ctk_box_pack_start (CTK_BOX (socket->box), socket->socket, TRUE, TRUE, 0);
  
  socket->frame = ctk_frame_new (NULL);
  ctk_frame_set_shadow_type (CTK_FRAME (socket->frame), CTK_SHADOW_IN);
  ctk_box_pack_start (CTK_BOX (socket->box), socket->frame, TRUE, TRUE, 0);
  ctk_widget_show (socket->frame);
  
  label = ctk_label_new (NULL);
  ctk_label_set_markup (CTK_LABEL (label), "<span color=\"red\">Empty</span>");
  ctk_container_add (CTK_CONTAINER (socket->frame), label);
  ctk_widget_show (label);

  sockets = g_slist_prepend (sockets, socket);


  g_signal_connect (socket->socket, "destroy",
		    G_CALLBACK (socket_destroyed), socket);
  g_signal_connect (socket->socket, "plug_added",
		    G_CALLBACK (plug_added), socket);
  g_signal_connect (socket->socket, "plug_removed",
		    G_CALLBACK (plug_removed), socket);

  return socket;
}

void
remove_child (CtkWidget *window)
{
  if (sockets)
    {
      Socket *socket = sockets->data;
      ctk_widget_destroy (socket->box);
    }
}

static gboolean
child_read_watch (GIOChannel *channel, GIOCondition cond, gpointer data)
{
  GIOStatus status;
  GError *error = NULL;
  char *line;
  gsize term;
  int xid;
  
  status = g_io_channel_read_line (channel, &line, NULL, &term, &error);
  switch (status)
    {
    case G_IO_STATUS_NORMAL:
      line[term] = '\0';
      xid = strtol (line, NULL, 0);
      if (xid == 0)
	{
	  fprintf (stderr, "Invalid window id '%s'\n", line);
	}
      else
	{
	  Socket *socket = create_socket ();
	  ctk_box_pack_start (CTK_BOX (box), socket->box, TRUE, TRUE, 0);
	  ctk_widget_show (socket->box);
	  
	  ctk_socket_add_id (CTK_SOCKET (socket->socket), xid);
	}
      g_free (line);
      return TRUE;
    case G_IO_STATUS_AGAIN:
      return TRUE;
    case G_IO_STATUS_EOF:
      n_children--;
      return FALSE;
    case G_IO_STATUS_ERROR:
      fprintf (stderr, "Error reading fd from child: %s\n", error->message);
      exit (1);
      return FALSE;
    default:
      g_assert_not_reached ();
      return FALSE;
    }
  
}

void
add_child (CtkWidget *window,
	   gboolean   active)
{
  Socket *socket;
  char *argv[3] = { "./testsocket_child", NULL, NULL };
  char buffer[20];
  int out_fd;
  GIOChannel *channel;
  GError *error = NULL;

  if (active)
    {
      socket = create_socket ();
      ctk_box_pack_start (CTK_BOX (box), socket->box, TRUE, TRUE, 0);
      ctk_widget_show (socket->box);
      sprintf(buffer, "%#lx", (gulong) ctk_socket_get_id (CTK_SOCKET (socket->socket)));
      argv[1] = buffer;
    }
  
  if (!g_spawn_async_with_pipes (NULL, argv, NULL, 0, NULL, NULL, NULL, NULL, &out_fd, NULL, &error))
    {
      fprintf (stderr, "Can't exec testsocket_child: %s\n", error->message);
      exit (1);
    }

  n_children++;
  channel = g_io_channel_unix_new (out_fd);
  g_io_channel_set_close_on_unref (channel, TRUE);
  g_io_channel_set_flags (channel, G_IO_FLAG_NONBLOCK, &error);
  if (error)
    {
      fprintf (stderr, "Error making channel non-blocking: %s\n", error->message);
      exit (1);
    }
  
  g_io_add_watch (channel, G_IO_IN | G_IO_HUP, child_read_watch, NULL);
  g_io_channel_unref (channel);
}

void
add_active_child (CtkWidget *window)
{
  add_child (window, TRUE);
}

void
add_passive_child (CtkWidget *window)
{
  add_child (window, FALSE);
}

void
add_local_active_child (CtkWidget *window)
{
  Socket *socket;

  socket = create_socket ();
  ctk_box_pack_start (CTK_BOX (box), socket->box, TRUE, TRUE, 0);
  ctk_widget_show (socket->box);

  create_child_plug (ctk_socket_get_id (CTK_SOCKET (socket->socket)), TRUE);
}

void
add_local_passive_child (CtkWidget *window)
{
  Socket *socket;
  Window xid;

  socket = create_socket ();
  ctk_box_pack_start (CTK_BOX (box), socket->box, TRUE, TRUE, 0);
  ctk_widget_show (socket->box);

  xid = create_child_plug (0, TRUE);
  ctk_socket_add_id (CTK_SOCKET (socket->socket), xid);
}

static const char *
grab_string (int status)
{
  switch (status) {
  case GDK_GRAB_SUCCESS:          return "GrabSuccess";
  case GDK_GRAB_ALREADY_GRABBED:  return "AlreadyGrabbed";
  case GDK_GRAB_INVALID_TIME:     return "GrabInvalidTime";
  case GDK_GRAB_NOT_VIEWABLE:     return "GrabNotViewable";
  case GDK_GRAB_FROZEN:           return "GrabFrozen";
  default:
    {
      static char foo [255];
      sprintf (foo, "unknown status: %d", status);
      return foo;
    }
  }
}

static void
grab_window_toggled (CtkToggleButton *button,
		     CtkWidget       *widget)
{
  GdkDevice *device = ctk_get_current_event_device ();
  GdkSeat *seat = cdk_device_get_seat (device);

  if (ctk_toggle_button_get_active (button))
    {
      int status;

      status = cdk_seat_grab (seat, ctk_widget_get_window (widget),
                              GDK_SEAT_CAPABILITY_KEYBOARD,
                              FALSE, NULL, NULL, NULL, NULL);

      if (status != GDK_GRAB_SUCCESS)
	g_warning ("Could not grab keyboard!  (%s)", grab_string (status));

    }
  else
    {
      cdk_seat_ungrab (seat);
    }
}

int
main (int argc, char *argv[])
{
  CtkWidget *button;
  CtkWidget *hbox;
  CtkWidget *vbox;
  CtkWidget *menubar;
  CtkWidget *menuitem;
  CtkWidget *menu;
  CtkWidget *entry;
  CtkWidget *checkbutton;
  CtkAccelGroup *accel_group;

  ctk_init (&argc, &argv);

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  g_signal_connect (window, "destroy",
		    G_CALLBACK (ctk_main_quit), NULL);
  
  ctk_window_set_title (CTK_WINDOW (window), "Socket Test");
  ctk_container_set_border_width (CTK_CONTAINER (window), 0);

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  ctk_container_add (CTK_CONTAINER (window), vbox);

  menubar = ctk_menu_bar_new ();
  menuitem = ctk_menu_item_new_with_mnemonic ("_File");
  ctk_menu_shell_append (CTK_MENU_SHELL (menubar), menuitem);

  menu = ctk_menu_new ();
  ctk_menu_item_set_submenu (CTK_MENU_ITEM (menuitem), menu);
  menuitem = ctk_menu_item_new_with_mnemonic ("_Quit");
  g_signal_connect (menuitem, "activate", G_CALLBACK (quit_cb), window);
  ctk_menu_shell_append (CTK_MENU_SHELL (menu), menuitem);

  accel_group = ctk_accel_group_new ();
  ctk_window_add_accel_group (CTK_WINDOW (window), accel_group);
  ctk_box_pack_start (CTK_BOX (vbox), menubar, FALSE, FALSE, 0);

  button = ctk_button_new_with_label ("Add Active Child");
  ctk_box_pack_start (CTK_BOX(vbox), button, FALSE, FALSE, 0);

  g_signal_connect_swapped (button, "clicked",
			    G_CALLBACK (add_active_child), vbox);

  button = ctk_button_new_with_label ("Add Passive Child");
  ctk_box_pack_start (CTK_BOX(vbox), button, FALSE, FALSE, 0);

  g_signal_connect_swapped (button, "clicked",
			    G_CALLBACK (add_passive_child), vbox);

  button = ctk_button_new_with_label ("Add Local Active Child");
  ctk_box_pack_start (CTK_BOX(vbox), button, FALSE, FALSE, 0);

  g_signal_connect_swapped (button, "clicked",
			    G_CALLBACK (add_local_active_child), vbox);

  button = ctk_button_new_with_label ("Add Local Passive Child");
  ctk_box_pack_start (CTK_BOX(vbox), button, FALSE, FALSE, 0);

  g_signal_connect_swapped (button, "clicked",
			    G_CALLBACK (add_local_passive_child), vbox);

  button = ctk_button_new_with_label ("Remove Last Child");
  ctk_box_pack_start (CTK_BOX(vbox), button, FALSE, FALSE, 0);

  g_signal_connect_swapped (button, "clicked",
			    G_CALLBACK (remove_child), vbox);

  checkbutton = ctk_check_button_new_with_label ("Grab keyboard");
  ctk_box_pack_start (CTK_BOX (vbox), checkbutton, FALSE, FALSE, 0);

  g_signal_connect (checkbutton, "toggled",
		    G_CALLBACK (grab_window_toggled),
		    window);

  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
  ctk_box_pack_start (CTK_BOX(vbox), hbox, FALSE, FALSE, 0);

  entry = ctk_entry_new ();
  ctk_box_pack_start (CTK_BOX(hbox), entry, FALSE, FALSE, 0);

  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
  ctk_box_pack_start (CTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  box = hbox;
  
  ctk_widget_show_all (window);

  ctk_main ();

  if (n_children)
    {
      g_print ("Waiting for children to exit\n");

      while (n_children)
	g_main_context_iteration (NULL, TRUE);
    }

  return 0;
}
