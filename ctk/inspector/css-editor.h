/*
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

#ifndef _CTK_INSPECTOR_CSS_EDITOR_H_
#define _CTK_INSPECTOR_CSS_EDITOR_H_

#include <ctk/ctkbox.h>

#define CTK_TYPE_INSPECTOR_CSS_EDITOR            (ctk_inspector_css_editor_get_type())
#define CTK_INSPECTOR_CSS_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), CTK_TYPE_INSPECTOR_CSS_EDITOR, CtkInspectorCssEditor))
#define CTK_INSPECTOR_CSS_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), CTK_TYPE_INSPECTOR_CSS_EDITOR, CtkInspectorCssEditorClass))
#define CTK_INSPECTOR_IS_CSS_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), CTK_TYPE_INSPECTOR_CSS_EDITOR))
#define CTK_INSPECTOR_IS_CSS_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), CTK_TYPE_INSPECTOR_CSS_EDITOR))
#define CTK_INSPECTOR_CSS_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), CTK_TYPE_INSPECTOR_CSS_EDITOR, CtkInspectorCssEditorClass))


typedef struct _CtkInspectorCssEditorPrivate CtkInspectorCssEditorPrivate;

typedef struct _CtkInspectorCssEditor
{
  CtkBox parent;
  CtkInspectorCssEditorPrivate *priv;
} CtkInspectorCssEditor;

typedef struct _CtkInspectorCssEditorClass
{
  CtkBoxClass parent;
} CtkInspectorCssEditorClass;

G_BEGIN_DECLS

GType      ctk_inspector_css_editor_get_type   (void);
void       ctk_inspector_css_editor_set_object (CtkInspectorCssEditor *ce,
                                                GObject               *object);

G_END_DECLS

#endif // _CTK_INSPECTOR_CSS_EDITOR_H_

// vim: set et sw=2 ts=2:
