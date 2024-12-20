/*
 * Copyright (c) 2014 Red Hat, Inc.
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

#include "action-editor.h"

#include "ctksizegroup.h"
#include "ctktogglebutton.h"
#include "ctkentry.h"
#include "ctkbin.h"
#include "ctklabel.h"

struct _CtkInspectorActionEditorPrivate
{
  GActionGroup *group;
  gchar *prefix;
  gchar *name;
  gboolean enabled;
  const GVariantType *parameter_type;
  GVariantType *state_type;
  CtkWidget *activate_button;
  CtkWidget *parameter_entry;
  CtkWidget *state_entry;
  CtkSizeGroup *sg;
};

enum
{
  PROP_0,
  PROP_GROUP,
  PROP_PREFIX,
  PROP_NAME
};

G_DEFINE_TYPE_WITH_PRIVATE (CtkInspectorActionEditor, ctk_inspector_action_editor, CTK_TYPE_BOX)

static void
ctk_inspector_action_editor_init (CtkInspectorActionEditor *editor)
{
  editor->priv = ctk_inspector_action_editor_get_instance_private (editor);
  g_object_set (editor,
                "orientation", CTK_ORIENTATION_VERTICAL,
                "spacing", 10,
                "margin", 10,
                NULL);
}

typedef void (*VariantEditorChanged) (CtkWidget *editor, gpointer data);

typedef struct {
  CtkWidget *editor;
  VariantEditorChanged callback;
  gpointer   data;
} VariantEditorData;

static void
variant_editor_changed_cb (GObject           *obj G_GNUC_UNUSED,
                           GParamSpec        *pspec G_GNUC_UNUSED,
                           VariantEditorData *data)
{
  data->callback (data->editor, data->data);
}

static CtkWidget *
variant_editor_new (const GVariantType   *type,
                    VariantEditorChanged  callback,
                    gpointer              data)
{
  CtkWidget *editor;
  CtkWidget *label;
  CtkWidget *entry;
  VariantEditorData *d;

  d = g_new (VariantEditorData, 1);
  d->callback = callback;
  d->data = data;

  if (g_variant_type_equal (type, G_VARIANT_TYPE_BOOLEAN))
    {
      editor = ctk_toggle_button_new_with_label ("FALSE");
      g_signal_connect (editor, "notify::active", G_CALLBACK (variant_editor_changed_cb), d);
    }   
  else if (g_variant_type_equal (type, G_VARIANT_TYPE_STRING))
    {
      editor = ctk_entry_new ();
      g_signal_connect (editor, "notify::text", G_CALLBACK (variant_editor_changed_cb), d);
    }
  else
    {
      editor = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 10);
      entry = ctk_entry_new ();
      ctk_container_add (CTK_CONTAINER (editor), entry);
      label = ctk_label_new (g_variant_type_peek_string (type));
      ctk_container_add (CTK_CONTAINER (editor), label);
      g_signal_connect (entry, "notify::text", G_CALLBACK (variant_editor_changed_cb), d);
    }

  g_object_set_data (G_OBJECT (editor), "type", (gpointer)type);
  d->editor = editor;
  g_object_set_data_full (G_OBJECT (editor), "callback", d, g_free);

  ctk_widget_show_all (editor);

  return editor;
}

static void
variant_editor_set_value (CtkWidget *editor,
                          GVariant  *value)
{
  const GVariantType *type;
  gpointer data;

  data = g_object_get_data (G_OBJECT (editor), "callback");
  g_signal_handlers_block_by_func (editor, variant_editor_changed_cb, data);

  type = g_variant_get_type (value);
  if (g_variant_type_equal (type, G_VARIANT_TYPE_BOOLEAN))
    {
      CtkToggleButton *tb = CTK_TOGGLE_BUTTON (editor);
      CtkWidget *child;

      ctk_toggle_button_set_active (tb, g_variant_get_boolean (value));
      child = ctk_bin_get_child (CTK_BIN (tb));
      ctk_label_set_text (CTK_LABEL (child),
                          g_variant_get_boolean (value) ? "TRUE" : "FALSE");    
    }   
  else if (g_variant_type_equal (type, G_VARIANT_TYPE_STRING))
    {
      CtkEntry *entry = CTK_ENTRY (editor);
      ctk_entry_set_text (entry, g_variant_get_string (value, NULL));
    }
  else
    {
      GList *children;
      CtkEntry *entry;
      gchar *text;

      children = ctk_container_get_children (CTK_CONTAINER (editor));
      entry = children->data;
      g_list_free (children);

      text = g_variant_print (value, FALSE);
      ctk_entry_set_text (entry, text);
      g_free (text);
    }

  g_signal_handlers_unblock_by_func (editor, variant_editor_changed_cb, data);
}

static GVariant *
variant_editor_get_value (CtkWidget *editor)
{
  const GVariantType *type;
  GVariant *value;

  type = (const GVariantType *) g_object_get_data (G_OBJECT (editor), "type");
  if (g_variant_type_equal (type, G_VARIANT_TYPE_BOOLEAN))
    {
      CtkToggleButton *tb = CTK_TOGGLE_BUTTON (editor);
      value = g_variant_new_boolean (ctk_toggle_button_get_active (tb));
    }
  else if (g_variant_type_equal (type, G_VARIANT_TYPE_STRING))
    {
      CtkEntry *entry = CTK_ENTRY (editor);
      value = g_variant_new_string (ctk_entry_get_text (entry));
    }
  else
    {
      GList *children;
      CtkEntry *entry;
      const gchar *text;

      children = ctk_container_get_children (CTK_CONTAINER (editor));
      entry = children->data;
      text = ctk_entry_get_text (entry);
      g_list_free (children);

      value = g_variant_parse (type, text, NULL, NULL, NULL);
    }

  return value;
}

static void
activate_action (CtkWidget                *button G_GNUC_UNUSED,
                 CtkInspectorActionEditor *r)
{
  GVariant *parameter = NULL;

  if (r->priv->parameter_entry)
    parameter = variant_editor_get_value (r->priv->parameter_entry);
  g_action_group_activate_action (r->priv->group, r->priv->name, parameter);
}

static void
parameter_changed (CtkWidget *editor,
                   gpointer   data)
{
  CtkInspectorActionEditor *r = data;
  GVariant *value;

  value = variant_editor_get_value (editor);
  ctk_widget_set_sensitive (r->priv->activate_button, r->priv->enabled && value != NULL);
  if (value)
    g_variant_unref (value);
}

static void
state_changed (CtkWidget *editor,
               gpointer   data)
{
  CtkInspectorActionEditor *r = data;
  GVariant *value;

  value = variant_editor_get_value (editor);
  if (value)
    g_action_group_change_action_state (r->priv->group, r->priv->name, value);
}

static void
action_enabled_changed_cb (GActionGroup             *group G_GNUC_UNUSED,
                           const gchar              *action_name G_GNUC_UNUSED,
                           gboolean                  enabled,
                           CtkInspectorActionEditor *r)
{
  r->priv->enabled = enabled;
  if (r->priv->parameter_entry)
    {
      ctk_widget_set_sensitive (r->priv->parameter_entry, enabled);
      parameter_changed (r->priv->parameter_entry, r);
    }
}

static void
action_state_changed_cb (GActionGroup             *group G_GNUC_UNUSED,
                         const gchar              *action_name G_GNUC_UNUSED,
                         GVariant                 *state,
                         CtkInspectorActionEditor *r)
{
  if (r->priv->state_entry)
    variant_editor_set_value (r->priv->state_entry, state);
}

static void
constructed (GObject *object)
{
  CtkInspectorActionEditor *r = CTK_INSPECTOR_ACTION_EDITOR (object);
  GVariant *state;
  gchar *fullname;
  CtkWidget *row;

  r->priv->enabled = g_action_group_get_action_enabled (r->priv->group, r->priv->name);
  state = g_action_group_get_action_state (r->priv->group, r->priv->name);

  fullname = g_strdup_printf ("%s.%s", r->priv->prefix, r->priv->name);
  ctk_container_add (CTK_CONTAINER (r), ctk_label_new (fullname));
  g_free (fullname);

  r->priv->sg = ctk_size_group_new (CTK_SIZE_GROUP_HORIZONTAL);

  row = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 10);

  r->priv->activate_button = ctk_button_new_with_label (_("Activate"));
  g_signal_connect (r->priv->activate_button, "clicked", G_CALLBACK (activate_action), r);

  ctk_size_group_add_widget (r->priv->sg, r->priv->activate_button);
  ctk_widget_set_sensitive (r->priv->activate_button, r->priv->enabled);
  ctk_container_add (CTK_CONTAINER (row), r->priv->activate_button);

  r->priv->parameter_type = g_action_group_get_action_parameter_type (r->priv->group, r->priv->name);
  if (r->priv->parameter_type)
    {
      r->priv->parameter_entry = variant_editor_new (r->priv->parameter_type, parameter_changed, r);
      ctk_widget_set_sensitive (r->priv->parameter_entry, r->priv->enabled);
      ctk_container_add (CTK_CONTAINER (row), r->priv->parameter_entry);
    }

  ctk_container_add (CTK_CONTAINER (r), row);

  if (state)
    {
      CtkWidget *label;

      r->priv->state_type = g_variant_type_copy (g_variant_get_type (state));
      row = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 10);
      label = ctk_label_new (_("State"));
      ctk_size_group_add_widget (r->priv->sg, label);
      ctk_container_add (CTK_CONTAINER (row), label);
      r->priv->state_entry = variant_editor_new (r->priv->state_type, state_changed, r);
      variant_editor_set_value (r->priv->state_entry, state);
      ctk_container_add (CTK_CONTAINER (row), r->priv->state_entry);
      ctk_container_add (CTK_CONTAINER (r), row);
    }

  g_signal_connect (r->priv->group, "action-enabled-changed",
                    G_CALLBACK (action_enabled_changed_cb), r);
  g_signal_connect (r->priv->group, "action-state-changed",
                    G_CALLBACK (action_state_changed_cb), r);

  ctk_widget_show_all (CTK_WIDGET (r));
}

static void
finalize (GObject *object)
{
  CtkInspectorActionEditor *r = CTK_INSPECTOR_ACTION_EDITOR (object);

  g_free (r->priv->prefix);
  g_free (r->priv->name);
  g_object_unref (r->priv->sg);
  if (r->priv->state_type)
    g_variant_type_free (r->priv->state_type);
  g_signal_handlers_disconnect_by_func (r->priv->group, action_enabled_changed_cb, r);
  g_signal_handlers_disconnect_by_func (r->priv->group, action_state_changed_cb, r);

  G_OBJECT_CLASS (ctk_inspector_action_editor_parent_class)->finalize (object);
}

static void
get_property (GObject    *object,
              guint       param_id,
              GValue     *value,
              GParamSpec *pspec)
{
  CtkInspectorActionEditor *r = CTK_INSPECTOR_ACTION_EDITOR (object);

  switch (param_id)
    {
    case PROP_GROUP:
      g_value_set_object (value, r->priv->group);
      break;

    case PROP_PREFIX:
      g_value_set_string (value, r->priv->prefix);
      break;

    case PROP_NAME:
      g_value_set_string (value, r->priv->name);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
set_property (GObject      *object,
              guint         param_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  CtkInspectorActionEditor *r = CTK_INSPECTOR_ACTION_EDITOR (object);

  switch (param_id)
    {
    case PROP_GROUP:
      r->priv->group = g_value_get_object (value);
      break;

    case PROP_PREFIX:
      g_free (r->priv->prefix);
      r->priv->prefix = g_value_dup_string (value);
      break;

    case PROP_NAME:
      g_free (r->priv->name);
      r->priv->name = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
      break;
    }
}

static void
ctk_inspector_action_editor_class_init (CtkInspectorActionEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = constructed;
  object_class->finalize = finalize;
  object_class->get_property = get_property;
  object_class->set_property = set_property;

  g_object_class_install_property (object_class, PROP_GROUP,
      g_param_spec_object ("group", "Action Group", "The Action Group containing the action",
                           G_TYPE_ACTION_GROUP, G_PARAM_READWRITE|G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_PREFIX,
      g_param_spec_string ("prefix", "Prefix", "The action name prefix",
                           NULL, G_PARAM_READWRITE|G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_NAME,
      g_param_spec_string ("name", "Name", "The action name",
                           NULL, G_PARAM_READWRITE|G_PARAM_CONSTRUCT));
}

CtkWidget *
ctk_inspector_action_editor_new (GActionGroup *group,
                                 const gchar  *prefix,
                                 const gchar  *name)
{
  return g_object_new (CTK_TYPE_INSPECTOR_ACTION_EDITOR,
                       "group", group,
                       "prefix", prefix,
                       "name", name,
                       NULL);
}
