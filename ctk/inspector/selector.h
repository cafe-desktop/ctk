/*
 * Copyright (c) 2014 Red Hat, Inc.
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

#ifndef _CTK_INSPECTOR_SELECTOR_H_
#define _CTK_INSPECTOR_SELECTOR_H_

#include <gtk/gtkbox.h>

#define CTK_TYPE_INSPECTOR_SELECTOR            (ctk_inspector_selector_get_type())
#define CTK_INSPECTOR_SELECTOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), CTK_TYPE_INSPECTOR_SELECTOR, GtkInspectorSelector))
#define CTK_INSPECTOR_SELECTOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), CTK_TYPE_INSPECTOR_SELECTOR, GtkInspectorSelectorClass))
#define CTK_INSPECTOR_IS_SELECTOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), CTK_TYPE_INSPECTOR_SELECTOR))
#define CTK_INSPECTOR_IS_SELECTOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), CTK_TYPE_INSPECTOR_SELECTOR))
#define CTK_INSPECTOR_SELECTOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), CTK_TYPE_INSPECTOR_SELECTOR, GtkInspectorSelectorClass))


typedef struct _GtkInspectorSelectorPrivate GtkInspectorSelectorPrivate;

typedef struct _GtkInspectorSelector
{
  GtkBox parent;
  GtkInspectorSelectorPrivate *priv;
} GtkInspectorSelector;

typedef struct _GtkInspectorSelectorClass
{
  GtkBoxClass parent;
} GtkInspectorSelectorClass;

G_BEGIN_DECLS

GType      ctk_inspector_selector_get_type   (void);
void       ctk_inspector_selector_set_object (GtkInspectorSelector *oh,
                                              GObject              *object);

G_END_DECLS

#endif // _CTK_INSPECTOR_SELECTOR_H_

// vim: set et sw=2 ts=2:
