/* GTK - The GIMP Toolkit
 * Copyright (C) 2014 Benjamin Otte <otte@gnome.org>
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

#include "config.h"

#include "ctkcssnodeprivate.h"

#include "ctkcssanimatedstyleprivate.h"
#include "ctkcsssectionprivate.h"
#include "ctkcssstylepropertyprivate.h"
#include "ctkintl.h"
#include "ctkmarshalers.h"
#include "ctksettingsprivate.h"
#include "ctktypebuiltins.h"

/*
 * CSS nodes are the backbone of the GtkStyleContext implementation and
 * replace the role that GtkWidgetPath played in the past. A CSS node has
 * an element name and a state, and can have an id and style classes, which
 * is what is needed to determine the matching CSS selectors. CSS nodes have
 * a 'visible' property, which makes it possible to temporarily 'hide' them
 * from CSS matching - e.g. an invisible node will not affect :nth-child
 * matching and so forth.
 *
 * The API to manage states, names, ids and classes of CSS nodes is:
 * - ctk_css_node_get/set_state. States are represented as GtkStateFlags
 * - ctk_css_node_get/set_name. Names are represented as interned strings
 * - ctk_css_node_get/set_id. Ids are represented as interned strings
 * - ctk_css_node_add/remove/has_class and ctk_css_node_list_classes. Style
 *   classes are represented as quarks.
 *
 * CSS nodes are organized in a dom-like tree, and there is API to navigate
 * and manipulate this tree:
 * - ctk_css_node_set_parent
 * - ctk_css_node_insert_before/after
 * - ctk_css_node_get_parent
 * - ctk_css_node_get_first/last_child
 * - ctk_css_node_get_previous/next_sibling
 * Note that parents keep a reference on their children in this tree.
 *
 * Every widget has one or more CSS nodes - the first one gets created
 * automatically by GtkStyleContext. To set the name of the main node,
 * call ctk_widget_class_set_css_name() in class_init(). Widget implementations
 * can and should add subnodes as suitable.
 *
 * Best practice is:
 * - For permanent subnodes, create them in init(), and keep a pointer
 *   to the node (you don't have to keep a reference, cleanup will be
 *   automatic by means of the parent node getting cleaned up by the
 *   style context).
 * - For transient nodes, create/destroy them when the conditions that
 *   warrant their existence change.
 * - Keep the state of all your nodes up-to-date. This probably requires
 *   a ::state-flags-changed (and possibly ::direction-changed) handler,
 *   as well as code to update the state in other places. Note that GTK+
 *   does this automatically for the widget's main CSS node.
 * - The sibling ordering in the CSS node tree is supposed to correspond
 *   to the visible order of content: top-to-bottom and left-to-right.
 *   Reorder your nodes to maintain this correlation. In particular for
 *   horizontally layed out widgets, this will require listening to
 *   ::direction-changed.
 * - The draw function should just use ctk_style_context_save_to_node() to
 *   'switch' to the right node, not make any other changes to the style
 *   context.
 *
 * A noteworthy difference between ctk_style_context_save() and
 * ctk_style_context_save_to_node() is that the former inherits all the
 * style classes from the main CSS node, which often leads to unintended
 * inheritance.
 */

/* When these change we do a full restyling. Otherwise we try to figure out
 * if we need to change things. */
#define CTK_CSS_RADICAL_CHANGE (CTK_CSS_CHANGE_ID | CTK_CSS_CHANGE_NAME | CTK_CSS_CHANGE_CLASS | CTK_CSS_CHANGE_SOURCE | CTK_CSS_CHANGE_PARENT_STYLE)

G_DEFINE_TYPE (GtkCssNode, ctk_css_node, G_TYPE_OBJECT)

enum {
  NODE_ADDED,
  NODE_REMOVED,
  STYLE_CHANGED,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_CLASSES,
  PROP_ID,
  PROP_NAME,
  PROP_STATE,
  PROP_VISIBLE,
  PROP_WIDGET_TYPE,
  NUM_PROPERTIES
};

struct _GtkCssNodeStyleChange {
  GtkCssStyle *old_style;
  GtkCssStyle *new_style;
};

static guint cssnode_signals[LAST_SIGNAL] = { 0 };
static GParamSpec *cssnode_properties[NUM_PROPERTIES];

static GtkStyleProviderPrivate *
ctk_css_node_get_style_provider_or_null (GtkCssNode *cssnode)
{
  return CTK_CSS_NODE_GET_CLASS (cssnode)->get_style_provider (cssnode);
}

static void
ctk_css_node_set_invalid (GtkCssNode *node,
                          gboolean    invalid)
{
  if (node->invalid == invalid)
    return;

  node->invalid = invalid;

  if (node->visible)
    {
      if (node->parent)
        {
          if (invalid)
            ctk_css_node_set_invalid (node->parent, TRUE);
        }
      else
        {
          if (invalid)
            CTK_CSS_NODE_GET_CLASS (node)->queue_validate (node);
          else
            CTK_CSS_NODE_GET_CLASS (node)->dequeue_validate (node);
        }
    }
}

static void
ctk_css_node_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  GtkCssNode *cssnode = CTK_CSS_NODE (object);

  switch (property_id)
    {
    case PROP_CLASSES:
      g_value_take_boxed (value, ctk_css_node_get_classes (cssnode));
      break;

    case PROP_ID:
      g_value_set_string (value, ctk_css_node_get_id (cssnode));
      break;

    case PROP_NAME:
      g_value_set_string (value, ctk_css_node_get_name (cssnode));
      break;

    case PROP_STATE:
      g_value_set_flags (value, ctk_css_node_get_state (cssnode));
      break;

    case PROP_VISIBLE:
      g_value_set_boolean (value, ctk_css_node_get_visible (cssnode));
      break;

    case PROP_WIDGET_TYPE:
      g_value_set_gtype (value, ctk_css_node_get_widget_type (cssnode));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
ctk_css_node_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  GtkCssNode *cssnode = CTK_CSS_NODE (object);

  switch (property_id)
    {
    case PROP_CLASSES:
      ctk_css_node_set_classes (cssnode, g_value_get_boxed (value));
      break;

    case PROP_ID:
      ctk_css_node_set_id (cssnode, g_value_get_string (value));
      break;

    case PROP_NAME:
      ctk_css_node_set_name (cssnode, g_value_get_string (value));
      break;

    case PROP_STATE:
      ctk_css_node_set_state (cssnode, g_value_get_flags (value));
      break;

    case PROP_VISIBLE:
      ctk_css_node_set_visible (cssnode, g_value_get_boolean (value));
      break;

    case PROP_WIDGET_TYPE:
      ctk_css_node_set_widget_type (cssnode, g_value_get_gtype (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
ctk_css_node_dispose (GObject *object)
{
  GtkCssNode *cssnode = CTK_CSS_NODE (object);

  while (cssnode->first_child)
    {
      ctk_css_node_set_parent (cssnode->first_child, NULL);
    }

  ctk_css_node_set_invalid (cssnode, FALSE);
  
  g_clear_pointer (&cssnode->cache, ctk_css_node_style_cache_unref);

  G_OBJECT_CLASS (ctk_css_node_parent_class)->dispose (object);
}

static void
ctk_css_node_finalize (GObject *object)
{
  GtkCssNode *cssnode = CTK_CSS_NODE (object);

  if (cssnode->style)
    g_object_unref (cssnode->style);
  ctk_css_node_declaration_unref (cssnode->decl);

  G_OBJECT_CLASS (ctk_css_node_parent_class)->finalize (object);
}

static gboolean
ctk_css_node_is_first_child (GtkCssNode *node)
{
  GtkCssNode *iter;

  for (iter = node->previous_sibling;
       iter != NULL;
       iter = iter->previous_sibling)
    {
      if (iter->visible)
        return FALSE;
    }
  return TRUE;
}

static gboolean
ctk_css_node_is_last_child (GtkCssNode *node)
{
  GtkCssNode *iter;

  for (iter = node->next_sibling;
       iter != NULL;
       iter = iter->next_sibling)
    {
      if (iter->visible)
        return FALSE;
    }
  return TRUE;
}

static gboolean
may_use_global_parent_cache (GtkCssNode *node)
{
  GtkStyleProviderPrivate *provider;
  GtkCssNode *parent;
  
  parent = ctk_css_node_get_parent (node);
  if (parent == NULL)
    return FALSE;

  provider = ctk_css_node_get_style_provider_or_null (node);
  if (provider != NULL && provider != ctk_css_node_get_style_provider (parent))
    return FALSE;

  return TRUE;
}

static GtkCssStyle *
lookup_in_global_parent_cache (GtkCssNode                  *node,
                               const GtkCssNodeDeclaration *decl)
{
  GtkCssNode *parent;

  parent = node->parent;

  if (parent == NULL ||
      !may_use_global_parent_cache (node))
    return NULL;

  if (parent->cache == NULL)
    return NULL;

  g_assert (node->cache == NULL);
  node->cache = ctk_css_node_style_cache_lookup (parent->cache,
                                                 decl,
                                                 ctk_css_node_is_first_child (node),
                                                 ctk_css_node_is_last_child (node));
  if (node->cache == NULL)
    return NULL;

  return ctk_css_node_style_cache_get_style (node->cache);
}

static void
store_in_global_parent_cache (GtkCssNode                  *node,
                              const GtkCssNodeDeclaration *decl,
                              GtkCssStyle                 *style)
{
  GtkCssNode *parent;

  g_assert (CTK_IS_CSS_STATIC_STYLE (style));

  parent = node->parent;

  if (parent == NULL ||
      !may_use_global_parent_cache (node))
    return;

  if (parent->cache == NULL)
    parent->cache = ctk_css_node_style_cache_new (parent->style);

  node->cache = ctk_css_node_style_cache_insert (parent->cache,
                                                 (GtkCssNodeDeclaration *) decl,
                                                 ctk_css_node_is_first_child (node),
                                                 ctk_css_node_is_last_child (node),
                                                 style);
}

static GtkCssStyle *
ctk_css_node_create_style (GtkCssNode *cssnode)
{
  const GtkCssNodeDeclaration *decl;
  GtkCssMatcher matcher;
  GtkCssStyle *parent;
  GtkCssStyle *style;

  decl = ctk_css_node_get_declaration (cssnode);
  parent = cssnode->parent ? cssnode->parent->style : NULL;

  style = lookup_in_global_parent_cache (cssnode, decl);
  if (style)
    return g_object_ref (style);

  if (ctk_css_node_init_matcher (cssnode, &matcher))
    style = ctk_css_static_style_new_compute (ctk_css_node_get_style_provider (cssnode),
                                              &matcher,
                                              parent);
  else
    style = ctk_css_static_style_new_compute (ctk_css_node_get_style_provider (cssnode),
                                              NULL,
                                              parent);

  store_in_global_parent_cache (cssnode, decl, style);

  return style;
}

static gboolean
should_create_transitions (GtkCssChange change)
{
  return (change & CTK_CSS_CHANGE_ANIMATIONS) == 0;
}

static gboolean
ctk_css_style_needs_recreation (GtkCssStyle  *style,
                                GtkCssChange  change)
{
  /* Try to avoid invalidating if we can */
  if (change & CTK_CSS_RADICAL_CHANGE)
    return TRUE;

  if (CTK_IS_CSS_ANIMATED_STYLE (style))
    style = CTK_CSS_ANIMATED_STYLE (style)->style;

  if (ctk_css_static_style_get_change (CTK_CSS_STATIC_STYLE (style)) & change)
    return TRUE;
  else
    return FALSE;
}

static GtkCssStyle *
ctk_css_node_real_update_style (GtkCssNode   *cssnode,
                                GtkCssChange  change,
                                gint64        timestamp,
                                GtkCssStyle  *style)
{
  GtkCssStyle *static_style, *new_static_style, *new_style;

  if (CTK_IS_CSS_ANIMATED_STYLE (style))
    {
      static_style = CTK_CSS_ANIMATED_STYLE (style)->style;
    }
  else
    {
      static_style = style;
    }

  if (ctk_css_style_needs_recreation (static_style, change))
    new_static_style = ctk_css_node_create_style (cssnode);
  else
    new_static_style = g_object_ref (static_style);

  if (new_static_style != static_style || (change & CTK_CSS_CHANGE_ANIMATIONS))
    {
      GtkCssNode *parent = ctk_css_node_get_parent (cssnode);
      new_style = ctk_css_animated_style_new (new_static_style,
                                              parent ? ctk_css_node_get_style (parent) : NULL,
                                              timestamp,
                                              ctk_css_node_get_style_provider (cssnode),
                                              should_create_transitions (change) ? style : NULL);

      /* Clear the cache again, the static style we looked up above
       * may have populated it. */
      g_clear_pointer (&cssnode->cache, ctk_css_node_style_cache_unref);
    }
  else if (static_style != style && (change & CTK_CSS_CHANGE_TIMESTAMP))
    {
      new_style = ctk_css_animated_style_new_advance (CTK_CSS_ANIMATED_STYLE (style),
                                                      static_style,
                                                      timestamp);
    }
  else
    {
      new_style = g_object_ref (style);
    }

  if (!ctk_css_style_is_static (new_style))
    ctk_css_node_set_invalid (cssnode, TRUE);

  g_object_unref (new_static_style);

  return new_style;
}

static void
ctk_css_node_real_invalidate (GtkCssNode *node)
{
}

static void
ctk_css_node_real_queue_validate (GtkCssNode *node)
{
}

static void
ctk_css_node_real_dequeue_validate (GtkCssNode *node)
{
}

static void
ctk_css_node_real_validate (GtkCssNode *node)
{
}

gboolean
ctk_css_node_real_init_matcher (GtkCssNode     *cssnode,
                                GtkCssMatcher  *matcher)
{
  _ctk_css_matcher_node_init (matcher, cssnode);

  return TRUE;
}

static GtkWidgetPath *
ctk_css_node_real_create_widget_path (GtkCssNode *cssnode)
{
  return ctk_widget_path_new ();
}

static const GtkWidgetPath *
ctk_css_node_real_get_widget_path (GtkCssNode *cssnode)
{
  return NULL;
}

static GtkStyleProviderPrivate *
ctk_css_node_real_get_style_provider (GtkCssNode *cssnode)
{
  return NULL;
}

static GdkFrameClock *
ctk_css_node_real_get_frame_clock (GtkCssNode *cssnode)
{
  return NULL;
}

static void
ctk_css_node_real_node_removed (GtkCssNode *parent,
                                GtkCssNode *node,
                                GtkCssNode *previous)
{
  if (node->previous_sibling)
    node->previous_sibling->next_sibling = node->next_sibling;
  else
    node->parent->first_child = node->next_sibling;

  if (node->next_sibling)
    node->next_sibling->previous_sibling = node->previous_sibling;
  else
    node->parent->last_child = node->previous_sibling;

  node->previous_sibling = NULL;
  node->next_sibling = NULL;
  node->parent = NULL;
}

static void
ctk_css_node_real_node_added (GtkCssNode *parent,
                              GtkCssNode *node,
                              GtkCssNode *new_previous)
{
  if (new_previous)
    {
      node->previous_sibling = new_previous;
      node->next_sibling = new_previous->next_sibling;
      new_previous->next_sibling = node;
    }
  else
    {
      node->next_sibling = parent->first_child;
      parent->first_child = node;
    }

  if (node->next_sibling)
    node->next_sibling->previous_sibling = node;
  else
    parent->last_child = node;

  node->parent = parent;
}

static void
ctk_css_node_real_style_changed (GtkCssNode        *cssnode,
                                 GtkCssStyleChange *change)
{
  g_set_object (&cssnode->style, ctk_css_style_change_get_new_style (change));
}

static void
ctk_css_node_class_init (GtkCssNodeClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = ctk_css_node_get_property;
  object_class->set_property = ctk_css_node_set_property;
  object_class->dispose = ctk_css_node_dispose;
  object_class->finalize = ctk_css_node_finalize;

  klass->update_style = ctk_css_node_real_update_style;
  klass->invalidate = ctk_css_node_real_invalidate;
  klass->validate = ctk_css_node_real_validate;
  klass->queue_validate = ctk_css_node_real_queue_validate;
  klass->dequeue_validate = ctk_css_node_real_dequeue_validate;
  klass->init_matcher = ctk_css_node_real_init_matcher;
  klass->create_widget_path = ctk_css_node_real_create_widget_path;
  klass->get_widget_path = ctk_css_node_real_get_widget_path;
  klass->get_style_provider = ctk_css_node_real_get_style_provider;
  klass->get_frame_clock = ctk_css_node_real_get_frame_clock;

  klass->node_added = ctk_css_node_real_node_added;
  klass->node_removed = ctk_css_node_real_node_removed;
  klass->style_changed = ctk_css_node_real_style_changed;

  cssnode_signals[NODE_ADDED] =
    g_signal_new (I_("node-added"),
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GtkCssNodeClass, node_added),
		  NULL, NULL,
		  _ctk_marshal_VOID__OBJECT_OBJECT,
		  G_TYPE_NONE, 2,
		  CTK_TYPE_CSS_NODE, CTK_TYPE_CSS_NODE);
  g_signal_set_va_marshaller (cssnode_signals[NODE_ADDED],
                              G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_VOID__OBJECT_OBJECTv);

  cssnode_signals[NODE_REMOVED] =
    g_signal_new (I_("node-removed"),
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GtkCssNodeClass, node_removed),
		  NULL, NULL,
		  _ctk_marshal_VOID__OBJECT_OBJECT,
		  G_TYPE_NONE, 2,
		  CTK_TYPE_CSS_NODE, CTK_TYPE_CSS_NODE);
  g_signal_set_va_marshaller (cssnode_signals[NODE_REMOVED],
                              G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_VOID__OBJECT_OBJECTv);

  cssnode_signals[STYLE_CHANGED] =
    g_signal_new (I_("style-changed"),
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GtkCssNodeClass, style_changed),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 1,
		  G_TYPE_POINTER);

  cssnode_properties[PROP_CLASSES] =
    g_param_spec_boxed ("classes", P_("Style Classes"), P_("List of classes"),
                         G_TYPE_STRV,
                         G_PARAM_READWRITE
                         | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  cssnode_properties[PROP_ID] =
    g_param_spec_string ("id", P_("ID"), P_("Unique ID"),
                         NULL,
                         G_PARAM_READWRITE
                         | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  cssnode_properties[PROP_NAME] =
    g_param_spec_string ("name", P_("Name"), "Name identifying the type of node",
                         NULL,
                         G_PARAM_READWRITE
                         | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  cssnode_properties[PROP_STATE] =
    g_param_spec_flags ("state", P_("State"), P_("State flags"),
                        CTK_TYPE_STATE_FLAGS,
                        0,
                        G_PARAM_READWRITE
                        | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  cssnode_properties[PROP_VISIBLE] =
    g_param_spec_boolean ("visible", P_("Visible"), P_("If other nodes can see this node"),
                          TRUE,
                          G_PARAM_READWRITE
                          | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  cssnode_properties[PROP_WIDGET_TYPE] =
    g_param_spec_gtype ("widget-type", P_("Widget type"), P_("GType of the widget"),
                        G_TYPE_NONE,
                        G_PARAM_READWRITE
                        | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, NUM_PROPERTIES, cssnode_properties);
}

static void
ctk_css_node_init (GtkCssNode *cssnode)
{
  cssnode->decl = ctk_css_node_declaration_new ();

  cssnode->style = g_object_ref (ctk_css_static_style_get_default ());

  cssnode->visible = TRUE;
}

/**
 * ctk_css_node_new:
 *
 * Creates a new CSS node.
 *
 * Returns: (transfer full): the new CSS node
 */
GtkCssNode *
ctk_css_node_new (void)
{
  return g_object_new (CTK_TYPE_CSS_NODE, NULL);
}

static GdkFrameClock *
ctk_css_node_get_frame_clock_or_null (GtkCssNode *cssnode)
{
  while (cssnode->parent)
    cssnode = cssnode->parent;

  return CTK_CSS_NODE_GET_CLASS (cssnode)->get_frame_clock (cssnode);
}

static gint64
ctk_css_node_get_timestamp (GtkCssNode *cssnode)
{
  GdkFrameClock *frameclock;

  frameclock = ctk_css_node_get_frame_clock_or_null (cssnode);
  if (frameclock == NULL)
    return 0;

  return gdk_frame_clock_get_frame_time (frameclock);
}

static void
ctk_css_node_parent_was_unset (GtkCssNode *node)
{
  if (node->visible && node->invalid)
    CTK_CSS_NODE_GET_CLASS (node)->queue_validate (node);
}

static void
ctk_css_node_parent_will_be_set (GtkCssNode *node)
{
  if (node->visible && node->invalid)
    CTK_CSS_NODE_GET_CLASS (node)->dequeue_validate (node);
}

static void
ctk_css_node_invalidate_style (GtkCssNode *cssnode)
{
  if (cssnode->style_is_invalid)
    return;

  cssnode->style_is_invalid = TRUE;
  ctk_css_node_set_invalid (cssnode, TRUE);
  
  if (cssnode->first_child)
    ctk_css_node_invalidate_style (cssnode->first_child);
  
  if (cssnode->next_sibling)
    ctk_css_node_invalidate_style (cssnode->next_sibling);
}

static void
ctk_css_node_reposition (GtkCssNode *node,
                         GtkCssNode *new_parent,
                         GtkCssNode *previous)
{
  GtkCssNode *old_parent;

  g_assert (! (new_parent == NULL && previous != NULL));

  old_parent = node->parent;
  /* Take a reference here so the whole function has a reference */
  g_object_ref (node);

  if (node->visible)
    {
      if (node->next_sibling)
        ctk_css_node_invalidate (node->next_sibling,
                                 CTK_CSS_CHANGE_ANY_SIBLING
                                 | CTK_CSS_CHANGE_NTH_CHILD
                                 | (node->previous_sibling ? 0 : CTK_CSS_CHANGE_FIRST_CHILD));
      else if (node->previous_sibling)
        ctk_css_node_invalidate (node->previous_sibling, CTK_CSS_CHANGE_LAST_CHILD);
    }

  if (old_parent != NULL)
    {
      g_signal_emit (old_parent, cssnode_signals[NODE_REMOVED], 0, node, node->previous_sibling);
      if (old_parent->first_child && node->visible)
        ctk_css_node_invalidate (old_parent->first_child, CTK_CSS_CHANGE_NTH_LAST_CHILD);
    }

  if (old_parent != new_parent)
    {
      if (old_parent == NULL)
        {
          ctk_css_node_parent_will_be_set (node);
        }
      else
        {
          g_object_unref (node);
        }

      if (ctk_css_node_get_style_provider_or_null (node) == NULL)
        ctk_css_node_invalidate_style_provider (node);
      ctk_css_node_invalidate (node, CTK_CSS_CHANGE_TIMESTAMP | CTK_CSS_CHANGE_ANIMATIONS);

      if (new_parent)
        {
          g_object_ref (node);

          if (node->pending_changes)
            new_parent->needs_propagation = TRUE;
          if (node->invalid && node->visible)
            ctk_css_node_set_invalid (new_parent, TRUE);
        }
      else
        {
          ctk_css_node_parent_was_unset (node);
        }
    }

  if (new_parent)
    {
      g_signal_emit (new_parent, cssnode_signals[NODE_ADDED], 0, node, previous);
      if (node->visible)
        ctk_css_node_invalidate (new_parent->first_child, CTK_CSS_CHANGE_NTH_LAST_CHILD);
    }

  if (node->visible)
    {
      if (node->next_sibling)
        {
          if (node->previous_sibling == NULL)
            ctk_css_node_invalidate (node->next_sibling, CTK_CSS_CHANGE_FIRST_CHILD);
          else
            ctk_css_node_invalidate_style (node->next_sibling);
        }
      else if (node->previous_sibling)
        {
          ctk_css_node_invalidate (node->previous_sibling, CTK_CSS_CHANGE_LAST_CHILD);
        }
    }
  else
    {
      if (node->next_sibling)
        ctk_css_node_invalidate_style (node->next_sibling);
    }

  ctk_css_node_invalidate (node, CTK_CSS_CHANGE_ANY_PARENT
                                 | CTK_CSS_CHANGE_ANY_SIBLING
                                 | CTK_CSS_CHANGE_NTH_CHILD
                                 | (node->previous_sibling ? 0 : CTK_CSS_CHANGE_FIRST_CHILD)
                                 | (node->next_sibling ? 0 : CTK_CSS_CHANGE_LAST_CHILD));

  g_object_unref (node);
}

void
ctk_css_node_set_parent (GtkCssNode *node,
                         GtkCssNode *parent)
{
  if (node->parent == parent)
    return;

  ctk_css_node_reposition (node, parent, parent ? parent->last_child : NULL);
}

/* If previous_sibling is NULL, insert at the beginning */
void
ctk_css_node_insert_after (GtkCssNode *parent,
                           GtkCssNode *cssnode,
                           GtkCssNode *previous_sibling)
{
  g_return_if_fail (previous_sibling == NULL || previous_sibling->parent == parent);
  g_return_if_fail (cssnode != previous_sibling);

  if (cssnode->previous_sibling == previous_sibling &&
      cssnode->parent == parent)
    return;

  ctk_css_node_reposition (cssnode,
                           parent,
                           previous_sibling);
}

/* If next_sibling is NULL, insert at the end */
void
ctk_css_node_insert_before (GtkCssNode *parent,
                            GtkCssNode *cssnode,
                            GtkCssNode *next_sibling)
{
  g_return_if_fail (next_sibling == NULL || next_sibling->parent == parent);
  g_return_if_fail (cssnode != next_sibling);

  if (cssnode->next_sibling == next_sibling && 
      cssnode->parent == parent)
    return;

  ctk_css_node_reposition (cssnode,
                           parent,
                           next_sibling ? next_sibling->previous_sibling : parent->last_child);
}

void
ctk_css_node_reverse_children (GtkCssNode *cssnode)
{
  GtkCssNode *end;

  end = cssnode->last_child;
  while (cssnode->first_child != end)
    {
      ctk_css_node_reposition (cssnode->first_child,
                               cssnode,
                               end);
    }

}

GtkCssNode *
ctk_css_node_get_parent (GtkCssNode *cssnode)
{
  return cssnode->parent;
}

GtkCssNode *
ctk_css_node_get_first_child (GtkCssNode *cssnode)
{
  return cssnode->first_child;
}

GtkCssNode *
ctk_css_node_get_last_child (GtkCssNode *cssnode)
{
  return cssnode->last_child;
}

GtkCssNode *
ctk_css_node_get_previous_sibling (GtkCssNode *cssnode)
{
  return cssnode->previous_sibling;
}

GtkCssNode *
ctk_css_node_get_next_sibling (GtkCssNode *cssnode)
{
  return cssnode->next_sibling;
}

static gboolean
ctk_css_node_set_style (GtkCssNode  *cssnode,
                        GtkCssStyle *style)
{
  GtkCssStyleChange change;
  gboolean style_changed;

  if (cssnode->style == style)
    return FALSE;

  ctk_css_style_change_init (&change, cssnode->style, style);

  style_changed = ctk_css_style_change_has_change (&change);
  if (style_changed)
    {
      g_signal_emit (cssnode, cssnode_signals[STYLE_CHANGED], 0, &change);
    }
  else if (cssnode->style != style &&
           (CTK_IS_CSS_ANIMATED_STYLE (cssnode->style) || CTK_IS_CSS_ANIMATED_STYLE (style)))
    {
      /* This is when animations are starting/stopping but they didn't change any CSS this frame */
      g_set_object (&cssnode->style, style);
    }

  ctk_css_style_change_finish (&change);

  return style_changed;
}

static void
ctk_css_node_propagate_pending_changes (GtkCssNode *cssnode,
                                        gboolean    style_changed)
{
  GtkCssChange change, child_change;
  GtkCssNode *child;

  change = _ctk_css_change_for_child (cssnode->pending_changes);
  if (style_changed)
    change |= CTK_CSS_CHANGE_PARENT_STYLE;

  if (!cssnode->needs_propagation && change == 0)
    return;

  for (child = ctk_css_node_get_first_child (cssnode);
       child;
       child = ctk_css_node_get_next_sibling (child))
    {
      child_change = child->pending_changes;
      ctk_css_node_invalidate (child, change);
      if (child->visible)
        change |= _ctk_css_change_for_sibling (child_change);
    }

  cssnode->needs_propagation = FALSE;
}

static gboolean
ctk_css_node_needs_new_style (GtkCssNode *cssnode)
{
  return cssnode->style_is_invalid || cssnode->needs_propagation;
}

static void
ctk_css_node_ensure_style (GtkCssNode *cssnode,
                           gint64      current_time)
{
  gboolean style_changed;

  if (!ctk_css_node_needs_new_style (cssnode))
    return;

  if (cssnode->parent)
    ctk_css_node_ensure_style (cssnode->parent, current_time);

  if (cssnode->style_is_invalid)
    {
      GtkCssStyle *new_style;

      if (cssnode->previous_sibling)
        ctk_css_node_ensure_style (cssnode->previous_sibling, current_time);

      g_clear_pointer (&cssnode->cache, ctk_css_node_style_cache_unref);

      new_style = CTK_CSS_NODE_GET_CLASS (cssnode)->update_style (cssnode,
                                                                  cssnode->pending_changes,
                                                                  current_time,
                                                                  cssnode->style);

      style_changed = ctk_css_node_set_style (cssnode, new_style);
      g_object_unref (new_style);
    }
  else
    {
      style_changed = FALSE;
    }

  ctk_css_node_propagate_pending_changes (cssnode, style_changed);

  cssnode->pending_changes = 0;
  cssnode->style_is_invalid = FALSE;
}

GtkCssStyle *
ctk_css_node_get_style (GtkCssNode *cssnode)
{
  if (ctk_css_node_needs_new_style (cssnode))
    {
      gint64 timestamp = ctk_css_node_get_timestamp (cssnode);

      ctk_css_node_ensure_style (cssnode, timestamp);
    }

  return cssnode->style;
}

void
ctk_css_node_set_visible (GtkCssNode *cssnode,
                          gboolean    visible)
{
  GtkCssNode *iter;

  if (cssnode->visible == visible)
    return;

  cssnode->visible = visible;
  g_object_notify_by_pspec (G_OBJECT (cssnode), cssnode_properties[PROP_VISIBLE]);

  if (cssnode->invalid)
    {
      if (cssnode->visible)
        {
          if (cssnode->parent)
            ctk_css_node_set_invalid (cssnode->parent, TRUE);
          else
            CTK_CSS_NODE_GET_CLASS (cssnode)->queue_validate (cssnode);
        }
      else
        {
          if (cssnode->parent == NULL)
            CTK_CSS_NODE_GET_CLASS (cssnode)->dequeue_validate (cssnode);
        }
    }

  if (cssnode->next_sibling)
    {
      ctk_css_node_invalidate (cssnode->next_sibling, CTK_CSS_CHANGE_ANY_SIBLING | CTK_CSS_CHANGE_NTH_CHILD);
      if (ctk_css_node_is_first_child (cssnode))
        {
          for (iter = cssnode->next_sibling;
               iter != NULL;
               iter = iter->next_sibling)
            {
              ctk_css_node_invalidate (iter, CTK_CSS_CHANGE_FIRST_CHILD);
              if (iter->visible)
                break;
            }
        }
    }
               
  if (cssnode->previous_sibling)
    {
      if (ctk_css_node_is_last_child (cssnode))
        {
          for (iter = cssnode->previous_sibling;
               iter != NULL;
               iter = iter->previous_sibling)
            {
              ctk_css_node_invalidate (iter, CTK_CSS_CHANGE_LAST_CHILD);
              if (iter->visible)
                break;
            }
        }
      ctk_css_node_invalidate (cssnode->parent->first_child, CTK_CSS_CHANGE_NTH_LAST_CHILD);
    }
}

gboolean
ctk_css_node_get_visible (GtkCssNode *cssnode)
{
  return cssnode->visible;
}

void
ctk_css_node_set_name (GtkCssNode              *cssnode,
                       /*interned*/ const char *name)
{
  if (ctk_css_node_declaration_set_name (&cssnode->decl, name))
    {
      ctk_css_node_invalidate (cssnode, CTK_CSS_CHANGE_NAME);
      g_object_notify_by_pspec (G_OBJECT (cssnode), cssnode_properties[PROP_NAME]);
    }
}

/* interned */ const char *
ctk_css_node_get_name (GtkCssNode *cssnode)
{
  return ctk_css_node_declaration_get_name (cssnode->decl);
}

void
ctk_css_node_set_widget_type (GtkCssNode *cssnode,
                              GType       widget_type)
{
  if (ctk_css_node_declaration_set_type (&cssnode->decl, widget_type))
    {
      ctk_css_node_invalidate (cssnode, CTK_CSS_CHANGE_NAME);
      g_object_notify_by_pspec (G_OBJECT (cssnode), cssnode_properties[PROP_WIDGET_TYPE]);
    }
}

GType
ctk_css_node_get_widget_type (GtkCssNode *cssnode)
{
  return ctk_css_node_declaration_get_type (cssnode->decl);
}

void
ctk_css_node_set_id (GtkCssNode                *cssnode,
                     /* interned */ const char *id)
{
  if (ctk_css_node_declaration_set_id (&cssnode->decl, id))
    {
      ctk_css_node_invalidate (cssnode, CTK_CSS_CHANGE_ID);
      g_object_notify_by_pspec (G_OBJECT (cssnode), cssnode_properties[PROP_ID]);
    }
}

/* interned */ const char *
ctk_css_node_get_id (GtkCssNode *cssnode)
{
  return ctk_css_node_declaration_get_id (cssnode->decl);
}

void
ctk_css_node_set_state (GtkCssNode    *cssnode,
                        GtkStateFlags  state_flags)
{
  if (ctk_css_node_declaration_set_state (&cssnode->decl, state_flags))
    {
      ctk_css_node_invalidate (cssnode, CTK_CSS_CHANGE_STATE);
      g_object_notify_by_pspec (G_OBJECT (cssnode), cssnode_properties[PROP_STATE]);
    }
}

GtkStateFlags
ctk_css_node_get_state (GtkCssNode *cssnode)
{
  return ctk_css_node_declaration_get_state (cssnode->decl);
}

void
ctk_css_node_set_junction_sides (GtkCssNode       *cssnode,
                                 GtkJunctionSides  junction_sides)
{
  ctk_css_node_declaration_set_junction_sides (&cssnode->decl, junction_sides);
}

GtkJunctionSides
ctk_css_node_get_junction_sides (GtkCssNode *cssnode)
{
  return ctk_css_node_declaration_get_junction_sides (cssnode->decl);
}

static void
ctk_css_node_clear_classes (GtkCssNode *cssnode)
{
  if (ctk_css_node_declaration_clear_classes (&cssnode->decl))
    {
      ctk_css_node_invalidate (cssnode, CTK_CSS_CHANGE_CLASS);
      g_object_notify_by_pspec (G_OBJECT (cssnode), cssnode_properties[PROP_CLASSES]);
    }
}

void
ctk_css_node_set_classes (GtkCssNode  *cssnode,
                          const char **classes)
{
  guint i;

  g_object_freeze_notify (G_OBJECT (cssnode));

  ctk_css_node_clear_classes (cssnode);

  if (classes)
    {
      for (i = 0; classes[i] != NULL; i++)
        {
          ctk_css_node_add_class (cssnode, g_quark_from_string (classes[i]));
        }
    }

  g_object_thaw_notify (G_OBJECT (cssnode));
}

char **
ctk_css_node_get_classes (GtkCssNode *cssnode)
{
  const GQuark *classes;
  char **result;
  guint n_classes, i, j;

  classes = ctk_css_node_declaration_get_classes (cssnode->decl, &n_classes);
  result = g_new (char *, n_classes + 1);

  for (i = n_classes, j = 0; i-- > 0; ++j)
    {
      result[j] = g_strdup (g_quark_to_string (classes[i]));
    }

  result[n_classes] = NULL;
  return result;
}

void
ctk_css_node_add_class (GtkCssNode *cssnode,
                        GQuark      style_class)
{
  if (ctk_css_node_declaration_add_class (&cssnode->decl, style_class))
    {
      ctk_css_node_invalidate (cssnode, CTK_CSS_CHANGE_CLASS);
      g_object_notify_by_pspec (G_OBJECT (cssnode), cssnode_properties[PROP_CLASSES]);
    }
}

void
ctk_css_node_remove_class (GtkCssNode *cssnode,
                           GQuark      style_class)
{
  if (ctk_css_node_declaration_remove_class (&cssnode->decl, style_class))
    {
      ctk_css_node_invalidate (cssnode, CTK_CSS_CHANGE_CLASS);
      g_object_notify_by_pspec (G_OBJECT (cssnode), cssnode_properties[PROP_CLASSES]);
    }
}

gboolean
ctk_css_node_has_class (GtkCssNode *cssnode,
                        GQuark      style_class)
{
  return ctk_css_node_declaration_has_class (cssnode->decl, style_class);
}

const GQuark *
ctk_css_node_list_classes (GtkCssNode *cssnode,
                           guint      *n_classes)
{
  return ctk_css_node_declaration_get_classes (cssnode->decl, n_classes);
}

void
ctk_css_node_add_region (GtkCssNode     *cssnode,
                         GQuark          region,
                         GtkRegionFlags  flags)
{
  ctk_css_node_declaration_add_region (&cssnode->decl, region, flags);
}

void
ctk_css_node_remove_region (GtkCssNode *cssnode,
                            GQuark      region)
{
  ctk_css_node_declaration_remove_region (&cssnode->decl, region);
}

gboolean
ctk_css_node_has_region (GtkCssNode     *cssnode,
                         GQuark          region,
                         GtkRegionFlags *out_flags)
{
  return ctk_css_node_declaration_has_region (cssnode->decl, region, out_flags);
}

GList *
ctk_css_node_list_regions (GtkCssNode *cssnode)
{
  return ctk_css_node_declaration_list_regions (cssnode->decl);
}


const GtkCssNodeDeclaration *
ctk_css_node_get_declaration (GtkCssNode *cssnode)
{
  return cssnode->decl;
}

void
ctk_css_node_invalidate_style_provider (GtkCssNode *cssnode)
{
  GtkCssNode *child;

  ctk_css_node_invalidate (cssnode, CTK_CSS_CHANGE_SOURCE);

  for (child = cssnode->first_child;
       child;
       child = child->next_sibling)
    {
      if (ctk_css_node_get_style_provider_or_null (child) == NULL)
        ctk_css_node_invalidate_style_provider (child);
    }
}

static void
ctk_css_node_invalidate_timestamp (GtkCssNode *cssnode)
{
  GtkCssNode *child;

  if (!cssnode->invalid)
    return;

  if (!ctk_css_style_is_static (cssnode->style))
    ctk_css_node_invalidate (cssnode, CTK_CSS_CHANGE_TIMESTAMP);

  for (child = cssnode->first_child; child; child = child->next_sibling)
    {
      ctk_css_node_invalidate_timestamp (child);
    }
}

void
ctk_css_node_invalidate_frame_clock (GtkCssNode *cssnode,
                                     gboolean    just_timestamp)
{
  /* frame clock is handled by the top level */
  if (cssnode->parent)
    return;

  ctk_css_node_invalidate_timestamp (cssnode);

  if (!just_timestamp)
    ctk_css_node_invalidate (cssnode, CTK_CSS_CHANGE_ANIMATIONS);
}

void
ctk_css_node_invalidate (GtkCssNode   *cssnode,
                         GtkCssChange  change)
{
  if (!cssnode->invalid)
    change &= ~CTK_CSS_CHANGE_TIMESTAMP;

  if (change == 0)
    return;

  cssnode->pending_changes |= change;

  CTK_CSS_NODE_GET_CLASS (cssnode)->invalidate (cssnode);

  if (cssnode->parent)
    cssnode->parent->needs_propagation = TRUE;
  ctk_css_node_invalidate_style (cssnode);
}

void
ctk_css_node_validate_internal (GtkCssNode *cssnode,
                                gint64      timestamp)
{
  GtkCssNode *child;

  if (!cssnode->invalid)
    return;

  ctk_css_node_ensure_style (cssnode, timestamp);

  /* need to set to FALSE then to TRUE here to make it chain up */
  ctk_css_node_set_invalid (cssnode, FALSE);
  if (!ctk_css_style_is_static (cssnode->style))
    ctk_css_node_set_invalid (cssnode, TRUE);

  CTK_CSS_NODE_GET_CLASS (cssnode)->validate (cssnode);

  for (child = ctk_css_node_get_first_child (cssnode);
       child;
       child = ctk_css_node_get_next_sibling (child))
    {
      if (child->visible)
        ctk_css_node_validate_internal (child, timestamp);
    }
}

void
ctk_css_node_validate (GtkCssNode *cssnode)
{
  gint64 timestamp;

  timestamp = ctk_css_node_get_timestamp (cssnode);

  ctk_css_node_validate_internal (cssnode, timestamp);
}

gboolean
ctk_css_node_init_matcher (GtkCssNode     *cssnode,
                           GtkCssMatcher  *matcher)
{
  return CTK_CSS_NODE_GET_CLASS (cssnode)->init_matcher (cssnode, matcher);
}

GtkWidgetPath *
ctk_css_node_create_widget_path (GtkCssNode *cssnode)
{
  return CTK_CSS_NODE_GET_CLASS (cssnode)->create_widget_path (cssnode);
}

const GtkWidgetPath *
ctk_css_node_get_widget_path (GtkCssNode *cssnode)
{
  return CTK_CSS_NODE_GET_CLASS (cssnode)->get_widget_path (cssnode);
}

GtkStyleProviderPrivate *
ctk_css_node_get_style_provider (GtkCssNode *cssnode)
{
  GtkStyleProviderPrivate *result;
  GtkSettings *settings;

  result = ctk_css_node_get_style_provider_or_null (cssnode);
  if (result)
    return result;

  if (cssnode->parent)
    return ctk_css_node_get_style_provider (cssnode->parent);

  settings = ctk_settings_get_default ();
  if (!settings)
    return NULL;

  return CTK_STYLE_PROVIDER_PRIVATE (_ctk_settings_get_style_cascade (settings, 1));
}

void
ctk_css_node_print (GtkCssNode                *cssnode,
                    GtkStyleContextPrintFlags  flags,
                    GString                   *string,
                    guint                      indent)
{
  gboolean need_newline = FALSE;

  g_string_append_printf (string, "%*s", indent, "");

  if (!cssnode->visible)
    g_string_append_c (string, '[');

  ctk_css_node_declaration_print (cssnode->decl, string);

  if (!cssnode->visible)
    g_string_append_c (string, ']');

  g_string_append_c (string, '\n');

  if (flags & CTK_STYLE_CONTEXT_PRINT_SHOW_STYLE)
    need_newline = ctk_css_style_print (ctk_css_node_get_style (cssnode), string, indent + 2, TRUE);

  if (flags & CTK_STYLE_CONTEXT_PRINT_RECURSE)
    {
      GtkCssNode *node;

      if (need_newline && ctk_css_node_get_first_child (cssnode))
        g_string_append_c (string, '\n');

      for (node = ctk_css_node_get_first_child (cssnode); node; node = ctk_css_node_get_next_sibling (node))
        ctk_css_node_print (node, flags, string, indent + 2);
    }
}
