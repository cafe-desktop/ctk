/*
 * Copyright (c) 2014 Intel Corporation
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
 * Author:
 *      Ikey Doherty <michael.i.doherty@intel.com>
 */

#ifndef __CTK_STACK_SIDEBAR_H__
#define __CTK_STACK_SIDEBAR_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <gtk/gtk.h> can be included directly."
#endif

#include <gtk/gtkbin.h>
#include <gtk/gtkstack.h>

G_BEGIN_DECLS

#define CTK_TYPE_STACK_SIDEBAR           (ctk_stack_sidebar_get_type ())
#define CTK_STACK_SIDEBAR(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_STACK_SIDEBAR, GtkStackSidebar))
#define CTK_IS_STACK_SIDEBAR(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_STACK_SIDEBAR))
#define CTK_STACK_SIDEBAR_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_STACK_SIDEBAR, GtkStackSidebarClass))
#define CTK_IS_STACK_SIDEBAR_CLASS(klass)(G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_STACK_SIDEBAR))
#define CTK_STACK_SIDEBAR_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_STACK_SIDEBAR, GtkStackSidebarClass))

typedef struct _GtkStackSidebar        GtkStackSidebar;
typedef struct _GtkStackSidebarPrivate GtkStackSidebarPrivate;
typedef struct _GtkStackSidebarClass   GtkStackSidebarClass;

struct _GtkStackSidebar
{
  GtkBin parent;
};

struct _GtkStackSidebarClass
{
  GtkBinClass parent_class;

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

GDK_AVAILABLE_IN_3_16
GType       ctk_stack_sidebar_get_type  (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_3_16
GtkWidget * ctk_stack_sidebar_new       (void);
GDK_AVAILABLE_IN_3_16
void        ctk_stack_sidebar_set_stack (GtkStackSidebar *sidebar,
                                         GtkStack        *stack);
GDK_AVAILABLE_IN_3_16
GtkStack *  ctk_stack_sidebar_get_stack (GtkStackSidebar *sidebar);

G_END_DECLS

#endif /* __CTK_STACK_SIDEBAR_H__ */