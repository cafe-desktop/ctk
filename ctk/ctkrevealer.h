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

#ifndef __CTK_REVEALER_H__
#define __CTK_REVEALER_H__

#include <ctk/ctkbin.h>

G_BEGIN_DECLS


#define CTK_TYPE_REVEALER (ctk_revealer_get_type ())
#define CTK_REVEALER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_REVEALER, CtkRevealer))
#define CTK_REVEALER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_REVEALER, CtkRevealerClass))
#define CTK_IS_REVEALER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_REVEALER))
#define CTK_IS_REVEALER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_REVEALER))
#define CTK_REVEALER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_REVEALER, CtkRevealerClass))

typedef struct _CtkRevealer CtkRevealer;
typedef struct _CtkRevealerClass CtkRevealerClass;

typedef enum {
  CTK_REVEALER_TRANSITION_TYPE_NONE,
  CTK_REVEALER_TRANSITION_TYPE_CROSSFADE,
  CTK_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT,
  CTK_REVEALER_TRANSITION_TYPE_SLIDE_LEFT,
  CTK_REVEALER_TRANSITION_TYPE_SLIDE_UP,
  CTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN
} CtkRevealerTransitionType;

struct _CtkRevealer {
  CtkBin parent_instance;
};

/**
 * CtkRevealerClass:
 * @parent_class: The parent class.
 */
struct _CtkRevealerClass {
  CtkBinClass parent_class;
};

GDK_AVAILABLE_IN_3_10
GType                      ctk_revealer_get_type                (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_3_10
CtkWidget*                 ctk_revealer_new                     (void);
GDK_AVAILABLE_IN_3_10
gboolean                   ctk_revealer_get_reveal_child        (CtkRevealer               *revealer);
GDK_AVAILABLE_IN_3_10
void                       ctk_revealer_set_reveal_child        (CtkRevealer               *revealer,
                                                                 gboolean                   reveal_child);
GDK_AVAILABLE_IN_3_10
gboolean                   ctk_revealer_get_child_revealed      (CtkRevealer               *revealer);
GDK_AVAILABLE_IN_3_10
guint                      ctk_revealer_get_transition_duration (CtkRevealer               *revealer);
GDK_AVAILABLE_IN_3_10
void                       ctk_revealer_set_transition_duration (CtkRevealer               *revealer,
                                                                 guint                      duration);
GDK_AVAILABLE_IN_3_10
void                       ctk_revealer_set_transition_type     (CtkRevealer               *revealer,
                                                                 CtkRevealerTransitionType  transition);
GDK_AVAILABLE_IN_3_10
CtkRevealerTransitionType  ctk_revealer_get_transition_type     (CtkRevealer               *revealer);


G_END_DECLS

#endif
