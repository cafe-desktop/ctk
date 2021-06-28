/* ctkshortcutswindow.c
 *
 * Copyright (C) 2015 Christian Hergert <christian@hergert.me>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "ctkshortcutswindow.h"
#include "ctkscrolledwindow.h"
#include "ctkshortcutssection.h"
#include "ctkshortcutsgroup.h"
#include "ctkshortcutsshortcutprivate.h"
#include "ctksearchbar.h"
#include "ctksearchentry.h"
#include "ctkwidgetprivate.h"
#include "ctkprivate.h"
#include "ctkintl.h"

/**
 * SECTION:ctkshortcutswindow
 * @Title: CtkShortcutsWindow
 * @Short_description: Toplevel which shows help for shortcuts
 *
 * A CtkShortcutsWindow shows brief information about the keyboard shortcuts
 * and gestures of an application. The shortcuts can be grouped, and you can
 * have multiple sections in this window, corresponding to the major modes of
 * your application.
 *
 * Additionally, the shortcuts can be filtered by the current view, to avoid
 * showing information that is not relevant in the current application context.
 *
 * The recommended way to construct a CtkShortcutsWindow is with CtkBuilder,
 * by populating a #CtkShortcutsWindow with one or more #CtkShortcutsSection
 * objects, which contain #CtkShortcutsGroups that in turn contain objects of
 * class #CtkShortcutsShortcut.
 *
 * # A simple example:
 *
 * ![](gedit-shortcuts.png)
 *
 * This example has as single section. As you can see, the shortcut groups
 * are arranged in columns, and spread across several pages if there are too
 * many to find on a single page.
 *
 * The .ui file for this example can be found [here](https://git.gnome.org/browse/ctk+/tree/demos/ctk-demo/shortcuts-gedit.ui).
 *
 * # An example with multiple views:
 *
 * ![](clocks-shortcuts.png)
 *
 * This example shows a #CtkShortcutsWindow that has been configured to show only
 * the shortcuts relevant to the "stopwatch" view.
 *
 * The .ui file for this example can be found [here](https://git.gnome.org/browse/ctk+/tree/demos/ctk-demo/shortcuts-clocks.ui).
 *
 * # An example with multiple sections:
 *
 * ![](builder-shortcuts.png)
 *
 * This example shows a #CtkShortcutsWindow with two sections, "Editor Shortcuts"
 * and "Terminal Shortcuts".
 *
 * The .ui file for this example can be found [here](https://git.gnome.org/browse/ctk+/tree/demos/ctk-demo/shortcuts-builder.ui).
 */

typedef struct
{
  GHashTable     *keywords;
  gchar          *initial_section;
  gchar          *last_section_name;
  gchar          *view_name;
  CtkSizeGroup   *search_text_group;
  CtkSizeGroup   *search_image_group;
  GHashTable     *search_items_hash;

  CtkStack       *stack;
  CtkStack       *title_stack;
  CtkMenuButton  *menu_button;
  CtkLabel       *menu_label;
  CtkSearchBar   *search_bar;
  CtkSearchEntry *search_entry;
  CtkHeaderBar   *header_bar;
  CtkWidget      *main_box;
  CtkPopover     *popover;
  CtkListBox     *list_box;
  CtkBox         *search_gestures;
  CtkBox         *search_shortcuts;

  CtkWindow      *window;
  gulong          keys_changed_id;
} CtkShortcutsWindowPrivate;

typedef struct
{
  CtkShortcutsWindow *self;
  CtkBuilder        *builder;
  GQueue            *stack;
  gchar             *property_name;
  guint              translatable : 1;
} ViewsParserData;


G_DEFINE_TYPE_WITH_PRIVATE (CtkShortcutsWindow, ctk_shortcuts_window, CTK_TYPE_WINDOW)


enum {
  CLOSE,
  SEARCH,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_SECTION_NAME,
  PROP_VIEW_NAME,
  LAST_PROP
};

static GParamSpec *properties[LAST_PROP];
static guint signals[LAST_SIGNAL];


static gint
number_of_children (CtkContainer *container)
{
  GList *children;
  gint n;

  children = ctk_container_get_children (container);
  n = g_list_length (children);
  g_list_free (children);

  return n;
}

static void
update_title_stack (CtkShortcutsWindow *self)
{
  CtkShortcutsWindowPrivate *priv = ctk_shortcuts_window_get_instance_private (self);
  CtkWidget *visible_child;

  visible_child = ctk_stack_get_visible_child (priv->stack);

  if (CTK_IS_SHORTCUTS_SECTION (visible_child))
    {
      if (number_of_children (CTK_CONTAINER (priv->stack)) > 3)
        {
          gchar *title;

          ctk_stack_set_visible_child_name (priv->title_stack, "sections");
          g_object_get (visible_child, "title", &title, NULL);
          ctk_label_set_label (priv->menu_label, title);
          g_free (title);
        }
      else
        {
          ctk_stack_set_visible_child_name (priv->title_stack, "title");
        }
    }
  else if (visible_child != NULL)
    {
      ctk_stack_set_visible_child_name (priv->title_stack, "search");
    }
}

static void
ctk_shortcuts_window_add_search_item (CtkWidget *child, gpointer data)
{
  CtkShortcutsWindow *self = data;
  CtkShortcutsWindowPrivate *priv = ctk_shortcuts_window_get_instance_private (self);
  CtkWidget *item;
  gchar *accelerator = NULL;
  gchar *title = NULL;
  gchar *hash_key = NULL;
  GIcon *icon = NULL;
  gboolean icon_set = FALSE;
  gboolean subtitle_set = FALSE;
  CtkTextDirection direction;
  CtkShortcutType shortcut_type;
  gchar *action_name = NULL;
  gchar *subtitle;
  gchar *str;
  gchar *keywords;

  if (CTK_IS_SHORTCUTS_SHORTCUT (child))
    {
      GEnumClass *class;
      GEnumValue *value;

      g_object_get (child,
                    "accelerator", &accelerator,
                    "title", &title,
                    "direction", &direction,
                    "icon-set", &icon_set,
                    "subtitle-set", &subtitle_set,
                    "shortcut-type", &shortcut_type,
                    "action-name", &action_name,
                    NULL);

      class = G_ENUM_CLASS (g_type_class_ref (CTK_TYPE_SHORTCUT_TYPE));
      value = g_enum_get_value (class, shortcut_type);

      hash_key = g_strdup_printf ("%s-%s-%s", title, value->value_nick, accelerator);

      g_type_class_unref (class);

      if (g_hash_table_contains (priv->search_items_hash, hash_key))
        {
          g_free (hash_key);
          g_free (title);
          g_free (accelerator);
          return;
        }

      g_hash_table_insert (priv->search_items_hash, hash_key, GINT_TO_POINTER (1));

      item = g_object_new (CTK_TYPE_SHORTCUTS_SHORTCUT,
                           "accelerator", accelerator,
                           "title", title,
                           "direction", direction,
                           "shortcut-type", shortcut_type,
                           "accel-size-group", priv->search_image_group,
                           "title-size-group", priv->search_text_group,
                           "action-name", action_name,
                           NULL);
      if (icon_set)
        {
          g_object_get (child, "icon", &icon, NULL);
          g_object_set (item, "icon", icon, NULL);
          g_clear_object (&icon);
        }
      if (subtitle_set)
        {
          g_object_get (child, "subtitle", &subtitle, NULL);
          g_object_set (item, "subtitle", subtitle, NULL);
          g_free (subtitle);
        }
      str = g_strdup_printf ("%s %s", accelerator, title);
      keywords = g_utf8_strdown (str, -1);

      g_hash_table_insert (priv->keywords, item, keywords);
      if (shortcut_type == CTK_SHORTCUT_ACCELERATOR)
        ctk_container_add (CTK_CONTAINER (priv->search_shortcuts), item);
      else
        ctk_container_add (CTK_CONTAINER (priv->search_gestures), item);

      g_free (title);
      g_free (accelerator);
      g_free (str);
      g_free (action_name);
    }
  else if (CTK_IS_CONTAINER (child))
    {
      ctk_container_foreach (CTK_CONTAINER (child), ctk_shortcuts_window_add_search_item, self);
    }
}

static void
section_notify_cb (GObject    *section,
                   GParamSpec *pspec,
                   gpointer    data)
{
  CtkShortcutsWindow *self = data;
  CtkShortcutsWindowPrivate *priv = ctk_shortcuts_window_get_instance_private (self);

  if (strcmp (pspec->name, "section-name") == 0)
    {
      gchar *name;

      g_object_get (section, "section-name", &name, NULL);
      ctk_container_child_set (CTK_CONTAINER (priv->stack), CTK_WIDGET (section), "name", name, NULL);
      g_free (name);
    }
  else if (strcmp (pspec->name, "title") == 0)
    {
      gchar *title;
      CtkWidget *label;

      label = g_object_get_data (section, "ctk-shortcuts-title");
      g_object_get (section, "title", &title, NULL);
      ctk_label_set_label (CTK_LABEL (label), title);
      g_free (title);
    }
}

static void
ctk_shortcuts_window_add_section (CtkShortcutsWindow  *self,
                                  CtkShortcutsSection *section)
{
  CtkShortcutsWindowPrivate *priv = ctk_shortcuts_window_get_instance_private (self);
  CtkListBoxRow *row;
  gchar *title;
  gchar *name;
  const gchar *visible_section;
  CtkWidget *label;

  ctk_container_foreach (CTK_CONTAINER (section), ctk_shortcuts_window_add_search_item, self);

  g_object_get (section,
                "section-name", &name,
                "title", &title,
                NULL);

  g_signal_connect (section, "notify", G_CALLBACK (section_notify_cb), self);

  if (name == NULL)
    name = g_strdup ("shortcuts");

  ctk_stack_add_titled (priv->stack, CTK_WIDGET (section), name, title);

  visible_section = ctk_stack_get_visible_child_name (priv->stack);
  if (strcmp (visible_section, "internal-search") == 0 ||
      (priv->initial_section && strcmp (priv->initial_section, visible_section) == 0))
    ctk_stack_set_visible_child (priv->stack, CTK_WIDGET (section));

  row = g_object_new (CTK_TYPE_LIST_BOX_ROW,
                      "visible", TRUE,
                      NULL);
  g_object_set_data (G_OBJECT (row), "ctk-shortcuts-section", section);
  label = g_object_new (CTK_TYPE_LABEL,
                        "margin", 6,
                        "label", title,
                        "xalign", 0.5f,
                        "visible", TRUE,
                        NULL);
  g_object_set_data (G_OBJECT (section), "ctk-shortcuts-title", label);
  ctk_container_add (CTK_CONTAINER (row), CTK_WIDGET (label));
  ctk_container_add (CTK_CONTAINER (priv->list_box), CTK_WIDGET (row));

  update_title_stack (self);

  g_free (name);
  g_free (title);
}

static void
ctk_shortcuts_window_add (CtkContainer *container,
                          CtkWidget    *widget)
{
  CtkShortcutsWindow *self = (CtkShortcutsWindow *)container;

  if (CTK_IS_SHORTCUTS_SECTION (widget))
    ctk_shortcuts_window_add_section (self, CTK_SHORTCUTS_SECTION (widget));
  else
    g_warning ("Can't add children of type %s to %s",
               G_OBJECT_TYPE_NAME (widget),
               G_OBJECT_TYPE_NAME (container));
}

static void
ctk_shortcuts_window_remove (CtkContainer *container,
                             CtkWidget    *widget)
{
  CtkShortcutsWindow *self = (CtkShortcutsWindow *)container;
  CtkShortcutsWindowPrivate *priv = ctk_shortcuts_window_get_instance_private (self);

  g_signal_handlers_disconnect_by_func (widget, section_notify_cb, self);

  if (widget == (CtkWidget *)priv->header_bar ||
      widget == (CtkWidget *)priv->main_box)
    CTK_CONTAINER_CLASS (ctk_shortcuts_window_parent_class)->remove (container, widget);
  else
    ctk_container_remove (CTK_CONTAINER (priv->stack), widget);
}

static void
ctk_shortcuts_window_forall (CtkContainer *container,
                             gboolean      include_internal,
                             CtkCallback   callback,
                             gpointer      callback_data)
{
  CtkShortcutsWindow *self = (CtkShortcutsWindow *)container;
  CtkShortcutsWindowPrivate *priv = ctk_shortcuts_window_get_instance_private (self);

  if (include_internal)
    {
      CTK_CONTAINER_CLASS (ctk_shortcuts_window_parent_class)->forall (container, include_internal, callback, callback_data);
    }
  else
    {
      if (priv->stack)
        {
          GList *children, *l;
          CtkWidget *search;
          CtkWidget *empty;

          search = ctk_stack_get_child_by_name (CTK_STACK (priv->stack), "internal-search");
          empty = ctk_stack_get_child_by_name (CTK_STACK (priv->stack), "no-search-results");
          children = ctk_container_get_children (CTK_CONTAINER (priv->stack));
          for (l = children; l; l = l->next)
            {
              CtkWidget *child = l->data;

              if (include_internal ||
                  (child != search && child != empty))
                callback (child, callback_data);
            }
          g_list_free (children);
        }
    }
}

static void
ctk_shortcuts_window_set_view_name (CtkShortcutsWindow *self,
                                    const gchar        *view_name)
{
  CtkShortcutsWindowPrivate *priv = ctk_shortcuts_window_get_instance_private (self);
  GList *sections, *l;

  g_free (priv->view_name);
  priv->view_name = g_strdup (view_name);

  sections = ctk_container_get_children (CTK_CONTAINER (priv->stack));
  for (l = sections; l; l = l->next)
    {
      CtkShortcutsSection *section = l->data;

      if (CTK_IS_SHORTCUTS_SECTION (section))
        g_object_set (section, "view-name", priv->view_name, NULL);
    }
  g_list_free (sections);
}

static void
ctk_shortcuts_window_set_section_name (CtkShortcutsWindow *self,
                                       const gchar        *section_name)
{
  CtkShortcutsWindowPrivate *priv = ctk_shortcuts_window_get_instance_private (self);
  CtkWidget *section = NULL;

  g_free (priv->initial_section);
  priv->initial_section = g_strdup (section_name);

  if (section_name)
    section = ctk_stack_get_child_by_name (priv->stack, section_name);
  if (section)
    ctk_stack_set_visible_child (priv->stack, section);
}

static void
update_accels_cb (CtkWidget *widget,
                  gpointer   data)
{
  CtkShortcutsWindow *self = data;
  CtkShortcutsWindowPrivate *priv = ctk_shortcuts_window_get_instance_private (self);

  if (CTK_IS_SHORTCUTS_SHORTCUT (widget))
    ctk_shortcuts_shortcut_update_accel (CTK_SHORTCUTS_SHORTCUT (widget), priv->window);
  else if (CTK_IS_CONTAINER (widget))
    ctk_container_foreach (CTK_CONTAINER (widget), update_accels_cb, self);
}

static void
update_accels_for_actions (CtkShortcutsWindow *self)
{
  CtkShortcutsWindowPrivate *priv = ctk_shortcuts_window_get_instance_private (self);

  if (priv->window)
    ctk_container_forall (CTK_CONTAINER (self), update_accels_cb, self);
}

static void
keys_changed_handler (CtkWindow          *window,
                      CtkShortcutsWindow *self)
{
  update_accels_for_actions (self);
}

void
ctk_shortcuts_window_set_window (CtkShortcutsWindow *self,
                                 CtkWindow          *window)
{
  CtkShortcutsWindowPrivate *priv = ctk_shortcuts_window_get_instance_private (self);

  if (priv->keys_changed_id)
    {
      g_signal_handler_disconnect (priv->window, priv->keys_changed_id);
      priv->keys_changed_id = 0;
    }

  priv->window = window;

  if (priv->window)
    priv->keys_changed_id = g_signal_connect (window, "keys-changed",
                                              G_CALLBACK (keys_changed_handler),
                                              self);

  update_accels_for_actions (self);
}

static void
ctk_shortcuts_window__list_box__row_activated (CtkShortcutsWindow *self,
                                               CtkListBoxRow      *row,
                                               CtkListBox         *list_box)
{
  CtkShortcutsWindowPrivate *priv = ctk_shortcuts_window_get_instance_private (self);
  CtkWidget *section;

  section = g_object_get_data (G_OBJECT (row), "ctk-shortcuts-section");
  ctk_stack_set_visible_child (priv->stack, section);
  ctk_popover_popdown (priv->popover);
}

static gboolean
hidden_by_direction (CtkWidget *widget)
{
  if (CTK_IS_SHORTCUTS_SHORTCUT (widget))
    {
      CtkTextDirection dir;

      g_object_get (widget, "direction", &dir, NULL);
      if (dir != CTK_TEXT_DIR_NONE &&
          dir != ctk_widget_get_direction (widget))
        return TRUE;
    }

  return FALSE;
}

static void
ctk_shortcuts_window__entry__changed (CtkShortcutsWindow *self,
                                     CtkSearchEntry      *search_entry)
{
  CtkShortcutsWindowPrivate *priv = ctk_shortcuts_window_get_instance_private (self);
  gchar *downcase = NULL;
  GHashTableIter iter;
  const gchar *text;
  const gchar *last_section_name;
  gpointer key;
  gpointer value;
  gboolean has_result;

  text = ctk_entry_get_text (CTK_ENTRY (search_entry));

  if (!text || !*text)
    {
      if (priv->last_section_name != NULL)
        {
          ctk_stack_set_visible_child_name (priv->stack, priv->last_section_name);
          return;

        }
    }

  last_section_name = ctk_stack_get_visible_child_name (priv->stack);

  if (g_strcmp0 (last_section_name, "internal-search") != 0 &&
      g_strcmp0 (last_section_name, "no-search-results") != 0)
    {
      g_free (priv->last_section_name);
      priv->last_section_name = g_strdup (last_section_name);
    }

  downcase = g_utf8_strdown (text, -1);

  g_hash_table_iter_init (&iter, priv->keywords);

  has_result = FALSE;
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      CtkWidget *widget = key;
      const gchar *keywords = value;
      gboolean match;

      if (hidden_by_direction (widget))
        match = FALSE;
      else
        match = strstr (keywords, downcase) != NULL;

      ctk_widget_set_visible (widget, match);
      has_result |= match;
    }

  g_free (downcase);

  if (has_result)
    ctk_stack_set_visible_child_name (priv->stack, "internal-search");
  else
    ctk_stack_set_visible_child_name (priv->stack, "no-search-results");
}

static void
ctk_shortcuts_window__search_mode__changed (CtkShortcutsWindow *self)
{
  CtkShortcutsWindowPrivate *priv = ctk_shortcuts_window_get_instance_private (self);

  if (!ctk_search_bar_get_search_mode (priv->search_bar))
    {
      if (priv->last_section_name != NULL)
        ctk_stack_set_visible_child_name (priv->stack, priv->last_section_name);
    }
}

static void
ctk_shortcuts_window_close (CtkShortcutsWindow *self)
{
  ctk_window_close (CTK_WINDOW (self));
}

static void
ctk_shortcuts_window_search (CtkShortcutsWindow *self)
{
  CtkShortcutsWindowPrivate *priv = ctk_shortcuts_window_get_instance_private (self);

  ctk_search_bar_set_search_mode (priv->search_bar, TRUE);
}

static void
ctk_shortcuts_window_constructed (GObject *object)
{
  CtkShortcutsWindow *self = (CtkShortcutsWindow *)object;
  CtkShortcutsWindowPrivate *priv = ctk_shortcuts_window_get_instance_private (self);

  G_OBJECT_CLASS (ctk_shortcuts_window_parent_class)->constructed (object);

  if (priv->initial_section != NULL)
    ctk_stack_set_visible_child_name (priv->stack, priv->initial_section);
}

static void
ctk_shortcuts_window_finalize (GObject *object)
{
  CtkShortcutsWindow *self = (CtkShortcutsWindow *)object;
  CtkShortcutsWindowPrivate *priv = ctk_shortcuts_window_get_instance_private (self);

  g_clear_pointer (&priv->keywords, g_hash_table_unref);
  g_clear_pointer (&priv->initial_section, g_free);
  g_clear_pointer (&priv->view_name, g_free);
  g_clear_pointer (&priv->last_section_name, g_free);
  g_clear_pointer (&priv->search_items_hash, g_hash_table_unref);

  g_clear_object (&priv->search_image_group);
  g_clear_object (&priv->search_text_group);

  G_OBJECT_CLASS (ctk_shortcuts_window_parent_class)->finalize (object);
}

static void
ctk_shortcuts_window_dispose (GObject *object)
{
  CtkShortcutsWindow *self = (CtkShortcutsWindow *)object;
  CtkShortcutsWindowPrivate *priv = ctk_shortcuts_window_get_instance_private (self);

  g_signal_handlers_disconnect_by_func (priv->stack, G_CALLBACK (update_title_stack), self);

  ctk_shortcuts_window_set_window (self, NULL);

  if (priv->header_bar)
    {
      ctk_widget_destroy (CTK_WIDGET (priv->header_bar));
      priv->header_bar = NULL;
      priv->popover = NULL;
    }

  G_OBJECT_CLASS (ctk_shortcuts_window_parent_class)->dispose (object);

#if 0
  if (priv->main_box)
    {
      ctk_widget_destroy (CTK_WIDGET (priv->main_box));
      priv->main_box = NULL;
    }
#endif
}

static void
ctk_shortcuts_window_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  CtkShortcutsWindow *self = (CtkShortcutsWindow *)object;
  CtkShortcutsWindowPrivate *priv = ctk_shortcuts_window_get_instance_private (self);

  switch (prop_id)
    {
    case PROP_SECTION_NAME:
      {
        CtkWidget *child = ctk_stack_get_visible_child (priv->stack);

        if (child != NULL)
          {
            gchar *name = NULL;

            ctk_container_child_get (CTK_CONTAINER (priv->stack), child,
                                     "name", &name,
                                     NULL);
            g_value_take_string (value, name);
          }
      }
      break;

    case PROP_VIEW_NAME:
      g_value_set_string (value, priv->view_name);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
ctk_shortcuts_window_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  CtkShortcutsWindow *self = (CtkShortcutsWindow *)object;

  switch (prop_id)
    {
    case PROP_SECTION_NAME:
      ctk_shortcuts_window_set_section_name (self, g_value_get_string (value));
      break;

    case PROP_VIEW_NAME:
      ctk_shortcuts_window_set_view_name (self, g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
ctk_shortcuts_window_unmap (CtkWidget *widget)
{
  CtkShortcutsWindow *self = (CtkShortcutsWindow *)widget;
  CtkShortcutsWindowPrivate *priv = ctk_shortcuts_window_get_instance_private (self);

  ctk_search_bar_set_search_mode (priv->search_bar, FALSE);

  CTK_WIDGET_CLASS (ctk_shortcuts_window_parent_class)->unmap (widget);
}

static GType
ctk_shortcuts_window_child_type (CtkContainer *container)
{
  return CTK_TYPE_SHORTCUTS_SECTION;
}

static void
ctk_shortcuts_window_class_init (CtkShortcutsWindowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);
  CtkContainerClass *container_class = CTK_CONTAINER_CLASS (klass);
  CtkBindingSet *binding_set = ctk_binding_set_by_class (klass);

  object_class->constructed = ctk_shortcuts_window_constructed;
  object_class->finalize = ctk_shortcuts_window_finalize;
  object_class->get_property = ctk_shortcuts_window_get_property;
  object_class->set_property = ctk_shortcuts_window_set_property;
  object_class->dispose = ctk_shortcuts_window_dispose;

  widget_class->unmap = ctk_shortcuts_window_unmap;
  container_class->add = ctk_shortcuts_window_add;
  container_class->remove = ctk_shortcuts_window_remove;
  container_class->child_type = ctk_shortcuts_window_child_type;
  container_class->forall = ctk_shortcuts_window_forall;

  klass->close = ctk_shortcuts_window_close;
  klass->search = ctk_shortcuts_window_search;

  /**
   * CtkShortcutsWindow:section-name:
   *
   * The name of the section to show.
   *
   * This should be the section-name of one of the #CtkShortcutsSection
   * objects that are in this shortcuts window.
   */
  properties[PROP_SECTION_NAME] =
    g_param_spec_string ("section-name", P_("Section Name"), P_("Section Name"),
                         "internal-search",
                         (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * CtkShortcutsWindow:view-name:
   *
   * The view name by which to filter the contents.
   *
   * This should correspond to the #CtkShortcutsGroup:view property of some of
   * the #CtkShortcutsGroup objects that are inside this shortcuts window.
   *
   * Set this to %NULL to show all groups.
   */
  properties[PROP_VIEW_NAME] =
    g_param_spec_string ("view-name", P_("View Name"), P_("View Name"),
                         NULL,
                         (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, LAST_PROP, properties);

  /**
   * CtkShortcutsWindow::close:
   *
   * The ::close signal is a
   * [keybinding signal][CtkBindingSignal]
   * which gets emitted when the user uses a keybinding to close
   * the window.
   *
   * The default binding for this signal is the Escape key.
   */
  signals[CLOSE] = g_signal_new (I_("close"),
                                 G_TYPE_FROM_CLASS (klass),
                                 G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                 G_STRUCT_OFFSET (CtkShortcutsWindowClass, close),
                                 NULL, NULL, NULL,
                                 G_TYPE_NONE,
                                 0);

  /**
   * CtkShortcutsWindow::search:
   *
   * The ::search signal is a
   * [keybinding signal][CtkBindingSignal]
   * which gets emitted when the user uses a keybinding to start a search.
   *
   * The default binding for this signal is Control-F.
   */
  signals[SEARCH] = g_signal_new (I_("search"),
                                 G_TYPE_FROM_CLASS (klass),
                                 G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                 G_STRUCT_OFFSET (CtkShortcutsWindowClass, search),
                                 NULL, NULL, NULL,
                                 G_TYPE_NONE,
                                 0);

  ctk_binding_entry_add_signal (binding_set, GDK_KEY_Escape, 0, "close", 0);
  ctk_binding_entry_add_signal (binding_set, GDK_KEY_f, GDK_CONTROL_MASK, "search", 0);

  g_type_ensure (CTK_TYPE_SHORTCUTS_GROUP);
  g_type_ensure (CTK_TYPE_SHORTCUTS_SHORTCUT);
}

static gboolean
window_key_press_event_cb (CtkWidget *window,
                           CdkEvent  *event,
                           gpointer   data)
{
  CtkShortcutsWindow *self = CTK_SHORTCUTS_WINDOW (window);
  CtkShortcutsWindowPrivate *priv = ctk_shortcuts_window_get_instance_private (self);

  return ctk_search_bar_handle_event (priv->search_bar, event);
}

static void
ctk_shortcuts_window_init (CtkShortcutsWindow *self)
{
  CtkShortcutsWindowPrivate *priv = ctk_shortcuts_window_get_instance_private (self);
  CtkToggleButton *search_button;
  CtkBox *menu_box;
  CtkBox *box;
  CtkArrow *arrow;
  CtkWidget *scroller;
  CtkWidget *label;
  CtkWidget *empty;
  PangoAttrList *attributes;

  ctk_window_set_resizable (CTK_WINDOW (self), FALSE);
  ctk_window_set_type_hint (CTK_WINDOW (self), GDK_WINDOW_TYPE_HINT_DIALOG);

  g_signal_connect (self, "key-press-event",
                    G_CALLBACK (window_key_press_event_cb), NULL);

  priv->keywords = g_hash_table_new_full (NULL, NULL, NULL, g_free);
  priv->search_items_hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

  priv->search_text_group = ctk_size_group_new (CTK_SIZE_GROUP_HORIZONTAL);
  priv->search_image_group = ctk_size_group_new (CTK_SIZE_GROUP_HORIZONTAL);

  priv->header_bar = g_object_new (CTK_TYPE_HEADER_BAR,
                                   "show-close-button", TRUE,
                                   "visible", TRUE,
                                   NULL);
  ctk_window_set_titlebar (CTK_WINDOW (self), CTK_WIDGET (priv->header_bar));

  search_button = g_object_new (CTK_TYPE_TOGGLE_BUTTON,
                                "child", g_object_new (CTK_TYPE_IMAGE,
                                                       "visible", TRUE,
                                                       "icon-name", "edit-find-symbolic",
                                                       NULL),
                                "visible", TRUE,
                                NULL);
  ctk_style_context_add_class (ctk_widget_get_style_context (CTK_WIDGET (search_button)), "image-button");
  ctk_container_add (CTK_CONTAINER (priv->header_bar), CTK_WIDGET (search_button));

  priv->main_box = g_object_new (CTK_TYPE_BOX,
                           "orientation", CTK_ORIENTATION_VERTICAL,
                           "visible", TRUE,
                           NULL);
  CTK_CONTAINER_CLASS (ctk_shortcuts_window_parent_class)->add (CTK_CONTAINER (self), CTK_WIDGET (priv->main_box));

  priv->search_bar = g_object_new (CTK_TYPE_SEARCH_BAR,
                                   "visible", TRUE,
                                   NULL);
  g_object_bind_property (priv->search_bar, "search-mode-enabled",
                          search_button, "active",
                          G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
  ctk_container_add (CTK_CONTAINER (priv->main_box), CTK_WIDGET (priv->search_bar));

  priv->stack = g_object_new (CTK_TYPE_STACK,
                              "expand", TRUE,
                              "homogeneous", TRUE,
                              "transition-type", CTK_STACK_TRANSITION_TYPE_CROSSFADE,
                              "visible", TRUE,
                              NULL);
  ctk_container_add (CTK_CONTAINER (priv->main_box), CTK_WIDGET (priv->stack));

  priv->title_stack = g_object_new (CTK_TYPE_STACK,
                                    "visible", TRUE,
                                    NULL);
  ctk_header_bar_set_custom_title (priv->header_bar, CTK_WIDGET (priv->title_stack));

  label = ctk_label_new (_("Shortcuts"));
  ctk_widget_show (label);
  ctk_style_context_add_class (ctk_widget_get_style_context (label), CTK_STYLE_CLASS_TITLE);
  ctk_stack_add_named (priv->title_stack, label, "title");

  label = ctk_label_new (_("Search Results"));
  ctk_widget_show (label);
  ctk_style_context_add_class (ctk_widget_get_style_context (label), CTK_STYLE_CLASS_TITLE);
  ctk_stack_add_named (priv->title_stack, label, "search");

  priv->menu_button = g_object_new (CTK_TYPE_MENU_BUTTON,
                                    "focus-on-click", FALSE,
                                    "visible", TRUE,
                                    "relief", CTK_RELIEF_NONE,
                                    NULL);
  ctk_stack_add_named (priv->title_stack, CTK_WIDGET (priv->menu_button), "sections");

  menu_box = g_object_new (CTK_TYPE_BOX,
                           "orientation", CTK_ORIENTATION_HORIZONTAL,
                           "spacing", 6,
                           "visible", TRUE,
                           NULL);
  ctk_container_add (CTK_CONTAINER (priv->menu_button), CTK_WIDGET (menu_box));

  priv->menu_label = g_object_new (CTK_TYPE_LABEL,
                                   "visible", TRUE,
                                   NULL);
  ctk_container_add (CTK_CONTAINER (menu_box), CTK_WIDGET (priv->menu_label));

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  arrow = g_object_new (CTK_TYPE_ARROW,
                        "arrow-type", CTK_ARROW_DOWN,
                        "visible", TRUE,
                        NULL);
  ctk_container_add (CTK_CONTAINER (menu_box), CTK_WIDGET (arrow));
  G_GNUC_END_IGNORE_DEPRECATIONS;

  priv->popover = g_object_new (CTK_TYPE_POPOVER,
                                "border-width", 6,
                                "relative-to", priv->menu_button,
                                "position", CTK_POS_BOTTOM,
                                NULL);
  ctk_menu_button_set_popover (priv->menu_button, CTK_WIDGET (priv->popover));

  priv->list_box = g_object_new (CTK_TYPE_LIST_BOX,
                                 "selection-mode", CTK_SELECTION_NONE,
                                 "visible", TRUE,
                                 NULL);
  g_signal_connect_object (priv->list_box,
                           "row-activated",
                           G_CALLBACK (ctk_shortcuts_window__list_box__row_activated),
                           self,
                           G_CONNECT_SWAPPED);
  ctk_container_add (CTK_CONTAINER (priv->popover), CTK_WIDGET (priv->list_box));

  priv->search_entry = CTK_SEARCH_ENTRY (ctk_search_entry_new ());
  ctk_widget_show (CTK_WIDGET (priv->search_entry));
  ctk_container_add (CTK_CONTAINER (priv->search_bar), CTK_WIDGET (priv->search_entry));
  g_object_set (priv->search_entry,
                "placeholder-text", _("Search Shortcuts"),
                "width-chars", 40,
                NULL);
  g_signal_connect_object (priv->search_entry,
                           "search-changed",
                           G_CALLBACK (ctk_shortcuts_window__entry__changed),
                           self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (priv->search_bar,
                           "notify::search-mode-enabled",
                           G_CALLBACK (ctk_shortcuts_window__search_mode__changed),
                           self,
                           G_CONNECT_SWAPPED);

  scroller = g_object_new (CTK_TYPE_SCROLLED_WINDOW,
                           "visible", TRUE,
                           NULL);
  box = g_object_new (CTK_TYPE_BOX,
                      "border-width", 24,
                      "halign", CTK_ALIGN_CENTER,
                      "spacing", 24,
                      "orientation", CTK_ORIENTATION_VERTICAL,
                      "visible", TRUE,
                      NULL);
  ctk_container_add (CTK_CONTAINER (scroller), CTK_WIDGET (box));
  ctk_stack_add_named (priv->stack, scroller, "internal-search");

  priv->search_shortcuts = g_object_new (CTK_TYPE_BOX,
                                         "halign", CTK_ALIGN_CENTER,
                                         "spacing", 6,
                                         "orientation", CTK_ORIENTATION_VERTICAL,
                                         "visible", TRUE,
                                         NULL);
  ctk_container_add (CTK_CONTAINER (box), CTK_WIDGET (priv->search_shortcuts));

  priv->search_gestures = g_object_new (CTK_TYPE_BOX,
                                        "halign", CTK_ALIGN_CENTER,
                                        "spacing", 6,
                                        "orientation", CTK_ORIENTATION_VERTICAL,
                                        "visible", TRUE,
                                        NULL);
  ctk_container_add (CTK_CONTAINER (box), CTK_WIDGET (priv->search_gestures));

  empty = g_object_new (CTK_TYPE_GRID,
                        "visible", TRUE,
                        "row-spacing", 12,
                        "margin", 12,
                        "hexpand", TRUE,
                        "vexpand", TRUE,
                        "halign", CTK_ALIGN_CENTER,
                        "valign", CTK_ALIGN_CENTER,
                        NULL);
  ctk_style_context_add_class (ctk_widget_get_style_context (empty), CTK_STYLE_CLASS_DIM_LABEL);
  ctk_grid_attach (CTK_GRID (empty),
                   g_object_new (CTK_TYPE_IMAGE,
                                 "visible", TRUE,
                                 "icon-name", "edit-find-symbolic",
                                 "pixel-size", 72,
                                 NULL),
                   0, 0, 1, 1);
  attributes = pango_attr_list_new ();
  pango_attr_list_insert (attributes, pango_attr_weight_new (PANGO_WEIGHT_BOLD));
  pango_attr_list_insert (attributes, pango_attr_scale_new (1.44));
  label = g_object_new (CTK_TYPE_LABEL,
                        "visible", TRUE,
                        "label", _("No Results Found"),
                        "attributes", attributes,
                        NULL);
  pango_attr_list_unref (attributes);
  ctk_grid_attach (CTK_GRID (empty), label, 0, 1, 1, 1);
  label = g_object_new (CTK_TYPE_LABEL,
                        "visible", TRUE,
                        "label", _("Try a different search"),
                        NULL);
  ctk_grid_attach (CTK_GRID (empty), label, 0, 2, 1, 1);

  ctk_stack_add_named (priv->stack, empty, "no-search-results");

  g_signal_connect_object (priv->stack, "notify::visible-child",
                           G_CALLBACK (update_title_stack), self, G_CONNECT_SWAPPED);

}
