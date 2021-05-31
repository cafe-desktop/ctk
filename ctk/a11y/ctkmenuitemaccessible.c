/* GTK+ - accessibility implementations
 * Copyright 2001, 2002, 2003 Sun Microsystems Inc.
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

#include "config.h"

#include <glib/gi18n-lib.h>
#include <string.h>
#include <gtk/gtk.h>
#include "gtkmenuitemaccessible.h"
#include "gtkwidgetaccessibleprivate.h"
#include "gtk/gtkmenuitemprivate.h"

struct _GtkMenuItemAccessiblePrivate
{
  gchar *text;
  gboolean selected;
};

#define KEYBINDING_SEPARATOR ";"

static void menu_item_select   (GtkMenuItem *item);
static void menu_item_deselect (GtkMenuItem *item);

static GtkWidget *get_label_from_container   (GtkWidget *container);
static gchar     *get_text_from_label_widget (GtkWidget *widget);

static gint menu_item_insert_gtk (GtkMenuShell   *shell,
                                  GtkWidget      *widget,
                                  gint            position);
static gint menu_item_remove_gtk (GtkContainer   *container,
                                  GtkWidget      *widget);

static void atk_action_interface_init    (AtkActionIface *iface);
static void atk_selection_interface_init (AtkSelectionIface *iface);

G_DEFINE_TYPE_WITH_CODE (GtkMenuItemAccessible, ctk_menu_item_accessible, CTK_TYPE_CONTAINER_ACCESSIBLE,
                         G_ADD_PRIVATE (GtkMenuItemAccessible)
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_ACTION, atk_action_interface_init);
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_SELECTION, atk_selection_interface_init))

static void
ctk_menu_item_accessible_initialize (AtkObject *obj,
                                     gpointer   data)
{
  GtkWidget *widget;
  GtkWidget *parent;
  GtkWidget *menu;

  ATK_OBJECT_CLASS (ctk_menu_item_accessible_parent_class)->initialize (obj, data);
  g_signal_connect (data, "select", G_CALLBACK (menu_item_select), NULL);
  g_signal_connect (data, "deselect", G_CALLBACK (menu_item_deselect), NULL);

  widget = CTK_WIDGET (data);
  if ((ctk_widget_get_state_flags (widget) & CTK_STATE_FLAG_PRELIGHT) != 0)
    CTK_MENU_ITEM_ACCESSIBLE (obj)->priv->selected = TRUE;

  parent = ctk_widget_get_parent (widget);
  if (CTK_IS_MENU (parent))
    {
      GtkWidget *parent_widget;

      parent_widget =  ctk_menu_get_attach_widget (CTK_MENU (parent));

      if (!CTK_IS_MENU_ITEM (parent_widget))
        parent_widget = ctk_widget_get_parent (widget);
      if (parent_widget)
        atk_object_set_parent (obj, ctk_widget_get_accessible (parent_widget));
    }

  _ctk_widget_accessible_set_layer (CTK_WIDGET_ACCESSIBLE (obj), ATK_LAYER_POPUP);

  obj->role = ATK_ROLE_MENU_ITEM;

  menu = ctk_menu_item_get_submenu (CTK_MENU_ITEM (data));
  if (menu)
    {
      g_signal_connect (menu, "insert", G_CALLBACK (menu_item_insert_gtk), NULL);
      g_signal_connect (menu, "remove", G_CALLBACK (menu_item_remove_gtk), NULL);
    }
}

static gint
ctk_menu_item_accessible_get_n_children (AtkObject *obj)
{
  GtkWidget *widget;
  GtkWidget *submenu;
  gint count = 0;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  if (widget == NULL)
    return count;

  submenu = ctk_menu_item_get_submenu (CTK_MENU_ITEM (widget));
  if (submenu)
    {
      GList *children;

      children = ctk_container_get_children (CTK_CONTAINER (submenu));
      count = g_list_length (children);
      g_list_free (children);
    }
  return count;
}

static AtkObject *
ctk_menu_item_accessible_ref_child (AtkObject *obj,
                                    gint       i)
{
  AtkObject  *accessible;
  GtkWidget *widget;
  GtkWidget *submenu;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  if (widget == NULL)
    return NULL;

  accessible = NULL;
  submenu = ctk_menu_item_get_submenu (CTK_MENU_ITEM (widget));
  if (submenu)
    {
      GList *children;
      GList *tmp_list;

      children = ctk_container_get_children (CTK_CONTAINER (submenu));
      tmp_list = g_list_nth (children, i);
      if (tmp_list)
        {
          accessible = ctk_widget_get_accessible (CTK_WIDGET (tmp_list->data));
          g_object_ref (accessible);
        }
      g_list_free (children);
    }

  return accessible;
}

static AtkStateSet *
ctk_menu_item_accessible_ref_state_set (AtkObject *obj)
{
  AtkObject *menu_item;
  AtkStateSet *state_set, *parent_state_set;

  state_set = ATK_OBJECT_CLASS (ctk_menu_item_accessible_parent_class)->ref_state_set (obj);

  atk_state_set_add_state (state_set, ATK_STATE_SELECTABLE);
  if (CTK_MENU_ITEM_ACCESSIBLE (obj)->priv->selected)
    atk_state_set_add_state (state_set, ATK_STATE_SELECTED);

  menu_item = atk_object_get_parent (obj);

  if (menu_item)
    {
      if (!CTK_IS_MENU_ITEM (ctk_accessible_get_widget (CTK_ACCESSIBLE (menu_item))))
        return state_set;

      parent_state_set = atk_object_ref_state_set (menu_item);
      if (!atk_state_set_contains_state (parent_state_set, ATK_STATE_SELECTED))
        {
          atk_state_set_remove_state (state_set, ATK_STATE_FOCUSED);
          atk_state_set_remove_state (state_set, ATK_STATE_SHOWING);
        }
      g_object_unref (parent_state_set);
    }

  return state_set;
}

static AtkRole
ctk_menu_item_accessible_get_role (AtkObject *obj)
{
  GtkWidget *widget;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  if (widget != NULL &&
      ctk_menu_item_get_submenu (CTK_MENU_ITEM (widget)))
    return ATK_ROLE_MENU;

  return ATK_OBJECT_CLASS (ctk_menu_item_accessible_parent_class)->get_role (obj);
}

static const gchar *
ctk_menu_item_accessible_get_name (AtkObject *obj)
{
  const gchar *name;
  GtkWidget *widget;
  GtkWidget *label;
  GtkMenuItemAccessible *accessible;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  if (widget == NULL)
    return NULL;

  name = ATK_OBJECT_CLASS (ctk_menu_item_accessible_parent_class)->get_name (obj);
  if (name)
    return name;

  accessible = CTK_MENU_ITEM_ACCESSIBLE (obj);
  label = get_label_from_container (widget);

  g_free (accessible->priv->text);
  accessible->priv->text = get_text_from_label_widget (label);

  return accessible->priv->text;
}

static void
ctk_menu_item_accessible_finalize (GObject *object)
{
  GtkMenuItemAccessible *accessible = CTK_MENU_ITEM_ACCESSIBLE (object);

  g_free (accessible->priv->text);

  G_OBJECT_CLASS (ctk_menu_item_accessible_parent_class)->finalize (object);
}

static void
ctk_menu_item_accessible_notify_gtk (GObject    *obj,
                                     GParamSpec *pspec)
{
  AtkObject* atk_obj;

  atk_obj = ctk_widget_get_accessible (CTK_WIDGET (obj));

  if (strcmp (pspec->name, "label") == 0)
    {
      if (atk_obj->name == NULL)
        g_object_notify (G_OBJECT (atk_obj), "accessible-name");
      g_signal_emit_by_name (atk_obj, "visible-data-changed");
    }
  else
    CTK_WIDGET_ACCESSIBLE_CLASS (ctk_menu_item_accessible_parent_class)->notify_gtk (obj, pspec);
}

static void
ctk_menu_item_accessible_class_init (GtkMenuItemAccessibleClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);
  GtkWidgetAccessibleClass *widget_class = (GtkWidgetAccessibleClass*)klass;

  widget_class->notify_gtk = ctk_menu_item_accessible_notify_gtk;

  gobject_class->finalize = ctk_menu_item_accessible_finalize;

  class->get_n_children = ctk_menu_item_accessible_get_n_children;
  class->ref_child = ctk_menu_item_accessible_ref_child;
  class->ref_state_set = ctk_menu_item_accessible_ref_state_set;
  class->initialize = ctk_menu_item_accessible_initialize;
  class->get_name = ctk_menu_item_accessible_get_name;
  class->get_role = ctk_menu_item_accessible_get_role;
}

static void
ctk_menu_item_accessible_init (GtkMenuItemAccessible *menu_item)
{
  menu_item->priv = ctk_menu_item_accessible_get_instance_private (menu_item);
}

static GtkWidget *
get_label_from_container (GtkWidget *container)
{
  GtkWidget *label;
  GList *children, *tmp_list;

  if (!CTK_IS_CONTAINER (container))
    return NULL;

  children = ctk_container_get_children (CTK_CONTAINER (container));
  label = NULL;

  for (tmp_list = children; tmp_list != NULL; tmp_list = tmp_list->next)
    {
      if (CTK_IS_LABEL (tmp_list->data))
        {
          label = tmp_list->data;
          break;
        }
      else if (CTK_IS_CELL_VIEW (tmp_list->data))
        {
          label = tmp_list->data;
          break;
        }
      else if (CTK_IS_BOX (tmp_list->data))
        {
          label = get_label_from_container (CTK_WIDGET (tmp_list->data));
          if (label)
            break;
        }
    }
  g_list_free (children);

  return label;
}

static gchar *
get_text_from_label_widget (GtkWidget *label)
{
  if (CTK_IS_LABEL (label))
    return g_strdup (ctk_label_get_text (CTK_LABEL (label)));
  else if (CTK_IS_CELL_VIEW (label))
    {
      GList *cells, *l;
      GtkTreeModel *model;
      GtkTreeIter iter;
      GtkTreePath *path;
      GtkCellArea *area;
      gchar *text;

      model = ctk_cell_view_get_model (CTK_CELL_VIEW (label));
      path = ctk_cell_view_get_displayed_row (CTK_CELL_VIEW (label));
      ctk_tree_model_get_iter (model, &iter, path);
      ctk_tree_path_free (path);

      area = ctk_cell_layout_get_area (CTK_CELL_LAYOUT (label));
      ctk_cell_area_apply_attributes (area, model, &iter, FALSE, FALSE);
      cells = ctk_cell_layout_get_cells (CTK_CELL_LAYOUT (label));

      text = NULL;
      for (l = cells; l; l = l->next)
        {
          GtkCellRenderer *cell = l->data;

          if (CTK_IS_CELL_RENDERER_TEXT (cell))
            {
              g_object_get (cell, "text", &text, NULL);
              break;
            }
        }

      g_list_free (cells);

      return text;
    }

  return NULL;
}

static void
ensure_menus_unposted (GtkMenuItemAccessible *menu_item)
{
  AtkObject *parent;
  GtkWidget *widget;

  parent = atk_object_get_parent (ATK_OBJECT (menu_item));
  while (parent)
    {
      widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (parent));
      if (CTK_IS_MENU (widget))
        {
          if (ctk_widget_get_mapped (widget))
            ctk_menu_shell_cancel (CTK_MENU_SHELL (widget));

          return;
        }
      parent = atk_object_get_parent (parent);
    }
}

static gboolean
ctk_menu_item_accessible_do_action (AtkAction *action,
                                    gint       i)
{
  GtkWidget *item, *item_parent;
  gboolean item_mapped;

  item = ctk_accessible_get_widget (CTK_ACCESSIBLE (action));
  if (item == NULL)
    return FALSE;

  if (i != 0)
    return FALSE;

  if (!ctk_widget_get_sensitive (item) || !ctk_widget_get_visible (item))
    return FALSE;

  item_parent = ctk_widget_get_parent (item);
  if (!CTK_IS_MENU_SHELL (item_parent))
    return FALSE;

  ctk_menu_shell_select_item (CTK_MENU_SHELL (item_parent), item);
  item_mapped = ctk_widget_get_mapped (item);

  /* This is what is called when <Return> is pressed for a menu item.
   * The last argument means 'force hide'.
   */
  g_signal_emit_by_name (item_parent, "activate-current", 1);
  if (!item_mapped)
    ensure_menus_unposted (CTK_MENU_ITEM_ACCESSIBLE (action));

  return TRUE;
}

static gint
ctk_menu_item_accessible_get_n_actions (AtkAction *action)
{
  GtkWidget *item;

  item = ctk_accessible_get_widget (CTK_ACCESSIBLE (action));
  if (item == NULL)
    return 0;

  if (!_ctk_menu_item_is_selectable (item))
    return 0;

  return 1;
}

static const gchar *
ctk_menu_item_accessible_action_get_name (AtkAction *action,
                                          gint       i)
{
  if (i == 0 && ctk_menu_item_accessible_get_n_actions (action) > 0)
    return "click";
  return NULL;
}

static const gchar *
ctk_menu_item_accessible_action_get_localized_name (AtkAction *action,
                                                    gint       i)
{
  if (i == 0 && ctk_menu_item_accessible_get_n_actions (action) > 0)
    return C_("Action name", "Click");
  return NULL;
}

static const gchar *
ctk_menu_item_accessible_action_get_description (AtkAction *action,
                                                 gint       i)
{
  if (i == 0 && ctk_menu_item_accessible_get_n_actions (action) > 0)
    return C_("Action description", "Clicks the menuitem");
  return NULL;
}

static gboolean
find_accel_by_widget (GtkAccelKey *key,
                      GClosure    *closure,
                      gpointer     data)
{
  /* We assume that closure->data points to the widget
   * pending ctk_widget_get_accel_closures being made public
   */
  return data == (gpointer) closure->data;
}

static gboolean
find_accel_by_closure (GtkAccelKey *key,
                       GClosure    *closure,
                       gpointer     data)
{
  return data == (gpointer) closure;
}

static GtkWidget *
find_item_label (GtkWidget *item)
{
  GtkWidget *child;

  child = ctk_bin_get_child (CTK_BIN (item));
  if (CTK_IS_CONTAINER (child))
    {
      GList *children, *l;
      children = ctk_container_get_children (CTK_CONTAINER (child));
      for (l = children; l; l = l->next)
        {
          if (CTK_IS_LABEL (l->data))
            {
              child = l->data;
              break;
            }
        }
      g_list_free (children);
    }

  if (CTK_IS_LABEL (child))
    return child;

  return NULL;
}

/* This function returns a string of the form A;B;C where A is
 * the keybinding for the widget; B is the keybinding to traverse
 * from the menubar and C is the accelerator. The items in the
 * keybinding to traverse from the menubar are separated by “:”.
 */
static const gchar *
ctk_menu_item_accessible_get_keybinding (AtkAction *action,
                                         gint       i)
{
  gchar *keybinding = NULL;
  gchar *item_keybinding = NULL;
  gchar *full_keybinding = NULL;
  gchar *accelerator = NULL;
  GtkWidget *item;
  GtkWidget *temp_item;
  GtkWidget *child;
  GtkWidget *parent;

  item = ctk_accessible_get_widget (CTK_ACCESSIBLE (action));
  if (item == NULL)
    return NULL;

  if (i != 0)
    return NULL;

  temp_item = item;
  while (TRUE)
    {
      GdkModifierType mnemonic_modifier = 0;
      guint key_val;
      gchar *key, *temp_keybinding;

      if (ctk_bin_get_child (CTK_BIN (temp_item)) == NULL)
        return NULL;

      parent = ctk_widget_get_parent (temp_item);
      if (!parent)
        /* parent can be NULL when activating a window from the panel */
        return NULL;

      if (CTK_IS_MENU_BAR (parent))
        {
          GtkWidget *toplevel;

          toplevel = ctk_widget_get_toplevel (parent);
          if (toplevel && CTK_IS_WINDOW (toplevel))
            mnemonic_modifier =
              ctk_window_get_mnemonic_modifier (CTK_WINDOW (toplevel));
        }

      child = find_item_label (temp_item);
      if (CTK_IS_LABEL (child))
        {
          key_val = ctk_label_get_mnemonic_keyval (CTK_LABEL (child));
          if (key_val != GDK_KEY_VoidSymbol)
            {
              key = ctk_accelerator_name (key_val, mnemonic_modifier);
              if (full_keybinding)
                temp_keybinding = g_strconcat (key, ":", full_keybinding, NULL);
              else
                temp_keybinding = g_strdup (key);

              if (temp_item == item)
                item_keybinding = g_strdup (key);

              g_free (key);
              g_free (full_keybinding);
              full_keybinding = temp_keybinding;
            }
          else
            {
              /* No keybinding */
              g_free (full_keybinding);
              full_keybinding = NULL;
              break;
            }
        }

      /* We have reached the menu bar so we are finished */
      if (CTK_IS_MENU_BAR (parent))
        break;

      g_return_val_if_fail (CTK_IS_MENU (parent), NULL);
      temp_item = ctk_menu_get_attach_widget (CTK_MENU (parent));
      if (!CTK_IS_MENU_ITEM (temp_item))
        {
          /* Menu is attached to something other than a menu item;
           * probably an option menu
           */
          g_free (full_keybinding);
          full_keybinding = NULL;
          break;
        }
    }

  parent = ctk_widget_get_parent (item);
  if (CTK_IS_MENU (parent))
    {
      child = find_item_label (item);
      if (CTK_IS_ACCEL_LABEL (child))
        {
          guint accel_key;
          GdkModifierType accel_mods;

          ctk_accel_label_get_accel (CTK_ACCEL_LABEL (child), &accel_key, &accel_mods);

          if (accel_key)
            accelerator = ctk_accelerator_name (accel_key, accel_mods);
        }
        
      if (!accelerator)
        {
          GtkAccelGroup *group;
          GtkAccelKey *key = NULL;

          group = ctk_menu_get_accel_group (CTK_MENU (parent));
          if (group)
            key = ctk_accel_group_find (group, find_accel_by_widget, item);
          else if (CTK_IS_ACCEL_LABEL (child))
            {
              GtkAccelLabel *accel_label;
              GClosure      *accel_closure;

              accel_label = CTK_ACCEL_LABEL (child);
              g_object_get (accel_label, "accel-closure", &accel_closure, NULL);
              if (accel_closure)
                {
                  key = ctk_accel_group_find (ctk_accel_group_from_accel_closure (accel_closure),
                                              find_accel_by_closure,
                                              accel_closure);
                  g_closure_unref (accel_closure);
                }
            }

         if (key)
           accelerator = ctk_accelerator_name (key->accel_key, key->accel_mods);
        }
   }

  /* Concatenate the bindings */
  if (item_keybinding || full_keybinding || accelerator)
    {
      gchar *temp;
      if (item_keybinding)
        {
          keybinding = g_strconcat (item_keybinding, KEYBINDING_SEPARATOR, NULL);
          g_free (item_keybinding);
        }
      else
        keybinding = g_strdup (KEYBINDING_SEPARATOR);

      if (full_keybinding)
        {
          temp = g_strconcat (keybinding, full_keybinding,
                              KEYBINDING_SEPARATOR, NULL);
          g_free (full_keybinding);
        }
      else
        temp = g_strconcat (keybinding, KEYBINDING_SEPARATOR, NULL);

      g_free (keybinding);
      keybinding = temp;
      if (accelerator)
        {
          temp = g_strconcat (keybinding, accelerator, NULL);
          g_free (accelerator);
          g_free (keybinding);
          keybinding = temp;
      }
    }

  return keybinding;
}

static void
atk_action_interface_init (AtkActionIface *iface)
{
  iface->do_action = ctk_menu_item_accessible_do_action;
  iface->get_n_actions = ctk_menu_item_accessible_get_n_actions;
  iface->get_name = ctk_menu_item_accessible_action_get_name;
  iface->get_localized_name = ctk_menu_item_accessible_action_get_localized_name;
  iface->get_description = ctk_menu_item_accessible_action_get_description;
  iface->get_keybinding = ctk_menu_item_accessible_get_keybinding;
}

static void
menu_item_selection (GtkMenuItem  *item,
                     gboolean      selected)
{
  AtkObject *obj, *parent;
  gint i;

  obj = ctk_widget_get_accessible (CTK_WIDGET (item));
  CTK_MENU_ITEM_ACCESSIBLE (obj)->priv->selected = selected;
  atk_object_notify_state_change (obj, ATK_STATE_SELECTED, selected);

  for (i = 0; i < atk_object_get_n_accessible_children (obj); i++)
    {
      AtkObject *child;
      child = atk_object_ref_accessible_child (obj, i);
      atk_object_notify_state_change (child, ATK_STATE_SHOWING, selected);
      g_object_unref (child);
    }
  parent = atk_object_get_parent (obj);
  g_signal_emit_by_name (parent, "selection-changed");
}

static gboolean
ctk_menu_item_accessible_add_selection (AtkSelection *selection,
                                           gint          i)
{
  GtkMenuShell *shell;
  GList *kids;
  guint length;
  GtkWidget *widget;
  GtkWidget *menu;
  GtkWidget *child;

  widget =  ctk_accessible_get_widget (CTK_ACCESSIBLE (selection));
  if (widget == NULL)
    return FALSE;

  menu = ctk_menu_item_get_submenu (CTK_MENU_ITEM (widget));
  if (menu == NULL)
    return FALSE;

  shell = CTK_MENU_SHELL (menu);
  kids = ctk_container_get_children (CTK_CONTAINER (shell));
  length = g_list_length (kids);
  if (i < 0 || i > length)
    {
      g_list_free (kids);
      return FALSE;
    }

  child = g_list_nth_data (kids, i);
  g_list_free (kids);
  g_return_val_if_fail (CTK_IS_MENU_ITEM (child), FALSE);
  ctk_menu_shell_select_item (shell, CTK_WIDGET (child));
  return TRUE;
}

static gboolean
ctk_menu_item_accessible_clear_selection (AtkSelection *selection)
{
  GtkWidget *widget;
  GtkWidget *menu;

  widget =  ctk_accessible_get_widget (CTK_ACCESSIBLE (selection));
  if (widget == NULL)
    return FALSE;

  menu = ctk_menu_item_get_submenu (CTK_MENU_ITEM (widget));
  if (menu == NULL)
    return FALSE;

  ctk_menu_shell_deselect (CTK_MENU_SHELL (menu));

  return TRUE;
}

static AtkObject *
ctk_menu_item_accessible_ref_selection (AtkSelection *selection,
                                           gint          i)
{
  GtkMenuShell *shell;
  AtkObject *obj;
  GtkWidget *widget;
  GtkWidget *menu;
  GtkWidget *item;

  if (i != 0)
    return NULL;

  widget =  ctk_accessible_get_widget (CTK_ACCESSIBLE (selection));
  if (widget == NULL)
    return NULL;

  menu = ctk_menu_item_get_submenu (CTK_MENU_ITEM (widget));
  if (menu == NULL)
    return NULL;

  shell = CTK_MENU_SHELL (menu);

  item = ctk_menu_shell_get_selected_item (shell);
  if (item != NULL)
    {
      obj = ctk_widget_get_accessible (item);
      g_object_ref (obj);
      return obj;
    }

  return NULL;
}

static gint
ctk_menu_item_accessible_get_selection_count (AtkSelection *selection)
{
  GtkMenuShell *shell;
  GtkWidget *widget;
  GtkWidget *menu;

  widget =  ctk_accessible_get_widget (CTK_ACCESSIBLE (selection));
  if (widget == NULL)
    return 0;

  menu = ctk_menu_item_get_submenu (CTK_MENU_ITEM (widget));
  if (menu == NULL)
    return 0;

  shell = CTK_MENU_SHELL (menu);

  if (ctk_menu_shell_get_selected_item (shell) != NULL)
    return 1;

  return 0;
}

static gboolean
ctk_menu_item_accessible_is_child_selected (AtkSelection *selection,
                                               gint          i)
{
  GtkMenuShell *shell;
  gint j;
  GtkWidget *widget;
  GtkWidget *menu;
  GtkWidget *item;
  GList *kids;

  widget =  ctk_accessible_get_widget (CTK_ACCESSIBLE (selection));
  if (widget == NULL)
    return FALSE;

  menu = ctk_menu_item_get_submenu (CTK_MENU_ITEM (widget));
  if (menu == NULL)
    return FALSE;

  shell = CTK_MENU_SHELL (menu);

  item = ctk_menu_shell_get_selected_item (shell);
  if (item == NULL)
    return FALSE;

  kids = ctk_container_get_children (CTK_CONTAINER (shell));
  j = g_list_index (kids, item);
  g_list_free (kids);

  return j==i;
}

static gboolean
ctk_menu_item_accessible_remove_selection (AtkSelection *selection,
                                              gint          i)
{
  GtkMenuShell *shell;
  GtkWidget *widget;
  GtkWidget *menu;
  GtkWidget *item;

  if (i != 0)
    return FALSE;

  widget =  ctk_accessible_get_widget (CTK_ACCESSIBLE (selection));
  if (widget == NULL)
    return FALSE;

  menu = ctk_menu_item_get_submenu (CTK_MENU_ITEM (widget));
  if (menu == NULL)
    return FALSE;

  shell = CTK_MENU_SHELL (menu);

  item = ctk_menu_shell_get_selected_item (shell);
  if (item && ctk_menu_item_get_submenu (CTK_MENU_ITEM (item)))
    ctk_menu_shell_deselect (shell);

  return TRUE;
}

static void
atk_selection_interface_init (AtkSelectionIface *iface)
{
  iface->add_selection = ctk_menu_item_accessible_add_selection;
  iface->clear_selection = ctk_menu_item_accessible_clear_selection;
  iface->ref_selection = ctk_menu_item_accessible_ref_selection;
  iface->get_selection_count = ctk_menu_item_accessible_get_selection_count;
  iface->is_child_selected = ctk_menu_item_accessible_is_child_selected;
  iface->remove_selection = ctk_menu_item_accessible_remove_selection;
}

static gint
menu_item_insert_gtk (GtkMenuShell *shell,
                      GtkWidget    *widget,
                      gint          position)
{
  GtkWidget *parent_widget;

  g_return_val_if_fail (CTK_IS_MENU (shell), 1);

  parent_widget = ctk_menu_get_attach_widget (CTK_MENU (shell));
  if (CTK_IS_MENU_ITEM (parent_widget))
    CTK_CONTAINER_ACCESSIBLE_CLASS (ctk_menu_item_accessible_parent_class)->add_gtk (CTK_CONTAINER (shell), widget, ctk_widget_get_accessible (parent_widget));

  return 1;
}

static gint
menu_item_remove_gtk (GtkContainer *container,
                      GtkWidget    *widget)
{
  GtkWidget *parent_widget;

  g_return_val_if_fail (CTK_IS_MENU (container), 1);

  parent_widget = ctk_menu_get_attach_widget (CTK_MENU (container));
  if (CTK_IS_MENU_ITEM (parent_widget))
    {
      CTK_CONTAINER_ACCESSIBLE_CLASS (ctk_menu_item_accessible_parent_class)->remove_gtk (container, widget, ctk_widget_get_accessible (parent_widget));
    }
  return 1;
}

static void
menu_item_select (GtkMenuItem *item)
{
  menu_item_selection (item, TRUE);
}

static void
menu_item_deselect (GtkMenuItem *item)
{
  menu_item_selection (item, FALSE);
}
