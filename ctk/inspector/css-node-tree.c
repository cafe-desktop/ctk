/*
 * Copyright (c) 2014 Benjamin Otte <otte@gnome.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicntnse,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright noticnt and this permission noticnt shall be included
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

#include "css-node-tree.h"
#include "prop-editor.h"

#include "ctktreemodelcssnode.h"
#include "ctktreeview.h"
#include "ctklabel.h"
#include "ctkpopover.h"
#include "ctk/ctkwidgetprivate.h"
#include "ctkcssproviderprivate.h"
#include "ctkcssstylepropertyprivate.h"
#include "ctkcsssectionprivate.h"
#include "ctkcssstyleprivate.h"
#include "ctkcssvalueprivate.h"
#include "ctkcssselectorprivate.h"
#include "ctkliststore.h"
#include "ctksettings.h"
#include "ctktreeview.h"
#include "ctktreeselection.h"
#include "ctktypebuiltins.h"
#include "ctkmodelbutton.h"
#include "ctkstack.h"

enum {
  COLUMN_NODE_NAME,
  COLUMN_NODE_VISIBLE,
  COLUMN_NODE_CLASSES,
  COLUMN_NODE_ID,
  COLUMN_NODE_STATE,
  /* add more */
  N_NODE_COLUMNS
};

enum
{
  COLUMN_PROP_NAME,
  COLUMN_PROP_VALUE,
  COLUMN_PROP_LOCATION
};

struct _CtkInspectorCssNodeTreePrivate
{
  CtkWidget *node_tree;
  CtkTreeModel *node_model;
  CtkTreeViewColumn *node_name_column;
  CtkTreeViewColumn *node_id_column;
  CtkTreeViewColumn *node_classes_column;
  CtkListStore *prop_model;
  CtkWidget *prop_tree;
  CtkTreeViewColumn *prop_name_column;
  GHashTable *prop_iters;
  CtkCssNode *node;
};

G_DEFINE_TYPE_WITH_PRIVATE (CtkInspectorCssNodeTree, ctk_inspector_css_node_tree, CTK_TYPE_BOX)

typedef struct {
  CtkCssNode *node;
  const gchar *prop_name;
  CdkRectangle rect;
  CtkInspectorCssNodeTree *cnt;
} NodePropEditor;

static void
show_node_prop_editor (NodePropEditor *npe)
{
  CtkWidget *popover;
  CtkWidget *editor;

  popover = ctk_popover_new (CTK_WIDGET (npe->cnt->priv->node_tree));
  ctk_popover_set_pointing_to (CTK_POPOVER (popover), &npe->rect);

  editor = ctk_inspector_prop_editor_new (G_OBJECT (npe->node), npe->prop_name, FALSE);
  ctk_widget_show (editor);

  ctk_container_add (CTK_CONTAINER (popover), editor);

  if (ctk_inspector_prop_editor_should_expand (CTK_INSPECTOR_PROP_EDITOR (editor)))
    ctk_widget_set_vexpand (popover, TRUE);

  ctk_popover_popup (CTK_POPOVER (popover));

  g_signal_connect (popover, "unmap", G_CALLBACK (ctk_widget_destroy), NULL);
}

static void
row_activated (CtkTreeView             *tv,
               CtkTreePath             *path,
               CtkTreeViewColumn       *col,
               CtkInspectorCssNodeTree *cnt)
{
  CtkTreeIter iter;
  NodePropEditor npe;

  npe.cnt = cnt;

  if (col == cnt->priv->node_name_column)
    npe.prop_name = "name";
  else if (col == cnt->priv->node_id_column)
    npe.prop_name = "id";
  else if (col == cnt->priv->node_classes_column)
    npe.prop_name = "classes";
  else
    return;

  ctk_tree_model_get_iter (cnt->priv->node_model, &iter, path);
  npe.node = ctk_tree_model_css_node_get_node_from_iter (CTK_TREE_MODEL_CSS_NODE (cnt->priv->node_model), &iter);
  ctk_tree_view_get_cell_area (tv, path, col, &npe.rect);
  ctk_tree_view_convert_bin_window_to_widget_coords (tv, npe.rect.x, npe.rect.y, &npe.rect.x, &npe.rect.y);

  show_node_prop_editor (&npe);
}

static void
ctk_inspector_css_node_tree_set_node (CtkInspectorCssNodeTree *cnt,
                                      CtkCssNode              *node);

static void
selection_changed (CtkTreeSelection *selection, CtkInspectorCssNodeTree *cnt)
{
  CtkTreeIter iter;
  CtkCssNode *node;

  if (!ctk_tree_selection_get_selected (selection, NULL, &iter))
    return;

  node = ctk_tree_model_css_node_get_node_from_iter (CTK_TREE_MODEL_CSS_NODE (cnt->priv->node_model), &iter);
  ctk_inspector_css_node_tree_set_node (cnt, node);
}

static void
ctk_inspector_css_node_tree_unset_node (CtkInspectorCssNodeTree *cnt)
{
  CtkInspectorCssNodeTreePrivate *priv = cnt->priv;

  if (priv->node)
    {
      g_signal_handlers_disconnect_matched (priv->node,
                                            G_SIGNAL_MATCH_DATA,
                                            0, 0, NULL, NULL,
                                            cnt);
      g_object_unref (priv->node);
      priv->node = NULL;
    }
}

static void
ctk_inspector_css_node_tree_finalize (GObject *object)
{
  CtkInspectorCssNodeTree *cnt = CTK_INSPECTOR_CSS_NODE_TREE (object);

  ctk_inspector_css_node_tree_unset_node (cnt);

  g_hash_table_unref (cnt->priv->prop_iters);

  G_OBJECT_CLASS (ctk_inspector_css_node_tree_parent_class)->finalize (object);
}

static void
ensure_css_sections (void)
{
  CtkSettings *settings;
  gchar *theme_name;

  ctk_css_provider_set_keep_css_sections ();

  settings = ctk_settings_get_default ();
  g_object_get (settings, "ctk-theme-name", &theme_name, NULL);
  g_object_set (settings, "ctk-theme-name", theme_name, NULL);
  g_free (theme_name);
}

static void
ctk_inspector_css_node_tree_class_init (CtkInspectorCssNodeTreeClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);

  ensure_css_sections ();

  object_class->finalize = ctk_inspector_css_node_tree_finalize;

  ctk_widget_class_set_template_from_resource (widget_class, "/org/ctk/libctk/inspector/css-node-tree.ui");
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorCssNodeTree, node_tree);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorCssNodeTree, node_name_column);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorCssNodeTree, node_id_column);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorCssNodeTree, node_classes_column);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorCssNodeTree, prop_name_column);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorCssNodeTree, prop_model);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorCssNodeTree, prop_name_column);

  ctk_widget_class_bind_template_callback (widget_class, row_activated);
  ctk_widget_class_bind_template_callback (widget_class, selection_changed);
}

static int
sort_strv (gconstpointer a,
           gconstpointer b,
           gpointer      data G_GNUC_UNUSED)
{
  char **ap = (char **) a;
  char **bp = (char **) b;

  return g_ascii_strcasecmp (*ap, *bp);
}

static void
strv_sort (char **strv)
{
  g_qsort_with_data (strv,
		     g_strv_length (strv),
                     sizeof (char *),
                     sort_strv,
                     NULL);
}

static gchar *
format_state_flags (CtkStateFlags state)
{
  if (state)
    {
      GString *str;
      gint i;
      gboolean first = TRUE;

      str = g_string_new ("");

      for (i = 0; i < 31; i++)
        {
          if (state & (1 << i))
            {
              if (!first)
                g_string_append (str, " | ");
              first = FALSE;
              g_string_append (str, ctk_css_pseudoclass_name (1 << i));
            }
        }
      return g_string_free (str, FALSE);
    }

 return g_strdup ("");
}

static void
ctk_inspector_css_node_tree_get_node_value (CtkTreeModelCssNode *model G_GNUC_UNUSED,
                                            CtkCssNode          *node,
                                            int                  column,
                                            GValue              *value)
{
  char **strv;
  char *s;

  switch (column)
    {
    case COLUMN_NODE_NAME:
      g_value_set_string (value, ctk_css_node_get_name (node));
      break;

    case COLUMN_NODE_VISIBLE:
      g_value_set_boolean (value, ctk_css_node_get_visible (node));
      break;

    case COLUMN_NODE_CLASSES:
      strv = ctk_css_node_get_classes (node);
      strv_sort (strv);
      s = g_strjoinv (" ", strv);
      g_value_take_string (value, s);
      g_strfreev (strv);
      break;

    case COLUMN_NODE_ID:
      g_value_set_string (value, ctk_css_node_get_id (node));
      break;

    case COLUMN_NODE_STATE:
      g_value_take_string (value, format_state_flags (ctk_css_node_get_state (node)));
      break;

    default:
      g_assert_not_reached ();
      break;
    }
}

static void
ctk_inspector_css_node_tree_init (CtkInspectorCssNodeTree *cnt)
{
  CtkInspectorCssNodeTreePrivate *priv;
  gint i;

  cnt->priv = ctk_inspector_css_node_tree_get_instance_private (cnt);
  ctk_widget_init_template (CTK_WIDGET (cnt));
  priv = cnt->priv;

  priv->node_model = ctk_tree_model_css_node_new (ctk_inspector_css_node_tree_get_node_value,
                                                  N_NODE_COLUMNS,
                                                  G_TYPE_STRING,
                                                  G_TYPE_BOOLEAN,
                                                  G_TYPE_STRING,
                                                  G_TYPE_STRING,
                                                  G_TYPE_STRING);
  ctk_tree_view_set_model (CTK_TREE_VIEW (priv->node_tree), priv->node_model);
  g_object_unref (priv->node_model);

  ctk_tree_sortable_set_sort_column_id (CTK_TREE_SORTABLE (cnt->priv->prop_model),
                                        COLUMN_PROP_NAME,
                                        CTK_SORT_ASCENDING);

  priv->prop_iters = g_hash_table_new_full (g_str_hash, g_str_equal,
                                            NULL, (GDestroyNotify) ctk_tree_iter_free);

  for (i = 0; i < _ctk_css_style_property_get_n_properties (); i++)
    {
      CtkCssStyleProperty *prop;
      CtkTreeIter iter;
      const gchar *name;

      prop = _ctk_css_style_property_lookup_by_id (i);
      name = _ctk_style_property_get_name (CTK_STYLE_PROPERTY (prop));

      ctk_list_store_append (cnt->priv->prop_model, &iter);
      ctk_list_store_set (cnt->priv->prop_model, &iter, COLUMN_PROP_NAME, name, -1);
      g_hash_table_insert (cnt->priv->prop_iters, (gpointer)name, ctk_tree_iter_copy (&iter));
    }
}

void
ctk_inspector_css_node_tree_set_object (CtkInspectorCssNodeTree *cnt,
                                        GObject                 *object)
{
  CtkInspectorCssNodeTreePrivate *priv;
  CtkCssNode *node, *root;
  CtkTreePath *path;
  CtkTreeIter iter;

  g_return_if_fail (CTK_INSPECTOR_IS_CSS_NODE_TREE (cnt));

  priv = cnt->priv;

  if (!CTK_IS_WIDGET (object))
    {
      ctk_widget_hide (CTK_WIDGET (cnt));
      return;
    }

  ctk_widget_show (CTK_WIDGET (cnt));

  root = node = ctk_widget_get_css_node (CTK_WIDGET (object));
  while (ctk_css_node_get_parent (root))
    root = ctk_css_node_get_parent (root);

  ctk_tree_model_css_node_set_root_node (CTK_TREE_MODEL_CSS_NODE (priv->node_model), root);

  ctk_tree_model_css_node_get_iter_from_node (CTK_TREE_MODEL_CSS_NODE (priv->node_model), &iter, node);
  path = ctk_tree_model_get_path (priv->node_model, &iter);

  ctk_tree_view_expand_to_path (CTK_TREE_VIEW (priv->node_tree), path);
  ctk_tree_view_set_cursor (CTK_TREE_VIEW (priv->node_tree), path, NULL, FALSE);
  ctk_tree_view_scroll_to_cell (CTK_TREE_VIEW (priv->node_tree), path, NULL, TRUE, 0.5, 0.0);

  ctk_tree_path_free (path);
}

static void
ctk_inspector_css_node_tree_update_style (CtkInspectorCssNodeTree *cnt,
                                          CtkCssStyle             *new_style)
{
  CtkInspectorCssNodeTreePrivate *priv = cnt->priv;
  gint i;

  for (i = 0; i < _ctk_css_style_property_get_n_properties (); i++)
    {
      CtkCssStyleProperty *prop;
      const gchar *name;
      CtkTreeIter *iter;
      CtkCssSection *section;
      gchar *location;
      gchar *value;

      prop = _ctk_css_style_property_lookup_by_id (i);
      name = _ctk_style_property_get_name (CTK_STYLE_PROPERTY (prop));

      iter = (CtkTreeIter *)g_hash_table_lookup (priv->prop_iters, name);

      if (new_style)
        {
          value = _ctk_css_value_to_string (ctk_css_style_get_value (new_style, i));

          section = ctk_css_style_get_section (new_style, i);
          if (section)
            location = _ctk_css_section_to_string (section);
          else
            location = NULL;
        }
      else
        {
          value = NULL;
          location = NULL;
        }

      ctk_list_store_set (priv->prop_model,
                          iter,
                          COLUMN_PROP_VALUE, value,
                          COLUMN_PROP_LOCATION, location,
                          -1);

      g_free (location);
      g_free (value);
    }
}

static void
ctk_inspector_css_node_tree_update_style_cb (CtkCssNode              *node G_GNUC_UNUSED,
                                             CtkCssStyleChange       *change,
                                             CtkInspectorCssNodeTree *cnt)
{
  ctk_inspector_css_node_tree_update_style (cnt, ctk_css_style_change_get_new_style (change));
}

static void
ctk_inspector_css_node_tree_set_node (CtkInspectorCssNodeTree *cnt,
                                      CtkCssNode              *node)
{
  CtkInspectorCssNodeTreePrivate *priv = cnt->priv;

  if (priv->node == node)
    return;

  if (node)
    g_object_ref (node);

  ctk_inspector_css_node_tree_update_style (cnt, node ? ctk_css_node_get_style (node) : NULL);

  ctk_inspector_css_node_tree_unset_node (cnt);

  priv->node = node;

  g_signal_connect (node, "style-changed", G_CALLBACK (ctk_inspector_css_node_tree_update_style_cb), cnt);
}

// vim: set et sw=2 ts=2:
