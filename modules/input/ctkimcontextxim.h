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

#ifndef __CTK_IM_CONTEXT_XIM_H__
#define __CTK_IM_CONTEXT_XIM_H__

#include <ctk/ctk.h>
#include "x11/cdkx.h"

G_BEGIN_DECLS

extern GType ctk_type_im_context_xim;

#define CTK_TYPE_IM_CONTEXT_XIM            (ctk_type_im_context_xim)
#define CTK_IM_CONTEXT_XIM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_IM_CONTEXT_XIM, CtkIMContextXIM))
#define CTK_IM_CONTEXT_XIM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_IM_CONTEXT_XIM, CtkIMContextXIMClass))
#define CTK_IS_IM_CONTEXT_XIM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_IM_CONTEXT_XIM))
#define CTK_IS_IM_CONTEXT_XIM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_IM_CONTEXT_XIM))
#define CTK_IM_CONTEXT_XIM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_IM_CONTEXT_XIM, CtkIMContextXIMClass))


typedef struct _CtkIMContextXIM       CtkIMContextXIM;
typedef struct _CtkIMContextXIMClass  CtkIMContextXIMClass;

struct _CtkIMContextXIMClass
{
  CtkIMContextClass parent_class;
};

void ctk_im_context_xim_register_type (GTypeModule *type_module);
CtkIMContext *ctk_im_context_xim_new (void);

void ctk_im_context_xim_shutdown (void);

G_END_DECLS

#endif /* __CTK_IM_CONTEXT_XIM_H__ */
