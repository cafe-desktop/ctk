/* GTK - The GIMP Toolkit
 * Copyright (C) 2000 Red Hat Software
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

#ifndef __CTK_IM_CONTEXT_SIMPLE_H__
#define __CTK_IM_CONTEXT_SIMPLE_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <gtk/gtk.h> can be included directly."
#endif

#include <gtk/gtkimcontext.h>


G_BEGIN_DECLS

/**
 * CTK_MAX_COMPOSE_LEN:
 *
 * The maximum length of sequences in compose tables.
 */
#define CTK_MAX_COMPOSE_LEN 7

#define CTK_TYPE_IM_CONTEXT_SIMPLE              (ctk_im_context_simple_get_type ())
#define CTK_IM_CONTEXT_SIMPLE(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_IM_CONTEXT_SIMPLE, GtkIMContextSimple))
#define CTK_IM_CONTEXT_SIMPLE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_IM_CONTEXT_SIMPLE, GtkIMContextSimpleClass))
#define CTK_IS_IM_CONTEXT_SIMPLE(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_IM_CONTEXT_SIMPLE))
#define CTK_IS_IM_CONTEXT_SIMPLE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_IM_CONTEXT_SIMPLE))
#define CTK_IM_CONTEXT_SIMPLE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_IM_CONTEXT_SIMPLE, GtkIMContextSimpleClass))


typedef struct _GtkIMContextSimple              GtkIMContextSimple;
typedef struct _GtkIMContextSimplePrivate       GtkIMContextSimplePrivate;
typedef struct _GtkIMContextSimpleClass         GtkIMContextSimpleClass;

struct _GtkIMContextSimple
{
  GtkIMContext object;

  /*< private >*/
  GtkIMContextSimplePrivate *priv;
};

struct _GtkIMContextSimpleClass
{
  GtkIMContextClass parent_class;
};

GDK_AVAILABLE_IN_ALL
GType         ctk_im_context_simple_get_type  (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
GtkIMContext *ctk_im_context_simple_new       (void);

GDK_AVAILABLE_IN_ALL
void          ctk_im_context_simple_add_table (GtkIMContextSimple *context_simple,
					       guint16            *data,
					       gint                max_seq_len,
					       gint                n_seqs);
GDK_AVAILABLE_IN_3_20
void          ctk_im_context_simple_add_compose_file (GtkIMContextSimple *context_simple,
                                                      const gchar        *compose_file);


G_END_DECLS


#endif /* __CTK_IM_CONTEXT_SIMPLE_H__ */
