/* Menus
 *
 * There are several widgets involved in displaying menus. The
 * CtkMenuBar widget is a menu bar, which normally appears horizontally
 * at the top of an application, but can also be layed out vertically.
 * The CtkMenu widget is the actual menu that pops up. Both CtkMenuBar
 * and CtkMenu are subclasses of CtkMenuShell; a CtkMenuShell contains
 * menu items (CtkMenuItem). Each menu item contains text and/or images
 * and can be selected by the user.
 *
 * There are several kinds of menu item, including plain CtkMenuItem,
 * CtkCheckMenuItem which can be checked/unchecked, CtkRadioMenuItem
 * which is a check menu item that's in a mutually exclusive group,
 * CtkSeparatorMenuItem which is a separator bar, CtkTearoffMenuItem
 * which allows a CtkMenu to be torn off, and CtkImageMenuItem which
 * can place a CtkImage or other widget next to the menu text.
 *
 * A CtkMenuItem can have a submenu, which is simply a CtkMenu to pop
 * up when the menu item is selected. Typically, all menu items in a menu bar
 * have submenus.
 */

#include <ctk/ctk.h>
#include <gdk/gdkkeysyms.h>

#include <stdio.h>

static CtkWidget *
create_menu (gint depth)
{
  CtkWidget *menu;
  CtkRadioMenuItem *last_item;
  char buf[32];
  int i, j;

  if (depth < 1)
    return NULL;

  menu = ctk_menu_new ();
  last_item = NULL;

  for (i = 0, j = 1; i < 5; i++, j++)
    {
      CtkWidget *menu_item;

      sprintf (buf, "item %2d - %d", depth, j);

      menu_item = ctk_radio_menu_item_new_with_label_from_widget (NULL, buf);
      ctk_radio_menu_item_join_group (CTK_RADIO_MENU_ITEM (menu_item), last_item);
      last_item = CTK_RADIO_MENU_ITEM (menu_item);

      ctk_menu_shell_append (CTK_MENU_SHELL (menu), menu_item);
      ctk_widget_show (menu_item);
      if (i == 3)
        ctk_widget_set_sensitive (menu_item, FALSE);

      ctk_menu_item_set_submenu (CTK_MENU_ITEM (menu_item), create_menu (depth - 1));
    }

  return menu;
}

static void
change_orientation (CtkWidget *button,
                    CtkWidget *menubar)
{
  CtkWidget *parent;
  CtkOrientation orientation;

  parent = ctk_widget_get_parent (menubar);
  orientation = ctk_orientable_get_orientation (CTK_ORIENTABLE (parent));
  ctk_orientable_set_orientation (CTK_ORIENTABLE (parent), 1 - orientation);

  if (orientation == CTK_ORIENTATION_VERTICAL)
    g_object_set (menubar, "pack-direction", CTK_PACK_DIRECTION_TTB, NULL);
  else
    g_object_set (menubar, "pack-direction", CTK_PACK_DIRECTION_LTR, NULL);

}

static CtkWidget *window = NULL;

CtkWidget *
do_menus (CtkWidget *do_widget)
{
  CtkWidget *box;
  CtkWidget *box1;
  CtkWidget *box2;
  CtkWidget *button;

  if (!window)
    {
      CtkWidget *menubar;
      CtkWidget *menu;
      CtkWidget *menuitem;
      CtkAccelGroup *accel_group;

      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_screen (CTK_WINDOW (window),
                             ctk_widget_get_screen (do_widget));
      ctk_window_set_title (CTK_WINDOW (window), "Menus");
      g_signal_connect (window, "destroy",
                        G_CALLBACK(ctk_widget_destroyed), &window);

      accel_group = ctk_accel_group_new ();
      ctk_window_add_accel_group (CTK_WINDOW (window), accel_group);

      ctk_container_set_border_width (CTK_CONTAINER (window), 0);

      box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
      ctk_container_add (CTK_CONTAINER (window), box);
      ctk_widget_show (box);

      box1 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
      ctk_container_add (CTK_CONTAINER (box), box1);
      ctk_widget_show (box1);

      menubar = ctk_menu_bar_new ();
      ctk_widget_set_hexpand (menubar, TRUE);
      ctk_box_pack_start (CTK_BOX (box1), menubar, FALSE, TRUE, 0);
      ctk_widget_show (menubar);

      menu = create_menu (2);

      menuitem = ctk_menu_item_new_with_label ("test\nline2");
      ctk_menu_item_set_submenu (CTK_MENU_ITEM (menuitem), menu);
      ctk_menu_shell_append (CTK_MENU_SHELL (menubar), menuitem);
      ctk_widget_show (menuitem);

      menuitem = ctk_menu_item_new_with_label ("foo");
      ctk_menu_item_set_submenu (CTK_MENU_ITEM (menuitem), create_menu (3));
      ctk_menu_shell_append (CTK_MENU_SHELL (menubar), menuitem);
      ctk_widget_show (menuitem);

      menuitem = ctk_menu_item_new_with_label ("bar");
      ctk_menu_item_set_submenu (CTK_MENU_ITEM (menuitem), create_menu (4));
      ctk_menu_shell_append (CTK_MENU_SHELL (menubar), menuitem);
      ctk_widget_show (menuitem);

      box2 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 10);
      ctk_container_set_border_width (CTK_CONTAINER (box2), 10);
      ctk_box_pack_start (CTK_BOX (box1), box2, FALSE, TRUE, 0);
      ctk_widget_show (box2);

      button = ctk_button_new_with_label ("Flip");
      g_signal_connect (button, "clicked",
                        G_CALLBACK (change_orientation), menubar);
      ctk_box_pack_start (CTK_BOX (box2), button, TRUE, TRUE, 0);
      ctk_widget_show (button);

      button = ctk_button_new_with_label ("Close");
      g_signal_connect_swapped (button, "clicked",
                                G_CALLBACK(ctk_widget_destroy), window);
      ctk_box_pack_start (CTK_BOX (box2), button, TRUE, TRUE, 0);
      ctk_widget_set_can_default (button, TRUE);
      ctk_widget_grab_default (button);
      ctk_widget_show (button);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show (window);
  else
    ctk_widget_destroy (window);

  return window;
}
