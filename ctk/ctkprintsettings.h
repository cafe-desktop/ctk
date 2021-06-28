/* CTK - The GIMP Toolkit
 * ctkprintsettings.h: Print Settings
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
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkpapersize.h>

G_BEGIN_DECLS

typedef struct _CtkPrintSettings CtkPrintSettings;

#define CTK_TYPE_PRINT_SETTINGS    (ctk_print_settings_get_type ())
#define CTK_PRINT_SETTINGS(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_PRINT_SETTINGS, CtkPrintSettings))
#define CTK_IS_PRINT_SETTINGS(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_PRINT_SETTINGS))

typedef void  (*CtkPrintSettingsFunc)  (const gchar *key,
					const gchar *value,
					gpointer     user_data);

typedef struct _CtkPageRange CtkPageRange;
/**
 * CtkPageRange:
 * @start: start of page range.
 * @end: end of page range.
 *
 * See also ctk_print_settings_set_page_ranges().
 */
struct _CtkPageRange
{
  gint start;
  gint end;
};

CDK_AVAILABLE_IN_ALL
GType             ctk_print_settings_get_type                (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
CtkPrintSettings *ctk_print_settings_new                     (void);

CDK_AVAILABLE_IN_ALL
CtkPrintSettings *ctk_print_settings_copy                    (CtkPrintSettings     *other);

CDK_AVAILABLE_IN_ALL
CtkPrintSettings *ctk_print_settings_new_from_file           (const gchar          *file_name,
							      GError              **error);
CDK_AVAILABLE_IN_ALL
gboolean          ctk_print_settings_load_file               (CtkPrintSettings     *settings,
							      const gchar          *file_name,
							      GError              **error);
CDK_AVAILABLE_IN_ALL
gboolean          ctk_print_settings_to_file                 (CtkPrintSettings     *settings,
							      const gchar          *file_name,
							      GError              **error);
CDK_AVAILABLE_IN_ALL
CtkPrintSettings *ctk_print_settings_new_from_key_file       (GKeyFile             *key_file,
							      const gchar          *group_name,
							      GError              **error);
CDK_AVAILABLE_IN_ALL
gboolean          ctk_print_settings_load_key_file           (CtkPrintSettings     *settings,
							      GKeyFile             *key_file,
							      const gchar          *group_name,
							      GError              **error);
CDK_AVAILABLE_IN_ALL
void              ctk_print_settings_to_key_file             (CtkPrintSettings     *settings,
							      GKeyFile             *key_file,
							      const gchar          *group_name);
CDK_AVAILABLE_IN_ALL
gboolean          ctk_print_settings_has_key                 (CtkPrintSettings     *settings,
							      const gchar          *key);
CDK_AVAILABLE_IN_ALL
const gchar *     ctk_print_settings_get                     (CtkPrintSettings     *settings,
							      const gchar          *key);
CDK_AVAILABLE_IN_ALL
void              ctk_print_settings_set                     (CtkPrintSettings     *settings,
							      const gchar          *key,
							      const gchar          *value);
CDK_AVAILABLE_IN_ALL
void              ctk_print_settings_unset                   (CtkPrintSettings     *settings,
							      const gchar          *key);
CDK_AVAILABLE_IN_ALL
void              ctk_print_settings_foreach                 (CtkPrintSettings     *settings,
							      CtkPrintSettingsFunc  func,
							      gpointer              user_data);
CDK_AVAILABLE_IN_ALL
gboolean          ctk_print_settings_get_bool                (CtkPrintSettings     *settings,
							      const gchar          *key);
CDK_AVAILABLE_IN_ALL
void              ctk_print_settings_set_bool                (CtkPrintSettings     *settings,
							      const gchar          *key,
							      gboolean              value);
CDK_AVAILABLE_IN_ALL
gdouble           ctk_print_settings_get_double              (CtkPrintSettings     *settings,
							      const gchar          *key);
CDK_AVAILABLE_IN_ALL
gdouble           ctk_print_settings_get_double_with_default (CtkPrintSettings     *settings,
							      const gchar          *key,
							      gdouble               def);
CDK_AVAILABLE_IN_ALL
void              ctk_print_settings_set_double              (CtkPrintSettings     *settings,
							      const gchar          *key,
							      gdouble               value);
CDK_AVAILABLE_IN_ALL
gdouble           ctk_print_settings_get_length              (CtkPrintSettings     *settings,
							      const gchar          *key,
							      CtkUnit               unit);
CDK_AVAILABLE_IN_ALL
void              ctk_print_settings_set_length              (CtkPrintSettings     *settings,
							      const gchar          *key,
							      gdouble               value,
							      CtkUnit               unit);
CDK_AVAILABLE_IN_ALL
gint              ctk_print_settings_get_int                 (CtkPrintSettings     *settings,
							      const gchar          *key);
CDK_AVAILABLE_IN_ALL
gint              ctk_print_settings_get_int_with_default    (CtkPrintSettings     *settings,
							      const gchar          *key,
							      gint                  def);
CDK_AVAILABLE_IN_ALL
void              ctk_print_settings_set_int                 (CtkPrintSettings     *settings,
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
 * to which the output should be written. CTK+ itself supports
 * only “file://” URIs.
 */
#define CTK_PRINT_SETTINGS_OUTPUT_URI          "output-uri"

#define CTK_PRINT_SETTINGS_WIN32_DRIVER_VERSION "win32-driver-version"
#define CTK_PRINT_SETTINGS_WIN32_DRIVER_EXTRA   "win32-driver-extra"

/* Helpers: */

CDK_AVAILABLE_IN_ALL
const gchar *         ctk_print_settings_get_printer           (CtkPrintSettings   *settings);
CDK_AVAILABLE_IN_ALL
void                  ctk_print_settings_set_printer           (CtkPrintSettings   *settings,
								const gchar        *printer);
CDK_AVAILABLE_IN_ALL
CtkPageOrientation    ctk_print_settings_get_orientation       (CtkPrintSettings   *settings);
CDK_AVAILABLE_IN_ALL
void                  ctk_print_settings_set_orientation       (CtkPrintSettings   *settings,
								CtkPageOrientation  orientation);
CDK_AVAILABLE_IN_ALL
CtkPaperSize *        ctk_print_settings_get_paper_size        (CtkPrintSettings   *settings);
CDK_AVAILABLE_IN_ALL
void                  ctk_print_settings_set_paper_size        (CtkPrintSettings   *settings,
								CtkPaperSize       *paper_size);
CDK_AVAILABLE_IN_ALL
gdouble               ctk_print_settings_get_paper_width       (CtkPrintSettings   *settings,
								CtkUnit             unit);
CDK_AVAILABLE_IN_ALL
void                  ctk_print_settings_set_paper_width       (CtkPrintSettings   *settings,
								gdouble             width,
								CtkUnit             unit);
CDK_AVAILABLE_IN_ALL
gdouble               ctk_print_settings_get_paper_height      (CtkPrintSettings   *settings,
								CtkUnit             unit);
CDK_AVAILABLE_IN_ALL
void                  ctk_print_settings_set_paper_height      (CtkPrintSettings   *settings,
								gdouble             height,
								CtkUnit             unit);
CDK_AVAILABLE_IN_ALL
gboolean              ctk_print_settings_get_use_color         (CtkPrintSettings   *settings);
CDK_AVAILABLE_IN_ALL
void                  ctk_print_settings_set_use_color         (CtkPrintSettings   *settings,
								gboolean            use_color);
CDK_AVAILABLE_IN_ALL
gboolean              ctk_print_settings_get_collate           (CtkPrintSettings   *settings);
CDK_AVAILABLE_IN_ALL
void                  ctk_print_settings_set_collate           (CtkPrintSettings   *settings,
								gboolean            collate);
CDK_AVAILABLE_IN_ALL
gboolean              ctk_print_settings_get_reverse           (CtkPrintSettings   *settings);
CDK_AVAILABLE_IN_ALL
void                  ctk_print_settings_set_reverse           (CtkPrintSettings   *settings,
								gboolean            reverse);
CDK_AVAILABLE_IN_ALL
CtkPrintDuplex        ctk_print_settings_get_duplex            (CtkPrintSettings   *settings);
CDK_AVAILABLE_IN_ALL
void                  ctk_print_settings_set_duplex            (CtkPrintSettings   *settings,
								CtkPrintDuplex      duplex);
CDK_AVAILABLE_IN_ALL
CtkPrintQuality       ctk_print_settings_get_quality           (CtkPrintSettings   *settings);
CDK_AVAILABLE_IN_ALL
void                  ctk_print_settings_set_quality           (CtkPrintSettings   *settings,
								CtkPrintQuality     quality);
CDK_AVAILABLE_IN_ALL
gint                  ctk_print_settings_get_n_copies          (CtkPrintSettings   *settings);
CDK_AVAILABLE_IN_ALL
void                  ctk_print_settings_set_n_copies          (CtkPrintSettings   *settings,
								gint                num_copies);
CDK_AVAILABLE_IN_ALL
gint                  ctk_print_settings_get_number_up         (CtkPrintSettings   *settings);
CDK_AVAILABLE_IN_ALL
void                  ctk_print_settings_set_number_up         (CtkPrintSettings   *settings,
								gint                number_up);
CDK_AVAILABLE_IN_ALL
CtkNumberUpLayout     ctk_print_settings_get_number_up_layout  (CtkPrintSettings   *settings);
CDK_AVAILABLE_IN_ALL
void                  ctk_print_settings_set_number_up_layout  (CtkPrintSettings   *settings,
								CtkNumberUpLayout   number_up_layout);
CDK_AVAILABLE_IN_ALL
gint                  ctk_print_settings_get_resolution        (CtkPrintSettings   *settings);
CDK_AVAILABLE_IN_ALL
void                  ctk_print_settings_set_resolution        (CtkPrintSettings   *settings,
								gint                resolution);
CDK_AVAILABLE_IN_ALL
gint                  ctk_print_settings_get_resolution_x      (CtkPrintSettings   *settings);
CDK_AVAILABLE_IN_ALL
gint                  ctk_print_settings_get_resolution_y      (CtkPrintSettings   *settings);
CDK_AVAILABLE_IN_ALL
void                  ctk_print_settings_set_resolution_xy     (CtkPrintSettings   *settings,
								gint                resolution_x,
								gint                resolution_y);
CDK_AVAILABLE_IN_ALL
gdouble               ctk_print_settings_get_printer_lpi       (CtkPrintSettings   *settings);
CDK_AVAILABLE_IN_ALL
void                  ctk_print_settings_set_printer_lpi       (CtkPrintSettings   *settings,
								gdouble             lpi);
CDK_AVAILABLE_IN_ALL
gdouble               ctk_print_settings_get_scale             (CtkPrintSettings   *settings);
CDK_AVAILABLE_IN_ALL
void                  ctk_print_settings_set_scale             (CtkPrintSettings   *settings,
								gdouble             scale);
CDK_AVAILABLE_IN_ALL
CtkPrintPages         ctk_print_settings_get_print_pages       (CtkPrintSettings   *settings);
CDK_AVAILABLE_IN_ALL
void                  ctk_print_settings_set_print_pages       (CtkPrintSettings   *settings,
								CtkPrintPages       pages);
CDK_AVAILABLE_IN_ALL
CtkPageRange *        ctk_print_settings_get_page_ranges       (CtkPrintSettings   *settings,
								gint               *num_ranges);
CDK_AVAILABLE_IN_ALL
void                  ctk_print_settings_set_page_ranges       (CtkPrintSettings   *settings,
								CtkPageRange       *page_ranges,
								gint                num_ranges);
CDK_AVAILABLE_IN_ALL
CtkPageSet            ctk_print_settings_get_page_set          (CtkPrintSettings   *settings);
CDK_AVAILABLE_IN_ALL
void                  ctk_print_settings_set_page_set          (CtkPrintSettings   *settings,
								CtkPageSet          page_set);
CDK_AVAILABLE_IN_ALL
const gchar *         ctk_print_settings_get_default_source    (CtkPrintSettings   *settings);
CDK_AVAILABLE_IN_ALL
void                  ctk_print_settings_set_default_source    (CtkPrintSettings   *settings,
								const gchar        *default_source);
CDK_AVAILABLE_IN_ALL
const gchar *         ctk_print_settings_get_media_type        (CtkPrintSettings   *settings);
CDK_AVAILABLE_IN_ALL
void                  ctk_print_settings_set_media_type        (CtkPrintSettings   *settings,
								const gchar        *media_type);
CDK_AVAILABLE_IN_ALL
const gchar *         ctk_print_settings_get_dither            (CtkPrintSettings   *settings);
CDK_AVAILABLE_IN_ALL
void                  ctk_print_settings_set_dither            (CtkPrintSettings   *settings,
								const gchar        *dither);
CDK_AVAILABLE_IN_ALL
const gchar *         ctk_print_settings_get_finishings        (CtkPrintSettings   *settings);
CDK_AVAILABLE_IN_ALL
void                  ctk_print_settings_set_finishings        (CtkPrintSettings   *settings,
								const gchar        *finishings);
CDK_AVAILABLE_IN_ALL
const gchar *         ctk_print_settings_get_output_bin        (CtkPrintSettings   *settings);
CDK_AVAILABLE_IN_ALL
void                  ctk_print_settings_set_output_bin        (CtkPrintSettings   *settings,
								const gchar        *output_bin);

CDK_AVAILABLE_IN_3_22
GVariant             *ctk_print_settings_to_gvariant           (CtkPrintSettings   *settings);
CDK_AVAILABLE_IN_3_22
CtkPrintSettings     *ctk_print_settings_new_from_gvariant     (GVariant           *variant);


G_END_DECLS

#endif /* __CTK_PRINT_SETTINGS_H__ */
