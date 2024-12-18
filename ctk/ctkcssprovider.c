/* CTK - The GIMP Toolkit
 * Copyright (C) 2010 Carlos Garnacho <carlosg@gnome.org>
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

#include <string.h>
#include <stdlib.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <cairo-gobject.h>

#include "ctkcssproviderprivate.h"

#include "ctkbitmaskprivate.h"
#include "ctkcssarrayvalueprivate.h"
#include "ctkcsscolorvalueprivate.h"
#include "ctkcsskeyframesprivate.h"
#include "ctkcssparserprivate.h"
#include "ctkcsssectionprivate.h"
#include "ctkcssselectorprivate.h"
#include "ctkcssshorthandpropertyprivate.h"
#include "ctkcssstylefuncsprivate.h"
#include "ctksettingsprivate.h"
#include "ctkstyleprovider.h"
#include "ctkstylecontextprivate.h"
#include "ctkstylepropertyprivate.h"
#include "ctkstyleproviderprivate.h"
#include "ctkwidgetpath.h"
#include "ctkbindings.h"
#include "ctkmarshalers.h"
#include "ctkprivate.h"
#include "ctkintl.h"
#include "ctkutilsprivate.h"
#include "ctkversion.h"

/**
 * SECTION:ctkcssprovider
 * @Short_description: CSS-like styling for widgets
 * @Title: CtkCssProvider
 * @See_also: #CtkStyleContext, #CtkStyleProvider
 *
 * CtkCssProvider is an object implementing the #CtkStyleProvider interface.
 * It is able to parse [CSS-like][css-overview] input in order to style widgets.
 *
 * An application can make CTK+ parse a specific CSS style sheet by calling
 * ctk_css_provider_load_from_file() or ctk_css_provider_load_from_resource()
 * and adding the provider with ctk_style_context_add_provider() or
 * ctk_style_context_add_provider_for_screen().

 * In addition, certain files will be read when CTK+ is initialized. First, the
 * file `$XDG_CONFIG_HOME/ctk-3.0/ctk.css` is loaded if it exists. Then, CTK+
 * loads the first existing file among
 * `XDG_DATA_HOME/themes/THEME/ctk-VERSION/ctk.css`,
 * `$HOME/.themes/THEME/ctk-VERSION/ctk.css`,
 * `$XDG_DATA_DIRS/themes/THEME/ctk-VERSION/ctk.css` and
 * `DATADIR/share/themes/THEME/ctk-VERSION/ctk.css`, where `THEME` is the name of
 * the current theme (see the #CtkSettings:ctk-theme-name setting), `DATADIR`
 * is the prefix configured when CTK+ was compiled (unless overridden by the
 * `CTK_DATA_PREFIX` environment variable), and `VERSION` is the CTK+ version number.
 * If no file is found for the current version, CTK+ tries older versions all the
 * way back to 3.0.
 *
 * In the same way, CTK+ tries to load a ctk-keys.css file for the current
 * key theme, as defined by #CtkSettings:ctk-key-theme-name.
 */


typedef struct CtkCssRuleset CtkCssRuleset;
typedef struct _CtkCssScanner CtkCssScanner;
typedef struct _PropertyValue PropertyValue;
typedef struct _WidgetPropertyValue WidgetPropertyValue;
typedef enum ParserScope ParserScope;
typedef enum ParserSymbol ParserSymbol;

struct _PropertyValue {
  CtkCssStyleProperty *property;
  CtkCssValue         *value;
  CtkCssSection       *section;
};

struct _WidgetPropertyValue {
  WidgetPropertyValue *next;
  char *name;
  char *value;

  CtkCssSection *section;
};

struct CtkCssRuleset
{
  CtkCssSelector *selector;
  CtkCssSelectorTree *selector_match;
  WidgetPropertyValue *widget_style;
  PropertyValue *styles;
  CtkBitmask *set_styles;
  guint n_styles;
  guint owns_styles : 1;
  guint owns_widget_style : 1;
};

struct _CtkCssScanner
{
  CtkCssProvider *provider;
  CtkCssParser *parser;
  CtkCssSection *section;
  CtkCssScanner *parent;
  GSList *state;
};

struct _CtkCssProviderPrivate
{
  GScanner *scanner;

  GHashTable *symbolic_colors;
  GHashTable *keyframes;

  GArray *rulesets;
  CtkCssSelectorTree *tree;
  GResource *resource;
  gchar *path;
};

enum {
  PARSING_ERROR,
  LAST_SIGNAL
};

static gboolean ctk_keep_css_sections = FALSE;

static guint css_provider_signals[LAST_SIGNAL] = { 0 };

static void ctk_css_provider_finalize (GObject *object);
static void ctk_css_style_provider_iface_init (CtkStyleProviderIface *iface);
static void ctk_css_style_provider_private_iface_init (CtkStyleProviderPrivateInterface *iface);
static void widget_property_value_list_free (WidgetPropertyValue *head);
static void ctk_css_style_provider_emit_error (CtkStyleProviderPrivate *provider,
                                               CtkCssSection           *section,
                                               const GError            *error);

static gboolean
ctk_css_provider_load_internal (CtkCssProvider *css_provider,
                                CtkCssScanner  *scanner,
                                GFile          *file,
                                const char     *data,
                                GError        **error);

GQuark
ctk_css_provider_error_quark (void)
{
  return g_quark_from_static_string ("ctk-css-provider-error-quark");
}

G_DEFINE_TYPE_EXTENDED (CtkCssProvider, ctk_css_provider, G_TYPE_OBJECT, 0,
                        G_ADD_PRIVATE (CtkCssProvider)
                        G_IMPLEMENT_INTERFACE (CTK_TYPE_STYLE_PROVIDER,
                                               ctk_css_style_provider_iface_init)
                        G_IMPLEMENT_INTERFACE (CTK_TYPE_STYLE_PROVIDER_PRIVATE,
                                               ctk_css_style_provider_private_iface_init));

static void
ctk_css_provider_parsing_error (CtkCssProvider  *provider,
                                CtkCssSection   *section,
                                const GError    *error)
{
  /* Only emit a warning when we have no error handlers. This is our
   * default handlers. And in this case erroneous CSS files are a bug
   * and should be fixed.
   * Note that these warnings can also be triggered by a broken theme
   * that people installed from some weird location on the internets.
   */
  if (!g_signal_has_handler_pending (provider,
                                     css_provider_signals[PARSING_ERROR],
                                     0,
                                     TRUE))
    {
      char *s = _ctk_css_section_to_string (section);

      g_warning ("Theme parsing error: %s: %s",
                 s,
                 error->message);

      g_free (s);
    }
}

/* This is exported privately for use in CtkInspector.
 * It is the callers responsibility to reparse the current theme.
 */
void
ctk_css_provider_set_keep_css_sections (void)
{
  ctk_keep_css_sections = TRUE;
}

static void
ctk_css_provider_class_init (CtkCssProviderClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  if (g_getenv ("CTK_CSS_DEBUG"))
    ctk_css_provider_set_keep_css_sections ();

  /**
   * CtkCssProvider::parsing-error:
   * @provider: the provider that had a parsing error
   * @section: section the error happened in
   * @error: The parsing error
   *
   * Signals that a parsing error occurred. the @path, @line and @position
   * describe the actual location of the error as accurately as possible.
   *
   * Parsing errors are never fatal, so the parsing will resume after
   * the error. Errors may however cause parts of the given
   * data or even all of it to not be parsed at all. So it is a useful idea
   * to check that the parsing succeeds by connecting to this signal.
   *
   * Note that this signal may be emitted at any time as the css provider
   * may opt to defer parsing parts or all of the input to a later time
   * than when a loading function was called.
   */
  css_provider_signals[PARSING_ERROR] =
    g_signal_new (I_("parsing-error"),
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkCssProviderClass, parsing_error),
                  NULL, NULL,
                  _ctk_marshal_VOID__BOXED_BOXED,
                  G_TYPE_NONE, 2, CTK_TYPE_CSS_SECTION, G_TYPE_ERROR);

  object_class->finalize = ctk_css_provider_finalize;

  klass->parsing_error = ctk_css_provider_parsing_error;
}

static void
ctk_css_ruleset_init_copy (CtkCssRuleset       *new,
                           CtkCssRuleset       *ruleset,
                           CtkCssSelector      *selector)
{
  memcpy (new, ruleset, sizeof (CtkCssRuleset));

  new->selector = selector;
  /* First copy takes over ownership */
  if (ruleset->owns_styles)
    ruleset->owns_styles = FALSE;
  if (ruleset->owns_widget_style)
    ruleset->owns_widget_style = FALSE;
  if (new->set_styles)
    new->set_styles = _ctk_bitmask_copy (new->set_styles);
}

static void
ctk_css_ruleset_clear (CtkCssRuleset *ruleset)
{
  if (ruleset->owns_styles)
    {
      guint i;

      for (i = 0; i < ruleset->n_styles; i++)
        {
          _ctk_css_value_unref (ruleset->styles[i].value);
	  ruleset->styles[i].value = NULL;
	  if (ruleset->styles[i].section)
	    ctk_css_section_unref (ruleset->styles[i].section);
        }
      g_free (ruleset->styles);
    }
  if (ruleset->set_styles)
    _ctk_bitmask_free (ruleset->set_styles);
  if (ruleset->owns_widget_style)
    widget_property_value_list_free (ruleset->widget_style);
  if (ruleset->selector)
    _ctk_css_selector_free (ruleset->selector);

  memset (ruleset, 0, sizeof (CtkCssRuleset));
}

static WidgetPropertyValue *
widget_property_value_new (char *name, CtkCssSection *section)
{
  WidgetPropertyValue *value;

  value = g_slice_new0 (WidgetPropertyValue);

  value->name = name;
  if (ctk_keep_css_sections)
    value->section = ctk_css_section_ref (section);

  return value;
}

static void
widget_property_value_free (WidgetPropertyValue *value)
{
  g_free (value->value);
  g_free (value->name);
  if (value->section)
    ctk_css_section_unref (value->section);

  g_slice_free (WidgetPropertyValue, value);
}

static void
widget_property_value_list_free (WidgetPropertyValue *head)
{
  WidgetPropertyValue *l, *next;
  for (l = head; l != NULL; l = next)
    {
      next = l->next;
      widget_property_value_free (l);
    }
}

static WidgetPropertyValue *
widget_property_value_list_remove_name (WidgetPropertyValue *head, const char *name)
{
  WidgetPropertyValue *l, **last;

  last = &head;

  for (l = head; l != NULL; l = l->next)
    {
      if (strcmp (l->name, name) == 0)
	{
	  *last = l->next;
	  widget_property_value_free (l);
	  break;
	}

      last = &l->next;
    }

  return head;
}

static void
ctk_css_ruleset_add_style (CtkCssRuleset *ruleset,
                           char          *name,
                           WidgetPropertyValue *value)
{
  value->next = widget_property_value_list_remove_name (ruleset->widget_style, name);
  ruleset->widget_style = value;
  ruleset->owns_widget_style = TRUE;
}

static void
ctk_css_ruleset_add (CtkCssRuleset       *ruleset,
                     CtkCssStyleProperty *property,
                     CtkCssValue         *value,
                     CtkCssSection       *section)
{
  guint i;

  g_return_if_fail (ruleset->owns_styles || ruleset->n_styles == 0);

  if (ruleset->set_styles == NULL)
    ruleset->set_styles = _ctk_bitmask_new ();

  ruleset->set_styles = _ctk_bitmask_set (ruleset->set_styles,
                                          _ctk_css_style_property_get_id (property),
                                          TRUE);

  ruleset->owns_styles = TRUE;

  for (i = 0; i < ruleset->n_styles; i++)
    {
      if (ruleset->styles[i].property == property)
        {
          _ctk_css_value_unref (ruleset->styles[i].value);
	  ruleset->styles[i].value = NULL;
	  if (ruleset->styles[i].section)
	    ctk_css_section_unref (ruleset->styles[i].section);
          break;
        }
    }
  if (i == ruleset->n_styles)
    {
      ruleset->n_styles++;
      ruleset->styles = g_realloc (ruleset->styles, ruleset->n_styles * sizeof (PropertyValue));
      ruleset->styles[i].value = NULL;
      ruleset->styles[i].property = property;
    }

  ruleset->styles[i].value = value;
  if (ctk_keep_css_sections)
    ruleset->styles[i].section = ctk_css_section_ref (section);
  else
    ruleset->styles[i].section = NULL;
}

static void
ctk_css_scanner_destroy (CtkCssScanner *scanner)
{
  if (scanner->section)
    ctk_css_section_unref (scanner->section);
  g_object_unref (scanner->provider);
  _ctk_css_parser_free (scanner->parser);

  g_slice_free (CtkCssScanner, scanner);
}

static void
ctk_css_style_provider_emit_error (CtkStyleProviderPrivate *provider,
                                   CtkCssSection           *section,
                                   const GError            *error)
{
  g_signal_emit (provider, css_provider_signals[PARSING_ERROR], 0, section, error);
}

static void
ctk_css_provider_emit_error (CtkCssProvider *provider,
                             CtkCssScanner  *scanner,
                             const GError   *error)
{
  ctk_css_style_provider_emit_error (CTK_STYLE_PROVIDER_PRIVATE (provider),
                                     scanner ? scanner->section : NULL,
                                     error);
}

static void
ctk_css_scanner_parser_error (CtkCssParser *parser G_GNUC_UNUSED,
                              const GError *error,
                              gpointer      user_data)
{
  CtkCssScanner *scanner = user_data;

  ctk_css_provider_emit_error (scanner->provider, scanner, error);
}

static CtkCssScanner *
ctk_css_scanner_new (CtkCssProvider *provider,
                     CtkCssScanner  *parent,
                     CtkCssSection  *section,
                     GFile          *file,
                     const gchar    *text)
{
  CtkCssScanner *scanner;

  scanner = g_slice_new0 (CtkCssScanner);

  g_object_ref (provider);
  scanner->provider = provider;
  scanner->parent = parent;
  if (section)
    scanner->section = ctk_css_section_ref (section);

  scanner->parser = _ctk_css_parser_new (text,
                                         file,
                                         ctk_css_scanner_parser_error,
                                         scanner);

  return scanner;
}

static gboolean
ctk_css_scanner_would_recurse (CtkCssScanner *scanner,
                               GFile         *file)
{
  while (scanner)
    {
      GFile *parser_file = _ctk_css_parser_get_file (scanner->parser);
      if (parser_file && g_file_equal (parser_file, file))
        return TRUE;

      scanner = scanner->parent;
    }

  return FALSE;
}

static void
ctk_css_scanner_push_section (CtkCssScanner     *scanner,
                              CtkCssSectionType  section_type)
{
  CtkCssSection *section;

  section = _ctk_css_section_new (scanner->section,
                                  section_type,
                                  scanner->parser);

  if (scanner->section)
    ctk_css_section_unref (scanner->section);
  scanner->section = section;
}

static void
ctk_css_scanner_pop_section (CtkCssScanner *scanner,
                             CtkCssSectionType check_type)
{
  CtkCssSection *parent;
  
  g_assert (ctk_css_section_get_section_type (scanner->section) == check_type);

  parent = ctk_css_section_get_parent (scanner->section);
  if (parent)
    ctk_css_section_ref (parent);

  _ctk_css_section_end (scanner->section);
  ctk_css_section_unref (scanner->section);

  scanner->section = parent;
}

static void
ctk_css_provider_init (CtkCssProvider *css_provider)
{
  CtkCssProviderPrivate *priv;

  priv = css_provider->priv = ctk_css_provider_get_instance_private (css_provider);

  priv->rulesets = g_array_new (FALSE, FALSE, sizeof (CtkCssRuleset));

  priv->symbolic_colors = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                 (GDestroyNotify) g_free,
                                                 (GDestroyNotify) _ctk_css_value_unref);
  priv->keyframes = g_hash_table_new_full (g_str_hash, g_str_equal,
                                           (GDestroyNotify) g_free,
                                           (GDestroyNotify) _ctk_css_keyframes_unref);
}

static void
verify_tree_match_results (CtkCssProvider      *provider G_GNUC_UNUSED,
			   const CtkCssMatcher *matcher G_GNUC_UNUSED,
			   GPtrArray           *tree_rules G_GNUC_UNUSED)
{
#ifdef VERIFY_TREE
  CtkCssProviderPrivate *priv = provider->priv;
  CtkCssRuleset *ruleset;
  gboolean should_match;
  int i, j;

  for (i = 0; i < priv->rulesets->len; i++)
    {
      gboolean found = FALSE;

      ruleset = &g_array_index (priv->rulesets, CtkCssRuleset, i);

      for (j = 0; j < tree_rules->len; j++)
	{
	  if (ruleset == tree_rules->pdata[j])
	    {
	      found = TRUE;
	      break;
	    }
	}
      should_match = _ctk_css_selector_matches (ruleset->selector, matcher);
      if (found != !!should_match)
	{
	  g_error ("expected rule '%s' to %s, but it %s",
		   _ctk_css_selector_to_string (ruleset->selector),
		   should_match ? "match" : "not match",
		   found ? "matched" : "didn't match");
	}
    }
#endif
}

static void
verify_tree_get_change_results (CtkCssProvider      *provider G_GNUC_UNUSED,
				const CtkCssMatcher *matcher G_GNUC_UNUSED,
				CtkCssChange         change G_GNUC_UNUSED)
{
#ifdef VERIFY_TREE
  {
    CtkCssChange verify_change = 0;
    GPtrArray *tree_rules;
    int i;

    tree_rules = _ctk_css_selector_tree_match_all (provider->priv->tree, matcher);
    if (tree_rules)
      {
        verify_tree_match_results (provider, matcher, tree_rules);

        for (i = tree_rules->len - 1; i >= 0; i--)
          {
	    CtkCssRuleset *ruleset;

            ruleset = tree_rules->pdata[i];

            verify_change |= _ctk_css_selector_get_change (ruleset->selector);
          }

        g_ptr_array_free (tree_rules, TRUE);
      }

    if (change != verify_change)
      {
	GString *s;

	s = g_string_new ("");
	g_string_append (s, "expected change ");
        ctk_css_change_print (verify_change, s);
        g_string_append (s, ", but it was ");
        ctk_css_change_print (change, s);
	if ((change & ~verify_change) != 0)
          {
	    g_string_append (s, ", unexpectedly set: ");
            ctk_css_change_print (change & ~verify_change, s);
          }
	if ((~change & verify_change) != 0)
          {
	    g_string_append_printf (s, ", unexpectedly not set: ");
            ctk_css_change_print (~change & verify_change, s);
          }
	g_warning (s->str);
	g_string_free (s, TRUE);
      }
  }
#endif
}


static gboolean
ctk_css_provider_get_style_property (CtkStyleProvider *provider,
                                     CtkWidgetPath    *path,
                                     CtkStateFlags     state,
                                     GParamSpec       *pspec,
                                     GValue           *value)
{
  CtkCssProvider *css_provider = CTK_CSS_PROVIDER (provider);
  CtkCssProviderPrivate *priv = css_provider->priv;
  WidgetPropertyValue *val;
  GPtrArray *tree_rules;
  CtkCssMatcher matcher;
  gboolean found = FALSE;
  gchar *prop_name;
  gint i;

  if (state == ctk_widget_path_iter_get_state (path, -1))
    {
      ctk_widget_path_ref (path);
    }
  else
    {
      path = ctk_widget_path_copy (path);
      ctk_widget_path_iter_set_state (path, -1, state);
    }

  if (!_ctk_css_matcher_init (&matcher, path, NULL))
    {
      ctk_widget_path_unref (path);
      return FALSE;
    }

  tree_rules = _ctk_css_selector_tree_match_all (priv->tree, &matcher);
  if (tree_rules)
    {
      verify_tree_match_results (css_provider, &matcher, tree_rules);

      prop_name = g_strdup_printf ("-%s-%s",
                                   g_type_name (pspec->owner_type),
                                   pspec->name);

      for (i = tree_rules->len - 1; i >= 0; i--)
        {
          CtkCssRuleset *ruleset = tree_rules->pdata[i];

          if (ruleset->widget_style == NULL)
            continue;

          for (val = ruleset->widget_style; val != NULL; val = val->next)
            {
              if (strcmp (val->name, prop_name) == 0)
                {
                  CtkCssScanner *scanner;

	          scanner = ctk_css_scanner_new (css_provider,
                                                 NULL,
                                                 val->section,
                                                 val->section != NULL ? ctk_css_section_get_file (val->section) : NULL,
                                                 val->value);
                  if (!val->section)
                    ctk_css_scanner_push_section (scanner, CTK_CSS_SECTION_VALUE);
                  found = _ctk_css_style_funcs_parse_value (value, scanner->parser);
                  if (!val->section)
                    ctk_css_scanner_pop_section (scanner, CTK_CSS_SECTION_VALUE);
                  ctk_css_scanner_destroy (scanner);
	          break;
                }
            }

          if (found)
            break;
        }

      g_free (prop_name);
      g_ptr_array_free (tree_rules, TRUE);
    }

  ctk_widget_path_unref (path);

  return found;
}

static void
ctk_css_style_provider_iface_init (CtkStyleProviderIface *iface)
{
  iface->get_style_property = ctk_css_provider_get_style_property;
}

static CtkCssValue *
ctk_css_style_provider_get_color (CtkStyleProviderPrivate *provider,
                                  const char              *name)
{
  CtkCssProvider *css_provider = CTK_CSS_PROVIDER (provider);

  return g_hash_table_lookup (css_provider->priv->symbolic_colors, name);
}

static CtkCssKeyframes *
ctk_css_style_provider_get_keyframes (CtkStyleProviderPrivate *provider,
                                      const char              *name)
{
  CtkCssProvider *css_provider = CTK_CSS_PROVIDER (provider);

  return g_hash_table_lookup (css_provider->priv->keyframes, name);
}

static void
ctk_css_style_provider_lookup (CtkStyleProviderPrivate *provider,
                               const CtkCssMatcher     *matcher,
                               CtkCssLookup            *lookup,
                               CtkCssChange            *change)
{
  CtkCssProvider *css_provider;
  CtkCssProviderPrivate *priv;
  CtkCssRuleset *ruleset;
  guint j;
  int i;
  GPtrArray *tree_rules;

  css_provider = CTK_CSS_PROVIDER (provider);
  priv = css_provider->priv;

  tree_rules = _ctk_css_selector_tree_match_all (priv->tree, matcher);
  if (tree_rules)
    {
      verify_tree_match_results (css_provider, matcher, tree_rules);

      for (i = tree_rules->len - 1; i >= 0; i--)
        {
          ruleset = tree_rules->pdata[i];

          if (ruleset->styles == NULL)
            continue;

          if (!_ctk_bitmask_intersects (_ctk_css_lookup_get_missing (lookup),
                                        ruleset->set_styles))
          continue;

          for (j = 0; j < ruleset->n_styles; j++)
            {
              CtkCssStyleProperty *prop = ruleset->styles[j].property;
              guint id = _ctk_css_style_property_get_id (prop);

              if (!_ctk_css_lookup_is_missing (lookup, id))
                continue;

              _ctk_css_lookup_set (lookup,
                                   id,
                                   ruleset->styles[j].section,
                                  ruleset->styles[j].value);
            }

          if (_ctk_bitmask_is_empty (_ctk_css_lookup_get_missing (lookup)))
            break;
        }

      g_ptr_array_free (tree_rules, TRUE);
    }

  if (change)
    {
      CtkCssMatcher change_matcher;

      _ctk_css_matcher_superset_init (&change_matcher, matcher, CTK_CSS_CHANGE_NAME | CTK_CSS_CHANGE_CLASS);

      *change = _ctk_css_selector_tree_get_change_all (priv->tree, &change_matcher);
      verify_tree_get_change_results (css_provider, &change_matcher, *change);
    }
}

static void
ctk_css_style_provider_private_iface_init (CtkStyleProviderPrivateInterface *iface)
{
  iface->get_color = ctk_css_style_provider_get_color;
  iface->get_keyframes = ctk_css_style_provider_get_keyframes;
  iface->lookup = ctk_css_style_provider_lookup;
  iface->emit_error = ctk_css_style_provider_emit_error;
}

static void
ctk_css_provider_finalize (GObject *object)
{
  CtkCssProvider *css_provider;
  CtkCssProviderPrivate *priv;
  guint i;

  css_provider = CTK_CSS_PROVIDER (object);
  priv = css_provider->priv;

  for (i = 0; i < priv->rulesets->len; i++)
    ctk_css_ruleset_clear (&g_array_index (priv->rulesets, CtkCssRuleset, i));

  g_array_free (priv->rulesets, TRUE);
  _ctk_css_selector_tree_free (priv->tree);

  g_hash_table_destroy (priv->symbolic_colors);
  g_hash_table_destroy (priv->keyframes);

  if (priv->resource)
    {
      g_resources_unregister (priv->resource);
      g_resource_unref (priv->resource);
      priv->resource = NULL;
    }

  g_free (priv->path);

  G_OBJECT_CLASS (ctk_css_provider_parent_class)->finalize (object);
}

/**
 * ctk_css_provider_new:
 *
 * Returns a newly created #CtkCssProvider.
 *
 * Returns: A new #CtkCssProvider
 **/
CtkCssProvider *
ctk_css_provider_new (void)
{
  return g_object_new (CTK_TYPE_CSS_PROVIDER, NULL);
}

static void
ctk_css_provider_take_error (CtkCssProvider *provider,
                             CtkCssScanner  *scanner,
                             GError         *error)
{
  ctk_css_provider_emit_error (provider, scanner, error);
  g_error_free (error);
}

static void
ctk_css_provider_error_literal (CtkCssProvider *provider,
                                CtkCssScanner  *scanner,
                                GQuark          domain,
                                gint            code,
                                const char     *message)
{
  ctk_css_provider_take_error (provider,
                               scanner,
                               g_error_new_literal (domain, code, message));
}

static void
ctk_css_provider_error (CtkCssProvider *provider,
                        CtkCssScanner  *scanner,
                        GQuark          domain,
                        gint            code,
                        const char     *format,
                        ...)  G_GNUC_PRINTF (5, 6);
static void
ctk_css_provider_error (CtkCssProvider *provider,
                        CtkCssScanner  *scanner,
                        GQuark          domain,
                        gint            code,
                        const char     *format,
                        ...)
{
  GError *error;
  va_list args;

  ctk_internal_return_if_fail (CTK_IS_CSS_PROVIDER (provider));
  ctk_internal_return_if_fail (scanner != NULL);

  va_start (args, format);
  error = g_error_new_valist (domain, code, format, args);
  va_end (args);

  ctk_css_provider_take_error (provider, scanner, error);
}

static void
ctk_css_provider_invalid_token (CtkCssProvider *provider,
                                CtkCssScanner  *scanner,
                                const char     *expected)
{
  ctk_css_provider_error (provider,
                          scanner,
                          CTK_CSS_PROVIDER_ERROR,
                          CTK_CSS_PROVIDER_ERROR_SYNTAX,
                          "expected %s", expected);
}

static void
css_provider_commit (CtkCssProvider *css_provider,
                     GSList         *selectors,
                     CtkCssRuleset  *ruleset)
{
  CtkCssProviderPrivate *priv;
  GSList *l;

  priv = css_provider->priv;

  if (ruleset->styles == NULL && ruleset->widget_style == NULL)
    {
      g_slist_free_full (selectors, (GDestroyNotify) _ctk_css_selector_free);
      return;
    }

  for (l = selectors; l; l = l->next)
    {
      CtkCssRuleset new;

      ctk_css_ruleset_init_copy (&new, ruleset, l->data);

      g_array_append_val (priv->rulesets, new);
    }

  g_slist_free (selectors);
}

static void
ctk_css_provider_reset (CtkCssProvider *css_provider)
{
  CtkCssProviderPrivate *priv;
  guint i;

  priv = css_provider->priv;

  if (priv->resource)
    {
      g_resources_unregister (priv->resource);
      g_resource_unref (priv->resource);
      priv->resource = NULL;
    }

  if (priv->path)
    {
      g_free (priv->path);
      priv->path = NULL;
    }

  g_hash_table_remove_all (priv->symbolic_colors);
  g_hash_table_remove_all (priv->keyframes);

  for (i = 0; i < priv->rulesets->len; i++)
    ctk_css_ruleset_clear (&g_array_index (priv->rulesets, CtkCssRuleset, i));
  g_array_set_size (priv->rulesets, 0);
  _ctk_css_selector_tree_free (priv->tree);
  priv->tree = NULL;

}

static void
ctk_css_provider_propagate_error (CtkCssProvider  *provider G_GNUC_UNUSED,
                                  CtkCssSection   *section,
                                  const GError    *error,
                                  GError         **propagate_to)
{

  char *s;

  /* don't fail for deprecations */
  if (g_error_matches (error, CTK_CSS_PROVIDER_ERROR, CTK_CSS_PROVIDER_ERROR_DEPRECATED))
    {
      s = _ctk_css_section_to_string (section);
      g_warning ("Theme parsing error: %s: %s", s, error->message);
      g_free (s);
      return;
    }

  /* we already set an error. And we'd like to keep the first one */
  if (*propagate_to)
    return;

  *propagate_to = g_error_copy (error);
  if (section)
    {
      s = _ctk_css_section_to_string (section);
      g_prefix_error (propagate_to, "%s", s);
      g_free (s);
    }
}

static gboolean
parse_import (CtkCssScanner *scanner)
{
  GFile *file;

  ctk_css_scanner_push_section (scanner, CTK_CSS_SECTION_IMPORT);

  if (!_ctk_css_parser_try (scanner->parser, "@import", TRUE))
    {
      ctk_css_scanner_pop_section (scanner, CTK_CSS_SECTION_IMPORT);
      return FALSE;
    }

  if (_ctk_css_parser_is_string (scanner->parser))
    {
      char *uri;

      uri = _ctk_css_parser_read_string (scanner->parser);
      file = _ctk_css_parser_get_file_for_path (scanner->parser, uri);
      g_free (uri);
    }
  else
    {
      file = _ctk_css_parser_read_url (scanner->parser);
    }

  if (file == NULL)
    {
      _ctk_css_parser_resync (scanner->parser, TRUE, 0);
      ctk_css_scanner_pop_section (scanner, CTK_CSS_SECTION_IMPORT);
      return TRUE;
    }

  if (!_ctk_css_parser_try (scanner->parser, ";", FALSE))
    {
      ctk_css_provider_invalid_token (scanner->provider, scanner, "semicolon");
      _ctk_css_parser_resync (scanner->parser, TRUE, 0);
    }
  else if (ctk_css_scanner_would_recurse (scanner, file))
    {
       char *path = g_file_get_path (file);
       ctk_css_provider_error (scanner->provider,
                               scanner,
                               CTK_CSS_PROVIDER_ERROR,
                               CTK_CSS_PROVIDER_ERROR_IMPORT,
                               "Loading '%s' would recurse",
                               path);
       g_free (path);
    }
  else
    {
      ctk_css_provider_load_internal (scanner->provider,
                                      scanner,
                                      file,
                                      NULL,
                                      NULL);
    }

  g_object_unref (file);

  ctk_css_scanner_pop_section (scanner, CTK_CSS_SECTION_IMPORT);
  _ctk_css_parser_skip_whitespace (scanner->parser);

  return TRUE;
}

static gboolean
parse_color_definition (CtkCssScanner *scanner)
{
  CtkCssValue *color;
  char *name;

  ctk_css_scanner_push_section (scanner, CTK_CSS_SECTION_COLOR_DEFINITION);

  if (!_ctk_css_parser_try (scanner->parser, "@define-color", TRUE))
    {
      ctk_css_scanner_pop_section (scanner, CTK_CSS_SECTION_COLOR_DEFINITION);
      return FALSE;
    }

  name = _ctk_css_parser_try_name (scanner->parser, TRUE);
  if (name == NULL)
    {
      ctk_css_provider_error_literal (scanner->provider,
                                      scanner,
                                      CTK_CSS_PROVIDER_ERROR,
                                      CTK_CSS_PROVIDER_ERROR_SYNTAX,
                                      "Not a valid color name");
      _ctk_css_parser_resync (scanner->parser, TRUE, 0);
      ctk_css_scanner_pop_section (scanner, CTK_CSS_SECTION_COLOR_DEFINITION);
      return TRUE;
    }

  color = _ctk_css_color_value_parse (scanner->parser);
  if (color == NULL)
    {
      g_free (name);
      _ctk_css_parser_resync (scanner->parser, TRUE, 0);
      ctk_css_scanner_pop_section (scanner, CTK_CSS_SECTION_COLOR_DEFINITION);
      return TRUE;
    }

  if (!_ctk_css_parser_try (scanner->parser, ";", TRUE))
    {
      g_free (name);
      _ctk_css_value_unref (color);
      ctk_css_provider_error_literal (scanner->provider,
                                      scanner,
                                      CTK_CSS_PROVIDER_ERROR,
                                      CTK_CSS_PROVIDER_ERROR_SYNTAX,
                                      "Missing semicolon at end of color definition");
      _ctk_css_parser_resync (scanner->parser, TRUE, 0);

      ctk_css_scanner_pop_section (scanner, CTK_CSS_SECTION_COLOR_DEFINITION);
      return TRUE;
    }

  g_hash_table_insert (scanner->provider->priv->symbolic_colors, name, color);

  ctk_css_scanner_pop_section (scanner, CTK_CSS_SECTION_COLOR_DEFINITION);
  return TRUE;
}

static gboolean
parse_binding_set (CtkCssScanner *scanner)
{
  CtkBindingSet *binding_set;
  char *name;

  ctk_css_scanner_push_section (scanner, CTK_CSS_SECTION_BINDING_SET);

  if (!_ctk_css_parser_try (scanner->parser, "@binding-set", TRUE))
    {
      ctk_css_scanner_pop_section (scanner, CTK_CSS_SECTION_BINDING_SET);
      return FALSE;
    }

  name = _ctk_css_parser_try_ident (scanner->parser, TRUE);
  if (name == NULL)
    {
      ctk_css_provider_error_literal (scanner->provider,
                                      scanner,
                                      CTK_CSS_PROVIDER_ERROR,
                                      CTK_CSS_PROVIDER_ERROR_SYNTAX,
                                      "Expected name for binding set");
      _ctk_css_parser_resync (scanner->parser, TRUE, 0);
      goto skip_semicolon;
    }

  binding_set = ctk_binding_set_find (name);
  if (!binding_set)
    {
      binding_set = ctk_binding_set_new (name);
      binding_set->parsed = TRUE;
    }
  g_free (name);

  if (!_ctk_css_parser_try (scanner->parser, "{", TRUE))
    {
      ctk_css_provider_error_literal (scanner->provider,
                                      scanner,
                                      CTK_CSS_PROVIDER_ERROR,
                                      CTK_CSS_PROVIDER_ERROR_SYNTAX,
                                      "Expected '{' for binding set");
      _ctk_css_parser_resync (scanner->parser, TRUE, 0);
      goto skip_semicolon;
    }

  while (!_ctk_css_parser_is_eof (scanner->parser) &&
         !_ctk_css_parser_begins_with (scanner->parser, '}'))
    {
      name = _ctk_css_parser_read_value (scanner->parser);
      if (name == NULL)
        {
          _ctk_css_parser_resync (scanner->parser, TRUE, '}');
          continue;
        }

      if (ctk_binding_entry_add_signal_from_string (binding_set, name) != G_TOKEN_NONE)
        {
          ctk_css_provider_error_literal (scanner->provider,
                                          scanner,
                                          CTK_CSS_PROVIDER_ERROR,
                                          CTK_CSS_PROVIDER_ERROR_SYNTAX,
                                          "Failed to parse binding set.");
        }

      g_free (name);

      if (!_ctk_css_parser_try (scanner->parser, ";", TRUE))
        {
          if (!_ctk_css_parser_begins_with (scanner->parser, '}') &&
              !_ctk_css_parser_is_eof (scanner->parser))
            {
              ctk_css_provider_error_literal (scanner->provider,
                                              scanner,
                                              CTK_CSS_PROVIDER_ERROR,
                                              CTK_CSS_PROVIDER_ERROR_SYNTAX,
                                              "Expected semicolon");
              _ctk_css_parser_resync (scanner->parser, TRUE, '}');
            }
        }
    }

  if (!_ctk_css_parser_try (scanner->parser, "}", TRUE))
    {
      ctk_css_provider_error_literal (scanner->provider,
                                      scanner,
                                      CTK_CSS_PROVIDER_ERROR,
                                      CTK_CSS_PROVIDER_ERROR_SYNTAX,
                                      "expected '}' after declarations");
      if (!_ctk_css_parser_is_eof (scanner->parser))
        _ctk_css_parser_resync (scanner->parser, FALSE, 0);
    }

skip_semicolon:
  if (_ctk_css_parser_begins_with (scanner->parser, ';'))
    {
      ctk_css_provider_error_literal (scanner->provider,
                                      scanner,
                                      CTK_CSS_PROVIDER_ERROR,
                                      CTK_CSS_PROVIDER_ERROR_DEPRECATED,
                                      "Nonstandard semicolon at end of binding set");
      _ctk_css_parser_try (scanner->parser, ";", TRUE);
    }

  ctk_css_scanner_pop_section (scanner, CTK_CSS_SECTION_BINDING_SET);

  return TRUE;
}

static gboolean
parse_keyframes (CtkCssScanner *scanner)
{
  CtkCssKeyframes *keyframes;
  char *name;

  ctk_css_scanner_push_section (scanner, CTK_CSS_SECTION_KEYFRAMES);

  if (!_ctk_css_parser_try (scanner->parser, "@keyframes", TRUE))
    {
      ctk_css_scanner_pop_section (scanner, CTK_CSS_SECTION_KEYFRAMES);
      return FALSE;
    }

  name = _ctk_css_parser_try_ident (scanner->parser, TRUE);
  if (name == NULL)
    {
      ctk_css_provider_error_literal (scanner->provider,
                                      scanner,
                                      CTK_CSS_PROVIDER_ERROR,
                                      CTK_CSS_PROVIDER_ERROR_SYNTAX,
                                      "Expected name for keyframes");
      _ctk_css_parser_resync (scanner->parser, TRUE, 0);
      goto exit;
    }

  if (!_ctk_css_parser_try (scanner->parser, "{", TRUE))
    {
      ctk_css_provider_error_literal (scanner->provider,
                                      scanner,
                                      CTK_CSS_PROVIDER_ERROR,
                                      CTK_CSS_PROVIDER_ERROR_SYNTAX,
                                      "Expected '{' for keyframes");
      _ctk_css_parser_resync (scanner->parser, TRUE, 0);
      g_free (name);
      goto exit;
    }

  keyframes = _ctk_css_keyframes_parse (scanner->parser);
  if (keyframes == NULL)
    {
      _ctk_css_parser_resync (scanner->parser, TRUE, '}');
      g_free (name);
      goto exit;
    }

  g_hash_table_insert (scanner->provider->priv->keyframes, name, keyframes);

  if (!_ctk_css_parser_try (scanner->parser, "}", TRUE))
    {
      ctk_css_provider_error_literal (scanner->provider,
                                      scanner,
                                      CTK_CSS_PROVIDER_ERROR,
                                      CTK_CSS_PROVIDER_ERROR_SYNTAX,
                                      "expected '}' after declarations");
      if (!_ctk_css_parser_is_eof (scanner->parser))
        _ctk_css_parser_resync (scanner->parser, FALSE, 0);
    }

exit:
  ctk_css_scanner_pop_section (scanner, CTK_CSS_SECTION_KEYFRAMES);

  return TRUE;
}

static void
parse_at_keyword (CtkCssScanner *scanner)
{
  if (parse_import (scanner))
    return;
  if (parse_color_definition (scanner))
    return;
  if (parse_binding_set (scanner))
    return;
  if (parse_keyframes (scanner))
    return;

  else
    {
      ctk_css_provider_error_literal (scanner->provider,
                                      scanner,
                                      CTK_CSS_PROVIDER_ERROR,
                                      CTK_CSS_PROVIDER_ERROR_SYNTAX,
                                      "unknown @ rule");
      _ctk_css_parser_resync (scanner->parser, TRUE, 0);
    }
}

static GSList *
parse_selector_list (CtkCssScanner *scanner)
{
  GSList *selectors = NULL;

  ctk_css_scanner_push_section (scanner, CTK_CSS_SECTION_SELECTOR);

  do {
      CtkCssSelector *select = _ctk_css_selector_parse (scanner->parser);

      if (select == NULL)
        {
          g_slist_free_full (selectors, (GDestroyNotify) _ctk_css_selector_free);
          _ctk_css_parser_resync (scanner->parser, FALSE, 0);
          ctk_css_scanner_pop_section (scanner, CTK_CSS_SECTION_SELECTOR);
          return NULL;
        }

      selectors = g_slist_prepend (selectors, select);
    }
  while (_ctk_css_parser_try (scanner->parser, ",", TRUE));

  ctk_css_scanner_pop_section (scanner, CTK_CSS_SECTION_SELECTOR);

  return selectors;
}

static gboolean
name_is_style_property (const char *name)
{
  if (name[0] != '-')
    return FALSE;

  if (g_str_has_prefix (name, "-ctk-"))
    return FALSE;

  return TRUE;
}

static void
warn_if_deprecated (CtkCssScanner *scanner,
                    const gchar   *name)
{
  gchar *n = NULL;
  gchar *p;
  const gchar *type_name;
  const gchar *property_name;
  GType type;
  GTypeClass *class = NULL;
  GParamSpec *pspec;

  n = g_strdup (name);

  /* skip initial - */
  type_name = n + 1;

  p = strchr (type_name, '-');
  if (!p)
    goto out;

  p[0] = '\0';
  property_name = p + 1;

  type = g_type_from_name (type_name);
  if (type == G_TYPE_INVALID ||
      !g_type_is_a (type, CTK_TYPE_WIDGET))
    goto out;

  class = g_type_class_ref (type);
  pspec = ctk_widget_class_find_style_property (CTK_WIDGET_CLASS (class), property_name);
  if (!pspec)
    goto out;

  if (!(pspec->flags & G_PARAM_DEPRECATED))
    goto out;

  _ctk_css_parser_error_full (scanner->parser,
                              CTK_CSS_PROVIDER_ERROR_DEPRECATED,
                              "The style property %s:%s is deprecated and shouldn't be "
                              "used anymore. It will be removed in a future version",
                              g_type_name (pspec->owner_type), pspec->name);

out:
  g_free (n);
  if (class)
    g_type_class_unref (class);
}

static void
parse_declaration (CtkCssScanner *scanner,
                   CtkCssRuleset *ruleset)
{
  CtkStyleProperty *property;
  char *name;

  ctk_css_scanner_push_section (scanner, CTK_CSS_SECTION_DECLARATION);

  name = _ctk_css_parser_try_ident (scanner->parser, TRUE);
  if (name == NULL)
    goto check_for_semicolon;

  property = _ctk_style_property_lookup (name);
  if (property == NULL && !name_is_style_property (name))
    {
      ctk_css_provider_error (scanner->provider,
                              scanner,
                              CTK_CSS_PROVIDER_ERROR,
                              CTK_CSS_PROVIDER_ERROR_NAME,
                              "'%s' is not a valid property name",
                              name);
      _ctk_css_parser_resync (scanner->parser, TRUE, '}');
      g_free (name);
      ctk_css_scanner_pop_section (scanner, CTK_CSS_SECTION_DECLARATION);
      return;
    }

  if (property != NULL && strcmp (name, property->name) != 0)
    {
      ctk_css_provider_error (scanner->provider,
                              scanner,
                              CTK_CSS_PROVIDER_ERROR,
                              CTK_CSS_PROVIDER_ERROR_DEPRECATED,
                              "The '%s' property has been renamed to '%s'",
                              name, property->name);
    }
  else if (strcmp (name, "engine") == 0)
    {
      ctk_css_provider_error (scanner->provider,
                              scanner,
                              CTK_CSS_PROVIDER_ERROR,
                              CTK_CSS_PROVIDER_ERROR_DEPRECATED,
                              "The '%s' property is ignored",
                              name);
    }

  if (!_ctk_css_parser_try (scanner->parser, ":", TRUE))
    {
      ctk_css_provider_invalid_token (scanner->provider, scanner, "':'");
      _ctk_css_parser_resync (scanner->parser, TRUE, '}');
      g_free (name);
      ctk_css_scanner_pop_section (scanner, CTK_CSS_SECTION_DECLARATION);
      return;
    }

  if (property)
    {
      CtkCssValue *value;

      g_free (name);

      ctk_css_scanner_push_section (scanner, CTK_CSS_SECTION_VALUE);

      value = _ctk_style_property_parse_value (property,
                                               scanner->parser);

      if (value == NULL)
        {
          _ctk_css_parser_resync (scanner->parser, TRUE, '}');
          ctk_css_scanner_pop_section (scanner, CTK_CSS_SECTION_VALUE);
          ctk_css_scanner_pop_section (scanner, CTK_CSS_SECTION_DECLARATION);
          return;
        }

      if (!_ctk_css_parser_begins_with (scanner->parser, ';') &&
          !_ctk_css_parser_begins_with (scanner->parser, '}') &&
          !_ctk_css_parser_is_eof (scanner->parser))
        {
          ctk_css_provider_error (scanner->provider,
                                  scanner,
                                  CTK_CSS_PROVIDER_ERROR,
                                  CTK_CSS_PROVIDER_ERROR_SYNTAX,
                                  "Junk at end of value for %s", property->name);
          _ctk_css_parser_resync (scanner->parser, TRUE, '}');
          ctk_css_scanner_pop_section (scanner, CTK_CSS_SECTION_VALUE);
          ctk_css_scanner_pop_section (scanner, CTK_CSS_SECTION_DECLARATION);
          return;
        }

      if (CTK_IS_CSS_SHORTHAND_PROPERTY (property))
        {
          CtkCssShorthandProperty *shorthand = CTK_CSS_SHORTHAND_PROPERTY (property);
          guint i;

          for (i = 0; i < _ctk_css_shorthand_property_get_n_subproperties (shorthand); i++)
            {
              CtkCssStyleProperty *child = _ctk_css_shorthand_property_get_subproperty (shorthand, i);
              CtkCssValue *sub = _ctk_css_array_value_get_nth (value, i);
              
              ctk_css_ruleset_add (ruleset, child, _ctk_css_value_ref (sub), scanner->section);
            }
          
            _ctk_css_value_unref (value);
        }
      else if (CTK_IS_CSS_STYLE_PROPERTY (property))
        {
          ctk_css_ruleset_add (ruleset, CTK_CSS_STYLE_PROPERTY (property), value, scanner->section);
        }
      else
        {
          g_assert_not_reached ();
          _ctk_css_value_unref (value);
        }


      ctk_css_scanner_pop_section (scanner, CTK_CSS_SECTION_VALUE);
    }
  else if (name_is_style_property (name))
    {
      char *value_str;

      warn_if_deprecated (scanner, name);

      ctk_css_scanner_push_section (scanner, CTK_CSS_SECTION_VALUE);

      value_str = _ctk_css_parser_read_value (scanner->parser);
      if (value_str)
        {
          WidgetPropertyValue *val;

          val = widget_property_value_new (name, scanner->section);
	  val->value = value_str;

          ctk_css_ruleset_add_style (ruleset, name, val);
        }
      else
        {
          _ctk_css_parser_resync (scanner->parser, TRUE, '}');
          ctk_css_scanner_pop_section (scanner, CTK_CSS_SECTION_VALUE);
          ctk_css_scanner_pop_section (scanner, CTK_CSS_SECTION_DECLARATION);
          return;
        }

      ctk_css_scanner_pop_section (scanner, CTK_CSS_SECTION_VALUE);
    }
  else
    g_free (name);

check_for_semicolon:
  ctk_css_scanner_pop_section (scanner, CTK_CSS_SECTION_DECLARATION);

  if (!_ctk_css_parser_try (scanner->parser, ";", TRUE))
    {
      if (!_ctk_css_parser_begins_with (scanner->parser, '}') &&
          !_ctk_css_parser_is_eof (scanner->parser))
        {
          ctk_css_provider_error_literal (scanner->provider,
                                          scanner,
                                          CTK_CSS_PROVIDER_ERROR,
                                          CTK_CSS_PROVIDER_ERROR_SYNTAX,
                                          "Expected semicolon");
          _ctk_css_parser_resync (scanner->parser, TRUE, '}');
        }
    }
}

static void
parse_declarations (CtkCssScanner *scanner,
                    CtkCssRuleset *ruleset)
{
  while (!_ctk_css_parser_is_eof (scanner->parser) &&
         !_ctk_css_parser_begins_with (scanner->parser, '}'))
    {
      parse_declaration (scanner, ruleset);
    }
}

static void
parse_ruleset (CtkCssScanner *scanner)
{
  GSList *selectors;
  CtkCssRuleset ruleset = { 0, };

  ctk_css_scanner_push_section (scanner, CTK_CSS_SECTION_RULESET);

  selectors = parse_selector_list (scanner);
  if (selectors == NULL)
    {
      ctk_css_scanner_pop_section (scanner, CTK_CSS_SECTION_RULESET);
      return;
    }

  if (!_ctk_css_parser_try (scanner->parser, "{", TRUE))
    {
      ctk_css_provider_error_literal (scanner->provider,
                                      scanner,
                                      CTK_CSS_PROVIDER_ERROR,
                                      CTK_CSS_PROVIDER_ERROR_SYNTAX,
                                      "expected '{' after selectors");
      _ctk_css_parser_resync (scanner->parser, FALSE, 0);
      g_slist_free_full (selectors, (GDestroyNotify) _ctk_css_selector_free);
      ctk_css_scanner_pop_section (scanner, CTK_CSS_SECTION_RULESET);
      return;
    }

  parse_declarations (scanner, &ruleset);

  if (!_ctk_css_parser_try (scanner->parser, "}", TRUE))
    {
      ctk_css_provider_error_literal (scanner->provider,
                                      scanner,
                                      CTK_CSS_PROVIDER_ERROR,
                                      CTK_CSS_PROVIDER_ERROR_SYNTAX,
                                      "expected '}' after declarations");
      if (!_ctk_css_parser_is_eof (scanner->parser))
        {
          _ctk_css_parser_resync (scanner->parser, FALSE, 0);
          g_slist_free_full (selectors, (GDestroyNotify) _ctk_css_selector_free);
          ctk_css_ruleset_clear (&ruleset);
          ctk_css_scanner_pop_section (scanner, CTK_CSS_SECTION_RULESET);
        }
    }

  css_provider_commit (scanner->provider, selectors, &ruleset);
  ctk_css_ruleset_clear (&ruleset);
  ctk_css_scanner_pop_section (scanner, CTK_CSS_SECTION_RULESET);
}

static void
parse_statement (CtkCssScanner *scanner)
{
  if (_ctk_css_parser_begins_with (scanner->parser, '@'))
    parse_at_keyword (scanner);
  else
    parse_ruleset (scanner);
}

static void
parse_stylesheet (CtkCssScanner *scanner)
{
  ctk_css_scanner_push_section (scanner, CTK_CSS_SECTION_DOCUMENT);

  _ctk_css_parser_skip_whitespace (scanner->parser);

  while (!_ctk_css_parser_is_eof (scanner->parser))
    {
      if (_ctk_css_parser_try (scanner->parser, "<!--", TRUE) ||
          _ctk_css_parser_try (scanner->parser, "-->", TRUE))
        continue;

      parse_statement (scanner);
    }

  ctk_css_scanner_pop_section (scanner, CTK_CSS_SECTION_DOCUMENT);
}

static int
ctk_css_provider_compare_rule (gconstpointer a_,
                               gconstpointer b_)
{
  const CtkCssRuleset *a = (const CtkCssRuleset *) a_;
  const CtkCssRuleset *b = (const CtkCssRuleset *) b_;
  int compare;

  compare = _ctk_css_selector_compare (a->selector, b->selector);
  if (compare != 0)
    return compare;

  return 0;
}

static void
ctk_css_provider_postprocess (CtkCssProvider *css_provider)
{
  CtkCssProviderPrivate *priv = css_provider->priv;
  CtkCssSelectorTreeBuilder *builder;
  guint i;

  g_array_sort (priv->rulesets, ctk_css_provider_compare_rule);

  builder = _ctk_css_selector_tree_builder_new ();
  for (i = 0; i < priv->rulesets->len; i++)
    {
      CtkCssRuleset *ruleset;

      ruleset = &g_array_index (priv->rulesets, CtkCssRuleset, i);

      _ctk_css_selector_tree_builder_add (builder,
					  ruleset->selector,
					  &ruleset->selector_match,
					  ruleset);
    }

  priv->tree = _ctk_css_selector_tree_builder_build (builder);
  _ctk_css_selector_tree_builder_free (builder);

#ifndef VERIFY_TREE
  for (i = 0; i < priv->rulesets->len; i++)
    {
      CtkCssRuleset *ruleset;

      ruleset = &g_array_index (priv->rulesets, CtkCssRuleset, i);

      _ctk_css_selector_free (ruleset->selector);
      ruleset->selector = NULL;
    }
#endif
}

static gboolean
ctk_css_provider_load_internal (CtkCssProvider *css_provider,
                                CtkCssScanner  *parent,
                                GFile          *file,
                                const char     *text,
                                GError        **error)
{
  GBytes *free_bytes = NULL;
  CtkCssScanner *scanner;
  gulong error_handler;

  if (error)
    error_handler = g_signal_connect (css_provider,
                                      "parsing-error",
                                      G_CALLBACK (ctk_css_provider_propagate_error),
                                      error);
  else
    error_handler = 0; /* silence gcc */

  if (text == NULL)
    {
      GError *load_error = NULL;

      free_bytes = ctk_file_load_bytes (file, NULL, &load_error);

      if (free_bytes != NULL)
        {
          text = g_bytes_get_data (free_bytes, NULL);
        }
      else
        {
          if (parent == NULL)
            {
              scanner = ctk_css_scanner_new (css_provider,
                                             NULL,
                                             NULL,
                                             file,
                                             "");

              ctk_css_scanner_push_section (scanner, CTK_CSS_SECTION_DOCUMENT);
            }
          else
            scanner = parent;

          ctk_css_provider_error (css_provider,
                                  scanner,
                                  CTK_CSS_PROVIDER_ERROR,
                                  CTK_CSS_PROVIDER_ERROR_IMPORT,
                                  "Failed to import: %s",
                                  load_error->message);

          if (parent == NULL)
            {
              ctk_css_scanner_pop_section (scanner, CTK_CSS_SECTION_DOCUMENT);

              ctk_css_scanner_destroy (scanner);
            }
        }
    }

  if (text)
    {
      scanner = ctk_css_scanner_new (css_provider,
                                     parent,
                                     parent ? parent->section : NULL,
                                     file,
                                     text);

      parse_stylesheet (scanner);

      ctk_css_scanner_destroy (scanner);

      if (parent == NULL)
        ctk_css_provider_postprocess (css_provider);
    }

  if (free_bytes)
    g_bytes_unref (free_bytes);

  if (error)
    {
      g_signal_handler_disconnect (css_provider, error_handler);

      if (*error)
        {
          /* We clear all contents from the provider for backwards compat reasons */
          ctk_css_provider_reset (css_provider);
          return FALSE;
        }
    }

  return TRUE;
}

/**
 * ctk_css_provider_load_from_data:
 * @css_provider: a #CtkCssProvider
 * @data: (array length=length) (element-type guint8): CSS data loaded in memory
 * @length: the length of @data in bytes, or -1 for NUL terminated strings. If
 *   @length is not -1, the code will assume it is not NUL terminated and will
 *   potentially do a copy.
 * @error: (out) (allow-none): return location for a #GError, or %NULL
 *
 * Loads @data into @css_provider, and by doing so clears any previously loaded
 * information.
 *
 * Returns: %TRUE. The return value is deprecated and %FALSE will only be
 *     returned for backwards compatibility reasons if an @error is not 
 *     %NULL and a loading error occurred. To track errors while loading
 *     CSS, connect to the #CtkCssProvider::parsing-error signal.
 **/
gboolean
ctk_css_provider_load_from_data (CtkCssProvider  *css_provider,
                                 const gchar     *data,
                                 gssize           length,
                                 GError         **error)
{
  char *free_data;
  gboolean ret;

  g_return_val_if_fail (CTK_IS_CSS_PROVIDER (css_provider), FALSE);
  g_return_val_if_fail (data != NULL, FALSE);

  if (length < 0)
    {
      length = strlen (data);
      free_data = NULL;
    }
  else
    {
      free_data = g_strndup (data, length);
      data = free_data;
    }

  ctk_css_provider_reset (css_provider);

  ret = ctk_css_provider_load_internal (css_provider, NULL, NULL, data, error);

  g_free (free_data);

  _ctk_style_provider_private_changed (CTK_STYLE_PROVIDER_PRIVATE (css_provider));

  return ret;
}

/**
 * ctk_css_provider_load_from_file:
 * @css_provider: a #CtkCssProvider
 * @file: #GFile pointing to a file to load
 * @error: (out) (allow-none): return location for a #GError, or %NULL
 *
 * Loads the data contained in @file into @css_provider, making it
 * clear any previously loaded information.
 *
 * Returns: %TRUE. The return value is deprecated and %FALSE will only be
 *     returned for backwards compatibility reasons if an @error is not 
 *     %NULL and a loading error occurred. To track errors while loading
 *     CSS, connect to the #CtkCssProvider::parsing-error signal.
 **/
gboolean
ctk_css_provider_load_from_file (CtkCssProvider  *css_provider,
                                 GFile           *file,
                                 GError         **error)
{
  gboolean success;

  g_return_val_if_fail (CTK_IS_CSS_PROVIDER (css_provider), FALSE);
  g_return_val_if_fail (G_IS_FILE (file), FALSE);

  ctk_css_provider_reset (css_provider);

  success = ctk_css_provider_load_internal (css_provider, NULL, file, NULL, error);

  _ctk_style_provider_private_changed (CTK_STYLE_PROVIDER_PRIVATE (css_provider));

  return success;
}

/**
 * ctk_css_provider_load_from_path:
 * @css_provider: a #CtkCssProvider
 * @path: the path of a filename to load, in the GLib filename encoding
 * @error: (out) (allow-none): return location for a #GError, or %NULL
 *
 * Loads the data contained in @path into @css_provider, making it clear
 * any previously loaded information.
 *
 * Returns: %TRUE. The return value is deprecated and %FALSE will only be
 *     returned for backwards compatibility reasons if an @error is not 
 *     %NULL and a loading error occurred. To track errors while loading
 *     CSS, connect to the #CtkCssProvider::parsing-error signal.
 **/
gboolean
ctk_css_provider_load_from_path (CtkCssProvider  *css_provider,
                                 const gchar     *path,
                                 GError         **error)
{
  GFile *file;
  gboolean result;

  g_return_val_if_fail (CTK_IS_CSS_PROVIDER (css_provider), FALSE);
  g_return_val_if_fail (path != NULL, FALSE);

  file = g_file_new_for_path (path);
  
  result = ctk_css_provider_load_from_file (css_provider, file, error);

  g_object_unref (file);

  return result;
}

/**
 * ctk_css_provider_load_from_resource:
 * @css_provider: a #CtkCssProvider
 * @resource_path: a #GResource resource path
 *
 * Loads the data contained in the resource at @resource_path into
 * the #CtkCssProvider, clearing any previously loaded information.
 *
 * To track errors while loading CSS, connect to the
 * #CtkCssProvider::parsing-error signal.
 *
 * Since: 3.16
 */
void
ctk_css_provider_load_from_resource (CtkCssProvider *css_provider,
			             const gchar    *resource_path)
{
  GFile *file;
  gchar *uri, *escaped;

  g_return_if_fail (CTK_IS_CSS_PROVIDER (css_provider));
  g_return_if_fail (resource_path != NULL);

  escaped = g_uri_escape_string (resource_path,
				 G_URI_RESERVED_CHARS_ALLOWED_IN_PATH, FALSE);
  uri = g_strconcat ("resource://", escaped, NULL);
  g_free (escaped);

  file = g_file_new_for_uri (uri);
  g_free (uri);

  ctk_css_provider_load_from_file (css_provider, file, NULL);

  g_object_unref (file);
}

/**
 * ctk_css_provider_get_default:
 *
 * Returns the provider containing the style settings used as a
 * fallback for all widgets.
 *
 * Returns: (transfer none): The provider used for fallback styling.
 *          This memory is owned by CTK+, and you must not free it.
 *
 * Deprecated: 3.24: Use ctk_css_provider_new() instead.
 **/
CtkCssProvider *
ctk_css_provider_get_default (void)
{
  static CtkCssProvider *provider;

  if (G_UNLIKELY (!provider))
    {
      provider = ctk_css_provider_new ();
    }

  return provider;
}

gchar *
_ctk_get_theme_dir (void)
{
  const gchar *var;

  var = g_getenv ("CTK_DATA_PREFIX");
  if (var == NULL)
    var = _ctk_get_data_prefix ();
  return g_build_filename (var, "share", "themes", NULL);
}

/* Return the path that this providers ctk.css was loaded from,
 * if it is part of a theme, otherwise NULL.
 */
const gchar *
_ctk_css_provider_get_theme_dir (CtkCssProvider *provider)
{
  return provider->priv->path;
}

#if (CTK_MINOR_VERSION % 2)
#define MINOR (CTK_MINOR_VERSION + 1)
#else
#define MINOR CTK_MINOR_VERSION
#endif

/*
 * Look for
 * $dir/$subdir/ctk-3.16/ctk-$variant.css
 * $dir/$subdir/ctk-3.14/ctk-$variant.css
 *  ...
 * $dir/$subdir/ctk-3.0/ctk-$variant.css
 * and return the first found file.
 * We don't check versions before 3.14,
 * since those CTK+ versions didn't have
 * the versioned loading mechanism.
 */
static gchar *
_ctk_css_find_theme_dir (const gchar *dir,
                         const gchar *subdir,
                         const gchar *name,
                         const gchar *variant)
{
  gchar *file;
  gchar *base;
  gchar *subsubdir;
  gint i;
  gchar *path;

  if (variant)
    file = g_strconcat ("ctk-", variant, ".css", NULL);
  else
    file = g_strdup ("ctk.css");

  if (subdir)
    base = g_build_filename (dir, subdir, name, NULL);
  else
    base = g_build_filename (dir, name, NULL);

  for (i = MINOR; i >= 0; i = i - 2)
    {
      if (i < 14)
        i = 0;

      subsubdir = g_strdup_printf ("ctk-3.%d", i);
      path = g_build_filename (base, subsubdir, file, NULL);
      g_free (subsubdir);

      if (g_file_test (path, G_FILE_TEST_EXISTS))
        break;

      g_free (path);
      path = NULL;
    }

  g_free (file);
  g_free (base);

  return path;
}

#undef MINOR

static gchar *
_ctk_css_find_theme (const gchar *name,
                     const gchar *variant)
{
  gchar *path;
  const char *const *dirs;
  int i;
  char *dir;

  /* First look in the user's data directory */
  path = _ctk_css_find_theme_dir (g_get_user_data_dir (), "themes", name, variant);
  if (path)
    return path;

  /* Next look in the user's home directory */
  path = _ctk_css_find_theme_dir (g_get_home_dir (), ".themes", name, variant);
  if (path)
    return path;

  /* Look in system data directories */
  dirs = g_get_system_data_dirs ();
  for (i = 0; dirs[i]; i++)
    {
      path = _ctk_css_find_theme_dir (dirs[i], "themes", name, variant);
      if (path)
        return path;
    }

  /* Finally, try in the default theme directory */
  dir = _ctk_get_theme_dir ();
  path = _ctk_css_find_theme_dir (dir, NULL, name, variant);
  g_free (dir);

  return path;
}

/**
 * _ctk_css_provider_load_named:
 * @provider: a #CtkCssProvider
 * @name: A theme name
 * @variant: (allow-none): variant to load, for example, "dark", or
 *     %NULL for the default
 *
 * Loads a theme from the usual theme paths. The actual process of
 * finding the theme might change between releases, but it is
 * guaranteed that this function uses the same mechanism to load the
 * theme than CTK uses for loading its own theme.
 **/
void
_ctk_css_provider_load_named (CtkCssProvider *provider,
                              const gchar    *name,
                              const gchar    *variant)
{
  gchar *path;
  gchar *resource_path;

  g_return_if_fail (CTK_IS_CSS_PROVIDER (provider));
  g_return_if_fail (name != NULL);

  ctk_css_provider_reset (provider);

  /* try loading the resource for the theme. This is mostly meant for built-in
   * themes.
   */
  if (variant)
    resource_path = g_strdup_printf ("/org/ctk/libctk/theme/%s/ctk-%s.css", name, variant);
  else
    resource_path = g_strdup_printf ("/org/ctk/libctk/theme/%s/ctk.css", name);

  if (g_resources_get_info (resource_path, 0, NULL, NULL, NULL))
    {
      ctk_css_provider_load_from_resource (provider, resource_path);
      g_free (resource_path);
      return;
    }
  g_free (resource_path);

  /* Next try looking for files in the various theme directories. */
  path = _ctk_css_find_theme (name, variant);
  if (path)
    {
      char *dir, *resource_file;
      GResource *resource;

      dir = g_path_get_dirname (path);
      resource_file = g_build_filename (dir, "ctk.gresource", NULL);
      resource = g_resource_load (resource_file, NULL);
      g_free (resource_file);

      if (resource != NULL)
        g_resources_register (resource);

      ctk_css_provider_load_from_path (provider, path, NULL);

      /* Only set this after load, as load_from_path will clear it */
      provider->priv->resource = resource;
      provider->priv->path = dir;

      g_free (path);
    }
  else
    {
      /* Things failed! Fall back! Fall back! */

      if (variant)
        {
          /* If there was a variant, try without */
          _ctk_css_provider_load_named (provider, name, NULL);
        }
      else
        {
          /* Worst case, fall back to the default */
          g_return_if_fail (!g_str_equal (name, DEFAULT_THEME_NAME)); /* infloop protection */
          _ctk_css_provider_load_named (provider, DEFAULT_THEME_NAME, NULL);
        }
    }
}

/**
 * ctk_css_provider_get_named:
 * @name: A theme name
 * @variant: (allow-none): variant to load, for example, "dark", or
 *     %NULL for the default
 *
 * Loads a theme from the usual theme paths
 *
 * Returns: (transfer none): a #CtkCssProvider with the theme loaded.
 *     This memory is owned by CTK+, and you must not free it.
 */
CtkCssProvider *
ctk_css_provider_get_named (const gchar *name,
                            const gchar *variant)
{
  static GHashTable *themes = NULL;
  CtkCssProvider *provider;
  gchar *key;

  if (variant == NULL)
    key = g_strdup (name);
  else
    key = g_strconcat (name, "-", variant, NULL);
  if (G_UNLIKELY (!themes))
    themes = g_hash_table_new (g_str_hash, g_str_equal);

  provider = g_hash_table_lookup (themes, key);
  
  if (!provider)
    {
      provider = ctk_css_provider_new ();
      _ctk_css_provider_load_named (provider, name, variant);
      g_hash_table_insert (themes, g_strdup (key), provider);
    }
  
  g_free (key);

  return provider;
}

static int
compare_properties (gconstpointer a, gconstpointer b, gpointer style)
{
  const guint *ua = a;
  const guint *ub = b;
  PropertyValue *styles = style;

  return strcmp (_ctk_style_property_get_name (CTK_STYLE_PROPERTY (styles[*ua].property)),
                 _ctk_style_property_get_name (CTK_STYLE_PROPERTY (styles[*ub].property)));
}

static int
compare_names (gconstpointer a, gconstpointer b)
{
  const WidgetPropertyValue *aa = a;
  const WidgetPropertyValue *bb = b;
  return strcmp (aa->name, bb->name);
}

static void
ctk_css_ruleset_print (const CtkCssRuleset *ruleset,
                       GString             *str)
{
  GList *values, *walk;
  WidgetPropertyValue *widget_value;
  guint i;

  _ctk_css_selector_tree_match_print (ruleset->selector_match, str);

  g_string_append (str, " {\n");

  if (ruleset->styles)
    {
      guint *sorted = g_new (guint, ruleset->n_styles);

      for (i = 0; i < ruleset->n_styles; i++)
        sorted[i] = i;

      /* so the output is identical for identical selector styles */
      g_qsort_with_data (sorted, ruleset->n_styles, sizeof (guint), compare_properties, ruleset->styles);

      for (i = 0; i < ruleset->n_styles; i++)
        {
          PropertyValue *prop = &ruleset->styles[sorted[i]];
          g_string_append (str, "  ");
          g_string_append (str, _ctk_style_property_get_name (CTK_STYLE_PROPERTY (prop->property)));
          g_string_append (str, ": ");
          _ctk_css_value_print (prop->value, str);
          g_string_append (str, ";\n");
        }

      g_free (sorted);
    }

  if (ruleset->widget_style)
    {
      values = NULL;
      for (widget_value = ruleset->widget_style; widget_value != NULL; widget_value = widget_value->next)
	values = g_list_prepend (values, widget_value);

      /* so the output is identical for identical selector styles */
      values = g_list_sort (values, compare_names);

      for (walk = values; walk; walk = walk->next)
        {
	  widget_value = walk->data;

          g_string_append (str, "  ");
          g_string_append (str, widget_value->name);
          g_string_append (str, ": ");
          g_string_append (str, widget_value->value);
          g_string_append (str, ";\n");
        }

      g_list_free (values);
    }

  g_string_append (str, "}\n");
}

static void
ctk_css_provider_print_colors (GHashTable *colors,
                               GString    *str)
{
  GList *keys, *walk;

  keys = g_hash_table_get_keys (colors);
  /* so the output is identical for identical styles */
  keys = g_list_sort (keys, (GCompareFunc) strcmp);

  for (walk = keys; walk; walk = walk->next)
    {
      const char *name = walk->data;
      CtkCssValue *color = g_hash_table_lookup (colors, (gpointer) name);

      g_string_append (str, "@define-color ");
      g_string_append (str, name);
      g_string_append (str, " ");
      _ctk_css_value_print (color, str);
      g_string_append (str, ";\n");
    }

  g_list_free (keys);
}

static void
ctk_css_provider_print_keyframes (GHashTable *keyframes,
                                  GString    *str)
{
  GList *keys, *walk;

  keys = g_hash_table_get_keys (keyframes);
  /* so the output is identical for identical styles */
  keys = g_list_sort (keys, (GCompareFunc) strcmp);

  for (walk = keys; walk; walk = walk->next)
    {
      const char *name = walk->data;
      CtkCssKeyframes *keyframe = g_hash_table_lookup (keyframes, (gpointer) name);

      if (str->len > 0)
        g_string_append (str, "\n");
      g_string_append (str, "@keyframes ");
      g_string_append (str, name);
      g_string_append (str, " {\n");
      _ctk_css_keyframes_print (keyframe, str);
      g_string_append (str, "}\n");
    }

  g_list_free (keys);
}

/**
 * ctk_css_provider_to_string:
 * @provider: the provider to write to a string
 *
 * Converts the @provider into a string representation in CSS
 * format.
 *
 * Using ctk_css_provider_load_from_data() with the return value
 * from this function on a new provider created with
 * ctk_css_provider_new() will basically create a duplicate of
 * this @provider.
 *
 * Returns: a new string representing the @provider.
 *
 * Since: 3.2
 **/
char *
ctk_css_provider_to_string (CtkCssProvider *provider)
{
  CtkCssProviderPrivate *priv;
  GString *str;
  guint i;

  g_return_val_if_fail (CTK_IS_CSS_PROVIDER (provider), NULL);

  priv = provider->priv;

  str = g_string_new ("");

  ctk_css_provider_print_colors (priv->symbolic_colors, str);
  ctk_css_provider_print_keyframes (priv->keyframes, str);

  for (i = 0; i < priv->rulesets->len; i++)
    {
      if (str->len != 0)
        g_string_append (str, "\n");
      ctk_css_ruleset_print (&g_array_index (priv->rulesets, CtkCssRuleset, i), str);
    }

  return g_string_free (str, FALSE);
}

