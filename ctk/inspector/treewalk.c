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

#include "treewalk.h"

struct _CtkTreeWalk
{
  CtkTreeModel *model;
  CtkTreeIter position;
  gboolean visited;
  RowPredicate predicate;
  gpointer data;
  GDestroyNotify destroy;
};

CtkTreeWalk *
ctk_tree_walk_new (CtkTreeModel   *model,
                   RowPredicate    predicate,
                   gpointer        data,
                   GDestroyNotify  destroy)
{
  CtkTreeWalk *walk;

  walk = g_new (CtkTreeWalk, 1);
  walk->model = g_object_ref (model);
  walk->visited = FALSE;
  walk->predicate = predicate;
  walk->data = data;
  walk->destroy = destroy;

  return walk;
}

void
ctk_tree_walk_free (CtkTreeWalk *walk)
{
  g_object_unref (walk->model);

  if (walk->destroy)
    walk->destroy (walk->data);

  g_free (walk);
}

void
ctk_tree_walk_reset (CtkTreeWalk *walk,
                     CtkTreeIter *iter)
{
  if (iter)
    {
      walk->position = *iter;
      walk->visited = TRUE;
    }
  else
    {
      walk->visited = FALSE;
    }
}

static gboolean
ctk_tree_walk_step_forward (CtkTreeWalk *walk)
{
  CtkTreeIter next, up;

  if (!walk->visited)
    {
      if (!ctk_tree_model_get_iter_first (walk->model, &walk->position))
        return FALSE;

      walk->visited = TRUE;
      return TRUE;
    }

  if (ctk_tree_model_iter_children (walk->model, &next, &walk->position))
    {
      walk->position = next;
      return TRUE;
    }

  next = walk->position;
  do
    {
      up = next;
      if (ctk_tree_model_iter_next (walk->model, &next))
        {
          walk->position = next;
          return TRUE;
        }
    }
  while (ctk_tree_model_iter_parent (walk->model, &next, &up));

  return FALSE;
}

static gboolean
ctk_tree_model_iter_last_child (CtkTreeModel *model,
                                CtkTreeIter  *iter,
                                CtkTreeIter  *parent)
{
  CtkTreeIter next;

  if (!ctk_tree_model_iter_children (model, &next, parent))
    return FALSE;

  do 
    *iter = next;
  while (ctk_tree_model_iter_next (model, &next));

  return TRUE;
}

static gboolean
ctk_tree_model_get_iter_last (CtkTreeModel *model,
                              CtkTreeIter  *iter)
{
  CtkTreeIter next;

  if (!ctk_tree_model_iter_last_child (model, &next, NULL))
    return FALSE;

  do
    *iter = next;
  while (ctk_tree_model_iter_last_child (model, &next, &next));

  return TRUE;
}

static gboolean
ctk_tree_walk_step_back (CtkTreeWalk *walk)
{
  CtkTreeIter previous, down;

  if (!walk->visited)
    {
      if (!ctk_tree_model_get_iter_last (walk->model, &walk->position))
        return FALSE;

      walk->visited = TRUE;
      return TRUE;
    }

  previous = walk->position;
  if (ctk_tree_model_iter_previous (walk->model, &previous))
    {
      while (ctk_tree_model_iter_last_child (walk->model, &down, &previous))
        previous = down;

      walk->position = previous;
      return TRUE; 
    }

  if (ctk_tree_model_iter_parent (walk->model, &previous, &walk->position))
    {
      walk->position = previous;
      return TRUE; 
    }

  return FALSE;
}

static gboolean
ctk_tree_walk_step (CtkTreeWalk *walk, gboolean backwards)
{
  if (backwards)
    return ctk_tree_walk_step_back (walk);
  else
    return ctk_tree_walk_step_forward (walk);
}

static gboolean
row_is_match (CtkTreeWalk *walk)
{
  if (walk->predicate)
    return walk->predicate (walk->model, &walk->position, walk->data);
  return TRUE;
}

gboolean
ctk_tree_walk_next_match (CtkTreeWalk *walk,
                          gboolean     force_move,
                          gboolean     backwards,
                          CtkTreeIter *iter)
{
  gboolean moved = FALSE;
  gboolean was_visited;
  CtkTreeIter position;

  was_visited = walk->visited;
  position = walk->position;

  do
    {
      if (moved || (!force_move && walk->visited))
        {
          if (row_is_match (walk))
            {
              *iter = walk->position;
              return TRUE;
            }
        }
      moved = TRUE;
    }
  while (ctk_tree_walk_step (walk, backwards));

  walk->visited = was_visited;
  walk->position = position;

  return FALSE;
}

gboolean
ctk_tree_walk_get_position (CtkTreeWalk *walk,
                            CtkTreeIter *iter)
{
  *iter = walk->position;
  return walk->visited;
}
