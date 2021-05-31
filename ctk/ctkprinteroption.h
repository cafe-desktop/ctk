/* GTK - The GIMP Toolkit
 * gtkprinteroption.h: printer option
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

#ifndef __CTK_PRINTER_OPTION_H__
#define __CTK_PRINTER_OPTION_H__

/* This is a "semi-private" header; it is meant only for
 * alternate GtkPrintDialog backend modules; no stability guarantees 
 * are made at this point
 */
#ifndef CTK_PRINT_BACKEND_ENABLE_UNSUPPORTED
#error "GtkPrintBackend is not supported API for general use"
#endif

#include <glib-object.h>

G_BEGIN_DECLS

#define CTK_TYPE_PRINTER_OPTION             (ctk_printer_option_get_type ())
#define CTK_PRINTER_OPTION(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_PRINTER_OPTION, GtkPrinterOption))
#define CTK_IS_PRINTER_OPTION(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_PRINTER_OPTION))

typedef struct _GtkPrinterOption       GtkPrinterOption;
typedef struct _GtkPrinterOptionClass  GtkPrinterOptionClass;

#define CTK_PRINTER_OPTION_GROUP_IMAGE_QUALITY "ImageQuality"
#define CTK_PRINTER_OPTION_GROUP_FINISHING "Finishing"

typedef enum {
  CTK_PRINTER_OPTION_TYPE_BOOLEAN,
  CTK_PRINTER_OPTION_TYPE_PICKONE,
  CTK_PRINTER_OPTION_TYPE_PICKONE_PASSWORD,
  CTK_PRINTER_OPTION_TYPE_PICKONE_PASSCODE,
  CTK_PRINTER_OPTION_TYPE_PICKONE_REAL,
  CTK_PRINTER_OPTION_TYPE_PICKONE_INT,
  CTK_PRINTER_OPTION_TYPE_PICKONE_STRING,
  CTK_PRINTER_OPTION_TYPE_ALTERNATIVE,
  CTK_PRINTER_OPTION_TYPE_STRING,
  CTK_PRINTER_OPTION_TYPE_FILESAVE,
  CTK_PRINTER_OPTION_TYPE_INFO
} GtkPrinterOptionType;

struct _GtkPrinterOption
{
  GObject parent_instance;

  char *name;
  char *display_text;
  GtkPrinterOptionType type;

  char *value;
  
  int num_choices;
  char **choices;
  char **choices_display;
  
  gboolean activates_default;

  gboolean has_conflict;
  char *group;
};

struct _GtkPrinterOptionClass
{
  GObjectClass parent_class;

  void (*changed) (GtkPrinterOption *option);

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

GDK_AVAILABLE_IN_ALL
GType   ctk_printer_option_get_type       (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
GtkPrinterOption *ctk_printer_option_new                    (const char           *name,
							     const char           *display_text,
							     GtkPrinterOptionType  type);
GDK_AVAILABLE_IN_ALL
void              ctk_printer_option_set                    (GtkPrinterOption     *option,
							     const char           *value);
GDK_AVAILABLE_IN_ALL
void              ctk_printer_option_set_has_conflict       (GtkPrinterOption     *option,
							     gboolean              has_conflict);
GDK_AVAILABLE_IN_ALL
void              ctk_printer_option_clear_has_conflict     (GtkPrinterOption     *option);
GDK_AVAILABLE_IN_ALL
void              ctk_printer_option_set_boolean            (GtkPrinterOption     *option,
							     gboolean              value);
GDK_AVAILABLE_IN_ALL
void              ctk_printer_option_allocate_choices       (GtkPrinterOption     *option,
							     int                   num);
GDK_AVAILABLE_IN_ALL
void              ctk_printer_option_choices_from_array     (GtkPrinterOption     *option,
							     int                   num_choices,
							     char                 *choices[],
							     char                 *choices_display[]);
GDK_AVAILABLE_IN_ALL
gboolean          ctk_printer_option_has_choice             (GtkPrinterOption     *option,
							    const char           *choice);
GDK_AVAILABLE_IN_ALL
void              ctk_printer_option_set_activates_default (GtkPrinterOption     *option,
							    gboolean              activates);
GDK_AVAILABLE_IN_ALL
gboolean          ctk_printer_option_get_activates_default (GtkPrinterOption     *option);


G_END_DECLS

#endif /* __CTK_PRINTER_OPTION_H__ */


