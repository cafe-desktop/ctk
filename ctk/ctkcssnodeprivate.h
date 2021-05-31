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

#ifndef __CTK_CSS_NODE_PRIVATE_H__
#define __CTK_CSS_NODE_PRIVATE_H__

#include "ctkcssnodedeclarationprivate.h"
#include "ctkcssnodestylecacheprivate.h"
#include "ctkcssstylechangeprivate.h"
#include "ctkbitmaskprivate.h"
#include "ctkcsstypesprivate.h"

G_BEGIN_DECLS

#define CTK_TYPE_CSS_NODE           (ctk_css_node_get_type ())
#define CTK_CSS_NODE(obj)           (G_TYPE_CHECK_INSTANCE_CAST (obj, CTK_TYPE_CSS_NODE, CtkCssNode))
#define CTK_CSS_NODE_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST (cls, CTK_TYPE_CSS_NODE, CtkCssNodeClass))
#define CTK_IS_CSS_NODE(obj)        (G_TYPE_CHECK_INSTANCE_TYPE (obj, CTK_TYPE_CSS_NODE))
#define CTK_IS_CSS_NODE_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE (obj, CTK_TYPE_CSS_NODE))
#define CTK_CSS_NODE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_CSS_NODE, CtkCssNodeClass))

typedef struct _CtkCssNodeClass         CtkCssNodeClass;

struct _CtkCssNode
{
  GObject object;

  CtkCssNode *parent;
  CtkCssNode *previous_sibling;
  CtkCssNode *next_sibling;
  CtkCssNode *first_child;
  CtkCssNode *last_child;

  CtkCssNodeDeclaration *decl;
  CtkCssStyle           *style;
  CtkCssNodeStyleCache  *cache;                 /* cache for children to look up styles */

  CtkCssChange           pending_changes;       /* changes that accumulated since the style was last computed */

  guint                  visible :1;            /* node will be skipped when validating or computing styles */
  guint                  invalid :1;            /* node or a child needs to be validated (even if just for animation) */
  guint                  needs_propagation :1;  /* children have state changes that need to be propagated to their siblings */
  /* Two invariants hold for this variable:
   * style_is_invalid == TRUE  =>  next_sibling->style_is_invalid == TRUE
   * style_is_invalid == FALSE =>  first_child->style_is_invalid == TRUE
   * So if a valid style is computed, one has to previously ensure that the parent's and the previous sibling's style
   * are valid. This allows both validation and invalidation to run in O(nodes-in-tree) */
  guint                  style_is_invalid :1;   /* the style needs to be recomputed */
};

struct _CtkCssNodeClass
{
  GObjectClass object_class;

  void                  (* node_added)                  (CtkCssNode            *cssnode,
                                                         CtkCssNode            *child,
                                                         CtkCssNode            *previous);
  void                  (* node_removed)                (CtkCssNode            *cssnode,
                                                         CtkCssNode            *child,
                                                         CtkCssNode            *previous);
  void                  (* style_changed)               (CtkCssNode            *cssnode,
                                                         CtkCssStyleChange     *style_change);

  gboolean              (* init_matcher)                (CtkCssNode            *cssnode,
                                                         CtkCssMatcher         *matcher);
  CtkWidgetPath *       (* create_widget_path)          (CtkCssNode            *cssnode);
  const CtkWidgetPath * (* get_widget_path)             (CtkCssNode            *cssnode);
  /* get style provider to use or NULL to use parent's */
  CtkStyleProviderPrivate *(* get_style_provider)       (CtkCssNode            *cssnode);
  /* get frame clock or NULL (only relevant for root node) */
  GdkFrameClock *       (* get_frame_clock)             (CtkCssNode            *cssnode);
  CtkCssStyle *         (* update_style)                (CtkCssNode            *cssnode,
                                                         CtkCssChange           pending_changes,
                                                         gint64                 timestamp,
                                                         CtkCssStyle           *old_style);
  void                  (* invalidate)                  (CtkCssNode            *node);
  void                  (* queue_validate)              (CtkCssNode            *node);
  void                  (* dequeue_validate)            (CtkCssNode            *node);
  void                  (* validate)                    (CtkCssNode            *node);
};

GType                   ctk_css_node_get_type           (void) G_GNUC_CONST;

CtkCssNode *            ctk_css_node_new                (void);

void                    ctk_css_node_set_parent         (CtkCssNode            *cssnode,
                                                         CtkCssNode            *parent);
void                    ctk_css_node_insert_after       (CtkCssNode            *parent,
                                                         CtkCssNode            *cssnode,
                                                         CtkCssNode            *previous_sibling);
void                    ctk_css_node_insert_before      (CtkCssNode            *parent,
                                                         CtkCssNode            *cssnode,
                                                         CtkCssNode            *next_sibling);
void                    ctk_css_node_reverse_children   (CtkCssNode            *cssnode);

CtkCssNode *            ctk_css_node_get_parent         (CtkCssNode            *cssnode);
CtkCssNode *            ctk_css_node_get_first_child    (CtkCssNode            *cssnode);
CtkCssNode *            ctk_css_node_get_last_child     (CtkCssNode            *cssnode);
CtkCssNode *            ctk_css_node_get_previous_sibling(CtkCssNode           *cssnode);
CtkCssNode *            ctk_css_node_get_next_sibling   (CtkCssNode            *cssnode);

void                    ctk_css_node_set_visible        (CtkCssNode            *cssnode,
                                                         gboolean               visible);
gboolean                ctk_css_node_get_visible        (CtkCssNode            *cssnode);

void                    ctk_css_node_set_name           (CtkCssNode            *cssnode,
                                                         /*interned*/const char*name);
/*interned*/const char *ctk_css_node_get_name           (CtkCssNode            *cssnode);
void                    ctk_css_node_set_widget_type    (CtkCssNode            *cssnode,
                                                         GType                  widget_type);
GType                   ctk_css_node_get_widget_type    (CtkCssNode            *cssnode);
void                    ctk_css_node_set_id             (CtkCssNode            *cssnode,
                                                         /*interned*/const char*id);
/*interned*/const char *ctk_css_node_get_id             (CtkCssNode            *cssnode);
void                    ctk_css_node_set_state          (CtkCssNode            *cssnode,
                                                         CtkStateFlags          state_flags);
CtkStateFlags           ctk_css_node_get_state          (CtkCssNode            *cssnode);
void                    ctk_css_node_set_junction_sides (CtkCssNode            *cssnode,
                                                         CtkJunctionSides       junction_sides);
CtkJunctionSides        ctk_css_node_get_junction_sides (CtkCssNode            *cssnode);
void                    ctk_css_node_set_classes        (CtkCssNode            *cssnode,
                                                         const char           **classes);
char **                 ctk_css_node_get_classes        (CtkCssNode            *cssnode);
void                    ctk_css_node_add_class          (CtkCssNode            *cssnode,
                                                         GQuark                 style_class);
void                    ctk_css_node_remove_class       (CtkCssNode            *cssnode,
                                                         GQuark                 style_class);
gboolean                ctk_css_node_has_class          (CtkCssNode            *cssnode,
                                                         GQuark                 style_class);
const GQuark *          ctk_css_node_list_classes       (CtkCssNode            *cssnode,
                                                         guint                 *n_classes);
void                    ctk_css_node_add_region         (CtkCssNode            *cssnode,
                                                         GQuark                 region,
                                                         CtkRegionFlags         flags);
void                    ctk_css_node_remove_region      (CtkCssNode            *cssnode,
                                                         GQuark                 region);
gboolean                ctk_css_node_has_region         (CtkCssNode            *cssnode,
                                                         GQuark                 region,
                                                         CtkRegionFlags        *out_flags);
GList *                 ctk_css_node_list_regions       (CtkCssNode            *cssnode);

const CtkCssNodeDeclaration *
                        ctk_css_node_get_declaration    (CtkCssNode            *cssnode);
CtkCssStyle *           ctk_css_node_get_style          (CtkCssNode            *cssnode);


void                    ctk_css_node_invalidate_style_provider
                                                        (CtkCssNode            *cssnode);
void                    ctk_css_node_invalidate_frame_clock
                                                        (CtkCssNode            *cssnode,
                                                         gboolean               just_timestamp);
void                    ctk_css_node_invalidate         (CtkCssNode            *cssnode,
                                                         CtkCssChange           change);
void                    ctk_css_node_validate           (CtkCssNode            *cssnode);

gboolean                ctk_css_node_init_matcher       (CtkCssNode            *cssnode,
                                                         CtkCssMatcher         *matcher);
CtkWidgetPath *         ctk_css_node_create_widget_path (CtkCssNode            *cssnode);
const CtkWidgetPath *   ctk_css_node_get_widget_path    (CtkCssNode            *cssnode);
CtkStyleProviderPrivate *ctk_css_node_get_style_provider(CtkCssNode            *cssnode);

void                    ctk_css_node_print              (CtkCssNode                *cssnode,
                                                         CtkStyleContextPrintFlags  flags,
                                                         GString                   *string,
                                                         guint                      indent);

G_END_DECLS

#endif /* __CTK_CSS_NODE_PRIVATE_H__ */
