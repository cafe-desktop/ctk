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

#include "misc-info.h"
#include "window.h"
#include "object-tree.h"

#include "ctktypebuiltins.h"
#include "ctktreeview.h"
#include "ctkbuildable.h"
#include "ctklabel.h"
#include "ctkframe.h"
#include "ctkbutton.h"
#include "ctkwidgetprivate.h"


struct _GtkInspectorMiscInfoPrivate {
  GtkInspectorObjectTree *object_tree;

  GObject *object;

  GtkWidget *address;
  GtkWidget *refcount_row;
  GtkWidget *refcount;
  GtkWidget *state_row;
  GtkWidget *state;
  GtkWidget *buildable_id_row;
  GtkWidget *buildable_id;
  GtkWidget *default_widget_row;
  GtkWidget *default_widget;
  GtkWidget *default_widget_button;
  GtkWidget *focus_widget_row;
  GtkWidget *focus_widget;
  GtkWidget *focus_widget_button;
  GtkWidget *mnemonic_label_row;
  GtkWidget *mnemonic_label;
  GtkWidget *request_mode_row;
  GtkWidget *request_mode;
  GtkWidget *allocated_size_row;
  GtkWidget *allocated_size;
  GtkWidget *baseline_row;
  GtkWidget *baseline;
  GtkWidget *clip_area_row;
  GtkWidget *clip_area;
  GtkWidget *frame_clock_row;
  GtkWidget *frame_clock;
  GtkWidget *frame_clock_button;
  GtkWidget *tick_callback_row;
  GtkWidget *tick_callback;
  GtkWidget *framerate_row;
  GtkWidget *framerate;
  GtkWidget *framecount_row;
  GtkWidget *framecount;
  GtkWidget *accessible_role_row;
  GtkWidget *accessible_role;
  GtkWidget *accessible_name_row;
  GtkWidget *accessible_name;
  GtkWidget *accessible_description_row;
  GtkWidget *accessible_description;
  GtkWidget *mapped_row;
  GtkWidget *mapped;
  GtkWidget *realized_row;
  GtkWidget *realized;
  GtkWidget *is_toplevel_row;
  GtkWidget *is_toplevel;
  GtkWidget *child_visible_row;
  GtkWidget *child_visible;

  guint update_source_id;
  gint64 last_frame;
};

enum
{
  PROP_0,
  PROP_OBJECT_TREE
};

G_DEFINE_TYPE_WITH_PRIVATE (GtkInspectorMiscInfo, ctk_inspector_misc_info, CTK_TYPE_SCROLLED_WINDOW)

static gchar *
format_state_flags (GtkStateFlags state)
{
  GFlagsClass *fclass;
  GString *str;
  gint i;

  str = g_string_new ("");

  if (state)
    {
      fclass = g_type_class_ref (CTK_TYPE_STATE_FLAGS);
      for (i = 0; i < fclass->n_values; i++)
        {
          if (state & fclass->values[i].value)
            {
              if (str->len)
                g_string_append (str, " | ");
              g_string_append (str, fclass->values[i].value_nick);
            }
        } 
      g_type_class_unref (fclass);
    }
  else
    g_string_append (str, "normal");

  return g_string_free (str, FALSE);
}

static void
state_flags_changed (GtkWidget *w, GtkStateFlags old_flags, GtkInspectorMiscInfo *sl)
{
  gchar *s;

  s = format_state_flags (ctk_widget_get_state_flags (w));
  ctk_label_set_label (CTK_LABEL (sl->priv->state), s);
  g_free (s);
}

static void
allocation_changed (GtkWidget *w, GdkRectangle *allocation, GtkInspectorMiscInfo *sl)
{
  GtkAllocation alloc;
  GtkAllocation clip;
  gchar *size_label;
  GEnumClass *class;
  GEnumValue *value;

  ctk_widget_get_allocation (w, &alloc);
  size_label = g_strdup_printf ("%d × %d +%d +%d",
                                alloc.width, alloc.height,
                                alloc.x, alloc.y);

  ctk_label_set_label (CTK_LABEL (sl->priv->allocated_size), size_label);
  g_free (size_label);

  size_label = g_strdup_printf ("%d", ctk_widget_get_allocated_baseline (w));
  ctk_label_set_label (CTK_LABEL (sl->priv->baseline), size_label);
  g_free (size_label);

  ctk_widget_get_clip (w, &clip);

  size_label = g_strdup_printf ("%d × %d +%d +%d",
                                clip.width, clip.height,
                                clip.x, clip.y);
  ctk_label_set_label (CTK_LABEL (sl->priv->clip_area), size_label);
  g_free (size_label);

  class = G_ENUM_CLASS (g_type_class_ref (CTK_TYPE_SIZE_REQUEST_MODE));
  value = g_enum_get_value (class, ctk_widget_get_request_mode (w));
  ctk_label_set_label (CTK_LABEL (sl->priv->request_mode), value->value_nick);
  g_type_class_unref (class);
}

static void
disconnect_each_other (gpointer  still_alive,
                       GObject  *for_science)
{
  if (CTK_INSPECTOR_IS_MISC_INFO (still_alive))
    {
      GtkInspectorMiscInfo *self = CTK_INSPECTOR_MISC_INFO (still_alive);
      self->priv->object = NULL;
    }

  g_signal_handlers_disconnect_matched (still_alive, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, for_science);
  g_object_weak_unref (still_alive, disconnect_each_other, for_science);
}

static void
show_object (GtkInspectorMiscInfo *sl,
             GObject              *object,
             const gchar          *tab)
{
  GtkTreeIter iter;

  g_object_set_data (G_OBJECT (sl->priv->object_tree), "next-tab", (gpointer)tab);
  if (ctk_inspector_object_tree_find_object (sl->priv->object_tree, object, &iter))
    {
      ctk_inspector_object_tree_select_object (sl->priv->object_tree, object);
    }
  else if (CTK_IS_WIDGET (object) &&
           ctk_inspector_object_tree_find_object (sl->priv->object_tree, G_OBJECT (ctk_widget_get_parent (CTK_WIDGET (object))), &iter))

    {
      ctk_inspector_object_tree_append_object (sl->priv->object_tree, object, &iter, NULL);
      ctk_inspector_object_tree_select_object (sl->priv->object_tree, object);
    }
  else
    {
      g_warning ("GtkInspector: couldn't find the object in the tree");
    }
}

static void
update_default_widget (GtkInspectorMiscInfo *sl)
{
  GtkWidget *widget;

  widget = ctk_window_get_default_widget (CTK_WINDOW (sl->priv->object));
  if (widget)
    {
      gchar *tmp;
      tmp = g_strdup_printf ("%p", widget);
      ctk_label_set_label (CTK_LABEL (sl->priv->default_widget), tmp);
      g_free (tmp);
      ctk_widget_set_sensitive (sl->priv->default_widget_button, TRUE);
    }
  else
    {
      ctk_label_set_label (CTK_LABEL (sl->priv->default_widget), "NULL");   
      ctk_widget_set_sensitive (sl->priv->default_widget_button, FALSE);
    }
}

static void
show_default_widget (GtkWidget *button, GtkInspectorMiscInfo *sl)
{
  GtkWidget *widget;

  update_default_widget (sl);
  widget = ctk_window_get_default_widget (CTK_WINDOW (sl->priv->object));
  if (widget)
    show_object (sl, G_OBJECT (widget), "properties"); 
}

static void
update_focus_widget (GtkInspectorMiscInfo *sl)
{
  GtkWidget *widget;

  widget = ctk_window_get_focus (CTK_WINDOW (sl->priv->object));
  if (widget)
    {
      gchar *tmp;
      tmp = g_strdup_printf ("%p", widget);
      ctk_label_set_label (CTK_LABEL (sl->priv->focus_widget), tmp);
      g_free (tmp);
      ctk_widget_set_sensitive (sl->priv->focus_widget_button, TRUE);
    }
  else
    {
      ctk_label_set_label (CTK_LABEL (sl->priv->focus_widget), "NULL");   
      ctk_widget_set_sensitive (sl->priv->focus_widget_button, FALSE);
    }
}

static void
set_focus_cb (GtkWindow *window, GtkWidget *focus, GtkInspectorMiscInfo *sl)
{
  update_focus_widget (sl);
}

static void
show_focus_widget (GtkWidget *button, GtkInspectorMiscInfo *sl)
{
  GtkWidget *widget;

  widget = ctk_window_get_focus (CTK_WINDOW (sl->priv->object));
  if (widget)
    show_object (sl, G_OBJECT (widget), "properties");
}

static void
show_mnemonic_label (GtkWidget *button, GtkInspectorMiscInfo *sl)
{
  GtkWidget *widget;

  widget = g_object_get_data (G_OBJECT (button), "mnemonic-label");
  if (widget)
    show_object (sl, G_OBJECT (widget), "properties");
}

static void
show_frame_clock (GtkWidget *button, GtkInspectorMiscInfo *sl)
{
  GObject *clock;

  clock = (GObject *)ctk_widget_get_frame_clock (CTK_WIDGET (sl->priv->object));
  if (clock)
    show_object (sl, G_OBJECT (clock), "properties");
}

static void
update_frame_clock (GtkInspectorMiscInfo *sl)
{
  GObject *clock;

  clock = (GObject *)ctk_widget_get_frame_clock (CTK_WIDGET (sl->priv->object));
  if (clock)
    {
      gchar *tmp;
      tmp = g_strdup_printf ("%p", clock);
      ctk_label_set_label (CTK_LABEL (sl->priv->frame_clock), tmp);
      g_free (tmp);
      ctk_widget_set_sensitive (sl->priv->frame_clock_button, TRUE);
    }
  else
    {
      ctk_label_set_label (CTK_LABEL (sl->priv->frame_clock), "NULL");
      ctk_widget_set_sensitive (sl->priv->frame_clock_button, FALSE);
    }
}

static gboolean
update_info (gpointer data)
{
  GtkInspectorMiscInfo *sl = data;
  gchar *tmp;

  tmp = g_strdup_printf ("%p", sl->priv->object);
  ctk_label_set_text (CTK_LABEL (sl->priv->address), tmp);
  g_free (tmp);

  if (G_IS_OBJECT (sl->priv->object))
    {
      tmp = g_strdup_printf ("%d", sl->priv->object->ref_count);
      ctk_label_set_text (CTK_LABEL (sl->priv->refcount), tmp);
      g_free (tmp);
    }

  if (CTK_IS_WIDGET (sl->priv->object))
    {
      AtkObject *accessible;
      AtkRole role;
      GList *list, *l;

      ctk_container_forall (CTK_CONTAINER (sl->priv->mnemonic_label), (GtkCallback)ctk_widget_destroy, NULL);
      list = ctk_widget_list_mnemonic_labels (CTK_WIDGET (sl->priv->object));
      for (l = list; l; l = l->next)
        {
          GtkWidget *button;

          tmp = g_strdup_printf ("%p (%s)", l->data, g_type_name_from_instance ((GTypeInstance*)l->data));
          button = ctk_button_new_with_label (tmp);
          g_free (tmp);
          ctk_widget_show (button);
          ctk_container_add (CTK_CONTAINER (sl->priv->mnemonic_label), button);
          g_object_set_data (G_OBJECT (button), "mnemonic-label", l->data);
          g_signal_connect (button, "clicked", G_CALLBACK (show_mnemonic_label), sl);
        }
      g_list_free (list);

      ctk_widget_set_visible (sl->priv->tick_callback, ctk_widget_has_tick_callback (CTK_WIDGET (sl->priv->object)));

      accessible = ATK_OBJECT (ctk_widget_get_accessible (CTK_WIDGET (sl->priv->object)));
      role = atk_object_get_role (accessible);
      ctk_label_set_text (CTK_LABEL (sl->priv->accessible_role), atk_role_get_name (role));
      ctk_label_set_text (CTK_LABEL (sl->priv->accessible_name), atk_object_get_name (accessible));
      ctk_label_set_text (CTK_LABEL (sl->priv->accessible_description), atk_object_get_description (accessible));
      ctk_widget_set_visible (sl->priv->mapped, ctk_widget_get_mapped (CTK_WIDGET (sl->priv->object)));
      ctk_widget_set_visible (sl->priv->realized, ctk_widget_get_realized (CTK_WIDGET (sl->priv->object)));
      ctk_widget_set_visible (sl->priv->is_toplevel, ctk_widget_is_toplevel (CTK_WIDGET (sl->priv->object)));
      ctk_widget_set_visible (sl->priv->child_visible, ctk_widget_get_child_visible (CTK_WIDGET (sl->priv->object)));

      update_frame_clock (sl);
    }

  if (CTK_IS_BUILDABLE (sl->priv->object))
    {
      ctk_label_set_text (CTK_LABEL (sl->priv->buildable_id),
                          ctk_buildable_get_name (CTK_BUILDABLE (sl->priv->object)));
    }

  if (CTK_IS_WINDOW (sl->priv->object))
    {
      update_default_widget (sl);
      update_focus_widget (sl);
    }

  if (GDK_IS_FRAME_CLOCK (sl->priv->object))
    {
      GdkFrameClock *clock;
      gint64 frame;
      gint64 frame_time;
      gint64 history_start;
      gint64 history_len;
      gint64 previous_frame_time;
      GdkFrameTimings *previous_timings;

      clock = GDK_FRAME_CLOCK (sl->priv->object);
      frame = gdk_frame_clock_get_frame_counter (clock);
      frame_time = gdk_frame_clock_get_frame_time (clock);

      tmp = g_strdup_printf ("%"G_GINT64_FORMAT, frame);
      ctk_label_set_label (CTK_LABEL (sl->priv->framecount), tmp);
      g_free (tmp);

      history_start = gdk_frame_clock_get_history_start (clock);
      history_len = frame - history_start;

      if (history_len > 0 && sl->priv->last_frame != frame)
        {
          previous_timings = gdk_frame_clock_get_timings (clock, history_start);
          previous_frame_time = gdk_frame_timings_get_frame_time (previous_timings);
          tmp = g_strdup_printf ("%4.1f ⁄ s", (G_USEC_PER_SEC * history_len) / (double) (frame_time - previous_frame_time));
          ctk_label_set_label (CTK_LABEL (sl->priv->framerate), tmp);
          g_free (tmp);
        }
      else
        {
          ctk_label_set_label (CTK_LABEL (sl->priv->framerate), "—");
        }

      sl->priv->last_frame = frame;
    }

  return G_SOURCE_CONTINUE;
}

void
ctk_inspector_misc_info_set_object (GtkInspectorMiscInfo *sl,
                                    GObject              *object)
{
  if (sl->priv->object)
    {
      g_signal_handlers_disconnect_by_func (sl->priv->object, state_flags_changed, sl);
      g_signal_handlers_disconnect_by_func (sl->priv->object, set_focus_cb, sl);
      g_signal_handlers_disconnect_by_func (sl->priv->object, allocation_changed, sl);
      disconnect_each_other (sl->priv->object, G_OBJECT (sl));
      disconnect_each_other (sl, sl->priv->object);
      sl->priv->object = NULL;
    }

  ctk_widget_show (CTK_WIDGET (sl));

  sl->priv->object = object;
  g_object_weak_ref (G_OBJECT (sl), disconnect_each_other, object);
  g_object_weak_ref (object, disconnect_each_other, sl);

  if (CTK_IS_WIDGET (object))
    {
      ctk_widget_show (sl->priv->refcount_row);
      ctk_widget_show (sl->priv->state_row);
      ctk_widget_show (sl->priv->request_mode_row);
      ctk_widget_show (sl->priv->allocated_size_row);
      ctk_widget_show (sl->priv->baseline_row);
      ctk_widget_show (sl->priv->clip_area_row);
      ctk_widget_show (sl->priv->mnemonic_label_row);
      ctk_widget_show (sl->priv->tick_callback_row);
      ctk_widget_show (sl->priv->accessible_role_row);
      ctk_widget_show (sl->priv->accessible_name_row);
      ctk_widget_show (sl->priv->accessible_description_row);
      ctk_widget_show (sl->priv->mapped_row);
      ctk_widget_show (sl->priv->realized_row);
      ctk_widget_show (sl->priv->is_toplevel_row);
      ctk_widget_show (sl->priv->is_toplevel_row);
      ctk_widget_show (sl->priv->frame_clock_row);

      g_signal_connect_object (object, "state-flags-changed", G_CALLBACK (state_flags_changed), sl, 0);
      state_flags_changed (CTK_WIDGET (sl->priv->object), 0, sl);

      g_signal_connect_object (object, "size-allocate", G_CALLBACK (allocation_changed), sl, 0);
      allocation_changed (CTK_WIDGET (sl->priv->object), NULL, sl);
    }
  else
    {
      ctk_widget_hide (sl->priv->state_row);
      ctk_widget_hide (sl->priv->request_mode_row);
      ctk_widget_hide (sl->priv->mnemonic_label_row);
      ctk_widget_hide (sl->priv->allocated_size_row);
      ctk_widget_hide (sl->priv->baseline_row);
      ctk_widget_hide (sl->priv->clip_area_row);
      ctk_widget_hide (sl->priv->tick_callback_row);
      ctk_widget_hide (sl->priv->accessible_role_row);
      ctk_widget_hide (sl->priv->accessible_name_row);
      ctk_widget_hide (sl->priv->accessible_description_row);
      ctk_widget_hide (sl->priv->mapped_row);
      ctk_widget_hide (sl->priv->realized_row);
      ctk_widget_hide (sl->priv->is_toplevel_row);
      ctk_widget_hide (sl->priv->child_visible_row);
      ctk_widget_hide (sl->priv->frame_clock_row);
    }

  if (CTK_IS_BUILDABLE (object))
    {
      ctk_widget_show (sl->priv->buildable_id_row);
    }
  else
    {
      ctk_widget_hide (sl->priv->buildable_id_row);
    }

  if (CTK_IS_WINDOW (object))
    {
      ctk_widget_show (sl->priv->default_widget_row);
      ctk_widget_show (sl->priv->focus_widget_row);

      g_signal_connect_object (object, "set-focus", G_CALLBACK (set_focus_cb), sl, G_CONNECT_AFTER);
    }
  else
    {
      ctk_widget_hide (sl->priv->default_widget_row);
      ctk_widget_hide (sl->priv->focus_widget_row);
    }

  if (GDK_IS_FRAME_CLOCK (object))
    {
      ctk_widget_show (sl->priv->framecount_row);
      ctk_widget_show (sl->priv->framerate_row);
    }
  else
    {
      ctk_widget_hide (sl->priv->framecount_row);
      ctk_widget_hide (sl->priv->framerate_row);
    }

  update_info (sl);
}

static void
ctk_inspector_misc_info_init (GtkInspectorMiscInfo *sl)
{
  sl->priv = ctk_inspector_misc_info_get_instance_private (sl);
  ctk_widget_init_template (CTK_WIDGET (sl));
}

static void
map (GtkWidget *widget)
{
  GtkInspectorMiscInfo *sl = CTK_INSPECTOR_MISC_INFO (widget);

  CTK_WIDGET_CLASS (ctk_inspector_misc_info_parent_class)->map (widget);

  sl->priv->update_source_id = gdk_threads_add_timeout_seconds (1, update_info, sl);
  update_info (sl);
}

static void
unmap (GtkWidget *widget)
{
  GtkInspectorMiscInfo *sl = CTK_INSPECTOR_MISC_INFO (widget);

  g_source_remove (sl->priv->update_source_id);
  sl->priv->update_source_id = 0;

  CTK_WIDGET_CLASS (ctk_inspector_misc_info_parent_class)->unmap (widget);
}

static void
get_property (GObject    *object,
              guint       param_id,
              GValue     *value,
              GParamSpec *pspec)
{
  GtkInspectorMiscInfo *sl = CTK_INSPECTOR_MISC_INFO (object);

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
  GtkInspectorMiscInfo *sl = CTK_INSPECTOR_MISC_INFO (object);

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
ctk_inspector_misc_info_class_init (GtkInspectorMiscInfoClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);

  object_class->get_property = get_property;
  object_class->set_property = set_property;

  widget_class->map = map;
  widget_class->unmap = unmap;

  g_object_class_install_property (object_class, PROP_OBJECT_TREE,
      g_param_spec_object ("object-tree", "Object Tree", "Object tree",
                           CTK_TYPE_WIDGET, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  ctk_widget_class_set_template_from_resource (widget_class, "/org/ctk/libctk/inspector/misc-info.ui");
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorMiscInfo, address);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorMiscInfo, refcount_row);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorMiscInfo, refcount);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorMiscInfo, state_row);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorMiscInfo, state);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorMiscInfo, buildable_id_row);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorMiscInfo, buildable_id);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorMiscInfo, default_widget_row);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorMiscInfo, default_widget);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorMiscInfo, default_widget_button);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorMiscInfo, focus_widget_row);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorMiscInfo, focus_widget);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorMiscInfo, focus_widget_button);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorMiscInfo, mnemonic_label_row);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorMiscInfo, mnemonic_label);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorMiscInfo, request_mode_row);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorMiscInfo, request_mode);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorMiscInfo, allocated_size_row);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorMiscInfo, allocated_size);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorMiscInfo, baseline_row);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorMiscInfo, baseline);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorMiscInfo, clip_area_row);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorMiscInfo, clip_area);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorMiscInfo, frame_clock_row);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorMiscInfo, frame_clock);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorMiscInfo, frame_clock_button);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorMiscInfo, tick_callback_row);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorMiscInfo, tick_callback);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorMiscInfo, framecount_row);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorMiscInfo, framecount);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorMiscInfo, framerate_row);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorMiscInfo, framerate);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorMiscInfo, accessible_role_row);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorMiscInfo, accessible_role);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorMiscInfo, accessible_name_row);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorMiscInfo, accessible_name);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorMiscInfo, accessible_description_row);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorMiscInfo, accessible_description);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorMiscInfo, mapped_row);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorMiscInfo, mapped);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorMiscInfo, realized_row);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorMiscInfo, realized);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorMiscInfo, is_toplevel_row);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorMiscInfo, is_toplevel);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorMiscInfo, child_visible_row);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorMiscInfo, child_visible);

  ctk_widget_class_bind_template_callback (widget_class, show_default_widget);
  ctk_widget_class_bind_template_callback (widget_class, show_focus_widget);
  ctk_widget_class_bind_template_callback (widget_class, show_frame_clock);
}

// vim: set et sw=2 ts=2:

