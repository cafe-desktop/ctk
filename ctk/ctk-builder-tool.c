/*  Copyright 2015 Red Hat, Inc.
 *
 * CTK+ is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * GLib is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with CTK+; see the file COPYING.  If not,
 * see <http://www.gnu.org/licenses/>.
 *
 * Author: Matthias Clasen
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <glib/gi18n.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>
#include <ctk/ctk.h>
#include "ctkbuilderprivate.h"


typedef struct {
  CtkBuilder *builder;
  GList *classes;
  gboolean packing;
  gboolean packing_started;
  gboolean cell_packing;
  gboolean cell_packing_started;
  gint in_child;
  gint child_started;
  gchar **attribute_names;
  gchar **attribute_values;
  GString *value;
  gboolean unclosed_starttag;
  gint indent;
  char *input_filename;
  char *output_filename;
  FILE *output;
} MyParserData;

static void
canonicalize_key (gchar *key)
{
  gchar *p;

  for (p = key; *p != 0; p++)
    {
      gchar c = *p;

      /* We may meet something like AtkObject::accessible-name */
      if (c == ':' && ((p > key && p[-1] == ':') || p[1] == ':'))
        continue;

      if (c != '-' &&
          (c < '0' || c > '9') &&
          (c < 'A' || c > 'Z') &&
          (c < 'a' || c > 'z'))
        *p = '-';
    }
}

static GParamSpec *
get_property_pspec (MyParserData *data,
                    const gchar  *class_name,
                    const gchar  *property_name)
{
  GType type;
  GObjectClass *class;
  GParamSpec *pspec;
  gchar *canonical_name;

  type = g_type_from_name (class_name);
  if (type == G_TYPE_INVALID)
    return NULL;

  class = g_type_class_ref (type);
  canonical_name = g_strdup (property_name);
  canonicalize_key (canonical_name);
  if (data->packing)
    pspec = ctk_container_class_find_child_property (class, canonical_name);
  else if (data->cell_packing)
    {
      GObjectClass *cell_class;

      /* We're just assuming that the cell layout is using a CtkCellAreaBox. */
      cell_class = g_type_class_ref (CTK_TYPE_CELL_AREA_BOX);
      pspec = ctk_cell_area_class_find_cell_property (CTK_CELL_AREA_CLASS (cell_class), canonical_name);
      g_type_class_unref (cell_class);
    }
  else
    pspec = g_object_class_find_property (class, canonical_name);
  g_free (canonical_name);
  g_type_class_unref (class);

  return pspec;
}


static gboolean
value_is_default (MyParserData *data,
                  const gchar  *class_name,
                  const gchar  *property_name,
                  const gchar  *value_string)
{
  GValue value = { 0, };
  gboolean ret;
  GError *error = NULL;
  GParamSpec *pspec;

  pspec = get_property_pspec (data, class_name, property_name);

  if (pspec == NULL)
    {
      if (data->packing)
        g_printerr (_("Packing property %s::%s not found\n"), class_name, property_name);
      else if (data->cell_packing)
        g_printerr (_("Cell property %s::%s not found\n"), class_name, property_name);
      else
        g_printerr (_("Property %s::%s not found\n"), class_name, property_name);
      return FALSE;
    }
  else if (g_type_is_a (G_PARAM_SPEC_VALUE_TYPE (pspec), G_TYPE_OBJECT))
    return FALSE;

  if (!ctk_builder_value_from_string (data->builder, pspec, value_string, &value, &error))
    {
      g_printerr (_("Couldn't parse value for %s::%s: %s\n"), class_name, property_name, error->message);
      g_error_free (error);
      ret = FALSE;
    }
  else
    ret = g_param_value_defaults (pspec, &value);

  g_value_reset (&value);

  return ret;
}

static gboolean
property_is_boolean (MyParserData *data,
                     const gchar  *class_name,
                     const gchar  *property_name)
{
  GParamSpec *pspec;

  pspec = get_property_pspec (data, class_name, property_name);
  if (pspec)
    return G_PARAM_SPEC_VALUE_TYPE (pspec) == G_TYPE_BOOLEAN;

  return FALSE;
}

static const gchar *
canonical_boolean_value (MyParserData *data,
                         const gchar  *string)
{
  GValue value = G_VALUE_INIT;
  gboolean b = FALSE;

  if (ctk_builder_value_from_string_type (data->builder, G_TYPE_BOOLEAN, string, &value, NULL))
    b = g_value_get_boolean (&value);

  return b ? "1" : "0";
}

/* A number of properties unfortunately can't be omitted even
 * if they are nominally set to their default value. In many
 * cases, this is due to subclasses not overriding the default
 * value from the superclass.
 */
static gboolean
needs_explicit_setting (MyParserData *data,
                        const gchar  *class_name,
                        const gchar  *property_name)
{
  struct _Prop {
    const char *class;
    const char *property;
    gboolean packing;
  } props[] = {
    { "CtkAboutDialog", "program-name", 0 },
    { "CtkCalendar", "year", 0 },
    { "CtkCalendar", "month", 0 },
    { "CtkCalendar", "day", 0 },
    { "CtkDialog", "border-width", 0 },
    { "CtkPlacesSidebar", "show-desktop", 0 },
    { "CtkRadioButton", "draw-indicator", 0 },
    { "CtkGrid", "left-attach", 1 },
    { "CtkGrid", "top-attach", 1 },
    { "CtkWidget", "hexpand", 0 },
    { "CtkWidget", "vexpand", 0 },
    { "CtkContainer", "border-width", 0 },
    { "CtkVBox", "expand", 1 },
    { "CtkHBox", "expand", 1 },
    { NULL, NULL, 0 }
  };
  gchar *canonical_name;
  gboolean found;
  gint k;

  canonical_name = g_strdup (property_name);
  g_strdelimit (canonical_name, "_", '-');

  found = FALSE;
  for (k = 0; props[k].class; k++)
    {
      if (strcmp (class_name, props[k].class) == 0 &&
          strcmp (canonical_name, props[k].property) == 0 &&
          data->packing == props[k].packing)
        {
          found = TRUE;
          break;
        }
    }

  g_free (canonical_name);

  return found;
}

static void
maybe_start_packing (MyParserData *data)
{
  if (data->packing)
    {
      if (!data->packing_started)
        {
          g_fprintf (data->output, "%*s<packing>\n", data->indent, "");
          data->indent += 2;
          data->packing_started = TRUE;
        }
    }
}

static void
maybe_start_cell_packing (MyParserData *data)
{
  if (data->cell_packing)
    {
      if (!data->cell_packing_started)
        {
          g_fprintf (data->output, "%*s<cell-packing>\n", data->indent, "");
          data->indent += 2;
          data->cell_packing_started = TRUE;
        }
    }
}

static void
maybe_start_child (MyParserData *data)
{
  if (data->in_child > 0)
    {
      if (data->child_started < data->in_child)
        {
          g_fprintf (data->output, "%*s<child>\n", data->indent, "");
          data->indent += 2;
          data->child_started += 1;
        }
    }
}

static void
maybe_emit_property (MyParserData *data)
{
  gint i;
  gboolean bound;
  gboolean translatable;
  gchar *escaped;
  const gchar *class_name;
  const gchar *property_name;
  const gchar *value_string;

  class_name = (const gchar *)data->classes->data;
  property_name = "";
  value_string = (const gchar *)data->value->str;

  bound = FALSE;
  translatable = FALSE;
  for (i = 0; data->attribute_names[i]; i++)
    {
      if (strcmp (data->attribute_names[i], "bind-source") == 0 ||
          strcmp (data->attribute_names[i], "bind_source") == 0)
        bound = TRUE;
      else if (strcmp (data->attribute_names[i], "translatable") == 0)
        translatable = TRUE;
      else if (strcmp (data->attribute_names[i], "name") == 0)
        property_name = (const gchar *)data->attribute_values[i];
    }

  if (!translatable &&
      !bound &&
      !needs_explicit_setting (data, class_name, property_name))
    {
      for (i = 0; data->attribute_names[i]; i++)
        {
          if (strcmp (data->attribute_names[i], "name") == 0)
            {
              if (data->classes == NULL)
                break;

              if (value_is_default (data, class_name, property_name, value_string))
                return;
            }
        }
    }

  maybe_start_packing (data);
  maybe_start_cell_packing (data);

  g_fprintf (data->output, "%*s<property", data->indent, "");
  for (i = 0; data->attribute_names[i]; i++)
    {
      if (!translatable &&
          (strcmp (data->attribute_names[i], "comments") == 0 ||
           strcmp (data->attribute_names[i], "context") == 0))
        continue;

      escaped = g_markup_escape_text (data->attribute_values[i], -1);

      if (strcmp (data->attribute_names[i], "name") == 0)
        canonicalize_key (escaped);

      g_fprintf (data->output, " %s=\"%s\"", data->attribute_names[i], escaped);
      g_free (escaped);
    }

  if (bound)
    {
      g_fprintf (data->output, "/>\n");
    }
  else
    {
      g_fprintf (data->output, ">");
      if (property_is_boolean (data, class_name, property_name))
        {
          g_fprintf (data->output, "%s", canonical_boolean_value (data, value_string));
        }
      else
        {
          escaped = g_markup_escape_text (value_string, -1);
          g_fprintf (data->output, "%s", escaped);
          g_free (escaped);
        }
      g_fprintf (data->output, "</property>\n");
    }
}

static void
maybe_close_starttag (MyParserData *data)
{
  if (data->unclosed_starttag)
    {
      g_fprintf (data->output, ">\n");
      data->unclosed_starttag = FALSE;
    }
}

static gboolean
stack_is (GMarkupParseContext *context,
          ...)
{
  va_list args;
  gchar *s;
  const GSList *stack;

  stack = g_markup_parse_context_get_element_stack (context);

  va_start (args, context);
  s = va_arg (args, gchar *);
  while (s)
    {
      gchar *p;

      if (stack == NULL)
        {
          va_end (args);
          return FALSE;
        }

      p = (gchar *)stack->data;
      if (strcmp (s, p) != 0)
        {
          va_end (args);
          return FALSE;
        }

      s = va_arg (args, gchar *);
      stack = stack->next;
    }

  va_end (args);
  return TRUE;
}

static void
start_element (GMarkupParseContext  *context,
               const gchar          *element_name,
               const gchar         **attribute_names,
               const gchar         **attribute_values,
               gpointer              user_data,
               GError              **error G_GNUC_UNUSED)
{
  gint i;
  MyParserData *data = user_data;

  maybe_close_starttag (data);

  if (strcmp (element_name, "property") == 0)
    {
      g_assert (data->attribute_names == NULL);
      g_assert (data->attribute_values == NULL);
      g_assert (data->value == NULL);

      data->attribute_names = g_strdupv ((gchar **)attribute_names);
      data->attribute_values = g_strdupv ((gchar **)attribute_values);
      data->value = g_string_new ("");

      return;
    }
  else if (strcmp (element_name, "packing") == 0)
    {
      data->packing = TRUE;
      data->packing_started = FALSE;

      return;
    }
  else if (strcmp (element_name, "cell-packing") == 0)
    {
      data->cell_packing = TRUE;
      data->cell_packing_started = FALSE;

      return;
    }
  else if (strcmp (element_name, "child") == 0)
    {
      data->in_child += 1;

      if (attribute_names[0] == NULL)
        return;

      data->child_started += 1;
    }
  else if (strcmp (element_name, "attribute") == 0)
    {
      /* attribute in label has no content */
      if (data->classes == NULL ||
          strcmp ((gchar *)data->classes->data, "CtkLabel") != 0)
        data->value = g_string_new ("");
    }
  else if (stack_is (context, "item", "items", NULL) ||
           stack_is (context, "action-widget", "action-widgets", NULL) ||
           stack_is (context, "mime-type", "mime-types", NULL) ||
           stack_is (context, "pattern", "patterns", NULL) ||
           stack_is (context, "application", "applications", NULL) ||
           stack_is (context, "col", "row", "data", NULL) ||
           stack_is (context, "mark", "marks", NULL) ||
           stack_is (context, "action", "accessibility", NULL))
    {
      data->value = g_string_new ("");
    }
  else if (strcmp (element_name, "placeholder") == 0)
    {
      return;
    }
  else if (strcmp (element_name, "object") == 0 ||
           strcmp (element_name, "template") == 0)
    {
      maybe_start_child (data);

      for (i = 0; attribute_names[i]; i++)
        {
          if (strcmp (attribute_names[i], "class") == 0)
            {
              data->classes = g_list_prepend (data->classes,
                                              g_strdup (attribute_values[i]));
              break;
            }
        }
    }

  g_fprintf (data->output, "%*s<%s", data->indent, "", element_name);
  for (i = 0; attribute_names[i]; i++)
    {
      gchar *escaped;

      escaped = g_markup_escape_text (attribute_values[i], -1);
      g_fprintf (data->output, " %s=\"%s\"", attribute_names[i], escaped);
      g_free (escaped);
    }
  data->unclosed_starttag = TRUE;
  data->indent += 2;
}

static void
end_element (GMarkupParseContext  *context G_GNUC_UNUSED,
             const gchar          *element_name,
             gpointer              user_data,
             GError              **error G_GNUC_UNUSED)
{
  MyParserData *data = user_data;

  if (strcmp (element_name, "property") == 0)
    {
      maybe_emit_property (data);

      g_clear_pointer (&data->attribute_names, g_strfreev);
      g_clear_pointer (&data->attribute_values, g_strfreev);
      g_string_free (data->value, TRUE);
      data->value = NULL;
      return;
    }
  else if (strcmp (element_name, "packing") == 0)
    {
      data->packing = FALSE;
      if (!data->packing_started)
        return;
    }
  else if (strcmp (element_name, "cell-packing") == 0)
    {
      data->cell_packing = FALSE;
      if (!data->cell_packing_started)
        return;
    }
  else if (strcmp (element_name, "child") == 0)
    {
      data->in_child -= 1;
      if (data->child_started == data->in_child)
        return;
      data->child_started -= 1;
    }
  else if (strcmp (element_name, "placeholder") == 0)
    {
      return;
    }
  else if (strcmp (element_name, "object") == 0 ||
           strcmp (element_name, "template") == 0)
    {
      g_free (data->classes->data);
      data->classes = g_list_delete_link (data->classes, data->classes);
    }

  if (data->value != NULL)
    {
      gchar *escaped;

      if (data->unclosed_starttag)
        g_fprintf (data->output, ">");

      escaped = g_markup_escape_text (data->value->str, -1);
      g_fprintf (data->output, "%s</%s>\n", escaped, element_name);
      g_free (escaped);

      g_string_free (data->value, TRUE);
      data->value = NULL;
    }
  else
    {
      if (data->unclosed_starttag)
        g_fprintf (data->output, "/>\n");
      else
        g_fprintf (data->output, "%*s</%s>\n", data->indent - 2, "", element_name);
    }

  data->indent -= 2;
  data->unclosed_starttag = FALSE;
}

static void
text (GMarkupParseContext  *context G_GNUC_UNUSED,
      const gchar          *text,
      gsize                 text_len,
      gpointer              user_data,
      GError              **error G_GNUC_UNUSED)
{
  MyParserData *data = user_data;

  if (data->value)
    {
      g_string_append_len (data->value, text, text_len);
      return;
    }
}

static void
passthrough (GMarkupParseContext  *context G_GNUC_UNUSED,
             const gchar          *text,
             gsize                 text_len G_GNUC_UNUSED,
             gpointer              user_data,
             GError              **error G_GNUC_UNUSED)
{
  MyParserData *data = user_data;

  maybe_close_starttag (data);

  g_fprintf (data->output, "%*s%s\n", data->indent, "", text);
}

GMarkupParser parser = {
  start_element,
  end_element,
  text,
  passthrough,
  NULL
};

static void
do_simplify (int          *argc,
             const char ***argv)
{
  GMarkupParseContext *context;
  gchar *buffer;
  MyParserData data;
  gboolean replace = FALSE;
  char **filenames = NULL;
  GOptionContext *ctx;
  const GOptionEntry entries[] = {
    { "replace", 0, 0, G_OPTION_ARG_NONE, &replace, NULL, NULL },
    { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &filenames, NULL, NULL },
    { NULL, }
  };
  GError *error = NULL;

  ctx = g_option_context_new (NULL);
  g_option_context_set_help_enabled (ctx, FALSE);
  g_option_context_add_main_entries (ctx, entries, NULL);

  if (!g_option_context_parse (ctx, argc, (char ***)argv, &error))
    {
      g_printerr ("%s\n", error->message);
      g_error_free (error);
      exit (1);
    }

  g_option_context_free (ctx);

  if (filenames == NULL)
    {
      g_printerr ("No .ui file specified\n");
      exit (1);
    }

  if (g_strv_length (filenames) > 1)
    {
      g_printerr ("Can only simplify a single .ui file\n");
      exit (1);
    }

  data.input_filename = filenames[0];
  data.output_filename = NULL;

  if (replace)
    {
      int fd;
      fd = g_file_open_tmp ("ctk-builder-tool-XXXXXX", &data.output_filename, NULL);
      data.output = fdopen (fd, "w");
    }
  else
    {
      data.output = stdout;
    }

  if (!g_file_get_contents (filenames[0], &buffer, NULL, &error))
    {
      g_printerr (_("Can't load file: %s\n"), error->message);
      exit (1);
    }

  data.builder = ctk_builder_new ();
  data.classes = NULL;
  data.attribute_names = NULL;
  data.attribute_values = NULL;
  data.value = NULL;
  data.packing = FALSE;
  data.packing_started = FALSE;
  data.cell_packing = FALSE;
  data.cell_packing_started = FALSE;
  data.in_child = 0;
  data.child_started = 0;
  data.unclosed_starttag = FALSE;
  data.indent = 0;

  context = g_markup_parse_context_new (&parser, G_MARKUP_TREAT_CDATA_AS_TEXT, &data, NULL);
  if (!g_markup_parse_context_parse (context, buffer, -1, &error))
    {
      g_printerr (_("Can't parse file: %s\n"), error->message);
      exit (1);
    }

  fclose (data.output);

  if (data.output_filename)
    {
      char *content;
      gsize length;

      if (!g_file_get_contents (data.output_filename, &content, &length, &error))
        {
          g_printerr ("Failed to read %s: %s\n", data.output_filename, error->message);
          exit (1);
        }

      if (!g_file_set_contents (data.input_filename, content, length, &error))
        {
          g_printerr ("Failed to write %s: %s\n", data.input_filename, error->message);
          exit (1);
        }
    }
}

static GType
make_fake_type (const gchar *type_name,
                const gchar *parent_name)
{
  GType parent_type;
  GTypeQuery query;

  parent_type = g_type_from_name (parent_name);
  if (parent_type == G_TYPE_INVALID)
    {
      g_printerr ("Failed to lookup template parent type %s\n", parent_name);
      exit (1);
    }

  g_type_query (parent_type, &query);
  return g_type_register_static_simple (parent_type,
                                        type_name,
                                        query.class_size,
                                        NULL,
                                        query.instance_size,
                                        NULL,
                                        0);
}

static void
do_validate_template (const gchar *filename,
                      const gchar *type_name,
                      const gchar *parent_name)
{
  GType template_type;
  CtkWidget *widget;
  CtkBuilder *builder;
  GError *error = NULL;
  gint ret;

  /* Only make a fake type if it doesn't exist yet.
   * This lets us e.g. validate the CtkFileChooserWidget template.
   */
  template_type = g_type_from_name (type_name);
  if (template_type == G_TYPE_INVALID)
    template_type = make_fake_type (type_name, parent_name);

  widget = g_object_new (template_type, NULL);
  if (!widget)
    {
      g_printerr ("Failed to create an instance of the template type %s\n", type_name);
      exit (1);
    }

  builder = ctk_builder_new ();
  ret = ctk_builder_extend_with_template (builder, widget, template_type, " ", 1, &error);
  if (ret)
    ret = ctk_builder_add_from_file (builder, filename, &error);
  g_object_unref (builder);

  if (ret == 0)
    {
      g_printerr ("%s\n", error->message);
      exit (1);
    }
}

static gboolean
parse_template_error (const gchar  *message,
                      gchar       **class_name,
                      gchar       **parent_name)
{
  gchar *p;

  if (!strstr (message, "Not expecting to handle a template"))
    return FALSE;

  p = strstr (message, "(class '");
  if (p)
    {
      *class_name = g_strdup (p + strlen ("(class '"));
      p = strstr (*class_name, "'");
      if (p)
        *p = '\0';
    }
  p = strstr (message, ", parent '");
  if (p)
    {
      *parent_name = g_strdup (p + strlen (", parent '"));
      p = strstr (*parent_name, "'");
      if (p)
        *p = '\0';
    }

  return TRUE;
}

static void
do_validate (const gchar *filename)
{
  CtkBuilder *builder;
  GError *error = NULL;
  gint ret;
  gchar *class_name = NULL;
  gchar *parent_name = NULL;

  builder = ctk_builder_new ();
  ret = ctk_builder_add_from_file (builder, filename, &error);
  g_object_unref (builder);

  if (ret == 0)
    {
      if (g_error_matches (error, CTK_BUILDER_ERROR, CTK_BUILDER_ERROR_UNHANDLED_TAG)  &&
          parse_template_error (error->message, &class_name, &parent_name))
        {
          do_validate_template (filename, class_name, parent_name);
        }
      else
        {
          g_printerr ("%s\n", error->message);
          exit (1);
        }
    }
}

static const gchar *
object_get_name (GObject *object)
{
  if (CTK_IS_BUILDABLE (object))
    return ctk_buildable_get_name (CTK_BUILDABLE (object));
  else
    return g_object_get_data (object, "ctk-builder-name");
}

static void
do_enumerate (const gchar *filename)
{
  CtkBuilder *builder;
  GError *error = NULL;
  gint ret;
  GSList *list, *l;

  builder = ctk_builder_new ();
  ret = ctk_builder_add_from_file (builder, filename, &error);

  if (ret == 0)
    {
      g_printerr ("%s\n", error->message);
      exit (1);
    }

  list = ctk_builder_get_objects (builder);
  for (l = list; l; l = l->next)
    {
      GObject *object;
      const gchar *name;

      object = l->data;
      name = object_get_name (object);
      if (g_str_has_prefix (name, "___") && g_str_has_suffix (name, "___"))
        continue;

      g_printf ("%s (%s)\n", name, g_type_name_from_instance ((GTypeInstance*)object));
    }
  g_slist_free (list);

  g_object_unref (builder);
}

static void
set_window_title (CtkWindow  *window,
                  const char *filename,
                  const char *id)
{
  gchar *name;
  gchar *title;

  name = g_path_get_basename (filename);

  if (id)
    title = g_strdup_printf ("%s in %s", id, name);
  else
    title = g_strdup (name);

  ctk_window_set_title (window, title);

  g_free (title);
  g_free (name);
}

static void
preview_file (const char *filename,
              const char *id,
              const char *cssfile)
{
  CtkBuilder *builder;
  GError *error = NULL;
  GObject *object;
  CtkWidget *window;

  if (cssfile)
    {
      CtkCssProvider *provider;

      provider = ctk_css_provider_new ();
      if (!ctk_css_provider_load_from_path (provider, cssfile, &error))
        {
          g_printerr ("%s\n", error->message);
          exit (1);
        }

      ctk_style_context_add_provider_for_screen (cdk_screen_get_default (),
                                                 CTK_STYLE_PROVIDER (provider),
                                                 CTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }

  builder = ctk_builder_new ();
  if (!ctk_builder_add_from_file (builder, filename, &error))
    {
      g_printerr ("%s\n", error->message);
      exit (1);
    }

  object = NULL;

  if (id)
    {
      object = ctk_builder_get_object (builder, id);
    }
  else
    {
      GSList *objects, *l;

      objects = ctk_builder_get_objects (builder);
      for (l = objects; l; l = l->next)
        {
          GObject *obj = l->data;

          if (CTK_IS_WINDOW (obj))
            {
              object = obj;
              break;
            }
          else if (CTK_IS_WIDGET (obj))
            {
              if (object == NULL)
                object = obj;
            }
        }
      g_slist_free (objects);
    }

  if (object == NULL)
    {
      if (id)
        g_printerr ("No object with ID '%s' found\n", id);
      else
        g_printerr ("No previewable object found\n");
      exit (1);
    }

  if (!CTK_IS_WIDGET (object))
    {
      g_printerr ("Objects of type %s can't be previewed\n", G_OBJECT_TYPE_NAME (object));
      exit (1);
    }

  if (CTK_IS_WINDOW (object))
    window = CTK_WIDGET (object);
  else
    {
      CtkWidget *widget = CTK_WIDGET (object);

      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);

      if (CTK_IS_BUILDABLE (object))
        id = ctk_buildable_get_name (CTK_BUILDABLE (object));

      set_window_title (CTK_WINDOW (window), filename, id);

      g_object_ref (widget);
      if (ctk_widget_get_parent (widget) != NULL)
        ctk_container_remove (CTK_CONTAINER (ctk_widget_get_parent (widget)), widget);
      ctk_container_add (CTK_CONTAINER (window), widget);
      g_object_unref (widget);
    }

  ctk_window_present (CTK_WINDOW (window));

  ctk_main ();

  g_object_unref (builder);
}

static void
do_preview (int          *argc,
            const char ***argv)
{
  GOptionContext *context;
  char *id = NULL;
  char *css = NULL;
  char **filenames = NULL;
  const GOptionEntry entries[] = {
    { "id", 0, 0, G_OPTION_ARG_STRING, &id, NULL, NULL },
    { "css", 0, 0, G_OPTION_ARG_FILENAME, &css, NULL, NULL },
    { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &filenames, NULL, NULL },
    { NULL, }
  };
  GError *error = NULL;

  context = g_option_context_new (NULL);
  g_option_context_set_help_enabled (context, FALSE);
  g_option_context_add_main_entries (context, entries, NULL);

  if (!g_option_context_parse (context, argc, (char ***)argv, &error))
    {
      g_printerr ("%s\n", error->message);
      g_error_free (error);
      exit (1);
    }

  g_option_context_free (context);

  if (filenames == NULL)
    {
      g_printerr ("No .ui file specified\n");
      exit (1);
    }

  if (g_strv_length (filenames) > 1)
    {
      g_printerr ("Can only preview a single .ui file\n");
      exit (1);
    }

  preview_file (filenames[0], id, css);

  g_strfreev (filenames);
  g_free (id);
  g_free (css);
}

static void
usage (void)
{
  g_print (_("Usage:\n"
             "  ctk-builder-tool [COMMAND] FILE\n"
             "\n"
             "Commands:\n"
             "  validate           Validate the file\n"
             "  simplify [OPTIONS] Simplify the file\n"
             "  enumerate          List all named objects\n"
             "  preview [OPTIONS]  Preview the file\n"
             "\n"
             "Simplify Options:\n"
             "  --replace          Replace the file\n"
             "\n"
             "Preview Options:\n"
             "  --id=ID            Preview only the named object\n"
             "  --css=FILE         Use style from CSS file\n"
             "\n"
             "Perform various tasks on CtkBuilder .ui files.\n"));
  exit (1);
}

int
main (int argc, const char *argv[])
{
  g_set_prgname ("ctk-builder-tool");

  ctk_init (NULL, NULL);

  ctk_test_register_all_types ();

  if (argc < 3)
    usage ();

  if (strcmp (argv[2], "--help") == 0)
    usage ();

  argv++;
  argc--;

  if (strcmp (argv[0], "validate") == 0)
    do_validate (argv[1]);
  else if (strcmp (argv[0], "simplify") == 0)
    do_simplify (&argc, &argv);
  else if (strcmp (argv[0], "enumerate") == 0)
    do_enumerate (argv[1]);
  else if (strcmp (argv[0], "preview") == 0)
    do_preview (&argc, &argv);
  else
    usage ();

  return 0;
}
