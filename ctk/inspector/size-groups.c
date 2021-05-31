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

#include "size-groups.h"
#include "window.h"

#include "ctkcomboboxtext.h"
#include "ctkframe.h"
#include "ctklabel.h"
#include "ctklistbox.h"
#include "ctksizegroup.h"
#include "ctkswitch.h"
#include "ctkwidgetprivate.h"


typedef struct {
  GtkListBoxRow parent;
  GtkWidget *widget;
} SizeGroupRow;

typedef struct {
  GtkListBoxRowClass parent;
} SizeGroupRowClass;

enum {
  PROP_WIDGET = 1,
  LAST_PROPERTY
};

GParamSpec *properties[LAST_PROPERTY] = { NULL };

GType size_group_row_get_type (void);

G_DEFINE_TYPE (SizeGroupRow, size_group_row, CTK_TYPE_LIST_BOX_ROW)

static void
size_group_row_init (SizeGroupRow *row)
{
}

static void
size_group_row_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  SizeGroupRow *row = (SizeGroupRow*)object;

  switch (property_id)
    {
    case PROP_WIDGET:
      g_value_set_pointer (value, row->widget);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
size_group_row_widget_destroyed (GtkWidget *widget, SizeGroupRow *row)
{
  GtkWidget *parent;

  parent = ctk_widget_get_parent (CTK_WIDGET (row));
  if (parent)
    ctk_container_remove (CTK_CONTAINER (parent), CTK_WIDGET (row));
}

static void
set_widget (SizeGroupRow *row, GtkWidget *widget)
{
  if (row->widget)
    g_signal_handlers_disconnect_by_func (row->widget,
                                          size_group_row_widget_destroyed, row);

  row->widget = widget;

  if (row->widget)
    g_signal_connect (row->widget, "destroy",
                      G_CALLBACK (size_group_row_widget_destroyed), row);
}

static void
size_group_row_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  SizeGroupRow *row = (SizeGroupRow*)object;

  switch (property_id)
    {
    case PROP_WIDGET:
      set_widget (row, g_value_get_pointer (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}
static void
size_group_row_finalize (GObject *object)
{
  SizeGroupRow *row = (SizeGroupRow *)object;

  set_widget (row, NULL);

  G_OBJECT_CLASS (size_group_row_parent_class)->finalize (object);
}

static void
size_group_state_flags_changed (GtkWidget     *widget,
                                GtkStateFlags  old_state)
{
  SizeGroupRow *row = (SizeGroupRow*)widget;
  GtkStateFlags state;

  if (!row->widget)
    return;

  state = ctk_widget_get_state_flags (widget);
  if ((state & CTK_STATE_FLAG_PRELIGHT) != (old_state & CTK_STATE_FLAG_PRELIGHT))
    {
      if (state & CTK_STATE_FLAG_PRELIGHT)
        ctk_inspector_start_highlight (row->widget);
      else
        ctk_inspector_stop_highlight (row->widget);
    }
}

static void
size_group_row_class_init (SizeGroupRowClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = CTK_WIDGET_CLASS (class);

  object_class->finalize = size_group_row_finalize;
  object_class->get_property = size_group_row_get_property;
  object_class->set_property = size_group_row_set_property;

  widget_class->state_flags_changed = size_group_state_flags_changed;

  properties[PROP_WIDGET] =
    g_param_spec_pointer ("widget", "Widget", "Widget", G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, LAST_PROPERTY, properties);

}

G_DEFINE_TYPE (GtkInspectorSizeGroups, ctk_inspector_size_groups, CTK_TYPE_BOX)

static void
clear_view (GtkInspectorSizeGroups *sl)
{
  GList *children, *l;
  GtkWidget *child;

  children = ctk_container_get_children (CTK_CONTAINER (sl));
  for (l = children; l; l = l->next)
    {
      child = l->data;
      ctk_container_remove (CTK_CONTAINER (sl), child);
    }
  g_list_free (children);
}

static void
add_widget (GtkInspectorSizeGroups *sl,
            GtkListBox             *listbox,
            GtkWidget              *widget)
{
  GtkWidget *row;
  GtkWidget *label;
  gchar *text;

  row = g_object_new (size_group_row_get_type (), "widget", widget, NULL);
  text = g_strdup_printf ("%p (%s)", widget, g_type_name_from_instance ((GTypeInstance*)widget));
  label = ctk_label_new (text);
  g_free (text);
  g_object_set (label, "margin", 10, NULL);
  ctk_widget_set_halign (label, CTK_ALIGN_START);
  ctk_widget_set_valign (label, CTK_ALIGN_BASELINE);
  ctk_widget_show (label);
  ctk_container_add (CTK_CONTAINER (row), label);
  ctk_container_add (CTK_CONTAINER (listbox), row);
}

static void
add_size_group (GtkInspectorSizeGroups *sl,
                GtkSizeGroup           *group)
{
  GtkWidget *frame, *box, *box2;
  GtkWidget *label, *sw, *combo;
  GSList *widgets, *l;
  GtkWidget *listbox;

  frame = ctk_frame_new (NULL);
  ctk_container_add (CTK_CONTAINER (sl), frame);
  box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  ctk_style_context_add_class (ctk_widget_get_style_context (box), CTK_STYLE_CLASS_VIEW);
  ctk_container_add (CTK_CONTAINER (frame), box);

  box2 = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 10);
  ctk_container_add (CTK_CONTAINER (box), box2);

  label = ctk_label_new (_("Ignore hidden"));
  g_object_set (label, "margin", 10, NULL);
  ctk_widget_set_halign (label, CTK_ALIGN_START);
  ctk_widget_set_valign (label, CTK_ALIGN_BASELINE);
  ctk_box_pack_start (CTK_BOX (box2), label, TRUE, TRUE, 0);

  sw = ctk_switch_new ();
  g_object_set (sw, "margin", 10, NULL);
  ctk_widget_set_halign (sw, CTK_ALIGN_END);
  ctk_widget_set_valign (sw, CTK_ALIGN_BASELINE);
  g_object_bind_property (group, "ignore-hidden",
                          sw, "active",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
  ctk_box_pack_start (CTK_BOX (box2), sw, FALSE, FALSE, 0);

  box2 = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 10);
  ctk_container_add (CTK_CONTAINER (box), box2);

  label = ctk_label_new (_("Mode"));
  g_object_set (label, "margin", 10, NULL);
  ctk_widget_set_halign (label, CTK_ALIGN_START);
  ctk_widget_set_valign (label, CTK_ALIGN_BASELINE);
  ctk_box_pack_start (CTK_BOX (box2), label, TRUE, TRUE, 0);

  combo = ctk_combo_box_text_new ();
  g_object_set (combo, "margin", 10, NULL);
  ctk_widget_set_halign (combo, CTK_ALIGN_END);
  ctk_widget_set_valign (combo, CTK_ALIGN_BASELINE);
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo), C_("sizegroup mode", "None"));
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo), C_("sizegroup mode", "Horizontal"));
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo), C_("sizegroup mode", "Vertical"));
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo), C_("sizegroup mode", "Both"));
  g_object_bind_property (group, "mode",
                          combo, "active",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
  ctk_box_pack_start (CTK_BOX (box2), combo, FALSE, FALSE, 0);

  listbox = ctk_list_box_new ();
  ctk_container_add (CTK_CONTAINER (box), listbox);
  ctk_list_box_set_selection_mode (CTK_LIST_BOX (listbox), CTK_SELECTION_NONE);

  widgets = ctk_size_group_get_widgets (group);
  for (l = widgets; l; l = l->next)
    add_widget (sl, CTK_LIST_BOX (listbox), CTK_WIDGET (l->data));

  ctk_widget_show_all (frame);
}

void
ctk_inspector_size_groups_set_object (GtkInspectorSizeGroups *sl,
                                      GObject                *object)
{
  GSList *groups, *l;

  clear_view (sl);

  if (!CTK_IS_WIDGET (object))
    {
      ctk_widget_hide (CTK_WIDGET (sl));
      return;
    }

  groups = _ctk_widget_get_sizegroups (CTK_WIDGET (object));
  if (groups)
    ctk_widget_show (CTK_WIDGET (sl));
  for (l = groups; l; l = l->next)
    {
      GtkSizeGroup *group = l->data;
      add_size_group (sl, group);
    }
}

static void
ctk_inspector_size_groups_init (GtkInspectorSizeGroups *sl)
{
  g_object_set (sl,
                "orientation", CTK_ORIENTATION_VERTICAL,
                "margin-start", 60,
                "margin-end", 60,
                "margin-bottom", 60,
                "margin-bottom", 30,
                "spacing", 10,
                NULL);
}

static void
ctk_inspector_size_groups_class_init (GtkInspectorSizeGroupsClass *klass)
{
}

// vim: set et sw=2 ts=2:
