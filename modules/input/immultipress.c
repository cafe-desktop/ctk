/* Copyright (C) 2006 Openismus GmbH
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

#include "ctkimcontextmultipress.h"
#include <ctk/ctkimmodule.h> /* For CtkIMContextInfo */
#include <glib/gi18n.h>
#include <string.h> /* For strcmp() */

#define CONTEXT_ID "multipress"
 
/** NOTE: Change the default language from "" to "*" to enable this input method by default for all locales.
 */
static const CtkIMContextInfo info = { 
  CONTEXT_ID,		   /* ID */
  NC_("input method menu", "Multipress"),     /* Human readable name */
  GETTEXT_PACKAGE,	   /* Translation domain. Defined in configure.ac */
  MULTIPRESS_LOCALEDIR,	   /* Dir for bindtextdomain (not strictly needed for "ctk+"). Defined in the Makefile.am */
  ""			   /* Languages for which this module is the default */
};

static const CtkIMContextInfo *info_list[] = {
  &info
};

#ifndef INCLUDE_IM_multipress
#define MODULE_ENTRY(type, function) G_MODULE_EXPORT type im_module_ ## function
#else
#define MODULE_ENTRY(type, function) type _ctk_immodule_multipress_ ## function
#endif

MODULE_ENTRY (void, init) (GTypeModule *module)
{
  ctk_im_context_multipress_register_type(module);
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
  if (strcmp (context_id, CONTEXT_ID) == 0)
  {
    CtkIMContext* imcontext = CTK_IM_CONTEXT(g_object_new (CTK_TYPE_IM_CONTEXT_MULTIPRESS, NULL));
    return imcontext;
  }
  else
    return NULL;
}
