/*
 * Copyright (c) 2015 Red Hat, Inc.
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

#include "strv-editor.h"
#include "ctkbutton.h"
#include "ctkentry.h"
#include "ctkbox.h"
#include "ctkstylecontext.h"
#include "ctkorientable.h"
#include "ctkmarshalers.h"

enum
{
  CHANGED,
  N_SIGNALS
};

static guint signals[N_SIGNALS] = { 0 };

G_DEFINE_TYPE (CtkInspectorStrvEditor, ctk_inspector_strv_editor, CTK_TYPE_BOX);

static void
emit_changed (CtkInspectorStrvEditor *editor)
{
  if (editor->blocked)
    return;

  g_signal_emit (editor, signals[CHANGED], 0);
}

static void
remove_string (CtkButton              *button,
               CtkInspectorStrvEditor *editor)
{
  ctk_widget_destroy (ctk_widget_get_parent (CTK_WIDGET (button)));
  emit_changed (editor);
}

static void
add_string (CtkInspectorStrvEditor *editor,
            const gchar            *str)
{
  CtkWidget *box;
  CtkWidget *entry;
  CtkWidget *button;

  box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
  ctk_style_context_add_class (ctk_widget_get_style_context (box), "linked");
  ctk_widget_show (box);

  entry = ctk_entry_new ();
  ctk_entry_set_text (CTK_ENTRY (entry), str);
  ctk_widget_show (entry);
  ctk_box_pack_start (CTK_BOX (box), entry, FALSE, TRUE, 0);
  g_object_set_data (G_OBJECT (box), "entry", entry);
  g_signal_connect_swapped (entry, "notify::text", G_CALLBACK (emit_changed), editor);

  button = ctk_button_new_from_icon_name ("user-trash-symbolic", CTK_ICON_SIZE_MENU);
  ctk_style_context_add_class (ctk_widget_get_style_context (button), "image-button");
  ctk_widget_show (button);
  ctk_box_pack_start (CTK_BOX (box), button, FALSE, FALSE, 0);
  g_signal_connect (button, "clicked", G_CALLBACK (remove_string), editor);

  ctk_box_pack_start (CTK_BOX (editor->box), box, FALSE, FALSE, 0);

  ctk_widget_grab_focus (entry);

  emit_changed (editor);
}

static void
add_cb (CtkButton              *button G_GNUC_UNUSED,
        CtkInspectorStrvEditor *editor)
{
  add_string (editor, "");
}

static void
ctk_inspector_strv_editor_init (CtkInspectorStrvEditor *editor)
{
  ctk_box_set_spacing (CTK_BOX (editor), 6);
  ctk_orientable_set_orientation (CTK_ORIENTABLE (editor), CTK_ORIENTATION_VERTICAL);
  editor->box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 6);
  ctk_widget_show (editor->box);

  editor->button = ctk_button_new_from_icon_name ("list-add-symbolic", CTK_ICON_SIZE_MENU);
  ctk_style_context_add_class (ctk_widget_get_style_context (editor->button), "image-button");
  ctk_widget_set_focus_on_click (editor->button, FALSE);
  ctk_widget_set_halign (editor->button, CTK_ALIGN_END);
  ctk_widget_show (editor->button);
  g_signal_connect (editor->button, "clicked", G_CALLBACK (add_cb), editor);

  ctk_box_pack_start (CTK_BOX (editor), editor->box, FALSE, TRUE, 0);
  ctk_box_pack_start (CTK_BOX (editor), editor->button, FALSE, FALSE, 0);
}

static void
ctk_inspector_strv_editor_class_init (CtkInspectorStrvEditorClass *class)
{
  signals[CHANGED] =
    g_signal_new ("changed",
                  G_TYPE_FROM_CLASS (class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (CtkInspectorStrvEditorClass, changed),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);
}

void
ctk_inspector_strv_editor_set_strv (CtkInspectorStrvEditor  *editor,
                                    gchar                  **strv)
{
  GList *children, *l;
  gint i;

  editor->blocked = TRUE;

  children = ctk_container_get_children (CTK_CONTAINER (editor->box));
  for (l = children; l; l = l->next)
    ctk_widget_destroy (CTK_WIDGET (l->data));
  g_list_free (children);

  if (strv)
    {
      for (i = 0; strv[i]; i++)
        add_string (editor, strv[i]);
    }

  editor->blocked = FALSE;

  emit_changed (editor);
}

gchar **
ctk_inspector_strv_editor_get_strv (CtkInspectorStrvEditor *editor)
{
  GList *children, *l;
  GPtrArray *p;

  p = g_ptr_array_new ();

  children = ctk_container_get_children (CTK_CONTAINER (editor->box));
  for (l = children; l; l = l->next)
    {
      CtkEntry *entry;

      entry = CTK_ENTRY (g_object_get_data (G_OBJECT (l->data), "entry"));
      g_ptr_array_add (p, g_strdup (ctk_entry_get_text (entry)));
    }
  g_list_free (children);

  g_ptr_array_add (p, NULL);

  return (gchar **)g_ptr_array_free (p, FALSE);
}
