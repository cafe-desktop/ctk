/* CTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * CtkStatusbar Copyright (C) 1998 Shawn T. Amundson
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

/*
 * Modified by the CTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CTK_STATUSBAR_H__
#define __CTK_STATUSBAR_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkbox.h>

G_BEGIN_DECLS

#define CTK_TYPE_STATUSBAR            (ctk_statusbar_get_type ())
#define CTK_STATUSBAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_STATUSBAR, CtkStatusbar))
#define CTK_STATUSBAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_STATUSBAR, CtkStatusbarClass))
#define CTK_IS_STATUSBAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_STATUSBAR))
#define CTK_IS_STATUSBAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_STATUSBAR))
#define CTK_STATUSBAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_STATUSBAR, CtkStatusbarClass))


typedef struct _CtkStatusbar              CtkStatusbar;
typedef struct _CtkStatusbarPrivate       CtkStatusbarPrivate;
typedef struct _CtkStatusbarClass         CtkStatusbarClass;

struct _CtkStatusbar
{
  CtkBox parent_widget;

  /*< private >*/
  CtkStatusbarPrivate *priv;
};

struct _CtkStatusbarClass
{
  CtkBoxClass parent_class;

  gpointer reserved;

  void	(*text_pushed)	(CtkStatusbar	*statusbar,
			 guint		 context_id,
			 const gchar	*text);
  void	(*text_popped)	(CtkStatusbar	*statusbar,
			 guint		 context_id,
			 const gchar	*text);

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};


CDK_AVAILABLE_IN_ALL
GType      ctk_statusbar_get_type     	(void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
CtkWidget* ctk_statusbar_new          	(void);
/* If you don't want to use contexts, 0 is a predefined global
 * context_id you can pass to push/pop/remove
 */
CDK_AVAILABLE_IN_ALL
guint	   ctk_statusbar_get_context_id	(CtkStatusbar *statusbar,
					 const gchar  *context_description);
/* Returns message_id used for ctk_statusbar_remove */
CDK_AVAILABLE_IN_ALL
guint      ctk_statusbar_push          	(CtkStatusbar *statusbar,
					 guint	       context_id,
					 const gchar  *text);
CDK_AVAILABLE_IN_ALL
void       ctk_statusbar_pop          	(CtkStatusbar *statusbar,
					 guint	       context_id);
CDK_AVAILABLE_IN_ALL
void       ctk_statusbar_remove        	(CtkStatusbar *statusbar,
					 guint	       context_id,
					 guint         message_id);
CDK_AVAILABLE_IN_ALL
void       ctk_statusbar_remove_all    	(CtkStatusbar *statusbar,
					 guint	       context_id);

CDK_AVAILABLE_IN_ALL
CtkWidget* ctk_statusbar_get_message_area  (CtkStatusbar *statusbar);

G_END_DECLS

#endif /* __CTK_STATUSBAR_H__ */
