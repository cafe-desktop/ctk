/*
 * Copyright (c) 2008-2009  Christian Hammond
 * Copyright (c) 2008-2009  David Trowbridge
 * Copyright (c) 2013 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#include <stdlib.h>

#include "window.h"
#include "prop-list.h"
#include "css-editor.h"
#include "css-node-tree.h"
#include "object-hierarchy.h"
#include "object-tree.h"
#include "selector.h"
#include "size-groups.h"
#include "data-list.h"
#include "signals-list.h"
#include "actions.h"
#include "menu.h"
#include "misc-info.h"
#include "gestures.h"
#include "magnifier.h"

#include "ctklabel.h"
#include "ctkbutton.h"
#include "ctkstack.h"
#include "ctktreeviewcolumn.h"
#include "ctkmodulesprivate.h"
#include "ctkwindow.h"
#include "ctkwindowgroup.h"

G_DEFINE_TYPE (GtkInspectorWindow, ctk_inspector_window, CTK_TYPE_WINDOW)

static gboolean
set_selected_object (GtkInspectorWindow *iw,
                     GObject            *selected)
{
  GList *l;
  const char *title;

  if (!ctk_inspector_prop_list_set_object (CTK_INSPECTOR_PROP_LIST (iw->prop_list), selected))
    return FALSE;

  title = (const char *)g_object_get_data (selected, "ctk-inspector-object-title");
  ctk_label_set_label (CTK_LABEL (iw->object_title), title);

  ctk_inspector_prop_list_set_object (CTK_INSPECTOR_PROP_LIST (iw->child_prop_list), selected);
  ctk_inspector_signals_list_set_object (CTK_INSPECTOR_SIGNALS_LIST (iw->signals_list), selected);
  ctk_inspector_object_hierarchy_set_object (CTK_INSPECTOR_OBJECT_HIERARCHY (iw->object_hierarchy), selected);
  ctk_inspector_selector_set_object (CTK_INSPECTOR_SELECTOR (iw->selector), selected);
  ctk_inspector_misc_info_set_object (CTK_INSPECTOR_MISC_INFO (iw->misc_info), selected);
  ctk_inspector_css_node_tree_set_object (CTK_INSPECTOR_CSS_NODE_TREE (iw->widget_css_node_tree), selected);
  ctk_inspector_size_groups_set_object (CTK_INSPECTOR_SIZE_GROUPS (iw->size_groups), selected);
  ctk_inspector_data_list_set_object (CTK_INSPECTOR_DATA_LIST (iw->data_list), selected);
  ctk_inspector_actions_set_object (CTK_INSPECTOR_ACTIONS (iw->actions), selected);
  ctk_inspector_menu_set_object (CTK_INSPECTOR_MENU (iw->menu), selected);
  ctk_inspector_gestures_set_object (CTK_INSPECTOR_GESTURES (iw->gestures), selected);
  ctk_inspector_magnifier_set_object (CTK_INSPECTOR_MAGNIFIER (iw->magnifier), selected);

  for (l = iw->extra_pages; l != NULL; l = l->next)
    g_object_set (l->data, "object", selected, NULL);

  return TRUE;
}

static void
on_object_activated (GtkInspectorObjectTree *wt,
                     GObject                *selected,
                     const gchar            *name,
                     GtkInspectorWindow     *iw)
{
  const gchar *tab;

  if (!set_selected_object (iw, selected))
    return;

  tab = g_object_get_data (G_OBJECT (wt), "next-tab");
  if (tab)
    ctk_stack_set_visible_child_name (CTK_STACK (iw->object_details), tab);

  ctk_stack_set_visible_child_name (CTK_STACK (iw->object_stack), "object-details");
  ctk_stack_set_visible_child_name (CTK_STACK (iw->object_buttons), "details");
}

static void
on_object_selected (GtkInspectorObjectTree *wt,
                    GObject                *selected,
                    GtkInspectorWindow     *iw)
{
  ctk_widget_set_sensitive (iw->object_details_button, selected != NULL);
  if (CTK_IS_WIDGET (selected))
    ctk_inspector_flash_widget (iw, CTK_WIDGET (selected));
}

static void
close_object_details (GtkWidget *button, GtkInspectorWindow *iw)
{
  ctk_stack_set_visible_child_name (CTK_STACK (iw->object_stack), "object-tree");
  ctk_stack_set_visible_child_name (CTK_STACK (iw->object_buttons), "list");
}

static void
open_object_details (GtkWidget *button, GtkInspectorWindow *iw)
{
  GObject *selected;

  selected = ctk_inspector_object_tree_get_selected (CTK_INSPECTOR_OBJECT_TREE (iw->object_tree));
 
  if (!set_selected_object (iw, selected))
    return;

  ctk_stack_set_visible_child_name (CTK_STACK (iw->object_stack), "object-details");
  ctk_stack_set_visible_child_name (CTK_STACK (iw->object_buttons), "details");
}

static gboolean
translate_visible_child_name (GBinding     *binding,
                              const GValue *from,
                              GValue       *to,
                              gpointer      user_data)
{
  GtkInspectorWindow *iw = user_data;
  const char *name;

  name = g_value_get_string (from);

  if (ctk_stack_get_child_by_name (CTK_STACK (iw->object_start_stack), name))
    g_value_set_string (to, name);
  else
    g_value_set_string (to, "empty");

  return TRUE;
}

static void
ctk_inspector_window_init (GtkInspectorWindow *iw)
{
  GIOExtensionPoint *extension_point;
  GList *l, *extensions;

  ctk_widget_init_template (CTK_WIDGET (iw));

  g_object_bind_property_full (iw->object_details, "visible-child-name",
                               iw->object_start_stack, "visible-child-name",
                               G_BINDING_SYNC_CREATE,
                               translate_visible_child_name,
                               NULL,
                               iw,
                               NULL);

  ctk_window_group_add_window (ctk_window_group_new (), CTK_WINDOW (iw));

  extension_point = g_io_extension_point_lookup ("ctk-inspector-page");
  extensions = g_io_extension_point_get_extensions (extension_point);

  for (l = extensions; l != NULL; l = l->next)
    {
      GIOExtension *extension = l->data;
      GType type;
      GtkWidget *widget;
      const char *name;
      char *title;
      GtkWidget *button;
      gboolean use_picker;

      type = g_io_extension_get_type (extension);

      widget = g_object_new (type, NULL);

      iw->extra_pages = g_list_prepend (iw->extra_pages, widget);

      name = g_io_extension_get_name (extension);
      g_object_get (widget, "title", &title, NULL);

      if (g_object_class_find_property (G_OBJECT_GET_CLASS (widget), "use-picker"))
        g_object_get (widget, "use-picker", &use_picker, NULL);
      else
        use_picker = FALSE;

      if (use_picker)
        {
          button = ctk_button_new_from_icon_name ("find-location-symbolic",
                                                  CTK_ICON_SIZE_MENU);
          ctk_widget_set_focus_on_click (button, FALSE);
          ctk_widget_set_halign (button, CTK_ALIGN_START);
          ctk_widget_set_valign (button, CTK_ALIGN_CENTER);
          g_signal_connect (button, "clicked",
                            G_CALLBACK (ctk_inspector_on_inspect), iw);
        }
      else
        button = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);

      ctk_stack_add_titled (CTK_STACK (iw->top_stack), widget, name, title);
      ctk_stack_add_named (CTK_STACK (iw->button_stack), button, name);
      ctk_widget_show (widget);
      ctk_widget_show (button);

      g_free (title);
    }
}

static void
ctk_inspector_window_constructed (GObject *object)
{
  GtkInspectorWindow *iw = CTK_INSPECTOR_WINDOW (object);

  G_OBJECT_CLASS (ctk_inspector_window_parent_class)->constructed (object);

  ctk_inspector_object_tree_scan (CTK_INSPECTOR_OBJECT_TREE (iw->object_tree), NULL);
}

static void
object_details_changed (GtkWidget          *combo,
                        GParamSpec         *pspec,
                        GtkInspectorWindow *iw)
{
  ctk_stack_set_visible_child_name (CTK_STACK (iw->object_center_stack), "title");
}

static void
ctk_inspector_window_class_init (GtkInspectorWindowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);

  object_class->constructed = ctk_inspector_window_constructed;

  ctk_widget_class_set_template_from_resource (widget_class, "/org/ctk/libctk/inspector/window.ui");

  ctk_widget_class_bind_template_child (widget_class, GtkInspectorWindow, top_stack);
  ctk_widget_class_bind_template_child (widget_class, GtkInspectorWindow, button_stack);
  ctk_widget_class_bind_template_child (widget_class, GtkInspectorWindow, object_stack);
  ctk_widget_class_bind_template_child (widget_class, GtkInspectorWindow, object_tree);
  ctk_widget_class_bind_template_child (widget_class, GtkInspectorWindow, object_details);
  ctk_widget_class_bind_template_child (widget_class, GtkInspectorWindow, object_start_stack);
  ctk_widget_class_bind_template_child (widget_class, GtkInspectorWindow, object_center_stack);
  ctk_widget_class_bind_template_child (widget_class, GtkInspectorWindow, object_buttons);
  ctk_widget_class_bind_template_child (widget_class, GtkInspectorWindow, object_details_button);
  ctk_widget_class_bind_template_child (widget_class, GtkInspectorWindow, select_object);
  ctk_widget_class_bind_template_child (widget_class, GtkInspectorWindow, prop_list);
  ctk_widget_class_bind_template_child (widget_class, GtkInspectorWindow, child_prop_list);
  ctk_widget_class_bind_template_child (widget_class, GtkInspectorWindow, signals_list);
  ctk_widget_class_bind_template_child (widget_class, GtkInspectorWindow, widget_css_node_tree);
  ctk_widget_class_bind_template_child (widget_class, GtkInspectorWindow, object_hierarchy);
  ctk_widget_class_bind_template_child (widget_class, GtkInspectorWindow, object_title);
  ctk_widget_class_bind_template_child (widget_class, GtkInspectorWindow, selector);
  ctk_widget_class_bind_template_child (widget_class, GtkInspectorWindow, size_groups);
  ctk_widget_class_bind_template_child (widget_class, GtkInspectorWindow, data_list);
  ctk_widget_class_bind_template_child (widget_class, GtkInspectorWindow, actions);
  ctk_widget_class_bind_template_child (widget_class, GtkInspectorWindow, menu);
  ctk_widget_class_bind_template_child (widget_class, GtkInspectorWindow, misc_info);
  ctk_widget_class_bind_template_child (widget_class, GtkInspectorWindow, gestures);
  ctk_widget_class_bind_template_child (widget_class, GtkInspectorWindow, magnifier);

  ctk_widget_class_bind_template_callback (widget_class, ctk_inspector_on_inspect);
  ctk_widget_class_bind_template_callback (widget_class, on_object_activated);
  ctk_widget_class_bind_template_callback (widget_class, on_object_selected);
  ctk_widget_class_bind_template_callback (widget_class, open_object_details);
  ctk_widget_class_bind_template_callback (widget_class, close_object_details);
  ctk_widget_class_bind_template_callback (widget_class, object_details_changed);
}

static GdkScreen *
get_inspector_screen (void)
{
  static GdkDisplay *display = NULL;

  if (display == NULL)
    {
      const gchar *name;

      name = g_getenv ("CTK_INSPECTOR_DISPLAY");
      display = gdk_display_open (name);

      if (display)
        g_debug ("Using display %s for GtkInspector", name);
      else
        g_message ("Failed to open display %s", name);
    }

  if (!display)
    {
      display = gdk_display_open (NULL);
      if (display)
        g_debug ("Using default display for GtkInspector");
      else
        g_message ("Failed to separate connection to default display");
    }

  if (!display)
    display = gdk_display_get_default ();

  return gdk_display_get_default_screen (display);
}

GtkWidget *
ctk_inspector_window_new (void)
{
  return CTK_WIDGET (g_object_new (CTK_TYPE_INSPECTOR_WINDOW,
                                   "screen", get_inspector_screen (),
                                   NULL));
}

void
ctk_inspector_window_rescan (GtkWidget *widget)
{
  GtkInspectorWindow *iw = CTK_INSPECTOR_WINDOW (widget);

  ctk_inspector_object_tree_scan (CTK_INSPECTOR_OBJECT_TREE (iw->object_tree), NULL);
}

// vim: set et sw=2 ts=2:
