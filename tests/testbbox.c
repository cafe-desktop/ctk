/*
 * Copyright (C) 2006 Nokia Corporation.
 * Author: Xan Lopez <xan.lopez@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <gtk/gtk.h>

#define N_BUTTONS 3

GtkWidget *bbox = NULL;
GtkWidget *hbbox = NULL, *vbbox = NULL;

static const char* styles[] = { "CTK_BUTTONBOX_SPREAD",
				"CTK_BUTTONBOX_EDGE",
				"CTK_BUTTONBOX_START",
				"CTK_BUTTONBOX_END",
				"CTK_BUTTONBOX_CENTER",
				"CTK_BUTTONBOX_EXPAND",
				NULL};

static const char* types[] = { "GtkHButtonBox",
			       "GtkVButtonBox",
			       NULL};

static void
populate_combo_with (GtkComboBoxText *combo, const char** elements)
{
  int i;
  
  for (i = 0; elements[i] != NULL; i++) {
    ctk_combo_box_text_append_text (combo, elements[i]);
  }
  
  ctk_combo_box_set_active (CTK_COMBO_BOX (combo), 0);
}

static void
combo_changed_cb (GtkComboBoxText *combo,
		  gpointer user_data)
{
  gint active = ctk_combo_box_get_active (CTK_COMBO_BOX (combo));

  ctk_button_box_set_layout (CTK_BUTTON_BOX (bbox),
			     (GtkButtonBoxStyle) (active + 1));
}

static void
reparent_widget (GtkWidget *widget,
		 GtkWidget *old_parent,
		 GtkWidget *new_parent)
{
  g_object_ref (widget);
  ctk_container_remove (CTK_CONTAINER (old_parent), widget);
  ctk_container_add (CTK_CONTAINER (new_parent), widget);
  g_object_unref (widget);
}

static void
combo_types_changed_cb (GtkComboBoxText *combo,
			GtkWidget **buttons)
{
  int i;
  GtkWidget *old_parent, *new_parent;
  GtkButtonBoxStyle style;

  gint active = ctk_combo_box_get_active (CTK_COMBO_BOX (combo));

  if (active == CTK_ORIENTATION_HORIZONTAL) {
    old_parent = vbbox;
    new_parent = hbbox;
  } else {
    old_parent = hbbox;
    new_parent = vbbox;
  }

  bbox = new_parent;

  for (i = 0; i < N_BUTTONS; i++) {
    reparent_widget (buttons[i], old_parent, new_parent);
  }
  
  ctk_widget_hide (old_parent);
  style = ctk_button_box_get_layout (CTK_BUTTON_BOX (old_parent));
  ctk_button_box_set_layout (CTK_BUTTON_BOX (new_parent), style);
  ctk_widget_show (new_parent);
}

static void
option_cb (GtkToggleButton *option,
	   GtkWidget *button)
{
  gboolean active = ctk_toggle_button_get_active (option);
  
  ctk_button_box_set_child_secondary (CTK_BUTTON_BOX (bbox),
				      button, active);
}

static const char* strings[] = { "Ok", "Cancel", "Help" };

int
main (int    argc,
      char **argv)
{
  GtkWidget *window, *buttons[N_BUTTONS];
  GtkWidget *vbox, *hbox, *combo_styles, *combo_types, *option;
  int i;
  
  ctk_init (&argc, &argv);
  
  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  g_signal_connect (G_OBJECT (window), "delete-event", G_CALLBACK (ctk_main_quit), NULL);
  
  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  ctk_container_add (CTK_CONTAINER (window), vbox);
  
  /* GtkHButtonBox */
  
  hbbox = ctk_button_box_new (CTK_ORIENTATION_HORIZONTAL);
  ctk_box_pack_start (CTK_BOX (vbox), hbbox, TRUE, TRUE, 5);
  
  for (i = 0; i < N_BUTTONS; i++) {
    buttons[i] = ctk_button_new_with_label (strings[i]);
    ctk_container_add (CTK_CONTAINER (hbbox), buttons[i]);
  }
  
  bbox = hbbox;

  ctk_button_box_set_layout (CTK_BUTTON_BOX (hbbox), CTK_BUTTONBOX_SPREAD);
  
  /* GtkVButtonBox */
  vbbox = ctk_button_box_new (CTK_ORIENTATION_VERTICAL);
  ctk_box_pack_start (CTK_BOX (vbox), vbbox, TRUE, TRUE, 5);
  
  /* Options */
  
  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
  ctk_box_pack_start (CTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  
  combo_types = ctk_combo_box_text_new ();
  populate_combo_with (CTK_COMBO_BOX_TEXT (combo_types), types);
  g_signal_connect (G_OBJECT (combo_types), "changed", G_CALLBACK (combo_types_changed_cb), buttons);
  ctk_box_pack_start (CTK_BOX (hbox), combo_types, TRUE, TRUE, 0);
  
  combo_styles = ctk_combo_box_text_new ();
  populate_combo_with (CTK_COMBO_BOX_TEXT (combo_styles), styles);
  g_signal_connect (G_OBJECT (combo_styles), "changed", G_CALLBACK (combo_changed_cb), NULL);
  ctk_box_pack_start (CTK_BOX (hbox), combo_styles, TRUE, TRUE, 0);
  
  option = ctk_check_button_new_with_label ("Help is secondary");
  g_signal_connect (G_OBJECT (option), "toggled", G_CALLBACK (option_cb), buttons[N_BUTTONS - 1]);
  
  ctk_box_pack_start (CTK_BOX (hbox), option, FALSE, FALSE, 0);
  
  ctk_widget_show_all (window);
  ctk_widget_hide (vbbox);
  
  ctk_main ();
  
  return 0;
}
