/* ctktreemodelrefcount.h
 * Copyright (C) 2011  Kristian Rietveld <kris@ctk.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __CTK_TREE_MODEL_REF_COUNT_H__
#define __CTK_TREE_MODEL_REF_COUNT_H__

#include <ctk/ctk.h>

G_BEGIN_DECLS

#define CTK_TYPE_TREE_MODEL_REF_COUNT              (ctk_tree_model_ref_count_get_type ())
#define CTK_TREE_MODEL_REF_COUNT(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_TREE_MODEL_REF_COUNT, GtkTreeModelRefCount))
#define CTK_TREE_MODEL_REF_COUNT_CLASS(vtable)     (G_TYPE_CHECK_CLASS_CAST ((vtable), CTK_TYPE_TREE_MODEL_REF_COUNT, GtkTreeModelRefCountClass))
#define CTK_IS_TREE_MODEL_REF_COUNT(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_TREE_MODEL_REF_COUNT))
#define CTK_IS_TREE_MODEL_REF_COUNT_CLASS(vtable)  (G_TYPE_CHECK_CLASS_TYPE ((vtable), CTK_TYPE_TREE_MODEL_REF_COUNT))
#define CTK_TREE_MODEL_REF_COUNT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_TREE_MODEL_REF_COUNT, GtkTreeModelRefCountClass))


typedef struct _GtkTreeModelRefCount        GtkTreeModelRefCount;
typedef struct _GtkTreeModelRefCountClass   GtkTreeModelRefCountClass;
typedef struct _GtkTreeModelRefCountPrivate GtkTreeModelRefCountPrivate;

struct _GtkTreeModelRefCount
{
  GtkTreeStore parent;

  /* < private > */
  GtkTreeModelRefCountPrivate *priv;
};

struct _GtkTreeModelRefCountClass
{
  GtkTreeStoreClass parent_class;
};


GType         ctk_tree_model_ref_count_get_type    (void) G_GNUC_CONST;
GtkTreeModel *ctk_tree_model_ref_count_new         (void);

void          ctk_tree_model_ref_count_dump        (GtkTreeModelRefCount *ref_model);
gboolean      ctk_tree_model_ref_count_check_level (GtkTreeModelRefCount *ref_model,
                                                    GtkTreeIter          *parent,
                                                    gint                  expected_ref_count,
                                                    gboolean              recurse,
                                                    gboolean              may_assert);
gboolean      ctk_tree_model_ref_count_check_node  (GtkTreeModelRefCount *ref_model,
                                                    GtkTreeIter          *iter,
                                                    gint                  expected_ref_count,
                                                    gboolean              may_assert);

/* A couple of helpers for the tests.  Since this model will never be used
 * outside of unit tests anyway, it is probably fine to have these here
 * without namespacing.
 */

static inline void
assert_entire_model_unreferenced (GtkTreeModelRefCount *ref_model)
{
  ctk_tree_model_ref_count_check_level (ref_model, NULL, 0, TRUE, TRUE);
}

static inline void
assert_root_level_unreferenced (GtkTreeModelRefCount *ref_model)
{
  ctk_tree_model_ref_count_check_level (ref_model, NULL, 0, FALSE, TRUE);
}

static inline void
assert_level_unreferenced (GtkTreeModelRefCount *ref_model,
                           GtkTreeIter          *iter)
{
  ctk_tree_model_ref_count_check_level (ref_model, iter, 0, FALSE, TRUE);
}

static inline void
assert_entire_model_referenced (GtkTreeModelRefCount *ref_model,
                                gint                  ref_count)
{
  ctk_tree_model_ref_count_check_level (ref_model, NULL, ref_count, TRUE, TRUE);
}

static inline void
assert_not_entire_model_referenced (GtkTreeModelRefCount *ref_model,
                                    gint                  ref_count)
{
  g_assert_cmpint (ctk_tree_model_ref_count_check_level (ref_model, NULL,
                                                         ref_count,
                                                         TRUE, FALSE),
                   ==, FALSE);
}

static inline void
assert_root_level_referenced (GtkTreeModelRefCount *ref_model,
                              gint                  ref_count)
{
  ctk_tree_model_ref_count_check_level (ref_model, NULL, ref_count,
                                        FALSE, TRUE);
}

static inline void
assert_level_referenced (GtkTreeModelRefCount *ref_model,
                         gint                  ref_count,
                         GtkTreeIter          *iter)
{
  ctk_tree_model_ref_count_check_level (ref_model, iter, ref_count,
                                        FALSE, TRUE);
}

static inline void
assert_node_ref_count (GtkTreeModelRefCount *ref_model,
                       GtkTreeIter          *iter,
                       gint                  ref_count)
{
  ctk_tree_model_ref_count_check_node (ref_model, iter, ref_count, TRUE);
}


#endif /* __CTK_TREE_MODEL_REF_COUNT_H__ */
