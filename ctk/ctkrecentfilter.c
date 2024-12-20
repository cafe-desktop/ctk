/* CTK - The GIMP Toolkit
 * ctkrecentfilter.h - Filter object for recently used resources
 * Copyright (C) 2006, Emmanuele Bassi
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
 * SECTION:ctkrecentfilter
 * @Short_Description: A filter for selecting a subset of recently used files
 * @Title: CtkRecentFilter
 *
 * A #CtkRecentFilter can be used to restrict the files being shown
 * in a #CtkRecentChooser.  Files can be filtered based on their name
 * (with ctk_recent_filter_add_pattern()), on their mime type (with
 * ctk_file_filter_add_mime_type()), on the application that has
 * registered them (with ctk_recent_filter_add_application()), or by
 * a custom filter function (with ctk_recent_filter_add_custom()).
 *
 * Filtering by mime type handles aliasing and subclassing of mime
 * types; e.g. a filter for text/plain also matches a file with mime
 * type application/rtf, since application/rtf is a subclass of text/plain.
 * Note that #CtkRecentFilter allows wildcards for the subtype of a
 * mime type, so you can e.g. filter for image/\*.
 *
 * Normally, filters are used by adding them to a #CtkRecentChooser,
 * see ctk_recent_chooser_add_filter(), but it is also possible to
 * manually use a filter on a file with ctk_recent_filter_filter().
 *
 * Recently used files are supported since CTK+ 2.10.
 *
 * ## CtkRecentFilter as CtkBuildable
 *
 * The CtkRecentFilter implementation of the CtkBuildable interface
 * supports adding rules using the <mime-types>, <patterns> and
 * <applications> elements and listing the rules within. Specifying
 * a <mime-type>, <pattern> or <application> has the same effect as
 * calling ctk_recent_filter_add_mime_type(),
 * ctk_recent_filter_add_pattern() or ctk_recent_filter_add_application().
 *
 * An example of a UI definition fragment specifying CtkRecentFilter rules:
 * |[
 * <object class="CtkRecentFilter">
 *   <mime-types>
 *     <mime-type>text/plain</mime-type>
 *     <mime-type>image/png</mime-type>
 *   </mime-types>
 *   <patterns>
 *     <pattern>*.txt</pattern>
 *     <pattern>*.png</pattern>
 *   </patterns>
 *   <applications>
 *     <application>gimp</application>
 *     <application>gedit</application>
 *     <application>glade</application>
 *   </applications>
 * </object>
 * ]|
 */

#include "config.h"
#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#include "ctkrecentfilter.h"
#include "ctkbuildable.h"
#include "ctkbuilderprivate.h"
#include "ctkintl.h"
#include "ctkprivate.h"

static void     ctk_recent_filter_buildable_init                 (CtkBuildableIface *iface);
static gboolean ctk_recent_filter_buildable_custom_tag_start     (CtkBuildable  *buildable,
								  CtkBuilder    *builder,
								  GObject       *child,
								  const gchar   *tagname,
								  GMarkupParser *parser,
								  gpointer      *data);
static void     ctk_recent_filter_buildable_custom_tag_end       (CtkBuildable  *buildable,
								  CtkBuilder    *builder,
								  GObject       *child,
								  const gchar   *tagname,
								  gpointer      *data);

typedef struct _CtkRecentFilterClass CtkRecentFilterClass;
typedef struct _FilterRule FilterRule;

typedef enum {
  FILTER_RULE_URI,
  FILTER_RULE_DISPLAY_NAME,
  FILTER_RULE_MIME_TYPE,
  FILTER_RULE_PIXBUF_FORMATS,
  FILTER_RULE_APPLICATION,
  FILTER_RULE_AGE,
  FILTER_RULE_GROUP,
  FILTER_RULE_CUSTOM
} FilterRuleType;

struct _CtkRecentFilter
{
  GInitiallyUnowned parent_instance;

  gchar *name;
  GSList *rules;
  
  CtkRecentFilterFlags needed;
};

struct _CtkRecentFilterClass
{
  GInitiallyUnownedClass parent_class;
};

struct _FilterRule
{
  FilterRuleType type;
  CtkRecentFilterFlags needed;
  
  union {
    gchar *uri;
    gchar *pattern;
    gchar *mime_type;
    GSList *pixbuf_formats;
    gchar *application;
    gchar *group;
    gint age;
    struct {
      CtkRecentFilterFunc func;
      gpointer data;
      GDestroyNotify data_destroy;
    } custom;
  } u;
};

G_DEFINE_TYPE_WITH_CODE (CtkRecentFilter, ctk_recent_filter, G_TYPE_INITIALLY_UNOWNED,
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_BUILDABLE,
                                                ctk_recent_filter_buildable_init))


static void
filter_rule_free (FilterRule *rule)
{
  switch (rule->type)
    {
    case FILTER_RULE_MIME_TYPE:
      g_free (rule->u.mime_type);
      break;
    case FILTER_RULE_URI:
      g_free (rule->u.uri);
      break;
    case FILTER_RULE_DISPLAY_NAME:
      g_free (rule->u.pattern);
      break;
    case FILTER_RULE_PIXBUF_FORMATS:
      g_slist_free (rule->u.pixbuf_formats);
      break;
    case FILTER_RULE_AGE:
      break;
    case FILTER_RULE_APPLICATION:
      g_free (rule->u.application);
      break;
    case FILTER_RULE_GROUP:
      g_free (rule->u.group);
      break;
    case FILTER_RULE_CUSTOM:
      if (rule->u.custom.data_destroy)
        rule->u.custom.data_destroy (rule->u.custom.data);
      break;
    default:
      g_assert_not_reached ();
      break;
    }
  
  g_free (rule);
}

static void
ctk_recent_filter_finalize (GObject *object)
{
  CtkRecentFilter *filter = CTK_RECENT_FILTER (object);
  
  g_free (filter->name);
  g_slist_free_full (filter->rules, (GDestroyNotify) filter_rule_free);

  G_OBJECT_CLASS (ctk_recent_filter_parent_class)->finalize (object);
}

static void
ctk_recent_filter_class_init (CtkRecentFilterClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  
  gobject_class->finalize = ctk_recent_filter_finalize;
}

static void
ctk_recent_filter_init (CtkRecentFilter *filter G_GNUC_UNUSED)
{

}


/*
 * CtkBuildable implementation
 */
static void
ctk_recent_filter_buildable_init (CtkBuildableIface *iface)
{
  iface->custom_tag_start = ctk_recent_filter_buildable_custom_tag_start;
  iface->custom_tag_end = ctk_recent_filter_buildable_custom_tag_end;
}


typedef enum {
  PARSE_MIME_TYPES,
  PARSE_PATTERNS,
  PARSE_APPLICATIONS
} ParserType;

typedef struct {
  CtkRecentFilter *filter;
  CtkBuilder      *builder;
  ParserType       type;
  GString         *string;
  gboolean         parsing;
} SubParserData;

static void
parser_start_element (GMarkupParseContext *context,
		      const gchar         *element_name,
		      const gchar        **names,
		      const gchar        **values,
		      gpointer             user_data,
		      GError             **error)
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
      strcmp (element_name, "patterns") == 0 ||
      strcmp (element_name, "applications") == 0)
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
  else if (strcmp (element_name, "application") == 0)
    {
      if (!_ctk_builder_check_parent (data->builder, context, "applications", error))
        return;

      data->parsing = TRUE;
    }
  else
    {
      _ctk_builder_error_unhandled_tag (data->builder, context,
                                        "CtkRecentFilter", element_name,
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

  if (data->string)
    {
      switch (data->type)
	{
	case PARSE_MIME_TYPES:
	  ctk_recent_filter_add_mime_type (data->filter, data->string->str);
	  break;
	case PARSE_PATTERNS:
	  ctk_recent_filter_add_pattern (data->filter, data->string->str);
	  break;
	case PARSE_APPLICATIONS:
	  ctk_recent_filter_add_application (data->filter, data->string->str);
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
ctk_recent_filter_buildable_custom_tag_start (CtkBuildable  *buildable,
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
      data->filter = CTK_RECENT_FILTER (buildable);
      data->builder = builder;

      *parser = sub_parser;
      *parser_data = data;
    }
  else if (strcmp (tagname, "patterns") == 0)
    {
      data = g_slice_new0 (SubParserData);
      data->string = g_string_new ("");
      data->type = PARSE_PATTERNS;
      data->filter = CTK_RECENT_FILTER (buildable);
      data->builder = builder;

      *parser = sub_parser;
      *parser_data = data;
    }
  else if (strcmp (tagname, "applications") == 0)
    {
      data = g_slice_new0 (SubParserData);
      data->string = g_string_new ("");
      data->type = PARSE_APPLICATIONS;
      data->filter = CTK_RECENT_FILTER (buildable);
      data->builder = builder;

      *parser = sub_parser;
      *parser_data = data;
    }

  return data != NULL;
}

static void
ctk_recent_filter_buildable_custom_tag_end (CtkBuildable *buildable G_GNUC_UNUSED,
					    CtkBuilder   *builder G_GNUC_UNUSED,
					    GObject      *child G_GNUC_UNUSED,
					    const gchar  *tagname,
					    gpointer     *parser_data)
{
  if (strcmp (tagname, "mime-types") == 0 ||
      strcmp (tagname, "patterns") == 0 ||
      strcmp (tagname, "applications") == 0)
    {
      SubParserData *data = (SubParserData*)parser_data;

      g_string_free (data->string, TRUE);
      g_slice_free (SubParserData, data);
    }
}

/*
 * Public API
 */
 
/**
 * ctk_recent_filter_new:
 *
 * Creates a new #CtkRecentFilter with no rules added to it.
 * Such filter does not accept any recently used resources, so is not
 * particularly useful until you add rules with
 * ctk_recent_filter_add_pattern(), ctk_recent_filter_add_mime_type(),
 * ctk_recent_filter_add_application(), ctk_recent_filter_add_age().
 * To create a filter that accepts any recently used resource, use:
 * |[<!-- language="C" -->
 * CtkRecentFilter *filter = ctk_recent_filter_new ();
 * ctk_recent_filter_add_pattern (filter, "*");
 * ]|
 *
 * Returns: a new #CtkRecentFilter
 *
 * Since: 2.10
 */
CtkRecentFilter *
ctk_recent_filter_new (void)
{
  return g_object_new (CTK_TYPE_RECENT_FILTER, NULL);
}

/**
 * ctk_recent_filter_set_name:
 * @filter: a #CtkRecentFilter
 * @name: then human readable name of @filter
 *
 * Sets the human-readable name of the filter; this is the string
 * that will be displayed in the recently used resources selector
 * user interface if there is a selectable list of filters.
 *
 * Since: 2.10
 */
void
ctk_recent_filter_set_name (CtkRecentFilter *filter,
			    const gchar     *name)
{
  g_return_if_fail (CTK_IS_RECENT_FILTER (filter));
  
  g_free (filter->name);
  
  if (name)
    filter->name = g_strdup (name);
}

/**
 * ctk_recent_filter_get_name:
 * @filter: a #CtkRecentFilter
 *
 * Gets the human-readable name for the filter.
 * See ctk_recent_filter_set_name().
 *
 * Returns: (nullable): the name of the filter, or %NULL.  The returned string
 *   is owned by the filter object and should not be freed.
 *
 * Since: 2.10
 */
const gchar *
ctk_recent_filter_get_name (CtkRecentFilter *filter)
{
  g_return_val_if_fail (CTK_IS_RECENT_FILTER (filter), NULL);
  
  return filter->name;
}

/**
 * ctk_recent_filter_get_needed:
 * @filter: a #CtkRecentFilter
 *
 * Gets the fields that need to be filled in for the #CtkRecentFilterInfo
 * passed to ctk_recent_filter_filter()
 * 
 * This function will not typically be used by applications; it
 * is intended principally for use in the implementation of
 * #CtkRecentChooser.
 * 
 * Returns: bitfield of flags indicating needed fields when
 *   calling ctk_recent_filter_filter()
 *
 * Since: 2.10
 */
CtkRecentFilterFlags
ctk_recent_filter_get_needed (CtkRecentFilter *filter)
{
  return filter->needed;
}

static void
recent_filter_add_rule (CtkRecentFilter *filter,
			FilterRule      *rule)
{
  filter->needed |= rule->needed;
  filter->rules = g_slist_append (filter->rules, rule);
}

/**
 * ctk_recent_filter_add_mime_type:
 * @filter: a #CtkRecentFilter
 * @mime_type: a MIME type
 *
 * Adds a rule that allows resources based on their registered MIME type.
 *
 * Since: 2.10
 */
void
ctk_recent_filter_add_mime_type (CtkRecentFilter *filter,
				 const gchar     *mime_type)
{
  FilterRule *rule;
  
  g_return_if_fail (CTK_IS_RECENT_FILTER (filter));
  g_return_if_fail (mime_type != NULL);
  
  rule = g_new0 (FilterRule, 1);
  rule->type = FILTER_RULE_MIME_TYPE;
  rule->needed = CTK_RECENT_FILTER_MIME_TYPE;
  rule->u.mime_type = g_strdup (mime_type);
  
  recent_filter_add_rule (filter, rule);
}

/**
 * ctk_recent_filter_add_pattern:
 * @filter: a #CtkRecentFilter
 * @pattern: a file pattern
 *
 * Adds a rule that allows resources based on a pattern matching their
 * display name.
 *
 * Since: 2.10
 */
void
ctk_recent_filter_add_pattern (CtkRecentFilter *filter,
			       const gchar     *pattern)
{
  FilterRule *rule;
  
  g_return_if_fail (CTK_IS_RECENT_FILTER (filter));
  g_return_if_fail (pattern != NULL);
  
  rule = g_new0 (FilterRule, 1);
  rule->type = FILTER_RULE_DISPLAY_NAME;
  rule->needed = CTK_RECENT_FILTER_DISPLAY_NAME;
  rule->u.pattern = g_strdup (pattern);
  
  recent_filter_add_rule (filter, rule);
}

/**
 * ctk_recent_filter_add_pixbuf_formats:
 * @filter: a #CtkRecentFilter
 *
 * Adds a rule allowing image files in the formats supported
 * by GdkPixbuf.
 *
 * Since: 2.10
 */
void
ctk_recent_filter_add_pixbuf_formats (CtkRecentFilter *filter)
{
  FilterRule *rule;

  g_return_if_fail (CTK_IS_RECENT_FILTER (filter));

  rule = g_new0 (FilterRule, 1);
  rule->type = FILTER_RULE_PIXBUF_FORMATS;
  rule->needed = CTK_RECENT_FILTER_MIME_TYPE;
  rule->u.pixbuf_formats = gdk_pixbuf_get_formats ();
  
  recent_filter_add_rule (filter, rule);
}

/**
 * ctk_recent_filter_add_application:
 * @filter: a #CtkRecentFilter
 * @application: an application name
 *
 * Adds a rule that allows resources based on the name of the application
 * that has registered them.
 *
 * Since: 2.10
 */
void
ctk_recent_filter_add_application (CtkRecentFilter *filter,
				   const gchar     *application)
{
  FilterRule *rule;
  
  g_return_if_fail (CTK_IS_RECENT_FILTER (filter));
  g_return_if_fail (application != NULL);
  
  rule = g_new0 (FilterRule, 1);
  rule->type = FILTER_RULE_APPLICATION;
  rule->needed = CTK_RECENT_FILTER_APPLICATION;
  rule->u.application = g_strdup (application);
  
  recent_filter_add_rule (filter, rule);
}

/**
 * ctk_recent_filter_add_group:
 * @filter: a #CtkRecentFilter
 * @group: a group name
 *
 * Adds a rule that allows resources based on the name of the group
 * to which they belong
 *
 * Since: 2.10
 */
void
ctk_recent_filter_add_group (CtkRecentFilter *filter,
			     const gchar     *group)
{
  FilterRule *rule;
  
  g_return_if_fail (CTK_IS_RECENT_FILTER (filter));
  g_return_if_fail (group != NULL);
  
  rule = g_new0 (FilterRule, 1);
  rule->type = FILTER_RULE_GROUP;
  rule->needed = CTK_RECENT_FILTER_GROUP;
  rule->u.group = g_strdup (group);
  
  recent_filter_add_rule (filter, rule);
}

/**
 * ctk_recent_filter_add_age:
 * @filter: a #CtkRecentFilter
 * @days: number of days
 *
 * Adds a rule that allows resources based on their age - that is, the number
 * of days elapsed since they were last modified.
 *
 * Since: 2.10
 */
void
ctk_recent_filter_add_age (CtkRecentFilter *filter,
			   gint             days)
{
  FilterRule *rule;
  
  g_return_if_fail (CTK_IS_RECENT_FILTER (filter));
  
  rule = g_new0 (FilterRule, 1);
  rule->type = FILTER_RULE_AGE;
  rule->needed = CTK_RECENT_FILTER_AGE;
  rule->u.age = days;
  
  recent_filter_add_rule (filter, rule);
}

/**
 * ctk_recent_filter_add_custom:
 * @filter: a #CtkRecentFilter
 * @needed: bitfield of flags indicating the information that the custom
 *          filter function needs.
 * @func: callback function; if the function returns %TRUE, then
 *   the file will be displayed.
 * @data: data to pass to @func
 * @data_destroy: function to call to free @data when it is no longer needed.
 * 
 * Adds a rule to a filter that allows resources based on a custom callback
 * function. The bitfield @needed which is passed in provides information
 * about what sorts of information that the filter function needs;
 * this allows CTK+ to avoid retrieving expensive information when
 * it isn’t needed by the filter.
 * 
 * Since: 2.10
 **/
void
ctk_recent_filter_add_custom (CtkRecentFilter      *filter,
			      CtkRecentFilterFlags  needed,
			      CtkRecentFilterFunc   func,
			      gpointer              data,
			      GDestroyNotify        data_destroy)
{
  FilterRule *rule;
  
  g_return_if_fail (CTK_IS_RECENT_FILTER (filter));
  g_return_if_fail (func != NULL);

  rule = g_new0 (FilterRule, 1);
  rule->type = FILTER_RULE_CUSTOM;
  rule->needed = needed;
  rule->u.custom.func = func;
  rule->u.custom.data = data;
  rule->u.custom.data_destroy = data_destroy;

  recent_filter_add_rule (filter, rule);
}


/**
 * ctk_recent_filter_filter:
 * @filter: a #CtkRecentFilter
 * @filter_info: a #CtkRecentFilterInfo containing information
 *   about a recently used resource
 *
 * Tests whether a file should be displayed according to @filter.
 * The #CtkRecentFilterInfo @filter_info should include
 * the fields returned from ctk_recent_filter_get_needed(), and
 * must set the #CtkRecentFilterInfo.contains field of @filter_info
 * to indicate which fields have been set.
 *
 * This function will not typically be used by applications; it
 * is intended principally for use in the implementation of
 * #CtkRecentChooser.
 * 
 * Returns: %TRUE if the file should be displayed
 *
 * Since: 2.10
 */
gboolean
ctk_recent_filter_filter (CtkRecentFilter           *filter,
			  const CtkRecentFilterInfo *filter_info)
{
  GSList *l;
  
  g_return_val_if_fail (CTK_IS_RECENT_FILTER (filter), FALSE);
  g_return_val_if_fail (filter_info != NULL, FALSE);
  
  for (l = filter->rules; l != NULL; l = l->next)
    {
      FilterRule *rule = (FilterRule *) l->data;

      if ((filter_info->contains & rule->needed) != rule->needed)
        continue;

      switch (rule->type)
        {
        case FILTER_RULE_MIME_TYPE:
          if (filter_info->mime_type != NULL)
            {
              gchar *filter_content_type, *rule_content_type;
              gboolean match = FALSE;

              filter_content_type = g_content_type_from_mime_type (filter_info->mime_type);
              rule_content_type = g_content_type_from_mime_type (rule->u.mime_type);
              if (filter_content_type != NULL && rule_content_type != NULL)
                match = g_content_type_is_a (filter_content_type, rule_content_type);
              g_free (filter_content_type);
              g_free (rule_content_type);

              if (match)
                return TRUE;
            }
          break;
        case FILTER_RULE_APPLICATION:
          if (filter_info->applications)
            {
              gint i;
              
              for (i = 0; filter_info->applications[i] != NULL; i++)
                {
                  if (strcmp (filter_info->applications[i], rule->u.application) == 0)
                    return TRUE;
                }
            }
          break;
	case FILTER_RULE_GROUP:
	  if (filter_info->groups)
            {
	      gint i;

	      for (i = 0; filter_info->groups[i] != NULL; i++)
		{
		  if (strcmp (filter_info->groups[i], rule->u.group) == 0)
		    return TRUE;
		}
	    }
	  break;
	case FILTER_RULE_PIXBUF_FORMATS:
	  {
            GSList *list;
	    if (!filter_info->mime_type)
	      break;

	    for (list = rule->u.pixbuf_formats; list; list = list->next)
              {
                gint i;
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
        case FILTER_RULE_URI:
          if ((filter_info->uri != NULL) &&
              _ctk_fnmatch (rule->u.uri, filter_info->uri, FALSE))
            return TRUE;
          break;
        case FILTER_RULE_DISPLAY_NAME:
          if ((filter_info->display_name != NULL) &&
              _ctk_fnmatch (rule->u.pattern, filter_info->display_name, FALSE))
            return TRUE;
          break;
        case FILTER_RULE_AGE:
          if ((filter_info->age != -1) &&
              (filter_info->age < rule->u.age))
            return TRUE;
          break;
        case FILTER_RULE_CUSTOM:
          if (rule->u.custom.func (filter_info, rule->u.custom.data))
            return TRUE;
          break;
        }
    }
  
  return FALSE;
}
