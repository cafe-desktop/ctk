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

#include <ctk/ctk.h>

static char *baseline_pos_str[] = {
  "BASELINE_POSITION_TOP",
  "BASELINE_POSITION_CENTER",
  "BASELINE_POSITION_BOTTOM"
};

static void
baseline_row_value_changed (CtkSpinButton *spin_button,
			    CtkGrid *grid)
{
  gint row = ctk_spin_button_get_value_as_int (spin_button);

  ctk_grid_set_baseline_row (grid, row);
}

static void
homogeneous_changed (CtkToggleButton *toggle_button,
		    CtkGrid *grid)
{
  ctk_grid_set_row_homogeneous (grid, ctk_toggle_button_get_active (toggle_button));
}

static void
baseline_position_changed (CtkComboBox *combo,
			   CtkBox *hbox)
{
  int i = ctk_combo_box_get_active (combo);

  ctk_box_set_baseline_position (hbox, i);
}

static void
image_size_value_changed (CtkSpinButton *spin_button,
			  CtkImage *image)
{
  gint size = ctk_spin_button_get_value_as_int (spin_button);

  ctk_image_set_pixel_size (CTK_IMAGE (image), size);
}

static void
set_font_size (CtkWidget *widget, gint size)
{
  const gchar *class[3] = { "small-font", "medium-font", "large-font" };

  ctk_style_context_add_class (ctk_widget_get_style_context (widget), class[size]);
}

int
main (int    argc,
      char **argv)
{
  CtkWidget *window, *label, *entry, *button, *grid, *notebook;
  CtkWidget *vbox, *hbox, *grid_hbox, *spin, *spin2, *toggle, *combo, *image, *ebox;
  CtkAdjustment *adjustment;
  int i, j;
  CtkCssProvider *provider;

  ctk_init (&argc, &argv);

  provider = ctk_css_provider_new ();
  ctk_css_provider_load_from_data (provider,
    ".small-font { font-size: 5px; }"
    ".medium-font { font-size: 10px; }"
    ".large-font { font-size: 15px; }", -1, NULL);
  ctk_style_context_add_provider_for_screen (gdk_screen_get_default (),
                                             CTK_STYLE_PROVIDER (provider),
                                             CTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_object_unref (provider);
  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  g_signal_connect (G_OBJECT (window), "delete-event", G_CALLBACK (ctk_main_quit), NULL);

  notebook = ctk_notebook_new ();
  ctk_container_add (CTK_CONTAINER (window), notebook);

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  ctk_notebook_append_page (CTK_NOTEBOOK (notebook),
			    vbox, ctk_label_new ("hboxes"));

  for (j = 0; j < 2; j++)
    {
      char *aligns_names[] = { "FILL", "BASELINE" };
      CtkAlign aligns[] = { CTK_ALIGN_FILL, CTK_ALIGN_BASELINE};

      hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 10);
      ctk_box_pack_start (CTK_BOX (vbox), hbox, FALSE, FALSE, 5);

      label = ctk_label_new (aligns_names[j]);
      ctk_container_add (CTK_CONTAINER (hbox), label);

      for (i = 0; i < 3; i++) {
	label = ctk_label_new ("│XYyj,Ö...");

        set_font_size (label, i);

	ctk_widget_set_valign (label, aligns[j]);

	ctk_container_add (CTK_CONTAINER (hbox), label);
      }

      for (i = 0; i < 3; i++) {
	entry = ctk_entry_new ();
	ctk_entry_set_text (CTK_ENTRY (entry), "│XYyj,Ö...");

        set_font_size (entry, i);

	ctk_widget_set_valign (entry, aligns[j]);

	ctk_container_add (CTK_CONTAINER (hbox), entry);
      }

      spin = ctk_spin_button_new (NULL, 0, 1);
      ctk_orientable_set_orientation (CTK_ORIENTABLE (spin), CTK_ORIENTATION_VERTICAL);
      ctk_widget_set_valign (spin, aligns[j]);
      ctk_container_add (CTK_CONTAINER (hbox), spin);
    }

  grid_hbox = hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 10);
  ctk_box_pack_start (CTK_BOX (vbox), hbox, TRUE, TRUE, 5);

  combo = ctk_combo_box_text_new ();
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo), baseline_pos_str[0]);
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo), baseline_pos_str[1]);
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo), baseline_pos_str[2]);
  ctk_combo_box_set_active (CTK_COMBO_BOX (combo), 1);
  ctk_container_add (CTK_CONTAINER (hbox), combo);

  for (j = 0; j < 2; j++)
    {
      hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 10);
      ctk_box_pack_start (CTK_BOX (vbox), hbox, TRUE, TRUE, 5);

      g_signal_connect (G_OBJECT (combo), "changed",
			G_CALLBACK (baseline_position_changed), hbox);

      if (j == 0)
	label = ctk_label_new ("Baseline:");
      else
	label = ctk_label_new ("Normal:");
      ctk_container_add (CTK_CONTAINER (hbox), label);

      for (i = 0; i < 3; i++)
	{
	  button = ctk_button_new_with_label ("│Xyj,Ö");

          set_font_size (button, i);

	  if (j == 0)
	    ctk_widget_set_valign (button, CTK_ALIGN_BASELINE);

	  ctk_container_add (CTK_CONTAINER (hbox), button);
	}

      for (i = 0; i < 3; i++)
	{
	  button = ctk_button_new_with_label ("│Xyj,Ö");

	  ctk_button_set_image (CTK_BUTTON (button),
				ctk_image_new_from_icon_name ("face-sad", CTK_ICON_SIZE_BUTTON));
	  ctk_button_set_always_show_image (CTK_BUTTON (button), TRUE);

          set_font_size (button, i);

	  if (j == 0)
	    ctk_widget_set_valign (button, CTK_ALIGN_BASELINE);

	  ctk_container_add (CTK_CONTAINER (hbox), button);
	}

      ebox = ctk_event_box_new ();
      if (j == 0)
	ctk_widget_set_valign (ebox, CTK_ALIGN_BASELINE);
      ctk_container_add (CTK_CONTAINER (hbox), ebox);

      image = ctk_image_new_from_icon_name ("face-sad", CTK_ICON_SIZE_BUTTON);
      ctk_image_set_pixel_size (CTK_IMAGE (image), 34);
      if (j == 0)
	ctk_widget_set_valign (image, CTK_ALIGN_BASELINE);
      ctk_container_add (CTK_CONTAINER (ebox), image);

      button = ctk_toggle_button_new_with_label ("│Xyj,Ö");
      if (j == 0)
	ctk_widget_set_valign (button, CTK_ALIGN_BASELINE);
      ctk_container_add (CTK_CONTAINER (hbox), button);

      button = ctk_toggle_button_new_with_label ("│Xyj,Ö");
      ctk_toggle_button_set_mode (CTK_TOGGLE_BUTTON (button), TRUE);
      if (j == 0)
	ctk_widget_set_valign (button, CTK_ALIGN_BASELINE);
      ctk_container_add (CTK_CONTAINER (hbox), button);

      button = ctk_check_button_new_with_label ("│Xyj,Ö");
      if (j == 0)
	ctk_widget_set_valign (button, CTK_ALIGN_BASELINE);
      ctk_container_add (CTK_CONTAINER (hbox), button);

      button = ctk_radio_button_new_with_label (NULL, "│Xyj,Ö");
      if (j == 0)
	ctk_widget_set_valign (button, CTK_ALIGN_BASELINE);
      ctk_container_add (CTK_CONTAINER (hbox), button);
    }


  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  ctk_notebook_append_page (CTK_NOTEBOOK (notebook),
			    vbox, ctk_label_new ("grid"));

  grid_hbox = hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 10);
  ctk_box_pack_start (CTK_BOX (vbox), hbox, TRUE, TRUE, 5);

  label = ctk_label_new ("Align me:");
  ctk_widget_set_valign (label, CTK_ALIGN_BASELINE);

  ctk_container_add (CTK_CONTAINER (hbox), label);

  grid = ctk_grid_new ();
  ctk_widget_set_valign (grid, CTK_ALIGN_BASELINE);
  ctk_grid_set_column_spacing (CTK_GRID (grid), 8);
  ctk_grid_set_row_spacing (CTK_GRID (grid), 8);

  for (j = 0; j < 4; j++)
    {
      char *labels[] = { "Normal:", "Baseline (top):", "Baseline (center):", "Baseline (bottom):"};
      label = ctk_label_new (labels[j]);

      ctk_grid_attach (CTK_GRID (grid),
		       label,
		       0, j,
		       1, 1);
      ctk_widget_set_vexpand (label, TRUE);

      if (j != 0)
	ctk_grid_set_row_baseline_position (CTK_GRID (grid),
					    j, (CtkBaselinePosition)(j-1));

      for (i = 0; i < 3; i++)
	{
	  label = ctk_label_new ("Xyjg,Ö.");

          set_font_size (label, i);

	  if (j != 0)
	    ctk_widget_set_valign (label, CTK_ALIGN_BASELINE);

	  ctk_grid_attach (CTK_GRID (grid),
			   label,
			   i+1, j,
			   1, 1);
	}

      for (i = 0; i < 3; i++)
	{
	  button = ctk_button_new_with_label ("│Xyj,Ö");

	  ctk_button_set_image (CTK_BUTTON (button),
				ctk_image_new_from_icon_name ("face-sad", CTK_ICON_SIZE_BUTTON));
	  ctk_button_set_always_show_image (CTK_BUTTON (button), TRUE);

          set_font_size (button, i);

	  if (j != 0)
	    ctk_widget_set_valign (button, CTK_ALIGN_BASELINE);

	  ctk_grid_attach (CTK_GRID (grid),
			   button,
			   i+4, j,
			   1, 1);
	}

    }

  ctk_container_add (CTK_CONTAINER (hbox), grid);

  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 10);
  ctk_box_pack_start (CTK_BOX (vbox), hbox, FALSE, FALSE, 5);

  adjustment = ctk_adjustment_new (0.0, -1.0, 5.0, 1.0, 1.0, 0.0);
  spin = ctk_spin_button_new (adjustment, 1.0, 0);
  g_signal_connect (spin, "value-changed", (GCallback)baseline_row_value_changed, grid);
  ctk_container_add (CTK_CONTAINER (hbox), spin);

  toggle = ctk_toggle_button_new_with_label ("Homogeneous");
  g_signal_connect (toggle, "toggled", (GCallback)homogeneous_changed, grid);
  ctk_container_add (CTK_CONTAINER (hbox), toggle);

  combo = ctk_combo_box_text_new ();
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo), baseline_pos_str[0]);
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo), baseline_pos_str[1]);
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo), baseline_pos_str[2]);
  ctk_combo_box_set_active (CTK_COMBO_BOX (combo), 1);
  g_signal_connect (G_OBJECT (combo), "changed",
		    G_CALLBACK (baseline_position_changed), grid_hbox);
  ctk_container_add (CTK_CONTAINER (hbox), combo);

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  ctk_notebook_append_page (CTK_NOTEBOOK (notebook),
			    vbox, ctk_label_new ("button box"));

  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 10);
  ctk_box_pack_start (CTK_BOX (vbox), hbox, FALSE, FALSE, 5);

  adjustment = ctk_adjustment_new (34.0, 1.0, 64.0, 1.0, 1.0, 0.0);
  spin = ctk_spin_button_new (adjustment, 1.0, 0);
  ctk_container_add (CTK_CONTAINER (hbox), spin);

  adjustment = ctk_adjustment_new (16.0, 1.0, 64.0, 1.0, 1.0, 0.0);
  spin2 = ctk_spin_button_new (adjustment, 1.0, 0);
  ctk_container_add (CTK_CONTAINER (hbox), spin2);

  for (j = 0; j < 3; j++)
    {
      hbox = ctk_button_box_new (CTK_ORIENTATION_HORIZONTAL);
      ctk_box_pack_start (CTK_BOX (vbox), hbox, TRUE, TRUE, 5);

      ctk_box_set_baseline_position (CTK_BOX (hbox), j);

      label = ctk_label_new (baseline_pos_str[j]);
      ctk_container_add (CTK_CONTAINER (hbox), label);
      ctk_widget_set_vexpand (label, TRUE);

      image = ctk_image_new_from_icon_name ("face-sad", CTK_ICON_SIZE_BUTTON);
      ctk_image_set_pixel_size (CTK_IMAGE (image), 34);
      ctk_container_add (CTK_CONTAINER (hbox), image);

      g_signal_connect (spin, "value-changed", (GCallback)image_size_value_changed, image);

      for (i = 0; i < 3; i++)
	{
	  button = ctk_button_new_with_label ("│Xyj,Ö");

          set_font_size (button, i);

	  if (i != 0)
	    ctk_widget_set_valign (button, CTK_ALIGN_BASELINE);

	  ctk_container_add (CTK_CONTAINER (hbox), button);
	}

      for (i = 0; i < 3; i++)
	{
	  button = ctk_button_new_with_label ("│Xyj,Ö");

	  image = ctk_image_new_from_icon_name ("face-sad", CTK_ICON_SIZE_BUTTON);
	  ctk_image_set_pixel_size (CTK_IMAGE (image), 16);
	  ctk_button_set_image (CTK_BUTTON (button), image);
	  if (i == 0)
	    g_signal_connect (spin2, "value-changed", (GCallback)image_size_value_changed, image);
	  ctk_button_set_always_show_image (CTK_BUTTON (button), TRUE);

          set_font_size (button, i);

	  ctk_widget_set_valign (button, CTK_ALIGN_BASELINE);

	  ctk_container_add (CTK_CONTAINER (hbox), button);
	}
    }

  ctk_widget_show_all (window);

  ctk_main ();

  return 0;
}
