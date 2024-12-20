/* CTK - The GIMP Toolkit
 * Copyright (C) 2000 Red Hat Software
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Owen Taylor <otaylor@redhat.com>
 *
 */

#include "config.h"
#include <string.h>

#include "ctk/ctk.h"
#include "cdk/cdkkeysyms.h"

#include "ctk/ctkimmodule.h"
#include "ctk/ctkintl.h"


GType type_cedilla = 0;

static void cedilla_class_init (CtkIMContextSimpleClass *class);
static void cedilla_init (CtkIMContextSimple *im_context);

static void
cedilla_register_type (GTypeModule *module)
{
  const GTypeInfo object_info =
  {
    .class_size = sizeof (CtkIMContextSimpleClass),
    .base_init = (GBaseInitFunc) NULL,
    .base_finalize = (GBaseFinalizeFunc) NULL,
    .class_init = (GClassInitFunc) cedilla_class_init,
    .instance_size = sizeof (CtkIMContextSimple),
    .n_preallocs = 0,
    .instance_init = (GInstanceInitFunc) cedilla_init,
  };

  type_cedilla = 
    g_type_module_register_type (module,
				 CTK_TYPE_IM_CONTEXT_SIMPLE,
				 "CtkIMContextCedillaTranslit",
				 &object_info, 0);
}

/* The difference between this and the default input method is the handling
 * of C+acute - this method produces C WITH CEDILLA rather than C WITH ACUTE.
 * For languages that use CCedilla and not acute, this is the preferred mapping,
 * and is particularly important for pt_BR, where the us-intl keyboard is
 * used extensively.
 */
static guint16 cedilla_compose_seqs[] = {
  CDK_KEY_dead_acute,	CDK_KEY_C,	0,	0,	0,	0x00C7,	/* LATIN_CAPITAL_LETTER_C_WITH_CEDILLA */
  CDK_KEY_dead_acute,	CDK_KEY_c,	0,	0,	0,	0x00E7,	/* LATIN_SMALL_LETTER_C_WITH_CEDILLA */
  CDK_KEY_Multi_key,	CDK_KEY_apostrophe,	CDK_KEY_C,  0,      0,      0x00C7, /* LATIN_CAPITAL_LETTER_C_WITH_CEDILLA */
  CDK_KEY_Multi_key,	CDK_KEY_apostrophe,	CDK_KEY_c,  0,      0,      0x00E7, /* LATIN_SMALL_LETTER_C_WITH_CEDILLA */
  CDK_KEY_Multi_key,	CDK_KEY_C,  CDK_KEY_apostrophe,	0,      0,      0x00C7, /* LATIN_CAPITAL_LETTER_C_WITH_CEDILLA */
  CDK_KEY_Multi_key,	CDK_KEY_c,  CDK_KEY_apostrophe,	0,      0,      0x00E7, /* LATIN_SMALL_LETTER_C_WITH_CEDILLA */
};

static void
cedilla_class_init (CtkIMContextSimpleClass *class G_GNUC_UNUSED)
{
}

static void
cedilla_init (CtkIMContextSimple *im_context)
{
  ctk_im_context_simple_add_table (im_context,
				   cedilla_compose_seqs,
				   4,
				   G_N_ELEMENTS (cedilla_compose_seqs) / (4 + 2));
}

static const CtkIMContextInfo cedilla_info = { 
  "cedilla",		           /* ID */
  NC_("input method menu", "Cedilla"), /* Human readable name */
  GETTEXT_PACKAGE,		   /* Translation domain */
  CTK_LOCALEDIR,		   /* Dir for bindtextdomain */
  "az:ca:co:fr:gv:oc:pt:sq:tr:wa"  /* Languages for which this module is the default */
};

static const CtkIMContextInfo *info_list[] = {
  &cedilla_info
};

#ifndef INCLUDE_IM_cedilla
#define MODULE_ENTRY(type, function) G_MODULE_EXPORT type im_module_ ## function
#else
#define MODULE_ENTRY(type, function) type _ctk_immodule_cedilla_ ## function
#endif

MODULE_ENTRY (void, init) (GTypeModule *module)
{
  cedilla_register_type (module);
}

MODULE_ENTRY (void, exit) (void)
{
}

MODULE_ENTRY (void, list) (const CtkIMContextInfo ***contexts,
			   int                      *n_contexts)
{
  *contexts = info_list;
  *n_contexts = G_N_ELEMENTS (info_list);
}

MODULE_ENTRY (CtkIMContext *, create) (const gchar *context_id)
{
  if (strcmp (context_id, "cedilla") == 0)
    return g_object_new (type_cedilla, NULL);
  else
    return NULL;
}
