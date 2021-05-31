/* GTK - The GIMP Toolkit
 * gtkrecentchooserwidget.h: embeddable recently used resources chooser widget
 * Copyright (C) 2006 Emmanuele Bassi
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

#ifndef __CTK_RECENT_CHOOSER_WIDGET_H__
#define __CTK_RECENT_CHOOSER_WIDGET_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <gtk/gtk.h> can be included directly."
#endif

#include <gtk/gtkrecentchooser.h>
#include <gtk/gtkbox.h>

G_BEGIN_DECLS

#define CTK_TYPE_RECENT_CHOOSER_WIDGET		  (ctk_recent_chooser_widget_get_type ())
#define CTK_RECENT_CHOOSER_WIDGET(obj)		  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_RECENT_CHOOSER_WIDGET, GtkRecentChooserWidget))
#define CTK_IS_RECENT_CHOOSER_WIDGET(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_RECENT_CHOOSER_WIDGET))
#define CTK_RECENT_CHOOSER_WIDGET_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_RECENT_CHOOSER_WIDGET, GtkRecentChooserWidgetClass))
#define CTK_IS_RECENT_CHOOSER_WIDGET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_RECENT_CHOOSER_WIDGET))
#define CTK_RECENT_CHOOSER_WIDGET_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_RECENT_CHOOSER_WIDGET, GtkRecentChooserWidgetClass))

typedef struct _GtkRecentChooserWidget        GtkRecentChooserWidget;
typedef struct _GtkRecentChooserWidgetClass   GtkRecentChooserWidgetClass;

typedef struct _GtkRecentChooserWidgetPrivate GtkRecentChooserWidgetPrivate;

struct _GtkRecentChooserWidget
{
  GtkBox parent_instance;

  /*< private >*/
  GtkRecentChooserWidgetPrivate *priv;
};

struct _GtkRecentChooserWidgetClass
{
  GtkBoxClass parent_class;

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

GDK_AVAILABLE_IN_ALL
GType      ctk_recent_chooser_widget_get_type        (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
GtkWidget *ctk_recent_chooser_widget_new             (void);
GDK_AVAILABLE_IN_ALL
GtkWidget *ctk_recent_chooser_widget_new_for_manager (GtkRecentManager *manager);

G_END_DECLS

#endif /* __CTK_RECENT_CHOOSER_WIDGET_H__ */
