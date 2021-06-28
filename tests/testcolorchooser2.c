#include <ctk/ctk.h>

static void rgba_changed (CtkColorChooser *chooser, GParamSpec *pspec, gpointer data);

static void
text_activated (CtkEntry *entry, gpointer data)
{
  CtkColorChooser *chooser = data;
  GdkRGBA rgba;
  const char *text;

  text = ctk_entry_get_text (entry);

  g_signal_handlers_block_by_func (entry, rgba_changed, entry);
  if (cdk_rgba_parse (&rgba, text))
    ctk_color_chooser_set_rgba (chooser, &rgba);
  g_signal_handlers_unblock_by_func (entry, rgba_changed, entry);
}

static void
rgba_changed (CtkColorChooser *chooser, GParamSpec *pspec, gpointer data)
{
  CtkWidget *entry = data;
  GdkRGBA color;
  char *s;

  ctk_color_chooser_get_rgba (chooser, &color);
  s = cdk_rgba_to_string (&color);

  g_signal_handlers_block_by_func (entry, text_activated, chooser);
  ctk_entry_set_text (CTK_ENTRY (entry), s);
  g_signal_handlers_unblock_by_func (entry, text_activated, chooser);

  g_free (s);
}

int main (int argc, char *argv[])
{
  CtkWidget *window;
  CtkWidget *chooser;
  CtkWidget *entry;
  CtkBuilder *builder;

  ctk_init (NULL, NULL);

  builder = ctk_builder_new_from_file ("testcolorchooser2.ui");
  window = CTK_WIDGET (ctk_builder_get_object (builder, "window1"));
  chooser = CTK_WIDGET (ctk_builder_get_object (builder, "chooser"));
  entry = CTK_WIDGET (ctk_builder_get_object (builder, "entry"));

  g_signal_connect (chooser, "notify::rgba", G_CALLBACK (rgba_changed), entry);
  g_signal_connect (entry, "activate", G_CALLBACK (text_activated), chooser);

  ctk_widget_show (window);

  ctk_main ();

  return 0;
}
