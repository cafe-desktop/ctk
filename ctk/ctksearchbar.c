/* CTK - The GIMP Toolkit
 * Copyright (C) 2013 Red Hat, Inc.
 *
 * Authors:
 * - Bastien Nocera <bnocera@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the CTK+ Team and others 2013.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#include "config.h"

#include "ctkentry.h"
#include "ctkentryprivate.h"
#include "ctkintl.h"
#include "ctkprivate.h"
#include "ctkrender.h"
#include "ctksearchbar.h"
#include "ctksearchentryprivate.h"

/**
 * SECTION:ctksearchbar
 * @Short_description: A toolbar to integrate a search entry with
 * @Title: CtkSearchBar
 *
 * #CtkSearchBar is a container made to have a search entry (possibly
 * with additional connex widgets, such as drop-down menus, or buttons)
 * built-in. The search bar would appear when a search is started through
 * typing on the keyboard, or the application’s search mode is toggled on.
 *
 * For keyboard presses to start a search, events will need to be
 * forwarded from the top-level window that contains the search bar.
 * See ctk_search_bar_handle_event() for example code. Common shortcuts
 * such as Ctrl+F should be handled as an application action, or through
 * the menu items.
 *
 * You will also need to tell the search bar about which entry you
 * are using as your search entry using ctk_search_bar_connect_entry().
 * The following example shows you how to create a more complex search
 * entry.
 *
 * # CSS nodes
 *
 * CtkSearchBar has a single CSS node with name searchbar.
 *
 * ## Creating a search bar
 *
 * [A simple example](https://gitlab.gnome.org/GNOME/ctk/blob/ctk-3-24/examples/search-bar.c)
 *
 * Since: 3.10
 */

typedef struct {
  /* Template widgets */
  CtkWidget   *revealer;
  CtkWidget   *tool_box;
  CtkWidget   *box_center;
  CtkWidget   *close_button;

  CtkWidget   *entry;
  gboolean     reveal_child;
} CtkSearchBarPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (CtkSearchBar, ctk_search_bar, CTK_TYPE_BIN)

enum {
  PROP_0,
  PROP_SEARCH_MODE_ENABLED,
  PROP_SHOW_CLOSE_BUTTON,
  LAST_PROPERTY
};

static GParamSpec *widget_props[LAST_PROPERTY] = { NULL, };

static void
stop_search_cb (CtkWidget    *entry G_GNUC_UNUSED,
                CtkSearchBar *bar)
{
  CtkSearchBarPrivate *priv = ctk_search_bar_get_instance_private (bar);
  ctk_revealer_set_reveal_child (CTK_REVEALER (priv->revealer), FALSE);
}

static gboolean
entry_key_pressed_event_cb (CtkWidget    *widget,
                            CdkEvent     *event,
                            CtkSearchBar *bar)
{
  if (event->key.keyval == CDK_KEY_Escape)
    {
      stop_search_cb (widget, bar);
      return CDK_EVENT_STOP;
    }
  else
    return CDK_EVENT_PROPAGATE;
}

static void
preedit_changed_cb (CtkEntry  *entry G_GNUC_UNUSED,
                    CtkWidget *popup G_GNUC_UNUSED,
                    gboolean  *preedit_changed)
{
  *preedit_changed = TRUE;
}

static gboolean
ctk_search_bar_handle_event_for_entry (CtkSearchBar *bar,
                                       CdkEvent     *event)
{
  CtkSearchBarPrivate *priv = ctk_search_bar_get_instance_private (bar);
  gboolean handled;
  gboolean preedit_changed;
  guint preedit_change_id;
  gboolean res;
  char *old_text, *new_text;

  if (ctk_search_entry_is_keynav_event (event) ||
      event->key.keyval == CDK_KEY_space ||
      event->key.keyval == CDK_KEY_Menu)
    return CDK_EVENT_PROPAGATE;

  if (!ctk_widget_get_realized (priv->entry))
    ctk_widget_realize (priv->entry);

  handled = CDK_EVENT_PROPAGATE;
  preedit_changed = FALSE;
  preedit_change_id = g_signal_connect (priv->entry, "preedit-changed",
                                        G_CALLBACK (preedit_changed_cb), &preedit_changed);

  old_text = g_strdup (ctk_entry_get_text (CTK_ENTRY (priv->entry)));
  res = ctk_widget_event (priv->entry, event);
  new_text = g_strdup (ctk_entry_get_text (CTK_ENTRY (priv->entry)));

  g_signal_handler_disconnect (priv->entry, preedit_change_id);

  if ((res && g_strcmp0 (new_text, old_text) != 0) || preedit_changed)
    handled = CDK_EVENT_STOP;

  g_free (old_text);
  g_free (new_text);

  return handled;
}

/**
 * ctk_search_bar_handle_event:
 * @bar: a #CtkSearchBar
 * @event: a #CdkEvent containing key press events
 *
 * This function should be called when the top-level
 * window which contains the search bar received a key event.
 *
 * If the key event is handled by the search bar, the bar will
 * be shown, the entry populated with the entered text and %CDK_EVENT_STOP
 * will be returned. The caller should ensure that events are
 * not propagated further.
 *
 * If no entry has been connected to the search bar, using
 * ctk_search_bar_connect_entry(), this function will return
 * immediately with a warning.
 *
 * ## Showing the search bar on key presses
 *
 * |[<!-- language="C" -->
 * static gboolean
 * on_key_press_event (CtkWidget *widget,
 *                     CdkEvent  *event,
 *                     gpointer   user_data)
 * {
 *   CtkSearchBar *bar = CTK_SEARCH_BAR (user_data);
 *   return ctk_search_bar_handle_event (bar, event);
 * }
 *
 * static void
 * create_toplevel (void)
 * {
 *   CtkWidget *window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
 *   CtkWindow *search_bar = ctk_search_bar_new ();
 *
 *  // Add more widgets to the window...
 *
 *   g_signal_connect (window,
 *                    "key-press-event",
 *                     G_CALLBACK (on_key_press_event),
 *                     search_bar);
 * }
 * ]|
 *
 * Returns: %CDK_EVENT_STOP if the key press event resulted
 *     in text being entered in the search entry (and revealing
 *     the search bar if necessary), %CDK_EVENT_PROPAGATE otherwise.
 *
 * Since: 3.10
 */
gboolean
ctk_search_bar_handle_event (CtkSearchBar *bar,
                             CdkEvent     *event)
{
  CtkSearchBarPrivate *priv = ctk_search_bar_get_instance_private (bar);
  gboolean handled;

  if (priv->reveal_child)
    return CDK_EVENT_PROPAGATE;

  if (priv->entry == NULL)
    {
      g_warning ("The search bar does not have an entry connected to it. Call ctk_search_bar_connect_entry() to connect one.");
      return CDK_EVENT_PROPAGATE;
    }

  if (CTK_IS_SEARCH_ENTRY (priv->entry))
    handled = ctk_search_entry_handle_event (CTK_SEARCH_ENTRY (priv->entry), event);
  else
    handled = ctk_search_bar_handle_event_for_entry (bar, event);

  if (handled == CDK_EVENT_STOP)
    ctk_revealer_set_reveal_child (CTK_REVEALER (priv->revealer), TRUE);

  return handled;
}

static void
reveal_child_changed_cb (GObject      *object,
                         GParamSpec   *pspec G_GNUC_UNUSED,
                         CtkSearchBar *bar)
{
  CtkSearchBarPrivate *priv = ctk_search_bar_get_instance_private (bar);
  gboolean reveal_child;

  g_object_get (object, "reveal-child", &reveal_child, NULL);
  if (reveal_child)
    ctk_widget_set_child_visible (priv->revealer, TRUE);

  if (reveal_child == priv->reveal_child)
    return;

  priv->reveal_child = reveal_child;

  if (priv->entry)
    {
      if (reveal_child)
        _ctk_entry_grab_focus (CTK_ENTRY (priv->entry), FALSE);
      else
        ctk_entry_set_text (CTK_ENTRY (priv->entry), "");
    }

  g_object_notify (G_OBJECT (bar), "search-mode-enabled");
}

static void
child_revealed_changed_cb (GObject      *object,
                           GParamSpec   *pspec G_GNUC_UNUSED,
                           CtkSearchBar *bar)
{
  CtkSearchBarPrivate *priv = ctk_search_bar_get_instance_private (bar);
  gboolean val;

  g_object_get (object, "child-revealed", &val, NULL);
  if (!val)
    ctk_widget_set_child_visible (priv->revealer, FALSE);
}

static void
close_button_clicked_cb (CtkWidget    *button G_GNUC_UNUSED,
                         CtkSearchBar *bar)
{
  CtkSearchBarPrivate *priv = ctk_search_bar_get_instance_private (bar);

  ctk_revealer_set_reveal_child (CTK_REVEALER (priv->revealer), FALSE);
}

static void
ctk_search_bar_add (CtkContainer *container,
                    CtkWidget    *child)
{
  CtkSearchBar *bar = CTK_SEARCH_BAR (container);
  CtkSearchBarPrivate *priv = ctk_search_bar_get_instance_private (bar);

  /* When constructing the widget, we want the revealer to be added
   * as the first child of the search bar, as an implementation detail.
   * After that, the child added by the application should be added
   * to box_center.
   */
  if (priv->box_center == NULL)
    {
      CTK_CONTAINER_CLASS (ctk_search_bar_parent_class)->add (container, child);
    }
  else
    {
      ctk_container_add (CTK_CONTAINER (priv->box_center), child);
      /* If an entry is the only child, save the developer a couple of
       * lines of code
       */
      if (CTK_IS_ENTRY (child))
        ctk_search_bar_connect_entry (bar, CTK_ENTRY (child));
    }
}

static void
ctk_search_bar_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  CtkSearchBar *bar = CTK_SEARCH_BAR (object);

  switch (prop_id)
    {
    case PROP_SEARCH_MODE_ENABLED:
      ctk_search_bar_set_search_mode (bar, g_value_get_boolean (value));
      break;
    case PROP_SHOW_CLOSE_BUTTON:
      ctk_search_bar_set_show_close_button (bar, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_search_bar_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  CtkSearchBar *bar = CTK_SEARCH_BAR (object);
  CtkSearchBarPrivate *priv = ctk_search_bar_get_instance_private (bar);

  switch (prop_id)
    {
    case PROP_SEARCH_MODE_ENABLED:
      g_value_set_boolean (value, priv->reveal_child);
      break;
    case PROP_SHOW_CLOSE_BUTTON:
      g_value_set_boolean (value, ctk_search_bar_get_show_close_button (bar));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void ctk_search_bar_set_entry (CtkSearchBar *bar,
                                      CtkEntry     *entry);

static void
ctk_search_bar_dispose (GObject *object)
{
  CtkSearchBar *bar = CTK_SEARCH_BAR (object);

  ctk_search_bar_set_entry (bar, NULL);

  G_OBJECT_CLASS (ctk_search_bar_parent_class)->dispose (object);
}

static gboolean
ctk_search_bar_draw (CtkWidget *widget,
                     cairo_t *cr)
{
  gint width, height;
  CtkStyleContext *context;

  width = ctk_widget_get_allocated_width (widget);
  height = ctk_widget_get_allocated_height (widget);
  context = ctk_widget_get_style_context (widget);

  ctk_render_background (context, cr, 0, 0, width, height);
  ctk_render_frame (context, cr, 0, 0, width, height);

  CTK_WIDGET_CLASS (ctk_search_bar_parent_class)->draw (widget, cr);

  return FALSE;
}

static void
ctk_search_bar_class_init (CtkSearchBarClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);
  CtkContainerClass *container_class = CTK_CONTAINER_CLASS (klass);

  object_class->dispose = ctk_search_bar_dispose;
  object_class->set_property = ctk_search_bar_set_property;
  object_class->get_property = ctk_search_bar_get_property;
  widget_class->draw = ctk_search_bar_draw;

  container_class->add = ctk_search_bar_add;

  /**
   * CtkEntry:search-mode-enabled:
   *
   * Whether the search mode is on and the search bar shown.
   *
   * See ctk_search_bar_set_search_mode() for details.
   */
  widget_props[PROP_SEARCH_MODE_ENABLED] = g_param_spec_boolean ("search-mode-enabled",
                                                                 P_("Search Mode Enabled"),
                                                                 P_("Whether the search mode is on and the search bar shown"),
                                                                 FALSE,
                                                                 CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkEntry:show-close-button:
   *
   * Whether to show the close button in the toolbar.
   */
  widget_props[PROP_SHOW_CLOSE_BUTTON] = g_param_spec_boolean ("show-close-button",
                                                               P_("Show Close Button"),
                                                               P_("Whether to show the close button in the toolbar"),
                                                               FALSE,
                                                               CTK_PARAM_READWRITE|G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROPERTY, widget_props);

  ctk_widget_class_set_template_from_resource (widget_class, "/org/ctk/libctk/ui/ctksearchbar.ui");
  ctk_widget_class_bind_template_child_private (widget_class, CtkSearchBar, tool_box);
  ctk_widget_class_bind_template_child_private (widget_class, CtkSearchBar, revealer);
  ctk_widget_class_bind_template_child_private (widget_class, CtkSearchBar, box_center);
  ctk_widget_class_bind_template_child_private (widget_class, CtkSearchBar, close_button);

  ctk_widget_class_set_css_name (widget_class, "searchbar");
}

static void
ctk_search_bar_init (CtkSearchBar *bar)
{
  CtkSearchBarPrivate *priv = ctk_search_bar_get_instance_private (bar);

  ctk_widget_init_template (CTK_WIDGET (bar));

  /* We use child-visible to avoid the unexpanded revealer
   * peaking out by 1 pixel
   */
  ctk_widget_set_child_visible (priv->revealer, FALSE);

  g_signal_connect (priv->revealer, "notify::reveal-child",
                    G_CALLBACK (reveal_child_changed_cb), bar);
  g_signal_connect (priv->revealer, "notify::child-revealed",
                    G_CALLBACK (child_revealed_changed_cb), bar);

  ctk_widget_set_no_show_all (priv->close_button, TRUE);
  g_signal_connect (priv->close_button, "clicked",
                    G_CALLBACK (close_button_clicked_cb), bar);
};

/**
 * ctk_search_bar_new:
 *
 * Creates a #CtkSearchBar. You will need to tell it about
 * which widget is going to be your text entry using
 * ctk_search_bar_connect_entry().
 *
 * Returns: a new #CtkSearchBar
 *
 * Since: 3.10
 */
CtkWidget *
ctk_search_bar_new (void)
{
  return g_object_new (CTK_TYPE_SEARCH_BAR, NULL);
}

static void
ctk_search_bar_set_entry (CtkSearchBar *bar,
                          CtkEntry     *entry)
{
  CtkSearchBarPrivate *priv = ctk_search_bar_get_instance_private (bar);

  if (priv->entry != NULL)
    {
      if (CTK_IS_SEARCH_ENTRY (priv->entry))
        g_signal_handlers_disconnect_by_func (priv->entry, stop_search_cb, bar);
      else
        g_signal_handlers_disconnect_by_func (priv->entry, entry_key_pressed_event_cb, bar);
      g_object_remove_weak_pointer (G_OBJECT (priv->entry), (gpointer *) &priv->entry);
    }

  priv->entry = CTK_WIDGET (entry);

  if (priv->entry != NULL)
    {
      g_object_add_weak_pointer (G_OBJECT (priv->entry), (gpointer *) &priv->entry);
      if (CTK_IS_SEARCH_ENTRY (priv->entry))
        g_signal_connect (priv->entry, "stop-search",
                          G_CALLBACK (stop_search_cb), bar);
      else
        g_signal_connect (priv->entry, "key-press-event",
                          G_CALLBACK (entry_key_pressed_event_cb), bar);
    }
}

/**
 * ctk_search_bar_connect_entry:
 * @bar: a #CtkSearchBar
 * @entry: a #CtkEntry
 *
 * Connects the #CtkEntry widget passed as the one to be used in
 * this search bar. The entry should be a descendant of the search bar.
 * This is only required if the entry isn’t the direct child of the
 * search bar (as in our main example).
 *
 * Since: 3.10
 */
void
ctk_search_bar_connect_entry (CtkSearchBar *bar,
                              CtkEntry     *entry)
{
  g_return_if_fail (CTK_IS_SEARCH_BAR (bar));
  g_return_if_fail (entry == NULL || CTK_IS_ENTRY (entry));

  ctk_search_bar_set_entry (bar, entry);
}

/**
 * ctk_search_bar_get_search_mode:
 * @bar: a #CtkSearchBar
 *
 * Returns whether the search mode is on or off.
 *
 * Returns: whether search mode is toggled on
 *
 * Since: 3.10
 */
gboolean
ctk_search_bar_get_search_mode (CtkSearchBar *bar)
{
  CtkSearchBarPrivate *priv = ctk_search_bar_get_instance_private (bar);

  g_return_val_if_fail (CTK_IS_SEARCH_BAR (bar), FALSE);

  return priv->reveal_child;
}

/**
 * ctk_search_bar_set_search_mode:
 * @bar: a #CtkSearchBar
 * @search_mode: the new state of the search mode
 *
 * Switches the search mode on or off.
 *
 * Since: 3.10
 */
void
ctk_search_bar_set_search_mode (CtkSearchBar *bar,
                                gboolean      search_mode)
{
  CtkSearchBarPrivate *priv = ctk_search_bar_get_instance_private (bar);

  g_return_if_fail (CTK_IS_SEARCH_BAR (bar));

  ctk_revealer_set_reveal_child (CTK_REVEALER (priv->revealer), search_mode);
}

/**
 * ctk_search_bar_get_show_close_button:
 * @bar: a #CtkSearchBar
 *
 * Returns whether the close button is shown.
 *
 * Returns: whether the close button is shown
 *
 * Since: 3.10
 */
gboolean
ctk_search_bar_get_show_close_button (CtkSearchBar *bar)
{
  CtkSearchBarPrivate *priv = ctk_search_bar_get_instance_private (bar);

  g_return_val_if_fail (CTK_IS_SEARCH_BAR (bar), FALSE);

  return ctk_widget_get_visible (priv->close_button);
}

/**
 * ctk_search_bar_set_show_close_button:
 * @bar: a #CtkSearchBar
 * @visible: whether the close button will be shown or not
 *
 * Shows or hides the close button. Applications that
 * already have a “search” toggle button should not show a close
 * button in their search bar, as it duplicates the role of the
 * toggle button.
 *
 * Since: 3.10
 */
void
ctk_search_bar_set_show_close_button (CtkSearchBar *bar,
                                      gboolean      visible)
{
  CtkSearchBarPrivate *priv = ctk_search_bar_get_instance_private (bar);

  g_return_if_fail (CTK_IS_SEARCH_BAR (bar));

  visible = visible != FALSE;

  if (ctk_widget_get_visible (priv->close_button) != visible)
    {
      ctk_widget_set_visible (priv->close_button, visible);
      g_object_notify (G_OBJECT (bar), "show-close-button");
    }
}
