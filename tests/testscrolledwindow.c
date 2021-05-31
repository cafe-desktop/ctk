#include <gtk/gtk.h>


static void
horizontal_policy_changed (GtkComboBox *combo_box,
			   GtkViewport *viewport)
{
  GtkScrollablePolicy policy = ctk_combo_box_get_active (combo_box);

  ctk_scrollable_set_hscroll_policy (CTK_SCROLLABLE (viewport), policy);
}

static void
vertical_policy_changed (GtkComboBox *combo_box,
			 GtkViewport *viewport)
{
  GtkScrollablePolicy policy = ctk_combo_box_get_active (combo_box);

  ctk_scrollable_set_vscroll_policy (CTK_SCROLLABLE (viewport), policy);
}

static void
label_flip_changed (GtkComboBox *combo_box,
		    GtkLabel    *label)
{
  gint active = ctk_combo_box_get_active (combo_box);

  if (active == 0)
    ctk_label_set_angle (label, 0.0);
  else 
    ctk_label_set_angle (label, 90.0);
}

static void
content_width_changed (GtkSpinButton *spin_button,
                       gpointer       data)
{
  GtkScrolledWindow *swindow = data;
  gdouble value;

  value = ctk_spin_button_get_value (spin_button);
  ctk_scrolled_window_set_min_content_width (swindow, (gint)value);
}

static void
content_height_changed (GtkSpinButton *spin_button,
                        gpointer       data)
{
  GtkScrolledWindow *swindow = data;
  gdouble value;

  value = ctk_spin_button_get_value (spin_button);
  ctk_scrolled_window_set_min_content_height (swindow, (gint)value);
}

static void
kinetic_scrolling_changed (GtkToggleButton *toggle_button,
                           gpointer         data)
{
  GtkScrolledWindow *swindow = data;
  gboolean enabled = ctk_toggle_button_get_active (toggle_button);

  ctk_scrolled_window_set_kinetic_scrolling (swindow, enabled);
}

static void
add_row (GtkButton  *button,
         GtkListBox *listbox)
{
  GtkWidget *row;

  row = g_object_new (CTK_TYPE_LIST_BOX_ROW, "border-width", 12, NULL);
  ctk_container_add (CTK_CONTAINER (row), ctk_label_new ("test"));
  ctk_container_add (CTK_CONTAINER (listbox), row);

  ctk_widget_show_all (row);
}

static void
remove_row (GtkButton  *button,
            GtkListBox *listbox)
{
  GList *children, *last;

  children = ctk_container_get_children (CTK_CONTAINER (listbox));
  last = g_list_last (children);

  if (last)
    ctk_container_remove (CTK_CONTAINER (listbox), last->data);

  g_list_free (children);
}

static void
scrollable_policy (void)
{
  GtkWidget *window, *swindow, *hbox, *vbox, *frame, *cntl, *listbox;
  GtkWidget *viewport, *label, *expander, *widget, *popover;

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  hbox   = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 2);
  vbox   = ctk_box_new (CTK_ORIENTATION_VERTICAL, 6);

  ctk_container_set_border_width (CTK_CONTAINER (window), 8);

  ctk_widget_show (vbox);
  ctk_widget_show (hbox);
  ctk_container_add (CTK_CONTAINER (window), hbox);
  ctk_box_pack_start (CTK_BOX (hbox), vbox, FALSE, FALSE, 0);

  frame = ctk_frame_new ("Scrolled Window");
  ctk_widget_show (frame);
  ctk_box_pack_start (CTK_BOX (hbox), frame, TRUE, TRUE, 0);

  swindow = ctk_scrolled_window_new (NULL, NULL);
  ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (swindow),
                                  CTK_POLICY_AUTOMATIC, CTK_POLICY_AUTOMATIC);
  
  ctk_widget_show (swindow);
  ctk_container_add (CTK_CONTAINER (frame), swindow);

  viewport = ctk_viewport_new (NULL, NULL);
  label = ctk_label_new ("Here is a wrapping label with a minimum width-chars of 40 and "
			 "a natural max-width-chars of 100 to demonstrate the usage of "
			 "scrollable widgets \"hscroll-policy\" and \"vscroll-policy\" "
			 "properties. Note also that when playing with the window height, "
			 "one can observe that the vscrollbar disappears as soon as there "
			 "is enough height to fit the content vertically if the window were "
			 "to be allocated a width without a vscrollbar present");

  ctk_label_set_line_wrap  (CTK_LABEL (label), TRUE);
  ctk_label_set_width_chars  (CTK_LABEL (label), 40);
  ctk_label_set_max_width_chars  (CTK_LABEL (label), 100);

  ctk_widget_show (label);
  ctk_widget_show (viewport);
  ctk_container_add (CTK_CONTAINER (viewport), label);
  ctk_container_add (CTK_CONTAINER (swindow), viewport);

  /* Add controls here */
  expander = ctk_expander_new ("Controls");
  ctk_expander_set_expanded (CTK_EXPANDER (expander), TRUE);
  cntl = ctk_box_new (CTK_ORIENTATION_VERTICAL, 2);
  ctk_widget_show (cntl);
  ctk_widget_show (expander);
  ctk_container_add (CTK_CONTAINER (expander), cntl);
  ctk_box_pack_start (CTK_BOX (vbox), expander, FALSE, FALSE, 0);

  /* Add Horizontal policy control here */
  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 2);
  ctk_widget_show (hbox);

  widget = ctk_label_new ("hscroll-policy");
  ctk_widget_show (widget);
  ctk_box_pack_start (CTK_BOX (hbox), widget, TRUE, TRUE, 0);

  widget = ctk_combo_box_text_new ();
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (widget), "Minimum");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (widget), "Natural");
  ctk_combo_box_set_active (CTK_COMBO_BOX (widget), 0);
  ctk_widget_show (widget);

  ctk_box_pack_start (CTK_BOX (hbox), widget, TRUE, TRUE, 0);
  ctk_box_pack_start (CTK_BOX (cntl), hbox, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (widget), "changed",
                    G_CALLBACK (horizontal_policy_changed), viewport);

  /* Add Vertical policy control here */
  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 2);
  ctk_widget_show (hbox);

  widget = ctk_label_new ("vscroll-policy");
  ctk_widget_show (widget);
  ctk_box_pack_start (CTK_BOX (hbox), widget, TRUE, TRUE, 0);

  widget = ctk_combo_box_text_new ();
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (widget), "Minimum");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (widget), "Natural");
  ctk_combo_box_set_active (CTK_COMBO_BOX (widget), 0);
  ctk_widget_show (widget);

  ctk_box_pack_start (CTK_BOX (hbox), widget, TRUE, TRUE, 0);
  ctk_box_pack_start (CTK_BOX (cntl), hbox, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (widget), "changed",
                    G_CALLBACK (vertical_policy_changed), viewport);

  /* Content size controls */
  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 2);

  widget = ctk_label_new ("min-content-width");
  ctk_widget_show (widget);
  ctk_box_pack_start (CTK_BOX (hbox), widget, TRUE, TRUE, 0);

  widget = ctk_spin_button_new_with_range (100.0, 1000.0, 10.0);
  ctk_box_pack_start (CTK_BOX (hbox), widget, TRUE, TRUE, 0);
  ctk_box_pack_start (CTK_BOX (cntl), hbox, FALSE, FALSE, 0);
  ctk_widget_show (widget);
  ctk_widget_show (hbox);

  g_signal_connect (G_OBJECT (widget), "value-changed",
                    G_CALLBACK (content_width_changed), swindow);

  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 2);

  widget = ctk_label_new ("min-content-height");
  ctk_widget_show (widget);
  ctk_box_pack_start (CTK_BOX (hbox), widget, TRUE, TRUE, 0);

  widget = ctk_spin_button_new_with_range (100.0, 1000.0, 10.0);
  ctk_box_pack_start (CTK_BOX (hbox), widget, TRUE, TRUE, 0);
  ctk_box_pack_start (CTK_BOX (cntl), hbox, FALSE, FALSE, 0);
  ctk_widget_show (widget);
  ctk_widget_show (hbox);

  g_signal_connect (G_OBJECT (widget), "value-changed",
                    G_CALLBACK (content_height_changed), swindow);

  /* Add Label orientation control here */
  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 2);
  ctk_widget_show (hbox);

  widget = ctk_label_new ("label-flip");
  ctk_widget_show (widget);
  ctk_box_pack_start (CTK_BOX (hbox), widget, TRUE, TRUE, 0);

  widget = ctk_combo_box_text_new ();
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (widget), "Horizontal");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (widget), "Vertical");
  ctk_combo_box_set_active (CTK_COMBO_BOX (widget), 0);
  ctk_widget_show (widget);

  ctk_box_pack_start (CTK_BOX (hbox), widget, TRUE, TRUE, 0);
  ctk_box_pack_start (CTK_BOX (cntl), hbox, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (widget), "changed",
                    G_CALLBACK (label_flip_changed), label);

  /* Add Kinetic scrolling control here */
  widget = ctk_check_button_new_with_label ("Kinetic scrolling");
  ctk_widget_show (widget);
  ctk_box_pack_start (CTK_BOX (cntl), widget, TRUE, TRUE, 0);
  g_signal_connect (G_OBJECT (widget), "toggled",
                    G_CALLBACK (kinetic_scrolling_changed), swindow);

  ctk_widget_show (window);

  /* Popover */
  popover = ctk_popover_new (NULL);

  widget = ctk_menu_button_new ();
  ctk_menu_button_set_popover (CTK_MENU_BUTTON (widget), popover);
  ctk_container_add (CTK_CONTAINER (widget), ctk_label_new ("Popover"));
  ctk_box_pack_start (CTK_BOX (cntl), widget, FALSE, FALSE, 0);
  ctk_widget_show_all (widget);

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 6);
  ctk_container_add (CTK_CONTAINER (popover), vbox);
  ctk_widget_show (vbox);

  /* Popover's scrolled window */
  swindow = ctk_scrolled_window_new (NULL, NULL);
  ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (swindow),
                                  CTK_POLICY_AUTOMATIC, CTK_POLICY_AUTOMATIC);

  ctk_box_pack_end (CTK_BOX (vbox), swindow, FALSE, FALSE, 0);
  ctk_widget_show (swindow);
  ctk_widget_show (hbox);

  /* Listbox */
  listbox = ctk_list_box_new ();
  ctk_container_add (CTK_CONTAINER (swindow), listbox);
  ctk_widget_show (listbox);

  /* Min content */
  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 2);

  widget = ctk_label_new ("min-content-width");
  ctk_widget_show (widget);
  ctk_box_pack_start (CTK_BOX (hbox), widget, TRUE, TRUE, 0);

  widget = ctk_spin_button_new_with_range (0.0, 150.0, 10.0);
  ctk_box_pack_start (CTK_BOX (hbox), widget, TRUE, TRUE, 0);
  ctk_widget_show (widget);
  ctk_widget_show (hbox);

  g_object_bind_property (ctk_spin_button_get_adjustment (CTK_SPIN_BUTTON (widget)),
                          "value",
                          swindow,
                          "min-content-width",
                          G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE);

  widget = ctk_label_new ("min-content-height");
  ctk_widget_show (widget);
  ctk_box_pack_start (CTK_BOX (hbox), widget, TRUE, TRUE, 0);


  widget = ctk_spin_button_new_with_range (0.0, 150.0, 10.0);
  ctk_box_pack_start (CTK_BOX (hbox), widget, TRUE, TRUE, 0);
  ctk_box_pack_start (CTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  ctk_widget_show (widget);
  ctk_widget_show (hbox);

  g_object_bind_property (ctk_spin_button_get_adjustment (CTK_SPIN_BUTTON (widget)),
                          "value",
                          swindow,
                          "min-content-height",
                          G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE);

  /* Max content */
  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 2);

  widget = ctk_label_new ("max-content-width");
  ctk_widget_show (widget);
  ctk_box_pack_start (CTK_BOX (hbox), widget, TRUE, TRUE, 0);

  widget = ctk_spin_button_new_with_range (250.0, 1000.0, 10.0);
  ctk_box_pack_start (CTK_BOX (hbox), widget, TRUE, TRUE, 0);
  ctk_widget_show (widget);
  ctk_widget_show (hbox);

  g_object_bind_property (ctk_spin_button_get_adjustment (CTK_SPIN_BUTTON (widget)),
                          "value",
                          swindow,
                          "max-content-width",
                          G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE);

  widget = ctk_label_new ("max-content-height");
  ctk_widget_show (widget);
  ctk_box_pack_start (CTK_BOX (hbox), widget, TRUE, TRUE, 0);

  widget = ctk_spin_button_new_with_range (250.0, 1000.0, 10.0);
  ctk_box_pack_start (CTK_BOX (hbox), widget, TRUE, TRUE, 0);
  ctk_box_pack_start (CTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  ctk_widget_show (widget);
  ctk_widget_show (hbox);

  g_object_bind_property (ctk_spin_button_get_adjustment (CTK_SPIN_BUTTON (widget)),
                          "value",
                          swindow,
                          "max-content-height",
                          G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE);

  /* Add and Remove buttons */
  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 2);

  widget = ctk_button_new_with_label ("Remove");
  ctk_widget_show (widget);
  ctk_box_pack_start (CTK_BOX (hbox), widget, TRUE, TRUE, 0);

  g_signal_connect (widget, "clicked",
                    G_CALLBACK (remove_row), listbox);

  widget = ctk_button_new_with_label ("Add");
  ctk_box_pack_start (CTK_BOX (hbox), widget, TRUE, TRUE, 0);
  ctk_box_pack_start (CTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  ctk_widget_show (widget);
  ctk_widget_show (hbox);

  g_signal_connect (widget, "clicked",
                    G_CALLBACK (add_row), listbox);
}


int
main (int argc, char *argv[])
{
  ctk_init (NULL, NULL);

  scrollable_policy ();

  ctk_main ();

  return 0;
}
