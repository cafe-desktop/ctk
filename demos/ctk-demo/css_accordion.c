/* Theming/CSS Accordion
 *
 * A simple accordion demo written using CSS transitions and multiple backgrounds
 *
 */

#include <ctk/ctk.h>

static void
apply_css (GtkWidget *widget, GtkStyleProvider *provider)
{
  ctk_style_context_add_provider (ctk_widget_get_style_context (widget), provider, G_MAXUINT);
  if (CTK_IS_CONTAINER (widget))
    ctk_container_forall (CTK_CONTAINER (widget), (GtkCallback) apply_css, provider);
}

GtkWidget *
do_css_accordion (GtkWidget *do_widget)
{
  static GtkWidget *window = NULL;

  if (!window)
    {
      GtkWidget *container, *child;
      GtkStyleProvider *provider;

      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_title (CTK_WINDOW (window), "CSS Accordion");
      ctk_window_set_transient_for (CTK_WINDOW (window), CTK_WINDOW (do_widget));
      ctk_window_set_default_size (CTK_WINDOW (window), 600, 300);
      g_signal_connect (window, "destroy",
                        G_CALLBACK (ctk_widget_destroyed), &window);

      container = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
      ctk_widget_set_halign (container, CTK_ALIGN_CENTER);
      ctk_widget_set_valign (container, CTK_ALIGN_CENTER);
      ctk_container_add (CTK_CONTAINER (window), container);

      child = ctk_button_new_with_label ("This");
      ctk_container_add (CTK_CONTAINER (container), child);

      child = ctk_button_new_with_label ("Is");
      ctk_container_add (CTK_CONTAINER (container), child);

      child = ctk_button_new_with_label ("A");
      ctk_container_add (CTK_CONTAINER (container), child);

      child = ctk_button_new_with_label ("CSS");
      ctk_container_add (CTK_CONTAINER (container), child);

      child = ctk_button_new_with_label ("Accordion");
      ctk_container_add (CTK_CONTAINER (container), child);

      child = ctk_button_new_with_label (":-)");
      ctk_container_add (CTK_CONTAINER (container), child);

      provider = CTK_STYLE_PROVIDER (ctk_css_provider_new ());
      ctk_css_provider_load_from_resource (CTK_CSS_PROVIDER (provider), "/css_accordion/css_accordion.css");

      apply_css (window, provider);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);

  return window;
}
