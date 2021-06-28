/* CTK - The GIMP Toolkit
 * ctkprintoperationpreview.h: Abstract print preview interface
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
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <cairo.h>
#include <ctk/ctkprintcontext.h>

G_BEGIN_DECLS

#define CTK_TYPE_PRINT_OPERATION_PREVIEW                  (ctk_print_operation_preview_get_type ())
#define CTK_PRINT_OPERATION_PREVIEW(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_PRINT_OPERATION_PREVIEW, CtkPrintOperationPreview))
#define CTK_IS_PRINT_OPERATION_PREVIEW(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_PRINT_OPERATION_PREVIEW))
#define CTK_PRINT_OPERATION_PREVIEW_GET_IFACE(obj)        (G_TYPE_INSTANCE_GET_INTERFACE ((obj), CTK_TYPE_PRINT_OPERATION_PREVIEW, CtkPrintOperationPreviewIface))

typedef struct _CtkPrintOperationPreview      CtkPrintOperationPreview;      /*dummy typedef */
typedef struct _CtkPrintOperationPreviewIface CtkPrintOperationPreviewIface;


struct _CtkPrintOperationPreviewIface
{
  GTypeInterface g_iface;

  /* signals */
  void              (*ready)          (CtkPrintOperationPreview *preview,
				       CtkPrintContext          *context);
  void              (*got_page_size)  (CtkPrintOperationPreview *preview,
				       CtkPrintContext          *context,
				       CtkPageSetup             *page_setup);

  /* methods */
  void              (*render_page)    (CtkPrintOperationPreview *preview,
				       gint                      page_nr);
  gboolean          (*is_selected)    (CtkPrintOperationPreview *preview,
				       gint                      page_nr);
  void              (*end_preview)    (CtkPrintOperationPreview *preview);

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

CDK_AVAILABLE_IN_ALL
GType   ctk_print_operation_preview_get_type       (void) G_GNUC_CONST;

CDK_AVAILABLE_IN_ALL
void     ctk_print_operation_preview_render_page (CtkPrintOperationPreview *preview,
						  gint                      page_nr);
CDK_AVAILABLE_IN_ALL
void     ctk_print_operation_preview_end_preview (CtkPrintOperationPreview *preview);
CDK_AVAILABLE_IN_ALL
gboolean ctk_print_operation_preview_is_selected (CtkPrintOperationPreview *preview,
						  gint                      page_nr);

G_END_DECLS

#endif /* __CTK_PRINT_OPERATION_PREVIEW_H__ */
