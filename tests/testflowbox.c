/*
 * Copyright (C) 2010 Openismus GmbH
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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include <ctk/ctk.h>

enum {
  SIMPLE_ITEMS = 0,
  FOCUS_ITEMS,
  WRAPPY_ITEMS,
  STOCK_ITEMS,
  IMAGE_ITEMS,
  BUTTON_ITEMS
};

#define INITIAL_HALIGN          CTK_ALIGN_FILL
#define INITIAL_VALIGN          CTK_ALIGN_START
#define INITIAL_MINIMUM_LENGTH  3
#define INITIAL_MAXIMUM_LENGTH  6
#define INITIAL_CSPACING        2
#define INITIAL_RSPACING        2
#define N_ITEMS 1000

static CtkFlowBox    *the_flowbox       = NULL;
static gint           items_type       = SIMPLE_ITEMS;
static CtkOrientation text_orientation = CTK_ORIENTATION_HORIZONTAL;

static void
populate_flowbox_simple (CtkFlowBox *flowbox)
{
  gint i;

  for (i = 0; i < N_ITEMS; i++)
    {
      CtkWidget *widget, *frame;

      gchar *text = g_strdup_printf ("Item %02d", i);

      widget = ctk_label_new (text);
      frame  = ctk_frame_new (NULL);
      ctk_widget_show (widget);
      ctk_widget_show (frame);

      ctk_container_add (CTK_CONTAINER (frame), widget);

      if (text_orientation == CTK_ORIENTATION_VERTICAL)
        ctk_label_set_angle (CTK_LABEL (widget), 90);
      g_object_set_data_full (G_OBJECT (frame), "id", (gpointer)g_strdup (text), g_free);
      ctk_container_add (CTK_CONTAINER (flowbox), frame);

      g_free (text);
    }
}

static void
populate_flowbox_focus (CtkFlowBox *flowbox)
{
  gint i;

  for (i = 0; i < 200; i++)
    {
      CtkWidget *widget, *frame, *box;
      gboolean sensitive;

      sensitive = TRUE;
      frame = ctk_frame_new (NULL);
      ctk_frame_set_shadow_type (CTK_FRAME (frame), CTK_SHADOW_NONE);

      box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 6);
      ctk_container_add (CTK_CONTAINER (frame), box);

      widget = ctk_label_new ("Label");
      ctk_container_add (CTK_CONTAINER (box), widget);

      switch (i % 4)
        {
        case 0:
          widget = ctk_entry_new ();
          break;
        case 1:
          widget = ctk_button_new_with_label ("Button");
          break;
        case 2:
          widget = ctk_label_new ("bla");
          break;
        case 3:
          widget = ctk_label_new ("bla");
          sensitive = FALSE;
          break;
        }

      ctk_container_add (CTK_CONTAINER (box), widget);

      if (i % 5 == 0)
        ctk_container_add (CTK_CONTAINER (box), ctk_switch_new ());

      ctk_widget_show_all (frame);

      ctk_container_add (CTK_CONTAINER (flowbox), frame);
      if (!sensitive)
        ctk_widget_set_sensitive (ctk_widget_get_parent (frame), FALSE);
    }
}

static void
populate_flowbox_buttons (CtkFlowBox *flowbox)
{
  gint i;

  for (i = 0; i < 50; i++)
    {
      CtkWidget *widget;

      widget = ctk_button_new_with_label ("Button");
      ctk_widget_show (widget);
      ctk_container_add (CTK_CONTAINER (flowbox), widget);
      widget = ctk_widget_get_parent (widget);
      ctk_widget_set_can_focus (widget, FALSE);
    }
}

static void
populate_flowbox_wrappy (CtkFlowBox *flowbox)
{
  gint i;

  const gchar *strings[] = {
    "These are", "some wrappy label", "texts", "of various", "lengths.",
    "They should always be", "shown", "consecutively. Except it's",
    "hard to say", "where exactly the", "label", "will wrap", "and where exactly",
    "the actual", "container", "will wrap.", "This label is really really really long !",
    "Let's add some more", "labels to the",
    "mix. Just to", "make sure we", "got something to work", "with here."
  };

  for (i = 0; i < G_N_ELEMENTS (strings); i++)
    {
      CtkWidget *widget, *frame;

      widget = ctk_label_new (strings[i]);
      frame  = ctk_frame_new (NULL);
      ctk_widget_show (widget);
      ctk_widget_show (frame);

      if (text_orientation == CTK_ORIENTATION_VERTICAL)
        ctk_label_set_angle (CTK_LABEL (widget), 90);

      ctk_container_add (CTK_CONTAINER (frame), widget);

      ctk_label_set_line_wrap (CTK_LABEL (widget), TRUE);
      ctk_label_set_line_wrap_mode (CTK_LABEL (widget), PANGO_WRAP_WORD);
      ctk_label_set_width_chars (CTK_LABEL (widget), 10);
      g_object_set_data_full (G_OBJECT (frame), "id", (gpointer)g_strdup (strings[i]), g_free);

      ctk_container_add (CTK_CONTAINER (flowbox), frame);
    }
}

static void
populate_flowbox_stock (CtkFlowBox *flowbox)
{
  static GSList *stock_ids = NULL;
  GSList *l;
  gint i;

  if (!stock_ids)
    {
      stock_ids = ctk_stock_list_ids ();
    }

  for (i = 0, l = stock_ids; i < 30 && l != NULL; i++, l = l->next)
    {
      CtkWidget *widget;

      gchar *stock_id = l->data;
      gchar *text = g_strdup_printf ("Item %02d", i);

      widget = ctk_button_new_from_stock (stock_id);

      ctk_widget_show (widget);

      g_object_set_data_full (G_OBJECT (widget), "id", (gpointer)g_strdup (text), g_free);
      ctk_container_add (CTK_CONTAINER (flowbox), widget);
    }
}

static void
populate_flowbox_images (CtkFlowBox *flowbox)
{
  gint i;

  for (i = 0; i < N_ITEMS; i++)
    {
      CtkWidget *widget, *image, *label;

      gchar *text = g_strdup_printf ("Item %02d", i);

      widget = ctk_box_new (CTK_ORIENTATION_VERTICAL, 6);
      ctk_widget_set_hexpand (widget, TRUE);

      image = ctk_image_new_from_icon_name ("face-wink", CTK_ICON_SIZE_DIALOG);
      ctk_widget_set_hexpand (image, TRUE);
      ctk_image_set_pixel_size (CTK_IMAGE (image), 256);

      label = ctk_label_new (text);

      ctk_container_add (CTK_CONTAINER (widget), image);
      ctk_container_add (CTK_CONTAINER (widget), label);
      ctk_widget_show_all (widget);

      if (text_orientation == CTK_ORIENTATION_VERTICAL)
        ctk_label_set_angle (CTK_LABEL (widget), 90);

      g_object_set_data_full (G_OBJECT (widget), "id", (gpointer)g_strdup (text), g_free);
      ctk_container_add (CTK_CONTAINER (flowbox), widget);

      g_free (text);
    }
}

static void
populate_items (CtkFlowBox *flowbox)
{
  GList *children, *l;

  /* Remove all children first */
  children = ctk_container_get_children (CTK_CONTAINER (flowbox));
  for (l = children; l; l = l->next)
    {
      CtkWidget *child = l->data;

      ctk_container_remove (CTK_CONTAINER (flowbox), child);
    }
  g_list_free (children);

  if (items_type == SIMPLE_ITEMS)
    populate_flowbox_simple (flowbox);
  else if (items_type == FOCUS_ITEMS)
    populate_flowbox_focus (flowbox);
  else if (items_type == WRAPPY_ITEMS)
    populate_flowbox_wrappy (flowbox);
  else if (items_type == STOCK_ITEMS)
    populate_flowbox_stock (flowbox);
  else if (items_type == IMAGE_ITEMS)
    populate_flowbox_images (flowbox);
  else if (items_type == BUTTON_ITEMS)
    populate_flowbox_buttons (flowbox);
}

static void
horizontal_alignment_changed (CtkComboBox   *box,
                              CtkFlowBox    *flowbox)
{
  CtkAlign alignment = ctk_combo_box_get_active (box);

  ctk_widget_set_halign (CTK_WIDGET (flowbox), alignment);
}

static void
vertical_alignment_changed (CtkComboBox   *box,
                            CtkFlowBox    *flowbox)
{
  CtkAlign alignment = ctk_combo_box_get_active (box);

  ctk_widget_set_valign (CTK_WIDGET (flowbox), alignment);
}

static void
orientation_changed (CtkComboBox   *box,
                     CtkFlowBox *flowbox)
{
  CtkOrientation orientation = ctk_combo_box_get_active (box);

  ctk_orientable_set_orientation (CTK_ORIENTABLE (flowbox), orientation);
}

static void
selection_mode_changed (CtkComboBox *box,
                        CtkFlowBox  *flowbox)
{
  CtkSelectionMode mode = ctk_combo_box_get_active (box);

  ctk_flow_box_set_selection_mode (flowbox, mode);
}

static void
line_length_changed (CtkSpinButton *spin,
                     CtkFlowBox *flowbox)
{
  gint length = ctk_spin_button_get_value_as_int (spin);

  ctk_flow_box_set_min_children_per_line (flowbox, length);
}

static void
max_line_length_changed (CtkSpinButton *spin,
                         CtkFlowBox *flowbox)
{
  gint length = ctk_spin_button_get_value_as_int (spin);

  ctk_flow_box_set_max_children_per_line (flowbox, length);
}

static void
spacing_changed (CtkSpinButton *button,
                 gpointer       data)
{
  CtkOrientation orientation = GPOINTER_TO_INT (data);
  gint           state = ctk_spin_button_get_value_as_int (button);

  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    ctk_flow_box_set_column_spacing (the_flowbox, state);
  else
    ctk_flow_box_set_row_spacing (the_flowbox, state);
}

static void
items_changed (CtkComboBox   *box,
               CtkFlowBox *flowbox)
{
  items_type = ctk_combo_box_get_active (box);

  populate_items (flowbox);
}

static void
text_orientation_changed (CtkComboBox   *box,
                          CtkFlowBox *flowbox)
{
  text_orientation = ctk_combo_box_get_active (box);

  populate_items (flowbox);
}

static void
homogeneous_toggled (CtkToggleButton *button,
                     CtkFlowBox      *flowbox)
{
  gboolean state = ctk_toggle_button_get_active (button);

  ctk_flow_box_set_homogeneous (flowbox, state);
}

static void
on_child_activated (CtkFlowBox *self,
                    CtkWidget  *child)
{
  const char *id;
  id = g_object_get_data (G_OBJECT (ctk_bin_get_child (CTK_BIN (child))), "id");
  g_message ("Child activated %p: %s", child, id);
}

static G_GNUC_UNUSED void
selection_foreach (CtkFlowBox      *self,
                   CtkFlowBoxChild *child_info,
                   gpointer         data)
{
  const char *id;
  CtkWidget *child;

  child = ctk_bin_get_child (CTK_BIN (child_info));
  id = g_object_get_data (G_OBJECT (child), "id");
  g_message ("Child selected %p: %s", child, id);
}

static void
on_selected_children_changed (CtkFlowBox *self)
{
  g_message ("Selection changed");
  //ctk_flow_box_selected_foreach (self, selection_foreach, NULL);
}

static gboolean
filter_func (CtkFlowBoxChild *child, gpointer user_data)
{
  gint index;

  index = ctk_flow_box_child_get_index (child);

  return (index % 3) == 0;
}

static void
filter_toggled (CtkToggleButton *button,
                CtkFlowBox      *flowbox)
{
  gboolean state = ctk_toggle_button_get_active (button);

  if (state)
    ctk_flow_box_set_filter_func (flowbox, filter_func, NULL, NULL);
  else
    ctk_flow_box_set_filter_func (flowbox, NULL, NULL, NULL);
}

static gint
sort_func (CtkFlowBoxChild *a,
           CtkFlowBoxChild *b,
           gpointer         data)
{
  gchar *ida, *idb;

  ida = (gchar *)g_object_get_data (G_OBJECT (ctk_bin_get_child (CTK_BIN (a))), "id");
  idb = (gchar *)g_object_get_data (G_OBJECT (ctk_bin_get_child (CTK_BIN (b))), "id");
  return g_strcmp0 (ida, idb);
}

static void
sort_toggled (CtkToggleButton *button,
              CtkFlowBox      *flowbox)
{
  gboolean state = ctk_toggle_button_get_active (button);

  if (state)
    ctk_flow_box_set_sort_func (flowbox, sort_func, NULL, NULL);
  else
    ctk_flow_box_set_sort_func (flowbox, NULL, NULL, NULL);
}

static CtkWidget *
create_window (void)
{
  CtkWidget *window, *hbox, *vbox, *flowbox_cntl, *items_cntl;
  CtkWidget *flowbox, *widget, *expander, *swindow;

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  hbox   = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 6);
  vbox   = ctk_box_new (CTK_ORIENTATION_VERTICAL, 6);

  ctk_container_set_border_width (CTK_CONTAINER (window), 8);

  ctk_widget_show (vbox);
  ctk_widget_show (hbox);
  ctk_container_add (CTK_CONTAINER (window), hbox);
  ctk_box_pack_start (CTK_BOX (hbox), vbox, FALSE, FALSE, 0);

  swindow = ctk_scrolled_window_new (NULL, NULL);
  ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (swindow),
                                  CTK_POLICY_AUTOMATIC, CTK_POLICY_AUTOMATIC);

  ctk_widget_show (swindow);
  ctk_box_pack_start (CTK_BOX (hbox), swindow, TRUE, TRUE, 0);

  flowbox = ctk_flow_box_new ();
  ctk_widget_set_halign (flowbox, CTK_ALIGN_END);
  the_flowbox = (CtkFlowBox *)flowbox;
  ctk_widget_set_halign (flowbox, INITIAL_HALIGN);
  ctk_widget_set_valign (flowbox, INITIAL_VALIGN);
  ctk_flow_box_set_column_spacing (CTK_FLOW_BOX (flowbox), INITIAL_CSPACING);
  ctk_flow_box_set_row_spacing (CTK_FLOW_BOX (flowbox), INITIAL_RSPACING);
  ctk_flow_box_set_min_children_per_line (CTK_FLOW_BOX (flowbox), INITIAL_MINIMUM_LENGTH);
  ctk_flow_box_set_max_children_per_line (CTK_FLOW_BOX (flowbox), INITIAL_MAXIMUM_LENGTH);
  ctk_widget_show (flowbox);
  ctk_container_add (CTK_CONTAINER (swindow), flowbox);

  ctk_flow_box_set_hadjustment (CTK_FLOW_BOX (flowbox),
                                ctk_scrolled_window_get_hadjustment (CTK_SCROLLED_WINDOW (swindow)));
  ctk_flow_box_set_vadjustment (CTK_FLOW_BOX (flowbox),
                                ctk_scrolled_window_get_vadjustment (CTK_SCROLLED_WINDOW (swindow)));

  g_signal_connect (flowbox, "child-activated", G_CALLBACK (on_child_activated), NULL);
  g_signal_connect (flowbox, "selected-children-changed", G_CALLBACK (on_selected_children_changed), NULL);

  /* Add Flowbox test control frame */
  expander = ctk_expander_new ("Flow Box controls");
  ctk_expander_set_expanded (CTK_EXPANDER (expander), TRUE);
  flowbox_cntl = ctk_box_new (CTK_ORIENTATION_VERTICAL, 2);
  ctk_widget_show (flowbox_cntl);
  ctk_widget_show (expander);
  ctk_container_add (CTK_CONTAINER (expander), flowbox_cntl);
  ctk_box_pack_start (CTK_BOX (vbox), expander, FALSE, FALSE, 0);

  widget = ctk_check_button_new_with_label ("Homogeneous");
  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (widget), FALSE);
  ctk_widget_show (widget);

  ctk_widget_set_tooltip_text (widget, "Set whether the items should be displayed at the same size");
  ctk_box_pack_start (CTK_BOX (flowbox_cntl), widget, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (widget), "toggled",
                    G_CALLBACK (homogeneous_toggled), flowbox);

  widget = ctk_check_button_new_with_label ("Activate on single click");
  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (widget), FALSE);
  g_object_bind_property (widget, "active",
                          flowbox, "activate-on-single-click",
                          G_BINDING_SYNC_CREATE);
  ctk_widget_show (widget);
  ctk_box_pack_start (CTK_BOX (flowbox_cntl), widget, FALSE, FALSE, 0);

  /* Add alignment controls */
  widget = ctk_combo_box_text_new ();
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (widget), "Fill");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (widget), "Start");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (widget), "End");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (widget), "Center");
  ctk_combo_box_set_active (CTK_COMBO_BOX (widget), INITIAL_HALIGN);
  ctk_widget_show (widget);

  ctk_widget_set_tooltip_text (widget, "Set the horizontal alignment policy");
  ctk_box_pack_start (CTK_BOX (flowbox_cntl), widget, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (widget), "changed",
                    G_CALLBACK (horizontal_alignment_changed), flowbox);

  widget = ctk_combo_box_text_new ();
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (widget), "Fill");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (widget), "Start");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (widget), "End");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (widget), "Center");
  ctk_combo_box_set_active (CTK_COMBO_BOX (widget), INITIAL_VALIGN);
  ctk_widget_show (widget);

  ctk_widget_set_tooltip_text (widget, "Set the vertical alignment policy");
  ctk_box_pack_start (CTK_BOX (flowbox_cntl), widget, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (widget), "changed",
                    G_CALLBACK (vertical_alignment_changed), flowbox);

  /* Add Orientation control */
  widget = ctk_combo_box_text_new ();
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (widget), "Horizontal");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (widget), "Vertical");
  ctk_combo_box_set_active (CTK_COMBO_BOX (widget), 0);
  ctk_widget_show (widget);

  ctk_widget_set_tooltip_text (widget, "Set the flowbox orientation");
  ctk_box_pack_start (CTK_BOX (flowbox_cntl), widget, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (widget), "changed",
                    G_CALLBACK (orientation_changed), flowbox);

  /* Add selection mode control */
  widget = ctk_combo_box_text_new ();
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (widget), "None");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (widget), "Single");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (widget), "Browse");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (widget), "Multiple");
  ctk_combo_box_set_active (CTK_COMBO_BOX (widget), 1);
  ctk_widget_show (widget);

  ctk_widget_set_tooltip_text (widget, "Set the selection mode");
  ctk_box_pack_start (CTK_BOX (flowbox_cntl), widget, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (widget), "changed",
                    G_CALLBACK (selection_mode_changed), flowbox);

  /* Add minimum line length in items control */
  widget = ctk_spin_button_new_with_range (1, 10, 1);
  ctk_spin_button_set_value (CTK_SPIN_BUTTON (widget), INITIAL_MINIMUM_LENGTH);
  ctk_widget_show (widget);

  ctk_widget_set_tooltip_text (widget, "Set the minimum amount of items per line before wrapping");
  ctk_box_pack_start (CTK_BOX (flowbox_cntl), widget, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (widget), "changed",
                    G_CALLBACK (line_length_changed), flowbox);
  g_signal_connect (G_OBJECT (widget), "value-changed",
                    G_CALLBACK (line_length_changed), flowbox);

  /* Add natural line length in items control */
  widget = ctk_spin_button_new_with_range (1, 10, 1);
  ctk_spin_button_set_value (CTK_SPIN_BUTTON (widget), INITIAL_MAXIMUM_LENGTH);
  ctk_widget_show (widget);

  ctk_widget_set_tooltip_text (widget, "Set the natural amount of items per line ");
  ctk_box_pack_start (CTK_BOX (flowbox_cntl), widget, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (widget), "changed",
                    G_CALLBACK (max_line_length_changed), flowbox);
  g_signal_connect (G_OBJECT (widget), "value-changed",
                    G_CALLBACK (max_line_length_changed), flowbox);

  /* Add horizontal/vertical spacing controls */
  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 2);
  ctk_widget_show (hbox);

  widget = ctk_label_new ("H Spacing");
  ctk_widget_show (widget);
  ctk_box_pack_start (CTK_BOX (hbox), widget, TRUE, TRUE, 0);

  widget = ctk_spin_button_new_with_range (0, 30, 1);
  ctk_spin_button_set_value (CTK_SPIN_BUTTON (widget), INITIAL_CSPACING);
  ctk_widget_show (widget);

  ctk_widget_set_tooltip_text (widget, "Set the horizontal spacing between children");
  ctk_box_pack_start (CTK_BOX (hbox), widget, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (widget), "changed",
                    G_CALLBACK (spacing_changed), GINT_TO_POINTER (CTK_ORIENTATION_HORIZONTAL));
  g_signal_connect (G_OBJECT (widget), "value-changed",
                    G_CALLBACK (spacing_changed), GINT_TO_POINTER (CTK_ORIENTATION_HORIZONTAL));

  ctk_box_pack_start (CTK_BOX (flowbox_cntl), hbox, FALSE, FALSE, 0);

  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 2);
  ctk_widget_show (hbox);

  widget = ctk_label_new ("V Spacing");
  ctk_widget_show (widget);
  ctk_box_pack_start (CTK_BOX (hbox), widget, TRUE, TRUE, 0);

  widget = ctk_spin_button_new_with_range (0, 30, 1);
  ctk_spin_button_set_value (CTK_SPIN_BUTTON (widget), INITIAL_RSPACING);
  ctk_widget_show (widget);

  ctk_widget_set_tooltip_text (widget, "Set the vertical spacing between children");
  ctk_box_pack_start (CTK_BOX (hbox), widget, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (widget), "changed",
                    G_CALLBACK (spacing_changed), GINT_TO_POINTER (CTK_ORIENTATION_VERTICAL));
  g_signal_connect (G_OBJECT (widget), "value-changed",
                    G_CALLBACK (spacing_changed), GINT_TO_POINTER (CTK_ORIENTATION_VERTICAL));

  ctk_box_pack_start (CTK_BOX (flowbox_cntl), hbox, FALSE, FALSE, 0);

  /* filtering and sorting */

  widget = ctk_check_button_new_with_label ("Filter");
  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (widget), FALSE);
  ctk_widget_show (widget);

  ctk_widget_set_tooltip_text (widget, "Set whether some items should be filtered out");
  ctk_box_pack_start (CTK_BOX (flowbox_cntl), widget, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (widget), "toggled",
                    G_CALLBACK (filter_toggled), flowbox);

  widget = ctk_check_button_new_with_label ("Sort");
  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (widget), FALSE);
  ctk_widget_show (widget);

  ctk_widget_set_tooltip_text (widget, "Set whether items should be sorted");
  ctk_box_pack_start (CTK_BOX (flowbox_cntl), widget, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (widget), "toggled",
                    G_CALLBACK (sort_toggled), flowbox);


  /* Add test items control frame */
  expander = ctk_expander_new ("Test item controls");
  ctk_expander_set_expanded (CTK_EXPANDER (expander), TRUE);
  items_cntl = ctk_box_new (CTK_ORIENTATION_VERTICAL, 2);
  ctk_widget_show (items_cntl);
  ctk_widget_show (expander);
  ctk_container_add (CTK_CONTAINER (expander), items_cntl);
  ctk_box_pack_start (CTK_BOX (vbox), expander, FALSE, FALSE, 0);

  /* Add Items control */
  widget = ctk_combo_box_text_new ();
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (widget), "Simple");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (widget), "Focus");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (widget), "Wrappy");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (widget), "Stock");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (widget), "Images");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (widget), "Buttons");
  ctk_combo_box_set_active (CTK_COMBO_BOX (widget), 0);
  ctk_widget_show (widget);

  ctk_widget_set_tooltip_text (widget, "Set the item set to use");
  ctk_box_pack_start (CTK_BOX (items_cntl), widget, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (widget), "changed",
                    G_CALLBACK (items_changed), flowbox);


  /* Add Text Orientation control */
  widget = ctk_combo_box_text_new ();
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (widget), "Horizontal");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (widget), "Vertical");
  ctk_combo_box_set_active (CTK_COMBO_BOX (widget), 0);
  ctk_widget_show (widget);

  ctk_widget_set_tooltip_text (widget, "Set the item's text orientation (cant be done for stock buttons)");
  ctk_box_pack_start (CTK_BOX (items_cntl), widget, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (widget), "changed",
                    G_CALLBACK (text_orientation_changed), flowbox);

  populate_items (CTK_FLOW_BOX (flowbox));

  /* This line was added only for the convenience of reproducing
   * a height-for-width inside CtkScrolledWindow bug (bug 629778).
   *   -Tristan
   */
  ctk_window_set_default_size (CTK_WINDOW (window), 390, -1);

  return window;
}

int
main (int argc, char *argv[])
{
  CtkWidget *window;

  ctk_init (&argc, &argv);

  window = create_window ();

  g_signal_connect (window, "delete-event",
                    G_CALLBACK (ctk_main_quit), window);

  ctk_widget_show (window);

  ctk_main ();

  return 0;
}
