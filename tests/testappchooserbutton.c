/* testappchooserbutton.c
 * Copyright (C) 2010 Red Hat, Inc.
 * Authors: Cosimo Cecchi
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

#include <stdlib.h>
#include <gtk/gtk.h>

#define CUSTOM_ITEM "custom-item"

static GtkWidget *toplevel, *combobox, *box;
static GtkWidget *sel_image, *sel_name;

static void
combo_changed_cb (GtkComboBox *cb,
                  gpointer     user_data)
{
  GAppInfo *app_info;

  app_info = ctk_app_chooser_get_app_info (CTK_APP_CHOOSER (cb));

  if (app_info == NULL)
    return;

  ctk_image_set_from_gicon (CTK_IMAGE (sel_image), g_app_info_get_icon (app_info),
                            CTK_ICON_SIZE_DIALOG);
  ctk_label_set_text (CTK_LABEL (sel_name), g_app_info_get_display_name (app_info));

  g_object_unref (app_info);
}

static void
special_item_activated_cb (GtkAppChooserButton *b,
                           const gchar *item_name,
                           gpointer user_data)
{
  ctk_image_set_from_gicon (CTK_IMAGE (sel_image), g_themed_icon_new ("face-smile"),
                            CTK_ICON_SIZE_DIALOG);
  ctk_label_set_text (CTK_LABEL (sel_name), "Special Item");
}

static void
action_cb (GtkAppChooserButton *b,
           const gchar *item_name,
           gpointer user_data)
{
  g_print ("Activated custom item %s\n", item_name);
}

int
main (int argc,
      char **argv)
{
  GtkWidget *w;

  ctk_init (&argc, &argv);

  toplevel = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_container_set_border_width (CTK_CONTAINER (toplevel), 12);

  box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 6);
  ctk_container_add (CTK_CONTAINER (toplevel), box);

  combobox = ctk_app_chooser_button_new ("image/jpeg");
  ctk_box_pack_start (CTK_BOX (box), combobox, TRUE, TRUE, 0);

  g_signal_connect (combobox, "changed",
                    G_CALLBACK (combo_changed_cb), NULL);

  w = ctk_label_new (NULL);
  ctk_label_set_markup (CTK_LABEL (w), "<b>Selected app info</b>");
  ctk_box_pack_start (CTK_BOX (box), w, TRUE, TRUE, 0);

  w = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 6);
  ctk_box_pack_start (CTK_BOX (box), w, TRUE, TRUE, 0);

  sel_image = ctk_image_new ();
  ctk_box_pack_start (CTK_BOX (w), sel_image, TRUE, TRUE, 0);
  sel_name = ctk_label_new (NULL);
  ctk_box_pack_start (CTK_BOX (w), sel_name, TRUE, TRUE, 0);

  ctk_app_chooser_button_set_heading (CTK_APP_CHOOSER_BUTTON (combobox), "Choose one, <i>not</i> two");
  ctk_app_chooser_button_append_separator (CTK_APP_CHOOSER_BUTTON (combobox));
  ctk_app_chooser_button_append_custom_item (CTK_APP_CHOOSER_BUTTON (combobox),
                                             CUSTOM_ITEM,
                                             "Hey, I'm special!",
                                             g_themed_icon_new ("face-smile"));

  /* this one will trigger a warning, and will not be added */
  ctk_app_chooser_button_append_custom_item (CTK_APP_CHOOSER_BUTTON (combobox),
                                             CUSTOM_ITEM,
                                             "Hey, I'm fake!",
                                             g_themed_icon_new ("face-evil"));

  ctk_app_chooser_button_set_show_dialog_item (CTK_APP_CHOOSER_BUTTON (combobox),
                                               TRUE);
  ctk_app_chooser_button_set_show_default_item (CTK_APP_CHOOSER_BUTTON (combobox),
                                                TRUE);

  /* connect to the detailed signal */
  g_signal_connect (combobox, "custom-item-activated::" CUSTOM_ITEM,
                    G_CALLBACK (special_item_activated_cb), NULL);

  /* connect to the generic signal too */
  g_signal_connect (combobox, "custom-item-activated",
                    G_CALLBACK (action_cb), NULL);

  /* test refresh on a combo */
  ctk_app_chooser_refresh (CTK_APP_CHOOSER (combobox));

#if 0
  ctk_app_chooser_button_set_active_custom_item (CTK_APP_CHOOSER_BUTTON (combobox),
                                                 CUSTOM_ITEM);
#endif
  ctk_widget_show_all (toplevel);

  g_signal_connect (toplevel, "delete-event",
                    G_CALLBACK (ctk_main_quit), NULL);

  ctk_main ();

  return EXIT_SUCCESS;
}
