#include <gtk/gtk.h>

int main (int argc, char **argv)
{
	GtkWidget *window;
	GtkWidget *button;
	GtkWidget *grid;
        GIcon *icon;
        GIcon *icon2;

	ctk_init (&argc, &argv);

	window = ctk_window_new (GTK_WINDOW_TOPLEVEL);

	grid = ctk_grid_new ();
	ctk_container_set_border_width (GTK_CONTAINER (grid), 12);
	ctk_grid_set_row_spacing (GTK_GRID (grid), 12);
	ctk_grid_set_column_spacing (GTK_GRID (grid), 12);
	ctk_container_add (GTK_CONTAINER (window), grid);

        icon = g_themed_icon_new ("folder");
        button = ctk_image_new_from_gicon (icon, GTK_ICON_SIZE_MENU);
	ctk_grid_attach (GTK_GRID (grid), button, 1, 1, 1, 1);

        icon2 = g_themed_icon_new ("folder-symbolic");
        button = ctk_image_new_from_gicon (icon2, GTK_ICON_SIZE_MENU);
	ctk_grid_attach (GTK_GRID (grid), button, 2, 1, 1, 1);

	icon = g_emblemed_icon_new (icon, g_emblem_new (g_themed_icon_new ("emblem-new")));
        button = ctk_image_new_from_gicon (icon, GTK_ICON_SIZE_MENU);
	ctk_grid_attach (GTK_GRID (grid), button, 1, 2, 1, 1);

	icon2 = g_emblemed_icon_new (icon2, g_emblem_new (g_themed_icon_new ("emblem-new")));
        button = ctk_image_new_from_gicon (icon2, GTK_ICON_SIZE_MENU);
	ctk_grid_attach (GTK_GRID (grid), button, 2, 2, 1, 1);

	ctk_widget_show_all (window);

	ctk_main ();

	return 0;
}
