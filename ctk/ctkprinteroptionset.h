/* CTK - The GIMP Toolkit
 * ctkprinteroptionset.h: printer option set
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

#ifndef __CTK_PRINTER_OPTION_SET_H__
#define __CTK_PRINTER_OPTION_SET_H__

/* This is a "semi-private" header; it is meant only for
 * alternate CtkPrintDialog backend modules; no stability guarantees
 * are made at this point
 */
#ifndef CTK_PRINT_BACKEND_ENABLE_UNSUPPORTED
#error "CtkPrintBackend is not supported API for general use"
#endif

#include <glib-object.h>
#include <cdk/cdk.h>
#include "ctkprinteroption.h"

G_BEGIN_DECLS

#define CTK_TYPE_PRINTER_OPTION_SET             (ctk_printer_option_set_get_type ())
#define CTK_PRINTER_OPTION_SET(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_PRINTER_OPTION_SET, CtkPrinterOptionSet))
#define CTK_IS_PRINTER_OPTION_SET(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_PRINTER_OPTION_SET))

typedef struct _CtkPrinterOptionSet       CtkPrinterOptionSet;
typedef struct _CtkPrinterOptionSetClass  CtkPrinterOptionSetClass;

struct _CtkPrinterOptionSet
{
  GObject parent_instance;

  /*< private >*/
  GPtrArray *array;
  GHashTable *hash;
};

struct _CtkPrinterOptionSetClass
{
  GObjectClass parent_class;

  void (*changed) (CtkPrinterOptionSet *option);


  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

typedef void (*CtkPrinterOptionSetFunc) (CtkPrinterOption  *option,
					 gpointer           user_data);


CDK_AVAILABLE_IN_ALL
GType   ctk_printer_option_set_get_type       (void) G_GNUC_CONST;

CDK_AVAILABLE_IN_ALL
CtkPrinterOptionSet *ctk_printer_option_set_new              (void);
CDK_AVAILABLE_IN_ALL
void                 ctk_printer_option_set_add              (CtkPrinterOptionSet     *set,
							      CtkPrinterOption        *option);
CDK_AVAILABLE_IN_ALL
void                 ctk_printer_option_set_remove           (CtkPrinterOptionSet     *set,
							      CtkPrinterOption        *option);
CDK_AVAILABLE_IN_ALL
CtkPrinterOption *   ctk_printer_option_set_lookup           (CtkPrinterOptionSet     *set,
							      const char              *name);
CDK_AVAILABLE_IN_ALL
void                 ctk_printer_option_set_foreach          (CtkPrinterOptionSet     *set,
							      CtkPrinterOptionSetFunc  func,
							      gpointer                 user_data);
CDK_AVAILABLE_IN_ALL
void                 ctk_printer_option_set_clear_conflicts  (CtkPrinterOptionSet     *set);
CDK_AVAILABLE_IN_ALL
GList *              ctk_printer_option_set_get_groups       (CtkPrinterOptionSet     *set);
CDK_AVAILABLE_IN_ALL
void                 ctk_printer_option_set_foreach_in_group (CtkPrinterOptionSet     *set,
							      const char              *group,
							      CtkPrinterOptionSetFunc  func,
							      gpointer                 user_data);

G_END_DECLS

#endif /* __CTK_PRINTER_OPTION_SET_H__ */
