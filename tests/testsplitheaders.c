#include <ctk/ctk.h>

static void
split_decorations (GtkSettings *settings,
                   GParamSpec  *pspec,
                   GtkBuilder  *builder)
{
  GtkWidget *sheader, *mheader;
  gchar *layout, *p1, *p2;
  gchar **p;

  sheader = (GtkWidget *)ctk_builder_get_object (builder, "sidebar-header");
  mheader = (GtkWidget *)ctk_builder_get_object (builder, "main-header");

  g_object_get (settings, "ctk-decoration-layout", &layout, NULL);

  p = g_strsplit (layout, ":", -1);

  p1 = g_strconcat ("", p[0], ":", NULL);

  if (g_strv_length (p) >= 2)
    p2 = g_strconcat (":", p[1], NULL);
  else
    p2 = g_strdup ("");

  ctk_header_bar_set_decoration_layout (CTK_HEADER_BAR (sheader), p1);
  ctk_header_bar_set_decoration_layout (CTK_HEADER_BAR (mheader), p2);
 
  g_free (p1);
  g_free (p2);
  g_strfreev (p);
  g_free (layout);
}

int
main (int argc, char *argv[])
{
  GtkBuilder *builder;
  GtkSettings *settings;
  GtkWidget *win;
  GtkWidget *entry;
  GtkWidget *check;
  GtkWidget *header;

  ctk_init (NULL, NULL);

  builder = ctk_builder_new_from_file ("testsplitheaders.ui");

  win = (GtkWidget *)ctk_builder_get_object (builder, "window");
  settings = ctk_widget_get_settings (win);

  g_signal_connect (settings, "notify::ctk-decoration-layout",
                    G_CALLBACK (split_decorations), builder);
  split_decorations (settings, NULL, builder);
  
  entry = (GtkWidget *)ctk_builder_get_object (builder, "layout-entry");
  g_object_bind_property (settings, "ctk-decoration-layout",
                          entry, "text",
                          G_BINDING_BIDIRECTIONAL|G_BINDING_SYNC_CREATE);                      
  check = (GtkWidget *)ctk_builder_get_object (builder, "decorations");
  header = (GtkWidget *)ctk_builder_get_object (builder, "sidebar-header");
  g_object_bind_property (check, "active", 
                          header, "show-close-button",
			  G_BINDING_DEFAULT);                      
  header = (GtkWidget *)ctk_builder_get_object (builder, "main-header");
  g_object_bind_property (check, "active", 
                          header, "show-close-button",
			  G_BINDING_DEFAULT);                      
  ctk_window_present (CTK_WINDOW (win));

  ctk_main ();

  return 0;
}
