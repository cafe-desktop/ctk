/*
 * Copyright (c) 2008-2009  Christian Hammond
 * Copyright (c) 2008-2009  David Trowbridge
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
#ifndef _CTK_INSPECTOR_WINDOW_H_
#define _CTK_INSPECTOR_WINDOW_H_


#include <ctk/ctkwindow.h>

#define CTK_TYPE_INSPECTOR_WINDOW            (ctk_inspector_window_get_type())
#define CTK_INSPECTOR_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), CTK_TYPE_INSPECTOR_WINDOW, GtkInspectorWindow))
#define CTK_INSPECTOR_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), CTK_TYPE_INSPECTOR_WINDOW, GtkInspectorWindowClass))
#define CTK_INSPECTOR_IS_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), CTK_TYPE_INSPECTOR_WINDOW))
#define CTK_INSPECTOR_IS_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), CTK_TYPE_INSPECTOR_WINDOW))
#define CTK_INSPECTOR_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), CTK_TYPE_INSPECTOR_WINDOW, GtkInspectorWindowClass))


#define TREE_TEXT_SCALE 0.8
#define TREE_CHECKBOX_SIZE (gint)(0.8 * 13)

typedef struct
{
  GtkWindow parent;

  GtkWidget *top_stack;
  GtkWidget *object_stack;
  GtkWidget *button_stack;
  GtkWidget *object_tree;
  GtkWidget *object_id;
  GtkWidget *object_details;
  GtkWidget *object_buttons;
  GtkWidget *object_details_button;
  GtkWidget *select_object;
  GtkWidget *object_start_stack;
  GtkWidget *object_center_stack;
  GtkWidget *object_title;
  GtkWidget *prop_list;
  GtkWidget *child_prop_list;
  GtkWidget *selector;
  GtkWidget *signals_list;
  GtkWidget *style_prop_list;
  GtkWidget *classes_list;
  GtkWidget *widget_css_node_tree;
  GtkWidget *object_hierarchy;
  GtkWidget *size_groups;
  GtkWidget *data_list;
  GtkWidget *actions;
  GtkWidget *menu;
  GtkWidget *misc_info;
  GtkWidget *gestures;
  GtkWidget *magnifier;

  GtkWidget *invisible;
  GtkWidget *selected_widget;
  GtkWidget *flash_widget;

  GList *extra_pages;

  gboolean grabbed;

  gint flash_count;
  gint flash_cnx;

} GtkInspectorWindow;

typedef struct
{
  GtkWindowClass parent;
} GtkInspectorWindowClass;


G_BEGIN_DECLS

GType      ctk_inspector_window_get_type    (void);
GtkWidget *ctk_inspector_window_new         (void);

void       ctk_inspector_flash_widget       (GtkInspectorWindow *iw,
                                             GtkWidget          *widget);
void       ctk_inspector_start_highlight    (GtkWidget          *widget);
void       ctk_inspector_stop_highlight     (GtkWidget          *widget);

void       ctk_inspector_on_inspect         (GtkWidget          *widget,
                                             GtkInspectorWindow *iw);

void       ctk_inspector_window_select_widget_under_pointer (GtkInspectorWindow *iw);

void       ctk_inspector_window_rescan     (GtkWidget          *iw);

G_END_DECLS


#endif // _CTK_INSPECTOR_WINDOW_H_

// vim: set et sw=2 ts=2:
