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

#include <ctk/ctk.h>
//#include "ctktreemodel.h"


typedef gboolean (*RowPredicate) (CtkTreeModel *model,
                                  CtkTreeIter  *iter,
                                  gpointer      data);

typedef struct _CtkTreeWalk CtkTreeWalk;

CtkTreeWalk * ctk_tree_walk_new        (CtkTreeModel   *model,
                                        RowPredicate    predicate,
                                        gpointer        data,
                                        GDestroyNotify  destroy);

void          ctk_tree_walk_free       (CtkTreeWalk *walk);

void          ctk_tree_walk_reset      (CtkTreeWalk *walk,
                                        CtkTreeIter *iter);

gboolean      ctk_tree_walk_next_match (CtkTreeWalk *walk,
                                        gboolean     force_move,
                                        gboolean     backwards,
                                        CtkTreeIter *iter);

gboolean      ctk_tree_walk_get_position (CtkTreeWalk *walk,
                                          CtkTreeIter *iter);
