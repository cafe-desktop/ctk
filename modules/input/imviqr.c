/* CTK - The GIMP Toolkit
 * Copyright (C) 2000 Red Hat Software
 * Copyright (C) 2000 SuSE Linux Ltd
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
 * Original author: Owen Taylor <otaylor@redhat.com>
 * 
 * Modified for VIQR - Robert Brady <robert@suse.co.uk>
 *
 */

#include "config.h"
#include <string.h>

#include "ctk/ctk.h"
#include "cdk/cdkkeysyms.h"

#include "ctk/ctkimmodule.h"
#include "ctk/ctkintl.h"

GType type_viqr_translit = 0;

static void viqr_class_init (CtkIMContextSimpleClass *class);
static void viqr_init (CtkIMContextSimple *im_context);

static void
viqr_register_type (GTypeModule *module)
{
  const GTypeInfo object_info =
  {
    .class_size = sizeof (CtkIMContextSimpleClass),
    .base_init = (GBaseInitFunc) NULL,
    .base_finalize = (GBaseFinalizeFunc) NULL,
    .class_init = (GClassInitFunc) viqr_class_init,
    .instance_size = sizeof (CtkIMContextSimple),
    .n_preallocs = 0,
    .instance_init = (GInstanceInitFunc) viqr_init,
  };

  type_viqr_translit = 
    g_type_module_register_type (module,
				 CTK_TYPE_IM_CONTEXT_SIMPLE,
				 "CtkIMContextViqr",
				 &object_info, 0);
}

static guint16 viqr_compose_seqs[] = {
  CDK_KEY_A,                   0,                0, 0, 0, 'A',
  CDK_KEY_A,                   CDK_KEY_apostrophe,   0, 0, 0, 0xc1,
  CDK_KEY_A,  CDK_KEY_parenleft,   0,                0, 0,    0x102,
  CDK_KEY_A,  CDK_KEY_parenleft,   CDK_KEY_apostrophe,   0, 0,    0x1eae,
  CDK_KEY_A,  CDK_KEY_parenleft,   CDK_KEY_period,       0, 0,    0x1eb6,
  CDK_KEY_A,  CDK_KEY_parenleft,   CDK_KEY_question,     0, 0,    0x1eb2,
  CDK_KEY_A,  CDK_KEY_parenleft,   CDK_KEY_grave,        0, 0,    0x1eb0,
  CDK_KEY_A,  CDK_KEY_parenleft,   CDK_KEY_asciitilde,   0, 0,    0x1eb4,
  CDK_KEY_A,                   CDK_KEY_period,       0, 0, 0, 0x1ea0,
  CDK_KEY_A,                   CDK_KEY_question,     0, 0, 0, 0x1ea2,
  CDK_KEY_A,  CDK_KEY_asciicircum, 0,                0, 0,    0xc2,
  CDK_KEY_A,  CDK_KEY_asciicircum, CDK_KEY_apostrophe,   0, 0,    0x1ea4,
  CDK_KEY_A,  CDK_KEY_asciicircum, CDK_KEY_period,       0, 0,    0x1eac,
  CDK_KEY_A,  CDK_KEY_asciicircum, CDK_KEY_question,     0, 0,    0x1ea8,
  CDK_KEY_A,  CDK_KEY_asciicircum, CDK_KEY_grave,        0, 0,    0x1ea6,
  CDK_KEY_A,  CDK_KEY_asciicircum, CDK_KEY_asciitilde,   0, 0,    0x1eaa,
  CDK_KEY_A,                   CDK_KEY_grave,        0, 0, 0, 0xc0,
  CDK_KEY_A,                   CDK_KEY_asciitilde,   0, 0, 0, 0xc3,
  CDK_KEY_D,                   0,                0, 0, 0, 'D',
  CDK_KEY_D,                   CDK_KEY_D,            0, 0, 0, 0x110,
  CDK_KEY_D,                   CDK_KEY_d,            0, 0, 0, 0x110,
  CDK_KEY_E,                   0,                0, 0, 0, 'E',
  CDK_KEY_E,                   CDK_KEY_apostrophe,   0, 0, 0, 0xc9,
  CDK_KEY_E,                   CDK_KEY_period,       0, 0, 0, 0x1eb8,
  CDK_KEY_E,                   CDK_KEY_question,     0, 0, 0, 0x1eba,
  CDK_KEY_E,  CDK_KEY_asciicircum, 0,                0, 0,    0xca,
  CDK_KEY_E,  CDK_KEY_asciicircum, CDK_KEY_apostrophe,   0, 0,    0x1ebe,
  CDK_KEY_E,  CDK_KEY_asciicircum, CDK_KEY_period,       0, 0,    0x1ec6,
  CDK_KEY_E,  CDK_KEY_asciicircum, CDK_KEY_question,     0, 0,    0x1ec2,
  CDK_KEY_E,  CDK_KEY_asciicircum, CDK_KEY_grave,        0, 0,    0x1ec0,
  CDK_KEY_E,  CDK_KEY_asciicircum, CDK_KEY_asciitilde,   0, 0,    0x1ec4,
  CDK_KEY_E,                   CDK_KEY_grave,        0, 0, 0, 0xc8,
  CDK_KEY_E,                   CDK_KEY_asciitilde,   0, 0, 0, 0x1ebc,
  CDK_KEY_I,                   0,                0, 0, 0, 'I',
  CDK_KEY_I,                   CDK_KEY_apostrophe,   0, 0, 0, 0xcd,
  CDK_KEY_I,                   CDK_KEY_period,       0, 0, 0, 0x1eca,
  CDK_KEY_I,                   CDK_KEY_question,     0, 0, 0, 0x1ec8,
  CDK_KEY_I,                   CDK_KEY_grave,        0, 0, 0, 0xcc,
  CDK_KEY_I,                   CDK_KEY_asciitilde,   0, 0, 0, 0x128,
  CDK_KEY_O,                   0,                0, 0, 0, 'O',
  CDK_KEY_O,                   CDK_KEY_apostrophe,   0, 0, 0, 0xD3,
  CDK_KEY_O,  CDK_KEY_plus,        0,                0, 0,    0x1a0,
  CDK_KEY_O,  CDK_KEY_plus,        CDK_KEY_apostrophe,   0, 0,    0x1eda,
  CDK_KEY_O,  CDK_KEY_plus,        CDK_KEY_period,       0, 0,    0x1ee2,
  CDK_KEY_O,  CDK_KEY_plus,        CDK_KEY_question,     0, 0,    0x1ede,
  CDK_KEY_O,  CDK_KEY_plus,        CDK_KEY_grave,        0, 0,    0x1edc,
  CDK_KEY_O,  CDK_KEY_plus,        CDK_KEY_asciitilde,   0, 0,    0x1ee0,
  CDK_KEY_O,                   CDK_KEY_period,       0, 0, 0, 0x1ecc,
  CDK_KEY_O,                   CDK_KEY_question,     0, 0, 0, 0x1ece,
  CDK_KEY_O,  CDK_KEY_asciicircum, 0,                0, 0,    0xd4,
  CDK_KEY_O,  CDK_KEY_asciicircum, CDK_KEY_apostrophe,   0, 0,    0x1ed0,
  CDK_KEY_O,  CDK_KEY_asciicircum, CDK_KEY_period,       0, 0,    0x1ed8,
  CDK_KEY_O,  CDK_KEY_asciicircum, CDK_KEY_question,     0, 0,    0x1ed4,
  CDK_KEY_O,  CDK_KEY_asciicircum, CDK_KEY_grave,        0, 0,    0x1ed2,
  CDK_KEY_O,  CDK_KEY_asciicircum, CDK_KEY_asciitilde,   0, 0,    0x1ed6,
  CDK_KEY_O,                   CDK_KEY_grave,        0, 0, 0, 0xD2,
  CDK_KEY_O,                   CDK_KEY_asciitilde,   0, 0, 0, 0xD5,
  CDK_KEY_U,                   0,                0, 0, 0, 'U',
  CDK_KEY_U,                   CDK_KEY_apostrophe,   0, 0, 0, 0xDA,
  CDK_KEY_U,  CDK_KEY_plus,        0,                0, 0,    0x1af,
  CDK_KEY_U,  CDK_KEY_plus,        CDK_KEY_apostrophe,   0, 0,    0x1ee8,
  CDK_KEY_U,  CDK_KEY_plus,        CDK_KEY_period,       0, 0,    0x1ef0,
  CDK_KEY_U,  CDK_KEY_plus,        CDK_KEY_question,     0, 0,    0x1eec,
  CDK_KEY_U,  CDK_KEY_plus,        CDK_KEY_grave,        0, 0,    0x1eea,
  CDK_KEY_U,  CDK_KEY_plus,        CDK_KEY_asciitilde,   0, 0,    0x1eee,
  CDK_KEY_U,                   CDK_KEY_period,       0, 0, 0, 0x1ee4,
  CDK_KEY_U,                   CDK_KEY_question,     0, 0, 0, 0x1ee6,
  CDK_KEY_U,                   CDK_KEY_grave,        0, 0, 0, 0xd9,
  CDK_KEY_U,                   CDK_KEY_asciitilde,   0, 0, 0, 0x168,
  CDK_KEY_Y,                   0,                0, 0, 0, 'Y',
  CDK_KEY_Y,                   CDK_KEY_apostrophe,   0, 0, 0, 0xdd,
  CDK_KEY_Y,                   CDK_KEY_period,       0, 0, 0, 0x1ef4,
  CDK_KEY_Y,                   CDK_KEY_question,     0, 0, 0, 0x1ef6,
  CDK_KEY_Y,                   CDK_KEY_grave,        0, 0, 0, 0x1ef2,
  CDK_KEY_Y,                   CDK_KEY_asciitilde,   0, 0, 0, 0x1ef8,
  /* Do we need anything else here? */
  CDK_KEY_backslash,           0,                0, 0, 0, 0,
  CDK_KEY_backslash,           CDK_KEY_apostrophe,   0, 0, 0, '\'',
  CDK_KEY_backslash,           CDK_KEY_parenleft,    0, 0, 0, '(',
  CDK_KEY_backslash,           CDK_KEY_plus,         0, 0, 0, '+',
  CDK_KEY_backslash,           CDK_KEY_period,       0, 0, 0, '.',
  CDK_KEY_backslash,           CDK_KEY_question,     0, 0, 0, '?',
  CDK_KEY_backslash,           CDK_KEY_D,            0, 0, 0, 'D',
  CDK_KEY_backslash,           CDK_KEY_backslash,    0, 0, 0, '\\',
  CDK_KEY_backslash,           CDK_KEY_asciicircum,  0, 0, 0, '^',
  CDK_KEY_backslash,           CDK_KEY_grave,        0, 0, 0, '`',
  CDK_KEY_backslash,           CDK_KEY_d,            0, 0, 0, 'd',
  CDK_KEY_backslash,           CDK_KEY_asciitilde,   0, 0, 0, '~',
  CDK_KEY_a,                   0,                0, 0, 0, 'a',
  CDK_KEY_a,                   CDK_KEY_apostrophe,   0, 0, 0, 0xe1,
  CDK_KEY_a, CDK_KEY_parenleft,    0,                0, 0,    0x103,
  CDK_KEY_a, CDK_KEY_parenleft,    CDK_KEY_apostrophe,   0, 0,    0x1eaf,
  CDK_KEY_a, CDK_KEY_parenleft,    CDK_KEY_period,       0, 0,    0x1eb7,
  CDK_KEY_a, CDK_KEY_parenleft,    CDK_KEY_question,     0, 0,    0x1eb3,
  CDK_KEY_a, CDK_KEY_parenleft,    CDK_KEY_grave,        0, 0,    0x1eb1,
  CDK_KEY_a, CDK_KEY_parenleft,    CDK_KEY_asciitilde,   0, 0,    0x1eb5,
  CDK_KEY_a,                   CDK_KEY_period,       0, 0, 0, 0x1ea1,
  CDK_KEY_a,                   CDK_KEY_question,     0, 0, 0, 0x1ea3,
  CDK_KEY_a, CDK_KEY_asciicircum,  0,                0, 0,    0xe2,
  CDK_KEY_a, CDK_KEY_asciicircum,  CDK_KEY_apostrophe,   0, 0,    0x1ea5,
  CDK_KEY_a, CDK_KEY_asciicircum,  CDK_KEY_period,       0, 0,    0x1ead,
  CDK_KEY_a, CDK_KEY_asciicircum,  CDK_KEY_question,     0, 0,    0x1ea9,
  CDK_KEY_a, CDK_KEY_asciicircum,  CDK_KEY_grave,        0, 0,    0x1ea7,
  CDK_KEY_a, CDK_KEY_asciicircum,  CDK_KEY_asciitilde,   0, 0,    0x1eab,
  CDK_KEY_a,                   CDK_KEY_grave,        0, 0, 0, 0xe0,
  CDK_KEY_a,                   CDK_KEY_asciitilde,   0, 0, 0, 0xe3,
  CDK_KEY_d,                   0,                0, 0, 0, 'd',
  CDK_KEY_d,                   CDK_KEY_d,            0, 0, 0, 0x111,
  CDK_KEY_e,                   0,                0, 0, 0, 'e',
  CDK_KEY_e,                   CDK_KEY_apostrophe,   0, 0, 0, 0xe9,
  CDK_KEY_e,                   CDK_KEY_period,       0, 0, 0, 0x1eb9,
  CDK_KEY_e,                   CDK_KEY_question,     0, 0, 0, 0x1ebb,
  CDK_KEY_e, CDK_KEY_asciicircum,  0,                0, 0,    0xea,
  CDK_KEY_e, CDK_KEY_asciicircum,  CDK_KEY_apostrophe,   0, 0,    0x1ebf,
  CDK_KEY_e, CDK_KEY_asciicircum,  CDK_KEY_period,       0, 0,    0x1ec7,
  CDK_KEY_e, CDK_KEY_asciicircum,  CDK_KEY_question,     0, 0,    0x1ec3,
  CDK_KEY_e, CDK_KEY_asciicircum,  CDK_KEY_grave,        0, 0,    0x1ec1,
  CDK_KEY_e, CDK_KEY_asciicircum,  CDK_KEY_asciitilde,   0, 0,    0x1ec5,
  CDK_KEY_e,                   CDK_KEY_grave,        0, 0, 0, 0xe8,
  CDK_KEY_e,                   CDK_KEY_asciitilde,   0, 0, 0, 0x1ebd,
  CDK_KEY_i,                   0,                0, 0, 0, 'i',
  CDK_KEY_i,                   CDK_KEY_apostrophe,   0, 0, 0, 0xed,
  CDK_KEY_i,                   CDK_KEY_period,       0, 0, 0, 0x1ecb,
  CDK_KEY_i,                   CDK_KEY_question,     0, 0, 0, 0x1ec9,
  CDK_KEY_i,                   CDK_KEY_grave,        0, 0, 0, 0xec,
  CDK_KEY_i,                   CDK_KEY_asciitilde,   0, 0, 0, 0x129,
  CDK_KEY_o,                   0,                0, 0, 0, 'o',
  CDK_KEY_o,                   CDK_KEY_apostrophe,   0, 0, 0, 0xF3,
  CDK_KEY_o,  CDK_KEY_plus,        0,                0, 0,    0x1a1,
  CDK_KEY_o,  CDK_KEY_plus,        CDK_KEY_apostrophe,   0, 0,    0x1edb,
  CDK_KEY_o,  CDK_KEY_plus,        CDK_KEY_period,       0, 0,    0x1ee3,
  CDK_KEY_o,  CDK_KEY_plus,        CDK_KEY_question,     0, 0,    0x1edf,
  CDK_KEY_o,  CDK_KEY_plus,        CDK_KEY_grave,        0, 0,    0x1edd,
  CDK_KEY_o,  CDK_KEY_plus,        CDK_KEY_asciitilde,   0, 0,    0x1ee1,
  CDK_KEY_o,                   CDK_KEY_period,       0, 0, 0, 0x1ecd,
  CDK_KEY_o,                   CDK_KEY_question,     0, 0, 0, 0x1ecf,
  CDK_KEY_o,  CDK_KEY_asciicircum, 0,                0, 0,    0xf4,
  CDK_KEY_o,  CDK_KEY_asciicircum, CDK_KEY_apostrophe,   0, 0,    0x1ed1,
  CDK_KEY_o,  CDK_KEY_asciicircum, CDK_KEY_period,       0, 0,    0x1ed9,
  CDK_KEY_o,  CDK_KEY_asciicircum, CDK_KEY_question,     0, 0,    0x1ed5,
  CDK_KEY_o,  CDK_KEY_asciicircum, CDK_KEY_grave,        0, 0,    0x1ed3,
  CDK_KEY_o,  CDK_KEY_asciicircum, CDK_KEY_asciitilde,   0, 0,    0x1ed7,
  CDK_KEY_o,                   CDK_KEY_grave,        0, 0, 0, 0xF2,
  CDK_KEY_o,                   CDK_KEY_asciitilde,   0, 0, 0, 0xF5,
  CDK_KEY_u,                   0,                0, 0, 0, 'u',
  CDK_KEY_u,                   CDK_KEY_apostrophe,   0, 0, 0, 0xFA,
  CDK_KEY_u,  CDK_KEY_plus,        0,                0, 0,    0x1b0,
  CDK_KEY_u,  CDK_KEY_plus,        CDK_KEY_apostrophe,   0, 0,    0x1ee9,
  CDK_KEY_u,  CDK_KEY_plus,        CDK_KEY_period,       0, 0,    0x1ef1,
  CDK_KEY_u,  CDK_KEY_plus,        CDK_KEY_question,     0, 0,    0x1eed,
  CDK_KEY_u,  CDK_KEY_plus,        CDK_KEY_grave,        0, 0,    0x1eeb,
  CDK_KEY_u,  CDK_KEY_plus,        CDK_KEY_asciitilde,   0, 0,    0x1eef,
  CDK_KEY_u,                   CDK_KEY_period,       0, 0, 0, 0x1ee5,
  CDK_KEY_u,                   CDK_KEY_question,     0, 0, 0, 0x1ee7,
  CDK_KEY_u,                   CDK_KEY_grave,        0, 0, 0, 0xf9,
  CDK_KEY_u,                   CDK_KEY_asciitilde,   0, 0, 0, 0x169,
  CDK_KEY_y,                   0,                0, 0, 0, 'y',
  CDK_KEY_y,                   CDK_KEY_apostrophe,   0, 0, 0, 0xfd,
  CDK_KEY_y,                   CDK_KEY_period,       0, 0, 0, 0x1ef5,
  CDK_KEY_y,                   CDK_KEY_question,     0, 0, 0, 0x1ef7,
  CDK_KEY_y,                   CDK_KEY_grave,        0, 0, 0, 0x1ef3,
  CDK_KEY_y,                   CDK_KEY_asciitilde,   0, 0, 0, 0x1ef9,
};

static void
viqr_class_init (CtkIMContextSimpleClass *class G_GNUC_UNUSED)
{
}

static void
viqr_init (CtkIMContextSimple *im_context)
{
  ctk_im_context_simple_add_table (im_context,
				   viqr_compose_seqs,
				   4,
				   G_N_ELEMENTS (viqr_compose_seqs) / (4 + 2));
}

static const CtkIMContextInfo viqr_info = { 
  "viqr",		   /* ID */
  NC_("input method menu", "Vietnamese (VIQR)"), /* Human readable name */
  GETTEXT_PACKAGE,	   /* Translation domain */
   CTK_LOCALEDIR,	   /* Dir for bindtextdomain (not strictly needed for "ctk+") */
  ""			   /* Languages for which this module is the default */
};

static const CtkIMContextInfo *info_list[] = {
  &viqr_info
};

#ifndef INCLUDE_IM_viqr
#define MODULE_ENTRY(type, function) G_MODULE_EXPORT type im_module_ ## function
#else
#define MODULE_ENTRY(type, function) type _ctk_immodule_viqr_ ## function
#endif

MODULE_ENTRY (void, init) (GTypeModule *module)
{
  viqr_register_type (module);
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
  if (strcmp (context_id, "viqr") == 0)
    return g_object_new (type_viqr_translit, NULL);
  else
    return NULL;
}
