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
#ifndef _CTK_INSPECTOR_OBJECT_TREE_H_
#define _CTK_INSPECTOR_OBJECT_TREE_H_

#include <ctk/ctkbox.h>
#include <ctk/ctktreemodel.h>

#define CTK_TYPE_INSPECTOR_OBJECT_TREE            (ctk_inspector_object_tree_get_type())
#define CTK_INSPECTOR_OBJECT_TREE(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), CTK_TYPE_INSPECTOR_OBJECT_TREE, CtkInspectorObjectTree))
#define CTK_INSPECTOR_OBJECT_TREE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), CTK_TYPE_INSPECTOR_OBJECT_TREE, CtkInspectorObjectTreeClass))
#define CTK_INSPECTOR_IS_OBJECT_TREE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), CTK_TYPE_INSPECTOR_OBJECT_TREE))
#define CTK_INSPECTOR_IS_OBJECT_TREE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), CTK_TYPE_INSPECTOR_OBJECT_TREE))
#define CTK_INSPECTOR_OBJECT_TREE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), CTK_TYPE_INSPECTOR_OBJECT_TREE, CtkInspectorObjectTreeClass))


typedef struct _CtkInspectorObjectTreePrivate CtkInspectorObjectTreePrivate;

typedef struct _CtkInspectorObjectTree
{
  CtkBox parent;
  CtkInspectorObjectTreePrivate *priv;
} CtkInspectorObjectTree;

typedef struct _CtkInspectorObjectTreeClass
{
  CtkBoxClass parent;

  void (*object_selected)  (CtkInspectorObjectTree *wt,
                            GObject                *object);
  void (*object_activated) (CtkInspectorObjectTree *wt,
                            GObject                *object);
} CtkInspectorObjectTreeClass;


G_BEGIN_DECLS


GType      ctk_inspector_object_tree_get_type            (void);

void       ctk_inspector_object_tree_scan                (CtkInspectorObjectTree *wt,
                                                          CtkWidget              *window);
gboolean   ctk_inspector_object_tree_select_object       (CtkInspectorObjectTree *wt,
                                                          GObject                *object);
void       ctk_inspector_object_tree_append_object       (CtkInspectorObjectTree *wt,
                                                          GObject                *object,
                                                          CtkTreeIter            *parent_iter,
                                                          const gchar            *name);
gboolean   ctk_inspector_object_tree_find_object         (CtkInspectorObjectTree *wt,
                                                          GObject                *object,
                                                          CtkTreeIter            *iter);

GObject   *ctk_inspector_object_tree_get_selected        (CtkInspectorObjectTree *wt);

G_END_DECLS


#endif // _CTK_INSPECTOR_WIDGETTREE_H_

// vim: set et sw=2 ts=2:
