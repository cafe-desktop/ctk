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

#include <string.h>

#include "object-tree.h"
#include "prop-list.h"

#include "ctkbuildable.h"
#include "ctkbutton.h"
#include "ctkcelllayout.h"
#include "ctkcomboboxprivate.h"
#include "ctkiconview.h"
#include "ctklabel.h"
#include "ctkmenuitem.h"
#include "ctksettings.h"
#include "ctktextview.h"
#include "ctktreeview.h"
#include "ctktreeselection.h"
#include "ctktreestore.h"
#include "ctktreemodelsort.h"
#include "ctktreemodelfilter.h"
#include "ctkwidgetprivate.h"
#include "ctkstylecontext.h"
#include "ctksearchbar.h"
#include "ctksearchentry.h"
#include "treewalk.h"

enum
{
  OBJECT,
  OBJECT_TYPE,
  OBJECT_NAME,
  OBJECT_LABEL,
  OBJECT_CLASSES,
  SENSITIVE
};


enum
{
  OBJECT_SELECTED,
  OBJECT_ACTIVATED,
  LAST_SIGNAL
};


struct _CtkInspectorObjectTreePrivate
{
  CtkTreeView *tree;
  CtkTreeStore *model;
  gulong map_hook;
  gulong unmap_hook;
  CtkTreeViewColumn *object_column;
  CtkWidget *search_bar;
  CtkWidget *search_entry;
  CtkTreeWalk *walk;
  gint search_length;
};

typedef struct _ObjectTreeClassFuncs ObjectTreeClassFuncs;
typedef void (* ObjectTreeForallFunc) (GObject    *object,
                                       const char *name,
                                       gpointer    data);

struct _ObjectTreeClassFuncs {
  GType         (* get_type)            (void);
  GObject *     (* get_parent)          (GObject                *object);
  void          (* forall)              (GObject                *object,
                                         ObjectTreeForallFunc    forall_func,
                                         gpointer                forall_data);
  gboolean      (* get_sensitive)       (GObject                *object);
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE (CtkInspectorObjectTree, ctk_inspector_object_tree, CTK_TYPE_BOX)

static GObject *
object_tree_get_parent_default (GObject *object)
{
  return g_object_get_data (object, "inspector-object-tree-parent");
}

static void
object_tree_forall_default (GObject              *object,
                            ObjectTreeForallFunc  forall_func,
                            gpointer              forall_data)
{
}

static gboolean
object_tree_get_sensitive_default (GObject *object)
{
  return TRUE;
}

static GObject *
object_tree_widget_get_parent (GObject *object)
{
  return G_OBJECT (ctk_widget_get_parent (CTK_WIDGET (object)));
}

static void
object_tree_widget_forall (GObject              *object,
                           ObjectTreeForallFunc  forall_func,
                           gpointer              forall_data)
{
  struct {
    CtkPropagationPhase  phase;
    const gchar         *name;
  } phases[] = {
    { CTK_PHASE_CAPTURE, "capture" },
    { CTK_PHASE_TARGET,  "target" },
    { CTK_PHASE_BUBBLE,  "bubble" },
    { CTK_PHASE_NONE,    "" }
  };
  gint i;

  for (i = 0; i < G_N_ELEMENTS (phases); i++)
    {
      GList *list, *l;

      list = _ctk_widget_list_controllers (CTK_WIDGET (object), phases[i].phase);
      for (l = list; l; l = l->next)
        {
          GObject *controller = l->data;
          forall_func (controller, phases[i].name, forall_data);
        }
      g_list_free (list);
    }

   if (ctk_widget_is_toplevel (CTK_WIDGET (object)))
     {
       GObject *clock;

       clock = G_OBJECT (ctk_widget_get_frame_clock (CTK_WIDGET (object)));
       if (clock)
         forall_func (clock, "frame-clock", forall_data);
     }
}

static gboolean
object_tree_widget_get_sensitive (GObject *object)
{
  return ctk_widget_get_mapped (CTK_WIDGET (object));
}

typedef struct {
  ObjectTreeForallFunc forall_func;
  gpointer             forall_data;
} ForallData;

static void
container_children_callback (CtkWidget *widget,
                             gpointer   client_data)
{
  ForallData *forall_data = client_data;

  forall_data->forall_func (G_OBJECT (widget), NULL, forall_data->forall_data);
}

static void
object_tree_container_forall (GObject              *object,
                              ObjectTreeForallFunc  forall_func,
                              gpointer              forall_data)
{
  ForallData data = {
    forall_func,
    forall_data
  };

  ctk_container_forall (CTK_CONTAINER (object),
                        container_children_callback,
                        &data);
}

static void
object_tree_tree_model_sort_forall (GObject              *object,
                                    ObjectTreeForallFunc  forall_func,
                                    gpointer              forall_data)
{
  GObject *child = G_OBJECT (ctk_tree_model_sort_get_model (CTK_TREE_MODEL_SORT (object)));

  if (child)
    forall_func (child, "model", forall_data);
}

static void
object_tree_tree_model_filter_forall (GObject              *object,
                                      ObjectTreeForallFunc  forall_func,
                                      gpointer              forall_data)
{
  GObject *child = G_OBJECT (ctk_tree_model_filter_get_model (CTK_TREE_MODEL_FILTER (object)));

  if (child)
    forall_func (child, "model", forall_data);
}

static void
object_tree_menu_item_forall (GObject              *object,
                              ObjectTreeForallFunc  forall_func,
                              gpointer              forall_data)
{
  CtkWidget *submenu;

  submenu = ctk_menu_item_get_submenu (CTK_MENU_ITEM (object));
  if (submenu)
    forall_func (G_OBJECT (submenu), "submenu", forall_data);
}

static void
object_tree_combo_box_forall (GObject              *object,
                              ObjectTreeForallFunc  forall_func,
                              gpointer              forall_data)
{
  CtkWidget *popup;
  GObject *child;

  popup = ctk_combo_box_get_popup (CTK_COMBO_BOX (object));
  if (popup)
    forall_func (G_OBJECT (popup), "popup", forall_data);

  child = G_OBJECT (ctk_combo_box_get_model (CTK_COMBO_BOX (object)));
  if (child)
    forall_func (child, "model", forall_data);
}

static void
object_tree_tree_view_forall (GObject              *object,
                              ObjectTreeForallFunc  forall_func,
                              gpointer              forall_data)
{
  gint n_columns, i;
  GObject *child;

  child = G_OBJECT (ctk_tree_view_get_model (CTK_TREE_VIEW (object)));
  if (child)
    forall_func (child, "model", forall_data);

  child = G_OBJECT (ctk_tree_view_get_selection (CTK_TREE_VIEW (object)));
  if (child)
    forall_func (child, "selection", forall_data);

  n_columns = ctk_tree_view_get_n_columns (CTK_TREE_VIEW (object));
  for (i = 0; i < n_columns; i++)
    {
      child = G_OBJECT (ctk_tree_view_get_column (CTK_TREE_VIEW (object), i));
      forall_func (child, NULL, forall_data);
    }
}

static void
object_tree_icon_view_forall (GObject              *object,
                              ObjectTreeForallFunc  forall_func,
                              gpointer              forall_data)
{
  GObject *child;

  child = G_OBJECT (ctk_icon_view_get_model (CTK_ICON_VIEW (object)));
  if (child)
    forall_func (child, "model", forall_data);
}

typedef struct {
  ObjectTreeForallFunc forall_func;
  gpointer forall_data;
  GObject *parent;
} ParentForallData;

static gboolean
cell_callback (CtkCellRenderer *renderer,
               gpointer         data)
{
  ParentForallData *d = data;
  gpointer cell_layout;

  cell_layout = g_object_get_data (d->parent, "ctk-inspector-cell-layout");
  g_object_set_data (G_OBJECT (renderer), "ctk-inspector-cell-layout", cell_layout);
  d->forall_func (G_OBJECT (renderer), NULL, d->forall_data);

  return FALSE;
}

static void
object_tree_cell_area_forall (GObject              *object,
                              ObjectTreeForallFunc  forall_func,
                              gpointer              forall_data)
{
  ParentForallData data = {
    forall_func,
    forall_data,
    object
  };

  ctk_cell_area_foreach (CTK_CELL_AREA (object), cell_callback, &data);
}

static void
object_tree_cell_layout_forall (GObject              *object,
                                ObjectTreeForallFunc  forall_func,
                                gpointer              forall_data)
{
  CtkCellArea *area;

  /* cell areas handle their own stuff */
  if (CTK_IS_CELL_AREA (object))
    return;

  area = ctk_cell_layout_get_area (CTK_CELL_LAYOUT (object));
  if (!area)
    return;

  g_object_set_data (G_OBJECT (area), "ctk-inspector-cell-layout", object);
  forall_func (G_OBJECT (area), "cell-area", forall_data);
}

static void
object_tree_text_view_forall (GObject              *object,
                              ObjectTreeForallFunc  forall_func,
                              gpointer              forall_data)
{
  CtkTextBuffer *buffer;

  buffer = ctk_text_view_get_buffer (CTK_TEXT_VIEW (object));
  forall_func (G_OBJECT (buffer), "buffer", forall_data);
}

static void
object_tree_text_buffer_forall (GObject              *object,
                                ObjectTreeForallFunc  forall_func,
                                gpointer              forall_data)
{
  CtkTextTagTable *tags;

  tags = ctk_text_buffer_get_tag_table (CTK_TEXT_BUFFER (object));
  forall_func (G_OBJECT (tags), "tag-table", forall_data);
}

static void
tag_callback (CtkTextTag *tag,
              gpointer    data)
{
  ForallData *d = data;
  gchar *name;

  g_object_get (tag, "name", &name, NULL);
  d->forall_func (G_OBJECT (tag), name, d->forall_data);
  g_free (name);
}

static void
object_tree_text_tag_table_forall (GObject              *object,
                                   ObjectTreeForallFunc  forall_func,
                                   gpointer              forall_data)
{
  ForallData data = {
    forall_func,
    forall_data
  };

  ctk_text_tag_table_foreach (CTK_TEXT_TAG_TABLE (object), tag_callback, &data);
}

static void
object_tree_application_forall (GObject              *object,
                                ObjectTreeForallFunc  forall_func,
                                gpointer              forall_data)
{
  GObject *menu;

  menu = (GObject *)ctk_application_get_app_menu (CTK_APPLICATION (object));
  if (menu)
    forall_func (menu, "app-menu", forall_data);

  menu = (GObject *)ctk_application_get_menubar (CTK_APPLICATION (object));
  if (menu)
    forall_func (menu, "menubar", forall_data);
}

/* Note:
 * This tree must be sorted with the most specific types first.
 * We iterate over it top to bottom and return the first match
 * using g_type_is_a ()
 */
static const ObjectTreeClassFuncs object_tree_class_funcs[] = {
  {
    ctk_application_get_type,
    object_tree_get_parent_default,
    object_tree_application_forall,
    object_tree_get_sensitive_default
  },
  {
    ctk_text_tag_table_get_type,
    object_tree_get_parent_default,
    object_tree_text_tag_table_forall,
    object_tree_get_sensitive_default
  },
  {
    ctk_text_buffer_get_type,
    object_tree_get_parent_default,
    object_tree_text_buffer_forall,
    object_tree_get_sensitive_default
  },
  {
    ctk_text_view_get_type,
    object_tree_widget_get_parent,
    object_tree_text_view_forall,
    object_tree_widget_get_sensitive
  },
  {
    ctk_icon_view_get_type,
    object_tree_widget_get_parent,
    object_tree_icon_view_forall,
    object_tree_widget_get_sensitive
  },
  {
    ctk_tree_view_get_type,
    object_tree_widget_get_parent,
    object_tree_tree_view_forall,
    object_tree_widget_get_sensitive
  },
  {
    ctk_combo_box_get_type,
    object_tree_widget_get_parent,
    object_tree_combo_box_forall,
    object_tree_widget_get_sensitive
  },
  {
    ctk_menu_item_get_type,
    object_tree_widget_get_parent,
    object_tree_menu_item_forall,
    object_tree_widget_get_sensitive
  },
  {
    ctk_container_get_type,
    object_tree_widget_get_parent,
    object_tree_container_forall,
    object_tree_widget_get_sensitive
  },
  {
    ctk_widget_get_type,
    object_tree_widget_get_parent,
    object_tree_widget_forall,
    object_tree_widget_get_sensitive
  },
  {
    ctk_tree_model_filter_get_type,
    object_tree_get_parent_default,
    object_tree_tree_model_filter_forall,
    object_tree_get_sensitive_default
  },
  {
    ctk_tree_model_sort_get_type,
    object_tree_get_parent_default,
    object_tree_tree_model_sort_forall,
    object_tree_get_sensitive_default
  },
  {
    ctk_cell_area_get_type,
    object_tree_get_parent_default,
    object_tree_cell_area_forall,
    object_tree_get_sensitive_default
  },
  {
    ctk_cell_layout_get_type,
    object_tree_get_parent_default,
    object_tree_cell_layout_forall,
    object_tree_get_sensitive_default
  },
  {
    g_object_get_type,
    object_tree_get_parent_default,
    object_tree_forall_default,
    object_tree_get_sensitive_default
  },
};

static const ObjectTreeClassFuncs *
find_class_funcs (GObject *object)
{
  GType object_type;
  guint i;

  object_type = G_OBJECT_TYPE (object);

  for (i = 0; i < G_N_ELEMENTS (object_tree_class_funcs); i++)
    {
      if (g_type_is_a (object_type, object_tree_class_funcs[i].get_type ()))
        return &object_tree_class_funcs[i];
    }

  g_assert_not_reached ();

  return NULL;
}

static GObject *
object_get_parent (GObject *object)
{
  const ObjectTreeClassFuncs *funcs;

  funcs = find_class_funcs (object);
  
  return funcs->get_parent (object);
}

static void
object_forall (GObject              *object,
               ObjectTreeForallFunc  forall_func,
               gpointer              forall_data)
{
  GType object_type;
  guint i;

  object_type = G_OBJECT_TYPE (object);

  for (i = 0; i < G_N_ELEMENTS (object_tree_class_funcs); i++)
    {
      if (g_type_is_a (object_type, object_tree_class_funcs[i].get_type ()))
        object_tree_class_funcs[i].forall (object, forall_func, forall_data);
    }
}

static gboolean
object_get_sensitive (GObject *object)
{
  const ObjectTreeClassFuncs *funcs;

  funcs = find_class_funcs (object);
  
  return funcs->get_sensitive (object);
}

static void
on_row_activated (CtkTreeView            *tree,
                  CtkTreePath            *path,
                  CtkTreeViewColumn      *col,
                  CtkInspectorObjectTree *wt)
{
  CtkTreeIter iter;
  GObject *object;
  gchar *name;

  ctk_tree_model_get_iter (CTK_TREE_MODEL (wt->priv->model), &iter, path);
  ctk_tree_model_get (CTK_TREE_MODEL (wt->priv->model), &iter,
                      OBJECT, &object,
                      OBJECT_NAME, &name,
                      -1);

  g_signal_emit (wt, signals[OBJECT_ACTIVATED], 0, object, name);

  g_free (name);
}

GObject *
ctk_inspector_object_tree_get_selected (CtkInspectorObjectTree *wt)
{
  GObject *object;
  CtkTreeIter iter;
  CtkTreeSelection *sel;
  CtkTreeModel *model;

  object = NULL;
  sel = ctk_tree_view_get_selection (CTK_TREE_VIEW (wt->priv->tree));
  if (ctk_tree_selection_get_selected (sel, &model, &iter))
    ctk_tree_model_get (model, &iter,
                        OBJECT, &object,
                        -1);

  return object;
}

static void
on_selection_changed (CtkTreeSelection       *selection,
                      CtkInspectorObjectTree *wt)
{
  GObject *object;
  CtkTreeIter iter;

  if (ctk_tree_selection_get_selected (selection, NULL, &iter))
    ctk_tree_walk_reset (wt->priv->walk, &iter);
  else
    ctk_tree_walk_reset (wt->priv->walk, NULL);
  object = ctk_inspector_object_tree_get_selected (wt);
  g_signal_emit (wt, signals[OBJECT_SELECTED], 0, object);
}

typedef struct {
  GObject *dead_object;
  CtkTreeWalk *walk;
  CtkTreePath *walk_pos;
} RemoveData;

static gboolean
remove_cb (CtkTreeModel *model,
           CtkTreePath  *path,
           CtkTreeIter  *iter,
           gpointer      data)
{
  RemoveData *remove_data = data;
  GObject *lookup;

  ctk_tree_model_get (model, iter, OBJECT, &lookup, -1);

  if (lookup == remove_data->dead_object)
    {
      if (remove_data->walk_pos != NULL &&
          ctk_tree_path_compare (path, remove_data->walk_pos) == 0)
        ctk_tree_walk_reset (remove_data->walk, NULL);

      ctk_tree_store_remove (CTK_TREE_STORE (model), iter);

      return TRUE;
    }

  return FALSE;
}

static void
ctk_object_tree_remove_dead_object (gpointer data, GObject *dead_object)
{
  CtkInspectorObjectTree *wt = data;
  CtkTreeIter iter;
  RemoveData remove_data;

  remove_data.dead_object = dead_object;
  remove_data.walk = wt->priv->walk;
  if (ctk_tree_walk_get_position (wt->priv->walk, &iter))
    remove_data.walk_pos = ctk_tree_model_get_path (CTK_TREE_MODEL (wt->priv->model), &iter);
  else
    remove_data.walk_pos = NULL;

  ctk_tree_model_foreach (CTK_TREE_MODEL (wt->priv->model), remove_cb, &remove_data);

  if (remove_data.walk_pos)
    ctk_tree_path_free (remove_data.walk_pos);
}

static gboolean
weak_unref_cb (CtkTreeModel *model,
               CtkTreePath  *path,
               CtkTreeIter  *iter,
               gpointer      data)
{
  CtkInspectorObjectTree *wt = data;
  GObject *object;

  ctk_tree_model_get (model, iter, OBJECT, &object, -1);

  g_object_weak_unref (object, ctk_object_tree_remove_dead_object, wt);

  return FALSE;
}

static void
clear_store (CtkInspectorObjectTree *wt)
{
  if (wt->priv->model)
    {
      ctk_tree_model_foreach (CTK_TREE_MODEL (wt->priv->model), weak_unref_cb, wt);
      ctk_tree_store_clear (wt->priv->model);
      ctk_tree_walk_reset (wt->priv->walk, NULL);
    }
}

static gboolean
map_or_unmap (GSignalInvocationHint *ihint,
              guint                  n_params,
              const GValue          *params,
              gpointer               data)
{
  CtkInspectorObjectTree *wt = data;
  CtkWidget *widget;
  CtkTreeIter iter;

  widget = g_value_get_object (params);
  if (ctk_inspector_object_tree_find_object (wt, G_OBJECT (widget), &iter))
    ctk_tree_store_set (wt->priv->model, &iter,
                        SENSITIVE, ctk_widget_get_mapped (widget),
                        -1);
  return TRUE;
}

static void
move_search_to_row (CtkInspectorObjectTree *wt,
                    CtkTreeIter            *iter)
{
  CtkTreeSelection *selection;
  CtkTreePath *path;

  selection = ctk_tree_view_get_selection (wt->priv->tree);
  path = ctk_tree_model_get_path (CTK_TREE_MODEL (wt->priv->model), iter);
  ctk_tree_view_expand_to_path (wt->priv->tree, path);
  ctk_tree_selection_select_path (selection, path);
  ctk_tree_view_scroll_to_cell (wt->priv->tree, path, NULL, TRUE, 0.5, 0.0);
  ctk_tree_path_free (path);
}

static gboolean
key_press_event (CtkWidget              *window,
                 CdkEvent               *event,
                 CtkInspectorObjectTree *wt)
{
  if (ctk_widget_get_mapped (CTK_WIDGET (wt)))
    {
      CdkModifierType default_accel;
      gboolean search_started;

      search_started = ctk_search_bar_get_search_mode (CTK_SEARCH_BAR (wt->priv->search_bar));
      default_accel = ctk_widget_get_modifier_mask (CTK_WIDGET (wt), CDK_MODIFIER_INTENT_PRIMARY_ACCELERATOR);

      if (search_started &&
          (event->key.keyval == CDK_KEY_Return ||
           event->key.keyval == CDK_KEY_ISO_Enter ||
           event->key.keyval == CDK_KEY_KP_Enter))
        {
          CtkTreeSelection *selection;
          CtkTreeModel *model;
          CtkTreeIter iter;

          selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (wt->priv->tree));
          if (ctk_tree_selection_get_selected (selection, &model, &iter))
            {
              CtkTreePath *path;

              path = ctk_tree_model_get_path (model, &iter);
              ctk_tree_view_row_activated (CTK_TREE_VIEW (wt->priv->tree),
                                           path,
                                           wt->priv->object_column);
              ctk_tree_path_free (path);

              return CDK_EVENT_STOP;
            }
          else
            return CDK_EVENT_PROPAGATE;
        }
      else if (search_started &&
               (event->key.keyval == CDK_KEY_Escape))
        {
          ctk_search_bar_set_search_mode (CTK_SEARCH_BAR (wt->priv->search_bar), FALSE);
          return CDK_EVENT_STOP;
        }
      else if (search_started &&
               ((event->key.state & (default_accel | CDK_SHIFT_MASK)) == (default_accel | CDK_SHIFT_MASK)) &&
               (event->key.keyval == CDK_KEY_g || event->key.keyval == CDK_KEY_G))
        {
          CtkTreeIter iter;
          if (ctk_tree_walk_next_match (wt->priv->walk, TRUE, TRUE, &iter))
            move_search_to_row (wt, &iter);
          else
            ctk_widget_error_bell (CTK_WIDGET (wt));

          return CDK_EVENT_STOP;
        }
      else if (search_started &&
               ((event->key.state & (default_accel | CDK_SHIFT_MASK)) == default_accel) &&
               (event->key.keyval == CDK_KEY_g || event->key.keyval == CDK_KEY_G))
        {
          CtkTreeIter iter;

          if (ctk_tree_walk_next_match (wt->priv->walk, TRUE, FALSE, &iter))
            move_search_to_row (wt, &iter);
          else
            ctk_widget_error_bell (CTK_WIDGET (wt));

          return CDK_EVENT_STOP;
        }

      return ctk_search_bar_handle_event (CTK_SEARCH_BAR (wt->priv->search_bar), event);
    }
  else
    return CDK_EVENT_PROPAGATE;
}

static void
on_hierarchy_changed (CtkWidget *widget,
                      CtkWidget *previous_toplevel)
{
  if (previous_toplevel)
    g_signal_handlers_disconnect_by_func (previous_toplevel, key_press_event, widget);
  g_signal_connect (ctk_widget_get_toplevel (widget), "key-press-event",
                    G_CALLBACK (key_press_event), widget);
}

static void
on_search_changed (CtkSearchEntry         *entry,
                   CtkInspectorObjectTree *wt)
{
  CtkTreeIter iter;
  gint length;
  gboolean backwards;

  length = strlen (ctk_entry_get_text (CTK_ENTRY (entry)));
  backwards = length < wt->priv->search_length;
  wt->priv->search_length = length;

  if (length == 0)
    return;

  if (ctk_tree_walk_next_match (wt->priv->walk, backwards, backwards, &iter))
    move_search_to_row (wt, &iter);
  else if (!backwards)
    ctk_widget_error_bell (CTK_WIDGET (wt));
}

static gboolean
match_string (const gchar *string,
              const gchar *text)
{
  gboolean match = FALSE;

  if (string)
    {
      gchar *lower;

      lower = g_ascii_strdown (string, -1);
      match = g_str_has_prefix (lower, text);
      g_free (lower);
    }

  return match;
}

static gboolean
match_row (CtkTreeModel *model,
           CtkTreeIter  *iter,
           gpointer      data)
{
  CtkInspectorObjectTree *wt = data;
  gchar *type, *name, *label;
  const gchar *text;
  gboolean match;

  text = ctk_entry_get_text (CTK_ENTRY (wt->priv->search_entry));
  ctk_tree_model_get (model, iter,
                      OBJECT_TYPE, &type,
                      OBJECT_NAME, &name,
                      OBJECT_LABEL, &label,
                      -1);

  match = (match_string (type, text) ||
           match_string (name, text) ||
           match_string (label, text));

  g_free (type);
  g_free (name);
  g_free (label);

  return match;
}

static void
search_mode_changed (GObject                *search_bar,
                     GParamSpec             *pspec,
                     CtkInspectorObjectTree *wt)
{
  if (!ctk_search_bar_get_search_mode (CTK_SEARCH_BAR (search_bar)))
    {
      ctk_tree_walk_reset (wt->priv->walk, NULL);
      wt->priv->search_length = 0;
    }
}

static void
next_match (CtkButton              *button,
            CtkInspectorObjectTree *wt)
{
  if (ctk_search_bar_get_search_mode (CTK_SEARCH_BAR (wt->priv->search_bar)))
    {
      CtkTreeIter iter;

      if (ctk_tree_walk_next_match (wt->priv->walk, TRUE, FALSE, &iter))
        move_search_to_row (wt, &iter);
      else
        ctk_widget_error_bell (CTK_WIDGET (wt));
    }
}

static void
previous_match (CtkButton              *button,
                CtkInspectorObjectTree *wt)
{
  if (ctk_search_bar_get_search_mode (CTK_SEARCH_BAR (wt->priv->search_bar)))
    {
      CtkTreeIter iter;

      if (ctk_tree_walk_next_match (wt->priv->walk, TRUE, TRUE, &iter))
        move_search_to_row (wt, &iter);
      else
        ctk_widget_error_bell (CTK_WIDGET (wt));
    }
}

static void
stop_search (CtkWidget              *entry,
             CtkInspectorObjectTree *wt)
{
  ctk_entry_set_text (CTK_ENTRY (wt->priv->search_entry), "");
  ctk_search_bar_set_search_mode (CTK_SEARCH_BAR (wt->priv->search_bar), FALSE);
}

static void
ctk_inspector_object_tree_init (CtkInspectorObjectTree *wt)
{
  guint signal_id;

  wt->priv = ctk_inspector_object_tree_get_instance_private (wt);
  ctk_widget_init_template (CTK_WIDGET (wt));

  ctk_search_bar_connect_entry (CTK_SEARCH_BAR (wt->priv->search_bar),
                                CTK_ENTRY (wt->priv->search_entry));

  g_signal_connect (wt->priv->search_bar, "notify::search-mode-enabled",
                    G_CALLBACK (search_mode_changed), wt);
  wt->priv->walk = ctk_tree_walk_new (CTK_TREE_MODEL (wt->priv->model), match_row, wt, NULL);

  signal_id = g_signal_lookup ("map", CTK_TYPE_WIDGET);
  wt->priv->map_hook = g_signal_add_emission_hook (signal_id, 0,
                                                   map_or_unmap, wt, NULL);
  signal_id = g_signal_lookup ("unmap", CTK_TYPE_WIDGET);
  wt->priv->unmap_hook = g_signal_add_emission_hook (signal_id, 0,
                                                   map_or_unmap, wt, NULL);

  ctk_inspector_object_tree_append_object (wt, G_OBJECT (ctk_settings_get_default ()), NULL, NULL);
}

static void
ctk_inspector_object_tree_dispose (GObject *object)
{
  CtkInspectorObjectTree *wt = CTK_INSPECTOR_OBJECT_TREE (object);

  clear_store (wt);

  G_OBJECT_CLASS (ctk_inspector_object_tree_parent_class)->dispose (object);
}

static void
ctk_inspector_object_tree_finalize (GObject *object)
{
  CtkInspectorObjectTree *wt = CTK_INSPECTOR_OBJECT_TREE (object);
  guint signal_id;

  signal_id = g_signal_lookup ("map", CTK_TYPE_WIDGET);
  g_signal_remove_emission_hook (signal_id, wt->priv->map_hook);
  signal_id = g_signal_lookup ("unmap", CTK_TYPE_WIDGET);
  g_signal_remove_emission_hook (signal_id, wt->priv->unmap_hook);

  ctk_tree_walk_free (wt->priv->walk);

  G_OBJECT_CLASS (ctk_inspector_object_tree_parent_class)->finalize (object);
}

static void
ctk_inspector_object_tree_class_init (CtkInspectorObjectTreeClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);

  object_class->finalize = ctk_inspector_object_tree_finalize;
  object_class->dispose = ctk_inspector_object_tree_dispose;

  signals[OBJECT_ACTIVATED] =
      g_signal_new ("object-activated",
                    G_OBJECT_CLASS_TYPE (klass),
                    G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE,
                    G_STRUCT_OFFSET (CtkInspectorObjectTreeClass, object_activated),
                    NULL, NULL,
                    NULL,
                    G_TYPE_NONE, 2, G_TYPE_OBJECT, G_TYPE_STRING);

  signals[OBJECT_SELECTED] =
      g_signal_new ("object-selected",
                    G_OBJECT_CLASS_TYPE (klass),
                    G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE,
                    G_STRUCT_OFFSET (CtkInspectorObjectTreeClass, object_selected),
                    NULL, NULL,
                    NULL,
                    G_TYPE_NONE, 1, G_TYPE_OBJECT);

  ctk_widget_class_set_template_from_resource (widget_class, "/org/ctk/libctk/inspector/object-tree.ui");
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorObjectTree, model);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorObjectTree, tree);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorObjectTree, object_column);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorObjectTree, search_bar);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorObjectTree, search_entry);
  ctk_widget_class_bind_template_callback (widget_class, on_selection_changed);
  ctk_widget_class_bind_template_callback (widget_class, on_row_activated);
  ctk_widget_class_bind_template_callback (widget_class, on_hierarchy_changed);
  ctk_widget_class_bind_template_callback (widget_class, on_search_changed);
  ctk_widget_class_bind_template_callback (widget_class, next_match);
  ctk_widget_class_bind_template_callback (widget_class, previous_match);
  ctk_widget_class_bind_template_callback (widget_class, stop_search);
}

typedef struct
{
  CtkInspectorObjectTree *wt;
  CtkTreeIter *iter;
  GObject *parent;
} FindAllData;

static void
child_callback (GObject    *object,
                const char *name,
                gpointer    data)
{
  FindAllData *d = data;

  ctk_inspector_object_tree_append_object (d->wt, object, d->iter, NULL);
}

void
ctk_inspector_object_tree_append_object (CtkInspectorObjectTree *wt,
                                         GObject                *object,
                                         CtkTreeIter            *parent_iter,
                                         const gchar            *name)
{
  CtkTreeIter iter;
  const gchar *class_name;
  gchar *classes;
  const gchar *label;
  FindAllData data;

  class_name = G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (object));

  if (CTK_IS_WIDGET (object))
    {
      const gchar *id;
      CtkStyleContext *context;
      GList *list, *l;
      GString *string;

      id = ctk_widget_get_name (CTK_WIDGET (object));
      if (name == NULL && id != NULL && g_strcmp0 (id, class_name) != 0)
        name = id;

      context = ctk_widget_get_style_context (CTK_WIDGET (object));
      string = g_string_new ("");
      list = ctk_style_context_list_classes (context);
      for (l = list; l; l = l->next)
        {
          if (string->len > 0)
            g_string_append_c (string, ' ');
          g_string_append (string, (gchar *)l->data);
        }
      classes = g_string_free (string, FALSE);
      g_list_free (list);
    }
  else
    {
      if (parent_iter)
        {
          GObject *parent;

          ctk_tree_model_get (CTK_TREE_MODEL (wt->priv->model), parent_iter,
                              OBJECT, &parent,
                              -1);
          g_object_set_data (object, "inspector-object-tree-parent", parent);
        }
      classes = g_strdup ("");
    }

  if (CTK_IS_BUILDABLE (object))
    {
      const gchar *id;
      id = ctk_buildable_get_name (CTK_BUILDABLE (object));
      if (name == NULL && id != NULL && !g_str_has_prefix (id, "___object_"))
        name = id;
    }

  if (name == NULL)
    name = "";

  if (CTK_IS_LABEL (object))
    label = ctk_label_get_text (CTK_LABEL (object));
  else if (CTK_IS_BUTTON (object))
    label = ctk_button_get_label (CTK_BUTTON (object));
  else if (CTK_IS_WINDOW (object))
    label = ctk_window_get_title (CTK_WINDOW (object));
  else if (CTK_IS_TREE_VIEW_COLUMN (object))
    label = ctk_tree_view_column_get_title (CTK_TREE_VIEW_COLUMN (object));
  else
    label = "";

  ctk_tree_store_append (wt->priv->model, &iter, parent_iter);
  ctk_tree_store_set (wt->priv->model, &iter,
                      OBJECT, object,
                      OBJECT_TYPE, class_name,
                      OBJECT_NAME, name,
                      OBJECT_LABEL, label,
                      OBJECT_CLASSES, classes,
                      SENSITIVE, object_get_sensitive (object),
                      -1);

  if (name && *name)
    {
      gchar *title;
      title = g_strconcat (class_name, " — ", name, NULL);
      g_object_set_data_full (object, "ctk-inspector-object-title", title, g_free);
    }
  else
    {
      g_object_set_data (object, "ctk-inspector-object-title", (gpointer)class_name);
    }

  g_free (classes);

  g_object_weak_ref (object, ctk_object_tree_remove_dead_object, wt);
  
  data.wt = wt;
  data.iter = &iter;
  data.parent = object;

  object_forall (object, child_callback, &data);
}

static void
block_selection_changed (CtkInspectorObjectTree *wt)
{
  CtkTreeSelection *selection;

  selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (wt->priv->tree));
  g_signal_handlers_block_by_func (selection, on_selection_changed, wt);
}

static void
unblock_selection_changed (CtkInspectorObjectTree *wt)
{
  CtkTreeSelection *selection;

  selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (wt->priv->tree));
  g_signal_handlers_unblock_by_func (selection, on_selection_changed, wt);
}

gboolean
select_object_internal (CtkInspectorObjectTree *wt,
                        GObject                *object,
                        gboolean                activate)
{
  CtkTreeIter iter;

  if (ctk_inspector_object_tree_find_object (wt, object, &iter))
    {
      CtkTreePath *path;
      CtkTreeSelection *selection;

      selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (wt->priv->tree));
      path = ctk_tree_model_get_path (CTK_TREE_MODEL (wt->priv->model), &iter);
      ctk_tree_view_expand_to_path (CTK_TREE_VIEW (wt->priv->tree), path);
      if (!activate)
        block_selection_changed (wt);
      ctk_tree_selection_select_iter (selection, &iter);
      if (!activate)
        unblock_selection_changed (wt);

      ctk_tree_view_scroll_to_cell (CTK_TREE_VIEW (wt->priv->tree), path, NULL, TRUE, 0.5, 0);
      if (activate)
        ctk_tree_view_row_activated (CTK_TREE_VIEW (wt->priv->tree), path, NULL);
      ctk_tree_path_free (path);

      return TRUE;
    }

  return FALSE;
}

gboolean
ctk_inspector_object_tree_select_object (CtkInspectorObjectTree *wt,
                                         GObject                *object)
{
  return select_object_internal (wt, object, TRUE);
}

void
ctk_inspector_object_tree_scan (CtkInspectorObjectTree *wt,
                                CtkWidget              *window)
{
  CtkWidget *inspector_win;
  GList *toplevels, *l;
  CdkScreen *screen;
  GObject *selected;

  block_selection_changed (wt);

  selected = ctk_inspector_object_tree_get_selected (wt);

  clear_store (wt);
  ctk_inspector_object_tree_append_object (wt, G_OBJECT (ctk_settings_get_default ()), NULL, NULL);
  if (g_application_get_default ())
    ctk_inspector_object_tree_append_object (wt, G_OBJECT (g_application_get_default ()), NULL, NULL);

  if (window)
    ctk_inspector_object_tree_append_object (wt, G_OBJECT (window), NULL, NULL);

  screen = cdk_screen_get_default ();

  inspector_win = ctk_widget_get_toplevel (CTK_WIDGET (wt));
  toplevels = ctk_window_list_toplevels ();
  for (l = toplevels; l; l = l->next)
    {
      if (CTK_IS_WINDOW (l->data) &&
          ctk_window_get_window_type (l->data) == CTK_WINDOW_TOPLEVEL &&
          ctk_widget_get_screen (l->data) == screen &&
          l->data != window &&
          l->data != inspector_win)
        ctk_inspector_object_tree_append_object (wt, G_OBJECT (l->data), NULL, NULL);
    }
  g_list_free (toplevels);

  ctk_tree_view_columns_autosize (CTK_TREE_VIEW (wt->priv->tree));

  if (selected)
    select_object_internal (wt, selected, FALSE);

  unblock_selection_changed (wt);
}

static gboolean
ctk_inspector_object_tree_find_object_at_parent_iter (CtkTreeModel *model,
                                                      GObject      *object,
                                                      CtkTreeIter  *parent,
                                                      CtkTreeIter  *iter)
{
  if (!ctk_tree_model_iter_children (model, iter, parent))
    return FALSE;

  do {
    GObject *lookup;

    ctk_tree_model_get (model, iter, OBJECT, &lookup, -1);

    if (lookup == object)
      return TRUE;

  } while (ctk_tree_model_iter_next (model, iter));

  return FALSE;
}

gboolean
ctk_inspector_object_tree_find_object (CtkInspectorObjectTree *wt,
                                       GObject                *object,
                                       CtkTreeIter            *iter)
{
  CtkTreeIter parent_iter;
  GObject *parent;

  parent = object_get_parent (object);
  if (parent)
    {
      if (!ctk_inspector_object_tree_find_object (wt, parent, &parent_iter))
        return FALSE;

      return ctk_inspector_object_tree_find_object_at_parent_iter (CTK_TREE_MODEL (wt->priv->model),
                                                                   object,
                                                                   &parent_iter,
                                                                   iter);
    }
  else
    {
      return ctk_inspector_object_tree_find_object_at_parent_iter (CTK_TREE_MODEL (wt->priv->model),
                                                                   object,
                                                                   NULL,
                                                                   iter);
    }
}


// vim: set et sw=2 ts=2:
