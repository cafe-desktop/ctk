/* Benchmark/Fishbowl
 *
 * This demo models the fishbowl demos seen on the web in a CTK way.
 * It's also a neat little tool to see how fast your computer (or
 * your CTK version) is.
 */

#include <ctk/ctk.h>

#include "ctkfishbowl.h"

const char *const css =
".blurred-button {"
"  box-shadow: 0px 0px 5px 10px rgba(0, 0, 0, 0.5);"
"}"
"";

char **icon_names = NULL;
gsize n_icon_names = 0;

static void
init_icon_names (CtkIconTheme *theme)
{
  GPtrArray *icons;
  GList *l, *icon_list;

  if (icon_names)
    return;

  icon_list = ctk_icon_theme_list_icons (theme, NULL);
  icons = g_ptr_array_new ();

  for (l = icon_list; l; l = l->next)
    {
      if (g_str_has_suffix (l->data, "symbolic"))
        continue;

      g_ptr_array_add (icons, g_strdup (l->data));
    }

  n_icon_names = icons->len;
  g_ptr_array_add (icons, NULL); /* NULL-terminate the array */
  icon_names = (char **) g_ptr_array_free (icons, FALSE);

  /* don't free strings, we assigned them to the array */
  g_list_free_full (icon_list, g_free);
}

static const char *
get_random_icon_name (CtkIconTheme *theme)
{
  init_icon_names (theme);

  return icon_names[g_random_int_range(0, n_icon_names)];
}

CtkWidget *
create_icon (void)
{
  CtkWidget *image;

  image = ctk_image_new_from_icon_name (get_random_icon_name (ctk_icon_theme_get_default ()), CTK_ICON_SIZE_DND);

  return image;
}

static CtkWidget *
create_button (void)
{
  return ctk_button_new_with_label ("Button");
}

static CtkWidget *
create_blurred_button (void)
{
  CtkWidget *w = ctk_button_new ();

  ctk_style_context_add_class (ctk_widget_get_style_context (w), "blurred-button");

  return w;
}

static CtkWidget *
create_font_button (void)
{
  return ctk_font_button_new ();
}

static CtkWidget *
create_level_bar (void)
{
  CtkWidget *w = ctk_level_bar_new_for_interval (0, 100);

  ctk_level_bar_set_value (CTK_LEVEL_BAR (w), 50);

  /* Force them to be a bit larger */
  ctk_widget_set_size_request (w, 200, -1);

  return w;
}

static CtkWidget *
create_spinner (void)
{
  CtkWidget *w = ctk_spinner_new ();

  ctk_spinner_start (CTK_SPINNER (w));

  return w;
}

static CtkWidget *
create_spinbutton (void)
{
  CtkWidget *w = ctk_spin_button_new_with_range (0, 10, 1);

  return w;
}

static CtkWidget *
create_label (void)
{
  CtkWidget *w = ctk_label_new ("pLorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua.");

  ctk_label_set_line_wrap (CTK_LABEL (w), TRUE);
  ctk_label_set_max_width_chars (CTK_LABEL (w), 100);

  return w;
}

#if 0
static CtkWidget *
create_gears (void)
{
  CtkWidget *w = ctk_gears_new ();

  ctk_widget_set_size_request (w, 100, 100);

  return w;
}
#endif

static CtkWidget *
create_switch (void)
{
  CtkWidget *w = ctk_switch_new ();

  ctk_switch_set_state (CTK_SWITCH (w), TRUE);

  return w;
}

static const struct {
  const char *name;
  CtkWidget * (*create_func) (void);
} widget_types[] = {
  { "Icon",       create_icon           },
  { "Button",     create_button         },
  { "Blurbutton", create_blurred_button },
  { "Fontbutton", create_font_button    },
  { "Levelbar",   create_level_bar      },
  { "Label",      create_label          },
  { "Spinner",    create_spinner        },
  { "Spinbutton", create_spinbutton     },
 // { "Gears",      create_gears          },
  { "Switch",     create_switch         },
};

static int selected_widget_type = -1;
static const int N_WIDGET_TYPES = G_N_ELEMENTS (widget_types);

static void
set_widget_type (CtkFishbowl *fishbowl,
                 int          widget_type_index)
{
  CtkWidget *window, *headerbar;

  if (widget_type_index == selected_widget_type)
    return;

  selected_widget_type = widget_type_index;

  ctk_fishbowl_set_creation_func (fishbowl,
                                  widget_types[selected_widget_type].create_func);

  window = ctk_widget_get_toplevel (CTK_WIDGET (fishbowl));
  headerbar = ctk_window_get_titlebar (CTK_WINDOW (window));
  ctk_header_bar_set_title (CTK_HEADER_BAR (headerbar),
                            widget_types[selected_widget_type].name);
}

void
next_button_clicked_cb (CtkButton *source G_GNUC_UNUSED,
                        gpointer   user_data)
{
  CtkFishbowl *fishbowl = user_data;
  int new_index;

  if (selected_widget_type + 1 >= N_WIDGET_TYPES)
    new_index = 0;
  else
    new_index = selected_widget_type + 1;

  set_widget_type (fishbowl, new_index);
}

void
prev_button_clicked_cb (CtkButton *source G_GNUC_UNUSED,
                        gpointer   user_data)
{
  CtkFishbowl *fishbowl = user_data;
  int new_index;

  if (selected_widget_type - 1 < 0)
    new_index = N_WIDGET_TYPES - 1;
  else
    new_index = selected_widget_type - 1;

  set_widget_type (fishbowl, new_index);
}


CtkWidget *
do_fishbowl (CtkWidget *do_widget)
{
  static CtkWidget *window = NULL;
  static CtkCssProvider *provider = NULL;

  if (provider == NULL)
    {
      provider = ctk_css_provider_new ();
      ctk_css_provider_load_from_data (provider, css, -1, NULL);
      ctk_style_context_add_provider_for_screen (cdk_screen_get_default (),
                                                 CTK_STYLE_PROVIDER (provider),
                                                 CTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }

  if (!window)
    {
      CtkBuilder *builder;
      CtkWidget *bowl;

      g_type_ensure (CTK_TYPE_FISHBOWL);

      builder = ctk_builder_new_from_resource ("/fishbowl/fishbowl.ui");
      ctk_builder_add_callback_symbols (builder,
                                        "next_button_clicked_cb", G_CALLBACK (next_button_clicked_cb),
                                        "prev_button_clicked_cb", G_CALLBACK (prev_button_clicked_cb),
                                        NULL);
      ctk_builder_connect_signals (builder, NULL);
      window = CTK_WIDGET (ctk_builder_get_object (builder, "window"));
      bowl = CTK_WIDGET (ctk_builder_get_object (builder, "bowl"));
      set_widget_type (CTK_FISHBOWL (bowl), 0);
      ctk_window_set_screen (CTK_WINDOW (window),
                             ctk_widget_get_screen (do_widget));
      g_signal_connect (window, "destroy",
                        G_CALLBACK (ctk_widget_destroyed), &window);

      ctk_widget_realize (window);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show (window);
  else
    ctk_widget_destroy (window);


  return window;
}
