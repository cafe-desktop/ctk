/* GTK - The GIMP Toolkit
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

#include "config.h"

#include "gtkcssparserprivate.h"

#include "gtkcssdimensionvalueprivate.h"

#include <errno.h>
#include <string.h>

/* just for the errors, yay! */
#include "gtkcssprovider.h"

#define NEWLINE_CHARS "\r\n"
#define WHITESPACE_CHARS "\f \t"
#define NMSTART "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
#define NMCHAR NMSTART "01234567890-_"
#define URLCHAR NMCHAR "!#$%&*~"

#define CTK_IS_CSS_PARSER(parser) ((parser) != NULL)

struct _GtkCssParser
{
  const char            *data;
  GFile                 *file;
  GtkCssParserErrorFunc  error_func;
  gpointer               user_data;

  const char            *line_start;
  guint                  line;
};

GtkCssParser *
_ctk_css_parser_new (const char            *data,
                     GFile                 *file,
                     GtkCssParserErrorFunc  error_func,
                     gpointer               user_data)
{
  GtkCssParser *parser;

  g_return_val_if_fail (data != NULL, NULL);
  g_return_val_if_fail (file == NULL || G_IS_FILE (file), NULL);

  parser = g_slice_new0 (GtkCssParser);

  parser->data = data;
  if (file)
    parser->file = g_object_ref (file);
  parser->error_func = error_func;
  parser->user_data = user_data;

  parser->line_start = data;
  parser->line = 0;

  return parser;
}

void
_ctk_css_parser_free (GtkCssParser *parser)
{
  g_return_if_fail (CTK_IS_CSS_PARSER (parser));

  if (parser->file)
    g_object_unref (parser->file);

  g_slice_free (GtkCssParser, parser);
}

gboolean
_ctk_css_parser_is_eof (GtkCssParser *parser)
{
  g_return_val_if_fail (CTK_IS_CSS_PARSER (parser), TRUE);

  return *parser->data == 0;
}

gboolean
_ctk_css_parser_begins_with (GtkCssParser *parser,
                             char          c)
{
  g_return_val_if_fail (CTK_IS_CSS_PARSER (parser), TRUE);

  return *parser->data == c;
}

gboolean
_ctk_css_parser_has_prefix (GtkCssParser *parser,
                            const char   *prefix)
{
  g_return_val_if_fail (CTK_IS_CSS_PARSER (parser), FALSE);

  return g_ascii_strncasecmp (parser->data, prefix, strlen (prefix)) == 0;
}

guint
_ctk_css_parser_get_line (GtkCssParser *parser)
{
  g_return_val_if_fail (CTK_IS_CSS_PARSER (parser), 1);

  return parser->line;
}

guint
_ctk_css_parser_get_position (GtkCssParser *parser)
{
  g_return_val_if_fail (CTK_IS_CSS_PARSER (parser), 1);

  return parser->data - parser->line_start;
}

static GFile *
ctk_css_parser_get_base_file (GtkCssParser *parser)
{
  GFile *base;

  if (parser->file)
    {
      base = g_file_get_parent (parser->file);
    }
  else
    {
      char *dir = g_get_current_dir ();
      base = g_file_new_for_path (dir);
      g_free (dir);
    }

  return base;
}

GFile *
_ctk_css_parser_get_file_for_path (GtkCssParser *parser,
                                   const char   *path)
{
  GFile *base, *file;

  g_return_val_if_fail (parser != NULL, NULL);
  g_return_val_if_fail (path != NULL, NULL);

  base = ctk_css_parser_get_base_file (parser);
  file = g_file_resolve_relative_path (base, path);
  g_object_unref (base);

  return file;
}

GFile *
_ctk_css_parser_get_file (GtkCssParser *parser)
{
  g_return_val_if_fail (parser != NULL, NULL);

  return parser->file;
}

void
_ctk_css_parser_take_error (GtkCssParser *parser,
                            GError       *error)
{
  parser->error_func (parser, error, parser->user_data);

  g_error_free (error);
}

void
_ctk_css_parser_error (GtkCssParser *parser,
                       const char   *format,
                       ...)
{
  GError *error;

  va_list args;

  va_start (args, format);
  error = g_error_new_valist (CTK_CSS_PROVIDER_ERROR,
                              CTK_CSS_PROVIDER_ERROR_SYNTAX,
                              format, args);
  va_end (args);

  _ctk_css_parser_take_error (parser, error);
}

void
_ctk_css_parser_error_full (GtkCssParser        *parser,
                            GtkCssProviderError  code,
                            const char          *format,
                            ...)
{
  GError *error;

  va_list args;

  va_start (args, format);
  error = g_error_new_valist (CTK_CSS_PROVIDER_ERROR,
                              code, format, args);
  va_end (args);

  _ctk_css_parser_take_error (parser, error);
}
static gboolean
ctk_css_parser_new_line (GtkCssParser *parser)
{
  gboolean result = FALSE;

  if (*parser->data == '\r')
    {
      result = TRUE;
      parser->data++;
    }
  if (*parser->data == '\n')
    {
      result = TRUE;
      parser->data++;
    }

  if (result)
    {
      parser->line++;
      parser->line_start = parser->data;
    }

  return result;
}

static gboolean
ctk_css_parser_skip_comment (GtkCssParser *parser)
{
  if (parser->data[0] != '/' ||
      parser->data[1] != '*')
    return FALSE;

  parser->data += 2;

  while (*parser->data)
    {
      gsize len = strcspn (parser->data, NEWLINE_CHARS "/");

      parser->data += len;
  
      if (ctk_css_parser_new_line (parser))
        continue;

      parser->data++;

      if (len > 0 && parser->data[-2] == '*')
        return TRUE;
      if (parser->data[0] == '*')
        _ctk_css_parser_error (parser, "'/*' in comment block");
    }

  /* FIXME: position */
  _ctk_css_parser_error (parser, "Unterminated comment");
  return TRUE;
}

void
_ctk_css_parser_skip_whitespace (GtkCssParser *parser)
{
  size_t len;

  while (*parser->data)
    {
      if (ctk_css_parser_new_line (parser))
        continue;

      len = strspn (parser->data, WHITESPACE_CHARS);
      if (len)
        {
          parser->data += len;
          continue;
        }
      
      if (!ctk_css_parser_skip_comment (parser))
        break;
    }
}

gboolean
_ctk_css_parser_try (GtkCssParser *parser,
                     const char   *string,
                     gboolean      skip_whitespace)
{
  g_return_val_if_fail (CTK_IS_CSS_PARSER (parser), FALSE);
  g_return_val_if_fail (string != NULL, FALSE);

  if (g_ascii_strncasecmp (parser->data, string, strlen (string)) != 0)
    return FALSE;

  parser->data += strlen (string);

  if (skip_whitespace)
    _ctk_css_parser_skip_whitespace (parser);
  return TRUE;
}

static guint
get_xdigit (char c)
{
  if (c >= 'a')
    return c - 'a' + 10;
  else if (c >= 'A')
    return c - 'A' + 10;
  else
    return c - '0';
}

static void
_ctk_css_parser_unescape (GtkCssParser *parser,
                          GString      *str)
{
  guint i;
  gunichar result = 0;

  g_assert (*parser->data == '\\');

  parser->data++;

  for (i = 0; i < 6; i++)
    {
      if (!g_ascii_isxdigit (parser->data[i]))
        break;

      result = (result << 4) + get_xdigit (parser->data[i]);
    }

  if (i != 0)
    {
      g_string_append_unichar (str, result);
      parser->data += i;

      /* NB: ctk_css_parser_new_line() forward data pointer itself */
      if (!ctk_css_parser_new_line (parser) &&
          *parser->data &&
          strchr (WHITESPACE_CHARS, *parser->data))
        parser->data++;
      return;
    }

  if (ctk_css_parser_new_line (parser))
    return;

  g_string_append_c (str, *parser->data);
  parser->data++;

  return;
}

static gboolean
_ctk_css_parser_read_char (GtkCssParser *parser,
                           GString *     str,
                           const char *  allowed)
{
  if (*parser->data == 0)
    return FALSE;

  if (strchr (allowed, *parser->data))
    {
      g_string_append_c (str, *parser->data);
      parser->data++;
      return TRUE;
    }
  if (*parser->data >= 127)
    {
      gsize len = g_utf8_skip[(guint) *(guchar *) parser->data];

      g_string_append_len (str, parser->data, len);
      parser->data += len;
      return TRUE;
    }
  if (*parser->data == '\\')
    {
      _ctk_css_parser_unescape (parser, str);
      return TRUE;
    }

  return FALSE;
}

char *
_ctk_css_parser_try_name (GtkCssParser *parser,
                          gboolean      skip_whitespace)
{
  GString *name;

  g_return_val_if_fail (CTK_IS_CSS_PARSER (parser), NULL);

  name = g_string_new (NULL);

  while (_ctk_css_parser_read_char (parser, name, NMCHAR))
    ;

  if (skip_whitespace)
    _ctk_css_parser_skip_whitespace (parser);

  return g_string_free (name, FALSE);
}

char *
_ctk_css_parser_try_ident (GtkCssParser *parser,
                           gboolean      skip_whitespace)
{
  const char *start;
  GString *ident;

  g_return_val_if_fail (CTK_IS_CSS_PARSER (parser), NULL);

  start = parser->data;
  
  ident = g_string_new (NULL);

  if (*parser->data == '-')
    {
      g_string_append_c (ident, '-');
      parser->data++;
    }

  if (!_ctk_css_parser_read_char (parser, ident, NMSTART))
    {
      parser->data = start;
      g_string_free (ident, TRUE);
      return NULL;
    }

  while (_ctk_css_parser_read_char (parser, ident, NMCHAR))
    ;

  if (skip_whitespace)
    _ctk_css_parser_skip_whitespace (parser);

  return g_string_free (ident, FALSE);
}

gboolean
_ctk_css_parser_is_string (GtkCssParser *parser)
{
  g_return_val_if_fail (CTK_IS_CSS_PARSER (parser), FALSE);

  return *parser->data == '"' || *parser->data == '\'';
}

char *
_ctk_css_parser_read_string (GtkCssParser *parser)
{
  GString *str;
  char quote;

  g_return_val_if_fail (CTK_IS_CSS_PARSER (parser), NULL);

  quote = *parser->data;
  
  if (quote != '"' && quote != '\'')
    {
      _ctk_css_parser_error (parser, "Expected a string.");
      return NULL;
    }
  
  parser->data++;
  str = g_string_new (NULL);

  while (TRUE)
    {
      gsize len = strcspn (parser->data, "\\'\"\n\r\f");

      g_string_append_len (str, parser->data, len);

      parser->data += len;

      switch (*parser->data)
        {
        case '\\':
          _ctk_css_parser_unescape (parser, str);
          break;
        case '"':
        case '\'':
          if (*parser->data == quote)
            {
              parser->data++;
              _ctk_css_parser_skip_whitespace (parser);
              return g_string_free (str, FALSE);
            }
          
          g_string_append_c (str, *parser->data);
          parser->data++;
          break;
        case '\0':
          /* FIXME: position */
          _ctk_css_parser_error (parser, "Missing end quote in string.");
          g_string_free (str, TRUE);
          return NULL;
        default:
          _ctk_css_parser_error (parser, 
                                 "Invalid character in string. Must be escaped.");
          g_string_free (str, TRUE);
          return NULL;
        }
    }

  g_assert_not_reached ();
  return NULL;
}

gboolean
_ctk_css_parser_try_int (GtkCssParser *parser,
                         int          *value)
{
  gint64 result;
  char *end;

  g_return_val_if_fail (CTK_IS_CSS_PARSER (parser), FALSE);
  g_return_val_if_fail (value != NULL, FALSE);

  /* strtoll parses a plus, but we are not allowed to */
  if (*parser->data == '+')
    return FALSE;

  errno = 0;
  result = g_ascii_strtoll (parser->data, &end, 10);
  if (errno)
    return FALSE;
  if (result > G_MAXINT || result < G_MININT)
    return FALSE;
  if (parser->data == end)
    return FALSE;

  parser->data = end;
  *value = result;

  _ctk_css_parser_skip_whitespace (parser);

  return TRUE;
}

gboolean
_ctk_css_parser_try_uint (GtkCssParser *parser,
                          guint        *value)
{
  guint64 result;
  char *end;

  g_return_val_if_fail (CTK_IS_CSS_PARSER (parser), FALSE);
  g_return_val_if_fail (value != NULL, FALSE);

  errno = 0;
  result = g_ascii_strtoull (parser->data, &end, 10);
  if (errno)
    return FALSE;
  if (result > G_MAXUINT)
    return FALSE;
  if (parser->data == end)
    return FALSE;

  parser->data = end;
  *value = result;

  _ctk_css_parser_skip_whitespace (parser);

  return TRUE;
}

gboolean
_ctk_css_parser_try_double (GtkCssParser *parser,
                            gdouble      *value)
{
  gdouble result;
  char *end;

  g_return_val_if_fail (CTK_IS_CSS_PARSER (parser), FALSE);
  g_return_val_if_fail (value != NULL, FALSE);

  errno = 0;
  result = g_ascii_strtod (parser->data, &end);
  if (errno)
    return FALSE;
  if (parser->data == end)
    return FALSE;

  parser->data = end;
  *value = result;

  _ctk_css_parser_skip_whitespace (parser);

  return TRUE;
}

gboolean
_ctk_css_parser_has_number (GtkCssParser *parser)
{
  char c;

  if (parser->data[0] == '-' || parser->data[0] == '+')
    c = parser->data[1];
  else
    c = parser->data[0];

  /* ahem */
  return g_ascii_isdigit (c) || c == '.';
}

GtkCssValue *
ctk_css_dimension_value_parse (GtkCssParser           *parser,
                               GtkCssNumberParseFlags  flags)
{
  static const struct {
    const char *name;
    GtkCssUnit unit;
    GtkCssNumberParseFlags required_flags;
  } units[] = {
    { "px",   CTK_CSS_PX,      CTK_CSS_PARSE_LENGTH },
    { "pt",   CTK_CSS_PT,      CTK_CSS_PARSE_LENGTH },
    { "em",   CTK_CSS_EM,      CTK_CSS_PARSE_LENGTH },
    { "ex",   CTK_CSS_EX,      CTK_CSS_PARSE_LENGTH },
    { "rem",  CTK_CSS_REM,     CTK_CSS_PARSE_LENGTH },
    { "pc",   CTK_CSS_PC,      CTK_CSS_PARSE_LENGTH },
    { "in",   CTK_CSS_IN,      CTK_CSS_PARSE_LENGTH },
    { "cm",   CTK_CSS_CM,      CTK_CSS_PARSE_LENGTH },
    { "mm",   CTK_CSS_MM,      CTK_CSS_PARSE_LENGTH },
    { "rad",  CTK_CSS_RAD,     CTK_CSS_PARSE_ANGLE  },
    { "deg",  CTK_CSS_DEG,     CTK_CSS_PARSE_ANGLE  },
    { "grad", CTK_CSS_GRAD,    CTK_CSS_PARSE_ANGLE  },
    { "turn", CTK_CSS_TURN,    CTK_CSS_PARSE_ANGLE  },
    { "s",    CTK_CSS_S,       CTK_CSS_PARSE_TIME   },
    { "ms",   CTK_CSS_MS,      CTK_CSS_PARSE_TIME   }
  };
  char *end, *unit_name;
  double value;
  GtkCssUnit unit;

  g_return_val_if_fail (CTK_IS_CSS_PARSER (parser), NULL);

  errno = 0;
  value = g_ascii_strtod (parser->data, &end);
  if (errno)
    {
      _ctk_css_parser_error (parser, "not a number: %s", g_strerror (errno));
      return NULL;
    }
  if (parser->data == end)
    {
      _ctk_css_parser_error (parser, "not a number");
      return NULL;
    }

  parser->data = end;

  if (flags & CTK_CSS_POSITIVE_ONLY &&
      value < 0)
    {
      _ctk_css_parser_error (parser, "negative values are not allowed.");
      return NULL;
    }

  unit_name = _ctk_css_parser_try_ident (parser, FALSE);

  if (unit_name)
    {
      guint i;

      for (i = 0; i < G_N_ELEMENTS (units); i++)
        {
          if (flags & units[i].required_flags &&
              g_ascii_strcasecmp (unit_name, units[i].name) == 0)
            break;
        }

      if (i >= G_N_ELEMENTS (units))
        {
          _ctk_css_parser_error (parser, "'%s' is not a valid unit.", unit_name);
          g_free (unit_name);
          return NULL;
        }

      unit = units[i].unit;

      g_free (unit_name);
    }
  else
    {
      if ((flags & CTK_CSS_PARSE_PERCENT) &&
          _ctk_css_parser_try (parser, "%", FALSE))
        {
          unit = CTK_CSS_PERCENT;
        }
      else if (value == 0.0)
        {
          if (flags & CTK_CSS_PARSE_NUMBER)
            unit = CTK_CSS_NUMBER;
          else if (flags & CTK_CSS_PARSE_LENGTH)
            unit = CTK_CSS_PX;
          else if (flags & CTK_CSS_PARSE_ANGLE)
            unit = CTK_CSS_DEG;
          else if (flags & CTK_CSS_PARSE_TIME)
            unit = CTK_CSS_S;
          else
            unit = CTK_CSS_PERCENT;
        }
      else if (flags & CTK_CSS_NUMBER_AS_PIXELS)
        {
          _ctk_css_parser_error_full (parser,
                                      CTK_CSS_PROVIDER_ERROR_DEPRECATED,
                                      "Not using units is deprecated. Assuming 'px'.");
          unit = CTK_CSS_PX;
        }
      else if (flags & CTK_CSS_PARSE_NUMBER)
        {
          unit = CTK_CSS_NUMBER;
        }
      else
        {
          _ctk_css_parser_error (parser, "Unit is missing.");
          return NULL;
        }
    }

  _ctk_css_parser_skip_whitespace (parser);

  return ctk_css_dimension_value_new (value, unit);
}

/* XXX: we should introduce GtkCssLenght that deals with
 * different kind of units */
gboolean
_ctk_css_parser_try_length (GtkCssParser *parser,
                            int          *value)
{
  if (!_ctk_css_parser_try_int (parser, value))
    return FALSE;

  /* FIXME: _try_uint skips spaces while the
   * spec forbids them
   */
  _ctk_css_parser_try (parser, "px", TRUE);

  return TRUE;
}

gboolean
_ctk_css_parser_try_enum (GtkCssParser *parser,
			  GType         enum_type,
			  int          *value)
{
  GEnumClass *enum_class;
  gboolean result;
  const char *start;
  char *str;

  g_return_val_if_fail (CTK_IS_CSS_PARSER (parser), FALSE);
  g_return_val_if_fail (value != NULL, FALSE);

  result = FALSE;

  enum_class = g_type_class_ref (enum_type);

  start = parser->data;

  str = _ctk_css_parser_try_ident (parser, TRUE);
  if (str == NULL)
    return FALSE;

  if (enum_class->n_values)
    {
      GEnumValue *enum_value;

      for (enum_value = enum_class->values; enum_value->value_name; enum_value++)
	{
	  if (enum_value->value_nick &&
	      g_ascii_strcasecmp (str, enum_value->value_nick) == 0)
	    {
	      *value = enum_value->value;
	      result = TRUE;
	      break;
	    }
	}
    }

  g_free (str);
  g_type_class_unref (enum_class);

  if (!result)
    parser->data = start;

  return result;
}

gboolean
_ctk_css_parser_try_hash_color (GtkCssParser *parser,
                                GdkRGBA      *rgba)
{
  if (parser->data[0] == '#' &&
      g_ascii_isxdigit (parser->data[1]) &&
      g_ascii_isxdigit (parser->data[2]) &&
      g_ascii_isxdigit (parser->data[3]))
    {
      if (g_ascii_isxdigit (parser->data[4]) &&
          g_ascii_isxdigit (parser->data[5]) &&
          g_ascii_isxdigit (parser->data[6]))
        {
          rgba->red   = ((get_xdigit (parser->data[1]) << 4) + get_xdigit (parser->data[2])) / 255.0;
          rgba->green = ((get_xdigit (parser->data[3]) << 4) + get_xdigit (parser->data[4])) / 255.0;
          rgba->blue  = ((get_xdigit (parser->data[5]) << 4) + get_xdigit (parser->data[6])) / 255.0;
          rgba->alpha = 1.0;
          parser->data += 7;
        }
      else
        {
          rgba->red   = get_xdigit (parser->data[1]) / 15.0;
          rgba->green = get_xdigit (parser->data[2]) / 15.0;
          rgba->blue  = get_xdigit (parser->data[3]) / 15.0;
          rgba->alpha = 1.0;
          parser->data += 4;
        }

      _ctk_css_parser_skip_whitespace (parser);

      return TRUE;
    }

  return FALSE;
}

GFile *
_ctk_css_parser_read_url (GtkCssParser *parser)
{
  gchar *path;
  char *scheme;
  GFile *file;

  if (_ctk_css_parser_try (parser, "url", FALSE))
    {
      if (!_ctk_css_parser_try (parser, "(", TRUE))
        {
          _ctk_css_parser_skip_whitespace (parser);
          if (_ctk_css_parser_try (parser, "(", TRUE))
            {
              _ctk_css_parser_error_full (parser,
                                          CTK_CSS_PROVIDER_ERROR_DEPRECATED,
                                          "Whitespace between 'url' and '(' is deprecated");
            }
          else
            {
              _ctk_css_parser_error (parser, "Expected '(' after 'url'");
              return NULL;
            }
        }

      path = _ctk_css_parser_read_string (parser);
      if (path == NULL)
        return NULL;

      if (!_ctk_css_parser_try (parser, ")", TRUE))
        {
          _ctk_css_parser_error (parser, "No closing ')' found for 'url'");
          g_free (path);
          return NULL;
        }

      scheme = g_uri_parse_scheme (path);
      if (scheme != NULL)
	{
	  file = g_file_new_for_uri (path);
	  g_free (path);
	  g_free (scheme);
	  return file;
	}
    }
  else
    {
      path = _ctk_css_parser_try_name (parser, TRUE);
      if (path == NULL)
        {
          _ctk_css_parser_error (parser, "Not a valid url");
          return NULL;
        }
    }

  file = _ctk_css_parser_get_file_for_path (parser, path);
  g_free (path);

  return file;
}

static void
ctk_css_parser_resync_internal (GtkCssParser *parser,
                                gboolean      sync_at_semicolon,
                                gboolean      read_sync_token,
                                char          terminator)
{
  gsize len;

  do {
    len = strcspn (parser->data, "\\\"'/()[]{};" NEWLINE_CHARS);
    parser->data += len;

    if (ctk_css_parser_new_line (parser))
      continue;

    if (_ctk_css_parser_is_string (parser))
      {
        /* Hrm, this emits errors, and i suspect it shouldn't... */
        char *free_me = _ctk_css_parser_read_string (parser);
        g_free (free_me);
        continue;
      }

    if (ctk_css_parser_skip_comment (parser))
      continue;

    switch (*parser->data)
      {
      case '\\':
        {
          GString *ignore = g_string_new (NULL);
          _ctk_css_parser_unescape (parser, ignore);
          g_string_free (ignore, TRUE);
        }
        break;
      case ';':
        if (sync_at_semicolon && !read_sync_token)
          return;
        parser->data++;
        if (sync_at_semicolon)
          {
            _ctk_css_parser_skip_whitespace (parser);
            return;
          }
        break;
      case '(':
        parser->data++;
        _ctk_css_parser_resync (parser, FALSE, ')');
        if (*parser->data)
          parser->data++;
        break;
      case '[':
        parser->data++;
        _ctk_css_parser_resync (parser, FALSE, ']');
        if (*parser->data)
          parser->data++;
        break;
      case '{':
        parser->data++;
        _ctk_css_parser_resync (parser, FALSE, '}');
        if (*parser->data)
          parser->data++;
        if (sync_at_semicolon || !terminator)
          {
            _ctk_css_parser_skip_whitespace (parser);
            return;
          }
        break;
      case '}':
      case ')':
      case ']':
        if (terminator == *parser->data)
          {
            _ctk_css_parser_skip_whitespace (parser);
            return;
          }
        parser->data++;
        continue;
      case '\0':
        break;
      case '/':
      default:
        parser->data++;
        break;
      }
  } while (*parser->data);
}

char *
_ctk_css_parser_read_value (GtkCssParser *parser)
{
  const char *start;
  char *result;

  g_return_val_if_fail (CTK_IS_CSS_PARSER (parser), NULL);

  start = parser->data;

  /* This needs to be done better */
  ctk_css_parser_resync_internal (parser, TRUE, FALSE, '}');

  result = g_strndup (start, parser->data - start);
  if (result)
    {
      g_strchomp (result);
      if (result[0] == 0)
        {
          g_free (result);
          result = NULL;
        }
    }

  if (result == NULL)
    _ctk_css_parser_error (parser, "Expected a property value");

  return result;
}

void
_ctk_css_parser_resync (GtkCssParser *parser,
                        gboolean      sync_at_semicolon,
                        char          terminator)
{
  g_return_if_fail (CTK_IS_CSS_PARSER (parser));

  ctk_css_parser_resync_internal (parser, sync_at_semicolon, TRUE, terminator);
}

void
_ctk_css_print_string (GString    *str,
                       const char *string)
{
  gsize len;

  g_return_if_fail (str != NULL);
  g_return_if_fail (string != NULL);

  g_string_append_c (str, '"');

  do {
    len = strcspn (string, "\\\"\n\r\f");
    g_string_append_len (str, string, len);
    string += len;
    switch (*string)
      {
      case '\0':
        goto out;
      case '\n':
        g_string_append (str, "\\A ");
        break;
      case '\r':
        g_string_append (str, "\\D ");
        break;
      case '\f':
        g_string_append (str, "\\C ");
        break;
      case '\"':
        g_string_append (str, "\\\"");
        break;
      case '\\':
        g_string_append (str, "\\\\");
        break;
      default:
        g_assert_not_reached ();
        break;
      }
    string++;
  } while (*string);

out:
  g_string_append_c (str, '"');
}

