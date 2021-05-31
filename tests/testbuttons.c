/* testbuttons.c
 * Copyright (C) 2009 Red Hat, Inc.
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

#include <gtk/gtk.h>

/* various combinations of use_underline and use_stock */

int main (int argc, char *argv[])
{
	GtkWidget *window, *box, *button, *hbox;
        gchar *text;
	gboolean use_underline, use_stock;
	GtkWidget *image, *label;

	ctk_init (&argc, &argv);

	window = ctk_window_new (CTK_WINDOW_TOPLEVEL);

	box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);

	ctk_container_add (CTK_CONTAINER (window), box);

	hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
	ctk_container_add (CTK_CONTAINER (box), hbox);
        G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
	button = ctk_button_new_from_stock (CTK_STOCK_SAVE);
        G_GNUC_END_IGNORE_DEPRECATIONS;
	ctk_container_add (CTK_CONTAINER (hbox), button);

	g_object_get (button,
                      "label", &text,
                      "use-stock", &use_stock,
                      "use-underline", &use_underline,
		      "image", &image,
                      NULL);
	text = g_strdup_printf ("label: \"%s\" image: %p use-stock: %s use-underline: %s\n", text, image, use_stock ? "TRUE" : "FALSE", use_underline ? "TRUE" : "FALSE");
	label = ctk_label_new (text);
	g_free (text);
	ctk_container_add (CTK_CONTAINER (hbox), label);

	hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
	ctk_container_add (CTK_CONTAINER (box), hbox);
	button = g_object_new (CTK_TYPE_BUTTON,
                               "label", "document-save",
			       "use-stock", TRUE,
			       NULL);
	ctk_container_add (CTK_CONTAINER (hbox), button);

	g_object_get (button,
                      "label", &text,
                      "use-stock", &use_stock,
                      "use-underline", &use_underline,
		      "image", &image,
                      NULL);
	text = g_strdup_printf ("label: \"%s\" image: %p use-stock: %s use-underline: %s\n", text, image, use_stock ? "TRUE" : "FALSE", use_underline ? "TRUE" : "FALSE");
	label = ctk_label_new (text);
	g_free (text);
	ctk_container_add (CTK_CONTAINER (hbox), label);

	hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
	ctk_container_add (CTK_CONTAINER (box), hbox);
	button = ctk_button_new_with_label ("_Save");
	ctk_container_add (CTK_CONTAINER (hbox), button);

	g_object_get (button,
                      "label", &text,
                      "use-stock", &use_stock,
                      "use-underline", &use_underline,
		      "image", &image,
                      NULL);
	text = g_strdup_printf ("label: \"%s\" image: %p use-stock: %s use-underline: %s\n", text, image, use_stock ? "TRUE" : "FALSE", use_underline ? "TRUE" : "FALSE");
	label = ctk_label_new (text);
	g_free (text);
	ctk_container_add (CTK_CONTAINER (hbox), label);

	hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
	ctk_container_add (CTK_CONTAINER (box), hbox);
	button = ctk_button_new_with_mnemonic ("_Save");
	ctk_container_add (CTK_CONTAINER (hbox), button);

	g_object_get (button,
                      "label", &text,
                      "use-stock", &use_stock,
                      "use-underline", &use_underline,
		      "image", &image,
                      NULL);
	text = g_strdup_printf ("label: \"%s\" image: %p use-stock: %s use-underline: %s\n", text, image, use_stock ? "TRUE" : "FALSE", use_underline ? "TRUE" : "FALSE");
	label = ctk_label_new (text);
	g_free (text);
	ctk_container_add (CTK_CONTAINER (hbox), label);

	hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
	ctk_container_add (CTK_CONTAINER (box), hbox);
	button = ctk_button_new_with_label ("_Save");
	ctk_button_set_image (CTK_BUTTON (button), ctk_image_new_from_icon_name ("help-about", CTK_ICON_SIZE_BUTTON));
	ctk_container_add (CTK_CONTAINER (hbox), button);

	g_object_get (button,
                      "label", &text,
                      "use-stock", &use_stock,
                      "use-underline", &use_underline,
		      "image", &image,
                      NULL);
	text = g_strdup_printf ("label: \"%s\" image: %p use-stock: %s use-underline: %s\n", text, image, use_stock ? "TRUE" : "FALSE", use_underline ? "TRUE" : "FALSE");
	label = ctk_label_new (text);
	g_free (text);
	ctk_container_add (CTK_CONTAINER (hbox), label);

	hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
	ctk_container_add (CTK_CONTAINER (box), hbox);
	button = ctk_button_new_with_mnemonic ("_Save");
	ctk_button_set_image (CTK_BUTTON (button), ctk_image_new_from_icon_name ("help-about", CTK_ICON_SIZE_BUTTON));
	ctk_container_add (CTK_CONTAINER (hbox), button);
	g_object_get (button,
                      "label", &text,
                      "use-stock", &use_stock,
                      "use-underline", &use_underline,
		      "image", &image,
                      NULL);
	text = g_strdup_printf ("label: \"%s\" image: %p use-stock: %s use-underline: %s\n", text, image, use_stock ? "TRUE" : "FALSE", use_underline ? "TRUE" : "FALSE");
	label = ctk_label_new (text);
	g_free (text);
	ctk_container_add (CTK_CONTAINER (hbox), label);

	ctk_widget_show_all (window);

	ctk_main ();

	return 0;
}

