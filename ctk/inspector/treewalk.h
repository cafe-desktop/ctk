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


typedef gboolean (*RowPredicate) (GtkTreeModel *model,
                                  GtkTreeIter  *iter,
                                  gpointer      data);

typedef struct _GtkTreeWalk GtkTreeWalk;

GtkTreeWalk * ctk_tree_walk_new        (GtkTreeModel   *model,
                                        RowPredicate    predicate,
                                        gpointer        data,
                                        GDestroyNotify  destroy);

void          ctk_tree_walk_free       (GtkTreeWalk *walk);

void          ctk_tree_walk_reset      (GtkTreeWalk *walk,
                                        GtkTreeIter *iter);

gboolean      ctk_tree_walk_next_match (GtkTreeWalk *walk,
                                        gboolean     force_move,
                                        gboolean     backwards,
                                        GtkTreeIter *iter);

gboolean      ctk_tree_walk_get_position (GtkTreeWalk *walk,
                                          GtkTreeIter *iter);
