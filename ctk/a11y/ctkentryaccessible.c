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
#include <ctk/ctk.h>
#include "ctkpango.h"
#include "ctkentryaccessible.h"
#include "ctkentryprivate.h"
#include "ctkcomboboxaccessible.h"
#include "ctkstylecontextprivate.h"
#include "ctkwidgetprivate.h"

#define CTK_TYPE_ENTRY_ICON_ACCESSIBLE      (ctk_entry_icon_accessible_get_type ())
#define CTK_ENTRY_ICON_ACCESSIBLE(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_ENTRY_ICON_ACCESSIBLE, CtkEntryIconAccessible))
#define CTK_IS_ENTRY_ICON_ACCESSIBLE(obj)   (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_ENTRY_ICON_ACCESSIBLE))

struct _CtkEntryAccessiblePrivate
{
  gint cursor_position;
  gint selection_bound;
  AtkObject *icons[2];
};

typedef struct _CtkEntryIconAccessible CtkEntryIconAccessible;
typedef struct _CtkEntryIconAccessibleClass CtkEntryIconAccessibleClass;

struct _CtkEntryIconAccessible
{
  AtkObject parent;

  CtkEntryAccessible *entry;
  CtkEntryIconPosition pos;
};

struct _CtkEntryIconAccessibleClass
{
  AtkObjectClass parent_class;
};

static void atk_action_interface_init (AtkActionIface *iface);

static void icon_atk_action_interface_init (AtkActionIface *iface);
static void icon_atk_component_interface_init (AtkComponentIface *iface);

GType ctk_entry_icon_accessible_get_type (void);

G_DEFINE_TYPE_WITH_CODE (CtkEntryIconAccessible, ctk_entry_icon_accessible, ATK_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_ACTION, icon_atk_action_interface_init)
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_COMPONENT, icon_atk_component_interface_init))

static void
ctk_entry_icon_accessible_remove_entry (gpointer data, GObject *obj)
{
  CtkEntryIconAccessible *icon = data;

  if (icon->entry)
    {
      icon->entry = NULL;
      g_object_notify (G_OBJECT (icon), "accessible-parent");
      atk_object_notify_state_change (ATK_OBJECT (icon), ATK_STATE_DEFUNCT, TRUE);
    }
}

static AtkObject *
ctk_entry_icon_accessible_new (CtkEntryAccessible *entry,
                               CtkEntryIconPosition pos)
{
  CtkEntryIconAccessible *icon;
  AtkObject *accessible;

  icon = g_object_new (ctk_entry_icon_accessible_get_type (), NULL);
  icon->entry = entry;
  g_object_weak_ref (G_OBJECT (entry),
                     ctk_entry_icon_accessible_remove_entry,
                     icon);
  icon->pos = pos;

  accessible = ATK_OBJECT (icon);
  atk_object_initialize (accessible, NULL);
  return accessible;
}

static void
ctk_entry_icon_accessible_init (CtkEntryIconAccessible *icon)
{
}

static void
ctk_entry_icon_accessible_initialize (AtkObject *obj,
                                      gpointer   data)
{
  CtkEntryIconAccessible *icon = CTK_ENTRY_ICON_ACCESSIBLE (obj);
  CtkWidget *widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (icon->entry));
  CtkEntry *ctk_entry = CTK_ENTRY (widget);
  const gchar *name;
  gchar *text;

  ATK_OBJECT_CLASS (ctk_entry_icon_accessible_parent_class)->initialize (obj, data);
  atk_object_set_role (obj, ATK_ROLE_ICON);

  name = ctk_entry_get_icon_name (ctk_entry, icon->pos);
  if (name)
    atk_object_set_name (obj, name);

  text = ctk_entry_get_icon_tooltip_text (ctk_entry, icon->pos);
  if (text)
    {
      atk_object_set_description (obj, text);
      g_free (text);
    }

  atk_object_set_parent (obj, ATK_OBJECT (icon->entry));
}

static AtkObject *
ctk_entry_icon_accessible_get_parent (AtkObject *accessible)
{
  CtkEntryIconAccessible *icon = CTK_ENTRY_ICON_ACCESSIBLE (accessible);

  return ATK_OBJECT (icon->entry);
}

static AtkStateSet *
ctk_entry_icon_accessible_ref_state_set (AtkObject *accessible)
{
  CtkEntryIconAccessible *icon = CTK_ENTRY_ICON_ACCESSIBLE (accessible);
  AtkStateSet *set = atk_state_set_new ();
  AtkStateSet *entry_set;
  CtkWidget *widget;
  CtkEntry *ctk_entry;

  if (!icon->entry)
    {
      atk_state_set_add_state (set, ATK_STATE_DEFUNCT);
      return set;
    }

  entry_set = atk_object_ref_state_set (ATK_OBJECT (icon->entry));
  if (!entry_set || atk_state_set_contains_state (entry_set, ATK_STATE_DEFUNCT))
    {
      atk_state_set_add_state (set, ATK_STATE_DEFUNCT);
    g_clear_object (&entry_set);
      return set;
    }

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (icon->entry));
  ctk_entry = CTK_ENTRY (widget);

  if (atk_state_set_contains_state (entry_set, ATK_STATE_ENABLED))
    atk_state_set_add_state (set, ATK_STATE_ENABLED);
  if (atk_state_set_contains_state (entry_set, ATK_STATE_SENSITIVE))
    atk_state_set_add_state (set, ATK_STATE_SENSITIVE);
  if (atk_state_set_contains_state (entry_set, ATK_STATE_SHOWING))
    atk_state_set_add_state (set, ATK_STATE_SHOWING);
  if (atk_state_set_contains_state (entry_set, ATK_STATE_VISIBLE))
    atk_state_set_add_state (set, ATK_STATE_VISIBLE);

  if (!ctk_entry_get_icon_sensitive (ctk_entry, icon->pos))
      atk_state_set_remove_state (set, ATK_STATE_SENSITIVE);
  if (!ctk_entry_get_icon_activatable (ctk_entry, icon->pos))
      atk_state_set_remove_state (set, ATK_STATE_ENABLED);

  g_object_unref (entry_set);
  return set;
}

static void
ctk_entry_icon_accessible_invalidate (CtkEntryIconAccessible *icon)
{
  if (!icon->entry)
    return;
  g_object_weak_unref (G_OBJECT (icon->entry),
                       ctk_entry_icon_accessible_remove_entry,
                       icon);
  ctk_entry_icon_accessible_remove_entry (icon, NULL);
}

static void
ctk_entry_icon_accessible_finalize (GObject *object)
{
  CtkEntryIconAccessible *icon = CTK_ENTRY_ICON_ACCESSIBLE (object);

  ctk_entry_icon_accessible_invalidate (icon);

  G_OBJECT_CLASS (ctk_entry_icon_accessible_parent_class)->finalize (object);
}

static void
ctk_entry_icon_accessible_class_init (CtkEntryIconAccessibleClass *klass)
{
  AtkObjectClass  *atk_class = ATK_OBJECT_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  atk_class->initialize = ctk_entry_icon_accessible_initialize;
  atk_class->get_parent = ctk_entry_icon_accessible_get_parent;
  atk_class->ref_state_set = ctk_entry_icon_accessible_ref_state_set;

  gobject_class->finalize = ctk_entry_icon_accessible_finalize;
}

static gboolean
ctk_entry_icon_accessible_do_action (AtkAction *action,
                                     gint       i)
{
  CtkEntryIconAccessible *icon = (CtkEntryIconAccessible *)action;
  CtkWidget *widget;
  CtkEntry *ctk_entry;
  GdkEvent event;
  GdkRectangle icon_area;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (icon->entry));
  if (widget == NULL)
    return FALSE;

  if (i != 0)
    return FALSE;

  if (!ctk_widget_is_sensitive (widget) || !ctk_widget_get_visible (widget))
    return FALSE;

  ctk_entry = CTK_ENTRY (widget);

  if (!ctk_entry_get_icon_sensitive (ctk_entry, icon->pos) ||
      !ctk_entry_get_icon_activatable (ctk_entry, icon->pos))
    return FALSE;

  ctk_entry_get_icon_area (ctk_entry, icon->pos, &icon_area);
  memset (&event, 0, sizeof (event));
  event.button.type = GDK_BUTTON_PRESS;
  event.button.window = ctk_widget_get_window (widget);
  event.button.button = 1;
  event.button.send_event = TRUE;
  event.button.time = GDK_CURRENT_TIME;
  event.button.x = icon_area.x;
  event.button.y = icon_area.y;

  g_signal_emit_by_name (widget, "icon-press", 0, icon->pos, &event);
  return TRUE;
}

static gint
ctk_entry_icon_accessible_get_n_actions (AtkAction *action)
{
  CtkEntryIconAccessible *icon = CTK_ENTRY_ICON_ACCESSIBLE (action);
  CtkWidget *widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (icon->entry));
  CtkEntry *ctk_entry = CTK_ENTRY (widget);

  return (ctk_entry_get_icon_activatable (ctk_entry, icon->pos) ? 1 : 0);
}

static const gchar *
ctk_entry_icon_accessible_get_name (AtkAction *action,
                                    gint       i)
{
  CtkEntryIconAccessible *icon = CTK_ENTRY_ICON_ACCESSIBLE (action);
  CtkWidget *widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (icon->entry));
  CtkEntry *ctk_entry = CTK_ENTRY (widget);

  if (i != 0)
    return NULL;
  if (!ctk_entry_get_icon_activatable (ctk_entry, icon->pos))
    return NULL;

  return "activate";
}

static void
icon_atk_action_interface_init (AtkActionIface *iface)
{
  iface->do_action = ctk_entry_icon_accessible_do_action;
  iface->get_n_actions = ctk_entry_icon_accessible_get_n_actions;
  iface->get_name = ctk_entry_icon_accessible_get_name;
}

static void
ctk_entry_icon_accessible_get_extents (AtkComponent   *component,
                                       gint           *x,
                                       gint           *y,
                                       gint           *width,
                                       gint           *height,
                                       AtkCoordType    coord_type)
{
  CtkEntryIconAccessible *icon = CTK_ENTRY_ICON_ACCESSIBLE (component);
  GdkRectangle icon_area;
  CtkEntry *ctk_entry;
  CtkWidget *widget;

  *x = G_MININT;
  atk_component_get_extents (ATK_COMPONENT (icon->entry), x, y, width, height,
                             coord_type);
  if (*x == G_MININT)
    return;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (icon->entry));
  ctk_entry = CTK_ENTRY (widget);
  ctk_entry_get_icon_area (ctk_entry, icon->pos, &icon_area);
  *width = icon_area.width;
  *height = icon_area.height;
  *x += icon_area.x;
  *y += icon_area.y;
}

static void
ctk_entry_icon_accessible_get_position (AtkComponent   *component,
                                        gint           *x,
                                        gint           *y,
                                        AtkCoordType    coord_type)
{
  CtkEntryIconAccessible *icon = CTK_ENTRY_ICON_ACCESSIBLE (component);
  GdkRectangle icon_area;
  CtkEntry *ctk_entry;
  CtkWidget *widget;

  *x = G_MININT;
  atk_component_get_extents (ATK_COMPONENT (icon->entry), x, y, NULL, NULL,
                             coord_type);
  if (*x == G_MININT)
    return;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (icon->entry));
  ctk_entry = CTK_ENTRY (widget);
  ctk_entry_get_icon_area (ctk_entry, icon->pos, &icon_area);
  *x += icon_area.x;
  *y += icon_area.y;
}

static void
ctk_entry_icon_accessible_get_size (AtkComponent *component,
                                gint         *width,
                                gint         *height)
{
  CtkEntryIconAccessible *icon = CTK_ENTRY_ICON_ACCESSIBLE (component);
  GdkRectangle icon_area;
  CtkEntry *ctk_entry;
  CtkWidget *widget;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (icon->entry));
  ctk_entry = CTK_ENTRY (widget);
  ctk_entry_get_icon_area (ctk_entry, icon->pos, &icon_area);
  *width = icon_area.width;
  *height = icon_area.height;
}

static void
icon_atk_component_interface_init (AtkComponentIface *iface)
{
  iface->get_extents = ctk_entry_icon_accessible_get_extents;
  iface->get_size = ctk_entry_icon_accessible_get_size;
  iface->get_position = ctk_entry_icon_accessible_get_position;
}

/* Callbacks */

static void     insert_text_cb             (CtkEditable        *editable,
                                            gchar              *new_text,
                                            gint                new_text_length,
                                            gint               *position);
static void     delete_text_cb             (CtkEditable        *editable,
                                            gint                start,
                                            gint                end);

static gboolean check_for_selection_change (CtkEntryAccessible *entry,
                                            CtkEntry           *ctk_entry);


static void atk_editable_text_interface_init (AtkEditableTextIface *iface);
static void atk_text_interface_init          (AtkTextIface         *iface);
static void atk_action_interface_init        (AtkActionIface       *iface);


G_DEFINE_TYPE_WITH_CODE (CtkEntryAccessible, ctk_entry_accessible, CTK_TYPE_WIDGET_ACCESSIBLE,
                         G_ADD_PRIVATE (CtkEntryAccessible)
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_EDITABLE_TEXT, atk_editable_text_interface_init)
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_TEXT, atk_text_interface_init)
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_ACTION, atk_action_interface_init))


static AtkStateSet *
ctk_entry_accessible_ref_state_set (AtkObject *accessible)
{
  AtkStateSet *state_set;
  gboolean value;
  CtkWidget *widget;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (accessible));
  if (widget == NULL)
    return NULL;

  state_set = ATK_OBJECT_CLASS (ctk_entry_accessible_parent_class)->ref_state_set (accessible);

  g_object_get (G_OBJECT (widget), "editable", &value, NULL);
  if (value)
    atk_state_set_add_state (state_set, ATK_STATE_EDITABLE);
  atk_state_set_add_state (state_set, ATK_STATE_SINGLE_LINE);

  return state_set;
}

static AtkAttributeSet *
ctk_entry_accessible_get_attributes (AtkObject *accessible)
{
  CtkWidget *widget;
  AtkAttributeSet *attributes;
  AtkAttribute *placeholder_text;
  const gchar *text;

  attributes = ATK_OBJECT_CLASS (ctk_entry_accessible_parent_class)->get_attributes (accessible);

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (accessible));
  if (widget == NULL)
    return attributes;

  text = ctk_entry_get_placeholder_text (CTK_ENTRY (widget));
  if (text == NULL)
    return attributes;

  placeholder_text = g_malloc (sizeof (AtkAttribute));
  placeholder_text->name = g_strdup ("placeholder-text");
  placeholder_text->value = g_strdup (text);

  attributes = g_slist_append (attributes, placeholder_text);

  return attributes;
}

static void
ctk_entry_accessible_initialize (AtkObject *obj,
                                 gpointer   data)
{
  CtkEntry *entry;
  CtkEntryAccessible *ctk_entry_accessible;
  gint start_pos, end_pos;

  ATK_OBJECT_CLASS (ctk_entry_accessible_parent_class)->initialize (obj, data);

  ctk_entry_accessible = CTK_ENTRY_ACCESSIBLE (obj);

  entry = CTK_ENTRY (data);
  ctk_editable_get_selection_bounds (CTK_EDITABLE (entry), &start_pos, &end_pos);
  ctk_entry_accessible->priv->cursor_position = end_pos;
  ctk_entry_accessible->priv->selection_bound = start_pos;

  /* Set up signal callbacks */
  g_signal_connect_after (entry, "insert-text", G_CALLBACK (insert_text_cb), NULL);
  g_signal_connect (entry, "delete-text", G_CALLBACK (delete_text_cb), NULL);

  if (ctk_entry_get_visibility (entry))
    obj->role = ATK_ROLE_TEXT;
  else
    obj->role = ATK_ROLE_PASSWORD_TEXT;
}

static void
ctk_entry_accessible_notify_ctk (GObject    *obj,
                                 GParamSpec *pspec)
{
  CtkWidget *widget;
  AtkObject* atk_obj;
  CtkEntry* ctk_entry;
  CtkEntryAccessible* entry;
  CtkEntryAccessiblePrivate *priv;

  widget = CTK_WIDGET (obj);
  atk_obj = ctk_widget_get_accessible (widget);
  ctk_entry = CTK_ENTRY (widget);
  entry = CTK_ENTRY_ACCESSIBLE (atk_obj);
  priv = entry->priv;

  if (g_strcmp0 (pspec->name, "cursor-position") == 0)
    {
      if (check_for_selection_change (entry, ctk_entry))
        g_signal_emit_by_name (atk_obj, "text-selection-changed");
      /*
       * The entry cursor position has moved so generate the signal.
       */
      g_signal_emit_by_name (atk_obj, "text-caret-moved",
                             entry->priv->cursor_position);
    }
  else if (g_strcmp0 (pspec->name, "selection-bound") == 0)
    {
      if (check_for_selection_change (entry, ctk_entry))
        g_signal_emit_by_name (atk_obj, "text-selection-changed");
    }
  else if (g_strcmp0 (pspec->name, "editable") == 0)
    {
      gboolean value;

      g_object_get (obj, "editable", &value, NULL);
      atk_object_notify_state_change (atk_obj, ATK_STATE_EDITABLE, value);
    }
  else if (g_strcmp0 (pspec->name, "visibility") == 0)
    {
      gboolean visibility;
      AtkRole new_role;

      visibility = ctk_entry_get_visibility (ctk_entry);
      new_role = visibility ? ATK_ROLE_TEXT : ATK_ROLE_PASSWORD_TEXT;
      atk_object_set_role (atk_obj, new_role);
    }
  else if (g_strcmp0 (pspec->name, "primary-icon-storage-type") == 0)
    {
      if (ctk_entry_get_icon_storage_type (ctk_entry, CTK_ENTRY_ICON_PRIMARY) != CTK_IMAGE_EMPTY && !priv->icons[CTK_ENTRY_ICON_PRIMARY])
        {
          priv->icons[CTK_ENTRY_ICON_PRIMARY] = ctk_entry_icon_accessible_new (entry, CTK_ENTRY_ICON_PRIMARY);
          g_signal_emit_by_name (entry, "children-changed::add", 0,
                                 priv->icons[CTK_ENTRY_ICON_PRIMARY], NULL);
        }
      else if (ctk_entry_get_icon_storage_type (ctk_entry, CTK_ENTRY_ICON_PRIMARY) == CTK_IMAGE_EMPTY && priv->icons[CTK_ENTRY_ICON_PRIMARY])
        {
          ctk_entry_icon_accessible_invalidate (CTK_ENTRY_ICON_ACCESSIBLE (priv->icons[CTK_ENTRY_ICON_PRIMARY]));
          g_signal_emit_by_name (entry, "children-changed::remove", 0,
                                 priv->icons[CTK_ENTRY_ICON_PRIMARY], NULL);
          g_clear_object (&priv->icons[CTK_ENTRY_ICON_PRIMARY]);
        }
    }
  else if (g_strcmp0 (pspec->name, "secondary-icon-storage-type") == 0)
    {
      gint index = (priv->icons[CTK_ENTRY_ICON_PRIMARY] ? 1 : 0);
      if (ctk_entry_get_icon_storage_type (ctk_entry, CTK_ENTRY_ICON_SECONDARY) != CTK_IMAGE_EMPTY && !priv->icons[CTK_ENTRY_ICON_SECONDARY])
        {
          priv->icons[CTK_ENTRY_ICON_SECONDARY] = ctk_entry_icon_accessible_new (entry, CTK_ENTRY_ICON_SECONDARY);
          g_signal_emit_by_name (entry, "children-changed::add", index,
                                 priv->icons[CTK_ENTRY_ICON_SECONDARY], NULL);
        }
      else if (ctk_entry_get_icon_storage_type (ctk_entry, CTK_ENTRY_ICON_SECONDARY) == CTK_IMAGE_EMPTY && priv->icons[CTK_ENTRY_ICON_SECONDARY])
        {
          ctk_entry_icon_accessible_invalidate (CTK_ENTRY_ICON_ACCESSIBLE (priv->icons[CTK_ENTRY_ICON_SECONDARY]));
          g_signal_emit_by_name (entry, "children-changed::remove", index,
                                 priv->icons[CTK_ENTRY_ICON_SECONDARY], NULL);
          g_clear_object (&priv->icons[CTK_ENTRY_ICON_SECONDARY]);
        }
    }
  else if (g_strcmp0 (pspec->name, "primary-icon-name") == 0)
    {
      if (priv->icons[CTK_ENTRY_ICON_PRIMARY])
        {
          const gchar *name;
          name = ctk_entry_get_icon_name (ctk_entry,
                                          CTK_ENTRY_ICON_PRIMARY);
          if (name)
            atk_object_set_name (priv->icons[CTK_ENTRY_ICON_PRIMARY], name);
        }
    }
  else if (g_strcmp0 (pspec->name, "secondary-icon-name") == 0)
    {
      if (priv->icons[CTK_ENTRY_ICON_SECONDARY])
        {
          const gchar *name;
          name = ctk_entry_get_icon_name (ctk_entry,
                                          CTK_ENTRY_ICON_SECONDARY);
          if (name)
            atk_object_set_name (priv->icons[CTK_ENTRY_ICON_SECONDARY], name);
        }
    }
  else if (g_strcmp0 (pspec->name, "primary-icon-tooltip-text") == 0)
    {
      if (priv->icons[CTK_ENTRY_ICON_PRIMARY])
        {
          gchar *text;
          text = ctk_entry_get_icon_tooltip_text (ctk_entry,
                                                    CTK_ENTRY_ICON_PRIMARY);
          if (text)
            {
              atk_object_set_description (priv->icons[CTK_ENTRY_ICON_PRIMARY],
                                      text);
              g_free (text);
            }
          else
            {
              atk_object_set_description (priv->icons[CTK_ENTRY_ICON_PRIMARY],
                                      "");
            }
        }
    }
  else if (g_strcmp0 (pspec->name, "secondary-icon-tooltip-text") == 0)
    {
      if (priv->icons[CTK_ENTRY_ICON_SECONDARY])
        {
          gchar *text;
          text = ctk_entry_get_icon_tooltip_text (ctk_entry,
                                                    CTK_ENTRY_ICON_SECONDARY);
          if (text)
            {
              atk_object_set_description (priv->icons[CTK_ENTRY_ICON_SECONDARY],
                                      text);
              g_free (text);
            }
          else
            {
              atk_object_set_description (priv->icons[CTK_ENTRY_ICON_SECONDARY],
                                      "");
            }
        }
    }
  else if (g_strcmp0 (pspec->name, "primary-icon-activatable") == 0)
    {
      if (priv->icons[CTK_ENTRY_ICON_PRIMARY])
        {
          gboolean on = ctk_entry_get_icon_activatable (ctk_entry, CTK_ENTRY_ICON_PRIMARY);
          atk_object_notify_state_change (priv->icons[CTK_ENTRY_ICON_PRIMARY],
                                          ATK_STATE_ENABLED, on);
        }
    }
  else if (g_strcmp0 (pspec->name, "secondary-icon-activatable") == 0)
    {
      if (priv->icons[CTK_ENTRY_ICON_SECONDARY])
        {
          gboolean on = ctk_entry_get_icon_activatable (ctk_entry, CTK_ENTRY_ICON_SECONDARY);
          atk_object_notify_state_change (priv->icons[CTK_ENTRY_ICON_SECONDARY],
                                          ATK_STATE_ENABLED, on);
        }
    }
  else if (g_strcmp0 (pspec->name, "primary-icon-sensitive") == 0)
    {
      if (priv->icons[CTK_ENTRY_ICON_PRIMARY])
        {
          gboolean on = ctk_entry_get_icon_sensitive (ctk_entry, CTK_ENTRY_ICON_PRIMARY);
          atk_object_notify_state_change (priv->icons[CTK_ENTRY_ICON_PRIMARY],
                                          ATK_STATE_SENSITIVE, on);
        }
    }
  else if (g_strcmp0 (pspec->name, "secondary-icon-sensitive") == 0)
    {
      if (priv->icons[CTK_ENTRY_ICON_SECONDARY])
        {
          gboolean on = ctk_entry_get_icon_sensitive (ctk_entry, CTK_ENTRY_ICON_SECONDARY);
          atk_object_notify_state_change (priv->icons[CTK_ENTRY_ICON_SECONDARY],
                                          ATK_STATE_SENSITIVE, on);
        }
    }
  else
    CTK_WIDGET_ACCESSIBLE_CLASS (ctk_entry_accessible_parent_class)->notify_ctk (obj, pspec);
}

static gint
ctk_entry_accessible_get_index_in_parent (AtkObject *accessible)
{
  /*
   * If the parent widget is a combo box then the index is 1
   * otherwise do the normal thing.
   */
  if (accessible->accessible_parent)
    if (CTK_IS_COMBO_BOX_ACCESSIBLE (accessible->accessible_parent))
      return 1;

  return ATK_OBJECT_CLASS (ctk_entry_accessible_parent_class)->get_index_in_parent (accessible);
}

static gint
ctk_entry_accessible_get_n_children (AtkObject* obj)
{
  CtkWidget *widget;
  CtkEntry *entry;
  gint count = 0;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  if (widget == NULL)
    return 0;

  entry = CTK_ENTRY (widget);

  if (ctk_entry_get_icon_storage_type (entry, CTK_ENTRY_ICON_PRIMARY) != CTK_IMAGE_EMPTY)
    count++;
  if (ctk_entry_get_icon_storage_type (entry, CTK_ENTRY_ICON_SECONDARY) != CTK_IMAGE_EMPTY)
    count++;
  return count;
}

static AtkObject *
ctk_entry_accessible_ref_child (AtkObject *obj,
                                gint i)
{
  CtkEntryAccessible *accessible = CTK_ENTRY_ACCESSIBLE (obj);
  CtkEntryAccessiblePrivate *priv = accessible->priv;
  CtkWidget *widget;
  CtkEntry *entry;
  CtkEntryIconPosition pos;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  if (widget == NULL)
    return NULL;

  entry = CTK_ENTRY (widget);

  switch (i)
    {
    case 0:
      if (ctk_entry_get_icon_storage_type (entry, CTK_ENTRY_ICON_PRIMARY) != CTK_IMAGE_EMPTY)
        pos = CTK_ENTRY_ICON_PRIMARY;
      else if (ctk_entry_get_icon_storage_type (entry, CTK_ENTRY_ICON_SECONDARY) != CTK_IMAGE_EMPTY)
        pos = CTK_ENTRY_ICON_SECONDARY;
      else
        return NULL;
      break;
    case 1:
      if (ctk_entry_get_icon_storage_type (entry, CTK_ENTRY_ICON_PRIMARY) == CTK_IMAGE_EMPTY)
        return NULL;
      if (ctk_entry_get_icon_storage_type (entry, CTK_ENTRY_ICON_SECONDARY) == CTK_IMAGE_EMPTY)
        return NULL;
      pos = CTK_ENTRY_ICON_SECONDARY;
      break;
    default:
      return NULL;
    }

  if (!priv->icons[pos])
    priv->icons[pos] = ctk_entry_icon_accessible_new (accessible, pos);
  return g_object_ref (priv->icons[pos]);
}

static void
ctk_entry_accessible_finalize (GObject *object)
{
  CtkEntryAccessible *entry = CTK_ENTRY_ACCESSIBLE (object);
  CtkEntryAccessiblePrivate *priv = entry->priv;

  g_clear_object (&priv->icons[CTK_ENTRY_ICON_PRIMARY]);
  g_clear_object (&priv->icons[CTK_ENTRY_ICON_SECONDARY]);

  G_OBJECT_CLASS (ctk_entry_accessible_parent_class)->finalize (object);
}

static void
ctk_entry_accessible_class_init (CtkEntryAccessibleClass *klass)
{
  AtkObjectClass  *class = ATK_OBJECT_CLASS (klass);
  CtkWidgetAccessibleClass *widget_class = (CtkWidgetAccessibleClass*)klass;
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  class->ref_state_set = ctk_entry_accessible_ref_state_set;
  class->get_index_in_parent = ctk_entry_accessible_get_index_in_parent;
  class->initialize = ctk_entry_accessible_initialize;
  class->get_attributes = ctk_entry_accessible_get_attributes;
  class->get_n_children = ctk_entry_accessible_get_n_children;
  class->ref_child = ctk_entry_accessible_ref_child;

  widget_class->notify_ctk = ctk_entry_accessible_notify_ctk;

  gobject_class->finalize = ctk_entry_accessible_finalize;
}

static void
ctk_entry_accessible_init (CtkEntryAccessible *entry)
{
  entry->priv = ctk_entry_accessible_get_instance_private (entry);
  entry->priv->cursor_position = 0;
  entry->priv->selection_bound = 0;
}

static gchar *
ctk_entry_accessible_get_text (AtkText *atk_text,
                               gint     start_pos,
                               gint     end_pos)
{
  CtkWidget *widget;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (atk_text));
  if (widget == NULL)
    return NULL;

  return _ctk_entry_get_display_text (CTK_ENTRY (widget), start_pos, end_pos);
}

static gchar *
ctk_entry_accessible_get_text_before_offset (AtkText         *text,
                                             gint             offset,
                                             AtkTextBoundary  boundary_type,
                                             gint            *start_offset,
                                             gint            *end_offset)
{
  CtkWidget *widget;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return NULL;

  return _ctk_pango_get_text_before (ctk_entry_get_layout (CTK_ENTRY (widget)),
                                     boundary_type, offset,
                                     start_offset, end_offset);
}

static gchar *
ctk_entry_accessible_get_text_at_offset (AtkText         *text,
                                         gint             offset,
                                         AtkTextBoundary  boundary_type,
                                         gint            *start_offset,
                                         gint            *end_offset)
{
  CtkWidget *widget;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return NULL;

  return _ctk_pango_get_text_at (ctk_entry_get_layout (CTK_ENTRY (widget)),
                                 boundary_type, offset,
                                 start_offset, end_offset);
}

static gchar *
ctk_entry_accessible_get_text_after_offset (AtkText         *text,
                                            gint             offset,
                                            AtkTextBoundary  boundary_type,
                                            gint            *start_offset,
                                            gint            *end_offset)
{
  CtkWidget *widget;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return NULL;

  return _ctk_pango_get_text_after (ctk_entry_get_layout (CTK_ENTRY (widget)),
                                    boundary_type, offset,
                                    start_offset, end_offset);
}

static gint
ctk_entry_accessible_get_character_count (AtkText *atk_text)
{
  CtkWidget *widget;
  gchar *text;
  glong char_count;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (atk_text));
  if (widget == NULL)
    return 0;

  text = _ctk_entry_get_display_text (CTK_ENTRY (widget), 0, -1);

  char_count = 0;
  if (text)
    {
      char_count = g_utf8_strlen (text, -1);
      g_free (text);
    }

  return char_count;
}

static gint
ctk_entry_accessible_get_caret_offset (AtkText *text)
{
  CtkWidget *widget;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return 0;

  return ctk_editable_get_position (CTK_EDITABLE (widget));
}

static gboolean
ctk_entry_accessible_set_caret_offset (AtkText *text,
                                       gint     offset)
{
  CtkWidget *widget;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return FALSE;

  ctk_editable_set_position (CTK_EDITABLE (widget), offset);

  return TRUE;
}

static AtkAttributeSet *
add_text_attribute (AtkAttributeSet  *attributes,
                    AtkTextAttribute  attr,
                    gint              i)
{
  AtkAttribute *at;

  at = g_new (AtkAttribute, 1);
  at->name = g_strdup (atk_text_attribute_get_name (attr));
  at->value = g_strdup (atk_text_attribute_get_value (attr, i));

  return g_slist_prepend (attributes, at);
}

static AtkAttributeSet *
ctk_entry_accessible_get_run_attributes (AtkText *text,
                                         gint     offset,
                                         gint    *start_offset,
                                         gint    *end_offset)
{
  CtkWidget *widget;
  AtkAttributeSet *attributes;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return NULL;

  attributes = NULL;
  attributes = add_text_attribute (attributes, ATK_TEXT_ATTR_DIRECTION,
                                   ctk_widget_get_direction (widget));
  attributes = _ctk_pango_get_run_attributes (attributes,
                                              ctk_entry_get_layout (CTK_ENTRY (widget)),
                                              offset,
                                              start_offset,
                                              end_offset);

  return attributes;
}

static AtkAttributeSet *
ctk_entry_accessible_get_default_attributes (AtkText *text)
{
  CtkWidget *widget;
  AtkAttributeSet *attributes;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return NULL;

  attributes = NULL;
  attributes = add_text_attribute (attributes, ATK_TEXT_ATTR_DIRECTION,
                                   ctk_widget_get_direction (widget));
  attributes = _ctk_pango_get_default_attributes (attributes,
                                                  ctk_entry_get_layout (CTK_ENTRY (widget)));
  attributes = _ctk_style_context_get_attributes (attributes,
                                                  ctk_widget_get_style_context (widget),
                                                  ctk_widget_get_state_flags (widget));

  return attributes;
}

static void
ctk_entry_accessible_get_character_extents (AtkText      *text,
                                            gint          offset,
                                            gint         *x,
                                            gint         *y,
                                            gint         *width,
                                            gint         *height,
                                            AtkCoordType  coords)
{
  CtkWidget *widget;
  CtkEntry *entry;
  PangoRectangle char_rect;
  gchar *entry_text;
  gint index, x_layout, y_layout;
  GdkWindow *window;
  gint x_window, y_window;
  CtkAllocation allocation;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return;

  entry = CTK_ENTRY (widget);

  ctk_entry_get_layout_offsets (entry, &x_layout, &y_layout);
  entry_text = _ctk_entry_get_display_text (entry, 0, -1);
  index = g_utf8_offset_to_pointer (entry_text, offset) - entry_text;
  g_free (entry_text);

  pango_layout_index_to_pos (ctk_entry_get_layout (entry), index, &char_rect);
  pango_extents_to_pixels (&char_rect, NULL);

  _ctk_widget_get_allocation (widget, &allocation);

  window = ctk_widget_get_window (widget);
  gdk_window_get_origin (window, &x_window, &y_window);

  *x = x_window + allocation.x + x_layout + char_rect.x;
  *y = y_window + allocation.y + y_layout + char_rect.y;
  *width = char_rect.width;
  *height = char_rect.height;

  if (coords == ATK_XY_WINDOW)
    {
      window = gdk_window_get_toplevel (window);
      gdk_window_get_origin (window, &x_window, &y_window);

      *x -= x_window;
      *y -= y_window;
    }
}

static gint
ctk_entry_accessible_get_offset_at_point (AtkText      *atk_text,
                                          gint          x,
                                          gint          y,
                                          AtkCoordType  coords)
{
  CtkWidget *widget;
  CtkEntry *entry;
  gchar *text;
  gint index, x_layout, y_layout;
  gint x_window, y_window;
  gint x_local, y_local;
  GdkWindow *window;
  glong offset;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (atk_text));
  if (widget == NULL)
    return -1;

  entry = CTK_ENTRY (widget);

  ctk_entry_get_layout_offsets (entry, &x_layout, &y_layout);

  window = ctk_widget_get_window (widget);
  gdk_window_get_origin (window, &x_window, &y_window);

  x_local = x - x_layout - x_window;
  y_local = y - y_layout - y_window;

  if (coords == ATK_XY_WINDOW)
    {
      window = gdk_window_get_toplevel (window);
      gdk_window_get_origin (window, &x_window, &y_window);

      x_local += x_window;
      y_local += y_window;
    }
  if (!pango_layout_xy_to_index (ctk_entry_get_layout (entry),
                                 x_local * PANGO_SCALE,
                                 y_local * PANGO_SCALE,
                                 &index, NULL))
    {
      if (x_local < 0 || y_local < 0)
        index = 0;
      else
        index = -1;
    }

  offset = -1;
  if (index != -1)
    {
      text = _ctk_entry_get_display_text (entry, 0, -1);
      offset = g_utf8_pointer_to_offset (text, text + index);
      g_free (text);
    }

  return offset;
}

static gint
ctk_entry_accessible_get_n_selections (AtkText *text)
{
  CtkWidget *widget;
  gint start, end;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return 0;

  if (ctk_editable_get_selection_bounds (CTK_EDITABLE (widget), &start, &end))
    return 1;

  return 0;
}

static gchar *
ctk_entry_accessible_get_selection (AtkText *text,
                                    gint     selection_num,
                                    gint    *start_pos,
                                    gint    *end_pos)
{
  CtkWidget *widget;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return NULL;

  if (selection_num != 0)
     return NULL;

  if (ctk_editable_get_selection_bounds (CTK_EDITABLE (widget), start_pos, end_pos))
    return ctk_editable_get_chars (CTK_EDITABLE (widget), *start_pos, *end_pos);

  return NULL;
}

static gboolean
ctk_entry_accessible_add_selection (AtkText *text,
                                    gint     start_pos,
                                    gint     end_pos)
{
  CtkEntry *entry;
  CtkWidget *widget;
  gint start, end;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return FALSE;

  entry = CTK_ENTRY (widget);

  if (!ctk_editable_get_selection_bounds (CTK_EDITABLE (entry), &start, &end))
    {
      ctk_editable_select_region (CTK_EDITABLE (entry), start_pos, end_pos);
      return TRUE;
    }
  else
    return FALSE;
}

static gboolean
ctk_entry_accessible_remove_selection (AtkText *text,
                                       gint     selection_num)
{
  CtkWidget *widget;
  gint start, end;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return FALSE;

  if (selection_num != 0)
     return FALSE;

  if (ctk_editable_get_selection_bounds (CTK_EDITABLE (widget), &start, &end))
    {
      ctk_editable_select_region (CTK_EDITABLE (widget), end, end);
      return TRUE;
    }
  else
    return FALSE;
}

static gboolean
ctk_entry_accessible_set_selection (AtkText *text,
                                    gint     selection_num,
                                    gint     start_pos,
                                    gint     end_pos)
{
  CtkWidget *widget;
  gint start, end;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return FALSE;

  if (selection_num != 0)
     return FALSE;

  if (ctk_editable_get_selection_bounds (CTK_EDITABLE (widget), &start, &end))
    {
      ctk_editable_select_region (CTK_EDITABLE (widget), start_pos, end_pos);
      return TRUE;
    }
  else
    return FALSE;
}

static gunichar
ctk_entry_accessible_get_character_at_offset (AtkText *atk_text,
                                              gint     offset)
{
  CtkWidget *widget;
  gchar *text;
  gchar *index;
  gunichar result;

  result = '\0';

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (atk_text));
  if (widget == NULL)
    return result;

  if (!ctk_entry_get_visibility (CTK_ENTRY (widget)))
    return result;

  text = _ctk_entry_get_display_text (CTK_ENTRY (widget), 0, -1);
  if (offset < g_utf8_strlen (text, -1))
    {
      index = g_utf8_offset_to_pointer (text, offset);
      result = g_utf8_get_char (index);
      g_free (text);
    }

  return result;
}

static void
atk_text_interface_init (AtkTextIface *iface)
{
  iface->get_text = ctk_entry_accessible_get_text;
  iface->get_character_at_offset = ctk_entry_accessible_get_character_at_offset;
  iface->get_text_before_offset = ctk_entry_accessible_get_text_before_offset;
  iface->get_text_at_offset = ctk_entry_accessible_get_text_at_offset;
  iface->get_text_after_offset = ctk_entry_accessible_get_text_after_offset;
  iface->get_caret_offset = ctk_entry_accessible_get_caret_offset;
  iface->set_caret_offset = ctk_entry_accessible_set_caret_offset;
  iface->get_character_count = ctk_entry_accessible_get_character_count;
  iface->get_n_selections = ctk_entry_accessible_get_n_selections;
  iface->get_selection = ctk_entry_accessible_get_selection;
  iface->add_selection = ctk_entry_accessible_add_selection;
  iface->remove_selection = ctk_entry_accessible_remove_selection;
  iface->set_selection = ctk_entry_accessible_set_selection;
  iface->get_run_attributes = ctk_entry_accessible_get_run_attributes;
  iface->get_default_attributes = ctk_entry_accessible_get_default_attributes;
  iface->get_character_extents = ctk_entry_accessible_get_character_extents;
  iface->get_offset_at_point = ctk_entry_accessible_get_offset_at_point;
}

static void
ctk_entry_accessible_set_text_contents (AtkEditableText *text,
                                        const gchar     *string)
{
  CtkWidget *widget;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return;

  if (!ctk_editable_get_editable (CTK_EDITABLE (widget)))
    return;

  ctk_entry_set_text (CTK_ENTRY (widget), string);
}

static void
ctk_entry_accessible_insert_text (AtkEditableText *text,
                                  const gchar     *string,
                                  gint             length,
                                  gint            *position)
{
  CtkWidget *widget;
  CtkEditable *editable;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return;

  editable = CTK_EDITABLE (widget);
  if (!ctk_editable_get_editable (editable))
    return;

  ctk_editable_insert_text (editable, string, length, position);
  ctk_editable_set_position (editable, *position);
}

static void
ctk_entry_accessible_copy_text (AtkEditableText *text,
                                gint             start_pos,
                                gint             end_pos)
{
  CtkWidget *widget;
  CtkEditable *editable;
  gchar *str;
  CtkClipboard *clipboard;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return;

  if (!ctk_widget_has_screen (widget))
    return;

  editable = CTK_EDITABLE (widget);
  str = ctk_editable_get_chars (editable, start_pos, end_pos);
  clipboard = ctk_widget_get_clipboard (widget, GDK_SELECTION_CLIPBOARD);
  ctk_clipboard_set_text (clipboard, str, -1);
  g_free (str);
}

static void
ctk_entry_accessible_cut_text (AtkEditableText *text,
                               gint             start_pos,
                               gint             end_pos)
{
  CtkWidget *widget;
  CtkEditable *editable;
  gchar *str;
  CtkClipboard *clipboard;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return;

  if (!ctk_widget_has_screen (widget))
    return;

  editable = CTK_EDITABLE (widget);
  if (!ctk_editable_get_editable (editable))
    return;

  str = ctk_editable_get_chars (editable, start_pos, end_pos);
  clipboard = ctk_widget_get_clipboard (widget, GDK_SELECTION_CLIPBOARD);
  ctk_clipboard_set_text (clipboard, str, -1);
  ctk_editable_delete_text (editable, start_pos, end_pos);
}

static void
ctk_entry_accessible_delete_text (AtkEditableText *text,
                                  gint             start_pos,
                                  gint             end_pos)
{
  CtkWidget *widget;
  CtkEditable *editable;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return;

  editable = CTK_EDITABLE (widget);
  if (!ctk_editable_get_editable (editable))
    return;

  ctk_editable_delete_text (editable, start_pos, end_pos);
}

typedef struct
{
  CtkEntry* entry;
  gint position;
} PasteData;

static void
paste_received_cb (CtkClipboard *clipboard,
                   const gchar  *text,
                   gpointer      data)
{
  PasteData *paste = data;

  if (text)
    ctk_editable_insert_text (CTK_EDITABLE (paste->entry), text, -1,
                              &paste->position);

  g_object_unref (paste->entry);
  g_free (paste);
}

static void
ctk_entry_accessible_paste_text (AtkEditableText *text,
                                 gint             position)
{
  CtkWidget *widget;
  CtkEditable *editable;
  PasteData *paste;
  CtkClipboard *clipboard;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return;

  if (!ctk_widget_has_screen (widget))
    return;

  editable = CTK_EDITABLE (widget);
  if (!ctk_editable_get_editable (editable))
    return;

  paste = g_new0 (PasteData, 1);
  paste->entry = CTK_ENTRY (widget);
  paste->position = position;

  g_object_ref (paste->entry);
  clipboard = ctk_widget_get_clipboard (widget, GDK_SELECTION_CLIPBOARD);
  ctk_clipboard_request_text (clipboard, paste_received_cb, paste);
}

static void
atk_editable_text_interface_init (AtkEditableTextIface *iface)
{
  iface->set_text_contents = ctk_entry_accessible_set_text_contents;
  iface->insert_text = ctk_entry_accessible_insert_text;
  iface->copy_text = ctk_entry_accessible_copy_text;
  iface->cut_text = ctk_entry_accessible_cut_text;
  iface->delete_text = ctk_entry_accessible_delete_text;
  iface->paste_text = ctk_entry_accessible_paste_text;
  iface->set_run_attributes = NULL;
}

static void
insert_text_cb (CtkEditable *editable,
                gchar       *new_text,
                gint         new_text_length,
                gint        *position)
{
  CtkEntryAccessible *accessible;
  gint length;

  if (new_text_length == 0)
    return;

  accessible = CTK_ENTRY_ACCESSIBLE (ctk_widget_get_accessible (CTK_WIDGET (editable)));
  length = g_utf8_strlen (new_text, new_text_length);

  g_signal_emit_by_name (accessible,
                         "text-changed::insert",
                         *position - length,
                          length);
}

/* We connect to CtkEditable::delete-text, since it carries
 * the information we need. But we delay emitting our own
 * text_changed::delete signal until the entry has update
 * all its internal state and emits CtkEntry::changed.
 */
static void
delete_text_cb (CtkEditable *editable,
                gint         start,
                gint         end)
{
  CtkEntryAccessible *accessible;

  accessible = CTK_ENTRY_ACCESSIBLE (ctk_widget_get_accessible (CTK_WIDGET (editable)));

  if (end < 0)
    {
      gchar *text;

      text = _ctk_entry_get_display_text (CTK_ENTRY (editable), 0, -1);
      end = g_utf8_strlen (text, -1);
      g_free (text);
    }

  if (end == start)
    return;

  g_signal_emit_by_name (accessible,
                         "text-changed::delete",
                         start,
                         end - start);
}

static gboolean
check_for_selection_change (CtkEntryAccessible *accessible,
                            CtkEntry           *entry)
{
  gboolean ret_val = FALSE;
  gint start, end;

  if (ctk_editable_get_selection_bounds (CTK_EDITABLE (entry), &start, &end))
    {
      if (end != accessible->priv->cursor_position ||
          start != accessible->priv->selection_bound)
        /*
         * This check is here as this function can be called
         * for notification of selection_bound and current_pos.
         * The values of current_pos and selection_bound may be the same
         * for both notifications and we only want to generate one
         * text_selection_changed signal.
         */
        ret_val = TRUE;
    }
  else
    {
      /* We had a selection */
      ret_val = (accessible->priv->cursor_position != accessible->priv->selection_bound);
    }

  accessible->priv->cursor_position = end;
  accessible->priv->selection_bound = start;

  return ret_val;
}

static gboolean
ctk_entry_accessible_do_action (AtkAction *action,
                                gint       i)
{
  CtkWidget *widget;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (action));
  if (widget == NULL)
    return FALSE;

  if (!ctk_widget_get_sensitive (widget) || !ctk_widget_get_visible (widget))
    return FALSE;

  if (i != 0)
    return FALSE;

  ctk_widget_activate (widget);

  return TRUE;
}

static gint
ctk_entry_accessible_get_n_actions (AtkAction *action)
{
  return 1;
}

static const gchar *
ctk_entry_accessible_get_keybinding (AtkAction *action,
                                     gint       i)
{
  CtkWidget *widget;
  CtkWidget *label;
  AtkRelationSet *set;
  AtkRelation *relation;
  GPtrArray *target;
  gpointer target_object;
  guint key_val;

  if (i != 0)
    return NULL;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (action));
  if (widget == NULL)
    return NULL;

  set = atk_object_ref_relation_set (ATK_OBJECT (action));
  if (!set)
    return NULL;

  label = NULL;
  relation = atk_relation_set_get_relation_by_type (set, ATK_RELATION_LABELLED_BY);
  if (relation)
    {
      target = atk_relation_get_target (relation);

      target_object = g_ptr_array_index (target, 0);
      label = ctk_accessible_get_widget (CTK_ACCESSIBLE (target_object));
    }

  g_object_unref (set);

  if (CTK_IS_LABEL (label))
    {
      key_val = ctk_label_get_mnemonic_keyval (CTK_LABEL (label));
      if (key_val != GDK_KEY_VoidSymbol)
        return ctk_accelerator_name (key_val, GDK_MOD1_MASK);
    }

  return NULL;
}

static const gchar*
ctk_entry_accessible_action_get_name (AtkAction *action,
                                      gint       i)
{
  if (i == 0)
    return "activate";
  return NULL;
}

static const gchar*
ctk_entry_accessible_action_get_localized_name (AtkAction *action,
                                                gint       i)
{
  if (i == 0)
    return C_("Action name", "Activate");
  return NULL;
}

static const gchar*
ctk_entry_accessible_action_get_description (AtkAction *action,
                                             gint       i)
{
  if (i == 0)
    return C_("Action description", "Activates the entry");
  return NULL;
}

static void
atk_action_interface_init (AtkActionIface *iface)
{
  iface->do_action = ctk_entry_accessible_do_action;
  iface->get_n_actions = ctk_entry_accessible_get_n_actions;
  iface->get_keybinding = ctk_entry_accessible_get_keybinding;
  iface->get_name = ctk_entry_accessible_action_get_name;
  iface->get_localized_name = ctk_entry_accessible_action_get_localized_name;
  iface->get_description = ctk_entry_accessible_action_get_description;
}
