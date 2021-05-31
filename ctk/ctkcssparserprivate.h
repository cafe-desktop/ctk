/* CTK - The GIMP Toolkit
 * Copyright (C) 2011 Benjamin Otte <otte@gnome.org>
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

#ifndef __CTK_CSS_PARSER_PRIVATE_H__
#define __CTK_CSS_PARSER_PRIVATE_H__

#include "ctk/ctkcsstypesprivate.h"
#include <ctk/ctkcssprovider.h>

G_BEGIN_DECLS

typedef struct _CtkCssParser CtkCssParser;

typedef void (* CtkCssParserErrorFunc) (CtkCssParser *parser,
                                        const GError *error,
                                        gpointer      user_data);

CtkCssParser *  _ctk_css_parser_new               (const char            *data,
                                                   GFile                 *file,
                                                   CtkCssParserErrorFunc  error_func,
                                                   gpointer               user_data);
void            _ctk_css_parser_free              (CtkCssParser          *parser);

void            _ctk_css_parser_take_error        (CtkCssParser          *parser,
                                                   GError                *error);
void            _ctk_css_parser_error             (CtkCssParser          *parser,
                                                   const char            *format,
                                                    ...) G_GNUC_PRINTF (2, 3);
void            _ctk_css_parser_error_full        (CtkCssParser          *parser,
                                                   CtkCssProviderError    code,
                                                   const char            *format,
                                                    ...) G_GNUC_PRINTF (3, 4);

guint           _ctk_css_parser_get_line          (CtkCssParser          *parser);
guint           _ctk_css_parser_get_position      (CtkCssParser          *parser);
GFile *         _ctk_css_parser_get_file          (CtkCssParser          *parser);
GFile *         _ctk_css_parser_get_file_for_path (CtkCssParser          *parser,
                                                   const char            *path);

gboolean        _ctk_css_parser_is_eof            (CtkCssParser          *parser);
gboolean        _ctk_css_parser_begins_with       (CtkCssParser          *parser,
                                                   char                   c);
gboolean        _ctk_css_parser_has_prefix        (CtkCssParser          *parser,
                                                   const char            *prefix);
gboolean        _ctk_css_parser_is_string         (CtkCssParser          *parser);

/* IMPORTANT:
 * _try_foo() functions do not modify the data pointer if they fail, nor do they
 * signal an error. _read_foo() will modify the data pointer and position it at
 * the first token that is broken and emit an error about the failure.
 * So only call _read_foo() when you know that you are reading a foo. _try_foo()
 * however is fine to call if you don’t know yet if the token is a foo or a bar,
 * you can _try_bar() if try_foo() failed.
 */
gboolean        _ctk_css_parser_try               (CtkCssParser          *parser,
                                                   const char            *string,
                                                   gboolean               skip_whitespace);
char *          _ctk_css_parser_try_ident         (CtkCssParser          *parser,
                                                   gboolean               skip_whitespace);
char *          _ctk_css_parser_try_name          (CtkCssParser          *parser,
                                                   gboolean               skip_whitespace);
gboolean        _ctk_css_parser_try_int           (CtkCssParser          *parser,
                                                   int                   *value);
gboolean        _ctk_css_parser_try_uint          (CtkCssParser          *parser,
                                                   guint                 *value);
gboolean        _ctk_css_parser_try_double        (CtkCssParser          *parser,
                                                   gdouble               *value);
gboolean        _ctk_css_parser_try_length        (CtkCssParser          *parser,
                                                   int                   *value);
gboolean        _ctk_css_parser_try_enum          (CtkCssParser          *parser,
                                                   GType                  enum_type,
                                                   int                   *value);
gboolean        _ctk_css_parser_try_hash_color    (CtkCssParser          *parser,
                                                   GdkRGBA               *rgba);

gboolean        _ctk_css_parser_has_number        (CtkCssParser          *parser);
char *          _ctk_css_parser_read_string       (CtkCssParser          *parser);
char *          _ctk_css_parser_read_value        (CtkCssParser          *parser);
GFile *         _ctk_css_parser_read_url          (CtkCssParser          *parser);

void            _ctk_css_parser_skip_whitespace   (CtkCssParser          *parser);
void            _ctk_css_parser_resync            (CtkCssParser          *parser,
                                                   gboolean               sync_at_semicolon,
                                                   char                   terminator);

/* XXX: Find better place to put it? */
void            _ctk_css_print_string             (GString               *str,
                                                   const char            *string);


G_END_DECLS

#endif /* __CTK_CSS_PARSER_PRIVATE_H__ */
