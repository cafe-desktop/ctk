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

#ifndef __CTK_IM_MULTICONTEXT_H__
#define __CTK_IM_MULTICONTEXT_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <gtk/gtk.h> can be included directly."
#endif

#include <gtk/gtkimcontext.h>
#include <gtk/gtkmenushell.h>

G_BEGIN_DECLS

#define CTK_TYPE_IM_MULTICONTEXT              (ctk_im_multicontext_get_type ())
#define CTK_IM_MULTICONTEXT(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_IM_MULTICONTEXT, GtkIMMulticontext))
#define CTK_IM_MULTICONTEXT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_IM_MULTICONTEXT, GtkIMMulticontextClass))
#define CTK_IS_IM_MULTICONTEXT(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_IM_MULTICONTEXT))
#define CTK_IS_IM_MULTICONTEXT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_IM_MULTICONTEXT))
#define CTK_IM_MULTICONTEXT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_IM_MULTICONTEXT, GtkIMMulticontextClass))


typedef struct _GtkIMMulticontext        GtkIMMulticontext;
typedef struct _GtkIMMulticontextClass   GtkIMMulticontextClass;
typedef struct _GtkIMMulticontextPrivate GtkIMMulticontextPrivate;

struct _GtkIMMulticontext
{
  GtkIMContext object;

  /*< private >*/
  GtkIMMulticontextPrivate *priv;
};

struct _GtkIMMulticontextClass
{
  GtkIMContextClass parent_class;

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

GDK_AVAILABLE_IN_ALL
GType         ctk_im_multicontext_get_type (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
GtkIMContext *ctk_im_multicontext_new      (void);

GDK_DEPRECATED_IN_3_10
void          ctk_im_multicontext_append_menuitems (GtkIMMulticontext *context,
						    GtkMenuShell      *menushell);
GDK_AVAILABLE_IN_ALL
const char  * ctk_im_multicontext_get_context_id   (GtkIMMulticontext *context);

GDK_AVAILABLE_IN_ALL
void          ctk_im_multicontext_set_context_id   (GtkIMMulticontext *context,
                                                    const char        *context_id);
 
G_END_DECLS

#endif /* __CTK_IM_MULTICONTEXT_H__ */
