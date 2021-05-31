/* testdnd.c
 * Copyright (C) 1998  Red Hat, Inc.
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
#include "ctk/ctk.h"

/* Target side drag signals */

/* XPM */
static const char * drag_icon_xpm[] = {
"36 48 9 1",
" 	c None",
".	c #020204",
"+	c #8F8F90",
"@	c #D3D3D2",
"#	c #AEAEAC",
"$	c #ECECEC",
"%	c #A2A2A4",
"&	c #FEFEFC",
"*	c #BEBEBC",
"               .....................",
"              ..&&&&&&&&&&&&&&&&&&&.",
"             ...&&&&&&&&&&&&&&&&&&&.",
"            ..&.&&&&&&&&&&&&&&&&&&&.",
"           ..&&.&&&&&&&&&&&&&&&&&&&.",
"          ..&&&.&&&&&&&&&&&&&&&&&&&.",
"         ..&&&&.&&&&&&&&&&&&&&&&&&&.",
"        ..&&&&&.&&&@&&&&&&&&&&&&&&&.",
"       ..&&&&&&.*$%$+$&&&&&&&&&&&&&.",
"      ..&&&&&&&.%$%$+&&&&&&&&&&&&&&.",
"     ..&&&&&&&&.#&#@$&&&&&&&&&&&&&&.",
"    ..&&&&&&&&&.#$**#$&&&&&&&&&&&&&.",
"   ..&&&&&&&&&&.&@%&%$&&&&&&&&&&&&&.",
"  ..&&&&&&&&&&&.&&&&&&&&&&&&&&&&&&&.",
" ..&&&&&&&&&&&&.&&&&&&&&&&&&&&&&&&&.",
"................&$@&&&@&&&&&&&&&&&&.",
".&&&&&&&+&&#@%#+@#@*$%$+$&&&&&&&&&&.",
".&&&&&&&+&&#@#@&&@*%$%$+&&&&&&&&&&&.",
".&&&&&&&+&$%&#@&#@@#&#@$&&&&&&&&&&&.",
".&&&&&&@#@@$&*@&@#@#$**#$&&&&&&&&&&.",
".&&&&&&&&&&&&&&&&&&&@%&%$&&&&&&&&&&.",
".&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&.",
".&&&&&&&&$#@@$&&&&&&&&&&&&&&&&&&&&&.",
".&&&&&&&&&+&$+&$&@&$@&&$@&&&&&&&&&&.",
".&&&&&&&&&+&&#@%#+@#@*$%&+$&&&&&&&&.",
".&&&&&&&&&+&&#@#@&&@*%$%$+&&&&&&&&&.",
".&&&&&&&&&+&$%&#@&#@@#&#@$&&&&&&&&&.",
".&&&&&&&&@#@@$&*@&@#@#$#*#$&&&&&&&&.",
".&&&&&&&&&&&&&&&&&&&&&$%&%$&&&&&&&&.",
".&&&&&&&&&&$#@@$&&&&&&&&&&&&&&&&&&&.",
".&&&&&&&&&&&+&$%&$$@&$@&&$@&&&&&&&&.",
".&&&&&&&&&&&+&&#@%#+@#@*$%$+$&&&&&&.",
".&&&&&&&&&&&+&&#@#@&&@*#$%$+&&&&&&&.",
".&&&&&&&&&&&+&$+&*@&#@@#&#@$&&&&&&&.",
".&&&&&&&&&&$%@@&&*@&@#@#$#*#&&&&&&&.",
".&&&&&&&&&&&&&&&&&&&&&&&$%&%$&&&&&&.",
".&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&.",
".&&&&&&&&&&&&&&$#@@$&&&&&&&&&&&&&&&.",
".&&&&&&&&&&&&&&&+&$%&$$@&$@&&$@&&&&.",
".&&&&&&&&&&&&&&&+&&#@%#+@#@*$%$+$&&.",
".&&&&&&&&&&&&&&&+&&#@#@&&@*#$%$+&&&.",
".&&&&&&&&&&&&&&&+&$+&*@&#@@#&#@$&&&.",
".&&&&&&&&&&&&&&$%@@&&*@&@#@#$#*#&&&.",
".&&&&&&&&&&&&&&&&&&&&&&&&&&&$%&%$&&.",
".&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&.",
".&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&.",
".&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&.",
"...................................."};

/* XPM */
static const char * trashcan_closed_xpm[] = {
"64 80 17 1",
" 	c None",
".	c #030304",
"+	c #5A5A5C",
"@	c #323231",
"#	c #888888",
"$	c #1E1E1F",
"%	c #767677",
"&	c #494949",
"*	c #9E9E9C",
"=	c #111111",
"-	c #3C3C3D",
";	c #6B6B6B",
">	c #949494",
",	c #282828",
"'	c #808080",
")	c #545454",
"!	c #AEAEAC",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                       ==......=$$...===                        ",
"                 ..$------)+++++++++++++@$$...                  ",
"             ..=@@-------&+++++++++++++++++++-....              ",
"          =.$$@@@-&&)++++)-,$$$$=@@&+++++++++++++,..$           ",
"         .$$$$@@&+++++++&$$$@@@@-&,$,-++++++++++;;;&..          ",
"        $$$$,@--&++++++&$$)++++++++-,$&++++++;%%'%%;;$@         ",
"       .-@@-@-&++++++++-@++++++++++++,-++++++;''%;;;%*-$        ",
"       +------++++++++++++++++++++++++++++++;;%%%;;##*!.        ",
"        =+----+++++++++++++++++++++++;;;;;;;;;;;;%'>>).         ",
"         .=)&+++++++++++++++++;;;;;;;;;;;;;;%''>>#>#@.          ",
"          =..=&++++++++++++;;;;;;;;;;;;;%###>>###+%==           ",
"           .&....=-+++++%;;####''''''''''##'%%%)..#.            ",
"           .+-++@....=,+%#####'%%%%%%%%%;@$-@-@*++!.            ",
"           .+-++-+++-&-@$$=$=......$,,,@;&)+!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           =+-++-+++-+++++++++!++++!++++!+++!++!+++=            ",
"            $.++-+++-+++++++++!++++!++++!+++!++!+.$             ",
"              =.++++++++++++++!++++!++++!+++!++.=               ",
"                 $..+++++++++++++++!++++++...$                  ",
"                      $$=.............=$$                       ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                "};

/* XPM */
static const char * trashcan_open_xpm[] = {
"64 80 17 1",
" 	c None",
".	c #030304",
"+	c #5A5A5C",
"@	c #323231",
"#	c #888888",
"$	c #1E1E1F",
"%	c #767677",
"&	c #494949",
"*	c #9E9E9C",
"=	c #111111",
"-	c #3C3C3D",
";	c #6B6B6B",
">	c #949494",
",	c #282828",
"'	c #808080",
")	c #545454",
"!	c #AEAEAC",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                      .=.==.,@                  ",
"                                   ==.,@-&&&)-=                 ",
"                                 .$@,&++;;;%>*-                 ",
"                               $,-+)+++%%;;'#+.                 ",
"                            =---+++++;%%%;%##@.                 ",
"                           @)++++++++;%%%%'#%$                  ",
"                         $&++++++++++;%%;%##@=                  ",
"                       ,-++++)+++++++;;;'#%)                    ",
"                      @+++&&--&)++++;;%'#'-.                    ",
"                    ,&++-@@,,,,-)++;;;'>'+,                     ",
"                  =-++&@$@&&&&-&+;;;%##%+@                      ",
"                =,)+)-,@@&+++++;;;;%##%&@                       ",
"               @--&&,,@&)++++++;;;;'#)@                         ",
"              ---&)-,@)+++++++;;;%''+,                          ",
"            $--&)+&$-+++++++;;;%%'';-                           ",
"           .,-&+++-$&++++++;;;%''%&=                            ",
"          $,-&)++)-@++++++;;%''%),                              ",
"         =,@&)++++&&+++++;%'''+$@&++++++                        ",
"        .$@-++++++++++++;'#';,........=$@&++++                  ",
"       =$@@&)+++++++++++'##-.................=&++               ",
"      .$$@-&)+++++++++;%#+$.....................=)+             ",
"      $$,@-)+++++++++;%;@=........................,+            ",
"     .$$@@-++++++++)-)@=............................            ",
"     $,@---)++++&)@===............................,.            ",
"    $-@---&)))-$$=..............................=)!.            ",
"     --&-&&,,$=,==...........................=&+++!.            ",
"      =,=$..=$+)+++++&@$=.............=$@&+++++!++!.            ",
"           .)-++-+++++++++++++++++++++++++++!++!++!.            ",
"           .+-++-+++++++++++++++++++++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!+++!!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           =+-++-+++-+++++++++!++++!++++!+++!++!+++=            ",
"            $.++-+++-+++++++++!++++!++++!+++!++!+.$             ",
"              =.++++++++++++++!++++!++++!+++!++.=               ",
"                 $..+++++++++++++++!++++++...$                  ",
"                      $$==...........==$$                       ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                "};

GdkPixbuf *trashcan_open;
GdkPixbuf *trashcan_closed;

gboolean have_drag;

enum {
  TARGET_STRING,
  TARGET_ROOTWIN
};

static GtkTargetEntry target_table[] = {
  { "STRING",     0, TARGET_STRING },
  { "text/plain", 0, TARGET_STRING },
  { "application/x-rootwindow-drop", 0, TARGET_ROOTWIN }
};

static guint n_targets = sizeof(target_table) / sizeof(target_table[0]);

void  
target_drag_leave	   (GtkWidget	       *widget,
			    GdkDragContext     *context,
			    guint               time)
{
  g_print("leave\n");
  have_drag = FALSE;
  ctk_image_set_from_pixbuf (CTK_IMAGE (widget), trashcan_closed);
}

gboolean
target_drag_motion	   (GtkWidget	       *widget,
			    GdkDragContext     *context,
			    gint                x,
			    gint                y,
			    guint               time)
{
  GtkWidget *source_widget;
  GList *tmp_list;

  if (!have_drag)
    {
      have_drag = TRUE;
      ctk_image_set_from_pixbuf (CTK_IMAGE (widget), trashcan_open);
    }

  source_widget = ctk_drag_get_source_widget (context);
  g_print ("motion, source %s\n", source_widget ?
	   G_OBJECT_TYPE_NAME (source_widget) :
	   "NULL");

  tmp_list = gdk_drag_context_list_targets (context);
  while (tmp_list)
    {
      char *name = gdk_atom_name (GDK_POINTER_TO_ATOM (tmp_list->data));
      g_print ("%s\n", name);
      g_free (name);
      
      tmp_list = tmp_list->next;
    }

  gdk_drag_status (context, gdk_drag_context_get_suggested_action (context), time);

  return TRUE;
}

gboolean
target_drag_drop	   (GtkWidget	       *widget,
			    GdkDragContext     *context,
			    gint                x,
			    gint                y,
			    guint               time)
{
  g_print("drop\n");
  have_drag = FALSE;

  ctk_image_set_from_pixbuf (CTK_IMAGE (widget), trashcan_closed);

  if (gdk_drag_context_list_targets (context))
    {
      ctk_drag_get_data (widget, context,
			 GDK_POINTER_TO_ATOM (gdk_drag_context_list_targets (context)->data),
			 time);
      return TRUE;
    }
  
  return FALSE;
}

void  
target_drag_data_received  (GtkWidget          *widget,
			    GdkDragContext     *context,
			    gint                x,
			    gint                y,
			    GtkSelectionData   *selection_data,
			    guint               info,
			    guint               time)
{
  if (ctk_selection_data_get_length (selection_data) >= 0 &&
      ctk_selection_data_get_format (selection_data) == 8)
    {
      g_print ("Received \"%s\" in trashcan\n", (gchar *) ctk_selection_data_get_data (selection_data));
      ctk_drag_finish (context, TRUE, FALSE, time);
      return;
    }
  
  ctk_drag_finish (context, FALSE, FALSE, time);
}
  
void  
label_drag_data_received  (GtkWidget          *widget,
			    GdkDragContext     *context,
			    gint                x,
			    gint                y,
			    GtkSelectionData   *selection_data,
			    guint               info,
			    guint               time)
{
  if (ctk_selection_data_get_length (selection_data) >= 0 &&
      ctk_selection_data_get_format (selection_data) == 8)
    {
      g_print ("Received \"%s\" in label\n", (gchar *) ctk_selection_data_get_data (selection_data));
      ctk_drag_finish (context, TRUE, FALSE, time);
      return;
    }
  
  ctk_drag_finish (context, FALSE, FALSE, time);
}

void  
source_drag_data_get  (GtkWidget          *widget,
		       GdkDragContext     *context,
		       GtkSelectionData   *selection_data,
		       guint               info,
		       guint               time,
		       gpointer            data)
{
  if (info == TARGET_ROOTWIN)
    g_print ("I was dropped on the rootwin\n");
  else
    ctk_selection_data_set (selection_data,
			    ctk_selection_data_get_target (selection_data),
			    8, (guchar *) "I'm Data!", 9);
}
  
/* The following is a rather elaborate example demonstrating/testing
 * changing of the window hierarchy during a drag - in this case,
 * via a "spring-loaded" popup window.
 */
static GtkWidget *popup_window = NULL;

static gboolean popped_up = FALSE;
static gboolean in_popup = FALSE;
static guint popdown_timer = 0;
static guint popup_timer = 0;

gint
popdown_cb (gpointer data)
{
  popdown_timer = 0;

  ctk_widget_hide (popup_window);
  popped_up = FALSE;

  return FALSE;
}

gboolean
popup_motion	   (GtkWidget	       *widget,
		    GdkDragContext     *context,
		    gint                x,
		    gint                y,
		    guint               time)
{
  if (!in_popup)
    {
      in_popup = TRUE;
      if (popdown_timer)
	{
	  g_print ("removed popdown\n");
	  g_source_remove (popdown_timer);
	  popdown_timer = 0;
	}
    }

  return TRUE;
}

void  
popup_leave	   (GtkWidget	       *widget,
		    GdkDragContext     *context,
		    guint               time)
{
  if (in_popup)
    {
      in_popup = FALSE;
      if (!popdown_timer)
	{
	  g_print ("added popdown\n");
	  popdown_timer = gdk_threads_add_timeout (500, popdown_cb, NULL);
	}
    }
}

gboolean
popup_cb (gpointer data)
{
  if (!popped_up)
    {
      if (!popup_window)
	{
	  GtkWidget *button;
	  GtkWidget *grid;
	  int i, j;
	  
	  popup_window = ctk_window_new (CTK_WINDOW_POPUP);
	  ctk_window_set_position (CTK_WINDOW (popup_window), CTK_WIN_POS_MOUSE);

	  grid = ctk_grid_new ();

	  for (i=0; i<3; i++)
	    for (j=0; j<3; j++)
	      {
		char buffer[128];
		g_snprintf(buffer, sizeof(buffer), "%d,%d", i, j);
		button = ctk_button_new_with_label (buffer);
                ctk_widget_set_hexpand (button, TRUE);
                ctk_widget_set_vexpand (button, TRUE);
		ctk_grid_attach (CTK_GRID (grid), button, i, j, 1, 1);

		ctk_drag_dest_set (button,
				   CTK_DEST_DEFAULT_ALL,
				   target_table, n_targets - 1, /* no rootwin */
				   GDK_ACTION_COPY | GDK_ACTION_MOVE);
		g_signal_connect (button, "drag_motion",
				  G_CALLBACK (popup_motion), NULL);
		g_signal_connect (button, "drag_leave",
				  G_CALLBACK (popup_leave), NULL);
	      }

	  ctk_widget_show_all (grid);
	  ctk_container_add (CTK_CONTAINER (popup_window), grid);

	}
      ctk_widget_show (popup_window);
      popped_up = TRUE;
    }

  popdown_timer = gdk_threads_add_timeout (500, popdown_cb, NULL);
  g_print ("added popdown\n");

  popup_timer = FALSE;

  return FALSE;
}

gboolean
popsite_motion	   (GtkWidget	       *widget,
		    GdkDragContext     *context,
		    gint                x,
		    gint                y,
		    guint               time)
{
  if (!popup_timer)
    popup_timer = gdk_threads_add_timeout (500, popup_cb, NULL);

  return TRUE;
}

void  
popsite_leave	   (GtkWidget	       *widget,
		    GdkDragContext     *context,
		    guint               time)
{
  if (popup_timer)
    {
      g_source_remove (popup_timer);
      popup_timer = 0;
    }
}

void  
source_drag_data_delete  (GtkWidget          *widget,
			  GdkDragContext     *context,
			  gpointer            data)
{
  g_print ("Delete the data!\n");
}
  
void
test_init (void)
{
  if (g_file_test ("../modules/input/immodules.cache", G_FILE_TEST_EXISTS))
    g_setenv ("CTK_IM_MODULE_FILE", "../modules/input/immodules.cache", TRUE);
}

int 
main (int argc, char **argv)
{
  GtkWidget *window;
  GtkWidget *grid;
  GtkWidget *label;
  GtkWidget *pixmap;
  GtkWidget *button;
  GdkPixbuf *drag_icon;

  test_init ();
  
  ctk_init (&argc, &argv); 

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  g_signal_connect (window, "destroy",
		    G_CALLBACK (ctk_main_quit), NULL);

  
  grid = ctk_grid_new ();
  ctk_container_add (CTK_CONTAINER (window), grid);

  drag_icon = gdk_pixbuf_new_from_xpm_data (drag_icon_xpm);
  trashcan_open = gdk_pixbuf_new_from_xpm_data (trashcan_open_xpm);
  trashcan_closed = gdk_pixbuf_new_from_xpm_data (trashcan_closed_xpm);
  
  label = ctk_label_new ("Drop Here\n");

  ctk_drag_dest_set (label,
		     CTK_DEST_DEFAULT_ALL,
		     target_table, n_targets - 1, /* no rootwin */
		     GDK_ACTION_COPY | GDK_ACTION_MOVE);

  g_signal_connect (label, "drag_data_received",
		    G_CALLBACK( label_drag_data_received), NULL);

  ctk_widget_set_hexpand (label, TRUE);
  ctk_widget_set_vexpand (label, TRUE);
  ctk_grid_attach (CTK_GRID (grid), label, 0, 0, 1, 1);

  label = ctk_label_new ("Popup\n");

  ctk_drag_dest_set (label,
		     CTK_DEST_DEFAULT_ALL,
		     target_table, n_targets - 1, /* no rootwin */
		     GDK_ACTION_COPY | GDK_ACTION_MOVE);

  ctk_widget_set_hexpand (label, TRUE);
  ctk_widget_set_vexpand (label, TRUE);
  ctk_grid_attach (CTK_GRID (grid), label, 1, 1, 1, 1);

  g_signal_connect (label, "drag_motion",
		    G_CALLBACK (popsite_motion), NULL);
  g_signal_connect (label, "drag_leave",
		    G_CALLBACK (popsite_leave), NULL);
  
  pixmap = ctk_image_new_from_pixbuf (trashcan_closed);
  ctk_drag_dest_set (pixmap, 0, NULL, 0, 0);
  ctk_widget_set_hexpand (pixmap, TRUE);
  ctk_widget_set_vexpand (pixmap, TRUE);
  ctk_grid_attach (CTK_GRID (grid), pixmap, 1, 0, 1, 1);

  g_signal_connect (pixmap, "drag_leave",
		    G_CALLBACK (target_drag_leave), NULL);

  g_signal_connect (pixmap, "drag_motion",
		    G_CALLBACK (target_drag_motion), NULL);

  g_signal_connect (pixmap, "drag_drop",
		    G_CALLBACK (target_drag_drop), NULL);

  g_signal_connect (pixmap, "drag_data_received",
		    G_CALLBACK (target_drag_data_received), NULL);

  /* Drag site */

  button = ctk_button_new_with_label ("Drag Here\n");

  ctk_drag_source_set (button, GDK_BUTTON1_MASK | GDK_BUTTON3_MASK,
		       target_table, n_targets, 
		       GDK_ACTION_COPY | GDK_ACTION_MOVE);
  ctk_drag_source_set_icon_pixbuf (button, drag_icon);

  g_object_unref (drag_icon);

  ctk_widget_set_hexpand (button, TRUE);
  ctk_widget_set_vexpand (button, TRUE);
  ctk_grid_attach (CTK_GRID (grid), button, 0, 1, 1, 1);

  g_signal_connect (button, "drag_data_get",
		    G_CALLBACK (source_drag_data_get), NULL);
  g_signal_connect (button, "drag_data_delete",
		    G_CALLBACK (source_drag_data_delete), NULL);

  ctk_widget_show_all (window);

  ctk_main ();

  return 0;
}
