#include <ctk/ctk.h>

static void
destroy_cb (CtkWidget  *window G_GNUC_UNUSED,
            CtkBuilder *builder G_GNUC_UNUSED)
{
  ctk_main_quit ();
}

static void
populate_popup_cb (CtkAppChooserWidget *app_chooser_widget G_GNUC_UNUSED,
                   CtkMenu             *menu,
                   GAppInfo            *app_info G_GNUC_UNUSED,
                   gpointer             user_data G_GNUC_UNUSED)
{
  CtkWidget *menu_item;

  menu_item = ctk_menu_item_new_with_label ("Menu Item A");
  ctk_widget_show (menu_item);
  ctk_menu_shell_append (CTK_MENU_SHELL (menu), menu_item);

  menu_item = ctk_menu_item_new_with_label ("Menu Item B");
  ctk_widget_show (menu_item);
  ctk_menu_shell_append (CTK_MENU_SHELL (menu), menu_item);

  menu_item = ctk_menu_item_new_with_label ("Menu Item C");
  ctk_widget_show (menu_item);
  ctk_menu_shell_append (CTK_MENU_SHELL (menu), menu_item);

  menu_item = ctk_menu_item_new_with_label ("Menu Item D");
  ctk_widget_show (menu_item);
  ctk_menu_shell_append (CTK_MENU_SHELL (menu), menu_item);

  menu_item = ctk_menu_item_new_with_label ("Menu Item E");
  ctk_widget_show (menu_item);
  ctk_menu_shell_append (CTK_MENU_SHELL (menu), menu_item);
}

int
main (int   argc,
      char *argv[])
{
  CtkBuilder *builder;
  CtkWidget *window;
  CtkWidget *app_chooser_widget;

  ctk_init (&argc, &argv);

  builder = ctk_builder_new_from_file ("popupat.ui");

  window = CTK_WIDGET (ctk_builder_get_object (builder, "window"));
  g_signal_connect (window, "destroy", G_CALLBACK (destroy_cb), builder);

  app_chooser_widget = CTK_WIDGET (ctk_builder_get_object (builder, "appchooserwidget"));
  g_signal_connect (app_chooser_widget, "populate-popup", G_CALLBACK (populate_popup_cb), builder);

  ctk_widget_show_all (window);

  ctk_main ();

  g_object_unref (builder);

  return 0;
}
