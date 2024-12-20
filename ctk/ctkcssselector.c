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

#include "config.h"

#include "ctkcssselectorprivate.h"

#include <stdlib.h>
#include <string.h>

#include "ctkcssprovider.h"
#include "ctkstylecontextprivate.h"

#if defined(_MSC_VER) && _MSC_VER >= 1500
# include <intrin.h>
#endif

typedef struct _CtkCssSelectorClass CtkCssSelectorClass;
typedef gboolean (* CtkCssSelectorForeachFunc) (const CtkCssSelector *selector,
                                                const CtkCssMatcher  *matcher,
                                                gpointer              data);

struct _CtkCssSelectorClass {
  const char        *name;

  void              (* print)       (const CtkCssSelector       *selector,
                                     GString                    *string);
  /* NULL or an iterator that calls func with each submatcher of @matcher.
   * Potentially no submatcher exists.
   * If any @invocation of @func returns %TRUE, the function will immediately
   * return %TRUE itself. If @func never returns %TRUE (or isn't called at all),
   * %FALSE will be returned.
   */
  gboolean          (* foreach_matcher)  (const CtkCssSelector       *selector,
                                          const CtkCssMatcher        *matcher,
                                          CtkCssSelectorForeachFunc   func,
                                          gpointer                    data);
  gboolean          (* match_one)   (const CtkCssSelector       *selector,
                                     const CtkCssMatcher        *matcher);
  CtkCssChange      (* get_change)  (const CtkCssSelector       *selector,
				     CtkCssChange                previous_change);
  void              (* add_specificity)  (const CtkCssSelector  *selector,
                                          guint                 *ids,
                                          guint                 *classes,
                                          guint                 *elements);
  guint             (* hash_one)    (const CtkCssSelector       *selector);
  int               (* compare_one) (const CtkCssSelector       *a,
				     const CtkCssSelector       *b);

  guint         is_simple :1;
};

typedef enum {
  POSITION_FORWARD,
  POSITION_BACKWARD,
  POSITION_ONLY,
  POSITION_SORTED
} PositionType;
#define POSITION_TYPE_BITS 4
#define POSITION_NUMBER_BITS ((sizeof (gpointer) * 8 - POSITION_TYPE_BITS) / 2)

union _CtkCssSelector
{
  const CtkCssSelectorClass     *class;         /* type of check this selector does */
  struct {
    const CtkCssSelectorClass   *class;
    const char                  *name;          /* interned */
  }                              id;
  struct {
    const CtkCssSelectorClass   *class;
    const char                  *name;          /* interned */
    CtkRegionFlags               flags;
  }                              region;
  struct {
    const CtkCssSelectorClass   *class;
    GQuark                       style_class;
  }                              style_class;
  struct {
    const CtkCssSelectorClass   *class;
    const char                  *name;          /* interned */
  }                              name;
  struct {
    const CtkCssSelectorClass   *class;
    CtkStateFlags                state;
  }                              state;
  struct {
    const CtkCssSelectorClass   *class;
    PositionType                 type :POSITION_TYPE_BITS;
    gssize                       a :POSITION_NUMBER_BITS;
    gssize                       b :POSITION_NUMBER_BITS;
  }                              position;
};

#define CTK_CSS_SELECTOR_TREE_EMPTY_OFFSET G_MAXINT32
struct _CtkCssSelectorTree
{
  CtkCssSelector selector;
  gint32 parent_offset;
  gint32 previous_offset;
  gint32 sibling_offset;
  gint32 matches_offset; /* pointers that we return as matches if selector matches */
};

static gboolean
ctk_css_selector_equal (const CtkCssSelector *a,
			const CtkCssSelector *b)
{
  return
    a->class == b->class &&
    a->class->compare_one (a, b) == 0;
}

static guint
ctk_css_selector_hash_one (const CtkCssSelector *selector)
{
  return GPOINTER_TO_UINT (selector->class) ^ selector->class->hash_one (selector);
}

static gpointer *
ctk_css_selector_tree_get_matches (const CtkCssSelectorTree *tree)
{
  if (tree->matches_offset == CTK_CSS_SELECTOR_TREE_EMPTY_OFFSET)
    return NULL;

  return (gpointer *) ((guint8 *)tree + tree->matches_offset);
}

static void
g_ptr_array_insert_sorted (GPtrArray *array,
                           gpointer   data)
{
  gint i;

  for (i = 0; i < array->len; i++)
    {
      if (data == array->pdata[i])
        return;

      if (data < array->pdata[i])
        break;
    }

  g_ptr_array_insert (array, i, data);
}

static void
ctk_css_selector_tree_found_match (const CtkCssSelectorTree  *tree,
				   GPtrArray                **array)
{
  int i;
  gpointer *matches;

  matches = ctk_css_selector_tree_get_matches (tree);
  if (matches)
    {
      if (!*array)
        *array = g_ptr_array_sized_new (16);

      for (i = 0; matches[i] != NULL; i++)
        g_ptr_array_insert_sorted (*array, matches[i]);
    }
}

static gboolean
ctk_css_selector_match (const CtkCssSelector *selector,
                        const CtkCssMatcher  *matcher)
{
  return selector->class->match_one (selector, matcher);
}

static gboolean
ctk_css_selector_foreach (const CtkCssSelector      *selector,
                          const CtkCssMatcher       *matcher,
                          CtkCssSelectorForeachFunc  func,
                          gpointer                   data)
{
  return selector->class->foreach_matcher (selector, matcher, func, data);
}

static int
ctk_css_selector_compare_one (const CtkCssSelector *a, const CtkCssSelector *b)
{
  if (a->class != b->class)
    return strcmp (a->class->name, b->class->name);
  else
    return a->class->compare_one (a, b);
}
  
static const CtkCssSelector *
ctk_css_selector_previous (const CtkCssSelector *selector)
{
  selector = selector + 1;

  return selector->class ? selector : NULL;
}

static const CtkCssSelectorTree *
ctk_css_selector_tree_at_offset (const CtkCssSelectorTree *tree,
				 gint32 offset)
{
  if (offset == CTK_CSS_SELECTOR_TREE_EMPTY_OFFSET)
    return NULL;

  return (CtkCssSelectorTree *) ((guint8 *)tree + offset);
}

static const CtkCssSelectorTree *
ctk_css_selector_tree_get_parent (const CtkCssSelectorTree *tree)
{
  return ctk_css_selector_tree_at_offset (tree, tree->parent_offset);
}

static const CtkCssSelectorTree *
ctk_css_selector_tree_get_previous (const CtkCssSelectorTree *tree)
{
  return ctk_css_selector_tree_at_offset (tree, tree->previous_offset);
}

static const CtkCssSelectorTree *
ctk_css_selector_tree_get_sibling (const CtkCssSelectorTree *tree)
{
  return ctk_css_selector_tree_at_offset (tree, tree->sibling_offset);
}

/* DEFAULTS */

static void
ctk_css_selector_default_add_specificity (const CtkCssSelector *selector G_GNUC_UNUSED,
                                          guint                *ids G_GNUC_UNUSED,
                                          guint                *classes G_GNUC_UNUSED,
                                          guint                *elements G_GNUC_UNUSED)
{
  /* no specificity changes */
}
 
static gboolean
ctk_css_selector_default_foreach_matcher (const CtkCssSelector       *selector,
                                          const CtkCssMatcher        *matcher,
                                          CtkCssSelectorForeachFunc   func,
                                          gpointer                    data)
{
  return func (selector, matcher, data);
}

static gboolean
ctk_css_selector_default_match_one (const CtkCssSelector *selector G_GNUC_UNUSED,
                                    const CtkCssMatcher  *matcher G_GNUC_UNUSED)
{
  return TRUE;
}

static guint
ctk_css_selector_default_hash_one (const CtkCssSelector *selector G_GNUC_UNUSED)
{
  return 0;
}

static int
ctk_css_selector_default_compare_one (const CtkCssSelector *a G_GNUC_UNUSED,
                                      const CtkCssSelector *b G_GNUC_UNUSED)
{
  return 0;
}

/* DESCENDANT */

static void
ctk_css_selector_descendant_print (const CtkCssSelector *selector G_GNUC_UNUSED,
                                   GString              *string)
{
  g_string_append_c (string, ' ');
}

static gboolean
ctk_css_selector_descendant_foreach_matcher (const CtkCssSelector      *selector,
                                             const CtkCssMatcher       *matcher,
                                             CtkCssSelectorForeachFunc  func,
                                             gpointer                   data)
{
  CtkCssMatcher ancestor;

  while (_ctk_css_matcher_get_parent (&ancestor, matcher))
    {
      matcher = &ancestor;

      if (func (selector, &ancestor, data))
        return TRUE;

      /* any matchers are dangerous here, as we may loop forever, but
	 we can terminate now as all possible matches have already been added */
      if (_ctk_css_matcher_matches_any (matcher))
	break;
    }

  return FALSE;
}

static CtkCssChange
ctk_css_selector_descendant_get_change (const CtkCssSelector *selector G_GNUC_UNUSED,
                                        CtkCssChange          previous_change)
{
  return _ctk_css_change_for_child (previous_change);
}

static const CtkCssSelectorClass CTK_CSS_SELECTOR_DESCENDANT = {
  "descendant",
  ctk_css_selector_descendant_print,
  ctk_css_selector_descendant_foreach_matcher,
  ctk_css_selector_default_match_one,
  ctk_css_selector_descendant_get_change,
  ctk_css_selector_default_add_specificity,
  ctk_css_selector_default_hash_one,
  ctk_css_selector_default_compare_one,
  FALSE
};

/* CHILD */

static void
ctk_css_selector_child_print (const CtkCssSelector *selector G_GNUC_UNUSED,
                              GString              *string)
{
  g_string_append (string, " > ");
}

static gboolean
ctk_css_selector_child_foreach_matcher (const CtkCssSelector      *selector,
                                        const CtkCssMatcher       *matcher,
                                        CtkCssSelectorForeachFunc  func,
                                        gpointer                   data)
{
  CtkCssMatcher parent;

  if (!_ctk_css_matcher_get_parent (&parent, matcher))
    return FALSE;

  return func (selector, &parent, data);
}

static CtkCssChange
ctk_css_selector_child_get_change (const CtkCssSelector *selector G_GNUC_UNUSED,
                                   CtkCssChange          previous_change)
{
  return _ctk_css_change_for_child (previous_change);
}

static const CtkCssSelectorClass CTK_CSS_SELECTOR_CHILD = {
  "child",
  ctk_css_selector_child_print,
  ctk_css_selector_child_foreach_matcher,
  ctk_css_selector_default_match_one,
  ctk_css_selector_child_get_change,
  ctk_css_selector_default_add_specificity,
  ctk_css_selector_default_hash_one,
  ctk_css_selector_default_compare_one,
  FALSE
};

/* SIBLING */

static void
ctk_css_selector_sibling_print (const CtkCssSelector *selector G_GNUC_UNUSED,
                                GString              *string)
{
  g_string_append (string, " ~ ");
}

static gboolean
ctk_css_selector_sibling_foreach_matcher (const CtkCssSelector      *selector,
                                          const CtkCssMatcher       *matcher,
                                          CtkCssSelectorForeachFunc  func,
                                          gpointer                   data)
{
  CtkCssMatcher previous;

  while (_ctk_css_matcher_get_previous (&previous, matcher))
    {
      matcher = &previous;

      if (func (selector, matcher, data))
        return TRUE;

      /* any matchers are dangerous here, as we may loop forever, but
	 we can terminate now as all possible matches have already been added */
      if (_ctk_css_matcher_matches_any (matcher))
	break;
    }

  return FALSE;
}

static CtkCssChange
ctk_css_selector_sibling_get_change (const CtkCssSelector *selector G_GNUC_UNUSED,
                                     CtkCssChange          previous_change)
{
  return _ctk_css_change_for_sibling (previous_change);
}

static const CtkCssSelectorClass CTK_CSS_SELECTOR_SIBLING = {
  "sibling",
  ctk_css_selector_sibling_print,
  ctk_css_selector_sibling_foreach_matcher,
  ctk_css_selector_default_match_one,
  ctk_css_selector_sibling_get_change,
  ctk_css_selector_default_add_specificity,
  ctk_css_selector_default_hash_one,
  ctk_css_selector_default_compare_one,
  FALSE
};

/* ADJACENT */

static void
ctk_css_selector_adjacent_print (const CtkCssSelector *selector G_GNUC_UNUSED,
                                 GString              *string)
{
  g_string_append (string, " + ");
}

static gboolean
ctk_css_selector_adjacent_foreach_matcher (const CtkCssSelector      *selector,
                                           const CtkCssMatcher       *matcher,
                                           CtkCssSelectorForeachFunc  func,
                                           gpointer                   data)
{
  CtkCssMatcher previous;

  if (!_ctk_css_matcher_get_previous (&previous, matcher))
    return FALSE;
  
  return func (selector, &previous, data);
}

static CtkCssChange
ctk_css_selector_adjacent_get_change (const CtkCssSelector *selector G_GNUC_UNUSED,
                                      CtkCssChange          previous_change)
{
  return _ctk_css_change_for_sibling (previous_change);
}

static const CtkCssSelectorClass CTK_CSS_SELECTOR_ADJACENT = {
  "adjacent",
  ctk_css_selector_adjacent_print,
  ctk_css_selector_adjacent_foreach_matcher,
  ctk_css_selector_default_match_one,
  ctk_css_selector_adjacent_get_change,
  ctk_css_selector_default_add_specificity,
  ctk_css_selector_default_hash_one,
  ctk_css_selector_default_compare_one,
  FALSE
};

/* SIMPLE SELECTOR DEFINE */

#define DEFINE_SIMPLE_SELECTOR(n, \
                               c, \
                               print_func, \
                               match_func, \
                               hash_func, \
                               comp_func, \
                               increase_id_specificity, \
                               increase_class_specificity, \
                               increase_element_specificity) \
static void \
ctk_css_selector_ ## n ## _print (const CtkCssSelector *selector, \
                                    GString              *string) \
{ \
  print_func (selector, string); \
} \
\
static void \
ctk_css_selector_not_ ## n ## _print (const CtkCssSelector *selector, \
                                      GString              *string) \
{ \
  g_string_append (string, ":not("); \
  print_func (selector, string); \
  g_string_append (string, ")"); \
} \
\
static gboolean \
ctk_css_selector_not_ ## n ## _match_one (const CtkCssSelector *selector, \
                                          const CtkCssMatcher  *matcher) \
{ \
  return !match_func (selector, matcher); \
} \
\
static CtkCssChange \
ctk_css_selector_ ## n ## _get_change (const CtkCssSelector *selector G_GNUC_UNUSED, CtkCssChange previous_change) \
{ \
  return previous_change | CTK_CSS_CHANGE_ ## c; \
} \
\
static void \
ctk_css_selector_ ## n ## _add_specificity (const CtkCssSelector *selector G_GNUC_UNUSED, \
                                            guint                *ids, \
                                            guint                *classes, \
                                            guint                *elements) \
{ \
  if (increase_id_specificity) \
    { \
      (*ids)++; \
    } \
  if (increase_class_specificity) \
    { \
      (*classes)++; \
    } \
  if (increase_element_specificity) \
    { \
      (*elements)++; \
    } \
} \
\
static const CtkCssSelectorClass CTK_CSS_SELECTOR_ ## c = { \
  G_STRINGIFY(n), \
  ctk_css_selector_ ## n ## _print, \
  ctk_css_selector_default_foreach_matcher, \
  match_func, \
  ctk_css_selector_ ## n ## _get_change, \
  ctk_css_selector_ ## n ## _add_specificity, \
  hash_func, \
  comp_func, \
  TRUE \
};\
\
static const CtkCssSelectorClass CTK_CSS_SELECTOR_NOT_ ## c = { \
  "not_" G_STRINGIFY(n), \
  ctk_css_selector_not_ ## n ## _print, \
  ctk_css_selector_default_foreach_matcher, \
  ctk_css_selector_not_ ## n ## _match_one, \
  ctk_css_selector_ ## n ## _get_change, \
  ctk_css_selector_ ## n ## _add_specificity, \
  hash_func, \
  comp_func, \
  TRUE \
};

/* ANY */

static void
print_any (const CtkCssSelector *selector G_GNUC_UNUSED,
           GString              *string)
{
  g_string_append_c (string, '*');
}

static gboolean
match_any (const CtkCssSelector *selector G_GNUC_UNUSED,
           const CtkCssMatcher  *matcher G_GNUC_UNUSED)
{
  return TRUE;
}

#undef CTK_CSS_CHANGE_ANY
#define CTK_CSS_CHANGE_ANY 0
DEFINE_SIMPLE_SELECTOR(any, ANY, print_any, match_any, 
                       ctk_css_selector_default_hash_one, ctk_css_selector_default_compare_one,
                       FALSE, FALSE, FALSE)
#undef CTK_CSS_CHANGE_ANY

/* NAME */

static void
print_name (const CtkCssSelector *selector,
            GString              *string)
{
  g_string_append (string, selector->name.name);
}

static gboolean
match_name (const CtkCssSelector *selector,
            const CtkCssMatcher  *matcher)
{
  return _ctk_css_matcher_has_name (matcher, selector->name.name);
}

static guint
hash_name (const CtkCssSelector *a)
{
  return g_str_hash (a->name.name);
}

static int
comp_name (const CtkCssSelector *a,
           const CtkCssSelector *b)
{
  return strcmp (a->name.name,
		 b->name.name);
}

DEFINE_SIMPLE_SELECTOR(name, NAME, print_name, match_name, hash_name, comp_name, FALSE, FALSE, TRUE)

/* CLASS */

static void
print_class (const CtkCssSelector *selector,
             GString              *string)
{
  g_string_append_c (string, '.');
  g_string_append (string, g_quark_to_string (selector->style_class.style_class));
}

static gboolean
match_class (const CtkCssSelector *selector,
             const CtkCssMatcher  *matcher)
{
  return _ctk_css_matcher_has_class (matcher, selector->style_class.style_class);
}

static guint
hash_class (const CtkCssSelector *a)
{
  return a->style_class.style_class;
}

static int
comp_class (const CtkCssSelector *a,
            const CtkCssSelector *b)
{
  if (a->style_class.style_class < b->style_class.style_class)
    return -1;
  if (a->style_class.style_class > b->style_class.style_class)
    return 1;
  else
    return 0;
}

DEFINE_SIMPLE_SELECTOR(class, CLASS, print_class, match_class, hash_class, comp_class, FALSE, TRUE, FALSE)

/* ID */

static void
print_id (const CtkCssSelector *selector,
          GString              *string)
{
  g_string_append_c (string, '#');
  g_string_append (string, selector->id.name);
}

static gboolean
match_id (const CtkCssSelector *selector,
          const CtkCssMatcher  *matcher)
{
  return _ctk_css_matcher_has_id (matcher, selector->id.name);
}

static guint
hash_id (const CtkCssSelector *a)
{
  return GPOINTER_TO_UINT (a->id.name);
}

static int
comp_id (const CtkCssSelector *a,
	 const CtkCssSelector *b)
{
  if (a->id.name < b->id.name)
    return -1;
  else if (a->id.name > b->id.name)
    return 1;
  else
    return 0;
}

DEFINE_SIMPLE_SELECTOR(id, ID, print_id, match_id, hash_id, comp_id, TRUE, FALSE, FALSE)

const gchar *
ctk_css_pseudoclass_name (CtkStateFlags state)
{
  static const char * state_names[] = {
    "active",
    "hover",
    "selected",
    "disabled",
    "indeterminate",
    "focus",
    "backdrop",
    "dir(ltr)",
    "dir(rtl)",
    "link",
    "visited",
    "checked",
    "drop(active)"
  };
  guint i;

  for (i = 0; i < G_N_ELEMENTS (state_names); i++)
    {
      if (state == (1 << i))
        return state_names[i];
    }

  return NULL;
}

/* PSEUDOCLASS FOR STATE */
static void
print_pseudoclass_state (const CtkCssSelector *selector,
                         GString              *string)
{
  g_string_append_c (string, ':');
  g_string_append (string, ctk_css_pseudoclass_name (selector->state.state));
}

static gboolean
match_pseudoclass_state (const CtkCssSelector *selector,
                         const CtkCssMatcher  *matcher)
{
  return (_ctk_css_matcher_get_state (matcher) & selector->state.state) == selector->state.state;
}

static guint
hash_pseudoclass_state (const CtkCssSelector *selector)
{
  return selector->state.state;
}


static int
comp_pseudoclass_state (const CtkCssSelector *a,
		        const CtkCssSelector *b)
{
  return a->state.state - b->state.state;
}

#define CTK_CSS_CHANGE_PSEUDOCLASS_STATE CTK_CSS_CHANGE_STATE
DEFINE_SIMPLE_SELECTOR(pseudoclass_state, PSEUDOCLASS_STATE, print_pseudoclass_state,
                       match_pseudoclass_state, hash_pseudoclass_state, comp_pseudoclass_state,
                       FALSE, TRUE, FALSE)
#undef CTK_CSS_CHANGE_PSEUDOCLASS_STATE

/* PSEUDOCLASS FOR POSITION */

static void
print_pseudoclass_position (const CtkCssSelector *selector,
                            GString              *string)
{
  switch (selector->position.type)
    {
    case POSITION_FORWARD:
      if (selector->position.a == 0)
        {
          if (selector->position.b == 1)
            g_string_append (string, ":first-child");
          else
            g_string_append_printf (string, ":nth-child(%d)", selector->position.b);
        }
      else if (selector->position.a == 2 && selector->position.b == 0)
        g_string_append (string, ":nth-child(even)");
      else if (selector->position.a == 2 && selector->position.b == 1)
        g_string_append (string, ":nth-child(odd)");
      else
        {
          g_string_append (string, ":nth-child(");
          if (selector->position.a == 1)
            g_string_append (string, "n");
          else if (selector->position.a == -1)
            g_string_append (string, "-n");
          else
            g_string_append_printf (string, "%dn", selector->position.a);
          if (selector->position.b > 0)
            g_string_append_printf (string, "+%d)", selector->position.b);
          else if (selector->position.b < 0)
            g_string_append_printf (string, "%d)", selector->position.b);
          else
            g_string_append (string, ")");
        }
      break;
    case POSITION_BACKWARD:
      if (selector->position.a == 0)
        {
          if (selector->position.b == 1)
            g_string_append (string, ":last-child");
          else
            g_string_append_printf (string, ":nth-last-child(%d)", selector->position.b);
        }
      else if (selector->position.a == 2 && selector->position.b == 0)
        g_string_append (string, ":nth-last-child(even)");
      else if (selector->position.a == 2 && selector->position.b == 1)
        g_string_append (string, ":nth-last-child(odd)");
      else
        {
          g_string_append (string, ":nth-last-child(");
          if (selector->position.a == 1)
            g_string_append (string, "n");
          else if (selector->position.a == -1)
            g_string_append (string, "-n");
          else
            g_string_append_printf (string, "%dn", selector->position.a);
          if (selector->position.b > 0)
            g_string_append_printf (string, "+%d)", selector->position.b);
          else if (selector->position.b < 0)
            g_string_append_printf (string, "%d)", selector->position.b);
          else
            g_string_append (string, ")");
        }
      break;
    case POSITION_ONLY:
      g_string_append (string, ":only-child");
      break;
    case POSITION_SORTED:
      g_string_append (string, ":sorted");
      break;
    default:
      g_assert_not_reached ();
      break;
    }
}

static gboolean
match_pseudoclass_position (const CtkCssSelector *selector,
		            const CtkCssMatcher  *matcher)
{
  switch (selector->position.type)
    {
    case POSITION_FORWARD:
      if (!_ctk_css_matcher_has_position (matcher, TRUE, selector->position.a, selector->position.b))
        return FALSE;
      break;
    case POSITION_BACKWARD:
      if (!_ctk_css_matcher_has_position (matcher, FALSE, selector->position.a, selector->position.b))
        return FALSE;
      break;
    case POSITION_ONLY:
      if (!_ctk_css_matcher_has_position (matcher, TRUE, 0, 1) ||
          !_ctk_css_matcher_has_position (matcher, FALSE, 0, 1))
        return FALSE;
      break;
    case POSITION_SORTED:
      return FALSE;
    default:
      g_assert_not_reached ();
      return FALSE;
    }

  return TRUE;
}

static guint
hash_pseudoclass_position (const CtkCssSelector *a)
{
  return (guint)(((((gulong)a->position.type) << POSITION_NUMBER_BITS) | a->position.a) << POSITION_NUMBER_BITS) | a->position.b;
}

static int
comp_pseudoclass_position (const CtkCssSelector *a,
			   const CtkCssSelector *b)
{
  int diff;
  
  diff = a->position.type - b->position.type;
  if (diff)
    return diff;

  diff = a->position.a - b->position.a;
  if (diff)
    return diff;

  return a->position.b - b->position.b;
}

static CtkCssChange
change_pseudoclass_position (const CtkCssSelector *selector)
{
  switch (selector->position.type)
    {
    case POSITION_FORWARD:
      if (selector->position.a == 0 && selector->position.b == 1)
        return CTK_CSS_CHANGE_FIRST_CHILD;
      else
        return CTK_CSS_CHANGE_NTH_CHILD;
    case POSITION_BACKWARD:
      if (selector->position.a == 0 && selector->position.b == 1)
        return CTK_CSS_CHANGE_LAST_CHILD;
      else
        return CTK_CSS_CHANGE_NTH_LAST_CHILD;
    case POSITION_ONLY:
      return CTK_CSS_CHANGE_FIRST_CHILD | CTK_CSS_CHANGE_LAST_CHILD;
    default:
      g_assert_not_reached ();
    case POSITION_SORTED:
      return 0;
    }
}

#define CTK_CSS_CHANGE_PSEUDOCLASS_POSITION change_pseudoclass_position(selector)
DEFINE_SIMPLE_SELECTOR(pseudoclass_position, PSEUDOCLASS_POSITION, print_pseudoclass_position,
                       match_pseudoclass_position, hash_pseudoclass_position, comp_pseudoclass_position,
                       FALSE, TRUE, FALSE)
#undef CTK_CSS_CHANGE_PSEUDOCLASS_POSITION
/* API */

static guint
ctk_css_selector_size (const CtkCssSelector *selector)
{
  guint size = 0;

  while (selector)
    {
      selector = ctk_css_selector_previous (selector);
      size++;
    }

  return size;
}

static CtkCssSelector *
ctk_css_selector_new (const CtkCssSelectorClass *class,
                      CtkCssSelector            *selector)
{
  guint size;

  size = ctk_css_selector_size (selector);
  selector = g_realloc (selector, sizeof (CtkCssSelector) * (size + 1) + sizeof (gpointer));
  if (size == 0)
    selector[1].class = NULL;
  else
    memmove (selector + 1, selector, sizeof (CtkCssSelector) * size + sizeof (gpointer));

  memset (selector, 0, sizeof (CtkCssSelector));
  selector->class = class;

  return selector;
}

static CtkCssSelector *
parse_selector_class (CtkCssParser   *parser,
                      CtkCssSelector *selector,
                      gboolean        negate)
{
  char *name;
    
  name = _ctk_css_parser_try_name (parser, FALSE);

  if (name == NULL)
    {
      _ctk_css_parser_error (parser, "Expected a valid name for class");
      if (selector)
        _ctk_css_selector_free (selector);
      return NULL;
    }

  selector = ctk_css_selector_new (negate ? &CTK_CSS_SELECTOR_NOT_CLASS
                                          : &CTK_CSS_SELECTOR_CLASS,
                                   selector);
  selector->style_class.style_class = g_quark_from_string (name);

  g_free (name);

  return selector;
}

static CtkCssSelector *
parse_selector_id (CtkCssParser   *parser,
                   CtkCssSelector *selector,
                   gboolean        negate)
{
  char *name;
    
  name = _ctk_css_parser_try_name (parser, FALSE);

  if (name == NULL)
    {
      _ctk_css_parser_error (parser, "Expected a valid name for id");
      if (selector)
        _ctk_css_selector_free (selector);
      return NULL;
    }

  selector = ctk_css_selector_new (negate ? &CTK_CSS_SELECTOR_NOT_ID
                                          : &CTK_CSS_SELECTOR_ID,
                                   selector);
  selector->id.name = g_intern_string (name);

  g_free (name);

  return selector;
}

static CtkCssSelector *
parse_selector_pseudo_class_nth_child (CtkCssParser   *parser,
                                       CtkCssSelector *selector,
                                       PositionType    type,
                                       gboolean        negate)
{
  int a, b;

  if (!_ctk_css_parser_try (parser, "(", TRUE))
    {
      _ctk_css_parser_error (parser, "Missing opening bracket for pseudo-class");
      if (selector)
        _ctk_css_selector_free (selector);
      return NULL;
    }

  if (_ctk_css_parser_try (parser, "even", TRUE))
    {
      a = 2;
      b = 0;
    }
  else if (_ctk_css_parser_try (parser, "odd", TRUE))
    {
      a = 2;
      b = 1;
    }
  else if (type == POSITION_FORWARD &&
           _ctk_css_parser_try (parser, "first", TRUE))
    {
      a = 0;
      b = 1;
    }
  else if (type == POSITION_FORWARD &&
           _ctk_css_parser_try (parser, "last", TRUE))
    {
      a = 0;
      b = 1;
      type = POSITION_BACKWARD;
    }
  else
    {
      int multiplier;

      if (_ctk_css_parser_try (parser, "+", TRUE))
        multiplier = 1;
      else if (_ctk_css_parser_try (parser, "-", TRUE))
        multiplier = -1;
      else
        multiplier = 1;

      if (_ctk_css_parser_try_int (parser, &a))
        {
          if (a < 0)
            {
              _ctk_css_parser_error (parser, "Expected an integer");
              if (selector)
                _ctk_css_selector_free (selector);
              return NULL;
            }
          a *= multiplier;
        }
      else if (_ctk_css_parser_has_prefix (parser, "n"))
        {
          a = multiplier;
        }
      else
        {
          _ctk_css_parser_error (parser, "Expected an integer");
          if (selector)
            _ctk_css_selector_free (selector);
          return NULL;
        }

      if (_ctk_css_parser_try (parser, "n", TRUE))
        {
          if (_ctk_css_parser_try (parser, "+", TRUE))
            multiplier = 1;
          else if (_ctk_css_parser_try (parser, "-", TRUE))
            multiplier = -1;
          else
            multiplier = 1;

          if (_ctk_css_parser_try_int (parser, &b))
            {
              if (b < 0)
                {
                  _ctk_css_parser_error (parser, "Expected an integer");
                  if (selector)
                    _ctk_css_selector_free (selector);
                  return NULL;
                }
            }
          else
            b = 0;

          b *= multiplier;
        }
      else
        {
          b = a;
          a = 0;
        }
    }

  if (!_ctk_css_parser_try (parser, ")", FALSE))
    {
      _ctk_css_parser_error (parser, "Missing closing bracket for pseudo-class");
      if (selector)
        _ctk_css_selector_free (selector);
      return NULL;
    }

  selector = ctk_css_selector_new (negate ? &CTK_CSS_SELECTOR_NOT_PSEUDOCLASS_POSITION
                                          : &CTK_CSS_SELECTOR_PSEUDOCLASS_POSITION,
                                   selector);
  selector->position.type = type;
  selector->position.a = a;
  selector->position.b = b;

  return selector;
}

static CtkCssSelector *
parse_selector_pseudo_class (CtkCssParser   *parser,
                             CtkCssSelector *selector,
                             gboolean        negate)
{
  static const struct {
    const char    *name;
    gboolean       deprecated;
    CtkStateFlags  state_flag;
    PositionType   position_type;
    int            position_a;
    int            position_b;
  } pseudo_classes[] = {
    { .name = "first-child",   .deprecated = 0, .state_flag = 0, .position_type = POSITION_FORWARD,  .position_a = 0, .position_b = 1 },
    { .name = "last-child",    .deprecated = 0, .state_flag = 0, .position_type = POSITION_BACKWARD, .position_a = 0, .position_b = 1 },
    { .name = "only-child",    .deprecated = 0, .state_flag = 0, .position_type = POSITION_ONLY,     .position_a = 0, .position_b = 0 },
    { .name = "sorted",        .deprecated = 1, .state_flag = 0, .position_type = POSITION_SORTED,   .position_a = 0, .position_b = 0 },
    { .name = "active",        .deprecated = 0, .state_flag = CTK_STATE_FLAG_ACTIVE, },
    { .name = "prelight",      .deprecated = 1, .state_flag = CTK_STATE_FLAG_PRELIGHT, },
    { .name = "hover",         .deprecated = 0, .state_flag = CTK_STATE_FLAG_PRELIGHT, },
    { .name = "selected",      .deprecated = 0, .state_flag = CTK_STATE_FLAG_SELECTED, },
    { .name = "insensitive",   .deprecated = 1, .state_flag = CTK_STATE_FLAG_INSENSITIVE, },
    { .name = "disabled",      .deprecated = 0, .state_flag = CTK_STATE_FLAG_INSENSITIVE, },
    { .name = "inconsistent",  .deprecated = 1, .state_flag = CTK_STATE_FLAG_INCONSISTENT, },
    { .name = "indeterminate", .deprecated = 0, .state_flag = CTK_STATE_FLAG_INCONSISTENT, },
    { .name = "focused",       .deprecated = 1, .state_flag = CTK_STATE_FLAG_FOCUSED, },
    { .name = "focus",         .deprecated = 0, .state_flag = CTK_STATE_FLAG_FOCUSED, },
    { .name = "backdrop",      .deprecated = 0, .state_flag = CTK_STATE_FLAG_BACKDROP, },
    { .name = "dir(ltr)",      .deprecated = 0, .state_flag = CTK_STATE_FLAG_DIR_LTR, },
    { .name = "dir(rtl)",      .deprecated = 0, .state_flag = CTK_STATE_FLAG_DIR_RTL, },
    { .name = "link",          .deprecated = 0, .state_flag = CTK_STATE_FLAG_LINK, },
    { .name = "visited",       .deprecated = 0, .state_flag = CTK_STATE_FLAG_VISITED, },
    { .name = "checked",       .deprecated = 0, .state_flag = CTK_STATE_FLAG_CHECKED, },
    { .name = "drop(active)",  .deprecated = 0, .state_flag = CTK_STATE_FLAG_DROP_ACTIVE, }
  };

  guint i;

  if (_ctk_css_parser_try (parser, "nth-child", FALSE))
    return parse_selector_pseudo_class_nth_child (parser, selector, POSITION_FORWARD, negate);
  else if (_ctk_css_parser_try (parser, "nth-last-child", FALSE))
    return parse_selector_pseudo_class_nth_child (parser, selector, POSITION_BACKWARD, negate);

  for (i = 0; i < G_N_ELEMENTS (pseudo_classes); i++)
    {
      if (_ctk_css_parser_try (parser, pseudo_classes[i].name, FALSE))
        {
          if (pseudo_classes[i].state_flag)
            {
              selector = ctk_css_selector_new (negate ? &CTK_CSS_SELECTOR_NOT_PSEUDOCLASS_STATE
                                                      : &CTK_CSS_SELECTOR_PSEUDOCLASS_STATE,
                                               selector);
              selector->state.state = pseudo_classes[i].state_flag;
              if (pseudo_classes[i].deprecated)
                {
                  if (i + 1 < G_N_ELEMENTS (pseudo_classes) &&
                      pseudo_classes[i + 1].state_flag == pseudo_classes[i].state_flag)
                    _ctk_css_parser_error_full (parser,
                                                CTK_CSS_PROVIDER_ERROR_DEPRECATED,
                                                "The :%s pseudo-class is deprecated. Use :%s instead.",
                                                pseudo_classes[i].name,
                                                pseudo_classes[i + 1].name);
                  else
                    _ctk_css_parser_error_full (parser,
                                                CTK_CSS_PROVIDER_ERROR_DEPRECATED,
                                                "The :%s pseudo-class is deprecated.",
                                                pseudo_classes[i].name);
                }
            }
          else
            {
              selector = ctk_css_selector_new (negate ? &CTK_CSS_SELECTOR_NOT_PSEUDOCLASS_POSITION
                                                      : &CTK_CSS_SELECTOR_PSEUDOCLASS_POSITION,
                                               selector);
              selector->position.type = pseudo_classes[i].position_type;
              selector->position.a = pseudo_classes[i].position_a;
              selector->position.b = pseudo_classes[i].position_b;
            }
          return selector;
        }
    }
      
  _ctk_css_parser_error (parser, "Invalid name of pseudo-class");
  if (selector)
    _ctk_css_selector_free (selector);
  return NULL;
}

static CtkCssSelector *
parse_selector_negation (CtkCssParser   *parser,
                         CtkCssSelector *selector)
{
  char *name;

  name = _ctk_css_parser_try_ident (parser, FALSE);
  if (name)
    {
      selector = ctk_css_selector_new (&CTK_CSS_SELECTOR_NOT_NAME,
                                       selector);
      selector->name.name = g_intern_string (name);
      g_free (name);
    }
  else if (_ctk_css_parser_try (parser, "*", FALSE))
    selector = ctk_css_selector_new (&CTK_CSS_SELECTOR_NOT_ANY, selector);
  else if (_ctk_css_parser_try (parser, "#", FALSE))
    selector = parse_selector_id (parser, selector, TRUE);
  else if (_ctk_css_parser_try (parser, ".", FALSE))
    selector = parse_selector_class (parser, selector, TRUE);
  else if (_ctk_css_parser_try (parser, ":", FALSE))
    selector = parse_selector_pseudo_class (parser, selector, TRUE);
  else
    {
      _ctk_css_parser_error (parser, "Not a valid selector for :not()");
      if (selector)
        _ctk_css_selector_free (selector);
      return NULL;
    }
  
  _ctk_css_parser_skip_whitespace (parser);

  if (!_ctk_css_parser_try (parser, ")", FALSE))
    {
      _ctk_css_parser_error (parser, "Missing closing bracket for :not()");
      if (selector)
        _ctk_css_selector_free (selector);
      return NULL;
    }

  return selector;
}

static CtkCssSelector *
parse_simple_selector (CtkCssParser   *parser,
                       CtkCssSelector *selector)
{
  gboolean parsed_something = FALSE;
  char *name;

  name = _ctk_css_parser_try_ident (parser, FALSE);
  if (name)
    {
      selector = ctk_css_selector_new (&CTK_CSS_SELECTOR_NAME,
                                       selector);
      selector->name.name = g_intern_string (name);
      g_free (name);
      parsed_something = TRUE;
    }
  else if (_ctk_css_parser_try (parser, "*", FALSE))
    {
      selector = ctk_css_selector_new (&CTK_CSS_SELECTOR_ANY, selector);
      parsed_something = TRUE;
    }

  do {
      if (_ctk_css_parser_try (parser, "#", FALSE))
        selector = parse_selector_id (parser, selector, FALSE);
      else if (_ctk_css_parser_try (parser, ".", FALSE))
        selector = parse_selector_class (parser, selector, FALSE);
      else if (_ctk_css_parser_try (parser, ":not(", TRUE))
        selector = parse_selector_negation (parser, selector);
      else if (_ctk_css_parser_try (parser, ":", FALSE))
        selector = parse_selector_pseudo_class (parser, selector, FALSE);
      else if (!parsed_something)
        {
          _ctk_css_parser_error (parser, "Expected a valid selector");
          if (selector)
            _ctk_css_selector_free (selector);
          return NULL;
        }
      else
        break;

      parsed_something = TRUE;
    }
  while (selector && !_ctk_css_parser_is_eof (parser));

  _ctk_css_parser_skip_whitespace (parser);

  return selector;
}

CtkCssSelector *
_ctk_css_selector_parse (CtkCssParser *parser)
{
  CtkCssSelector *selector = NULL;

  while ((selector = parse_simple_selector (parser, selector)) &&
         !_ctk_css_parser_is_eof (parser) &&
         !_ctk_css_parser_begins_with (parser, ',') &&
         !_ctk_css_parser_begins_with (parser, '{'))
    {
      if (_ctk_css_parser_try (parser, "+", TRUE))
        selector = ctk_css_selector_new (&CTK_CSS_SELECTOR_ADJACENT, selector);
      else if (_ctk_css_parser_try (parser, "~", TRUE))
        selector = ctk_css_selector_new (&CTK_CSS_SELECTOR_SIBLING, selector);
      else if (_ctk_css_parser_try (parser, ">", TRUE))
        selector = ctk_css_selector_new (&CTK_CSS_SELECTOR_CHILD, selector);
      else
        selector = ctk_css_selector_new (&CTK_CSS_SELECTOR_DESCENDANT, selector);
    }

  return selector;
}

void
_ctk_css_selector_free (CtkCssSelector *selector)
{
  g_return_if_fail (selector != NULL);

  g_free (selector);
}

void
_ctk_css_selector_print (const CtkCssSelector *selector,
                         GString *             str)
{
  const CtkCssSelector *previous;

  g_return_if_fail (selector != NULL);

  previous = ctk_css_selector_previous (selector);
  if (previous)
    _ctk_css_selector_print (previous, str);

  selector->class->print (selector, str);
}

char *
_ctk_css_selector_to_string (const CtkCssSelector *selector)
{
  GString *string;

  g_return_val_if_fail (selector != NULL, NULL);

  string = g_string_new (NULL);

  _ctk_css_selector_print (selector, string);

  return g_string_free (string, FALSE);
}

static gboolean
ctk_css_selector_foreach_match (const CtkCssSelector *selector,
                                const CtkCssMatcher  *matcher,
                                gpointer              unused G_GNUC_UNUSED)
{
  selector = ctk_css_selector_previous (selector);

  if (selector == NULL)
    return TRUE;

  if (!ctk_css_selector_match (selector, matcher))
    return FALSE;

  return ctk_css_selector_foreach (selector, matcher, ctk_css_selector_foreach_match, NULL);
}

/**
 * _ctk_css_selector_matches:
 * @selector: the selector
 * @path: the path to check
 * @state: The state to match
 *
 * Checks if the @selector matches the given @path. If @length is
 * smaller than the number of elements in @path, it is assumed that
 * only the first @length element of @path are valid and the rest
 * does not exist. This is useful for doing parent matches for the
 * 'inherit' keyword.
 *
 * Returns: %TRUE if the selector matches @path
 **/
gboolean
_ctk_css_selector_matches (const CtkCssSelector *selector,
                           const CtkCssMatcher  *matcher)
{

  g_return_val_if_fail (selector != NULL, FALSE);
  g_return_val_if_fail (matcher != NULL, FALSE);

  if (!ctk_css_selector_match (selector, matcher))
    return FALSE;

  return ctk_css_selector_foreach (selector, matcher, ctk_css_selector_foreach_match, NULL);
}

/* Computes specificity according to CSS 2.1.
 * The arguments must be initialized to 0 */
static void
_ctk_css_selector_get_specificity (const CtkCssSelector *selector,
                                   guint                *ids,
                                   guint                *classes,
                                   guint                *elements)
{
  for (; selector; selector = ctk_css_selector_previous (selector))
    {
      selector->class->add_specificity (selector, ids, classes, elements);
    }
}

int
_ctk_css_selector_compare (const CtkCssSelector *a,
                           const CtkCssSelector *b)
{
  guint a_ids = 0, a_classes = 0, a_elements = 0;
  guint b_ids = 0, b_classes = 0, b_elements = 0;
  int compare;

  _ctk_css_selector_get_specificity (a, &a_ids, &a_classes, &a_elements);
  _ctk_css_selector_get_specificity (b, &b_ids, &b_classes, &b_elements);

  compare = a_ids - b_ids;
  if (compare)
    return compare;

  compare = a_classes - b_classes;
  if (compare)
    return compare;

  return a_elements - b_elements;
}

CtkCssChange
_ctk_css_selector_get_change (const CtkCssSelector *selector)
{
  if (selector == NULL)
    return 0;

  return selector->class->get_change (selector, _ctk_css_selector_get_change (ctk_css_selector_previous (selector)));
}

/******************** SelectorTree handling *****************/

static GHashTable *
ctk_css_selectors_count_initial_init (void)
{
  return g_hash_table_new ((GHashFunc)ctk_css_selector_hash_one, (GEqualFunc)ctk_css_selector_equal);
}

static void
ctk_css_selectors_count_initial (const CtkCssSelector *selector, GHashTable *hash_one)
{
  if (!selector->class->is_simple)
    {
      guint count = GPOINTER_TO_INT (g_hash_table_lookup (hash_one, selector));
      g_hash_table_replace (hash_one, (gpointer)selector, GUINT_TO_POINTER (count + 1));
      return;
    }

  for (;
       selector && selector->class->is_simple;
       selector = ctk_css_selector_previous (selector))
    {
      guint count = GPOINTER_TO_INT (g_hash_table_lookup (hash_one, selector));
      g_hash_table_replace (hash_one, (gpointer)selector, GUINT_TO_POINTER (count + 1));
    }
}

static gboolean
ctk_css_selectors_has_initial_selector (const CtkCssSelector *selector, const CtkCssSelector *initial)
{
  if (!selector->class->is_simple)
    return ctk_css_selector_equal (selector, initial);

  for (;
       selector && selector->class->is_simple;
       selector = ctk_css_selector_previous (selector))
    {
      if (ctk_css_selector_equal (selector, initial))
	return TRUE;
    }

  return FALSE;
}

static CtkCssSelector *
ctk_css_selectors_skip_initial_selector (CtkCssSelector *selector, const CtkCssSelector *initial)
{
  CtkCssSelector *found;
  CtkCssSelector tmp;

  /* If the initial simple selector is not first, move it there so we can skip it
     without losing any other selectors */
  if (!ctk_css_selector_equal (selector, initial))
    {
      for (found = selector; found && found->class->is_simple; found = (CtkCssSelector *)ctk_css_selector_previous (found))
	{
	  if (ctk_css_selector_equal (found, initial))
	    break;
	}

      g_assert (found != NULL && found->class->is_simple);

      tmp = *found;
      *found = *selector;
      *selector = tmp;
    }

  return (CtkCssSelector *)ctk_css_selector_previous (selector);
}

static gboolean
ctk_css_selector_tree_match_foreach (const CtkCssSelector *selector,
                                     const CtkCssMatcher  *matcher,
                                     gpointer              res)
{
  const CtkCssSelectorTree *tree = (const CtkCssSelectorTree *) selector;
  const CtkCssSelectorTree *prev;

  if (!ctk_css_selector_match (selector, matcher))
    return FALSE;

  ctk_css_selector_tree_found_match (tree, res);

  for (prev = ctk_css_selector_tree_get_previous (tree);
       prev != NULL;
       prev = ctk_css_selector_tree_get_sibling (prev))
    ctk_css_selector_foreach (&prev->selector, matcher, ctk_css_selector_tree_match_foreach, res);

  return FALSE;
}

GPtrArray *
_ctk_css_selector_tree_match_all (const CtkCssSelectorTree *tree,
				  const CtkCssMatcher *matcher)
{
  GPtrArray *array = NULL;

  for (; tree != NULL;
       tree = ctk_css_selector_tree_get_sibling (tree))
    ctk_css_selector_foreach (&tree->selector, matcher, ctk_css_selector_tree_match_foreach, &array);

  return array;
}

/* When checking for changes via the tree we need to know if a rule further
   down the tree matched, because if so we need to add "our bit" to the
   Change. For instance in a a match like *.class:active we'll
   get a tree that first checks :active, if that matches we continue down
   to the tree, and if we get a match we add CHANGE_CLASS. However, the
   end of the tree where we have a match is an ANY which doesn't actually
   modify the change, so we don't know if we have a match or not. We fix
   this by setting CTK_CSS_CHANGE_GOT_MATCH which lets us guarantee
   that change != 0 on any match. */
#define CTK_CSS_CHANGE_GOT_MATCH CTK_CSS_CHANGE_RESERVED_BIT

static CtkCssChange
ctk_css_selector_tree_collect_change (const CtkCssSelectorTree *tree)
{
  CtkCssChange change = 0;
  const CtkCssSelectorTree *prev;

  for (prev = ctk_css_selector_tree_get_previous (tree);
       prev != NULL;
       prev = ctk_css_selector_tree_get_sibling (prev))
    change |= ctk_css_selector_tree_collect_change (prev);

  change = tree->selector.class->get_change (&tree->selector, change);

  return change;
}

static CtkCssChange
ctk_css_selector_tree_get_change (const CtkCssSelectorTree *tree,
				  const CtkCssMatcher      *matcher)
{
  CtkCssChange change = 0;
  const CtkCssSelectorTree *prev;

  if (!ctk_css_selector_match (&tree->selector, matcher))
    return 0;

  if (!tree->selector.class->is_simple)
    return ctk_css_selector_tree_collect_change (tree) | CTK_CSS_CHANGE_GOT_MATCH;

  for (prev = ctk_css_selector_tree_get_previous (tree);
       prev != NULL;
       prev = ctk_css_selector_tree_get_sibling (prev))
    change |= ctk_css_selector_tree_get_change (prev, matcher);

  if (change || ctk_css_selector_tree_get_matches (tree))
    change = tree->selector.class->get_change (&tree->selector, change & ~CTK_CSS_CHANGE_GOT_MATCH) | CTK_CSS_CHANGE_GOT_MATCH;

  return change;
}

CtkCssChange
_ctk_css_selector_tree_get_change_all (const CtkCssSelectorTree *tree,
				       const CtkCssMatcher *matcher)
{
  CtkCssChange change;

  change = 0;

  /* no need to foreach here because we abort for non-simple selectors */
  for (; tree != NULL;
       tree = ctk_css_selector_tree_get_sibling (tree))
    change |= ctk_css_selector_tree_get_change (tree, matcher);

  /* Never return reserved bit set */
  return change & ~CTK_CSS_CHANGE_RESERVED_BIT;
}

#ifdef PRINT_TREE
static void
_ctk_css_selector_tree_print (const CtkCssSelectorTree *tree, GString *str, char *prefix)
{
  gboolean first = TRUE;
  int len, i;

  for (; tree != NULL; tree = ctk_css_selector_tree_get_sibling (tree), first = FALSE)
    {
      if (!first)
	g_string_append (str, prefix);

      if (first)
	{
	  if (ctk_css_selector_tree_get_sibling (tree))
	    g_string_append (str, "─┬─");
	  else
	    g_string_append (str, "───");
	}
      else
	{
	  if (ctk_css_selector_tree_get_sibling (tree))
	    g_string_append (str, " ├─");
	  else
	    g_string_append (str, " └─");
	}

      len = str->len;
      tree->selector.class->print (&tree->selector, str);
      len = str->len - len;

      if (ctk_css_selector_tree_get_previous (tree))
	{
	  GString *prefix2 = g_string_new (prefix);

	  if (ctk_css_selector_tree_get_sibling (tree))
	    g_string_append (prefix2, " │ ");
	  else
	    g_string_append (prefix2, "   ");
	  for (i = 0; i < len; i++)
	    g_string_append_c (prefix2, ' ');

	  _ctk_css_selector_tree_print (ctk_css_selector_tree_get_previous (tree), str, prefix2->str);
	  g_string_free (prefix2, TRUE);
	}
      else
	g_string_append (str, "\n");
    }
}
#endif

void
_ctk_css_selector_tree_match_print (const CtkCssSelectorTree *tree,
				    GString *str)
{
  const CtkCssSelectorTree *iter;

  g_return_if_fail (tree != NULL);

  /* print name and * selector before others */
  for (iter = tree; 
       iter && iter->selector.class->is_simple;
       iter = ctk_css_selector_tree_get_parent (iter))
    {
      if (iter->selector.class == &CTK_CSS_SELECTOR_NAME ||
          iter->selector.class == &CTK_CSS_SELECTOR_ANY)
        {
          iter->selector.class->print (&iter->selector, str);
        }
    }
  /* now print other simple selectors */
  for (iter = tree; 
       iter && iter->selector.class->is_simple;
       iter = ctk_css_selector_tree_get_parent (iter))
    {
      if (iter->selector.class != &CTK_CSS_SELECTOR_NAME &&
          iter->selector.class != &CTK_CSS_SELECTOR_ANY)
        {
          iter->selector.class->print (&iter->selector, str);
        }
    }

  /* now if there's a combinator, print that one */
  if (iter != NULL)
    {
      iter->selector.class->print (&iter->selector, str);
      tree = ctk_css_selector_tree_get_parent (iter);
      if (tree)
        _ctk_css_selector_tree_match_print (tree, str);
    }
}

void
_ctk_css_selector_tree_free (CtkCssSelectorTree *tree)
{
  if (tree == NULL)
    return;

  g_free (tree);
}


typedef struct {
  gpointer match;
  CtkCssSelector *current_selector;
  CtkCssSelectorTree **selector_match;
} CtkCssSelectorRuleSetInfo;

static CtkCssSelectorTree *
get_tree (GByteArray *array, gint32 offset)
{
  return (CtkCssSelectorTree *) (array->data + offset);
}

static CtkCssSelectorTree *
alloc_tree (GByteArray *array, gint32 *offset)
{
  CtkCssSelectorTree tree = { .selector = { NULL } };

  *offset = array->len;
  g_byte_array_append (array, (guint8 *)&tree, sizeof (CtkCssSelectorTree));
  return get_tree (array, *offset);
}

static gint32
subdivide_infos (GByteArray *array, GList *infos, gint32 parent_offset)
{
  GHashTable *ht;
  GList *l;
  GList *matched;
  GList *remaining;
  gint32 tree_offset;
  CtkCssSelectorTree *tree;
  CtkCssSelectorRuleSetInfo *info;
  CtkCssSelector max_selector;
  GHashTableIter iter;
  guint max_count;
  gpointer key, value;
  GPtrArray *exact_matches;
  gint32 res;

  if (infos == NULL)
    return CTK_CSS_SELECTOR_TREE_EMPTY_OFFSET;

  ht = ctk_css_selectors_count_initial_init ();

  for (l = infos; l != NULL; l = l->next)
    {
      info = l->data;
      ctk_css_selectors_count_initial (info->current_selector, ht);
    }

  /* Pick the selector with highest count, and use as decision on this level
     as that makes it possible to skip the largest amount of checks later */

  max_count = 0;

  g_hash_table_iter_init (&iter, ht);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      CtkCssSelector *selector = key;
      if (GPOINTER_TO_UINT (value) > max_count ||
	  (GPOINTER_TO_UINT (value) == max_count &&
	  ctk_css_selector_compare_one (selector, &max_selector) < 0))
	{
	  max_count = GPOINTER_TO_UINT (value);
	  max_selector = *selector;
	}
    }

  matched = NULL;
  remaining = NULL;

  tree = alloc_tree (array, &tree_offset);
  tree->parent_offset = parent_offset;
  tree->selector = max_selector;

  exact_matches = NULL;
  for (l = infos; l != NULL; l = l->next)
    {
      info = l->data;

      if (ctk_css_selectors_has_initial_selector (info->current_selector, &max_selector))
	{
	  info->current_selector = ctk_css_selectors_skip_initial_selector (info->current_selector, &max_selector);
	  if (info->current_selector == NULL)
	    {
	      /* Matches current node */
	      if (exact_matches == NULL)
		exact_matches = g_ptr_array_new ();
	      g_ptr_array_add (exact_matches, info->match);
	      if (info->selector_match != NULL)
		*info->selector_match = GUINT_TO_POINTER (tree_offset);
	    }
	  else
	    matched = g_list_prepend (matched, info);
	}
      else
	{
	  remaining = g_list_prepend (remaining, info);
	}
    }

  if (exact_matches)
    {
      g_ptr_array_add (exact_matches, NULL); /* Null terminate */
      res = array->len;
      g_byte_array_append (array, (guint8 *)exact_matches->pdata,
			   exact_matches->len * sizeof (gpointer));
      g_ptr_array_free (exact_matches, TRUE);
    }
  else
    res = CTK_CSS_SELECTOR_TREE_EMPTY_OFFSET;
  get_tree (array, tree_offset)->matches_offset = res;

  res = subdivide_infos (array, matched, tree_offset);
  get_tree (array, tree_offset)->previous_offset = res;

  res = subdivide_infos (array, remaining, parent_offset);
  get_tree (array, tree_offset)->sibling_offset = res;

  g_list_free (matched);
  g_list_free (remaining);
  g_hash_table_unref (ht);

  return tree_offset;
}

struct _CtkCssSelectorTreeBuilder {
  GList  *infos;
};

CtkCssSelectorTreeBuilder *
_ctk_css_selector_tree_builder_new (void)
{
  return g_new0 (CtkCssSelectorTreeBuilder, 1);
}

void
_ctk_css_selector_tree_builder_free  (CtkCssSelectorTreeBuilder *builder)
{
  g_list_free_full (builder->infos, g_free);
  g_free (builder);
}

void
_ctk_css_selector_tree_builder_add (CtkCssSelectorTreeBuilder *builder,
				    CtkCssSelector            *selectors,
				    CtkCssSelectorTree       **selector_match,
				    gpointer                   match)
{
  CtkCssSelectorRuleSetInfo *info = g_new0 (CtkCssSelectorRuleSetInfo, 1);

  info->match = match;
  info->current_selector = selectors;
  info->selector_match = selector_match;
  builder->infos = g_list_prepend (builder->infos, info);
}

/* Convert all offsets to node-relative */
static void
fixup_offsets (CtkCssSelectorTree *tree, guint8 *data)
{
  while (tree != NULL)
    {
      if (tree->parent_offset != CTK_CSS_SELECTOR_TREE_EMPTY_OFFSET)
	tree->parent_offset -= ((guint8 *)tree - data);

      if (tree->previous_offset != CTK_CSS_SELECTOR_TREE_EMPTY_OFFSET)
	tree->previous_offset -= ((guint8 *)tree - data);

      if (tree->sibling_offset != CTK_CSS_SELECTOR_TREE_EMPTY_OFFSET)
	tree->sibling_offset -= ((guint8 *)tree - data);

      if (tree->matches_offset != CTK_CSS_SELECTOR_TREE_EMPTY_OFFSET)
	tree->matches_offset -= ((guint8 *)tree - data);

      fixup_offsets ((CtkCssSelectorTree *)ctk_css_selector_tree_get_previous (tree), data);

      tree = (CtkCssSelectorTree *)ctk_css_selector_tree_get_sibling (tree);
    }
}

CtkCssSelectorTree *
_ctk_css_selector_tree_builder_build (CtkCssSelectorTreeBuilder *builder)
{
  CtkCssSelectorTree *tree;
  GByteArray *array;
  guint8 *data;
  guint len;
  GList *l;
  CtkCssSelectorRuleSetInfo *info;

  array = g_byte_array_new ();
  subdivide_infos (array, builder->infos, CTK_CSS_SELECTOR_TREE_EMPTY_OFFSET);

  len = array->len;
  data = g_byte_array_free (array, FALSE);

  /* shrink to final size */
  data = g_realloc (data, len);

  tree = (CtkCssSelectorTree *)data;

  fixup_offsets (tree, data);

  /* Convert offsets to final pointers */
  for (l = builder->infos; l != NULL; l = l->next)
    {
      info = l->data;
      if (info->selector_match)
	*info->selector_match = (CtkCssSelectorTree *)(data + GPOINTER_TO_UINT (*info->selector_match));
    }

#ifdef PRINT_TREE
  {
    GString *s = g_string_new ("");
    _ctk_css_selector_tree_print (tree, s, "");
    g_print ("%s", s->str);
    g_string_free (s, TRUE);
  }
#endif

  return tree;
}
