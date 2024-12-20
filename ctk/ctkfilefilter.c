/* CTK - The GIMP Toolkit
 * ctkfilefilter.c: Filters for selecting a file subset
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

/**
 * SECTION:ctkfilefilter
 * @Short_description: A filter for selecting a file subset
 * @Title: CtkFileFilter
 * @see_also: #CtkFileChooser
 *
 * A CtkFileFilter can be used to restrict the files being shown in a
 * #CtkFileChooser. Files can be filtered based on their name (with
 * ctk_file_filter_add_pattern()), on their mime type (with
 * ctk_file_filter_add_mime_type()), or by a custom filter function
 * (with ctk_file_filter_add_custom()).
 *
 * Filtering by mime types handles aliasing and subclassing of mime
 * types; e.g. a filter for text/plain also matches a file with mime
 * type application/rtf, since application/rtf is a subclass of
 * text/plain. Note that #CtkFileFilter allows wildcards for the
 * subtype of a mime type, so you can e.g. filter for image/\*.
 *
 * Normally, filters are used by adding them to a #CtkFileChooser,
 * see ctk_file_chooser_add_filter(), but it is also possible
 * to manually use a filter on a file with ctk_file_filter_filter().
 *
 * # CtkFileFilter as CtkBuildable
 *
 * The CtkFileFilter implementation of the CtkBuildable interface
 * supports adding rules using the <mime-types>, <patterns> and
 * <applications> elements and listing the rules within. Specifying
 * a <mime-type> or <pattern> has the same effect as as calling
 * ctk_file_filter_add_mime_type() or ctk_file_filter_add_pattern().
 *
 * An example of a UI definition fragment specifying CtkFileFilter
 * rules:
 * |[
 * <object class="CtkFileFilter">
 *   <mime-types>
 *     <mime-type>text/plain</mime-type>
 *     <mime-type>image/ *</mime-type>
 *   </mime-types>
 *   <patterns>
 *     <pattern>*.txt</pattern>
 *     <pattern>*.png</pattern>
 *   </patterns>
 * </object>
 * ]|
 */

#include "config.h"
#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#include "ctkfilefilterprivate.h"
#include "ctkbuildable.h"
#include "ctkbuilderprivate.h"
#include "ctkintl.h"
#include "ctkprivate.h"

typedef struct _CtkFileFilterClass CtkFileFilterClass;
typedef struct _FilterRule FilterRule;

#define CTK_FILE_FILTER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_FILE_FILTER, CtkFileFilterClass))
#define CTK_IS_FILE_FILTER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_FILE_FILTER))
#define CTK_FILE_FILTER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_FILE_FILTER, CtkFileFilterClass))

typedef enum {
  FILTER_RULE_PATTERN,
  FILTER_RULE_MIME_TYPE,
  FILTER_RULE_PIXBUF_FORMATS,
  FILTER_RULE_CUSTOM
} FilterRuleType;

struct _CtkFileFilterClass
{
  GInitiallyUnownedClass parent_class;
};

struct _CtkFileFilter
{
  GInitiallyUnowned parent_instance;

  gchar *name;
  GSList *rules;

  CtkFileFilterFlags needed;
};

struct _FilterRule
{
  FilterRuleType type;
  CtkFileFilterFlags needed;
  
  union {
    gchar *pattern;
    gchar *mime_type;
    GSList *pixbuf_formats;
    struct {
      CtkFileFilterFunc func;
      gpointer data;
      GDestroyNotify notify;
    } custom;
  } u;
};

static void ctk_file_filter_finalize   (GObject            *object);


static void     ctk_file_filter_buildable_init                 (CtkBuildableIface *iface);
static void     ctk_file_filter_buildable_set_name             (CtkBuildable *buildable,
                                                                const gchar  *name);
static const gchar* ctk_file_filter_buildable_get_name         (CtkBuildable *buildable);


static gboolean ctk_file_filter_buildable_custom_tag_start     (CtkBuildable  *buildable,
								CtkBuilder    *builder,
								GObject       *child,
								const gchar   *tagname,
								GMarkupParser *parser,
								gpointer      *data);
static void     ctk_file_filter_buildable_custom_tag_end       (CtkBuildable  *buildable,
								CtkBuilder    *builder,
								GObject       *child,
								const gchar   *tagname,
								gpointer      *data);

G_DEFINE_TYPE_WITH_CODE (CtkFileFilter, ctk_file_filter, G_TYPE_INITIALLY_UNOWNED,
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_BUILDABLE,
                                                ctk_file_filter_buildable_init))

static void
ctk_file_filter_init (CtkFileFilter *object G_GNUC_UNUSED)
{
}

static void
ctk_file_filter_class_init (CtkFileFilterClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

  gobject_class->finalize = ctk_file_filter_finalize;
}

static void
filter_rule_free (FilterRule *rule)
{
  switch (rule->type)
    {
    case FILTER_RULE_MIME_TYPE:
      g_free (rule->u.mime_type);
      break;
    case FILTER_RULE_PATTERN:
      g_free (rule->u.pattern);
      break;
    case FILTER_RULE_CUSTOM:
      if (rule->u.custom.notify)
	rule->u.custom.notify (rule->u.custom.data);
      break;
    case FILTER_RULE_PIXBUF_FORMATS:
      g_slist_free (rule->u.pixbuf_formats);
      break;
    default:
      g_assert_not_reached ();
    }

  g_slice_free (FilterRule, rule);
}

static void
ctk_file_filter_finalize (GObject  *object)
{
  CtkFileFilter *filter = CTK_FILE_FILTER (object);

  g_slist_free_full (filter->rules, (GDestroyNotify)filter_rule_free);

  g_free (filter->name);

  G_OBJECT_CLASS (ctk_file_filter_parent_class)->finalize (object);
}

/*
 * CtkBuildable implementation
 */
static void
ctk_file_filter_buildable_init (CtkBuildableIface *iface)
{
  iface->custom_tag_start = ctk_file_filter_buildable_custom_tag_start;
  iface->custom_tag_end = ctk_file_filter_buildable_custom_tag_end;
  iface->set_name = ctk_file_filter_buildable_set_name;
  iface->get_name = ctk_file_filter_buildable_get_name;
}

static void
ctk_file_filter_buildable_set_name (CtkBuildable *buildable,
                                    const gchar  *name)
{
  ctk_file_filter_set_name (CTK_FILE_FILTER (buildable), name);
}

static const gchar *
ctk_file_filter_buildable_get_name (CtkBuildable *buildable)
{
  return ctk_file_filter_get_name (CTK_FILE_FILTER (buildable));
}

typedef enum {
  PARSE_MIME_TYPES,
  PARSE_PATTERNS
} ParserType;

typedef struct {
  CtkFileFilter *filter;
  CtkBuilder    *builder;
  ParserType     type;
  GString       *string;
  gboolean       parsing;
} SubParserData;

static void
parser_start_element (GMarkupParseContext  *context,
                      const gchar          *element_name,
                      const gchar         **names,
                      const gchar         **values,
                      gpointer              user_data,
                      GError              **error)
{
  SubParserData *data = (SubParserData*)user_data;

  if (!g_markup_collect_attributes (element_name, names, values, error,
                                    G_MARKUP_COLLECT_INVALID, NULL, NULL,
                                    G_MARKUP_COLLECT_INVALID))
    {
      _ctk_builder_prefix_error (data->builder, context, error);
      return;
    }

  if (strcmp (element_name, "mime-types") == 0 ||
      strcmp (element_name, "patterns") == 0)
    {
      if (!_ctk_builder_check_parent (data->builder, context, "object", error))
        return;
    }
  else if (strcmp (element_name, "mime-type") == 0)
    {
      if (!_ctk_builder_check_parent (data->builder, context, "mime-types", error))
        return;

      data->parsing = TRUE;
    }
  else if (strcmp (element_name, "pattern") == 0)
    {
      if (!_ctk_builder_check_parent (data->builder, context, "patterns", error))
        return;

      data->parsing = TRUE;
    }
  else
    {
      _ctk_builder_error_unhandled_tag (data->builder, context,
                                        "CtkFileFilter", element_name,
                                        error);
    }
}

static void
parser_text_element (GMarkupParseContext *context G_GNUC_UNUSED,
                     const gchar         *text,
                     gsize                text_len,
                     gpointer             user_data,
                     GError             **error G_GNUC_UNUSED)
{
  SubParserData *data = (SubParserData*)user_data;

  if (data->parsing)
    g_string_append_len (data->string, text, text_len);
}

static void
parser_end_element (GMarkupParseContext *context G_GNUC_UNUSED,
                    const gchar         *element_name G_GNUC_UNUSED,
                    gpointer             user_data,
                    GError             **error G_GNUC_UNUSED)
{
  SubParserData *data = (SubParserData*)user_data;

  if (data->string != NULL && data->string->len != 0)
    {
      switch (data->type)
        {
        case PARSE_MIME_TYPES:
          ctk_file_filter_add_mime_type (data->filter, data->string->str);
          break;
        case PARSE_PATTERNS:
          ctk_file_filter_add_pattern (data->filter, data->string->str);
          break;
        default:
          break;
        }
    }

  g_string_set_size (data->string, 0);
  data->parsing = FALSE;
}

static const GMarkupParser sub_parser =
  {
    .start_element = parser_start_element,
    .end_element = parser_end_element,
    .text = parser_text_element,
  };

static gboolean
ctk_file_filter_buildable_custom_tag_start (CtkBuildable  *buildable,
                                            CtkBuilder    *builder,
                                            GObject       *child G_GNUC_UNUSED,
                                            const gchar   *tagname,
                                            GMarkupParser *parser,
                                            gpointer      *parser_data)
{
  SubParserData *data = NULL;

  if (strcmp (tagname, "mime-types") == 0)
    {
      data = g_slice_new0 (SubParserData);
      data->string = g_string_new ("");
      data->type = PARSE_MIME_TYPES;
      data->filter = CTK_FILE_FILTER (buildable);
      data->builder = builder;

      *parser = sub_parser;
      *parser_data = data;
    }
  else if (strcmp (tagname, "patterns") == 0)
    {
      data = g_slice_new0 (SubParserData);
      data->string = g_string_new ("");
      data->type = PARSE_PATTERNS;
      data->filter = CTK_FILE_FILTER (buildable);
      data->builder = builder;

      *parser = sub_parser;
      *parser_data = data;
    }

  return data != NULL;
}

static void
ctk_file_filter_buildable_custom_tag_end (CtkBuildable *buildable G_GNUC_UNUSED,
                                          CtkBuilder   *builder G_GNUC_UNUSED,
                                          GObject      *child G_GNUC_UNUSED,
                                          const gchar  *tagname,
                                          gpointer     *user_data)
{
  if (strcmp (tagname, "mime-types") == 0 ||
      strcmp (tagname, "patterns") == 0)
    {
      SubParserData *data = (SubParserData*)user_data;

      g_string_free (data->string, TRUE);
      g_slice_free (SubParserData, data);
    }
}


/**
 * ctk_file_filter_new:
 * 
 * Creates a new #CtkFileFilter with no rules added to it.
 * Such a filter doesn’t accept any files, so is not
 * particularly useful until you add rules with
 * ctk_file_filter_add_mime_type(), ctk_file_filter_add_pattern(),
 * or ctk_file_filter_add_custom(). To create a filter
 * that accepts any file, use:
 * |[<!-- language="C" -->
 * CtkFileFilter *filter = ctk_file_filter_new ();
 * ctk_file_filter_add_pattern (filter, "*");
 * ]|
 * 
 * Returns: a new #CtkFileFilter
 * 
 * Since: 2.4
 **/
CtkFileFilter *
ctk_file_filter_new (void)
{
  return g_object_new (CTK_TYPE_FILE_FILTER, NULL);
}

/**
 * ctk_file_filter_set_name:
 * @filter: a #CtkFileFilter
 * @name: (allow-none): the human-readable-name for the filter, or %NULL
 *   to remove any existing name.
 * 
 * Sets the human-readable name of the filter; this is the string
 * that will be displayed in the file selector user interface if
 * there is a selectable list of filters.
 * 
 * Since: 2.4
 **/
void
ctk_file_filter_set_name (CtkFileFilter *filter,
			  const gchar   *name)
{
  g_return_if_fail (CTK_IS_FILE_FILTER (filter));
  
  g_free (filter->name);

  filter->name = g_strdup (name);
}

/**
 * ctk_file_filter_get_name:
 * @filter: a #CtkFileFilter
 * 
 * Gets the human-readable name for the filter. See ctk_file_filter_set_name().
 * 
 * Returns: (nullable): The human-readable name of the filter,
 *   or %NULL. This value is owned by CTK+ and must not
 *   be modified or freed.
 * 
 * Since: 2.4
 **/
const gchar *
ctk_file_filter_get_name (CtkFileFilter *filter)
{
  g_return_val_if_fail (CTK_IS_FILE_FILTER (filter), NULL);
  
  return filter->name;
}

static void
file_filter_add_rule (CtkFileFilter *filter,
		      FilterRule    *rule)
{
  filter->needed |= rule->needed;
  filter->rules = g_slist_append (filter->rules, rule);
}

/**
 * ctk_file_filter_add_mime_type:
 * @filter: A #CtkFileFilter
 * @mime_type: name of a MIME type
 * 
 * Adds a rule allowing a given mime type to @filter.
 * 
 * Since: 2.4
 **/
void
ctk_file_filter_add_mime_type (CtkFileFilter *filter,
			       const gchar   *mime_type)
{
  FilterRule *rule;
  
  g_return_if_fail (CTK_IS_FILE_FILTER (filter));
  g_return_if_fail (mime_type != NULL);

  rule = g_slice_new (FilterRule);
  rule->type = FILTER_RULE_MIME_TYPE;
  rule->needed = CTK_FILE_FILTER_MIME_TYPE;
  rule->u.mime_type = g_strdup (mime_type);

  file_filter_add_rule (filter, rule);
}

/**
 * ctk_file_filter_add_pattern:
 * @filter: a #CtkFileFilter
 * @pattern: a shell style glob
 * 
 * Adds a rule allowing a shell style glob to a filter.
 * 
 * Since: 2.4
 **/
void
ctk_file_filter_add_pattern (CtkFileFilter *filter,
			     const gchar   *pattern)
{
  FilterRule *rule;
  
  g_return_if_fail (CTK_IS_FILE_FILTER (filter));
  g_return_if_fail (pattern != NULL);

  rule = g_slice_new (FilterRule);
  rule->type = FILTER_RULE_PATTERN;
  rule->needed = CTK_FILE_FILTER_DISPLAY_NAME;
  rule->u.pattern = g_strdup (pattern);

  file_filter_add_rule (filter, rule);
}

/**
 * ctk_file_filter_add_pixbuf_formats:
 * @filter: a #CtkFileFilter
 * 
 * Adds a rule allowing image files in the formats supported
 * by GdkPixbuf.
 * 
 * Since: 2.6
 **/
void
ctk_file_filter_add_pixbuf_formats (CtkFileFilter *filter)
{
  FilterRule *rule;
  
  g_return_if_fail (CTK_IS_FILE_FILTER (filter));

  rule = g_slice_new (FilterRule);
  rule->type = FILTER_RULE_PIXBUF_FORMATS;
  rule->needed = CTK_FILE_FILTER_MIME_TYPE;
  rule->u.pixbuf_formats = gdk_pixbuf_get_formats ();
  file_filter_add_rule (filter, rule);
}


/**
 * ctk_file_filter_add_custom:
 * @filter: a #CtkFileFilter
 * @needed: bitfield of flags indicating the information that the custom
 *          filter function needs.
 * @func: callback function; if the function returns %TRUE, then
 *   the file will be displayed.
 * @data: data to pass to @func
 * @notify: function to call to free @data when it is no longer needed.
 * 
 * Adds rule to a filter that allows files based on a custom callback
 * function. The bitfield @needed which is passed in provides information
 * about what sorts of information that the filter function needs;
 * this allows CTK+ to avoid retrieving expensive information when
 * it isn’t needed by the filter.
 * 
 * Since: 2.4
 **/
void
ctk_file_filter_add_custom (CtkFileFilter         *filter,
			    CtkFileFilterFlags     needed,
			    CtkFileFilterFunc      func,
			    gpointer               data,
			    GDestroyNotify         notify)
{
  FilterRule *rule;
  
  g_return_if_fail (CTK_IS_FILE_FILTER (filter));
  g_return_if_fail (func != NULL);

  rule = g_slice_new (FilterRule);
  rule->type = FILTER_RULE_CUSTOM;
  rule->needed = needed;
  rule->u.custom.func = func;
  rule->u.custom.data = data;
  rule->u.custom.notify = notify;

  file_filter_add_rule (filter, rule);
}

/**
 * ctk_file_filter_get_needed:
 * @filter: a #CtkFileFilter
 * 
 * Gets the fields that need to be filled in for the #CtkFileFilterInfo
 * passed to ctk_file_filter_filter()
 * 
 * This function will not typically be used by applications; it
 * is intended principally for use in the implementation of
 * #CtkFileChooser.
 * 
 * Returns: bitfield of flags indicating needed fields when
 *   calling ctk_file_filter_filter()
 * 
 * Since: 2.4
 **/
CtkFileFilterFlags
ctk_file_filter_get_needed (CtkFileFilter *filter)
{
  return filter->needed;
}

#ifdef CDK_WINDOWING_QUARTZ

#import <Foundation/Foundation.h>

NSArray * _ctk_file_filter_get_as_pattern_nsstrings (CtkFileFilter *filter)
{
  NSMutableArray *array = [[NSMutableArray alloc] init];
  GSList *tmp_list;

  for (tmp_list = filter->rules; tmp_list; tmp_list = tmp_list->next)
    {
      FilterRule *rule = tmp_list->data;

      switch (rule->type)
	{
	case FILTER_RULE_CUSTOM:
	  [array release];
          return NULL;
	  break;
	case FILTER_RULE_MIME_TYPE:
	  {
	    // convert mime-types to UTI
	    NSString *mime_type_nsstring = [NSString stringWithUTF8String: rule->u.mime_type];
	    NSString *uti_nsstring = (NSString *) UTTypeCreatePreferredIdentifierForTag (kUTTagClassMIMEType, (CFStringRef) mime_type_nsstring, NULL);
	    if (uti_nsstring == NULL)
	      {
	        [array release];
		return NULL;
	      }
	    [array addObject:uti_nsstring];
	  }
	  break;
	case FILTER_RULE_PATTERN:
	  {
	    // patterns will need to be stripped of their leading *.
	    GString *pattern = g_string_new (rule->u.pattern);
	    if (strncmp (pattern->str, "*.", 2) == 0)
	      {
	        pattern = g_string_erase (pattern, 0, 2);
	      }
	    else if (strncmp (pattern->str, "*", 1) == 0)
	      {
	        pattern = g_string_erase (pattern, 0, 1);
	      }
	    gchar *pattern_c = g_string_free (pattern, FALSE);
	    NSString *pattern_nsstring = [NSString stringWithUTF8String:pattern_c];
	    g_free (pattern_c);
	    [pattern_nsstring retain];
	    [array addObject:pattern_nsstring];
	  }
	  break;
	case FILTER_RULE_PIXBUF_FORMATS:
	  {
	    GSList *list;

	    for (list = rule->u.pixbuf_formats; list; list = list->next)
	      {
		int i;
		gchar **extensions;

		extensions = gdk_pixbuf_format_get_extensions (list->data);

		for (i = 0; extensions[i] != NULL; i++)
		  {
		    NSString *extension = [NSString stringWithUTF8String: extensions[i]];
		    [extension retain];
		    [array addObject:extension];
		  }
		g_strfreev (extensions);
	      }
	    break;
	  }
	}
    }
  return array;
}
#endif

char **
_ctk_file_filter_get_as_patterns (CtkFileFilter      *filter)
{
  GPtrArray *array;
  GSList *tmp_list;

  array = g_ptr_array_new_with_free_func (g_free);

  for (tmp_list = filter->rules; tmp_list; tmp_list = tmp_list->next)
    {
      FilterRule *rule = tmp_list->data;

      switch (rule->type)
	{
	case FILTER_RULE_CUSTOM:
	case FILTER_RULE_MIME_TYPE:
          g_ptr_array_free (array, TRUE);
          return NULL;
	  break;
	case FILTER_RULE_PATTERN:
          g_ptr_array_add (array, g_strdup (rule->u.pattern));
	  break;
	case FILTER_RULE_PIXBUF_FORMATS:
	  {
	    GSList *list;

	    for (list = rule->u.pixbuf_formats; list; list = list->next)
	      {
		int i;
		gchar **extensions;

		extensions = gdk_pixbuf_format_get_extensions (list->data);

		for (i = 0; extensions[i] != NULL; i++)
                  g_ptr_array_add (array, g_strdup_printf ("*.%s", extensions[i]));

		g_strfreev (extensions);
	      }
	    break;
	  }
	}
    }

  g_ptr_array_add (array, NULL); /* Null terminate */
  return (char **)g_ptr_array_free (array, FALSE);
}

/**
 * ctk_file_filter_filter:
 * @filter: a #CtkFileFilter
 * @filter_info: a #CtkFileFilterInfo containing information
 *  about a file.
 * 
 * Tests whether a file should be displayed according to @filter.
 * The #CtkFileFilterInfo @filter_info should include
 * the fields returned from ctk_file_filter_get_needed().
 *
 * This function will not typically be used by applications; it
 * is intended principally for use in the implementation of
 * #CtkFileChooser.
 * 
 * Returns: %TRUE if the file should be displayed
 * 
 * Since: 2.4
 **/
gboolean
ctk_file_filter_filter (CtkFileFilter           *filter,
			const CtkFileFilterInfo *filter_info)
{
  GSList *tmp_list;

  for (tmp_list = filter->rules; tmp_list; tmp_list = tmp_list->next)
    {
      FilterRule *rule = tmp_list->data;

      if ((filter_info->contains & rule->needed) != rule->needed)
	continue;

      switch (rule->type)
	{
	case FILTER_RULE_MIME_TYPE:
          if (filter_info->mime_type != NULL)
            {
              gchar *filter_content_type, *rule_content_type;
              gboolean match;

              filter_content_type = g_content_type_from_mime_type (filter_info->mime_type);
              rule_content_type = g_content_type_from_mime_type (rule->u.mime_type);
              match = filter_content_type != NULL &&
                      rule_content_type != NULL &&
                      g_content_type_is_a (filter_content_type, rule_content_type);
              g_free (filter_content_type);
              g_free (rule_content_type);

              if (match)
                return TRUE;
        }
	  break;
	case FILTER_RULE_PATTERN:
	  if (filter_info->display_name != NULL &&
	      _ctk_fnmatch (rule->u.pattern, filter_info->display_name, FALSE))
	    return TRUE;
	  break;
	case FILTER_RULE_PIXBUF_FORMATS:
	  {
	    GSList *list;

	    if (!filter_info->mime_type)
	      break;

	    for (list = rule->u.pixbuf_formats; list; list = list->next)
	      {
		int i;
		gchar **mime_types;

		mime_types = gdk_pixbuf_format_get_mime_types (list->data);

		for (i = 0; mime_types[i] != NULL; i++)
		  {
		    if (strcmp (mime_types[i], filter_info->mime_type) == 0)
		      {
			g_strfreev (mime_types);
			return TRUE;
		      }
		  }

		g_strfreev (mime_types);
	      }
	    break;
	  }
	case FILTER_RULE_CUSTOM:
	  if (rule->u.custom.func (filter_info, rule->u.custom.data))
	    return TRUE;
	  break;
	}
    }

  return FALSE;
}

/**
 * ctk_file_filter_to_gvariant:
 * @filter: a #CtkFileFilter
 *
 * Serialize a file filter to an a{sv} variant.
 *
 * Returns: (transfer none): a new, floating, #GVariant
 *
 * Since: 3.22
 */
GVariant *
ctk_file_filter_to_gvariant (CtkFileFilter *filter)
{
  GVariantBuilder builder;
  GSList *l;

  g_variant_builder_init (&builder, G_VARIANT_TYPE ("a(us)"));
  for (l = filter->rules; l; l = l->next)
    {
      FilterRule *rule = l->data;

      switch (rule->type)
        {
        case FILTER_RULE_PATTERN:
          g_variant_builder_add (&builder, "(us)", 0, rule->u.pattern);
          break;
        case FILTER_RULE_MIME_TYPE:
          g_variant_builder_add (&builder, "(us)", 1, rule->u.mime_type);
          break;
        case FILTER_RULE_PIXBUF_FORMATS:
          {
	    GSList *f;

	    for (f = rule->u.pixbuf_formats; f; f = f->next)
	      {
                GdkPixbufFormat *fmt = f->data;
                gchar **mime_types;
                int i;

                mime_types = gdk_pixbuf_format_get_mime_types (fmt);
                for (i = 0; mime_types[i]; i++)
                  g_variant_builder_add (&builder, "(us)", 1, mime_types[i]);
                g_strfreev (mime_types);
              }
          }
          break;
        case FILTER_RULE_CUSTOM:
        default:
          break;
        }
    }

  return g_variant_new ("(s@a(us))", filter->name, g_variant_builder_end (&builder));
}

/**
 * ctk_file_filter_new_from_gvariant:
 * @variant: an a{sv} #GVariant
 *
 * Deserialize a file filter from an a{sv} variant in
 * the format produced by ctk_file_filter_to_gvariant().
 *
 * Returns: (transfer full): a new #CtkFileFilter object
 *
 * Since: 3.22
 */
CtkFileFilter *
ctk_file_filter_new_from_gvariant (GVariant *variant)
{
  CtkFileFilter *filter;
  GVariantIter *iter;
  const char *name;
  int type;
  char *tmp;

  filter = ctk_file_filter_new ();

  g_variant_get (variant, "(&sa(us))", &name, &iter);

  ctk_file_filter_set_name (filter, name);

  while (g_variant_iter_next (iter, "(u&s)", &type, &tmp))
    {
      switch (type)
        {
        case 0:
          ctk_file_filter_add_pattern (filter, tmp);
          break;
        case 1:
          ctk_file_filter_add_mime_type (filter, tmp);
          break;
        default:
          break;
       }
    }
  g_variant_iter_free (iter);

  return filter;
}
