/* GTK - The GIMP Toolkit
 * gtkprintoperationpreview.h: Abstract print preview interface
 * Copyright (C) 2006, Red Hat, Inc.
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

#ifndef __CTK_PRINT_OPERATION_PREVIEW_H__
#define __CTK_PRINT_OPERATION_PREVIEW_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <gtk/gtk.h> can be included directly."
#endif

#include <cairo.h>
#include <gtk/gtkprintcontext.h>

G_BEGIN_DECLS

#define CTK_TYPE_PRINT_OPERATION_PREVIEW                  (ctk_print_operation_preview_get_type ())
#define CTK_PRINT_OPERATION_PREVIEW(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_PRINT_OPERATION_PREVIEW, GtkPrintOperationPreview))
#define CTK_IS_PRINT_OPERATION_PREVIEW(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_PRINT_OPERATION_PREVIEW))
#define CTK_PRINT_OPERATION_PREVIEW_GET_IFACE(obj)        (G_TYPE_INSTANCE_GET_INTERFACE ((obj), CTK_TYPE_PRINT_OPERATION_PREVIEW, GtkPrintOperationPreviewIface))

typedef struct _GtkPrintOperationPreview      GtkPrintOperationPreview;      /*dummy typedef */
typedef struct _GtkPrintOperationPreviewIface GtkPrintOperationPreviewIface;


struct _GtkPrintOperationPreviewIface
{
  GTypeInterface g_iface;

  /* signals */
  void              (*ready)          (GtkPrintOperationPreview *preview,
				       GtkPrintContext          *context);
  void              (*got_page_size)  (GtkPrintOperationPreview *preview,
				       GtkPrintContext          *context,
				       GtkPageSetup             *page_setup);

  /* methods */
  void              (*render_page)    (GtkPrintOperationPreview *preview,
				       gint                      page_nr);
  gboolean          (*is_selected)    (GtkPrintOperationPreview *preview,
				       gint                      page_nr);
  void              (*end_preview)    (GtkPrintOperationPreview *preview);

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
  void (*_ctk_reserved5) (void);
  void (*_ctk_reserved6) (void);
  void (*_ctk_reserved7) (void);
  void (*_ctk_reserved8) (void);
};

GDK_AVAILABLE_IN_ALL
GType   ctk_print_operation_preview_get_type       (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
void     ctk_print_operation_preview_render_page (GtkPrintOperationPreview *preview,
						  gint                      page_nr);
GDK_AVAILABLE_IN_ALL
void     ctk_print_operation_preview_end_preview (GtkPrintOperationPreview *preview);
GDK_AVAILABLE_IN_ALL
gboolean ctk_print_operation_preview_is_selected (GtkPrintOperationPreview *preview,
						  gint                      page_nr);

G_END_DECLS

#endif /* __CTK_PRINT_OPERATION_PREVIEW_H__ */