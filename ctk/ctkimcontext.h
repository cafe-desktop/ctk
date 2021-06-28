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

#ifndef __CTK_IM_CONTEXT_H__
#define __CTK_IM_CONTEXT_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <cdk/cdk.h>


G_BEGIN_DECLS

#define CTK_TYPE_IM_CONTEXT              (ctk_im_context_get_type ())
#define CTK_IM_CONTEXT(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_IM_CONTEXT, CtkIMContext))
#define CTK_IM_CONTEXT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_IM_CONTEXT, CtkIMContextClass))
#define CTK_IS_IM_CONTEXT(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_IM_CONTEXT))
#define CTK_IS_IM_CONTEXT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_IM_CONTEXT))
#define CTK_IM_CONTEXT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_IM_CONTEXT, CtkIMContextClass))


typedef struct _CtkIMContext       CtkIMContext;
typedef struct _CtkIMContextClass  CtkIMContextClass;

struct _CtkIMContext
{
  GObject parent_instance;
};

struct _CtkIMContextClass
{
  /*< private >*/
  GObjectClass parent_class;

  /*< public >*/
  /* Signals */
  void     (*preedit_start)        (CtkIMContext *context);
  void     (*preedit_end)          (CtkIMContext *context);
  void     (*preedit_changed)      (CtkIMContext *context);
  void     (*commit)               (CtkIMContext *context, const gchar *str);
  gboolean (*retrieve_surrounding) (CtkIMContext *context);
  gboolean (*delete_surrounding)   (CtkIMContext *context,
				    gint          offset,
				    gint          n_chars);

  /* Virtual functions */
  void     (*set_client_window)   (CtkIMContext   *context,
				   CdkWindow      *window);
  void     (*get_preedit_string)  (CtkIMContext   *context,
				   gchar         **str,
				   PangoAttrList **attrs,
				   gint           *cursor_pos);
  gboolean (*filter_keypress)     (CtkIMContext   *context,
			           CdkEventKey    *event);
  void     (*focus_in)            (CtkIMContext   *context);
  void     (*focus_out)           (CtkIMContext   *context);
  void     (*reset)               (CtkIMContext   *context);
  void     (*set_cursor_location) (CtkIMContext   *context,
				   CdkRectangle   *area);
  void     (*set_use_preedit)     (CtkIMContext   *context,
				   gboolean        use_preedit);
  void     (*set_surrounding)     (CtkIMContext   *context,
				   const gchar    *text,
				   gint            len,
				   gint            cursor_index);
  gboolean (*get_surrounding)     (CtkIMContext   *context,
				   gchar         **text,
				   gint           *cursor_index);
  /*< private >*/
  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
  void (*_ctk_reserved5) (void);
  void (*_ctk_reserved6) (void);
};

GDK_AVAILABLE_IN_ALL
GType    ctk_im_context_get_type            (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
void     ctk_im_context_set_client_window   (CtkIMContext       *context,
					     CdkWindow          *window);
GDK_AVAILABLE_IN_ALL
void     ctk_im_context_get_preedit_string  (CtkIMContext       *context,
					     gchar             **str,
					     PangoAttrList     **attrs,
					     gint               *cursor_pos);
GDK_AVAILABLE_IN_ALL
gboolean ctk_im_context_filter_keypress     (CtkIMContext       *context,
					     CdkEventKey        *event);
GDK_AVAILABLE_IN_ALL
void     ctk_im_context_focus_in            (CtkIMContext       *context);
GDK_AVAILABLE_IN_ALL
void     ctk_im_context_focus_out           (CtkIMContext       *context);
GDK_AVAILABLE_IN_ALL
void     ctk_im_context_reset               (CtkIMContext       *context);
GDK_AVAILABLE_IN_ALL
void     ctk_im_context_set_cursor_location (CtkIMContext       *context,
					     const CdkRectangle *area);
GDK_AVAILABLE_IN_ALL
void     ctk_im_context_set_use_preedit     (CtkIMContext       *context,
					     gboolean            use_preedit);
GDK_AVAILABLE_IN_ALL
void     ctk_im_context_set_surrounding     (CtkIMContext       *context,
					     const gchar        *text,
					     gint                len,
					     gint                cursor_index);
GDK_AVAILABLE_IN_ALL
gboolean ctk_im_context_get_surrounding     (CtkIMContext       *context,
					     gchar             **text,
					     gint               *cursor_index);
GDK_AVAILABLE_IN_ALL
gboolean ctk_im_context_delete_surrounding  (CtkIMContext       *context,
					     gint                offset,
					     gint                n_chars);

G_END_DECLS

#endif /* __CTK_IM_CONTEXT_H__ */
