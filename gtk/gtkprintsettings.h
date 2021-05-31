/* GTK - The GIMP Toolkit
 * gtkprintsettings.h: Print Settings
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

#ifndef __CTK_PRINT_SETTINGS_H__
#define __CTK_PRINT_SETTINGS_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <gtk/gtk.h> can be included directly."
#endif

#include <gtk/gtkpapersize.h>

G_BEGIN_DECLS

typedef struct _GtkPrintSettings GtkPrintSettings;

#define CTK_TYPE_PRINT_SETTINGS    (ctk_print_settings_get_type ())
#define CTK_PRINT_SETTINGS(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_PRINT_SETTINGS, GtkPrintSettings))
#define CTK_IS_PRINT_SETTINGS(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_PRINT_SETTINGS))

typedef void  (*GtkPrintSettingsFunc)  (const gchar *key,
					const gchar *value,
					gpointer     user_data);

typedef struct _GtkPageRange GtkPageRange;
/**
 * GtkPageRange:
 * @start: start of page range.
 * @end: end of page range.
 *
 * See also ctk_print_settings_set_page_ranges().
 */
struct _GtkPageRange
{
  gint start;
  gint end;
};

GDK_AVAILABLE_IN_ALL
GType             ctk_print_settings_get_type                (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
GtkPrintSettings *ctk_print_settings_new                     (void);

GDK_AVAILABLE_IN_ALL
GtkPrintSettings *ctk_print_settings_copy                    (GtkPrintSettings     *other);

GDK_AVAILABLE_IN_ALL
GtkPrintSettings *ctk_print_settings_new_from_file           (const gchar          *file_name,
							      GError              **error);
GDK_AVAILABLE_IN_ALL
gboolean          ctk_print_settings_load_file               (GtkPrintSettings     *settings,
							      const gchar          *file_name,
							      GError              **error);
GDK_AVAILABLE_IN_ALL
gboolean          ctk_print_settings_to_file                 (GtkPrintSettings     *settings,
							      const gchar          *file_name,
							      GError              **error);
GDK_AVAILABLE_IN_ALL
GtkPrintSettings *ctk_print_settings_new_from_key_file       (GKeyFile             *key_file,
							      const gchar          *group_name,
							      GError              **error);
GDK_AVAILABLE_IN_ALL
gboolean          ctk_print_settings_load_key_file           (GtkPrintSettings     *settings,
							      GKeyFile             *key_file,
							      const gchar          *group_name,
							      GError              **error);
GDK_AVAILABLE_IN_ALL
void              ctk_print_settings_to_key_file             (GtkPrintSettings     *settings,
							      GKeyFile             *key_file,
							      const gchar          *group_name);
GDK_AVAILABLE_IN_ALL
gboolean          ctk_print_settings_has_key                 (GtkPrintSettings     *settings,
							      const gchar          *key);
GDK_AVAILABLE_IN_ALL
const gchar *     ctk_print_settings_get                     (GtkPrintSettings     *settings,
							      const gchar          *key);
GDK_AVAILABLE_IN_ALL
void              ctk_print_settings_set                     (GtkPrintSettings     *settings,
							      const gchar          *key,
							      const gchar          *value);
GDK_AVAILABLE_IN_ALL
void              ctk_print_settings_unset                   (GtkPrintSettings     *settings,
							      const gchar          *key);
GDK_AVAILABLE_IN_ALL
void              ctk_print_settings_foreach                 (GtkPrintSettings     *settings,
							      GtkPrintSettingsFunc  func,
							      gpointer              user_data);
GDK_AVAILABLE_IN_ALL
gboolean          ctk_print_settings_get_bool                (GtkPrintSettings     *settings,
							      const gchar          *key);
GDK_AVAILABLE_IN_ALL
void              ctk_print_settings_set_bool                (GtkPrintSettings     *settings,
							      const gchar          *key,
							      gboolean              value);
GDK_AVAILABLE_IN_ALL
gdouble           ctk_print_settings_get_double              (GtkPrintSettings     *settings,
							      const gchar          *key);
GDK_AVAILABLE_IN_ALL
gdouble           ctk_print_settings_get_double_with_default (GtkPrintSettings     *settings,
							      const gchar          *key,
							      gdouble               def);
GDK_AVAILABLE_IN_ALL
void              ctk_print_settings_set_double              (GtkPrintSettings     *settings,
							      const gchar          *key,
							      gdouble               value);
GDK_AVAILABLE_IN_ALL
gdouble           ctk_print_settings_get_length              (GtkPrintSettings     *settings,
							      const gchar          *key,
							      GtkUnit               unit);
GDK_AVAILABLE_IN_ALL
void              ctk_print_settings_set_length              (GtkPrintSettings     *settings,
							      const gchar          *key,
							      gdouble               value,
							      GtkUnit               unit);
GDK_AVAILABLE_IN_ALL
gint              ctk_print_settings_get_int                 (GtkPrintSettings     *settings,
							      const gchar          *key);
GDK_AVAILABLE_IN_ALL
gint              ctk_print_settings_get_int_with_default    (GtkPrintSettings     *settings,
							      const gchar          *key,
							      gint                  def);
GDK_AVAILABLE_IN_ALL
void              ctk_print_settings_set_int                 (GtkPrintSettings     *settings,
							      const gchar          *key,
							      gint                  value);

#define CTK_PRINT_SETTINGS_PRINTER          "printer"
#define CTK_PRINT_SETTINGS_ORIENTATION      "orientation"
#define CTK_PRINT_SETTINGS_PAPER_FORMAT     "paper-format"
#define CTK_PRINT_SETTINGS_PAPER_WIDTH      "paper-width"
#define CTK_PRINT_SETTINGS_PAPER_HEIGHT     "paper-height"
#define CTK_PRINT_SETTINGS_N_COPIES         "n-copies"
#define CTK_PRINT_SETTINGS_DEFAULT_SOURCE   "default-source"
#define CTK_PRINT_SETTINGS_QUALITY          "quality"
#define CTK_PRINT_SETTINGS_RESOLUTION       "resolution"
#define CTK_PRINT_SETTINGS_USE_COLOR        "use-color"
#define CTK_PRINT_SETTINGS_DUPLEX           "duplex"
#define CTK_PRINT_SETTINGS_COLLATE          "collate"
#define CTK_PRINT_SETTINGS_REVERSE          "reverse"
#define CTK_PRINT_SETTINGS_MEDIA_TYPE       "media-type"
#define CTK_PRINT_SETTINGS_DITHER           "dither"
#define CTK_PRINT_SETTINGS_SCALE            "scale"
#define CTK_PRINT_SETTINGS_PRINT_PAGES      "print-pages"
#define CTK_PRINT_SETTINGS_PAGE_RANGES      "page-ranges"
#define CTK_PRINT_SETTINGS_PAGE_SET         "page-set"
#define CTK_PRINT_SETTINGS_FINISHINGS       "finishings"
#define CTK_PRINT_SETTINGS_NUMBER_UP        "number-up"
#define CTK_PRINT_SETTINGS_NUMBER_UP_LAYOUT "number-up-layout"
#define CTK_PRINT_SETTINGS_OUTPUT_BIN       "output-bin"
#define CTK_PRINT_SETTINGS_RESOLUTION_X     "resolution-x"
#define CTK_PRINT_SETTINGS_RESOLUTION_Y     "resolution-y"
#define CTK_PRINT_SETTINGS_PRINTER_LPI      "printer-lpi"

/**
 * CTK_PRINT_SETTINGS_OUTPUT_DIR:
 *
 * The key used by the “Print to file” printer to store the
 * directory to which the output should be written.
 *
 * Since: 3.6
 */
#define CTK_PRINT_SETTINGS_OUTPUT_DIR       "output-dir"

/**
 * CTK_PRINT_SETTINGS_OUTPUT_BASENAME:
 *
 * The key used by the “Print to file” printer to store the file
 * name of the output without the path to the directory and the
 * file extension.
 *
 * Since: 3.6
 */
#define CTK_PRINT_SETTINGS_OUTPUT_BASENAME  "output-basename"

/**
 * CTK_PRINT_SETTINGS_OUTPUT_FILE_FORMAT:
 *
 * The key used by the “Print to file” printer to store the format
 * of the output. The supported values are “PS” and “PDF”.
 */
#define CTK_PRINT_SETTINGS_OUTPUT_FILE_FORMAT  "output-file-format"

/**
 * CTK_PRINT_SETTINGS_OUTPUT_URI:
 *
 * The key used by the “Print to file” printer to store the URI
 * to which the output should be written. GTK+ itself supports
 * only “file://” URIs.
 */
#define CTK_PRINT_SETTINGS_OUTPUT_URI          "output-uri"

#define CTK_PRINT_SETTINGS_WIN32_DRIVER_VERSION "win32-driver-version"
#define CTK_PRINT_SETTINGS_WIN32_DRIVER_EXTRA   "win32-driver-extra"

/* Helpers: */

GDK_AVAILABLE_IN_ALL
const gchar *         ctk_print_settings_get_printer           (GtkPrintSettings   *settings);
GDK_AVAILABLE_IN_ALL
void                  ctk_print_settings_set_printer           (GtkPrintSettings   *settings,
								const gchar        *printer);
GDK_AVAILABLE_IN_ALL
GtkPageOrientation    ctk_print_settings_get_orientation       (GtkPrintSettings   *settings);
GDK_AVAILABLE_IN_ALL
void                  ctk_print_settings_set_orientation       (GtkPrintSettings   *settings,
								GtkPageOrientation  orientation);
GDK_AVAILABLE_IN_ALL
GtkPaperSize *        ctk_print_settings_get_paper_size        (GtkPrintSettings   *settings);
GDK_AVAILABLE_IN_ALL
void                  ctk_print_settings_set_paper_size        (GtkPrintSettings   *settings,
								GtkPaperSize       *paper_size);
GDK_AVAILABLE_IN_ALL
gdouble               ctk_print_settings_get_paper_width       (GtkPrintSettings   *settings,
								GtkUnit             unit);
GDK_AVAILABLE_IN_ALL
void                  ctk_print_settings_set_paper_width       (GtkPrintSettings   *settings,
								gdouble             width,
								GtkUnit             unit);
GDK_AVAILABLE_IN_ALL
gdouble               ctk_print_settings_get_paper_height      (GtkPrintSettings   *settings,
								GtkUnit             unit);
GDK_AVAILABLE_IN_ALL
void                  ctk_print_settings_set_paper_height      (GtkPrintSettings   *settings,
								gdouble             height,
								GtkUnit             unit);
GDK_AVAILABLE_IN_ALL
gboolean              ctk_print_settings_get_use_color         (GtkPrintSettings   *settings);
GDK_AVAILABLE_IN_ALL
void                  ctk_print_settings_set_use_color         (GtkPrintSettings   *settings,
								gboolean            use_color);
GDK_AVAILABLE_IN_ALL
gboolean              ctk_print_settings_get_collate           (GtkPrintSettings   *settings);
GDK_AVAILABLE_IN_ALL
void                  ctk_print_settings_set_collate           (GtkPrintSettings   *settings,
								gboolean            collate);
GDK_AVAILABLE_IN_ALL
gboolean              ctk_print_settings_get_reverse           (GtkPrintSettings   *settings);
GDK_AVAILABLE_IN_ALL
void                  ctk_print_settings_set_reverse           (GtkPrintSettings   *settings,
								gboolean            reverse);
GDK_AVAILABLE_IN_ALL
GtkPrintDuplex        ctk_print_settings_get_duplex            (GtkPrintSettings   *settings);
GDK_AVAILABLE_IN_ALL
void                  ctk_print_settings_set_duplex            (GtkPrintSettings   *settings,
								GtkPrintDuplex      duplex);
GDK_AVAILABLE_IN_ALL
GtkPrintQuality       ctk_print_settings_get_quality           (GtkPrintSettings   *settings);
GDK_AVAILABLE_IN_ALL
void                  ctk_print_settings_set_quality           (GtkPrintSettings   *settings,
								GtkPrintQuality     quality);
GDK_AVAILABLE_IN_ALL
gint                  ctk_print_settings_get_n_copies          (GtkPrintSettings   *settings);
GDK_AVAILABLE_IN_ALL
void                  ctk_print_settings_set_n_copies          (GtkPrintSettings   *settings,
								gint                num_copies);
GDK_AVAILABLE_IN_ALL
gint                  ctk_print_settings_get_number_up         (GtkPrintSettings   *settings);
GDK_AVAILABLE_IN_ALL
void                  ctk_print_settings_set_number_up         (GtkPrintSettings   *settings,
								gint                number_up);
GDK_AVAILABLE_IN_ALL
GtkNumberUpLayout     ctk_print_settings_get_number_up_layout  (GtkPrintSettings   *settings);
GDK_AVAILABLE_IN_ALL
void                  ctk_print_settings_set_number_up_layout  (GtkPrintSettings   *settings,
								GtkNumberUpLayout   number_up_layout);
GDK_AVAILABLE_IN_ALL
gint                  ctk_print_settings_get_resolution        (GtkPrintSettings   *settings);
GDK_AVAILABLE_IN_ALL
void                  ctk_print_settings_set_resolution        (GtkPrintSettings   *settings,
								gint                resolution);
GDK_AVAILABLE_IN_ALL
gint                  ctk_print_settings_get_resolution_x      (GtkPrintSettings   *settings);
GDK_AVAILABLE_IN_ALL
gint                  ctk_print_settings_get_resolution_y      (GtkPrintSettings   *settings);
GDK_AVAILABLE_IN_ALL
void                  ctk_print_settings_set_resolution_xy     (GtkPrintSettings   *settings,
								gint                resolution_x,
								gint                resolution_y);
GDK_AVAILABLE_IN_ALL
gdouble               ctk_print_settings_get_printer_lpi       (GtkPrintSettings   *settings);
GDK_AVAILABLE_IN_ALL
void                  ctk_print_settings_set_printer_lpi       (GtkPrintSettings   *settings,
								gdouble             lpi);
GDK_AVAILABLE_IN_ALL
gdouble               ctk_print_settings_get_scale             (GtkPrintSettings   *settings);
GDK_AVAILABLE_IN_ALL
void                  ctk_print_settings_set_scale             (GtkPrintSettings   *settings,
								gdouble             scale);
GDK_AVAILABLE_IN_ALL
GtkPrintPages         ctk_print_settings_get_print_pages       (GtkPrintSettings   *settings);
GDK_AVAILABLE_IN_ALL
void                  ctk_print_settings_set_print_pages       (GtkPrintSettings   *settings,
								GtkPrintPages       pages);
GDK_AVAILABLE_IN_ALL
GtkPageRange *        ctk_print_settings_get_page_ranges       (GtkPrintSettings   *settings,
								gint               *num_ranges);
GDK_AVAILABLE_IN_ALL
void                  ctk_print_settings_set_page_ranges       (GtkPrintSettings   *settings,
								GtkPageRange       *page_ranges,
								gint                num_ranges);
GDK_AVAILABLE_IN_ALL
GtkPageSet            ctk_print_settings_get_page_set          (GtkPrintSettings   *settings);
GDK_AVAILABLE_IN_ALL
void                  ctk_print_settings_set_page_set          (GtkPrintSettings   *settings,
								GtkPageSet          page_set);
GDK_AVAILABLE_IN_ALL
const gchar *         ctk_print_settings_get_default_source    (GtkPrintSettings   *settings);
GDK_AVAILABLE_IN_ALL
void                  ctk_print_settings_set_default_source    (GtkPrintSettings   *settings,
								const gchar        *default_source);
GDK_AVAILABLE_IN_ALL
const gchar *         ctk_print_settings_get_media_type        (GtkPrintSettings   *settings);
GDK_AVAILABLE_IN_ALL
void                  ctk_print_settings_set_media_type        (GtkPrintSettings   *settings,
								const gchar        *media_type);
GDK_AVAILABLE_IN_ALL
const gchar *         ctk_print_settings_get_dither            (GtkPrintSettings   *settings);
GDK_AVAILABLE_IN_ALL
void                  ctk_print_settings_set_dither            (GtkPrintSettings   *settings,
								const gchar        *dither);
GDK_AVAILABLE_IN_ALL
const gchar *         ctk_print_settings_get_finishings        (GtkPrintSettings   *settings);
GDK_AVAILABLE_IN_ALL
void                  ctk_print_settings_set_finishings        (GtkPrintSettings   *settings,
								const gchar        *finishings);
GDK_AVAILABLE_IN_ALL
const gchar *         ctk_print_settings_get_output_bin        (GtkPrintSettings   *settings);
GDK_AVAILABLE_IN_ALL
void                  ctk_print_settings_set_output_bin        (GtkPrintSettings   *settings,
								const gchar        *output_bin);

GDK_AVAILABLE_IN_3_22
GVariant             *ctk_print_settings_to_gvariant           (GtkPrintSettings   *settings);
GDK_AVAILABLE_IN_3_22
GtkPrintSettings     *ctk_print_settings_new_from_gvariant     (GVariant           *variant);


G_END_DECLS

#endif /* __CTK_PRINT_SETTINGS_H__ */
