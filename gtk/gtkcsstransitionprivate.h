/*
 * Copyright © 2012 Red Hat Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Benjamin Otte <otte@gnome.org>
 */

#ifndef __CTK_CSS_TRANSITION_PRIVATE_H__
#define __CTK_CSS_TRANSITION_PRIVATE_H__

#include "gtkstyleanimationprivate.h"
#include "gtkprogresstrackerprivate.h"

G_BEGIN_DECLS

#define CTK_TYPE_CSS_TRANSITION           (_ctk_css_transition_get_type ())
#define CTK_CSS_TRANSITION(obj)           (G_TYPE_CHECK_INSTANCE_CAST (obj, CTK_TYPE_CSS_TRANSITION, GtkCssTransition))
#define CTK_CSS_TRANSITION_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST (cls, CTK_TYPE_CSS_TRANSITION, GtkCssTransitionClass))
#define CTK_IS_CSS_TRANSITION(obj)        (G_TYPE_CHECK_INSTANCE_TYPE (obj, CTK_TYPE_CSS_TRANSITION))
#define CTK_IS_CSS_TRANSITION_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE (obj, CTK_TYPE_CSS_TRANSITION))
#define CTK_CSS_TRANSITION_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_CSS_TRANSITION, GtkCssTransitionClass))

typedef struct _GtkCssTransition           GtkCssTransition;
typedef struct _GtkCssTransitionClass      GtkCssTransitionClass;

struct _GtkCssTransition
{
  GtkStyleAnimation parent;

  guint               property;
  GtkCssValue        *start;
  GtkCssValue        *ease;
  GtkProgressTracker  tracker;
};

struct _GtkCssTransitionClass
{
  GtkStyleAnimationClass parent_class;
};

GType                   _ctk_css_transition_get_type        (void) G_GNUC_CONST;

GtkStyleAnimation *     _ctk_css_transition_new             (guint               property,
                                                             GtkCssValue        *start,
                                                             GtkCssValue        *ease,
                                                             gint64              timestamp,
                                                             gint64              duration_us,
                                                             gint64              delay_us);

guint                   _ctk_css_transition_get_property    (GtkCssTransition   *transition);

G_END_DECLS

#endif /* __CTK_CSS_TRANSITION_PRIVATE_H__ */
