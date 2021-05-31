/*
 * ctkappchooserwidget.h: an app-chooser widget
 *
 * Copyright (C) 2004 Novell, Inc.
 * Copyright (C) 2007, 2010 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Dave Camp <dave@novell.com>
 *          Alexander Larsson <alexl@redhat.com>
 *          Cosimo Cecchi <ccecchi@redhat.com>
 */

#ifndef __CTK_APP_CHOOSER_WIDGET_H__
#define __CTK_APP_CHOOSER_WIDGET_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkbox.h>
#include <ctk/ctkmenu.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define CTK_TYPE_APP_CHOOSER_WIDGET            (ctk_app_chooser_widget_get_type ())
#define CTK_APP_CHOOSER_WIDGET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_APP_CHOOSER_WIDGET, GtkAppChooserWidget))
#define CTK_APP_CHOOSER_WIDGET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_APP_CHOOSER_WIDGET, GtkAppChooserWidgetClass))
#define CTK_IS_APP_CHOOSER_WIDGET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_APP_CHOOSER_WIDGET))
#define CTK_IS_APP_CHOOSER_WIDGET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_APP_CHOOSER_WIDGET))
#define CTK_APP_CHOOSER_WIDGET_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_APP_CHOOSER_WIDGET, GtkAppChooserWidgetClass))

typedef struct _GtkAppChooserWidget        GtkAppChooserWidget;
typedef struct _GtkAppChooserWidgetClass   GtkAppChooserWidgetClass;
typedef struct _GtkAppChooserWidgetPrivate GtkAppChooserWidgetPrivate;

struct _GtkAppChooserWidget {
  GtkBox parent;

  /*< private >*/
  GtkAppChooserWidgetPrivate *priv;
};

/**
 * GtkAppChooserWidgetClass:
 * @parent_class: The parent class.
 * @application_selected: Signal emitted when an application item is
 *    selected from the widget’s list.
 * @application_activated: Signal emitted when an application item is
 *    activated from the widget’s list.
 * @populate_popup: Signal emitted when a context menu is about to
 *    popup over an application item.
 */
struct _GtkAppChooserWidgetClass {
  GtkBoxClass parent_class;

  /*< public >*/

  void (* application_selected)  (GtkAppChooserWidget *self,
                                  GAppInfo            *app_info);

  void (* application_activated) (GtkAppChooserWidget *self,
                                  GAppInfo            *app_info);

  void (* populate_popup)        (GtkAppChooserWidget *self,
                                  GtkMenu             *menu,
                                  GAppInfo            *app_info);

  /*< private >*/

  /* padding for future class expansion */
  gpointer padding[16];
};

GDK_AVAILABLE_IN_ALL
GType         ctk_app_chooser_widget_get_type             (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
GtkWidget *   ctk_app_chooser_widget_new                  (const gchar         *content_type);

GDK_AVAILABLE_IN_ALL
void          ctk_app_chooser_widget_set_show_default     (GtkAppChooserWidget *self,
                                                           gboolean             setting);
GDK_AVAILABLE_IN_ALL
gboolean      ctk_app_chooser_widget_get_show_default     (GtkAppChooserWidget *self);

GDK_AVAILABLE_IN_ALL
void          ctk_app_chooser_widget_set_show_recommended (GtkAppChooserWidget *self,
                                                           gboolean             setting);
GDK_AVAILABLE_IN_ALL
gboolean      ctk_app_chooser_widget_get_show_recommended (GtkAppChooserWidget *self);

GDK_AVAILABLE_IN_ALL
void          ctk_app_chooser_widget_set_show_fallback    (GtkAppChooserWidget *self,
                                                           gboolean             setting);
GDK_AVAILABLE_IN_ALL
gboolean      ctk_app_chooser_widget_get_show_fallback    (GtkAppChooserWidget *self);

GDK_AVAILABLE_IN_ALL
void          ctk_app_chooser_widget_set_show_other       (GtkAppChooserWidget *self,
                                                           gboolean             setting);
GDK_AVAILABLE_IN_ALL
gboolean      ctk_app_chooser_widget_get_show_other       (GtkAppChooserWidget *self);

GDK_AVAILABLE_IN_ALL
void          ctk_app_chooser_widget_set_show_all         (GtkAppChooserWidget *self,
                                                           gboolean             setting);
GDK_AVAILABLE_IN_ALL
gboolean      ctk_app_chooser_widget_get_show_all         (GtkAppChooserWidget *self);

GDK_AVAILABLE_IN_ALL
void          ctk_app_chooser_widget_set_default_text     (GtkAppChooserWidget *self,
                                                           const gchar         *text);
GDK_AVAILABLE_IN_ALL
const gchar * ctk_app_chooser_widget_get_default_text     (GtkAppChooserWidget *self);

G_END_DECLS

#endif /* __CTK_APP_CHOOSER_WIDGET_H__ */
