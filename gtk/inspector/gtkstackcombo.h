/*
 * Copyright (c) 2016 Red Hat, Inc.
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
 */

#ifndef __CTK_STACK_COMBO_H__
#define __CTK_STACK_COMBO_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <gtk/gtk.h> can be included directly."
#endif

#include <gtk/gtkcombobox.h>
#include <gtk/gtkstack.h>

G_BEGIN_DECLS

#define CTK_TYPE_STACK_COMBO            (ctk_stack_combo_get_type ())
#define CTK_STACK_COMBO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_STACK_COMBO, GtkStackCombo))
#define CTK_STACK_COMBO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_STACK_COMBO, GtkStackComboClass))
#define CTK_IS_STACK_COMBO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_STACK_COMBO))
#define CTK_IS_STACK_COMBO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_STACK_COMBO))
#define CTK_STACK_COMBO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_STACK_COMBO, GtkStackComboClass))

typedef struct _GtkStackCombo              GtkStackCombo;
typedef struct _GtkStackComboClass         GtkStackComboClass;

GType        ctk_stack_combo_get_type          (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __CTK_STACK_COMBO_H__ */
