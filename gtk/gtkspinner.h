/* GTK - The GIMP Toolkit
 *
 * Copyright (C) 2007 John Stowers, Neil Jagdish Patel.
 * Copyright (C) 2009 Bastien Nocera, David Zeuthen
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Code adapted from egg-spinner
 * by Christian Hergert <christian.hergert@gmail.com>
 */

#ifndef __CTK_SPINNER_H__
#define __CTK_SPINNER_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <gtk/gtk.h> can be included directly."
#endif

#include <gtk/gtkwidget.h>

G_BEGIN_DECLS

#define CTK_TYPE_SPINNER           (ctk_spinner_get_type ())
#define CTK_SPINNER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_SPINNER, GtkSpinner))
#define CTK_SPINNER_CLASS(obj)     (G_TYPE_CHECK_CLASS_CAST ((obj), CTK_TYPE_SPINNER,  GtkSpinnerClass))
#define CTK_IS_SPINNER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_SPINNER))
#define CTK_IS_SPINNER_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE ((obj), CTK_TYPE_SPINNER))
#define CTK_SPINNER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_SPINNER, GtkSpinnerClass))

typedef struct _GtkSpinner      GtkSpinner;
typedef struct _GtkSpinnerClass GtkSpinnerClass;
typedef struct _GtkSpinnerPrivate  GtkSpinnerPrivate;

struct _GtkSpinner
{
  GtkWidget parent;

  /*< private >*/
  GtkSpinnerPrivate *priv;
};

struct _GtkSpinnerClass
{
  GtkWidgetClass parent_class;

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

GDK_AVAILABLE_IN_ALL
GType      ctk_spinner_get_type  (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
GtkWidget *ctk_spinner_new (void);
GDK_AVAILABLE_IN_ALL
void       ctk_spinner_start      (GtkSpinner *spinner);
GDK_AVAILABLE_IN_ALL
void       ctk_spinner_stop       (GtkSpinner *spinner);

G_END_DECLS

#endif /* __CTK_SPINNER_H__ */
