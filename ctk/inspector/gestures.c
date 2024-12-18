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

#include "gestures.h"
#include "object-tree.h"

#include "ctksizegroup.h"
#include "ctkcomboboxtext.h"
#include "ctklistbox.h"
#include "ctkgesture.h"
#include "ctklabel.h"
#include "ctkframe.h"
#include "ctkwidgetprivate.h"

enum
{
  PROP_0,
  PROP_OBJECT_TREE
};

struct _CtkInspectorGesturesPrivate
{
  CtkSizeGroup *sizegroup;
  GObject *object;
  CtkInspectorObjectTree *object_tree;
};

G_DEFINE_TYPE_WITH_PRIVATE (CtkInspectorGestures, ctk_inspector_gestures, CTK_TYPE_BOX)

static void
ctk_inspector_gestures_init (CtkInspectorGestures *sl)
{
  sl->priv = ctk_inspector_gestures_get_instance_private (sl);
  sl->priv->sizegroup = ctk_size_group_new (CTK_SIZE_GROUP_HORIZONTAL);
  g_object_set (sl,
                "orientation", CTK_ORIENTATION_VERTICAL,
                "margin-start", 60,
                "margin-end", 60,
                "margin-top", 60,
                "margin-bottom", 30,
                "spacing", 10,
                NULL);
}

static void
clear_all (CtkInspectorGestures *sl)
{
  GList *children, *l;

  children = ctk_container_get_children (CTK_CONTAINER (sl));
  for (l = children; l; l = l->next)
    {
      CtkWidget *child;

      child = l->data;
      ctk_container_remove (CTK_CONTAINER (sl), child);
    }
  g_list_free (children);
}

static void
phase_changed_cb (CtkComboBox          *combo,
                  CtkInspectorGestures *sl G_GNUC_UNUSED)
{
  CtkWidget *row;
  CtkPropagationPhase phase;
  CtkGesture *gesture;

  phase = ctk_combo_box_get_active (combo);
  row = ctk_widget_get_ancestor (CTK_WIDGET (combo), CTK_TYPE_LIST_BOX_ROW);
  gesture = CTK_GESTURE (g_object_get_data (G_OBJECT (row), "gesture"));
  ctk_event_controller_set_propagation_phase (CTK_EVENT_CONTROLLER (gesture), phase);
}

static void
row_activated (CtkListBox           *box G_GNUC_UNUSED,
               CtkListBoxRow        *row,
               CtkInspectorGestures *sl)
{
  GObject *gesture;
  
  gesture = G_OBJECT (g_object_get_data (G_OBJECT (row), "gesture"));
  ctk_inspector_object_tree_select_object (sl->priv->object_tree, gesture);
}

static void
add_gesture (CtkInspectorGestures *sl,
             GObject              *object G_GNUC_UNUSED,
             CtkWidget            *listbox,
             CtkGesture           *gesture,
             CtkPropagationPhase   phase)
{
  CtkWidget *row;
  CtkWidget *box;
  CtkWidget *label;
  CtkWidget *combo;

  row = ctk_list_box_row_new ();
  ctk_container_add (CTK_CONTAINER (listbox), row);
  ctk_widget_show (row);
  box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 40);
  ctk_container_add (CTK_CONTAINER (row), box);
  g_object_set (box, "margin", 10, NULL);
  ctk_widget_show (box);
  label = ctk_label_new (g_type_name_from_instance ((GTypeInstance*)gesture));
  g_object_set (label, "xalign", 0.0, NULL);
  ctk_container_add (CTK_CONTAINER (box), label);
  ctk_size_group_add_widget (sl->priv->sizegroup, label);
  ctk_widget_show (label);
  ctk_widget_set_halign (label, CTK_ALIGN_START);
  ctk_widget_set_valign (label, CTK_ALIGN_BASELINE);

  combo = ctk_combo_box_text_new ();
  ctk_combo_box_text_insert_text (CTK_COMBO_BOX_TEXT (combo), CTK_PHASE_NONE, C_("event phase", "None"));
  ctk_combo_box_text_insert_text (CTK_COMBO_BOX_TEXT (combo), CTK_PHASE_CAPTURE, C_("event phase", "Capture"));
  ctk_combo_box_text_insert_text (CTK_COMBO_BOX_TEXT (combo), CTK_PHASE_BUBBLE, C_("event phase", "Bubble"));
  ctk_combo_box_text_insert_text (CTK_COMBO_BOX_TEXT (combo), CTK_PHASE_TARGET, C_("event phase", "Target"));
  ctk_combo_box_set_active (CTK_COMBO_BOX (combo), phase);
  ctk_container_add (CTK_CONTAINER (box), combo);
  ctk_widget_show (combo);
  ctk_widget_set_halign (label, CTK_ALIGN_END);
  ctk_widget_set_valign (label, CTK_ALIGN_BASELINE);

  g_object_set_data (G_OBJECT (row), "gesture", gesture);
  g_signal_connect (combo, "changed", G_CALLBACK (phase_changed_cb), sl);
}

static void
add_gesture_group (CtkInspectorGestures *sl,
                   GObject              *object,
                   CtkGesture           *gesture,
                   GHashTable           *hash)
{
  CtkWidget *frame;
  CtkWidget *listbox;
  GList *list, *l;
  CtkPropagationPhase phase;

  frame = ctk_frame_new (NULL);
  ctk_widget_show (frame);
  ctk_widget_set_halign (frame, CTK_ALIGN_CENTER);

  listbox = ctk_list_box_new ();
  g_signal_connect (listbox, "row-activated", G_CALLBACK (row_activated), sl);
  ctk_container_add (CTK_CONTAINER (frame), listbox);
  ctk_widget_show (listbox);
  ctk_list_box_set_selection_mode (CTK_LIST_BOX (listbox), CTK_SELECTION_NONE);

  list = ctk_gesture_get_group (gesture);
  for (l = list; l; l = l->next)
    {
      CtkGesture *g;

      g = l->data;
      phase = GPOINTER_TO_INT (g_hash_table_lookup (hash, g));
      add_gesture (sl, object, listbox, g, phase);
      g_hash_table_remove (hash, g);
    }
  g_list_free (list);

  ctk_container_add (CTK_CONTAINER (sl), frame);
}

void
ctk_inspector_gestures_set_object (CtkInspectorGestures *sl,
                                   GObject              *object)
{
  GHashTable *hash;
  GHashTableIter iter;
  GList *list, *l;
  gint phase;

  clear_all (sl);
  ctk_widget_hide (CTK_WIDGET (sl));

  if (!CTK_IS_WIDGET (object))
    return;

  hash = g_hash_table_new (g_direct_hash, g_direct_equal);
  for (phase = CTK_PHASE_NONE; phase <= CTK_PHASE_TARGET; phase++)
    {
      list = _ctk_widget_list_controllers (CTK_WIDGET (object), phase);
      for (l = list; l; l = l->next)
        {
          CtkEventController *controller = l->data;

          if (CTK_IS_GESTURE (controller))
            g_hash_table_insert (hash, controller, GINT_TO_POINTER (phase));
        }
      g_list_free (list);
    }

  if (g_hash_table_size (hash))
    ctk_widget_show (CTK_WIDGET (sl));

  while (g_hash_table_size (hash) > 0)
    {
      gpointer key, value;
      CtkGesture *gesture;
      g_hash_table_iter_init (&iter, hash);
      (void)g_hash_table_iter_next (&iter, &key, &value);
      gesture = key;
      add_gesture_group (sl, object, gesture, hash);
    }

  g_hash_table_unref (hash);
}

static void
get_property (GObject    *object,
              guint       param_id,
              GValue     *value,
              GParamSpec *pspec)
{
  CtkInspectorGestures *sl = CTK_INSPECTOR_GESTURES (object);

  switch (param_id)
    {
      case PROP_OBJECT_TREE:
        g_value_take_object (value, sl->priv->object_tree);
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
  CtkInspectorGestures *sl = CTK_INSPECTOR_GESTURES (object);

  switch (param_id)
    {
      case PROP_OBJECT_TREE:
        sl->priv->object_tree = g_value_get_object (value);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
        break;
    }
}

static void
ctk_inspector_gestures_class_init (CtkInspectorGesturesClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = get_property;
  object_class->set_property = set_property;

  g_object_class_install_property (object_class, PROP_OBJECT_TREE,
      g_param_spec_object ("object-tree", "Widget Tree", "Widget tree",
                           CTK_TYPE_WIDGET, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

// vim: set et sw=2 ts=2:
