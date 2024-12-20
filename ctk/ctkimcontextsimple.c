/* CTK - The GIMP Toolkit
 * Copyright (C) 2000 Red Hat, Inc.
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

#include <cdk/cdk.h>

#ifdef CDK_WINDOWING_X11
#include <cdk/cdkx.h>
#endif
#ifdef CDK_WINDOWING_WAYLAND
#include <wayland/cdkwayland.h>
#endif
#ifdef CDK_WINDOWING_WIN32
#include <win32/cdkwin32.h>
#endif

#include <stdlib.h>
#include <string.h>

#include "ctkprivate.h"
#include "ctkaccelgroup.h"
#include "ctkimcontextsimple.h"
#include "ctksettings.h"
#include "ctkwidget.h"
#include "ctkdebug.h"
#include "ctkintl.h"
#include "ctkcomposetable.h"

#include "ctkimcontextsimpleprivate.h"
#include "ctkimcontextsimpleseqs.h"

/**
 * SECTION:ctkimcontextsimple
 * @Short_description: An input method context supporting table-based input methods
 * @Title: CtkIMContextSimple
 *
 * CtkIMContextSimple is a simple input method context supporting table-based
 * input methods. It has a built-in table of compose sequences that is derived
 * from the X11 Compose files.
 *
 * CtkIMContextSimple reads additional compose sequences from the first of the
 * following files that is found: ~/.config/ctk-3.0/Compose, ~/.XCompose,
 * /usr/share/X11/locale/$locale/Compose (for locales that have a nontrivial
 * Compose file). The syntax of these files is described in the Compose(5)
 * manual page.
 *
 * CtkIMContextSimple also supports numeric entry of Unicode characters
 * by typing Ctrl-Shift-u, followed by a hexadecimal Unicode codepoint.
 * For example, Ctrl-Shift-u 1 2 3 Enter yields U+0123 LATIN SMALL LETTER
 * G WITH CEDILLA, i.e. ģ.
 */

struct _CtkIMContextSimplePrivate
{
  guint16        compose_buffer[CTK_MAX_COMPOSE_LEN + 1];
  gunichar       tentative_match;
  gint           tentative_match_len;

  guint          in_hex_sequence : 1;
  guint          modifiers_dropped : 1;
};

/* From the values below, the value 30 means the number of different first keysyms
 * that exist in the Compose file (from Xorg). When running compose-parse.py without
 * parameters, you get the count that you can put here. Needed when updating the
 * ctkimcontextsimpleseqs.h header file (contains the compose sequences).
 */
const CtkComposeTableCompact ctk_compose_table_compact = {
  ctk_compose_seqs_compact,
  5,
  30,
  6
};

G_LOCK_DEFINE_STATIC (global_tables);
static GSList *global_tables;

static const guint16 ctk_compose_ignore[] = {
  CDK_KEY_Shift_L,
  CDK_KEY_Shift_R,
  CDK_KEY_Control_L,
  CDK_KEY_Control_R,
  CDK_KEY_Caps_Lock,
  CDK_KEY_Shift_Lock,
  CDK_KEY_Meta_L,
  CDK_KEY_Meta_R,
  CDK_KEY_Alt_L,
  CDK_KEY_Alt_R,
  CDK_KEY_Super_L,
  CDK_KEY_Super_R,
  CDK_KEY_Hyper_L,
  CDK_KEY_Hyper_R,
  CDK_KEY_Mode_switch,
  CDK_KEY_ISO_Level3_Shift
};

static void     ctk_im_context_simple_finalize           (GObject                  *obj);
static gboolean ctk_im_context_simple_filter_keypress    (CtkIMContext             *context,
							  CdkEventKey              *key);
static void     ctk_im_context_simple_reset              (CtkIMContext             *context);
static void     ctk_im_context_simple_get_preedit_string (CtkIMContext             *context,
							  gchar                   **str,
							  PangoAttrList           **attrs,
							  gint                     *cursor_pos);
static void     ctk_im_context_simple_set_client_window  (CtkIMContext             *context,
                                                          CdkWindow                *window);

G_DEFINE_TYPE_WITH_PRIVATE (CtkIMContextSimple, ctk_im_context_simple, CTK_TYPE_IM_CONTEXT)

static void
ctk_im_context_simple_class_init (CtkIMContextSimpleClass *class)
{
  CtkIMContextClass *im_context_class = CTK_IM_CONTEXT_CLASS (class);
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

  im_context_class->filter_keypress = ctk_im_context_simple_filter_keypress;
  im_context_class->reset = ctk_im_context_simple_reset;
  im_context_class->get_preedit_string = ctk_im_context_simple_get_preedit_string;
  im_context_class->set_client_window = ctk_im_context_simple_set_client_window;
  gobject_class->finalize = ctk_im_context_simple_finalize;
}

static gchar*
get_x11_compose_file_dir (void)
{
  gchar* compose_file_dir;

#if defined (CDK_WINDOWING_X11)
  compose_file_dir = g_strdup (X11_DATA_PREFIX "/share/X11/locale");
#else
  compose_file_dir = g_build_filename (_ctk_get_datadir (), "X11", "locale", NULL);
#endif

  return compose_file_dir;
}

static void
ctk_im_context_simple_init_compose_table (CtkIMContextSimple *im_context_simple)
{
  gchar *path = NULL;
  const gchar *home;
  const gchar *locale;
  gchar **langs = NULL;
  gchar **lang = NULL;
  gchar * const sys_langs[] = { "el_gr", "fi_fi", "pt_br", NULL };
  gchar * const *sys_lang = NULL;
  gchar *x11_compose_file_dir = get_x11_compose_file_dir ();

  path = g_build_filename (g_get_user_config_dir (), "ctk-3.0", "Compose", NULL);
  if (g_file_test (path, G_FILE_TEST_EXISTS))
    {
      ctk_im_context_simple_add_compose_file (im_context_simple, path);
      g_free (path);
      return;
    }
  g_free (path);
  path = NULL;

  home = g_get_home_dir ();
  if (home == NULL)
    return;

  path = g_build_filename (home, ".XCompose", NULL);
  if (g_file_test (path, G_FILE_TEST_EXISTS))
    {
      ctk_im_context_simple_add_compose_file (im_context_simple, path);
      g_free (path);
      return;
    }
  g_free (path);
  path = NULL;

  locale = g_getenv ("LC_CTYPE");
  if (locale == NULL)
    locale = g_getenv ("LANG");
  if (locale == NULL)
    locale = "C";

  /* FIXME: https://bugzilla.gnome.org/show_bug.cgi?id=751826 */
  langs = g_get_locale_variants (locale);

  for (lang = langs; *lang; lang++)
    {
      if (g_str_has_prefix (*lang, "en_US"))
        break;
      if (**lang == 'C')
        break;

      /* Other languages just include en_us compose table. */
      for (sys_lang = sys_langs; *sys_lang; sys_lang++)
        {
          if (g_ascii_strncasecmp (*lang, *sys_lang, strlen (*sys_lang)) == 0)
            {
              path = g_build_filename (x11_compose_file_dir, *lang, "Compose", NULL);
              break;
            }
        }

      if (path == NULL)
        continue;

      if (g_file_test (path, G_FILE_TEST_EXISTS))
        break;
      g_free (path);
      path = NULL;
    }

  g_free (x11_compose_file_dir);
  g_strfreev (langs);

  if (path != NULL)
    ctk_im_context_simple_add_compose_file (im_context_simple, path);
  g_free (path);
  path = NULL;
}

static void
init_compose_table_thread_cb (GTask        *task,
                              gpointer      source_object G_GNUC_UNUSED,
                              gpointer      task_data,
                              GCancellable *cancellable G_GNUC_UNUSED)
{
  if (g_task_return_error_if_cancelled (task))
    return;

  g_return_if_fail (CTK_IS_IM_CONTEXT_SIMPLE (task_data));

  ctk_im_context_simple_init_compose_table (CTK_IM_CONTEXT_SIMPLE (task_data));
}

void
init_compose_table_async (CtkIMContextSimple   *im_context_simple,
                          GCancellable         *cancellable,
                          GAsyncReadyCallback   callback,
                          gpointer              user_data)
{
  GTask *task = g_task_new (NULL, cancellable, callback, user_data);
  g_task_set_source_tag (task, init_compose_table_async);
  g_task_set_task_data (task, g_object_ref (im_context_simple), g_object_unref);
  g_task_run_in_thread (task, init_compose_table_thread_cb);
  g_object_unref (task);
}

static void
ctk_im_context_simple_init (CtkIMContextSimple *im_context_simple)
{
  im_context_simple->priv = ctk_im_context_simple_get_instance_private (im_context_simple); 
}

static void
ctk_im_context_simple_finalize (GObject *obj)
{
  G_OBJECT_CLASS (ctk_im_context_simple_parent_class)->finalize (obj);
}

/**
 * ctk_im_context_simple_new:
 * 
 * Creates a new #CtkIMContextSimple.
 *
 * Returns: a new #CtkIMContextSimple.
 **/
CtkIMContext *
ctk_im_context_simple_new (void)
{
  return g_object_new (CTK_TYPE_IM_CONTEXT_SIMPLE, NULL);
}

static void
ctk_im_context_simple_commit_char (CtkIMContext *context,
				   gunichar ch)
{
  CtkIMContextSimple *context_simple = CTK_IM_CONTEXT_SIMPLE (context);
  CtkIMContextSimplePrivate *priv = context_simple->priv;
  gchar buf[10];
  gint len;

  g_return_if_fail (g_unichar_validate (ch));

  len = g_unichar_to_utf8 (ch, buf);
  buf[len] = '\0';

  if (priv->tentative_match || priv->in_hex_sequence)
    {
      priv->in_hex_sequence = FALSE;
      priv->tentative_match = 0;
      priv->tentative_match_len = 0;
      g_signal_emit_by_name (context_simple, "preedit-changed");
      g_signal_emit_by_name (context_simple, "preedit-end");
    }

  g_signal_emit_by_name (context, "commit", &buf);
}

static int
compare_seq_index (const void *key, const void *value)
{
  const guint16 *keysyms = key;
  const guint16 *seq = value;

  if (keysyms[0] < seq[0])
    return -1;
  else if (keysyms[0] > seq[0])
    return 1;

  return 0;
}

static int
compare_seq (const void *key, const void *value)
{
  int i = 0;
  const guint16 *keysyms = key;
  const guint16 *seq = value;

  while (keysyms[i])
    {
      if (keysyms[i] < seq[i])
	return -1;
      else if (keysyms[i] > seq[i])
	return 1;

      i++;
    }

  return 0;
}

static gboolean
check_table (CtkIMContextSimple    *context_simple,
	     const CtkComposeTable *table,
	     gint                   n_compose)
{
  CtkIMContextSimplePrivate *priv = context_simple->priv;
  gint row_stride = table->max_seq_len + 2; 
  guint16 *seq; 
  
  /* Will never match, if the sequence in the compose buffer is longer
   * than the sequences in the table.  Further, compare_seq (key, val)
   * will overrun val if key is longer than val. */
  if (n_compose > table->max_seq_len)
    return FALSE;
  
  seq = bsearch (priv->compose_buffer,
		 table->data, table->n_seqs,
		 sizeof (guint16) *  row_stride, 
		 compare_seq);

  if (seq)
    {
      guint16 *prev_seq;

      /* Back up to the first sequence that matches to make sure
       * we find the exact match if there is one.
       */
      while (seq > table->data)
	{
	  prev_seq = seq - row_stride;
	  if (compare_seq (priv->compose_buffer, prev_seq) != 0)
	    break;
	  seq = prev_seq;
	}
      
      if (n_compose == table->max_seq_len ||
	  seq[n_compose] == 0) /* complete sequence */
	{
	  guint16 *next_seq;
	  gunichar value = 
	    0x10000 * seq[table->max_seq_len] + seq[table->max_seq_len + 1];

	  /* We found a tentative match. See if there are any longer
	   * sequences containing this subsequence
	   */
	  next_seq = seq + row_stride;
	  if (next_seq < table->data + row_stride * table->n_seqs)
	    {
	      if (compare_seq (priv->compose_buffer, next_seq) == 0)
		{
		  priv->tentative_match = value;
		  priv->tentative_match_len = n_compose;

		  g_signal_emit_by_name (context_simple, "preedit-changed");

		  return TRUE;
		}
	    }

	  ctk_im_context_simple_commit_char (CTK_IM_CONTEXT (context_simple), value);
	  priv->compose_buffer[0] = 0;
	}
      
      return TRUE;
    }

  return FALSE;
}

/* Checks if a keysym is a dead key. Dead key keysym values are defined in
 * ../cdk/cdkkeysyms.h and the first is CDK_KEY_dead_grave. As X.Org is updated,
 * more dead keys are added and we need to update the upper limit.
 * Currently, the upper limit is CDK_KEY_dead_dasia+1. The +1 has to do with
 * a temporary issue in the X.Org header files.
 * In future versions it will be just the keysym (no +1).
 */
#define IS_DEAD_KEY(k) \
    ((k) >= CDK_KEY_dead_grave && (k) <= (CDK_KEY_dead_dasia+1))

#ifdef CDK_WINDOWING_WIN32

/* On Windows, user expectation is that typing a dead accent followed
 * by space will input the corresponding spacing character. The X
 * compose tables are different for dead acute and diaeresis, which
 * when followed by space produce a plain ASCII apostrophe and double
 * quote respectively. So special-case those.
 */

static gboolean
check_win32_special_cases (CtkIMContextSimple    *context_simple,
			   gint                   n_compose)
{
  CtkIMContextSimplePrivate *priv = context_simple->priv;
  if (n_compose == 2 &&
      priv->compose_buffer[1] == CDK_KEY_space)
    {
      gunichar value = 0;

      switch (priv->compose_buffer[0])
	{
	case CDK_KEY_dead_acute:
	  value = 0x00B4; break;
	case CDK_KEY_dead_diaeresis:
	  value = 0x00A8; break;
	}
      if (value > 0)
	{
	  ctk_im_context_simple_commit_char (CTK_IM_CONTEXT (context_simple), value);
	  priv->compose_buffer[0] = 0;

	  return TRUE;
	}
    }
  return FALSE;
}

static void
check_win32_special_case_after_compact_match (CtkIMContextSimple    *context_simple,
					      gint                   n_compose,
					      guint                  value)
{
  CtkIMContextSimplePrivate *priv = context_simple->priv;

  /* On Windows user expectation is that typing two dead accents will input
   * two corresponding spacing accents.
   */
  if (n_compose == 2 &&
      priv->compose_buffer[0] == priv->compose_buffer[1] &&
      IS_DEAD_KEY (priv->compose_buffer[0]))
    {
      ctk_im_context_simple_commit_char (CTK_IM_CONTEXT (context_simple), value);
    }
}

#endif

#ifdef CDK_WINDOWING_QUARTZ

static gboolean
check_quartz_special_cases (CtkIMContextSimple *context_simple,
                            gint                n_compose)
{
  CtkIMContextSimplePrivate *priv = context_simple->priv;
  guint value = 0;

  if (n_compose == 2)
    {
      switch (priv->compose_buffer[0])
        {
        case CDK_KEY_dead_doubleacute:
          switch (priv->compose_buffer[1])
            {
            case CDK_KEY_dead_doubleacute:
            case CDK_KEY_space:
              value = CDK_KEY_quotedbl; break;

            case 'a': value = CDK_KEY_adiaeresis; break;
            case 'A': value = CDK_KEY_Adiaeresis; break;
            case 'e': value = CDK_KEY_ediaeresis; break;
            case 'E': value = CDK_KEY_Ediaeresis; break;
            case 'i': value = CDK_KEY_idiaeresis; break;
            case 'I': value = CDK_KEY_Idiaeresis; break;
            case 'o': value = CDK_KEY_odiaeresis; break;
            case 'O': value = CDK_KEY_Odiaeresis; break;
            case 'u': value = CDK_KEY_udiaeresis; break;
            case 'U': value = CDK_KEY_Udiaeresis; break;
            case 'y': value = CDK_KEY_ydiaeresis; break;
            case 'Y': value = CDK_KEY_Ydiaeresis; break;
            }
          break;

        case CDK_KEY_dead_acute:
          switch (priv->compose_buffer[1])
            {
            case 'c': value = CDK_KEY_ccedilla; break;
            case 'C': value = CDK_KEY_Ccedilla; break;
            }
          break;
        }
    }

  if (value > 0)
    {
      ctk_im_context_simple_commit_char (CTK_IM_CONTEXT (context_simple),
                                         cdk_keyval_to_unicode (value));
      priv->compose_buffer[0] = 0;

      return TRUE;
    }

  return FALSE;
}

#endif

gboolean
ctk_check_compact_table (const CtkComposeTableCompact  *table,
                         guint16                       *compose_buffer,
                         gint                           n_compose,
                         gboolean                      *compose_finish,
                         gboolean                      *compose_match,
                         gunichar                      *output_char)
{
  gint row_stride;
  guint16 *seq_index;
  guint16 *seq;
  gint i;
  gboolean match;
  gunichar value;

  if (compose_finish)
    *compose_finish = FALSE;
  if (compose_match)
    *compose_match = FALSE;
  if (output_char)
    *output_char = 0;

  /* Will never match, if the sequence in the compose buffer is longer
   * than the sequences in the table.  Further, compare_seq (key, val)
   * will overrun val if key is longer than val.
   */
  if (n_compose > table->max_seq_len)
    return FALSE;

  seq_index = bsearch (compose_buffer,
                       table->data,
                       table->n_index_size,
                       sizeof (guint16) * table->n_index_stride,
                       compare_seq_index);

  if (!seq_index)
    return FALSE;

  if (seq_index && n_compose == 1)
    return TRUE;

  seq = NULL;
  match = FALSE;
  value = 0;

  for (i = n_compose - 1; i < table->max_seq_len; i++)
    {
      row_stride = i + 1;

      if (seq_index[i + 1] - seq_index[i] > 0)
        {
          seq = bsearch (compose_buffer + 1,
                         table->data + seq_index[i],
                         (seq_index[i + 1] - seq_index[i]) / row_stride,
                         sizeof (guint16) *  row_stride,
                         compare_seq);

          if (seq)
            {
              if (i == n_compose - 1)
                {
                  value = seq[row_stride - 1];
                  match = TRUE;
                }
              else
                {
                  if (output_char)
                    *output_char = value;
                  if (match)
                    {
                      if (compose_match)
                        *compose_match = TRUE;
                    }

                  return TRUE;
                }
            }
        }
    }

  if (match)
    {
      if (compose_match)
        *compose_match = TRUE;
      if (compose_finish)
        *compose_finish = TRUE;
      if (output_char)
        *output_char = value;

      return TRUE;
    }

  return FALSE;
}

/* This function receives a sequence of Unicode characters and tries to
 * normalize it (NFC). We check for the case where the resulting string
 * has length 1 (single character).
 * NFC normalisation normally rearranges diacritic marks, unless these
 * belong to the same Canonical Combining Class.
 * If they belong to the same canonical combining class, we produce all
 * permutations of the diacritic marks, then attempt to normalize.
 */
static gboolean
check_normalize_nfc (gunichar* combination_buffer, gint n_compose)
{
  gunichar combination_buffer_temp[CTK_MAX_COMPOSE_LEN];
  gchar *combination_utf8_temp = NULL;
  gchar *nfc_temp = NULL;
  gint n_combinations;
  gunichar temp_swap;
  gint i;

  n_combinations = 1;

  for (i = 1; i < n_compose; i++ )
     n_combinations *= i;

  /* Xorg reuses dead_tilde for the perispomeni diacritic mark.
   * We check if base character belongs to Greek Unicode block,
   * and if so, we replace tilde with perispomeni.
   */
  if (combination_buffer[0] >= 0x390 && combination_buffer[0] <= 0x3FF)
    {
      for (i = 1; i < n_compose; i++ )
        if (combination_buffer[i] == 0x303)
          combination_buffer[i] = 0x342;
    }

  memcpy (combination_buffer_temp, combination_buffer, CTK_MAX_COMPOSE_LEN * sizeof (gunichar) );

  for (i = 0; i < n_combinations; i++ )
    {
      g_unicode_canonical_ordering (combination_buffer_temp, n_compose);
      combination_utf8_temp = g_ucs4_to_utf8 (combination_buffer_temp, -1, NULL, NULL, NULL);
      nfc_temp = g_utf8_normalize (combination_utf8_temp, -1, G_NORMALIZE_NFC);

      if (g_utf8_strlen (nfc_temp, -1) == 1)
        {
          memcpy (combination_buffer, combination_buffer_temp, CTK_MAX_COMPOSE_LEN * sizeof (gunichar) );

          g_free (combination_utf8_temp);
          g_free (nfc_temp);

          return TRUE;
        }

      g_free (combination_utf8_temp);
      g_free (nfc_temp);

      if (n_compose > 2)
        {
          temp_swap = combination_buffer_temp[i % (n_compose - 1) + 1];
          combination_buffer_temp[i % (n_compose - 1) + 1] = combination_buffer_temp[(i+1) % (n_compose - 1) + 1];
          combination_buffer_temp[(i+1) % (n_compose - 1) + 1] = temp_swap;
        }
      else
        break;
    }

  return FALSE;
}

gboolean
ctk_check_algorithmically (const guint16       *compose_buffer,
                           gint                 n_compose,
                           gunichar            *output_char)

{
  gint i;
  gunichar combination_buffer[CTK_MAX_COMPOSE_LEN];
  gchar *combination_utf8, *nfc;

  if (output_char)
    *output_char = 0;

  if (n_compose >= CTK_MAX_COMPOSE_LEN)
    return FALSE;

  for (i = 0; i < n_compose && IS_DEAD_KEY (compose_buffer[i]); i++)
    ;
  if (i == n_compose)
    return TRUE;

  if (i > 0 && i == n_compose - 1)
    {
      combination_buffer[0] = cdk_keyval_to_unicode (compose_buffer[i]);
      combination_buffer[n_compose] = 0;
      i--;
      while (i >= 0)
	{
	  switch (compose_buffer[i])
	    {
#define CASE(keysym, unicode) \
	    case CDK_KEY_dead_##keysym: combination_buffer[i+1] = unicode; break

	    CASE (grave, 0x0300);
	    CASE (acute, 0x0301);
	    CASE (circumflex, 0x0302);
	    CASE (tilde, 0x0303);	/* Also used with perispomeni, 0x342. */
	    CASE (macron, 0x0304);
	    CASE (breve, 0x0306);
	    CASE (abovedot, 0x0307);
	    CASE (diaeresis, 0x0308);
	    CASE (hook, 0x0309);
	    CASE (abovering, 0x030A);
	    CASE (doubleacute, 0x030B);
	    CASE (caron, 0x030C);
	    CASE (abovecomma, 0x0313);         /* Equivalent to psili */
	    CASE (abovereversedcomma, 0x0314); /* Equivalent to dasia */
	    CASE (horn, 0x031B);	/* Legacy use for psili, 0x313 (or 0x343). */
	    CASE (belowdot, 0x0323);
	    CASE (cedilla, 0x0327);
	    CASE (ogonek, 0x0328);	/* Legacy use for dasia, 0x314.*/
	    CASE (iota, 0x0345);
	    CASE (voiced_sound, 0x3099);	/* Per Markus Kuhn keysyms.txt file. */
	    CASE (semivoiced_sound, 0x309A);	/* Per Markus Kuhn keysyms.txt file. */

	    /* The following cases are to be removed once xkeyboard-config,
 	     * xorg are fully updated.
 	     */
            /* Workaround for typo in 1.4.x xserver-xorg */
	    case 0xfe66: combination_buffer[i+1] = 0x314; break;
	    /* CASE (dasia, 0x314); */
	    /* CASE (perispomeni, 0x342); */
	    /* CASE (psili, 0x343); */
#undef CASE
	    default:
	      combination_buffer[i+1] = cdk_keyval_to_unicode (compose_buffer[i]);
	    }
	  i--;
	}
      
      /* If the buffer normalizes to a single character, then modify the order
       * of combination_buffer accordingly, if necessary, and return TRUE.
       */
      if (check_normalize_nfc (combination_buffer, n_compose))
        {
      	  combination_utf8 = g_ucs4_to_utf8 (combination_buffer, -1, NULL, NULL, NULL);
          nfc = g_utf8_normalize (combination_utf8, -1, G_NORMALIZE_NFC);

          if (output_char)
            *output_char = g_utf8_get_char (nfc);

          g_free (combination_utf8);
          g_free (nfc);

          return TRUE;
        }
    }

  return FALSE;
}

/* In addition to the table-driven sequences, we allow Unicode hex
 * codes to be entered. The method chosen here is similar to the
 * one recommended in ISO 14755, but not exactly the same, since we
 * don’t want to steal 16 valuable key combinations.
 *
 * A hex Unicode sequence must be started with Ctrl-Shift-U, followed
 * by a sequence of hex digits entered with Ctrl-Shift still held.
 * Releasing one of the modifiers or pressing space while the modifiers
 * are still held commits the character. It is possible to erase
 * digits using backspace.
 *
 * As an extension to the above, we also allow to start the sequence
 * with Ctrl-Shift-U, then release the modifiers before typing any
 * digits, and enter the digits without modifiers.
 */

static gboolean
check_hex (CtkIMContextSimple *context_simple,
           gint                n_compose)
{
  CtkIMContextSimplePrivate *priv = context_simple->priv;
  /* See if this is a hex sequence, return TRUE if so */
  gint i;
  GString *str;
  gulong n;
  gchar *nptr = NULL;
  gchar buf[7];

  priv->tentative_match = 0;
  priv->tentative_match_len = 0;

  str = g_string_new (NULL);
  
  i = 0;
  while (i < n_compose)
    {
      gunichar ch;
      
      ch = cdk_keyval_to_unicode (priv->compose_buffer[i]);
      
      if (ch == 0)
        return FALSE;

      if (!g_unichar_isxdigit (ch))
        return FALSE;

      buf[g_unichar_to_utf8 (ch, buf)] = '\0';

      g_string_append (str, buf);
      
      ++i;
    }

  n = strtoul (str->str, &nptr, 16);

  /* If strtoul fails it probably means non-latin digits were used;
   * we should in principle handle that, but we probably don't.
   */
  if (nptr - str->str < str->len)
    {
      g_string_free (str, TRUE);
      return FALSE;
    }
  else
    g_string_free (str, TRUE);

  if (g_unichar_validate (n))
    {
      priv->tentative_match = n;
      priv->tentative_match_len = n_compose;
    }
  
  return TRUE;
}

static void
beep_window (CdkWindow *window)
{
  CdkScreen *screen = cdk_window_get_screen (window);
  gboolean   beep;

  g_object_get (ctk_settings_get_for_screen (screen),
                "ctk-error-bell", &beep,
                NULL);

  if (beep)
    cdk_window_beep (window);
}

static gboolean
no_sequence_matches (CtkIMContextSimple *context_simple,
                     gint                n_compose,
                     CdkEventKey        *event)
{
  CtkIMContextSimplePrivate *priv = context_simple->priv;
  CtkIMContext *context;
  gunichar ch;
  
  context = CTK_IM_CONTEXT (context_simple);
  
  /* No compose sequences found, check first if we have a partial
   * match pending.
   */
  if (priv->tentative_match)
    {
      gint len = priv->tentative_match_len;
      int i;
      
      ctk_im_context_simple_commit_char (context, priv->tentative_match);
      priv->compose_buffer[0] = 0;
      
      for (i=0; i < n_compose - len - 1; i++)
	{
	  CdkEvent *tmp_event = cdk_event_copy ((CdkEvent *)event);
	  tmp_event->key.keyval = priv->compose_buffer[len + i];
	  
	  ctk_im_context_filter_keypress (context, (CdkEventKey *)tmp_event);
	  cdk_event_free (tmp_event);
	}

      return ctk_im_context_filter_keypress (context, event);
    }
  else
    {
      priv->compose_buffer[0] = 0;
      if (n_compose > 1)		/* Invalid sequence */
	{
	  beep_window (event->window);
	  return TRUE;
	}
  
      ch = cdk_keyval_to_unicode (event->keyval);
      if (ch != 0 && !g_unichar_iscntrl (ch))
	{
	  ctk_im_context_simple_commit_char (context, ch);
	  return TRUE;
	}
      else
	return FALSE;
    }
}

static gboolean
is_hex_keyval (guint keyval)
{
  gunichar ch = cdk_keyval_to_unicode (keyval);

  return g_unichar_isxdigit (ch);
}

static guint
canonical_hex_keyval (CdkEventKey *event)
{
  CdkKeymap *keymap = cdk_keymap_get_for_display (cdk_window_get_display (event->window));
  guint keyval;
  guint *keyvals = NULL;
  gint n_vals = 0;
  gint i;
  
  /* See if the keyval is already a hex digit */
  if (is_hex_keyval (event->keyval))
    return event->keyval;

  /* See if this key would have generated a hex keyval in
   * any other state, and return that hex keyval if so
   */
  cdk_keymap_get_entries_for_keycode (keymap,
				      event->hardware_keycode,
				      NULL,
				      &keyvals, &n_vals);

  keyval = 0;
  i = 0;
  while (i < n_vals)
    {
      if (is_hex_keyval (keyvals[i]))
        {
          keyval = keyvals[i];
          break;
        }

      ++i;
    }

  g_free (keyvals);
  
  if (keyval)
    return keyval;
  else
    /* No way to make it a hex digit
     */
    return 0;
}

static gboolean
ctk_im_context_simple_filter_keypress (CtkIMContext *context,
				       CdkEventKey  *event)
{
  CtkIMContextSimple *context_simple = CTK_IM_CONTEXT_SIMPLE (context);
  CtkIMContextSimplePrivate *priv = context_simple->priv;
  CdkDisplay *display = cdk_window_get_display (event->window);
  GSList *tmp_list;  
  int n_compose = 0;
  CdkModifierType hex_mod_mask;
  gboolean have_hex_mods;
  gboolean is_hex_start;
  gboolean is_hex_end;
  gboolean is_backspace;
  gboolean is_escape;
  guint hex_keyval;
  int i;
  gboolean compose_finish;
  gboolean compose_match;
  gunichar output_char;

  while (priv->compose_buffer[n_compose] != 0)
    n_compose++;

  if (event->type == CDK_KEY_RELEASE)
    {
      if (priv->in_hex_sequence &&
	  (event->keyval == CDK_KEY_Control_L || event->keyval == CDK_KEY_Control_R ||
	   event->keyval == CDK_KEY_Shift_L || event->keyval == CDK_KEY_Shift_R))
	{
	  if (priv->tentative_match &&
	      g_unichar_validate (priv->tentative_match))
	    {
	      ctk_im_context_simple_commit_char (context, priv->tentative_match);
	      priv->compose_buffer[0] = 0;

	    }
	  else if (n_compose == 0)
	    {
	      priv->modifiers_dropped = TRUE;
	    }
	  else
	    {
	      /* invalid hex sequence */
	      beep_window (event->window);
	      
	      priv->tentative_match = 0;
	      priv->in_hex_sequence = FALSE;
	      priv->compose_buffer[0] = 0;
	      
	      g_signal_emit_by_name (context_simple, "preedit-changed");
	      g_signal_emit_by_name (context_simple, "preedit-end");
	    }

	  return TRUE;
	}
      else
	return FALSE;
    }

  /* Ignore modifier key presses */
  for (i = 0; i < G_N_ELEMENTS (ctk_compose_ignore); i++)
    if (event->keyval == ctk_compose_ignore[i])
      return FALSE;

  hex_mod_mask = cdk_keymap_get_modifier_mask (cdk_keymap_get_for_display (display),
                                               CDK_MODIFIER_INTENT_PRIMARY_ACCELERATOR);
  hex_mod_mask |= CDK_SHIFT_MASK;

  if (priv->in_hex_sequence && priv->modifiers_dropped)
    have_hex_mods = TRUE;
  else
    have_hex_mods = (event->state & (hex_mod_mask)) == hex_mod_mask;
  is_hex_start = event->keyval == CDK_KEY_U;
  is_hex_end = (event->keyval == CDK_KEY_space ||
		event->keyval == CDK_KEY_KP_Space ||
		event->keyval == CDK_KEY_Return ||
		event->keyval == CDK_KEY_ISO_Enter ||
		event->keyval == CDK_KEY_KP_Enter);
  is_backspace = event->keyval == CDK_KEY_BackSpace;
  is_escape = event->keyval == CDK_KEY_Escape;
  hex_keyval = canonical_hex_keyval (event);

  /* If we are already in a non-hex sequence, or
   * this keystroke is not hex modifiers + hex digit, don't filter
   * key events with accelerator modifiers held down. We only treat
   * Control and Alt as accel modifiers here, since Super, Hyper and
   * Meta are often co-located with Mode_Switch, Multi_Key or
   * ISO_Level3_Switch.
   */
  if (!have_hex_mods ||
      (n_compose > 0 && !priv->in_hex_sequence) ||
      (n_compose == 0 && !priv->in_hex_sequence && !is_hex_start) ||
      (priv->in_hex_sequence && !hex_keyval &&
       !is_hex_start && !is_hex_end && !is_escape && !is_backspace))
    {
      CdkModifierType no_text_input_mask;

      no_text_input_mask =
        cdk_keymap_get_modifier_mask (cdk_keymap_get_for_display (display),
                                      CDK_MODIFIER_INTENT_NO_TEXT_INPUT);

      if (event->state & no_text_input_mask ||
	  (priv->in_hex_sequence && priv->modifiers_dropped &&
	   (event->keyval == CDK_KEY_Return ||
	    event->keyval == CDK_KEY_ISO_Enter ||
	    event->keyval == CDK_KEY_KP_Enter)))
	{
	  return FALSE;
	}
    }
  
  /* Handle backspace */
  if (priv->in_hex_sequence && have_hex_mods && is_backspace)
    {
      if (n_compose > 0)
	{
	  n_compose--;
	  priv->compose_buffer[n_compose] = 0;
          check_hex (context_simple, n_compose);
	}
      else
	{
	  priv->in_hex_sequence = FALSE;
	}

      g_signal_emit_by_name (context_simple, "preedit-changed");

      if (!priv->in_hex_sequence)
        g_signal_emit_by_name (context_simple, "preedit-end");
      
      return TRUE;
    }

  /* Check for hex sequence restart */
  if (priv->in_hex_sequence && have_hex_mods && is_hex_start)
    {
      if (priv->tentative_match &&
	  g_unichar_validate (priv->tentative_match))
	{
	  ctk_im_context_simple_commit_char (context, priv->tentative_match);
	  priv->compose_buffer[0] = 0;
	}
      else 
	{
	  /* invalid hex sequence */
	  if (n_compose > 0)
	    beep_window (event->window);
	  
	  priv->tentative_match = 0;
	  priv->in_hex_sequence = FALSE;
	  priv->compose_buffer[0] = 0;
	}
    }
  
  /* Check for hex sequence start */
  if (!priv->in_hex_sequence && have_hex_mods && is_hex_start)
    {
      priv->compose_buffer[0] = 0;
      priv->in_hex_sequence = TRUE;
      priv->modifiers_dropped = FALSE;
      priv->tentative_match = 0;

      g_signal_emit_by_name (context_simple, "preedit-start");
      g_signal_emit_by_name (context_simple, "preedit-changed");
  
      return TRUE;
    }
  
  /* Then, check for compose sequences */
  if (priv->in_hex_sequence)
    {
      if (hex_keyval)
	priv->compose_buffer[n_compose++] = hex_keyval;
      else if (is_escape)
	{
	  ctk_im_context_simple_reset (context);
	  
	  return TRUE;
	}
      else if (!is_hex_end)
	{
	  /* non-hex character in hex sequence */
	  beep_window (event->window);
	  
	  return TRUE;
	}
    }
  else
    priv->compose_buffer[n_compose++] = event->keyval;

  priv->compose_buffer[n_compose] = 0;

  if (priv->in_hex_sequence)
    {
      /* If the modifiers are still held down, consider the sequence again */
      if (have_hex_mods)
        {
          /* space or return ends the sequence, and we eat the key */
          if (n_compose > 0 && is_hex_end)
            {
	      if (priv->tentative_match &&
		  g_unichar_validate (priv->tentative_match))
		{
		  ctk_im_context_simple_commit_char (context, priv->tentative_match);
		  priv->compose_buffer[0] = 0;
		}
	      else
		{
		  /* invalid hex sequence */
		  beep_window (event->window);

		  priv->tentative_match = 0;
		  priv->in_hex_sequence = FALSE;
		  priv->compose_buffer[0] = 0;
		}
            }
          else if (!check_hex (context_simple, n_compose))
	    beep_window (event->window);
	  
	  g_signal_emit_by_name (context_simple, "preedit-changed");

	  if (!priv->in_hex_sequence)
	    g_signal_emit_by_name (context_simple, "preedit-end");

	  return TRUE;
        }
    }
  else
    {
      gboolean success = FALSE;

#ifdef CDK_WINDOWING_WIN32
      if (CDK_IS_WIN32_DISPLAY (display))
        {
          guint16  output[2];
          gsize    output_size = 2;

          switch (cdk_win32_keymap_check_compose (CDK_WIN32_KEYMAP (cdk_keymap_get_default ()),
                                                  priv->compose_buffer,
                                                  n_compose,
                                                  output, &output_size))
            {
            case CDK_WIN32_KEYMAP_MATCH_NONE:
              break;
            case CDK_WIN32_KEYMAP_MATCH_EXACT:
            case CDK_WIN32_KEYMAP_MATCH_PARTIAL:
              for (i = 0; i < output_size; i++)
                {
                  output_char = cdk_keyval_to_unicode (output[i]);
                  ctk_im_context_simple_commit_char (CTK_IM_CONTEXT (context_simple),
                                                     output_char);
                }
              priv->compose_buffer[0] = 0;
              return TRUE;
            case CDK_WIN32_KEYMAP_MATCH_INCOMPLETE:
              return TRUE;
            }
        }
#endif

      G_LOCK (global_tables);

      tmp_list = global_tables;
      while (tmp_list)
        {
          if (check_table (context_simple, tmp_list->data, n_compose))
            {
              success = TRUE;
              break;
            }
          tmp_list = tmp_list->next;
        }

      G_UNLOCK (global_tables);

      if (success)
        return TRUE;

#ifdef CDK_WINDOWING_WIN32
      if (check_win32_special_cases (context_simple, n_compose))
	return TRUE;
#endif

#ifdef CDK_WINDOWING_QUARTZ
      if (check_quartz_special_cases (context_simple, n_compose))
        return TRUE;
#endif

      if (ctk_check_compact_table (&ctk_compose_table_compact,
                                   priv->compose_buffer,
                                   n_compose, &compose_finish,
                                   &compose_match, &output_char))
        {
          if (compose_finish)
            {
              if (compose_match)
                {
                  ctk_im_context_simple_commit_char (CTK_IM_CONTEXT (context_simple),
                                                     output_char);
#ifdef G_OS_WIN32
                  check_win32_special_case_after_compact_match (context_simple,
                                                                n_compose,
                                                                output_char);
#endif
                  priv->compose_buffer[0] = 0;
                }
            }
          else
            {
              if (compose_match)
                {
                  priv->tentative_match = output_char;
                  priv->tentative_match_len = n_compose;
                }
              if (output_char)
                g_signal_emit_by_name (context_simple, "preedit-changed");
            }

          return TRUE;
        }
  
      if (ctk_check_algorithmically (priv->compose_buffer, n_compose, &output_char))
        {
          if (output_char)
            {
              ctk_im_context_simple_commit_char (CTK_IM_CONTEXT (context_simple),
                                                 output_char);
              priv->compose_buffer[0] = 0;
            }
	  return TRUE;
        }
    }
  
  /* The current compose_buffer doesn't match anything */
  return no_sequence_matches (context_simple, n_compose, event);
}

static void
ctk_im_context_simple_reset (CtkIMContext *context)
{
  CtkIMContextSimple *context_simple = CTK_IM_CONTEXT_SIMPLE (context);
  CtkIMContextSimplePrivate *priv = context_simple->priv;

  priv->compose_buffer[0] = 0;

  if (priv->tentative_match || priv->in_hex_sequence)
    {
      priv->in_hex_sequence = FALSE;
      priv->tentative_match = 0;
      priv->tentative_match_len = 0;
      g_signal_emit_by_name (context_simple, "preedit-changed");
      g_signal_emit_by_name (context_simple, "preedit-end");
    }
}

static void     
ctk_im_context_simple_get_preedit_string (CtkIMContext   *context,
					  gchar         **str,
					  PangoAttrList **attrs,
					  gint           *cursor_pos)
{
  CtkIMContextSimple *context_simple = CTK_IM_CONTEXT_SIMPLE (context);
  CtkIMContextSimplePrivate *priv = context_simple->priv;
  char outbuf[37]; /* up to 6 hex digits */
  int len = 0;

  if (priv->in_hex_sequence)
    {
      int hexchars = 0;
         
      outbuf[0] = 'u';
      len = 1;

      while (priv->compose_buffer[hexchars] != 0)
	{
	  len += g_unichar_to_utf8 (cdk_keyval_to_unicode (priv->compose_buffer[hexchars]),
				    outbuf + len);
	  ++hexchars;
	}

      g_assert (len < 25);
    }
  else if (priv->tentative_match)
    len = g_unichar_to_utf8 (priv->tentative_match, outbuf);
      
  outbuf[len] = '\0';      

  if (str)
    *str = g_strdup (outbuf);

  if (attrs)
    {
      *attrs = pango_attr_list_new ();
      
      if (len)
	{
	  PangoAttribute *attr = pango_attr_underline_new (PANGO_UNDERLINE_SINGLE);
	  attr->start_index = 0;
          attr->end_index = len;
	  pango_attr_list_insert (*attrs, attr);
	}
    }

  if (cursor_pos)
    *cursor_pos = len;
}

static void
ctk_im_context_simple_set_client_window  (CtkIMContext *context,
                                          CdkWindow    *window)
{
  CtkIMContextSimple *im_context_simple = CTK_IM_CONTEXT_SIMPLE (context);
  gboolean run_compose_table = FALSE;

  if (!window)
    return;

  /* Load compose table for X11 or Wayland. */
#ifdef CDK_WINDOWING_X11
  if (CDK_IS_X11_DISPLAY (cdk_window_get_display (window)))
    run_compose_table = TRUE;
#endif
#ifdef CDK_WINDOWING_WAYLAND
  if (CDK_IS_WAYLAND_DISPLAY (cdk_window_get_display (window)))
    run_compose_table = TRUE;
#endif

  if (run_compose_table)
    init_compose_table_async (im_context_simple, NULL, NULL, NULL);
}

/**
 * ctk_im_context_simple_add_table: (skip)
 * @context_simple: A #CtkIMContextSimple
 * @data: (array): the table
 * @max_seq_len: Maximum length of a sequence in the table
 *               (cannot be greater than #CTK_MAX_COMPOSE_LEN)
 * @n_seqs: number of sequences in the table
 * 
 * Adds an additional table to search to the input context.
 * Each row of the table consists of @max_seq_len key symbols
 * followed by two #guint16 interpreted as the high and low
 * words of a #gunicode value. Tables are searched starting
 * from the last added.
 *
 * The table must be sorted in dictionary order on the
 * numeric value of the key symbol fields. (Values beyond
 * the length of the sequence should be zero.)
 **/
void
ctk_im_context_simple_add_table (CtkIMContextSimple *context_simple,
				 guint16            *data,
				 gint                max_seq_len,
				 gint                n_seqs)
{
  g_return_if_fail (CTK_IS_IM_CONTEXT_SIMPLE (context_simple));

  G_LOCK (global_tables);

  global_tables = ctk_compose_table_list_add_array (global_tables,
                                                    data, max_seq_len, n_seqs);

  G_UNLOCK (global_tables);
}

/*
 * ctk_im_context_simple_add_compose_file:
 * @context_simple: A #CtkIMContextSimple
 * @compose_file: The path of compose file
 *
 * Adds an additional table from the X11 compose file.
 *
 * Since: 3.20
 */
void
ctk_im_context_simple_add_compose_file (CtkIMContextSimple *context_simple,
                                        const gchar        *compose_file)
{
  g_return_if_fail (CTK_IS_IM_CONTEXT_SIMPLE (context_simple));

  G_LOCK (global_tables);

  global_tables = ctk_compose_table_list_add_file (global_tables,
                                                   compose_file);

  G_UNLOCK (global_tables);
}
