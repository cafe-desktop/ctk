/*
 * ctkimmoduleime
 * Copyright (C) 2003 Takuro Ashie
 * Copyright (C) 2003 Kazuki IWAMOTO
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
 *
 * $Id$
 */

#include <ctk/ctk.h>

extern GType ctk_type_im_context_ime;

#define CTK_TYPE_IM_CONTEXT_IME            ctk_type_im_context_ime
#define CTK_IM_CONTEXT_IME(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_IM_CONTEXT_IME, CtkIMContextIME))
#define CTK_IM_CONTEXT_IME_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_IM_CONTEXT_IME, CtkIMContextIMEClass))
#define CTK_IS_IM_CONTEXT_IME(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_IM_CONTEXT_IME))
#define CTK_IS_IM_CONTEXT_IME_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_IM_CONTEXT_IME))
#define CTK_IM_CONTEXT_IME_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_IM_CONTEXT_IME, CtkIMContextIMEClass))

typedef struct _CtkIMContextIME CtkIMContextIME;
typedef struct _CtkIMContextIMEPrivate CtkIMContextIMEPrivate;
typedef struct _CtkIMContextIMEClass CtkIMContextIMEClass;

struct _CtkIMContextIME
{
  CtkIMContext object;

  CdkWindow *client_window;
  CdkWindow *toplevel;
  guint use_preedit : 1;
  guint preediting : 1;
  guint opened : 1;
  guint focus : 1;
  CdkRectangle cursor_location;
  gchar *commit_string;

  CtkIMContextIMEPrivate *priv;
};

struct _CtkIMContextIMEClass
{
  CtkIMContextClass parent_class;
};


void          ctk_im_context_ime_register_type (GTypeModule * type_module);
CtkIMContext *ctk_im_context_ime_new           (void);
