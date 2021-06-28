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

GType type_ipa = 0;

static void ipa_class_init (CtkIMContextSimpleClass *class);
static void ipa_init (CtkIMContextSimple *im_context);

static void
ipa_register_type (GTypeModule *module)
{
  const GTypeInfo object_info =
  {
    sizeof (CtkIMContextSimpleClass),
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) ipa_class_init,
    NULL,           /* class_finalize */
    NULL,           /* class_data */
    sizeof (CtkIMContextSimple),
    0,
    (GInstanceInitFunc) ipa_init,
  };

  type_ipa = 
    g_type_module_register_type (module,
				 CTK_TYPE_IM_CONTEXT_SIMPLE,
				 "CtkIMContextIpa",
				 &object_info, 0);
}

/* The sequences here match the sequences used in the emacs quail
 * mode cryllic-translit; they allow entering all characters
 * in iso-8859-5
 */
static guint16 ipa_compose_seqs[] = {
  CDK_KEY_ampersand, 0,           0,      0,      0,      0x263, 	/* LATIN SMALL LETTER GAMMA */
  CDK_KEY_apostrophe, 0,          0,      0,      0,      0x2C8, 	/* MODIFIER LETTER VERTICAL LINE */
  CDK_KEY_slash,  CDK_KEY_apostrophe, 0,      0,      0,      0x2CA, 	/* MODIFIER LETTER ACUTE ACCENT */
  CDK_KEY_slash,  CDK_KEY_slash,      0,      0,      0,      0x02F, 	/* SOLIDUS */
  CDK_KEY_slash,  CDK_KEY_3,          0,      0,      0,      0x25B, 	/* LATIN SMALL LETTER OPEN E */
  CDK_KEY_slash,  CDK_KEY_A,          0,      0,      0,      0x252, 	/* LATIN LETTER TURNED ALPHA */
  CDK_KEY_slash,  CDK_KEY_R,          0,      0,      0,      0x281, 	/* LATIN LETTER SMALL CAPITAL INVERTED R */
  CDK_KEY_slash,  CDK_KEY_a,          0,      0,      0,      0x250, 	/* LATIN SMALL LETTER TURNED A */
  CDK_KEY_slash,  CDK_KEY_c,          0,      0,      0,      0x254, 	/* LATIN SMALL LETTER OPEN O */
  CDK_KEY_slash,  CDK_KEY_e,          0,      0,      0,      0x259, 	/* LATIN SMALL LETTER SCHWA */
  CDK_KEY_slash,  CDK_KEY_h,          0,      0,      0,      0x265, 	/* LATIN SMALL LETTER TURNED H */
  CDK_KEY_slash,  CDK_KEY_m,          0,      0,      0,      0x26F, 	/* LATIN SMALL LETTER TURNED M */
  CDK_KEY_slash,  CDK_KEY_r,          0,      0,      0,      0x279, 	/* LATIN SMALL LETTER TURNED R */
  CDK_KEY_slash,  CDK_KEY_v,          0,      0,      0,      0x28C, 	/* LATIN SMALL LETTER TURNED V */
  CDK_KEY_slash,  CDK_KEY_w,          0,      0,      0,      0x28D, 	/* LATIN SMALL LETTER TURNED W */
  CDK_KEY_slash,  CDK_KEY_y,          0,      0,      0,      0x28E, 	/* LATIN SMALL LETTER TRUEND Y*/
  CDK_KEY_3,      0,              0,      0,      0,      0x292, 	/* LATIN SMALL LETTER EZH */
  CDK_KEY_colon,  0,              0,      0,      0,      0x2D0, 	/* MODIFIER LETTER TRIANGULAR COLON */
  CDK_KEY_A,      0,              0,      0,      0,      0x251, 	/* LATIN SMALL LETTER ALPHA */
  CDK_KEY_E,      0,              0,      0,      0,      0x25B, 	/* LATIN SMALL LETTER OPEN E */
  CDK_KEY_I,      0,              0,      0,      0,      0x26A, 	/* LATIN LETTER SMALL CAPITAL I */
  CDK_KEY_L,      0,              0,      0,      0,      0x29F, 	/* LATIN LETTER SMALL CAPITAL L */
  CDK_KEY_M,      0,              0,      0,      0,      0x28D, 	/* LATIN SMALL LETTER TURNED W */
  CDK_KEY_O,      0,              0,      0,      0,      0x04F, 	/* LATIN LETTER SMALL CAPITAL OE */
  CDK_KEY_O,      CDK_KEY_E,          0,      0,      0,      0x276, 	/* LATIN LETTER SMALL CAPITAL OE */
  CDK_KEY_R,      0,              0,      0,      0,      0x280, 	/* LATIN LETTER SMALL CAPITAL R */
  CDK_KEY_U,      0,              0,      0,      0,      0x28A, 	/* LATIN SMALL LETTER UPSILON */
  CDK_KEY_Y,      0,              0,      0,      0,      0x28F, 	/* LATIN LETTER SMALL CAPITAL Y */
  CDK_KEY_grave,  0,              0,      0,      0,      0x2CC, 	/* MODIFIER LETTER LOW VERTICAL LINE */
  CDK_KEY_a,      0,              0,      0,      0,      0x061, 	/* LATIN SMALL LETTER A */
  CDK_KEY_a,      CDK_KEY_e,          0,      0,      0,      0x0E6, 	/* LATIN SMALL LETTER AE */
  CDK_KEY_c,      0,              0,      0,      0,      0x063,    /* LATIN SMALL LETTER C */
  CDK_KEY_c,      CDK_KEY_comma,      0,      0,      0,      0x0E7,    /* LATIN SMALL LETTER C WITH CEDILLA */
  CDK_KEY_d,      0,              0,      0,      0,      0x064, 	/* LATIN SMALL LETTER E */
  CDK_KEY_d,      CDK_KEY_apostrophe, 0,      0,      0,      0x064, 	/* LATIN SMALL LETTER D */
  CDK_KEY_d,      CDK_KEY_h,          0,      0,      0,      0x0F0, 	/* LATIN SMALL LETTER ETH */
  CDK_KEY_e,      0,              0,      0,      0,      0x065, 	/* LATIN SMALL LETTER E */
  CDK_KEY_e,      CDK_KEY_minus,      0,      0,      0,      0x25A, 	/* LATIN SMALL LETTER SCHWA WITH HOOK */
  CDK_KEY_e,      CDK_KEY_bar,        0,      0,      0,      0x25A, 	/* LATIN SMALL LETTER SCHWA WITH HOOK */
  CDK_KEY_g,      0,              0,      0,      0,      0x067, 	/* LATIN SMALL LETTER G */
  CDK_KEY_g,      CDK_KEY_n,          0,      0,      0,      0x272, 	/* LATIN SMALL LETTER N WITH LEFT HOOK */
  CDK_KEY_i,      0,              0,      0,      0,      0x069, 	/* LATIN SMALL LETTER I */
  CDK_KEY_i,      CDK_KEY_minus,      0,      0,      0,      0x268, 	/* LATIN SMALL LETTER I WITH STROKE */
  CDK_KEY_n,      0,              0,      0,      0,      0x06e, 	/* LATIN SMALL LETTER N */
  CDK_KEY_n,      CDK_KEY_g,          0,      0,      0,      0x14B, 	/* LATIN SMALL LETTER ENG */
  CDK_KEY_o,      0,              0,      0,      0,      0x06f, 	/* LATIN SMALL LETTER O */
  CDK_KEY_o,      CDK_KEY_minus,      0,      0,      0,      0x275, 	/* LATIN LETTER BARRED O */
  CDK_KEY_o,      CDK_KEY_slash,      0,      0,      0,      0x0F8, 	/* LATIN SMALL LETTER O WITH STROKE */
  CDK_KEY_o,      CDK_KEY_e,          0,      0,      0,      0x153, 	/* LATIN SMALL LIGATURE OE */
  CDK_KEY_o,      CDK_KEY_bar,        0,      0,      0,      0x251, 	/* LATIN SMALL LETTER ALPHA */
  CDK_KEY_s,      0,              0,      0,      0,      0x073, 	/* LATIN SMALL LETTER_ESH */
  CDK_KEY_s,      CDK_KEY_h,          0,      0,      0,      0x283, 	/* LATIN SMALL LETTER_ESH */
  CDK_KEY_t,      0,              0,      0,      0,      0x074, 	/* LATIN SMALL LETTER T */
  CDK_KEY_t,      CDK_KEY_h,          0,      0,      0,      0x3B8, 	/* GREEK SMALL LETTER THETA */
  CDK_KEY_u,      0,              0,      0,      0,      0x075, 	/* LATIN SMALL LETTER U */
  CDK_KEY_u,      CDK_KEY_minus,      0,      0,      0,      0x289, 	/* LATIN LETTER U BAR */
  CDK_KEY_z,      0,              0,      0,      0,      0x07A, 	/* LATIN SMALL LETTER Z */
  CDK_KEY_z,      CDK_KEY_h,          0,      0,      0,      0x292, 	/* LATIN SMALL LETTER EZH */
  CDK_KEY_bar,    CDK_KEY_o,          0,      0,      0,      0x252, 	/* LATIN LETTER TURNED ALPHA */

  CDK_KEY_asciitilde, 0,          0,      0,      0,      0x303,    /* COMBINING TILDE */

};

static void
ipa_class_init (CtkIMContextSimpleClass *class)
{
}

static void
ipa_init (CtkIMContextSimple *im_context)
{
  ctk_im_context_simple_add_table (im_context,
				   ipa_compose_seqs,
				   4,
				   G_N_ELEMENTS (ipa_compose_seqs) / (4 + 2));
}

static const CtkIMContextInfo ipa_info = { 
  "ipa",		   /* ID */
  NC_("input method menu", "IPA"), /* Human readable name */
  GETTEXT_PACKAGE,		   /* Translation domain */
   CTK_LOCALEDIR,		   /* Dir for bindtextdomain (not strictly needed for "ctk+") */
  ""			           /* Languages for which this module is the default */
};

static const CtkIMContextInfo *info_list[] = {
  &ipa_info
};

#ifndef INCLUDE_IM_ipa
#define MODULE_ENTRY(type, function) G_MODULE_EXPORT type im_module_ ## function
#else
#define MODULE_ENTRY(type, function) type _ctk_immodule_ipa_ ## function
#endif

MODULE_ENTRY (void, init) (GTypeModule *module)
{
  ipa_register_type (module);
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
  if (strcmp (context_id, "ipa") == 0)
    return g_object_new (type_ipa, NULL);
  else
    return NULL;
}
