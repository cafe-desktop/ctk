#include <stdlib.h>
#include <ctk/ctk.h>

static gboolean
boolean_to_text (GBinding     *binding G_GNUC_UNUSED,
                 const GValue *source,
                 GValue       *target,
                 gpointer      dummy G_GNUC_UNUSED)
{
  if (g_value_get_boolean (source))
    g_value_set_string (target, "Enabled");
  else
    g_value_set_string (target, "Disabled");

  return TRUE;
}

static CtkWidget *
make_switch (gboolean is_on,
             gboolean is_sensitive)
{
  CtkWidget *hbox;
  CtkWidget *sw, *label;

  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 6);

  sw = ctk_switch_new ();
  ctk_switch_set_active (CTK_SWITCH (sw), is_on);
  ctk_box_pack_start (CTK_BOX (hbox), sw, FALSE, FALSE, 0);
  ctk_widget_set_sensitive (sw, is_sensitive);
  ctk_widget_show (sw);

  label = ctk_label_new (is_on ? "Enabled" : "Disabled");
  ctk_box_pack_end (CTK_BOX (hbox), label, TRUE, TRUE, 0);
  ctk_widget_show (label);

  g_object_bind_property_full (sw, "active",
                               label, "label",
                               G_BINDING_DEFAULT,
                               boolean_to_text,
                               NULL,
                               NULL, NULL);

  return hbox;
}

typedef struct {
  CtkSwitch *sw;
  gboolean state;
} SetStateData;

static gboolean
set_state_delayed (gpointer data)
{
  SetStateData *d = data;

  ctk_switch_set_state (d->sw, d->state);

  g_object_set_data (G_OBJECT (d->sw), "timeout", NULL);

  return G_SOURCE_REMOVE; 
}

static gboolean
set_state (CtkSwitch *sw,
	   gboolean   state,
	   gpointer   data G_GNUC_UNUSED)
{
  SetStateData *d;
  guint id;

  id = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (sw), "timeout"));

  if (id != 0)
    g_source_remove (id);

  d = g_new (SetStateData, 1);
  d->sw = sw;
  d->state = state;

  id = g_timeout_add_full (G_PRIORITY_DEFAULT, 2000, set_state_delayed, d, g_free);
  g_object_set_data (G_OBJECT (sw), "timeout", GUINT_TO_POINTER (id));

  return TRUE;
}

static void
sw_delay_notify (GObject    *obj,
		 GParamSpec *pspec G_GNUC_UNUSED,
		 gpointer    data)
{
  CtkWidget *spinner = data;
  gboolean active;
  gboolean state;

  g_object_get (obj,
                "active", &active,
                "state", &state,
                NULL);

  if (active != state)
    {
      ctk_spinner_start (CTK_SPINNER (spinner));
      ctk_widget_set_opacity (spinner, 1.0);
    }
  else
    {
      ctk_widget_set_opacity (spinner, 0.0);
      ctk_spinner_stop (CTK_SPINNER (spinner));
    }
}

static CtkWidget *
make_delayed_switch (gboolean is_on,
                     gboolean is_sensitive)
{
  CtkWidget *hbox;
  CtkWidget *sw, *label, *spinner, *check;

  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 6);

  sw = ctk_switch_new ();
  ctk_switch_set_active (CTK_SWITCH (sw), is_on);
  ctk_box_pack_start (CTK_BOX (hbox), sw, FALSE, FALSE, 0);
  ctk_widget_set_sensitive (sw, is_sensitive);
  ctk_widget_show (sw);

  g_signal_connect (sw, "state-set", G_CALLBACK (set_state), NULL);

  spinner = ctk_spinner_new ();
  ctk_box_pack_start (CTK_BOX (hbox), spinner, FALSE, TRUE, 0);
  ctk_widget_set_opacity (spinner, 0.0);
  ctk_widget_show (spinner);
  
  check = ctk_check_button_new ();
  ctk_box_pack_end (CTK_BOX (hbox), check, FALSE, TRUE, 0);
  ctk_widget_show (check);
  g_object_bind_property (sw, "state",
                          check, "active",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

  label = ctk_label_new (is_on ? "Enabled" : "Disabled");
  ctk_box_pack_end (CTK_BOX (hbox), label, TRUE, TRUE, 0);
  ctk_widget_show (label);

  g_object_bind_property_full (sw, "active",
                               label, "label",
                               G_BINDING_DEFAULT,
                               boolean_to_text,
                               NULL,
                               NULL, NULL);

  g_signal_connect (sw, "notify", G_CALLBACK (sw_delay_notify), spinner);

  return hbox;
}

int main (int argc, char *argv[])
{
  CtkWidget *window;
  CtkWidget *vbox, *hbox;

  ctk_init (&argc, &argv);

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_title (CTK_WINDOW (window), "CtkSwitch");
  ctk_window_set_default_size (CTK_WINDOW (window), 400, -1);
  ctk_container_set_border_width (CTK_CONTAINER (window), 6);
  g_signal_connect (window, "destroy", G_CALLBACK (ctk_main_quit), NULL);
  ctk_widget_show (window);

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 12);
  ctk_container_add (CTK_CONTAINER (window), vbox);
  ctk_widget_show (vbox);

  hbox = make_switch (FALSE, TRUE);
  ctk_container_add (CTK_CONTAINER (vbox), hbox);
  ctk_widget_show (hbox);

  hbox = make_switch (TRUE, TRUE);
  ctk_container_add (CTK_CONTAINER (vbox), hbox);
  ctk_widget_show (hbox);

  hbox = make_switch (FALSE, FALSE);
  ctk_container_add (CTK_CONTAINER (vbox), hbox);
  ctk_widget_show (hbox);

  hbox = make_switch (TRUE, FALSE);
  ctk_container_add (CTK_CONTAINER (vbox), hbox);
  ctk_widget_show (hbox);

  hbox = make_delayed_switch (FALSE, TRUE);
  ctk_container_add (CTK_CONTAINER (vbox), hbox);
  ctk_widget_show (hbox);

  ctk_main ();

  return EXIT_SUCCESS;
}
