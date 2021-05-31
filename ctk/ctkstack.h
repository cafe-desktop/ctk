/*
 * Copyright (c) 2013 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Alexander Larsson <alexl@redhat.com>
 *
 */

#ifndef __CTK_STACK_H__
#define __CTK_STACK_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkcontainer.h>

G_BEGIN_DECLS


#define CTK_TYPE_STACK (ctk_stack_get_type ())
#define CTK_STACK(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_STACK, GtkStack))
#define CTK_STACK_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_STACK, GtkStackClass))
#define CTK_IS_STACK(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_STACK))
#define CTK_IS_STACK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_STACK))
#define CTK_STACK_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_STACK, GtkStackClass))

typedef struct _GtkStack GtkStack;
typedef struct _GtkStackClass GtkStackClass;

typedef enum {
  CTK_STACK_TRANSITION_TYPE_NONE,
  CTK_STACK_TRANSITION_TYPE_CROSSFADE,
  CTK_STACK_TRANSITION_TYPE_SLIDE_RIGHT,
  CTK_STACK_TRANSITION_TYPE_SLIDE_LEFT,
  CTK_STACK_TRANSITION_TYPE_SLIDE_UP,
  CTK_STACK_TRANSITION_TYPE_SLIDE_DOWN,
  CTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT,
  CTK_STACK_TRANSITION_TYPE_SLIDE_UP_DOWN,
  CTK_STACK_TRANSITION_TYPE_OVER_UP,
  CTK_STACK_TRANSITION_TYPE_OVER_DOWN,
  CTK_STACK_TRANSITION_TYPE_OVER_LEFT,
  CTK_STACK_TRANSITION_TYPE_OVER_RIGHT,
  CTK_STACK_TRANSITION_TYPE_UNDER_UP,
  CTK_STACK_TRANSITION_TYPE_UNDER_DOWN,
  CTK_STACK_TRANSITION_TYPE_UNDER_LEFT,
  CTK_STACK_TRANSITION_TYPE_UNDER_RIGHT,
  CTK_STACK_TRANSITION_TYPE_OVER_UP_DOWN,
  CTK_STACK_TRANSITION_TYPE_OVER_DOWN_UP,
  CTK_STACK_TRANSITION_TYPE_OVER_LEFT_RIGHT,
  CTK_STACK_TRANSITION_TYPE_OVER_RIGHT_LEFT
} GtkStackTransitionType;

struct _GtkStack {
  GtkContainer parent_instance;
};

struct _GtkStackClass {
  GtkContainerClass parent_class;
};

GDK_AVAILABLE_IN_3_10
GType                  ctk_stack_get_type                (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_3_10
GtkWidget *            ctk_stack_new                     (void);
GDK_AVAILABLE_IN_3_10
void                   ctk_stack_add_named               (GtkStack               *stack,
                                                          GtkWidget              *child,
                                                          const gchar            *name);
GDK_AVAILABLE_IN_3_10
void                   ctk_stack_add_titled              (GtkStack               *stack,
                                                          GtkWidget              *child,
                                                          const gchar            *name,
                                                          const gchar            *title);
GDK_AVAILABLE_IN_3_12
GtkWidget *            ctk_stack_get_child_by_name       (GtkStack               *stack,
                                                          const gchar            *name);
GDK_AVAILABLE_IN_3_10
void                   ctk_stack_set_visible_child       (GtkStack               *stack,
                                                          GtkWidget              *child);
GDK_AVAILABLE_IN_3_10
GtkWidget *            ctk_stack_get_visible_child       (GtkStack               *stack);
GDK_AVAILABLE_IN_3_10
void                   ctk_stack_set_visible_child_name  (GtkStack               *stack,
                                                          const gchar            *name);
GDK_AVAILABLE_IN_3_10
const gchar *          ctk_stack_get_visible_child_name  (GtkStack               *stack);
GDK_AVAILABLE_IN_3_10
void                   ctk_stack_set_visible_child_full  (GtkStack               *stack,
                                                          const gchar            *name,
                                                          GtkStackTransitionType  transition);
GDK_AVAILABLE_IN_3_10
void                   ctk_stack_set_homogeneous         (GtkStack               *stack,
                                                          gboolean                homogeneous);
GDK_AVAILABLE_IN_3_10
gboolean               ctk_stack_get_homogeneous         (GtkStack               *stack);
GDK_AVAILABLE_IN_3_16
void                   ctk_stack_set_hhomogeneous        (GtkStack               *stack,
                                                          gboolean                hhomogeneous);
GDK_AVAILABLE_IN_3_16
gboolean               ctk_stack_get_hhomogeneous        (GtkStack               *stack);
GDK_AVAILABLE_IN_3_16
void                   ctk_stack_set_vhomogeneous        (GtkStack               *stack,
                                                          gboolean                vhomogeneous);
GDK_AVAILABLE_IN_3_16
gboolean               ctk_stack_get_vhomogeneous        (GtkStack               *stack);
GDK_AVAILABLE_IN_3_10
void                   ctk_stack_set_transition_duration (GtkStack               *stack,
                                                          guint                   duration);
GDK_AVAILABLE_IN_3_10
guint                  ctk_stack_get_transition_duration (GtkStack               *stack);
GDK_AVAILABLE_IN_3_10
void                   ctk_stack_set_transition_type     (GtkStack               *stack,
                                                          GtkStackTransitionType  transition);
GDK_AVAILABLE_IN_3_10
GtkStackTransitionType ctk_stack_get_transition_type     (GtkStack               *stack);
GDK_AVAILABLE_IN_3_12
gboolean               ctk_stack_get_transition_running  (GtkStack               *stack);
GDK_AVAILABLE_IN_3_18
void                   ctk_stack_set_interpolate_size    (GtkStack *stack,
                                                          gboolean  interpolate_size);
GDK_AVAILABLE_IN_3_18
gboolean               ctk_stack_get_interpolate_size    (GtkStack *stack);
G_END_DECLS

#endif
