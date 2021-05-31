/* CTK - The GIMP Toolkit
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
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkimcontext.h>
#include <ctk/ctkmenushell.h>

G_BEGIN_DECLS

#define CTK_TYPE_IM_MULTICONTEXT              (ctk_im_multicontext_get_type ())
#define CTK_IM_MULTICONTEXT(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_IM_MULTICONTEXT, CtkIMMulticontext))
#define CTK_IM_MULTICONTEXT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_IM_MULTICONTEXT, CtkIMMulticontextClass))
#define CTK_IS_IM_MULTICONTEXT(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_IM_MULTICONTEXT))
#define CTK_IS_IM_MULTICONTEXT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_IM_MULTICONTEXT))
#define CTK_IM_MULTICONTEXT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_IM_MULTICONTEXT, CtkIMMulticontextClass))


typedef struct _CtkIMMulticontext        CtkIMMulticontext;
typedef struct _CtkIMMulticontextClass   CtkIMMulticontextClass;
typedef struct _CtkIMMulticontextPrivate CtkIMMulticontextPrivate;

struct _CtkIMMulticontext
{
  CtkIMContext object;

  /*< private >*/
  CtkIMMulticontextPrivate *priv;
};

struct _CtkIMMulticontextClass
{
  CtkIMContextClass parent_class;

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

GDK_AVAILABLE_IN_ALL
GType         ctk_im_multicontext_get_type (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
CtkIMContext *ctk_im_multicontext_new      (void);

GDK_DEPRECATED_IN_3_10
void          ctk_im_multicontext_append_menuitems (CtkIMMulticontext *context,
						    CtkMenuShell      *menushell);
GDK_AVAILABLE_IN_ALL
const char  * ctk_im_multicontext_get_context_id   (CtkIMMulticontext *context);

GDK_AVAILABLE_IN_ALL
void          ctk_im_multicontext_set_context_id   (CtkIMMulticontext *context,
                                                    const char        *context_id);
 
G_END_DECLS

#endif /* __CTK_IM_MULTICONTEXT_H__ */
