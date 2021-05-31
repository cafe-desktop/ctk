#include <ctk/ctk.h>

static void
toggle_center (GtkCheckButton *button,
               GParamSpec     *pspec,
               GtkActionBar   *bar)
{
  if (ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (button)))
    {
      GtkWidget *button;

      button = ctk_button_new_with_label ("Center");
      ctk_widget_show (button);
      ctk_action_bar_set_center_widget (bar, button);
    }
  else
    {
      ctk_action_bar_set_center_widget (bar, NULL);
    }
}

static void
toggle_visibility (GtkCheckButton *button,
                   GParamSpec     *pspec,
                   GtkActionBar   *bar)
{
  if (ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (button)))
    ctk_widget_show (CTK_WIDGET (bar));
  else
    ctk_widget_hide (CTK_WIDGET (bar));
}

static void
create_widgets (GtkActionBar  *bar,
                GtkPackType    pack_type,
                gint           n)
{
  GList *children, *l;
  GtkWidget *child;
  gint i;
  gchar *label;

  children = ctk_container_get_children (CTK_CONTAINER (bar));
  for (l = children; l; l = l->next)
    {
      GtkPackType type;

      child = l->data;
      ctk_container_child_get (CTK_CONTAINER (bar), child, "pack-type", &type, NULL);
      if (type == pack_type)
        ctk_container_remove (CTK_CONTAINER (bar), child);
    }
  g_list_free (children);

  for (i = 0; i < n; i++)
    {
      label = g_strdup_printf ("%d", i);
      child = ctk_button_new_with_label (label);
      g_free (label);

      ctk_widget_show (child);
      if (pack_type == CTK_PACK_START)
        ctk_action_bar_pack_start (bar, child);
      else
        ctk_action_bar_pack_end (bar, child);
    }
}

static void
change_start (GtkSpinButton *button,
              GParamSpec    *pspec,
              GtkActionBar  *bar)
{
  create_widgets (bar,
                  CTK_PACK_START,
                  ctk_spin_button_get_value_as_int (button));
}

static void
change_end (GtkSpinButton *button,
            GParamSpec    *pspec,
            GtkActionBar  *bar)
{
  create_widgets (bar,
                  CTK_PACK_END,
                  ctk_spin_button_get_value_as_int (button));
}

static void
activate (GApplication *gapp)
{
  GtkApplication *app = CTK_APPLICATION (gapp);
  GtkWidget *window;
  GtkWidget *box;
  GtkWidget *grid;
  GtkWidget *label;
  GtkWidget *spin;
  GtkWidget *check;
  GtkWidget *bar;

  window = ctk_application_window_new (app);
  ctk_application_add_window (app, CTK_WINDOW (window));

  bar = ctk_action_bar_new ();
  ctk_widget_set_no_show_all (bar, TRUE);

  box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);

  grid = ctk_grid_new ();
  g_object_set (grid,
                "halign", CTK_ALIGN_CENTER,
                "margin", 20,
                "row-spacing", 12,
                "column-spacing", 12,
                NULL);
  ctk_box_pack_start (CTK_BOX (box), grid, FALSE, FALSE, 0);

  label = ctk_label_new ("Start");
  ctk_widget_set_halign (label, CTK_ALIGN_END);
  spin = ctk_spin_button_new_with_range (0, 10, 1);
  g_signal_connect (spin, "notify::value",
                    G_CALLBACK (change_start), bar);
  ctk_grid_attach (CTK_GRID (grid), label, 0, 0, 1, 1);
  ctk_grid_attach (CTK_GRID (grid), spin, 1, 0, 1, 1);

  label = ctk_label_new ("Center");
  ctk_widget_set_halign (label, CTK_ALIGN_END);
  check = ctk_check_button_new ();
  g_signal_connect (check, "notify::active",
                    G_CALLBACK (toggle_center), bar);
  ctk_grid_attach (CTK_GRID (grid), label, 0, 1, 1, 1);
  ctk_grid_attach (CTK_GRID (grid), check, 1, 1, 1, 1);

  label = ctk_label_new ("End");
  ctk_widget_set_halign (label, CTK_ALIGN_END);
  spin = ctk_spin_button_new_with_range (0, 10, 1);
  g_signal_connect (spin, "notify::value",
                    G_CALLBACK (change_end), bar);
  ctk_grid_attach (CTK_GRID (grid), label, 0, 2, 1, 1);
  ctk_grid_attach (CTK_GRID (grid), spin, 1, 2, 1, 1);

  label = ctk_label_new ("Visible");
  ctk_widget_set_halign (label, CTK_ALIGN_END);
  check = ctk_check_button_new ();
  g_signal_connect (check, "notify::active",
                    G_CALLBACK (toggle_visibility), bar);
  ctk_grid_attach (CTK_GRID (grid), label, 0, 3, 1, 1);
  ctk_grid_attach (CTK_GRID (grid), check, 1, 3, 1, 1);

  ctk_box_pack_end (CTK_BOX (box), bar, FALSE, FALSE, 0);
  ctk_container_add (CTK_CONTAINER (window), box);
  ctk_widget_show_all (window);
}

int
main (int argc, char *argv[])
{
  GtkApplication *app;

  app = ctk_application_new ("org.ctk.Test.ActionBar", 0);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);

  return g_application_run (G_APPLICATION (app), argc, argv);
}
