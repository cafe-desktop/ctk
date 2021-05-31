#include <glib.h>
#define GDK_VERSION_MIN_REQUIRED G_ENCODE_VERSION (3, 8)
#include <gtk/gtk.h>

int main (int argc, char **argv)
{
	GtkWidget *window;
	GtkWidget *grid;
	GtkWidget *button;
	GtkWidget *menu;
	GtkWidget *item;
	GtkWidget *box;
	GtkWidget *label;
	GtkAction *action1;
	GtkAction *action2;
        GtkAccelGroup *accel_group;

	ctk_init (&argc, &argv);

	action1 = ctk_action_new ("bold", NULL, NULL, GTK_STOCK_BOLD);
	action2 = ctk_action_new ("new", NULL, NULL, GTK_STOCK_NEW);
	ctk_action_set_always_show_image (action2, TRUE);

	window = ctk_window_new (GTK_WINDOW_TOPLEVEL);

        accel_group = ctk_accel_group_new ();
        ctk_window_add_accel_group (GTK_WINDOW (window), accel_group);

	grid = ctk_grid_new ();
	ctk_container_add (GTK_CONTAINER (window), grid);

        /* plain old stock button */
        button = ctk_button_new_from_stock (GTK_STOCK_DELETE);
        ctk_container_add (GTK_CONTAINER (grid), button);

	/* ctk_button_set_always_show_image still works */
        button = ctk_button_new_from_stock (GTK_STOCK_CLOSE);
        ctk_button_set_always_show_image (GTK_BUTTON (button), TRUE);
        ctk_container_add (GTK_CONTAINER (grid), button);

	/* old-style image-only button */
	button = ctk_button_new ();
	ctk_button_set_image (GTK_BUTTON (button), ctk_image_new_from_icon_name ("edit-find", GTK_ICON_SIZE_BUTTON));
        ctk_container_add (GTK_CONTAINER (grid), button);

	/* new-style image-only button */
        button = ctk_button_new_from_icon_name ("edit-clear", GTK_ICON_SIZE_BUTTON);
        ctk_container_add (GTK_CONTAINER (grid), button);

	/* GtkAction-backed button */
	button = ctk_button_new ();
	ctk_button_set_use_stock (GTK_BUTTON (button), TRUE);
        ctk_activatable_set_related_action (GTK_ACTIVATABLE (button), action1);
        ctk_container_add (GTK_CONTAINER (grid), button);

	/* ctk_action_set_always_show_image still works for buttons */
	button = ctk_button_new ();
	ctk_button_set_use_stock (GTK_BUTTON (button), TRUE);
        ctk_activatable_set_related_action (GTK_ACTIVATABLE (button), action2);
        ctk_container_add (GTK_CONTAINER (grid), button);

	button = ctk_menu_button_new ();
        ctk_container_add (GTK_CONTAINER (grid), button);

	menu = ctk_menu_new ();
        ctk_menu_set_accel_group (GTK_MENU (menu), accel_group);
        ctk_menu_set_accel_path (GTK_MENU (menu), "<menu>/TEST");

	ctk_menu_button_set_popup (GTK_MENU_BUTTON (button), menu);

	/* plain old stock menuitem */
	item = ctk_image_menu_item_new_from_stock (GTK_STOCK_DELETE, NULL);
	ctk_menu_shell_append (GTK_MENU_SHELL (menu), item);

	/* ctk_image_menu_item_set_always_show_image still works */
	item = ctk_image_menu_item_new_from_stock (GTK_STOCK_CLOSE, accel_group);
	ctk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM (item), TRUE);
	ctk_menu_shell_append (GTK_MENU_SHELL (menu), item);

	/* new-style menuitem with image */
	item = ctk_menu_item_new ();
	box = ctk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
	ctk_container_add (GTK_CONTAINER (item), box);
	ctk_container_add (GTK_CONTAINER (box), ctk_image_new_from_icon_name ("edit-clear", GTK_ICON_SIZE_MENU));
        label = ctk_accel_label_new ("C_lear");
        ctk_label_set_use_underline (GTK_LABEL (label), TRUE);
        ctk_label_set_xalign (GTK_LABEL (label), 0.0);
        ctk_widget_set_halign (label, GTK_ALIGN_FILL);

        ctk_widget_add_accelerator (item, "activate", accel_group,
                                    GDK_KEY_x, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
        ctk_accel_label_set_accel_widget (GTK_ACCEL_LABEL (label), item);
	ctk_box_pack_end (GTK_BOX (box), label, TRUE, TRUE, 0);
	ctk_menu_shell_append (GTK_MENU_SHELL (menu), item);

	/* GtkAction-backed menuitem */
	item = ctk_image_menu_item_new ();
	ctk_activatable_set_related_action (GTK_ACTIVATABLE (item), action1);
	ctk_menu_shell_append (GTK_MENU_SHELL (menu), item);

	/* ctk_action_set_always_show_image still works for menuitems */
	item = ctk_image_menu_item_new ();
	ctk_activatable_set_related_action (GTK_ACTIVATABLE (item), action2);
	ctk_menu_shell_append (GTK_MENU_SHELL (menu), item);

	ctk_widget_show_all (menu);

	ctk_widget_show_all (window);

	ctk_main ();

	return 0;
}
