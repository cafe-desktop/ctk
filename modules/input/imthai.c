/* CTK - The GIMP Toolkit
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
 * Author: Theppitak Karoonboonyanan <thep@linux.thai.net>
 *
 */

#include "config.h"
#include <string.h>

#include <cdk/cdkkeysyms.h>

#include "ctk/ctkintl.h"
#include "ctk/ctkimmodule.h"
#include "ctkimcontextthai.h"

GType type_thai = 0;

static const CtkIMContextInfo thai_info = { 
  "thai",	   /* ID */
  NC_("input method menu", "Thai-Lao"),  /* Human readable name */
  GETTEXT_PACKAGE, /* Translation domain */
  CTK_LOCALEDIR,   /* Dir for bindtextdomain (not strictly needed for "ctk+") */
  "lo:th"	   /* Languages for which this module is the default */
};

static const CtkIMContextInfo *info_list[] = {
  &thai_info
};

#ifndef INCLUDE_IM_thai
#define MODULE_ENTRY(type, function) G_MODULE_EXPORT type im_module_ ## function
#else
#define MODULE_ENTRY(type, function) type _ctk_immodule_thai_ ## function
#endif

MODULE_ENTRY (void, init) (GTypeModule *module)
{
  ctk_im_context_thai_register_type (module);
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
  if (strcmp (context_id, "thai") == 0)
    return ctk_im_context_thai_new ();
  else
    return NULL;
}
