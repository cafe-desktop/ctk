/* Theming/CSS Blend Modes
 *
 * You can blend multiple backgrounds using the CSS blend modes available.
 */

#include <ctk/ctk.h>

#define WID(x) ((CtkWidget*) ctk_builder_get_object (builder, x))

/*
 * These are the available blend modes.
 */
struct {
  gchar *name;
  gchar *id;
} blend_modes[] =
{
  { "Color", "color" },
  { "Color (burn)", "color-burn" },
  { "Color (dodge)", "color-dodge" },
  { "Darken", "darken" },
  { "Difference", "difference" },
  { "Exclusion", "exclusion" },
  { "Hard Light", "hard-light" },
  { "Hue", "hue" },
  { "Lighten", "lighten" },
  { "Luminosity", "luminosity" },
  { "Multiply", "multiply" },
  { "Normal", "normal" },
  { "Overlay", "overlay" },
  { "Saturate", "saturate" },
  { "Screen", "screen" },
  { "Soft Light", "soft-light" },
  { NULL }
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
static void
update_css_for_blend_mode (CtkCssProvider *provider,
                           const gchar    *blend_mode)
{
  GBytes *bytes;
  gchar *css;

  bytes = g_resources_lookup_data ("/css_blendmodes/css_blendmodes.css", 0, NULL);

  css = g_strdup_printf ((gchar*) g_bytes_get_data (bytes, NULL),
                         blend_mode,
                         blend_mode,
                         blend_mode);

  ctk_css_provider_load_from_data (provider, css, -1, NULL);

  g_bytes_unref (bytes);
  g_free (css);
}
#pragma GCC diagnostic pop

static void
row_activated (CtkListBox     *listbox,
               CtkListBoxRow  *row,
               CtkCssProvider *provider)
{
  const gchar *blend_mode;

  blend_mode = blend_modes[ctk_list_box_row_get_index (row)].id;

  update_css_for_blend_mode (provider, blend_mode);
}

static void
setup_listbox (CtkBuilder       *builder,
               CtkStyleProvider *provider)
{
  CtkWidget *normal_row;
  CtkWidget *listbox;
  gint i;

  normal_row = NULL;
  listbox = ctk_list_box_new ();
  ctk_container_add (CTK_CONTAINER (WID ("scrolledwindow")), listbox);

  g_signal_connect (listbox, "row-activated", G_CALLBACK (row_activated), provider);

  /* Add a row for each blend mode available */
  for (i = 0; blend_modes[i].name != NULL; i++)
    {
      CtkWidget *label;
      CtkWidget *row;

      row = ctk_list_box_row_new ();
      label = g_object_new (CTK_TYPE_LABEL,
                            "label", blend_modes[i].name,
                            "xalign", 0.0,
                            NULL);

      ctk_container_add (CTK_CONTAINER (row), label);

      ctk_container_add (CTK_CONTAINER (listbox), row);

      /* The first selected row is "normal" */
      if (g_strcmp0 (blend_modes[i].id, "normal") == 0)
        normal_row = row;
    }

  /* Select the "normal" row */
  ctk_list_box_select_row (CTK_LIST_BOX (listbox), CTK_LIST_BOX_ROW (normal_row));
  g_signal_emit_by_name (G_OBJECT (normal_row), "activate");

  ctk_widget_grab_focus (normal_row);
}

CtkWidget *
do_css_blendmodes (CtkWidget *do_widget)
{
  static CtkWidget *window = NULL;

  if (!window)
    {
      CtkStyleProvider *provider;
      CtkBuilder *builder;

      builder = ctk_builder_new_from_resource ("/css_blendmodes/blendmodes.ui");

      window = WID ("window");
      ctk_window_set_transient_for (CTK_WINDOW (window), CTK_WINDOW (do_widget));
      g_signal_connect (window, "destroy", G_CALLBACK (ctk_widget_destroyed), &window);

      /* Setup the CSS provider for window */
      provider = CTK_STYLE_PROVIDER (ctk_css_provider_new ());

      ctk_style_context_add_provider_for_screen (cdk_screen_get_default (),
                                                 provider,
                                                 CTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

      setup_listbox (builder, provider);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);

  return window;
}
