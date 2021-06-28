/*
 * Copyright (c) 2013 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "config.h"

#include "ctkheaderbar.h"
#include "ctkheaderbarprivate.h"
#include "ctkintl.h"
#include "ctkprivate.h"
#include "ctktypebuiltins.h"
#include "ctkwidgetprivate.h"
#include "ctkcssnodeprivate.h"
#include "ctkcsscustomgadgetprivate.h"
#include "ctkwindowprivate.h"
#include "ctkwidgetprivate.h"
#include "ctkcontainerprivate.h"
#include "a11y/ctkheaderbaraccessible.h"

#include <string.h>

/**
 * SECTION:ctkheaderbar
 * @Short_description: A box with a centered child
 * @Title: CtkHeaderBar
 * @See_also: #CtkBox, #CtkActionBar
 *
 * CtkHeaderBar is similar to a horizontal #CtkBox. It allows children to 
 * be placed at the start or the end. In addition, it allows a title and
 * subtitle to be displayed. The title will be centered with respect to
 * the width of the box, even if the children at either side take up
 * different amounts of space. The height of the titlebar will be
 * set to provide sufficient space for the subtitle, even if none is
 * currently set. If a subtitle is not needed, the space reservation
 * can be turned off with ctk_header_bar_set_has_subtitle().
 *
 * CtkHeaderBar can add typical window frame controls, such as minimize,
 * maximize and close buttons, or the window icon.
 *
 * For these reasons, CtkHeaderBar is the natural choice for use as the custom
 * titlebar widget of a #CtkWindow (see ctk_window_set_titlebar()), as it gives
 * features typical of titlebars while allowing the addition of child widgets.
 */

#define DEFAULT_SPACING 6
#define MIN_TITLE_CHARS 5

struct _CtkHeaderBarPrivate
{
  gchar *title;
  gchar *subtitle;
  CtkWidget *title_label;
  CtkWidget *subtitle_label;
  CtkWidget *label_box;
  CtkWidget *label_sizing_box;
  CtkWidget *subtitle_sizing_label;
  CtkWidget *custom_title;
  gint spacing;
  gboolean has_subtitle;

  GList *children;

  gboolean shows_wm_decorations;
  gchar *decoration_layout;
  gboolean decoration_layout_set;

  CtkWidget *titlebar_start_box;
  CtkWidget *titlebar_end_box;

  CtkWidget *titlebar_start_separator;
  CtkWidget *titlebar_end_separator;

  CtkWidget *titlebar_icon;

  CtkCssGadget *gadget;
};

typedef struct _Child Child;
struct _Child
{
  CtkWidget *widget;
  CtkPackType pack_type;
};

enum {
  PROP_0,
  PROP_TITLE,
  PROP_SUBTITLE,
  PROP_HAS_SUBTITLE,
  PROP_CUSTOM_TITLE,
  PROP_SPACING,
  PROP_SHOW_CLOSE_BUTTON,
  PROP_DECORATION_LAYOUT,
  PROP_DECORATION_LAYOUT_SET,
  LAST_PROP
};

enum {
  CHILD_PROP_0,
  CHILD_PROP_PACK_TYPE,
  CHILD_PROP_POSITION
};

static GParamSpec *header_bar_props[LAST_PROP] = { NULL, };

static void ctk_header_bar_buildable_init (CtkBuildableIface *iface);

G_DEFINE_TYPE_WITH_CODE (CtkHeaderBar, ctk_header_bar, CTK_TYPE_CONTAINER,
                         G_ADD_PRIVATE (CtkHeaderBar)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_BUILDABLE,
                                                ctk_header_bar_buildable_init));

static void
init_sizing_box (CtkHeaderBar *bar)
{
  CtkHeaderBarPrivate *priv = ctk_header_bar_get_instance_private (bar);
  CtkWidget *w;
  CtkStyleContext *context;

  /* We use this box to always request size for the two labels (title
   * and subtitle) as if they were always visible, but then allocate
   * the real label box with its actual size, to keep it center-aligned
   * in case we have only the title.
   */
  w = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  ctk_widget_show (w);
  priv->label_sizing_box = g_object_ref_sink (w);

  w = ctk_label_new (NULL);
  ctk_widget_show (w);
  context = ctk_widget_get_style_context (w);
  ctk_style_context_add_class (context, CTK_STYLE_CLASS_TITLE);
  ctk_box_pack_start (CTK_BOX (priv->label_sizing_box), w, FALSE, FALSE, 0);
  ctk_label_set_line_wrap (CTK_LABEL (w), FALSE);
  ctk_label_set_single_line_mode (CTK_LABEL (w), TRUE);
  ctk_label_set_ellipsize (CTK_LABEL (w), PANGO_ELLIPSIZE_END);
  ctk_label_set_width_chars (CTK_LABEL (w), MIN_TITLE_CHARS);

  w = ctk_label_new (NULL);
  context = ctk_widget_get_style_context (w);
  ctk_style_context_add_class (context, CTK_STYLE_CLASS_SUBTITLE);
  ctk_box_pack_start (CTK_BOX (priv->label_sizing_box), w, FALSE, FALSE, 0);
  ctk_label_set_line_wrap (CTK_LABEL (w), FALSE);
  ctk_label_set_single_line_mode (CTK_LABEL (w), TRUE);
  ctk_label_set_ellipsize (CTK_LABEL (w), PANGO_ELLIPSIZE_END);
  ctk_widget_set_visible (w, priv->has_subtitle || (priv->subtitle && priv->subtitle[0]));
  priv->subtitle_sizing_label = w;
}

static CtkWidget *
create_title_box (const char *title,
                  const char *subtitle,
                  CtkWidget **ret_title_label,
                  CtkWidget **ret_subtitle_label)
{
  CtkWidget *label_box;
  CtkWidget *title_label;
  CtkWidget *subtitle_label;
  CtkStyleContext *context;

  label_box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  ctk_widget_set_valign (label_box, CTK_ALIGN_CENTER);
  ctk_widget_show (label_box);

  title_label = ctk_label_new (title);
  context = ctk_widget_get_style_context (title_label);
  ctk_style_context_add_class (context, CTK_STYLE_CLASS_TITLE);
  ctk_label_set_line_wrap (CTK_LABEL (title_label), FALSE);
  ctk_label_set_single_line_mode (CTK_LABEL (title_label), TRUE);
  ctk_label_set_ellipsize (CTK_LABEL (title_label), PANGO_ELLIPSIZE_END);
  ctk_box_pack_start (CTK_BOX (label_box), title_label, FALSE, FALSE, 0);
  ctk_widget_show (title_label);
  ctk_label_set_width_chars (CTK_LABEL (title_label), MIN_TITLE_CHARS);

  subtitle_label = ctk_label_new (subtitle);
  context = ctk_widget_get_style_context (subtitle_label);
  ctk_style_context_add_class (context, CTK_STYLE_CLASS_SUBTITLE);
  ctk_label_set_line_wrap (CTK_LABEL (subtitle_label), FALSE);
  ctk_label_set_single_line_mode (CTK_LABEL (subtitle_label), TRUE);
  ctk_label_set_ellipsize (CTK_LABEL (subtitle_label), PANGO_ELLIPSIZE_END);
  ctk_box_pack_start (CTK_BOX (label_box), subtitle_label, FALSE, FALSE, 0);
  ctk_widget_set_no_show_all (subtitle_label, TRUE);
  ctk_widget_set_visible (subtitle_label, subtitle && subtitle[0]);

  if (ret_title_label)
    *ret_title_label = title_label;
  if (ret_subtitle_label)
    *ret_subtitle_label = subtitle_label;

  return label_box;
}

gboolean
_ctk_header_bar_update_window_icon (CtkHeaderBar *bar,
                                    CtkWindow    *window)
{
  CtkHeaderBarPrivate *priv = ctk_header_bar_get_instance_private (bar);
  CdkPixbuf *pixbuf;
  gint scale;

  if (priv->titlebar_icon == NULL)
    return FALSE;

  scale = ctk_widget_get_scale_factor (priv->titlebar_icon);
  if (CTK_IS_BUTTON (ctk_widget_get_parent (priv->titlebar_icon)))
    pixbuf = ctk_window_get_icon_for_size (window, scale * 16);
  else
    pixbuf = ctk_window_get_icon_for_size (window, scale * 20);

  if (pixbuf)
    {
      cairo_surface_t *surface;

      surface = cdk_cairo_surface_create_from_pixbuf (pixbuf, scale, ctk_widget_get_window (priv->titlebar_icon));

      ctk_image_set_from_surface (CTK_IMAGE (priv->titlebar_icon), surface);
      cairo_surface_destroy (surface);
      g_object_unref (pixbuf);
      ctk_widget_show (priv->titlebar_icon);

      return TRUE;
    }

  return FALSE;
}

static void
_ctk_header_bar_update_separator_visibility (CtkHeaderBar *bar)
{
  CtkHeaderBarPrivate *priv = ctk_header_bar_get_instance_private (bar);
  gboolean have_visible_at_start = FALSE;
  gboolean have_visible_at_end = FALSE;
  GList *l;
  
  for (l = priv->children; l != NULL; l = l->next)
    {
      Child *child = l->data;

      if (ctk_widget_get_visible (child->widget))
        {
          if (child->pack_type == CTK_PACK_START)
            have_visible_at_start = TRUE;
          else
            have_visible_at_end = TRUE;
        }
    }

  if (priv->titlebar_start_separator != NULL)
    ctk_widget_set_visible (priv->titlebar_start_separator, have_visible_at_start);

  if (priv->titlebar_end_separator != NULL)
    ctk_widget_set_visible (priv->titlebar_end_separator, have_visible_at_end);
}

void
_ctk_header_bar_update_window_buttons (CtkHeaderBar *bar)
{
  CtkHeaderBarPrivate *priv = ctk_header_bar_get_instance_private (bar);
  CtkWidget *widget = CTK_WIDGET (bar), *toplevel;
  CtkWindow *window;
  CtkTextDirection direction;
  gchar *layout_desc;
  gchar **tokens, **t;
  gint i, j;
  GMenuModel *menu;
  gboolean shown_by_shell;
  gboolean is_sovereign_window;

  toplevel = ctk_widget_get_toplevel (widget);
  if (!ctk_widget_is_toplevel (toplevel))
    return;

  if (priv->titlebar_start_box)
    {
      ctk_widget_unparent (priv->titlebar_start_box);
      priv->titlebar_start_box = NULL;
      priv->titlebar_start_separator = NULL;
    }
  if (priv->titlebar_end_box)
    {
      ctk_widget_unparent (priv->titlebar_end_box);
      priv->titlebar_end_box = NULL;
      priv->titlebar_end_separator = NULL;
    }

  priv->titlebar_icon = NULL;

  if (!priv->shows_wm_decorations)
    return;

  direction = ctk_widget_get_direction (widget);

  g_object_get (ctk_widget_get_settings (widget),
                "ctk-shell-shows-app-menu", &shown_by_shell,
                "ctk-decoration-layout", &layout_desc,
                NULL);

  if (priv->decoration_layout_set)
    {
      g_free (layout_desc);
      layout_desc = g_strdup (priv->decoration_layout);
    }

  window = CTK_WINDOW (toplevel);

  if (!shown_by_shell && ctk_window_get_application (window))
    menu = ctk_application_get_app_menu (ctk_window_get_application (window));
  else
    menu = NULL;

  is_sovereign_window = (!ctk_window_get_modal (window) &&
                          ctk_window_get_transient_for (window) == NULL &&
                          ctk_window_get_type_hint (window) == GDK_WINDOW_TYPE_HINT_NORMAL);

  tokens = g_strsplit (layout_desc, ":", 2);
  if (tokens)
    {
      for (i = 0; i < 2; i++)
        {
          CtkWidget *box;
          CtkWidget *separator;
          int n_children = 0;

          if (tokens[i] == NULL)
            break;

          t = g_strsplit (tokens[i], ",", -1);

          separator = ctk_separator_new (CTK_ORIENTATION_VERTICAL);
          ctk_widget_set_no_show_all (separator, TRUE);
          ctk_style_context_add_class (ctk_widget_get_style_context (separator), "titlebutton");

          box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, priv->spacing);

          for (j = 0; t[j]; j++)
            {
              CtkWidget *button = NULL;
              CtkWidget *image = NULL;
              AtkObject *accessible;

              if (strcmp (t[j], "icon") == 0 &&
                  is_sovereign_window)
                {
                  button = ctk_image_new ();
                  ctk_widget_set_valign (button, CTK_ALIGN_CENTER);
                  priv->titlebar_icon = button;
                  ctk_style_context_add_class (ctk_widget_get_style_context (button), "titlebutton");
                  ctk_style_context_add_class (ctk_widget_get_style_context (button), "icon");
                  ctk_widget_set_size_request (button, 20, 20);
                  ctk_widget_show (button);

                  if (!_ctk_header_bar_update_window_icon (bar, window))
                    {
                      ctk_widget_destroy (button);
                      priv->titlebar_icon = NULL;
                      button = NULL;
                    }
                }
              else if (strcmp (t[j], "menu") == 0 &&
                       menu != NULL &&
                       is_sovereign_window)
                {
                  button = ctk_menu_button_new ();
                  ctk_widget_set_valign (button, CTK_ALIGN_CENTER);
                  ctk_menu_button_set_menu_model (CTK_MENU_BUTTON (button), menu);
                  ctk_menu_button_set_use_popover (CTK_MENU_BUTTON (button), TRUE);
                  ctk_style_context_add_class (ctk_widget_get_style_context (button), "titlebutton");
                  ctk_style_context_add_class (ctk_widget_get_style_context (button), "appmenu");
                  image = ctk_image_new ();
                  ctk_container_add (CTK_CONTAINER (button), image);
                  ctk_widget_set_can_focus (button, FALSE);
                  ctk_widget_show_all (button);

                  accessible = ctk_widget_get_accessible (button);
                  if (CTK_IS_ACCESSIBLE (accessible))
                    atk_object_set_name (accessible, _("Application menu"));

                  priv->titlebar_icon = image;
                  if (!_ctk_header_bar_update_window_icon (bar, window))
                    ctk_image_set_from_icon_name (CTK_IMAGE (priv->titlebar_icon),
                                                  "application-x-executable-symbolic", CTK_ICON_SIZE_MENU);
                }
              else if (strcmp (t[j], "minimize") == 0 &&
                       is_sovereign_window)
                {
                  button = ctk_button_new ();
                  ctk_widget_set_valign (button, CTK_ALIGN_CENTER);
                  ctk_style_context_add_class (ctk_widget_get_style_context (button), "titlebutton");
                  ctk_style_context_add_class (ctk_widget_get_style_context (button), "minimize");
                  image = ctk_image_new_from_icon_name ("window-minimize-symbolic", CTK_ICON_SIZE_MENU);
                  g_object_set (image, "use-fallback", TRUE, NULL);
                  ctk_container_add (CTK_CONTAINER (button), image);
                  ctk_widget_set_can_focus (button, FALSE);
                  ctk_widget_show_all (button);
                  g_signal_connect_swapped (button, "clicked",
                                            G_CALLBACK (ctk_window_iconify), window);

                  accessible = ctk_widget_get_accessible (button);
                  if (CTK_IS_ACCESSIBLE (accessible))
                    atk_object_set_name (accessible, _("Minimize"));
                }
              else if (strcmp (t[j], "maximize") == 0 &&
                       ctk_window_get_resizable (window) &&
                       is_sovereign_window)
                {
                  const gchar *icon_name;
                  gboolean maximized = ctk_window_is_maximized (window);

                  icon_name = maximized ? "window-restore-symbolic" : "window-maximize-symbolic";
                  button = ctk_button_new ();
                  ctk_widget_set_valign (button, CTK_ALIGN_CENTER);
                  ctk_style_context_add_class (ctk_widget_get_style_context (button), "titlebutton");
                  ctk_style_context_add_class (ctk_widget_get_style_context (button), "maximize");
                  image = ctk_image_new_from_icon_name (icon_name, CTK_ICON_SIZE_MENU);
                  g_object_set (image, "use-fallback", TRUE, NULL);
                  ctk_container_add (CTK_CONTAINER (button), image);
                  ctk_widget_set_can_focus (button, FALSE);
                  ctk_widget_show_all (button);
                  g_signal_connect_swapped (button, "clicked",
                                            G_CALLBACK (_ctk_window_toggle_maximized), window);

                  accessible = ctk_widget_get_accessible (button);
                  if (CTK_IS_ACCESSIBLE (accessible))
                    atk_object_set_name (accessible, maximized ? _("Restore") : _("Maximize"));
                }
              else if (strcmp (t[j], "close") == 0 &&
                       ctk_window_get_deletable (window))
                {
                  button = ctk_button_new ();
                  ctk_widget_set_valign (button, CTK_ALIGN_CENTER);
                  image = ctk_image_new_from_icon_name ("window-close-symbolic", CTK_ICON_SIZE_MENU);
                  ctk_style_context_add_class (ctk_widget_get_style_context (button), "titlebutton");
                  ctk_style_context_add_class (ctk_widget_get_style_context (button), "close");
                  g_object_set (image, "use-fallback", TRUE, NULL);
                  ctk_container_add (CTK_CONTAINER (button), image);
                  ctk_widget_set_can_focus (button, FALSE);
                  ctk_widget_show_all (button);
                  g_signal_connect_swapped (button, "clicked",
                                            G_CALLBACK (ctk_window_close), window);

                  accessible = ctk_widget_get_accessible (button);
                  if (CTK_IS_ACCESSIBLE (accessible))
                    atk_object_set_name (accessible, _("Close"));
                }

              if (button)
                {
                  ctk_box_pack_start (CTK_BOX (box), button, FALSE, FALSE, 0);
                  n_children ++;
                }
            }
          g_strfreev (t);

          if (n_children == 0)
            {
              g_object_ref_sink (box);
              g_object_unref (box);
              g_object_ref_sink (separator);
              g_object_unref (separator);
              continue;
            }

          ctk_box_pack_start (CTK_BOX (box), separator, FALSE, FALSE, 0);
          if (i == 1)
            ctk_box_reorder_child (CTK_BOX (box), separator, 0);

          if ((direction == CTK_TEXT_DIR_LTR && i == 0) ||
              (direction == CTK_TEXT_DIR_RTL && i == 1))
            {
              ctk_style_context_add_class (ctk_widget_get_style_context (box), CTK_STYLE_CLASS_LEFT);
              ctk_css_node_insert_after (ctk_widget_get_css_node (CTK_WIDGET (bar)),
                                         ctk_widget_get_css_node (box),
                                         NULL);
            }
          else
            {
              ctk_style_context_add_class (ctk_widget_get_style_context (box), CTK_STYLE_CLASS_RIGHT);
              ctk_css_node_insert_before (ctk_widget_get_css_node (CTK_WIDGET (bar)),
                                          ctk_widget_get_css_node (box),
                                          NULL);
            }

          ctk_widget_show (box);
          ctk_widget_set_parent (box, CTK_WIDGET (bar));

          if (i == 0)
            {
              priv->titlebar_start_box = box;
              priv->titlebar_start_separator = separator;
            }
          else
            {
              priv->titlebar_end_box = box;
              priv->titlebar_end_separator = separator;
            }
        }
      g_strfreev (tokens);
    }
  g_free (layout_desc);

  _ctk_header_bar_update_separator_visibility (bar);
}

gboolean
_ctk_header_bar_shows_app_menu (CtkHeaderBar *bar)
{
  CtkHeaderBarPrivate *priv = ctk_header_bar_get_instance_private (bar);
  CtkWindow *window;
  gchar *layout_desc;
  gboolean ret;

  window = CTK_WINDOW (ctk_widget_get_toplevel (CTK_WIDGET (bar)));
  ctk_widget_style_get (CTK_WIDGET (window),
                        "decoration-button-layout", &layout_desc,
                        NULL);

  ret = priv->shows_wm_decorations &&
        (layout_desc && strstr (layout_desc, "menu"));

  g_free (layout_desc);

  return ret;
}

/* As an intended side effect, this function allows @child
 * to be the title/label box */
static void
ctk_header_bar_reorder_css_node (CtkHeaderBar *bar,
                                 CtkPackType   pack_type,
                                 CtkWidget    *widget)
{
  CtkHeaderBarPrivate *priv = ctk_header_bar_get_instance_private (bar);
  CtkWidget *previous_widget;
  GList *l;
  
  if (pack_type == CTK_PACK_START)
    previous_widget = priv->titlebar_start_box;
  else
    previous_widget = priv->titlebar_end_box;

  for (l = priv->children; l; l = l->next)
    {
      Child *iter = l->data;

      if (iter->widget == widget)
        break;

      if (iter->pack_type == pack_type)
        previous_widget = iter->widget;
    }

  if ((pack_type == CTK_PACK_START)
      ^ (ctk_widget_get_direction (CTK_WIDGET (bar)) == CTK_TEXT_DIR_LTR))
    ctk_css_node_insert_after (ctk_widget_get_css_node (CTK_WIDGET (bar)),
                               ctk_widget_get_css_node (widget),
                               previous_widget ? ctk_widget_get_css_node (previous_widget) : NULL);
  else
    ctk_css_node_insert_before (ctk_widget_get_css_node (CTK_WIDGET (bar)),
                                ctk_widget_get_css_node (widget),
                                previous_widget ? ctk_widget_get_css_node (previous_widget) : NULL);
}

static void
construct_label_box (CtkHeaderBar *bar)
{
  CtkHeaderBarPrivate *priv = ctk_header_bar_get_instance_private (bar);

  g_assert (priv->label_box == NULL);

  priv->label_box = create_title_box (priv->title,
                                      priv->subtitle,
                                      &priv->title_label,
                                      &priv->subtitle_label);
  ctk_header_bar_reorder_css_node (bar, CTK_PACK_START, priv->label_box);
  ctk_widget_set_parent (priv->label_box, CTK_WIDGET (bar));
}

static gint
count_visible_children (CtkHeaderBar *bar)
{
  CtkHeaderBarPrivate *priv = ctk_header_bar_get_instance_private (bar);
  GList *l;
  Child *child;
  gint n;

  n = 0;
  for (l = priv->children; l; l = l->next)
    {
      child = l->data;
      if (ctk_widget_get_visible (child->widget))
        n++;
    }

  return n;
}

static gboolean
add_child_size (CtkWidget      *child,
                CtkOrientation  orientation,
                gint           *minimum,
                gint           *natural)
{
  gint child_minimum, child_natural;

  if (!ctk_widget_get_visible (child))
    return FALSE;

  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    ctk_widget_get_preferred_width (child, &child_minimum, &child_natural);
  else
    ctk_widget_get_preferred_height (child, &child_minimum, &child_natural);

  if (CTK_ORIENTATION_HORIZONTAL == orientation)
    {
      *minimum += child_minimum;
      *natural += child_natural;
    }
  else
    {
      *minimum = MAX (*minimum, child_minimum);
      *natural = MAX (*natural, child_natural);
    }

  return TRUE;
}

static void
ctk_header_bar_get_size (CtkWidget      *widget,
                         CtkOrientation  orientation,
                         gint           *minimum_size,
                         gint           *natural_size)
{
  CtkHeaderBar *bar = CTK_HEADER_BAR (widget);
  CtkHeaderBarPrivate *priv = ctk_header_bar_get_instance_private (bar);
  GList *l;
  gint nvis_children;
  gint minimum, natural;
  gint center_min, center_nat;

  minimum = natural = 0;
  nvis_children = 0;

  for (l = priv->children; l; l = l->next)
    {
      Child *child = l->data;

      if (add_child_size (child->widget, orientation, &minimum, &natural))
        nvis_children += 1;
    }

  center_min = center_nat = 0;
  if (priv->label_box != NULL)
    {
      if (orientation == CTK_ORIENTATION_HORIZONTAL)
        add_child_size (priv->label_box, orientation, &center_min, &center_nat);
      else
        add_child_size (priv->label_sizing_box, orientation, &center_min, &center_nat);

      if (_ctk_widget_get_visible (priv->label_sizing_box))
        nvis_children += 1;
    }

  if (priv->custom_title != NULL)
    {
      if (add_child_size (priv->custom_title, orientation, &center_min, &center_nat))
        nvis_children += 1;
    }

  if (priv->titlebar_start_box != NULL)
    {
      if (add_child_size (priv->titlebar_start_box, orientation, &minimum, &natural))
        nvis_children += 1;
    }

  if (priv->titlebar_end_box != NULL)
    {
      if (add_child_size (priv->titlebar_end_box, orientation, &minimum, &natural))
        nvis_children += 1;
    }

  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    {
      minimum += center_min;
      natural += center_nat;
    }
  else
    {
      minimum = MAX (minimum, center_min);
      natural = MAX (natural, center_nat);
    }

  if (nvis_children > 0 && orientation == CTK_ORIENTATION_HORIZONTAL)
    {
      minimum += nvis_children * priv->spacing;
      natural += nvis_children * priv->spacing;
    }

  *minimum_size = minimum;
  *natural_size = natural;
}

static void
ctk_header_bar_compute_size_for_orientation (CtkWidget *widget,
                                             gint       avail_size,
                                             gint      *minimum_size,
                                             gint      *natural_size)
{
  CtkHeaderBar *bar = CTK_HEADER_BAR (widget);
  CtkHeaderBarPrivate *priv = ctk_header_bar_get_instance_private (bar);
  GList *children;
  gint required_size = 0;
  gint required_natural = 0;
  gint child_size;
  gint child_natural;
  gint nvis_children;

  nvis_children = 0;

  for (children = priv->children; children != NULL; children = children->next)
    {
      Child *child = children->data;

      if (ctk_widget_get_visible (child->widget))
        {
          ctk_widget_get_preferred_width_for_height (child->widget,
                                                     avail_size, &child_size, &child_natural);

          required_size += child_size;
          required_natural += child_natural;

          nvis_children += 1;
        }
    }

  if (priv->label_box != NULL)
    {
      ctk_widget_get_preferred_width (priv->label_sizing_box,
                                      &child_size, &child_natural);
      required_size += child_size;
      required_natural += child_natural;
    }

  if (priv->custom_title != NULL &&
      ctk_widget_get_visible (priv->custom_title))
    {
      ctk_widget_get_preferred_width (priv->custom_title,
                                      &child_size, &child_natural);
      required_size += child_size;
      required_natural += child_natural;
    }

  if (priv->titlebar_start_box != NULL)
    {
      ctk_widget_get_preferred_width (priv->titlebar_start_box,
                                      &child_size, &child_natural);
      required_size += child_size;
      required_natural += child_natural;
      nvis_children += 1;
    }

  if (priv->titlebar_end_box != NULL)
    {
      ctk_widget_get_preferred_width (priv->titlebar_end_box,
                                      &child_size, &child_natural);
      required_size += child_size;
      required_natural += child_natural;
      nvis_children += 1;
    }

  if (nvis_children > 0)
    {
      required_size += nvis_children * priv->spacing;
      required_natural += nvis_children * priv->spacing;
    }

  *minimum_size = required_size;
  *natural_size = required_natural;
}

static void
ctk_header_bar_compute_size_for_opposing_orientation (CtkWidget *widget,
                                                      gint       avail_size,
                                                      gint      *minimum_size,
                                                      gint      *natural_size)
{
  CtkHeaderBar *bar = CTK_HEADER_BAR (widget);
  CtkHeaderBarPrivate *priv = ctk_header_bar_get_instance_private (bar);
  Child *child;
  GList *children;
  gint nvis_children;
  gint computed_minimum = 0;
  gint computed_natural = 0;
  CtkRequestedSize *sizes;
  CtkPackType packing;
  gint size = 0;
  gint i;
  gint child_size;
  gint child_minimum;
  gint child_natural;
  gint center_min, center_nat;

  nvis_children = count_visible_children (bar);

  if (nvis_children <= 0)
    return;

  sizes = g_newa (CtkRequestedSize, nvis_children);

  /* Retrieve desired size for visible children */
  for (i = 0, children = priv->children; children; children = children->next)
    {
      child = children->data;

      if (ctk_widget_get_visible (child->widget))
        {
          ctk_widget_get_preferred_width (child->widget,
                                          &sizes[i].minimum_size,
                                          &sizes[i].natural_size);

          size -= sizes[i].minimum_size;
          sizes[i].data = child;
          i += 1;
        }
    }

  /* Bring children up to size first */
  size = ctk_distribute_natural_allocation (MAX (0, avail_size), nvis_children, sizes);

  /* Allocate child positions. */
  for (packing = CTK_PACK_START; packing <= CTK_PACK_END; ++packing)
    {
      for (i = 0, children = priv->children; children; children = children->next)
        {
          child = children->data;

          /* If widget is not visible, skip it. */
          if (!ctk_widget_get_visible (child->widget))
            continue;

          /* If widget is packed differently skip it, but still increment i,
           * since widget is visible and will be handled in next loop
           * iteration.
           */
          if (child->pack_type != packing)
            {
              i++;
              continue;
            }

          child_size = sizes[i].minimum_size;

          ctk_widget_get_preferred_height_for_width (child->widget,
                                                     child_size, &child_minimum, &child_natural);

          computed_minimum = MAX (computed_minimum, child_minimum);
          computed_natural = MAX (computed_natural, child_natural);
        }
      i += 1;
    }

  center_min = center_nat = 0;
  if (priv->label_box != NULL)
    {
      ctk_widget_get_preferred_height (priv->label_sizing_box,
                                       &center_min, &center_nat);
    }

  if (priv->custom_title != NULL &&
      ctk_widget_get_visible (priv->custom_title))
    {
      ctk_widget_get_preferred_height (priv->custom_title,
                                       &center_min, &center_nat);
    }

  if (priv->titlebar_start_box != NULL)
    {
      ctk_widget_get_preferred_height (priv->titlebar_start_box,
                                       &child_minimum, &child_natural);
      computed_minimum = MAX (computed_minimum, child_minimum);
      computed_natural = MAX (computed_natural, child_natural);
    }

  if (priv->titlebar_end_box != NULL)
    {
      ctk_widget_get_preferred_height (priv->titlebar_end_box,
                                       &child_minimum, &child_natural);
      computed_minimum = MAX (computed_minimum, child_minimum);
      computed_natural = MAX (computed_natural, child_natural);
    }

  *minimum_size = computed_minimum;
  *natural_size = computed_natural;
}

static void
ctk_header_bar_get_content_size (CtkCssGadget   *gadget,
                                 CtkOrientation  orientation,
                                 gint            for_size,
                                 gint           *minimum,
                                 gint           *natural,
                                 gint           *minimum_baseline,
                                 gint           *natural_baseline,
                                 gpointer        unused)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);

  if (for_size < 0)
    ctk_header_bar_get_size (widget, orientation, minimum, natural);
  else if (orientation == CTK_ORIENTATION_HORIZONTAL)
    ctk_header_bar_compute_size_for_orientation (widget, for_size, minimum, natural);
  else
    ctk_header_bar_compute_size_for_opposing_orientation (widget, for_size, minimum, natural);
}

static void
ctk_header_bar_get_preferred_width (CtkWidget *widget,
                                    gint      *minimum,
                                    gint      *natural)
{
  CtkHeaderBarPrivate *priv = ctk_header_bar_get_instance_private (CTK_HEADER_BAR (widget));

  ctk_css_gadget_get_preferred_size (priv->gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     -1,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_header_bar_get_preferred_height (CtkWidget *widget,
                                     gint      *minimum,
                                     gint      *natural)
{
  CtkHeaderBarPrivate *priv = ctk_header_bar_get_instance_private (CTK_HEADER_BAR (widget));

  ctk_css_gadget_get_preferred_size (priv->gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     -1,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_header_bar_get_preferred_width_for_height (CtkWidget *widget,
                                               gint       height,
                                               gint      *minimum,
                                               gint      *natural)
{
  CtkHeaderBarPrivate *priv = ctk_header_bar_get_instance_private (CTK_HEADER_BAR (widget));

  ctk_css_gadget_get_preferred_size (priv->gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     height,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_header_bar_get_preferred_height_for_width (CtkWidget *widget,
                                               gint       width,
                                               gint      *minimum,
                                               gint      *natural)
{
  CtkHeaderBarPrivate *priv = ctk_header_bar_get_instance_private (CTK_HEADER_BAR (widget));

  ctk_css_gadget_get_preferred_size (priv->gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     width,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_header_bar_size_allocate (CtkWidget     *widget,
                              CtkAllocation *allocation)
{
  CtkHeaderBarPrivate *priv = ctk_header_bar_get_instance_private (CTK_HEADER_BAR (widget));
  CtkAllocation clip;

  ctk_widget_set_allocation (widget, allocation);

  ctk_css_gadget_allocate (priv->gadget, allocation, ctk_widget_get_allocated_baseline (widget), &clip);

  ctk_widget_set_clip (widget, &clip);
}

static void
ctk_header_bar_allocate_contents (CtkCssGadget        *gadget,
                                  const CtkAllocation *allocation,
                                  int                  baseline,
                                  CtkAllocation       *out_clip,
                                  gpointer             unused)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);
  CtkWidget *title_widget;
  CtkHeaderBar *bar = CTK_HEADER_BAR (widget);
  CtkHeaderBarPrivate *priv = ctk_header_bar_get_instance_private (bar);
  CtkRequestedSize *sizes;
  gint width, height;
  gint nvis_children;
  gint title_minimum_size;
  gint title_natural_size;
  gboolean title_expands = FALSE;
  gint start_width, end_width;
  gint uniform_expand_bonus[2] = { 0 };
  gint leftover_expand_bonus[2] = { 0 };
  gint nexpand_children[2] = { 0 };
  gint side[2];
  GList *l;
  gint i;
  Child *child;
  CtkPackType packing;
  CtkAllocation child_allocation;
  gint x;
  gint child_size;
  CtkTextDirection direction;

  direction = ctk_widget_get_direction (widget);
  nvis_children = count_visible_children (bar);
  sizes = g_newa (CtkRequestedSize, nvis_children);

  width = allocation->width - nvis_children * priv->spacing;
  height = allocation->height;

  i = 0;
  for (l = priv->children; l; l = l->next)
    {
      child = l->data;
      if (!ctk_widget_get_visible (child->widget))
        continue;

      if (ctk_widget_compute_expand (child->widget, CTK_ORIENTATION_HORIZONTAL))
        nexpand_children[child->pack_type]++;

      ctk_widget_get_preferred_width_for_height (child->widget,
                                                 height,
                                                 &sizes[i].minimum_size,
                                                 &sizes[i].natural_size);
      width -= sizes[i].minimum_size;
      i++;
    }

  title_minimum_size = 0;
  title_natural_size = 0;

  if (priv->custom_title != NULL &&
      ctk_widget_get_visible (priv->custom_title))
    title_widget = priv->custom_title;
  else if (priv->label_box != NULL)
    title_widget = priv->label_box;
  else
    title_widget = NULL;

  if (title_widget)
    {
      ctk_widget_get_preferred_width_for_height (title_widget,
                                                 height,
                                                 &title_minimum_size,
                                                 &title_natural_size);
      width -= title_natural_size;

      title_expands = ctk_widget_compute_expand (title_widget, CTK_ORIENTATION_HORIZONTAL);
    }

  start_width = 0;
  if (priv->titlebar_start_box != NULL)
    {
      gint min, nat;
      ctk_widget_get_preferred_width_for_height (priv->titlebar_start_box,
                                                 height,
                                                 &min, &nat);
      start_width = nat + priv->spacing;
    }
  width -= start_width;

  end_width = 0;
  if (priv->titlebar_end_box != NULL)
    {
      gint min, nat;
      ctk_widget_get_preferred_width_for_height (priv->titlebar_end_box,
                                                 height,
                                                 &min, &nat);
      end_width = nat + priv->spacing;
    }
  width -= end_width;

  width = ctk_distribute_natural_allocation (MAX (0, width), nvis_children, sizes);

  /* compute the nominal size of the children filling up each side of
   * the title in titlebar
   */
  side[0] = start_width;
  side[1] = end_width;
  for (packing = CTK_PACK_START; packing <= CTK_PACK_END; packing++)
    {
      i = 0;
      for (l = priv->children; l != NULL; l = l->next)
        {
          child = l->data;
          if (!ctk_widget_get_visible (child->widget))
            continue;

          if (child->pack_type == packing)
            side[packing] += sizes[i].minimum_size + priv->spacing;

          i++;
        }
    }

  /* figure out how much space is left on each side of the title,
   * and earkmark that space for the expanded children.
   *
   * If the title itself is expanded, then it gets half the spoils
   * from each side.
   */
  for (packing = CTK_PACK_START; packing <= CTK_PACK_END; packing++)
    {
      gint side_free_space;

      side_free_space = allocation->width / 2 - title_natural_size / 2 - side[packing];

      if (side_free_space > 0 && nexpand_children[packing] > 0)
        {
          width -= side_free_space;

          if (title_expands)
            side_free_space -= side_free_space / 2;

          side[packing] += side_free_space;
          uniform_expand_bonus[packing] = side_free_space / nexpand_children[packing];
          leftover_expand_bonus[packing] = side_free_space % nexpand_children[packing];
        }
    }

  /* allocate the children on both sides of the title */
  for (packing = CTK_PACK_START; packing <= CTK_PACK_END; packing++)
    {
      child_allocation.y = allocation->y;
      child_allocation.height = height;
      if (packing == CTK_PACK_START)
        x = allocation->x + start_width;
      else
        x = allocation->x + allocation->width - end_width;

      i = 0;
      for (l = priv->children; l != NULL; l = l->next)
        {
          child = l->data;
          if (!ctk_widget_get_visible (child->widget))
            continue;

          if (child->pack_type != packing)
            goto next;

          child_size = sizes[i].minimum_size;

          /* if this child is expanded, give it extra space from the reserves */
          if (ctk_widget_compute_expand (child->widget, CTK_ORIENTATION_HORIZONTAL))
            {
              gint expand_bonus;

              expand_bonus = uniform_expand_bonus[packing];

              if (leftover_expand_bonus[packing] > 0)
                {
                  expand_bonus++;
                  leftover_expand_bonus[packing]--;
                }

              child_size += expand_bonus;
            }

          child_allocation.width = child_size;

          if (packing == CTK_PACK_START)
            {
              child_allocation.x = x;
              x += child_size;
              x += priv->spacing;
            }
          else
            {
              x -= child_size;
              child_allocation.x = x;
              x -= priv->spacing;
            }

          if (direction == CTK_TEXT_DIR_RTL)
            child_allocation.x = allocation->x + allocation->width - (child_allocation.x - allocation->x) - child_allocation.width;

          ctk_widget_size_allocate (child->widget, &child_allocation);

        next:
          i++;
        }
    }

  /* We don't enforce css borders on the center widget, to make
   * title/subtitle combinations fit without growing the header
   */
  child_allocation.y = allocation->y;
  child_allocation.height = allocation->height;

  child_size = MIN (allocation->width - side[0] - side[1], title_natural_size);

  child_allocation.x = allocation->x + (allocation->width - child_size) / 2;
  child_allocation.width = child_size;

  /* if the title widget is expanded, then grow it by all the available
   * free space, and recenter it
   */
  if (title_expands && width > 0)
    {
      child_allocation.width += width;
      child_allocation.x -= width / 2;
    }

  if (allocation->x + side[0] > child_allocation.x)
    child_allocation.x = allocation->x + side[0];
  else if (allocation->x + allocation->width - side[1] < child_allocation.x + child_allocation.width)
    child_allocation.x = allocation->x + allocation->width - side[1] - child_allocation.width;

  if (direction == CTK_TEXT_DIR_RTL)
    child_allocation.x = allocation->x + allocation->width - (child_allocation.x - allocation->x) - child_allocation.width;

  if (title_widget != NULL)
    ctk_widget_size_allocate (title_widget, &child_allocation);

  child_allocation.y = allocation->y;
  child_allocation.height = height;

  if (priv->titlebar_start_box)
    {
      gboolean left = (direction == CTK_TEXT_DIR_LTR);
      if (left)
        child_allocation.x = allocation->x;
      else
        child_allocation.x = allocation->x + allocation->width - start_width + priv->spacing;
      child_allocation.width = start_width - priv->spacing;
      ctk_widget_size_allocate (priv->titlebar_start_box, &child_allocation);
    }

  if (priv->titlebar_end_box)
    {
      gboolean left = (direction != CTK_TEXT_DIR_LTR);
      if (left)
        child_allocation.x = allocation->x;
      else
        child_allocation.x = allocation->x + allocation->width - end_width + priv->spacing;
      child_allocation.width = end_width - priv->spacing;
      ctk_widget_size_allocate (priv->titlebar_end_box, &child_allocation);
    }

  ctk_container_get_children_clip (CTK_CONTAINER (widget), out_clip);
}

/**
 * ctk_header_bar_set_title:
 * @bar: a #CtkHeaderBar
 * @title: (allow-none): a title, or %NULL
 *
 * Sets the title of the #CtkHeaderBar. The title should help a user
 * identify the current view. A good title should not include the
 * application name.
 *
 * Since: 3.10
 */
void
ctk_header_bar_set_title (CtkHeaderBar *bar,
                          const gchar  *title)
{
  CtkHeaderBarPrivate *priv = ctk_header_bar_get_instance_private (bar);
  gchar *new_title;

  g_return_if_fail (CTK_IS_HEADER_BAR (bar));

  new_title = g_strdup (title);
  g_free (priv->title);
  priv->title = new_title;

  if (priv->title_label != NULL)
    {
      ctk_label_set_label (CTK_LABEL (priv->title_label), priv->title);
      ctk_widget_queue_resize (CTK_WIDGET (bar));
    }

  g_object_notify_by_pspec (G_OBJECT (bar), header_bar_props[PROP_TITLE]);
}

/**
 * ctk_header_bar_get_title:
 * @bar: a #CtkHeaderBar
 *
 * Retrieves the title of the header. See ctk_header_bar_set_title().
 *
 * Returns: (nullable): the title of the header, or %NULL if none has
 *    been set explicitly. The returned string is owned by the widget
 *    and must not be modified or freed.
 *
 * Since: 3.10
 */
const gchar *
ctk_header_bar_get_title (CtkHeaderBar *bar)
{
  CtkHeaderBarPrivate *priv = ctk_header_bar_get_instance_private (bar);

  g_return_val_if_fail (CTK_IS_HEADER_BAR (bar), NULL);

  return priv->title;
}

/**
 * ctk_header_bar_set_subtitle:
 * @bar: a #CtkHeaderBar
 * @subtitle: (allow-none): a subtitle, or %NULL
 *
 * Sets the subtitle of the #CtkHeaderBar. The title should give a user
 * an additional detail to help him identify the current view.
 *
 * Note that CtkHeaderBar by default reserves room for the subtitle,
 * even if none is currently set. If this is not desired, set the
 * #CtkHeaderBar:has-subtitle property to %FALSE.
 *
 * Since: 3.10
 */
void
ctk_header_bar_set_subtitle (CtkHeaderBar *bar,
                             const gchar  *subtitle)
{
  CtkHeaderBarPrivate *priv = ctk_header_bar_get_instance_private (bar);
  gchar *new_subtitle;

  g_return_if_fail (CTK_IS_HEADER_BAR (bar));

  new_subtitle = g_strdup (subtitle);
  g_free (priv->subtitle);
  priv->subtitle = new_subtitle;

  if (priv->subtitle_label != NULL)
    {
      ctk_label_set_label (CTK_LABEL (priv->subtitle_label), priv->subtitle);
      ctk_widget_set_visible (priv->subtitle_label, priv->subtitle && priv->subtitle[0]);
      ctk_widget_queue_resize (CTK_WIDGET (bar));
    }

  ctk_widget_set_visible (priv->subtitle_sizing_label, priv->has_subtitle || (priv->subtitle && priv->subtitle[0]));

  g_object_notify_by_pspec (G_OBJECT (bar), header_bar_props[PROP_SUBTITLE]);
}

/**
 * ctk_header_bar_get_subtitle:
 * @bar: a #CtkHeaderBar
 *
 * Retrieves the subtitle of the header. See ctk_header_bar_set_subtitle().
 *
 * Returns: (nullable): the subtitle of the header, or %NULL if none has
 *    been set explicitly. The returned string is owned by the widget
 *    and must not be modified or freed.
 *
 * Since: 3.10
 */
const gchar *
ctk_header_bar_get_subtitle (CtkHeaderBar *bar)
{
  CtkHeaderBarPrivate *priv = ctk_header_bar_get_instance_private (bar);

  g_return_val_if_fail (CTK_IS_HEADER_BAR (bar), NULL);

  return priv->subtitle;
}

/**
 * ctk_header_bar_set_custom_title:
 * @bar: a #CtkHeaderBar
 * @title_widget: (allow-none): a custom widget to use for a title
 *
 * Sets a custom title for the #CtkHeaderBar.
 *
 * The title should help a user identify the current view. This
 * supersedes any title set by ctk_header_bar_set_title() or
 * ctk_header_bar_set_subtitle(). To achieve the same style as
 * the builtin title and subtitle, use the “title” and “subtitle”
 * style classes.
 *
 * You should set the custom title to %NULL, for the header title
 * label to be visible again.
 *
 * Since: 3.10
 */
void
ctk_header_bar_set_custom_title (CtkHeaderBar *bar,
                                 CtkWidget    *title_widget)
{
  CtkHeaderBarPrivate *priv = ctk_header_bar_get_instance_private (bar);

  g_return_if_fail (CTK_IS_HEADER_BAR (bar));
  if (title_widget)
    g_return_if_fail (CTK_IS_WIDGET (title_widget));

  /* No need to do anything if the custom widget stays the same */
  if (priv->custom_title == title_widget)
    return;

  if (priv->custom_title)
    {
      CtkWidget *custom = priv->custom_title;

      priv->custom_title = NULL;
      ctk_widget_unparent (custom);
    }

  if (title_widget != NULL)
    {
      priv->custom_title = title_widget;

      ctk_header_bar_reorder_css_node (bar, CTK_PACK_START, priv->custom_title);
      ctk_widget_set_parent (priv->custom_title, CTK_WIDGET (bar));
      ctk_widget_set_valign (priv->custom_title, CTK_ALIGN_CENTER);

      if (priv->label_box != NULL)
        {
          CtkWidget *label_box = priv->label_box;

          priv->label_box = NULL;
          priv->title_label = NULL;
          priv->subtitle_label = NULL;
          ctk_widget_unparent (label_box);
        }

    }
  else
    {
      if (priv->label_box == NULL)
        construct_label_box (bar);
    }

  ctk_widget_queue_resize (CTK_WIDGET (bar));

  g_object_notify_by_pspec (G_OBJECT (bar), header_bar_props[PROP_CUSTOM_TITLE]);
}

/**
 * ctk_header_bar_get_custom_title:
 * @bar: a #CtkHeaderBar
 *
 * Retrieves the custom title widget of the header. See
 * ctk_header_bar_set_custom_title().
 *
 * Returns: (nullable) (transfer none): the custom title widget
 *    of the header, or %NULL if none has been set explicitly.
 *
 * Since: 3.10
 */
CtkWidget *
ctk_header_bar_get_custom_title (CtkHeaderBar *bar)
{
  CtkHeaderBarPrivate *priv = ctk_header_bar_get_instance_private (bar);

  g_return_val_if_fail (CTK_IS_HEADER_BAR (bar), NULL);

  return priv->custom_title;
}

static void
ctk_header_bar_destroy (CtkWidget *widget)
{
  CtkHeaderBarPrivate *priv = ctk_header_bar_get_instance_private (CTK_HEADER_BAR (widget));

  if (priv->label_sizing_box)
    {
      ctk_widget_destroy (priv->label_sizing_box);
      g_clear_object (&priv->label_sizing_box);
    }

  if (priv->custom_title)
    {
      ctk_widget_unparent (priv->custom_title);
      priv->custom_title = NULL;
    }

  if (priv->label_box)
    {
      ctk_widget_unparent (priv->label_box);
      priv->label_box = NULL;
    }

  if (priv->titlebar_start_box)
    {
      ctk_widget_unparent (priv->titlebar_start_box);
      priv->titlebar_start_box = NULL;
      priv->titlebar_start_separator = NULL;
    }

  if (priv->titlebar_end_box)
    {
      ctk_widget_unparent (priv->titlebar_end_box);
      priv->titlebar_end_box = NULL;
      priv->titlebar_end_separator = NULL;
    }

  CTK_WIDGET_CLASS (ctk_header_bar_parent_class)->destroy (widget);
}

static void
ctk_header_bar_finalize (GObject *object)
{
  CtkHeaderBarPrivate *priv = ctk_header_bar_get_instance_private (CTK_HEADER_BAR (object));

  g_free (priv->title);
  g_free (priv->subtitle);
  g_free (priv->decoration_layout);

  g_clear_object (&priv->gadget);

  G_OBJECT_CLASS (ctk_header_bar_parent_class)->finalize (object);
}

static void
ctk_header_bar_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  CtkHeaderBar *bar = CTK_HEADER_BAR (object);
  CtkHeaderBarPrivate *priv = ctk_header_bar_get_instance_private (bar);

  switch (prop_id)
    {
    case PROP_TITLE:
      g_value_set_string (value, priv->title);
      break;

    case PROP_SUBTITLE:
      g_value_set_string (value, priv->subtitle);
      break;

    case PROP_CUSTOM_TITLE:
      g_value_set_object (value, priv->custom_title);
      break;

    case PROP_SPACING:
      g_value_set_int (value, priv->spacing);
      break;

    case PROP_SHOW_CLOSE_BUTTON:
      g_value_set_boolean (value, ctk_header_bar_get_show_close_button (bar));
      break;

    case PROP_HAS_SUBTITLE:
      g_value_set_boolean (value, ctk_header_bar_get_has_subtitle (bar));
      break;

    case PROP_DECORATION_LAYOUT:
      g_value_set_string (value, ctk_header_bar_get_decoration_layout (bar));
      break;

    case PROP_DECORATION_LAYOUT_SET:
      g_value_set_boolean (value, priv->decoration_layout_set);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_header_bar_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  CtkHeaderBar *bar = CTK_HEADER_BAR (object);
  CtkHeaderBarPrivate *priv = ctk_header_bar_get_instance_private (bar);

  switch (prop_id)
    {
    case PROP_TITLE:
      ctk_header_bar_set_title (bar, g_value_get_string (value));
      break;

    case PROP_SUBTITLE:
      ctk_header_bar_set_subtitle (bar, g_value_get_string (value));
      break;

    case PROP_CUSTOM_TITLE:
      ctk_header_bar_set_custom_title (bar, g_value_get_object (value));
      break;

    case PROP_SPACING:
      if (priv->spacing != g_value_get_int (value))
        {
          priv->spacing = g_value_get_int (value);
          ctk_widget_queue_resize (CTK_WIDGET (bar));
          g_object_notify_by_pspec (object, pspec);
        }
      break;

    case PROP_SHOW_CLOSE_BUTTON:
      ctk_header_bar_set_show_close_button (bar, g_value_get_boolean (value));
      break;

    case PROP_HAS_SUBTITLE:
      ctk_header_bar_set_has_subtitle (bar, g_value_get_boolean (value));
      break;

    case PROP_DECORATION_LAYOUT:
      ctk_header_bar_set_decoration_layout (bar, g_value_get_string (value));
      break;

    case PROP_DECORATION_LAYOUT_SET:
      priv->decoration_layout_set = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
notify_child_cb (GObject      *child,
                 GParamSpec   *pspec,
                 CtkHeaderBar *bar)
{
  _ctk_header_bar_update_separator_visibility (bar);
}

static void
ctk_header_bar_pack (CtkHeaderBar *bar,
                     CtkWidget    *widget,
                     CtkPackType   pack_type)
{
  CtkHeaderBarPrivate *priv = ctk_header_bar_get_instance_private (bar);
  Child *child;

  g_return_if_fail (ctk_widget_get_parent (widget) == NULL);

  child = g_new (Child, 1);
  child->widget = widget;
  child->pack_type = pack_type;

  priv->children = g_list_append (priv->children, child);

  ctk_widget_freeze_child_notify (widget);
  ctk_header_bar_reorder_css_node (bar, CTK_PACK_START, widget);
  ctk_widget_set_parent (widget, CTK_WIDGET (bar));
  g_signal_connect (widget, "notify::visible", G_CALLBACK (notify_child_cb), bar);
  ctk_widget_child_notify (widget, "pack-type");
  ctk_widget_child_notify (widget, "position");
  ctk_widget_thaw_child_notify (widget);

  _ctk_header_bar_update_separator_visibility (bar);
}

static void
ctk_header_bar_add (CtkContainer *container,
                    CtkWidget    *child)
{
  ctk_header_bar_pack (CTK_HEADER_BAR (container), child, CTK_PACK_START);
}

static GList *
find_child_link (CtkHeaderBar *bar,
                 CtkWidget    *widget,
                 gint         *position)
{
  CtkHeaderBarPrivate *priv = ctk_header_bar_get_instance_private (bar);
  GList *l;
  Child *child;
  gint i;

  for (l = priv->children, i = 0; l; l = l->next, i++)
    {
      child = l->data;
      if (child->widget == widget)
        {
          if (position)
            *position = i;
          return l;
        }
    }

  return NULL;
}

static void
ctk_header_bar_remove (CtkContainer *container,
                       CtkWidget    *widget)
{
  CtkHeaderBar *bar = CTK_HEADER_BAR (container);
  CtkHeaderBarPrivate *priv = ctk_header_bar_get_instance_private (bar);
  GList *l;
  Child *child;

  l = find_child_link (bar, widget, NULL);
  if (l)
    {
      child = l->data;
      g_signal_handlers_disconnect_by_func (widget, notify_child_cb, bar);
      ctk_widget_unparent (child->widget);
      priv->children = g_list_delete_link (priv->children, l);
      g_free (child);
      ctk_widget_queue_resize (CTK_WIDGET (container));
      _ctk_header_bar_update_separator_visibility (bar);
    }
}

static void
ctk_header_bar_forall (CtkContainer *container,
                       gboolean      include_internals,
                       CtkCallback   callback,
                       gpointer      callback_data)
{
  CtkHeaderBar *bar = CTK_HEADER_BAR (container);
  CtkHeaderBarPrivate *priv = ctk_header_bar_get_instance_private (bar);
  Child *child;
  GList *children;

  if (include_internals && priv->titlebar_start_box != NULL)
    (* callback) (priv->titlebar_start_box, callback_data);

  children = priv->children;
  while (children)
    {
      child = children->data;
      children = children->next;
      if (child->pack_type == CTK_PACK_START)
        (* callback) (child->widget, callback_data);
    }

  if (priv->custom_title != NULL)
    (* callback) (priv->custom_title, callback_data);

  if (include_internals && priv->label_box != NULL)
    (* callback) (priv->label_box, callback_data);

  children = priv->children;
  while (children)
    {
      child = children->data;
      children = children->next;
      if (child->pack_type == CTK_PACK_END)
        (* callback) (child->widget, callback_data);
    }

  if (include_internals && priv->titlebar_end_box != NULL)
    (* callback) (priv->titlebar_end_box, callback_data);
}

static void
ctk_header_bar_reorder_child (CtkHeaderBar *bar,
                              CtkWidget    *widget,
                              gint          position)
{
  CtkHeaderBarPrivate *priv = ctk_header_bar_get_instance_private (bar);
  GList *l;
  gint old_position;
  Child *child;

  l = find_child_link (bar, widget, &old_position);

  if (l == NULL)
    return;

  if (old_position == position)
    return;

  child = l->data; 
  priv->children = g_list_delete_link (priv->children, l);

  if (position < 0)
    l = NULL;
  else
    l = g_list_nth (priv->children, position);

  priv->children = g_list_insert_before (priv->children, l, child);
  ctk_header_bar_reorder_css_node (bar, child->pack_type, widget);
  ctk_widget_child_notify (widget, "position");
  ctk_widget_queue_resize (widget);
}

static GType
ctk_header_bar_child_type (CtkContainer *container)
{
  return CTK_TYPE_WIDGET;
}

static void
ctk_header_bar_get_child_property (CtkContainer *container,
                                   CtkWidget    *widget,
                                   guint         property_id,
                                   GValue       *value,
                                   GParamSpec   *pspec)
{
  CtkHeaderBar *bar = CTK_HEADER_BAR (container);
  CtkHeaderBarPrivate *priv = ctk_header_bar_get_instance_private (bar);
  GList *l;
  Child *child;

  l = find_child_link (bar, widget, NULL);
  if (l == NULL)
    {
      g_param_value_set_default (pspec, value);
      return;
    }

  child = l->data;

  switch (property_id)
    {
    case CHILD_PROP_PACK_TYPE:
      g_value_set_enum (value, child->pack_type);
      break;

    case CHILD_PROP_POSITION:
      g_value_set_int (value, g_list_position (priv->children, l));
      break;

    default:
      CTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      break;
    }
}

static void
ctk_header_bar_set_child_property (CtkContainer *container,
                                   CtkWidget    *widget,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  CtkHeaderBar *bar = CTK_HEADER_BAR (container);
  GList *l;
  Child *child;

  l = find_child_link (bar, widget, NULL);
  if (l == NULL)
    return;

  child = l->data;

  switch (property_id)
    {
    case CHILD_PROP_PACK_TYPE:
      child->pack_type = g_value_get_enum (value);
      _ctk_header_bar_update_separator_visibility (bar);
      ctk_widget_queue_resize (widget);
      break;

    case CHILD_PROP_POSITION:
      ctk_header_bar_reorder_child (bar, widget, g_value_get_int (value));
      break;

    default:
      CTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      break;
    }
}

static gint
ctk_header_bar_draw (CtkWidget *widget,
                     cairo_t   *cr)
{
  CtkHeaderBarPrivate *priv = ctk_header_bar_get_instance_private (CTK_HEADER_BAR (widget));

  ctk_css_gadget_draw (priv->gadget, cr);

  return FALSE;
}

static gboolean
ctk_header_bar_render_contents (CtkCssGadget *gadget,
                                cairo_t      *cr,
                                int           x,
                                int           y,
                                int           width,
                                int           height,
                                gpointer      unused)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);

  CTK_WIDGET_CLASS (ctk_header_bar_parent_class)->draw (widget, cr);

  return FALSE;
}

static void
ctk_header_bar_realize (CtkWidget *widget)
{
  CtkSettings *settings;

  CTK_WIDGET_CLASS (ctk_header_bar_parent_class)->realize (widget);

  settings = ctk_widget_get_settings (widget);
  g_signal_connect_swapped (settings, "notify::ctk-shell-shows-app-menu",
                            G_CALLBACK (_ctk_header_bar_update_window_buttons), widget);
  g_signal_connect_swapped (settings, "notify::ctk-decoration-layout",
                            G_CALLBACK (_ctk_header_bar_update_window_buttons), widget);
  _ctk_header_bar_update_window_buttons (CTK_HEADER_BAR (widget));
}

static void
ctk_header_bar_unrealize (CtkWidget *widget)
{
  CtkSettings *settings;

  settings = ctk_widget_get_settings (widget);

  g_signal_handlers_disconnect_by_func (settings, _ctk_header_bar_update_window_buttons, widget);

  CTK_WIDGET_CLASS (ctk_header_bar_parent_class)->unrealize (widget);
}

static gboolean
window_state_changed (CtkWidget           *window,
                      CdkEventWindowState *event,
                      gpointer             data)
{
  CtkHeaderBar *bar = CTK_HEADER_BAR (data);

  if (event->changed_mask & (GDK_WINDOW_STATE_FULLSCREEN |
                             GDK_WINDOW_STATE_MAXIMIZED |
                             GDK_WINDOW_STATE_TILED |
                             GDK_WINDOW_STATE_TOP_TILED |
                             GDK_WINDOW_STATE_RIGHT_TILED |
                             GDK_WINDOW_STATE_BOTTOM_TILED |
                             GDK_WINDOW_STATE_LEFT_TILED))
    _ctk_header_bar_update_window_buttons (bar);

  return FALSE;
}

static void
ctk_header_bar_hierarchy_changed (CtkWidget *widget,
                                  CtkWidget *previous_toplevel)
{
  CtkWidget *toplevel;
  CtkHeaderBar *bar = CTK_HEADER_BAR (widget);

  toplevel = ctk_widget_get_toplevel (widget);

  if (previous_toplevel)
    g_signal_handlers_disconnect_by_func (previous_toplevel,
                                          window_state_changed, widget);

  if (toplevel)
    g_signal_connect_after (toplevel, "window-state-event",
                            G_CALLBACK (window_state_changed), widget);

  _ctk_header_bar_update_window_buttons (bar);
}

static void
ctk_header_bar_direction_changed (CtkWidget        *widget,
                                  CtkTextDirection  previous_direction)
{
  CTK_WIDGET_CLASS (ctk_header_bar_parent_class)->direction_changed (widget, previous_direction);

  ctk_css_node_reverse_children (ctk_widget_get_css_node (widget));
}

static void
ctk_header_bar_class_init (CtkHeaderBarClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (class);
  CtkContainerClass *container_class = CTK_CONTAINER_CLASS (class);

  object_class->finalize = ctk_header_bar_finalize;
  object_class->get_property = ctk_header_bar_get_property;
  object_class->set_property = ctk_header_bar_set_property;

  widget_class->destroy = ctk_header_bar_destroy;
  widget_class->size_allocate = ctk_header_bar_size_allocate;
  widget_class->get_preferred_width = ctk_header_bar_get_preferred_width;
  widget_class->get_preferred_height = ctk_header_bar_get_preferred_height;
  widget_class->get_preferred_height_for_width = ctk_header_bar_get_preferred_height_for_width;
  widget_class->get_preferred_width_for_height = ctk_header_bar_get_preferred_width_for_height;
  widget_class->draw = ctk_header_bar_draw;
  widget_class->realize = ctk_header_bar_realize;
  widget_class->unrealize = ctk_header_bar_unrealize;
  widget_class->hierarchy_changed = ctk_header_bar_hierarchy_changed;
  widget_class->direction_changed = ctk_header_bar_direction_changed;

  container_class->add = ctk_header_bar_add;
  container_class->remove = ctk_header_bar_remove;
  container_class->forall = ctk_header_bar_forall;
  container_class->child_type = ctk_header_bar_child_type;
  container_class->set_child_property = ctk_header_bar_set_child_property;
  container_class->get_child_property = ctk_header_bar_get_child_property;
  ctk_container_class_handle_border_width (container_class);

  ctk_container_class_install_child_property (container_class,
                                              CHILD_PROP_PACK_TYPE,
                                              g_param_spec_enum ("pack-type",
                                                                 P_("Pack type"),
                                                                 P_("A CtkPackType indicating whether the child is packed with reference to the start or end of the parent"),
                                                                 CTK_TYPE_PACK_TYPE, CTK_PACK_START,
                                                                 CTK_PARAM_READWRITE));
  ctk_container_class_install_child_property (container_class,
                                              CHILD_PROP_POSITION,
                                              g_param_spec_int ("position",
                                                                P_("Position"),
                                                                P_("The index of the child in the parent"),
                                                                -1, G_MAXINT, 0,
                                                                CTK_PARAM_READWRITE));

  header_bar_props[PROP_TITLE] =
      g_param_spec_string ("title",
                           P_("Title"),
                           P_("The title to display"),
                           NULL,
                           G_PARAM_READWRITE);

  header_bar_props[PROP_SUBTITLE] =
      g_param_spec_string ("subtitle",
                           P_("Subtitle"),
                           P_("The subtitle to display"),
                           NULL,
                           G_PARAM_READWRITE);

  header_bar_props[PROP_CUSTOM_TITLE] =
      g_param_spec_object ("custom-title",
                           P_("Custom Title"),
                           P_("Custom title widget to display"),
                           CTK_TYPE_WIDGET,
                           G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS);

  header_bar_props[PROP_SPACING] =
      g_param_spec_int ("spacing",
                        P_("Spacing"),
                        P_("The amount of space between children"),
                        0, G_MAXINT,
                        DEFAULT_SPACING,
                        CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkHeaderBar:show-close-button:
   *
   * Whether to show window decorations.
   *
   * Which buttons are actually shown and where is determined
   * by the #CtkHeaderBar:decoration-layout property, and by
   * the state of the window (e.g. a close button will not be
   * shown if the window can't be closed).
   */
  header_bar_props[PROP_SHOW_CLOSE_BUTTON] =
      g_param_spec_boolean ("show-close-button",
                            P_("Show decorations"),
                            P_("Whether to show window decorations"),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkHeaderBar:decoration-layout:
   *
   * The decoration layout for buttons. If this property is
   * not set, the #CtkSettings:ctk-decoration-layout setting
   * is used.
   *
   * See ctk_header_bar_set_decoration_layout() for information
   * about the format of this string.
   *
   * Since: 3.12
   */
  header_bar_props[PROP_DECORATION_LAYOUT] =
      g_param_spec_string ("decoration-layout",
                           P_("Decoration Layout"),
                           P_("The layout for window decorations"),
                           NULL,
                           CTK_PARAM_READWRITE);

  /**
   * CtkHeaderBar:decoration-layout-set:
   *
   * Set to %TRUE if #CtkHeaderBar:decoration-layout is set.
   *
   * Since: 3.12
   */
  header_bar_props[PROP_DECORATION_LAYOUT_SET] =
      g_param_spec_boolean ("decoration-layout-set",
                            P_("Decoration Layout Set"),
                            P_("Whether the decoration-layout property has been set"),
                            FALSE,
                            CTK_PARAM_READWRITE);

  /**
   * CtkHeaderBar:has-subtitle:
   *
   * If %TRUE, reserve space for a subtitle, even if none
   * is currently set.
   *
   * Since: 3.12
   */
  header_bar_props[PROP_HAS_SUBTITLE] =
      g_param_spec_boolean ("has-subtitle",
                            P_("Has Subtitle"),
                            P_("Whether to reserve space for a subtitle"),
                            TRUE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, header_bar_props);

  ctk_widget_class_set_accessible_type (widget_class, CTK_TYPE_HEADER_BAR_ACCESSIBLE);
  ctk_widget_class_set_accessible_role (widget_class, ATK_ROLE_PANEL);
  ctk_widget_class_set_css_name (widget_class, "headerbar");
}

static void
ctk_header_bar_init (CtkHeaderBar *bar)
{
  CtkHeaderBarPrivate *priv;
  CtkCssNode *widget_node;

  priv = ctk_header_bar_get_instance_private (bar);

  ctk_widget_set_has_window (CTK_WIDGET (bar), FALSE);

  priv->title = NULL;
  priv->subtitle = NULL;
  priv->custom_title = NULL;
  priv->children = NULL;
  priv->spacing = DEFAULT_SPACING;
  priv->has_subtitle = TRUE;
  priv->decoration_layout = NULL;
  priv->decoration_layout_set = FALSE;

  init_sizing_box (bar);
  construct_label_box (bar);

  widget_node = ctk_widget_get_css_node (CTK_WIDGET (bar));
  priv->gadget = ctk_css_custom_gadget_new_for_node (widget_node,
                                                     CTK_WIDGET (bar),
                                                     ctk_header_bar_get_content_size,
                                                     ctk_header_bar_allocate_contents,
                                                     ctk_header_bar_render_contents,
                                                     NULL,
                                                     NULL);

}

static void
ctk_header_bar_buildable_add_child (CtkBuildable *buildable,
                                    CtkBuilder   *builder,
                                    GObject      *child,
                                    const gchar  *type)
{
  if (type && strcmp (type, "title") == 0)
    ctk_header_bar_set_custom_title (CTK_HEADER_BAR (buildable), CTK_WIDGET (child));
  else if (!type)
    ctk_container_add (CTK_CONTAINER (buildable), CTK_WIDGET (child));
  else
    CTK_BUILDER_WARN_INVALID_CHILD_TYPE (CTK_HEADER_BAR (buildable), type);
}

static void
ctk_header_bar_buildable_init (CtkBuildableIface *iface)
{
  iface->add_child = ctk_header_bar_buildable_add_child;
}

/**
 * ctk_header_bar_pack_start:
 * @bar: A #CtkHeaderBar
 * @child: the #CtkWidget to be added to @bar
 *
 * Adds @child to @bar, packed with reference to the
 * start of the @bar.
 *
 * Since: 3.10
 */
void
ctk_header_bar_pack_start (CtkHeaderBar *bar,
                           CtkWidget    *child)
{
  ctk_header_bar_pack (bar, child, CTK_PACK_START);
}

/**
 * ctk_header_bar_pack_end:
 * @bar: A #CtkHeaderBar
 * @child: the #CtkWidget to be added to @bar
 *
 * Adds @child to @bar, packed with reference to the
 * end of the @bar.
 *
 * Since: 3.10
 */
void
ctk_header_bar_pack_end (CtkHeaderBar *bar,
                         CtkWidget    *child)
{
  ctk_header_bar_pack (bar, child, CTK_PACK_END);
}

/**
 * ctk_header_bar_new:
 *
 * Creates a new #CtkHeaderBar widget.
 *
 * Returns: a new #CtkHeaderBar
 *
 * Since: 3.10
 */
CtkWidget *
ctk_header_bar_new (void)
{
  return CTK_WIDGET (g_object_new (CTK_TYPE_HEADER_BAR, NULL));
}

/**
 * ctk_header_bar_get_show_close_button:
 * @bar: a #CtkHeaderBar
 *
 * Returns whether this header bar shows the standard window
 * decorations.
 *
 * Returns: %TRUE if the decorations are shown
 *
 * Since: 3.10
 */
gboolean
ctk_header_bar_get_show_close_button (CtkHeaderBar *bar)
{
  CtkHeaderBarPrivate *priv;

  g_return_val_if_fail (CTK_IS_HEADER_BAR (bar), FALSE);

  priv = ctk_header_bar_get_instance_private (bar);

  return priv->shows_wm_decorations;
}

/**
 * ctk_header_bar_set_show_close_button:
 * @bar: a #CtkHeaderBar
 * @setting: %TRUE to show standard window decorations
 *
 * Sets whether this header bar shows the standard window decorations,
 * including close, maximize, and minimize.
 *
 * Since: 3.10
 */
void
ctk_header_bar_set_show_close_button (CtkHeaderBar *bar,
                                      gboolean      setting)
{
  CtkHeaderBarPrivate *priv;

  g_return_if_fail (CTK_IS_HEADER_BAR (bar));

  priv = ctk_header_bar_get_instance_private (bar);

  setting = setting != FALSE;

  if (priv->shows_wm_decorations == setting)
    return;

  priv->shows_wm_decorations = setting;
  _ctk_header_bar_update_window_buttons (bar);
  g_object_notify_by_pspec (G_OBJECT (bar), header_bar_props[PROP_SHOW_CLOSE_BUTTON]);
}

/**
 * ctk_header_bar_set_has_subtitle:
 * @bar: a #CtkHeaderBar
 * @setting: %TRUE to reserve space for a subtitle
 *
 * Sets whether the header bar should reserve space
 * for a subtitle, even if none is currently set.
 *
 * Since: 3.12
 */
void
ctk_header_bar_set_has_subtitle (CtkHeaderBar *bar,
                                 gboolean      setting)
{
  CtkHeaderBarPrivate *priv;

  g_return_if_fail (CTK_IS_HEADER_BAR (bar));

  priv = ctk_header_bar_get_instance_private (bar);

  setting = setting != FALSE;

  if (priv->has_subtitle == setting)
    return;

  priv->has_subtitle = setting;
  ctk_widget_set_visible (priv->subtitle_sizing_label, setting || (priv->subtitle && priv->subtitle[0]));

  ctk_widget_queue_resize (CTK_WIDGET (bar));

  g_object_notify_by_pspec (G_OBJECT (bar), header_bar_props[PROP_HAS_SUBTITLE]);
}

/**
 * ctk_header_bar_get_has_subtitle:
 * @bar: a #CtkHeaderBar
 *
 * Retrieves whether the header bar reserves space for
 * a subtitle, regardless if one is currently set or not.
 *
 * Returns: %TRUE if the header bar reserves space
 *     for a subtitle
 *
 * Since: 3.12
 */
gboolean
ctk_header_bar_get_has_subtitle (CtkHeaderBar *bar)
{
  CtkHeaderBarPrivate *priv;

  g_return_val_if_fail (CTK_IS_HEADER_BAR (bar), FALSE);

  priv = ctk_header_bar_get_instance_private (bar);

  return priv->has_subtitle;
}

/**
 * ctk_header_bar_set_decoration_layout:
 * @bar: a #CtkHeaderBar
 * @layout: (allow-none): a decoration layout, or %NULL to
 *     unset the layout
 *
 * Sets the decoration layout for this header bar, overriding
 * the #CtkSettings:ctk-decoration-layout setting. 
 *
 * There can be valid reasons for overriding the setting, such
 * as a header bar design that does not allow for buttons to take
 * room on the right, or only offers room for a single close button.
 * Split header bars are another example for overriding the
 * setting.
 *
 * The format of the string is button names, separated by commas.
 * A colon separates the buttons that should appear on the left
 * from those on the right. Recognized button names are minimize,
 * maximize, close, icon (the window icon) and menu (a menu button
 * for the fallback app menu).
 *
 * For example, “menu:minimize,maximize,close” specifies a menu
 * on the left, and minimize, maximize and close buttons on the right.
 *
 * Since: 3.12
 */
void
ctk_header_bar_set_decoration_layout (CtkHeaderBar *bar,
                                      const gchar  *layout)
{
  CtkHeaderBarPrivate *priv;

  g_return_if_fail (CTK_IS_HEADER_BAR (bar));

  priv = ctk_header_bar_get_instance_private (bar);

  g_free (priv->decoration_layout);
  priv->decoration_layout = g_strdup (layout);
  priv->decoration_layout_set = (layout != NULL);

  _ctk_header_bar_update_window_buttons (bar);

  g_object_notify_by_pspec (G_OBJECT (bar), header_bar_props[PROP_DECORATION_LAYOUT]);
  g_object_notify_by_pspec (G_OBJECT (bar), header_bar_props[PROP_DECORATION_LAYOUT_SET]);
}

/**
 * ctk_header_bar_get_decoration_layout:
 * @bar: a #CtkHeaderBar
 *
 * Gets the decoration layout set with
 * ctk_header_bar_set_decoration_layout().
 *
 * Returns: the decoration layout
 *
 * Since: 3.12 
 */
const gchar *
ctk_header_bar_get_decoration_layout (CtkHeaderBar *bar)
{
  CtkHeaderBarPrivate *priv;

  g_return_val_if_fail (CTK_IS_HEADER_BAR (bar), NULL);

  priv = ctk_header_bar_get_instance_private (bar);

  return priv->decoration_layout;
}
