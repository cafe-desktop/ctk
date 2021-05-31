#include <ctk/ctk.h>

#define INITIAL_HALIGN          CTK_ALIGN_START
#define INITIAL_VALIGN          CTK_ALIGN_START

static GList *menubuttons = NULL;

static void
horizontal_alignment_changed (GtkComboBox *box)
{
	GtkAlign alignment = ctk_combo_box_get_active (box);
	GList *l;

	for (l = menubuttons; l != NULL; l = l->next) {
		GtkMenu *popup = ctk_menu_button_get_popup (CTK_MENU_BUTTON (l->data));
		if (popup != NULL)
			ctk_widget_set_halign (CTK_WIDGET (popup), alignment);
	}
}

static void
vertical_alignment_changed (GtkComboBox *box)
{
	GtkAlign alignment = ctk_combo_box_get_active (box);
	GList *l;

	for (l = menubuttons; l != NULL; l = l->next) {
		GtkMenu *popup = ctk_menu_button_get_popup (CTK_MENU_BUTTON (l->data));
		if (popup != NULL)
			ctk_widget_set_valign (CTK_WIDGET (popup), alignment);
	}
}

int main (int argc, char **argv)
{
	GtkWidget *window;
	GtkWidget *button;
	GtkWidget *grid;
	GtkWidget *entry;
	GtkWidget *label;
	GtkWidget *check;
	GtkWidget *combo;
	GtkWidget *menu_widget;
	GtkAccelGroup *accel_group;
	guint i;
	guint row = 0;
	GMenu *menu;

	ctk_init (&argc, &argv);

	window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
	ctk_window_resize (CTK_WINDOW (window), 400, 300);

	grid = ctk_grid_new ();
	ctk_container_set_border_width (CTK_CONTAINER (grid), 12);
	ctk_grid_set_row_spacing (CTK_GRID (grid), 12);
	ctk_grid_set_column_spacing (CTK_GRID (grid), 12);
	ctk_container_add (CTK_CONTAINER (window), grid);

	accel_group = ctk_accel_group_new ();
	ctk_window_add_accel_group (CTK_WINDOW (window), accel_group);

	/* horizontal alignment */
	label = ctk_label_new ("Horizontal Alignment:");
	ctk_widget_show (label);
	ctk_grid_attach (CTK_GRID (grid), label, 0, row++, 1, 1);

	combo = ctk_combo_box_text_new ();
	ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo), "Fill");
	ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo), "Start");
	ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo), "End");
	ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo), "Center");
	ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo), "Baseline");
	ctk_combo_box_set_active (CTK_COMBO_BOX (combo), INITIAL_HALIGN);
	ctk_widget_show (combo);
	ctk_grid_attach_next_to (CTK_GRID (grid), combo, label, CTK_POS_RIGHT, 1, 1);
	g_signal_connect (G_OBJECT (combo), "changed",
			  G_CALLBACK (horizontal_alignment_changed), menubuttons);

	/* vertical alignment */
	label = ctk_label_new ("Vertical Alignment:");
	ctk_widget_show (label);
	ctk_grid_attach (CTK_GRID (grid), label, 0, row++, 1, 1);

	combo = ctk_combo_box_text_new ();
	ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo), "Fill");
	ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo), "Start");
	ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo), "End");
	ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo), "Center");
	ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo), "Baseline");
	ctk_combo_box_set_active (CTK_COMBO_BOX (combo), INITIAL_HALIGN);
	ctk_widget_show (combo);
	ctk_grid_attach_next_to (CTK_GRID (grid), combo, label, CTK_POS_RIGHT, 1, 1);
	g_signal_connect (G_OBJECT (combo), "changed",
			  G_CALLBACK (vertical_alignment_changed), menubuttons);

	/* Button next to entry */
	entry = ctk_entry_new ();
	ctk_grid_attach (CTK_GRID (grid), entry, 0, row++, 1, 1);
	button = ctk_menu_button_new ();
	ctk_widget_set_halign (button, CTK_ALIGN_START);

	ctk_grid_attach_next_to (CTK_GRID (grid), button, entry, CTK_POS_RIGHT, 1, 1);
	menubuttons = g_list_prepend (menubuttons, button);

	/* Button with GtkMenu */
	menu_widget = ctk_menu_new ();
	for (i = 0; i < 5; ++i) {
		GtkWidget *item;

		if (i == 2) {
			item = ctk_menu_item_new_with_mnemonic ("_Copy");
		} else {
			char *label;

			label = g_strdup_printf ("Item _%d", i + 1);
			item = ctk_menu_item_new_with_mnemonic (label);
			g_free (label);
		}

		ctk_menu_item_set_use_underline (CTK_MENU_ITEM (item), TRUE);
		ctk_container_add (CTK_CONTAINER (menu_widget), item);
	}
	ctk_widget_show_all (menu_widget);

	button = ctk_menu_button_new ();
	ctk_widget_set_halign (button, CTK_ALIGN_START);
	menubuttons = g_list_prepend (menubuttons, button);
	ctk_menu_button_set_popup (CTK_MENU_BUTTON (button), menu_widget);
	ctk_grid_attach (CTK_GRID (grid), button, 1, row++, 1, 1);

        check = ctk_check_button_new_with_label ("Popover");
        ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (check), TRUE);
	ctk_grid_attach (CTK_GRID (grid), check, 0, row, 1, 1);

	/* Button with GMenuModel */
	menu = g_menu_new ();
	for (i = 5; i > 0; i--) {
		char *label;
                GMenuItem *item;
		label = g_strdup_printf ("Item _%d", i);
                item = g_menu_item_new (label, NULL);
                if (i == 3)
                  g_menu_item_set_attribute (item, "icon", "s", "preferences-desktop-locale-symbolic");
		g_menu_insert_item (menu, 0, item);
                g_object_unref (item);
		g_free (label);
	}

	button = ctk_menu_button_new ();
        g_object_bind_property (check, "active", button, "use-popover", G_BINDING_SYNC_CREATE);

	ctk_widget_set_halign (button, CTK_ALIGN_START);
	menubuttons = g_list_prepend (menubuttons, button);
	ctk_menu_button_set_menu_model (CTK_MENU_BUTTON (button), G_MENU_MODEL (menu));
	ctk_grid_attach (CTK_GRID (grid), button, 1, row++, 1, 1);

	ctk_widget_show_all (window);

	ctk_main ();

	return 0;
}
