/* GTK - The GIMP Toolkit
 * ctkfilechooserwidget.h: Embeddable file selector widget
 * Copyright (C) 2003, Red Hat, Inc.
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

#ifndef __CTK_FILE_CHOOSER_WIDGET_H__
#define __CTK_FILE_CHOOSER_WIDGET_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkfilechooser.h>
#include <ctk/ctkbox.h>

G_BEGIN_DECLS

#define CTK_TYPE_FILE_CHOOSER_WIDGET             (ctk_file_chooser_widget_get_type ())
#define CTK_FILE_CHOOSER_WIDGET(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_FILE_CHOOSER_WIDGET, CtkFileChooserWidget))
#define CTK_FILE_CHOOSER_WIDGET_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_FILE_CHOOSER_WIDGET, CtkFileChooserWidgetClass))
#define CTK_IS_FILE_CHOOSER_WIDGET(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_FILE_CHOOSER_WIDGET))
#define CTK_IS_FILE_CHOOSER_WIDGET_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_FILE_CHOOSER_WIDGET))
#define CTK_FILE_CHOOSER_WIDGET_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_FILE_CHOOSER_WIDGET, CtkFileChooserWidgetClass))

typedef struct _CtkFileChooserWidget        CtkFileChooserWidget;
typedef struct _CtkFileChooserWidgetPrivate CtkFileChooserWidgetPrivate;
typedef struct _CtkFileChooserWidgetClass   CtkFileChooserWidgetClass;

struct _CtkFileChooserWidget
{
  CtkBox parent_instance;

  CtkFileChooserWidgetPrivate *priv;
};

/**
 * CtkFileChooserWidgetClass:
 * @parent_class: The parent class.
 */
struct _CtkFileChooserWidgetClass
{
  CtkBoxClass parent_class;

  /*< private >*/

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

GDK_AVAILABLE_IN_ALL
GType      ctk_file_chooser_widget_get_type         (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
CtkWidget *ctk_file_chooser_widget_new              (CtkFileChooserAction  action);

G_END_DECLS

#endif /* __CTK_FILE_CHOOSER_WIDGET_H__ */
