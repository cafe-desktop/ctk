/*
 * Copyright © 2014 Benjamin Otte <otte@gnome.org>
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

#include "ctkcssnodedeclarationprivate.h"
#include "ctkwidgetpathprivate.h"

#include <string.h>

typedef struct _CtkRegion CtkRegion;

struct _CtkRegion
{
  GQuark class_quark;
  CtkRegionFlags flags;
};

struct _CtkCssNodeDeclaration {
  guint refcount;
  CtkJunctionSides junction_sides;
  GType type;
  const /* interned */ char *name;
  const /* interned */ char *id;
  CtkStateFlags state;
  guint n_classes;
  guint n_regions;
  /* GQuark classes[n_classes]; */
  /* CtkRegion region[n_regions]; */
};

static inline GQuark *
get_classes (const CtkCssNodeDeclaration *decl)
{
  return (GQuark *) (decl + 1);
}

static inline CtkRegion *
get_regions (const CtkCssNodeDeclaration *decl)
{
  return (CtkRegion *) (get_classes (decl) + decl->n_classes);
}

static inline gsize
sizeof_node (guint n_classes,
             guint n_regions)
{
  return sizeof (CtkCssNodeDeclaration)
       + sizeof (GQuark) * n_classes
       + sizeof (CtkRegion) * n_regions;
}

static inline gsize
sizeof_this_node (CtkCssNodeDeclaration *decl)
{
  return sizeof_node (decl->n_classes, decl->n_regions);
}

static void
ctk_css_node_declaration_make_writable (CtkCssNodeDeclaration **decl)
{
  if ((*decl)->refcount == 1)
    return;

  (*decl)->refcount--;

  *decl = g_memdup2 (*decl, sizeof_this_node (*decl));
  (*decl)->refcount = 1;
}

static void
ctk_css_node_declaration_make_writable_resize (CtkCssNodeDeclaration **decl,
                                               gsize                   offset,
                                               gsize                   bytes_added,
                                               gsize                   bytes_removed)
{
  gsize old_size = sizeof_this_node (*decl);
  gsize new_size = old_size + bytes_added - bytes_removed;

  if ((*decl)->refcount == 1)
    {
      if (bytes_removed > 0 && old_size - offset - bytes_removed > 0)
        memmove (((char *) *decl) + offset, ((char *) *decl) + offset + bytes_removed, old_size - offset - bytes_removed);
      *decl = g_realloc (*decl, new_size);
      if (bytes_added > 0 && old_size - offset > 0)
        memmove (((char *) *decl) + offset + bytes_added, ((char *) *decl) + offset, old_size - offset);
    }
  else
    {
      CtkCssNodeDeclaration *old = *decl;

      old->refcount--;
  
      *decl = g_malloc (new_size);
      memcpy (*decl, old, offset);
      if (old_size - offset - bytes_removed > 0)
        memcpy (((char *) *decl) + offset + bytes_added, ((char *) old) + offset + bytes_removed, old_size - offset - bytes_removed);
      (*decl)->refcount = 1;
    }
}

CtkCssNodeDeclaration *
ctk_css_node_declaration_new (void)
{
  static CtkCssNodeDeclaration empty = {
    1, /* need to own a ref ourselves so the copy-on-write path kicks in when people change things */
    0,
    0,
    NULL,
    NULL,
    0,
    0,
    0
  };

  return ctk_css_node_declaration_ref (&empty);
}

CtkCssNodeDeclaration *
ctk_css_node_declaration_ref (CtkCssNodeDeclaration *decl)
{
  decl->refcount++;

  return decl;
}

void
ctk_css_node_declaration_unref (CtkCssNodeDeclaration *decl)
{
  decl->refcount--;
  if (decl->refcount > 0)
    return;

  g_free (decl);
}

gboolean
ctk_css_node_declaration_set_junction_sides (CtkCssNodeDeclaration **decl,
                                             CtkJunctionSides        junction_sides)
{
  if ((*decl)->junction_sides == junction_sides)
    return FALSE;
  
  ctk_css_node_declaration_make_writable (decl);
  (*decl)->junction_sides = junction_sides;

  return TRUE;
}

CtkJunctionSides
ctk_css_node_declaration_get_junction_sides (const CtkCssNodeDeclaration *decl)
{
  return decl->junction_sides;
}

gboolean
ctk_css_node_declaration_set_type (CtkCssNodeDeclaration **decl,
                                   GType                   type)
{
  if ((*decl)->type == type)
    return FALSE;

  ctk_css_node_declaration_make_writable (decl);
  (*decl)->type = type;

  return TRUE;
}

GType
ctk_css_node_declaration_get_type (const CtkCssNodeDeclaration *decl)
{
  return decl->type;
}

gboolean
ctk_css_node_declaration_set_name (CtkCssNodeDeclaration   **decl,
                                   /*interned*/ const char  *name)
{
  if ((*decl)->name == name)
    return FALSE;

  ctk_css_node_declaration_make_writable (decl);
  (*decl)->name = name;

  return TRUE;
}

/*interned*/ const char *
ctk_css_node_declaration_get_name (const CtkCssNodeDeclaration *decl)
{
  return decl->name;
}

gboolean
ctk_css_node_declaration_set_id (CtkCssNodeDeclaration **decl,
                                 const char             *id)
{
  id = g_intern_string (id);

  if ((*decl)->id == id)
    return FALSE;

  ctk_css_node_declaration_make_writable (decl);
  (*decl)->id = id;

  return TRUE;
}

const char *
ctk_css_node_declaration_get_id (const CtkCssNodeDeclaration *decl)
{
  return decl->id;
}

gboolean
ctk_css_node_declaration_set_state (CtkCssNodeDeclaration **decl,
                                    CtkStateFlags           state)
{
  if ((*decl)->state == state)
    return FALSE;
  
  ctk_css_node_declaration_make_writable (decl);
  (*decl)->state = state;

  return TRUE;
}

CtkStateFlags
ctk_css_node_declaration_get_state (const CtkCssNodeDeclaration *decl)
{
  return decl->state;
}

static gboolean
find_class (const CtkCssNodeDeclaration *decl,
            GQuark                       class_quark,
            guint                       *position)
{
  gint min, max, mid;
  gboolean found = FALSE;
  GQuark *classes;
  guint pos;

  *position = 0;

  if (decl->n_classes == 0)
    return FALSE;

  min = 0;
  max = decl->n_classes - 1;
  classes = get_classes (decl);

  do
    {
      GQuark item;

      mid = (min + max) / 2;
      item = classes[mid];

      if (class_quark == item)
        {
          found = TRUE;
          pos = mid;
          break;
        }
      else if (class_quark > item)
        min = pos = mid + 1;
      else
        {
          max = mid - 1;
          pos = mid;
        }
    }
  while (min <= max);

  *position = pos;

  return found;
}

gboolean
ctk_css_node_declaration_add_class (CtkCssNodeDeclaration **decl,
                                    GQuark                  class_quark)
{
  guint pos;

  if (find_class (*decl, class_quark, &pos))
    return FALSE;

  ctk_css_node_declaration_make_writable_resize (decl,
                                                 (char *) &get_classes (*decl)[pos] - (char *) *decl,
                                                 sizeof (GQuark),
                                                 0);
  (*decl)->n_classes++;
  get_classes(*decl)[pos] = class_quark;

  return TRUE;
}

gboolean
ctk_css_node_declaration_remove_class (CtkCssNodeDeclaration **decl,
                                       GQuark                  class_quark)
{
  guint pos;

  if (!find_class (*decl, class_quark, &pos))
    return FALSE;

  ctk_css_node_declaration_make_writable_resize (decl,
                                                 (char *) &get_classes (*decl)[pos] - (char *) *decl,
                                                 0,
                                                 sizeof (GQuark));
  (*decl)->n_classes--;

  return TRUE;
}

gboolean
ctk_css_node_declaration_clear_classes (CtkCssNodeDeclaration **decl)
{
  if ((*decl)->n_classes == 0)
    return FALSE;

  ctk_css_node_declaration_make_writable_resize (decl,
                                                 (char *) get_classes (*decl) - (char *) *decl,
                                                 0,
                                                 sizeof (GQuark) * (*decl)->n_classes);
  (*decl)->n_classes = 0;

  return TRUE;
}

gboolean
ctk_css_node_declaration_has_class (const CtkCssNodeDeclaration *decl,
                                    GQuark                       class_quark)
{
  guint pos;
  GQuark *classes = get_classes (decl);

  switch (decl->n_classes)
    {
    case 3:
      if (classes[2] == class_quark)
        return TRUE;

    case 2:
      if (classes[1] == class_quark)
        return TRUE;

    case 1:
      if (classes[0] == class_quark)
        return TRUE;

    case 0:
      return FALSE;

    default:
      return find_class (decl, class_quark, &pos);
    }
}

const GQuark *
ctk_css_node_declaration_get_classes (const CtkCssNodeDeclaration *decl,
                                      guint                       *n_classes)
{
  *n_classes = decl->n_classes;

  return get_classes (decl);
}

static gboolean
find_region (const CtkCssNodeDeclaration *decl,
             GQuark                       region_quark,
             guint                       *position)
{
  gint min, max, mid;
  gboolean found = FALSE;
  CtkRegion *regions;
  guint pos;

  if (position)
    *position = 0;

  if (decl->n_regions == 0)
    return FALSE;

  min = 0;
  max = decl->n_regions - 1;
  regions = get_regions (decl);

  do
    {
      GQuark item;

      mid = (min + max) / 2;
      item = regions[mid].class_quark;

      if (region_quark == item)
        {
          found = TRUE;
          pos = mid;
          break;
        }
      else if (region_quark > item)
        min = pos = mid + 1;
      else
        {
          max = mid - 1;
          pos = mid;
        }
    }
  while (min <= max);

  if (position)
    *position = pos;

  return found;
}

gboolean
ctk_css_node_declaration_add_region (CtkCssNodeDeclaration **decl,
                                     GQuark                  region_quark,
                                     CtkRegionFlags          flags)
{
  CtkRegion *regions;
  guint pos;

  if (find_region (*decl, region_quark, &pos))
    return FALSE;

  ctk_css_node_declaration_make_writable_resize (decl,
                                                 (char *) &get_regions (*decl)[pos] - (char *) *decl,
                                                 sizeof (CtkRegion),
                                                 0);
  (*decl)->n_regions++;
  regions = get_regions(*decl);
  regions[pos].class_quark = region_quark;
  regions[pos].flags = flags;

  return TRUE;
}

gboolean
ctk_css_node_declaration_remove_region (CtkCssNodeDeclaration **decl,
                                        GQuark                  region_quark)
{
  guint pos;

  if (!find_region (*decl, region_quark, &pos))
    return FALSE;

  ctk_css_node_declaration_make_writable_resize (decl,
                                                 (char *) &get_regions (*decl)[pos] - (char *) *decl,
                                                 0,
                                                 sizeof (CtkRegion));
  (*decl)->n_regions--;

  return TRUE;
}

gboolean
ctk_css_node_declaration_clear_regions (CtkCssNodeDeclaration **decl)
{
  if ((*decl)->n_regions == 0)
    return FALSE;

  ctk_css_node_declaration_make_writable_resize (decl,
                                                 (char *) get_regions (*decl) - (char *) *decl,
                                                 0,
                                                 sizeof (CtkRegion) * (*decl)->n_regions);
  (*decl)->n_regions = 0;

  return TRUE;
}

gboolean
ctk_css_node_declaration_has_region (const CtkCssNodeDeclaration  *decl,
                                     GQuark                        region_quark,
                                     CtkRegionFlags               *flags_return)
{
  guint pos;

  if (!find_region (decl, region_quark, &pos))
    {
      if (flags_return)
        *flags_return = 0;
      return FALSE;
    }

  if (flags_return)
    *flags_return = get_regions (decl)[pos].flags;

  return TRUE;
}

GList *
ctk_css_node_declaration_list_regions (const CtkCssNodeDeclaration *decl)
{
  CtkRegion *regions;
  GList *result;
  guint i;

  regions = get_regions (decl);
  result = NULL;

  for (i = 0; i < decl->n_regions; i++)
    {
      result = g_list_prepend (result, GUINT_TO_POINTER (regions[i].class_quark));
    }

  return result;
}

guint
ctk_css_node_declaration_hash (gconstpointer elem)
{
  const CtkCssNodeDeclaration *decl = elem;
  GQuark *classes;
  CtkRegion *regions;
  guint hash, i;
  
  hash = (guint) decl->type;
  hash ^= GPOINTER_TO_UINT (decl->name);
  hash <<= 5;
  hash ^= GPOINTER_TO_UINT (decl->id);

  classes = get_classes (decl);
  for (i = 0; i < decl->n_classes; i++)
    {
      hash <<= 5;
      hash += classes[i];
    }

  regions = get_regions (decl);
  for (i = 0; i < decl->n_regions; i++)
    {
      hash <<= 5;
      hash += regions[i].class_quark;
      hash += regions[i].flags;
    }

  hash ^= ((guint) decl->junction_sides) << (sizeof (guint) * 8 - 5);
  hash ^= decl->state;

  return hash;
}

gboolean
ctk_css_node_declaration_equal (gconstpointer elem1,
                                gconstpointer elem2)
{
  const CtkCssNodeDeclaration *decl1 = elem1;
  const CtkCssNodeDeclaration *decl2 = elem2;
  GQuark *classes1, *classes2;
  CtkRegion *regions1, *regions2;
  guint i;

  if (decl1 == decl2)
    return TRUE;

  if (decl1->type != decl2->type)
    return FALSE;

  if (decl1->name != decl2->name)
    return FALSE;

  if (decl1->state != decl2->state)
    return FALSE;

  if (decl1->id != decl2->id)
    return FALSE;

  if (decl1->n_classes != decl2->n_classes)
    return FALSE;

  classes1 = get_classes (decl1);
  classes2 = get_classes (decl2);
  for (i = 0; i < decl1->n_classes; i++)
    {
      if (classes1[i] != classes2[i])
        return FALSE;
    }

  if (decl1->n_regions != decl2->n_regions)
    return FALSE;

  regions1 = get_regions (decl1);
  regions2 = get_regions (decl2);
  for (i = 0; i < decl1->n_regions; i++)
    {
      if (regions1[i].class_quark != regions2[i].class_quark ||
          regions1[i].flags != regions2[i].flags)
        return FALSE;
    }

  if (decl1->junction_sides != decl2->junction_sides)
    return FALSE;

  return TRUE;
}

void
ctk_css_node_declaration_add_to_widget_path (const CtkCssNodeDeclaration *decl,
                                             CtkWidgetPath               *path,
                                             guint                        pos)
{
  GQuark *classes;
  CtkRegion *regions;
  guint i;

  /* Set name and id */
  ctk_widget_path_iter_set_object_name (path, pos, decl->name);
  if (decl->id)
    ctk_widget_path_iter_set_name (path, pos, decl->id);

  /* Set widget regions */
  regions = get_regions (decl);
  for (i = 0; i < decl->n_regions; i++)
    {
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      ctk_widget_path_iter_add_region (path, pos,
                                       g_quark_to_string (regions[i].class_quark),
                                       regions[i].flags);
G_GNUC_END_IGNORE_DEPRECATIONS
    }

  /* Set widget classes */
  classes = get_classes (decl);
  for (i = 0; i < decl->n_classes; i++)
    {
      ctk_widget_path_iter_add_qclass (path, pos, classes[i]);
    }

  /* Set widget state */
  ctk_widget_path_iter_set_state (path, pos, decl->state);
}

/* Append the declaration to the string, in selector format */
void
ctk_css_node_declaration_print (const CtkCssNodeDeclaration *decl,
                                GString                     *string)
{
  static const char *state_names[] = {
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
  const GQuark *classes;
  guint i;

  if (decl->name)
    g_string_append (string, decl->name);
  else
    g_string_append (string, g_type_name (decl->type));

  if (decl->id)
    {
      g_string_append_c (string, '#');
      g_string_append (string, decl->id);
    }

  classes = get_classes (decl);
  for (i = 0; i < decl->n_classes; i++)
    {
      g_string_append_c (string, '.');
      g_string_append (string, g_quark_to_string (classes[i]));
    }

  for (i = 0; i < G_N_ELEMENTS (state_names); i++)
    {
      if (decl->state & (1 << i))
        {
          g_string_append_c (string, ':');
          g_string_append (string, state_names[i]);
        }
    }
}
