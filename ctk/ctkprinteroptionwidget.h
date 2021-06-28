/* CtkPrinterOptionWidget 
 * Copyright (C) 2006 John (J5) Palmieri <johnp@redhat.com>
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
 */
#ifndef __CTK_PRINTER_OPTION_WIDGET_H__
#define __CTK_PRINTER_OPTION_WIDGET_H__

#include "ctkprinteroption.h"
#include "ctkbox.h"

G_BEGIN_DECLS

#define CTK_TYPE_PRINTER_OPTION_WIDGET                  (ctk_printer_option_widget_get_type ())
#define CTK_PRINTER_OPTION_WIDGET(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_PRINTER_OPTION_WIDGET, CtkPrinterOptionWidget))
#define CTK_PRINTER_OPTION_WIDGET_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_PRINTER_OPTION_WIDGET, CtkPrinterOptionWidgetClass))
#define CTK_IS_PRINTER_OPTION_WIDGET(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_PRINTER_OPTION_WIDGET))
#define CTK_IS_PRINTER_OPTION_WIDGET_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_PRINTER_OPTION_WIDGET))
#define CTK_PRINTER_OPTION_WIDGET_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_PRINTER_OPTION_WIDGET, CtkPrinterOptionWidgetClass))


typedef struct _CtkPrinterOptionWidget         CtkPrinterOptionWidget;
typedef struct _CtkPrinterOptionWidgetClass    CtkPrinterOptionWidgetClass;
typedef struct CtkPrinterOptionWidgetPrivate   CtkPrinterOptionWidgetPrivate;

struct _CtkPrinterOptionWidget
{
  CtkBox parent_instance;

  CtkPrinterOptionWidgetPrivate *priv;
};

struct _CtkPrinterOptionWidgetClass
{
  CtkBoxClass parent_class;

  void (*changed) (CtkPrinterOptionWidget *widget);

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

CDK_AVAILABLE_IN_ALL
GType	     ctk_printer_option_widget_get_type           (void) G_GNUC_CONST;

CDK_AVAILABLE_IN_ALL
CtkWidget   *ctk_printer_option_widget_new                (CtkPrinterOption       *source);
CDK_AVAILABLE_IN_ALL
void         ctk_printer_option_widget_set_source         (CtkPrinterOptionWidget *setting,
		 					   CtkPrinterOption       *source);
CDK_AVAILABLE_IN_ALL
gboolean     ctk_printer_option_widget_has_external_label (CtkPrinterOptionWidget *setting);
CDK_AVAILABLE_IN_ALL
CtkWidget   *ctk_printer_option_widget_get_external_label (CtkPrinterOptionWidget *setting);
CDK_AVAILABLE_IN_ALL
const gchar *ctk_printer_option_widget_get_value          (CtkPrinterOptionWidget *setting);

G_END_DECLS

#endif /* __CTK_PRINTER_OPTION_WIDGET_H__ */
