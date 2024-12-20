/* Pango/Font Features
 *
 * This example demonstrates support for OpenType font features with
 * Pango attributes. The attributes can be used manually or via Pango
 * markup.
 *
 * It can also be used to explore available features in OpenType fonts
 * and their effect.
 */

#include <ctk/ctk.h>
#include <pango/pangofc-font.h>
#include <hb.h>
#include <hb-ot.h>
#include <hb-ft.h>

static CtkWidget *label;
static CtkWidget *settings;
static CtkWidget *font;
static CtkWidget *script_lang;
static CtkWidget *resetbutton;
static CtkWidget *numcasedefault;
static CtkWidget *numspacedefault;
static CtkWidget *fractiondefault;
static CtkWidget *stack;
static CtkWidget *entry;

#define num_features 40

static CtkWidget *toggle[num_features];
static CtkWidget *icon[num_features];
static const char *feature_names[num_features] = {
  "kern", "liga", "dlig", "hlig", "clig", "smcp", "c2sc", "pcap", "c2pc", "unic",
  "cpsp", "case", "lnum", "onum", "pnum", "tnum", "frac", "afrc", "zero", "nalt",
  "sinf", "swsh", "cswh", "locl", "calt", "hist", "salt", "titl", "rand", "subs",
  "sups", "init", "medi", "fina", "isol", "ss01", "ss02", "ss03", "ss04", "ss05"
};

static void
update_display (void)
{
  GString *s;
  char *font_desc;
  char *font_settings;
  const char *text;
  gboolean has_feature;
  int i;
  hb_tag_t lang_tag;
  CtkTreeIter iter;
  const char *lang;

  text = ctk_entry_get_text (CTK_ENTRY (entry));

  font_desc = ctk_font_chooser_get_font (CTK_FONT_CHOOSER (font));

  s = g_string_new ("");

  has_feature = FALSE;
  for (i = 0; i < num_features; i++)
    {
      if (!ctk_widget_is_sensitive (toggle[i]))
        continue;

      if (CTK_IS_RADIO_BUTTON (toggle[i]))
        {
          if (ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (toggle[i])))
            {
              if (has_feature)
                g_string_append (s, ", ");
              g_string_append (s, ctk_buildable_get_name (CTK_BUILDABLE (toggle[i])));
              g_string_append (s, " 1");
              has_feature = TRUE;
            }
        }
      else
        {
          if (has_feature)
            g_string_append (s, ", ");
          g_string_append (s, ctk_buildable_get_name (CTK_BUILDABLE (toggle[i])));
          if (ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (toggle[i])))
            g_string_append (s, " 1");
          else
            g_string_append (s, " 0");
          has_feature = TRUE;
        }
    }

  font_settings = g_string_free (s, FALSE);

  ctk_label_set_text (CTK_LABEL (settings), font_settings);


  if (ctk_combo_box_get_active_iter (CTK_COMBO_BOX (script_lang), &iter))
    {
      CtkTreeModel *model;

      model = ctk_combo_box_get_model (CTK_COMBO_BOX (script_lang));
      ctk_tree_model_get (model, &iter,
                          3, &lang_tag,
                          -1);

      lang = hb_language_to_string (hb_ot_tag_to_language (lang_tag));
    }
  else
    lang = NULL;

  s = g_string_new ("");
  g_string_append_printf (s, "<span font_desc='%s' font_features='%s'", font_desc, font_settings);
  if (lang)
    g_string_append_printf (s, " lang='%s'", lang);
  g_string_append_printf (s, ">%s</span>", text);

  ctk_label_set_markup (CTK_LABEL (label), s->str);

  g_string_free (s, TRUE);

  g_free (font_desc);
  g_free (font_settings);
}

static PangoFont *
get_pango_font (void)
{
  PangoFontDescription *desc;
  PangoContext *context;
  PangoFontMap *map;

  desc = ctk_font_chooser_get_font_desc (CTK_FONT_CHOOSER (font));
  context = ctk_widget_get_pango_context (font);
  map = pango_context_get_font_map (context);

  return pango_font_map_load_font (map, context, desc);
}

static struct { const char *name; hb_script_t script; } script_names[] = {
  { "Common", HB_SCRIPT_COMMON },
  { "Inherited", HB_SCRIPT_INHERITED },
  { "Unknown", HB_SCRIPT_UNKNOWN },
  { "Arabic", HB_SCRIPT_ARABIC },
  { "Armenian", HB_SCRIPT_ARMENIAN },
  { "Bengali", HB_SCRIPT_BENGALI },
  { "Cyrillic", HB_SCRIPT_CYRILLIC },
  { "Devanagari", HB_SCRIPT_DEVANAGARI },
  { "Georgian", HB_SCRIPT_GEORGIAN },
  { "Greek", HB_SCRIPT_GREEK },
  { "Gujarati", HB_SCRIPT_GUJARATI },
  { "Gurmukhi", HB_SCRIPT_GURMUKHI },
  { "Hangul", HB_SCRIPT_HANGUL },
  { "Han", HB_SCRIPT_HAN },
  { "Hebrew", HB_SCRIPT_HEBREW },
  { "Hiragana", HB_SCRIPT_HIRAGANA },
  { "Kannada", HB_SCRIPT_KANNADA },
  { "Katakana", HB_SCRIPT_KATAKANA },
  { "Lao", HB_SCRIPT_LAO },
  { "Latin", HB_SCRIPT_LATIN },
  { "Malayalam", HB_SCRIPT_MALAYALAM },
  { "Oriya", HB_SCRIPT_ORIYA },
  { "Tamil", HB_SCRIPT_TAMIL },
  { "Telugu", HB_SCRIPT_TELUGU },
  { "Thai", HB_SCRIPT_THAI },
  { "Tibetan", HB_SCRIPT_TIBETAN },
  { "Bopomofo", HB_SCRIPT_BOPOMOFO }
  /* FIXME: complete */
};

static struct { const char *name; hb_tag_t tag; } language_names[] = {
  { "Arabic", HB_TAG ('A','R','A',' ') },
  { "Romanian", HB_TAG ('R','O','M',' ') },
  { "Skolt Sami", HB_TAG ('S','K','S',' ') },
  { "Northern Sami", HB_TAG ('N','S','M',' ') },
  { "Kildin Sami", HB_TAG ('K','S','M',' ') },
  { "Moldavian", HB_TAG ('M','O','L',' ') },
  { "Turkish", HB_TAG ('T','R','K',' ') },
  { "Azerbaijani", HB_TAG ('A','Z','E',' ') },
  { "Crimean Tatar", HB_TAG ('C','R','T',' ') },
  { "Serbian", HB_TAG ('S','R','B',' ') },
  { "German", HB_TAG ('D','E','U',' ') }
  /* FIXME: complete */
};

typedef struct {
  hb_tag_t script_tag;
  hb_tag_t lang_tag;
  unsigned int script_index;
  unsigned int lang_index;
} TagPair;

static guint
tag_pair_hash (gconstpointer data)
{
  const TagPair *pair = data;

  return pair->script_tag + pair->lang_tag;
}

static gboolean
tag_pair_equal (gconstpointer a, gconstpointer b)
{
  const TagPair *pair_a = a;
  const TagPair *pair_b = b;

  return pair_a->script_tag == pair_b->script_tag && pair_a->lang_tag == pair_b->lang_tag;
}

static void
update_script_combo (void)
{
  CtkListStore *store;
  hb_font_t *hb_font;
  gint i, j, k, l;
  PangoFont *pango_font;
  GHashTable *tags;
  GHashTableIter iter;
  TagPair *pair;

  store = ctk_list_store_new (4, G_TYPE_STRING, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT);

  pango_font = get_pango_font ();
  hb_font = pango_font_get_hb_font (pango_font);

  tags = g_hash_table_new_full (tag_pair_hash, tag_pair_equal, g_free, NULL);

  pair = g_new (TagPair, 1);
  pair->script_tag = HB_OT_TAG_DEFAULT_SCRIPT;
  pair->lang_tag = HB_OT_TAG_DEFAULT_LANGUAGE;
  g_hash_table_insert (tags, pair, g_strdup ("Default"));

  if (hb_font)
    {
      hb_tag_t tables[2] = { HB_OT_TAG_GSUB, HB_OT_TAG_GPOS };
      hb_face_t *hb_face;

      hb_face = hb_font_get_face (hb_font);

      for (i= 0; i < 2; i++)
        {
          hb_tag_t scripts[80];
          unsigned int script_count = G_N_ELEMENTS (scripts);

          hb_ot_layout_table_get_script_tags (hb_face, tables[i], 0, &script_count, scripts);
          for (j = 0; j < script_count; j++)
            {
              hb_tag_t languages[80];
              unsigned int language_count = G_N_ELEMENTS (languages);

              pair = g_new (TagPair, 1);
              pair->script_tag = scripts[j];
              pair->lang_tag = HB_OT_TAG_DEFAULT_LANGUAGE;
              pair->script_index = j;
              pair->lang_index = HB_OT_LAYOUT_DEFAULT_LANGUAGE_INDEX;
              g_hash_table_add (tags, pair);

              hb_ot_layout_script_get_language_tags (hb_face, tables[i], j, 0, &language_count, languages);
              for (k = 0; k < language_count; k++)
                {
                  pair = g_new (TagPair, 1);
                  pair->script_tag = scripts[j];
                  pair->lang_tag = languages[k];
                  pair->script_index = j;
                  pair->lang_index = k;
                  g_hash_table_add (tags, pair);
                }
            }
        }
    }

  g_object_unref (pango_font);

  g_hash_table_iter_init (&iter, tags);
  while (g_hash_table_iter_next (&iter, (gpointer *)&pair, NULL))
    {
      const char *scriptname;
      char scriptbuf[5];
      const char *langname;
      char langbuf[5];
      char *name;

      if (pair->script_tag == HB_OT_TAG_DEFAULT_SCRIPT)
        scriptname = "Default";
      else if (pair->script_tag == HB_TAG ('m','a','t','h'))
        scriptname = "Math";
      else
        {
          hb_script_t script;

          hb_tag_to_string (pair->script_tag, scriptbuf);
          scriptbuf[4] = 0;
          scriptname = scriptbuf;

          script = hb_script_from_iso15924_tag (pair->script_tag);
          for (k = 0; k < G_N_ELEMENTS (script_names); k++)
            {
              if (script == script_names[k].script)
                {
                  scriptname = script_names[k].name;
                  break;
                }
            }
        }

      if (pair->lang_tag == HB_OT_TAG_DEFAULT_LANGUAGE)
        langname = "Default";
      else
        {
          hb_tag_to_string (pair->lang_tag, langbuf);
          langbuf[4] = 0;
          langname = langbuf;

          for (l = 0; l < G_N_ELEMENTS (language_names); l++)
            {
              if (pair->lang_tag == language_names[l].tag)
                {
                  langname = language_names[l].name;
                  break;
                }
            }
        }

      name = g_strdup_printf ("%s - %s", scriptname, langname);

      ctk_list_store_insert_with_values (store, NULL, -1,
                                         0, name,
                                         1, pair->script_index,
                                         2, pair->lang_index,
                                         3, pair->lang_tag,
                                         -1);

      g_free (name);
    }

  g_hash_table_destroy (tags);

  ctk_combo_box_set_model (CTK_COMBO_BOX (script_lang), CTK_TREE_MODEL (store));
  ctk_combo_box_set_active (CTK_COMBO_BOX (script_lang), 0);
}

static void
update_features (void)
{
  gint i, j, k;
  CtkTreeModel *model;
  CtkTreeIter iter;
  guint script_index, lang_index;
  PangoFont *pango_font;
  hb_font_t *hb_font;

  for (i = 0; i < num_features; i++)
    ctk_widget_set_opacity (icon[i], 0);

  /* set feature presence checks from the font features */

  if (!ctk_combo_box_get_active_iter (CTK_COMBO_BOX (script_lang), &iter))
    return;

  model = ctk_combo_box_get_model (CTK_COMBO_BOX (script_lang));
  ctk_tree_model_get (model, &iter,
                      1, &script_index,
                      2, &lang_index,
                      -1);

  pango_font = get_pango_font ();
  hb_font = pango_font_get_hb_font (pango_font);

  if (hb_font)
    {
      hb_tag_t tables[2] = { HB_OT_TAG_GSUB, HB_OT_TAG_GPOS };
      hb_face_t *hb_face;

      hb_face = hb_font_get_face (hb_font);

      for (i = 0; i < 2; i++)
        {
          hb_tag_t features[80];
          unsigned int count = G_N_ELEMENTS(features);

          hb_ot_layout_language_get_feature_tags (hb_face,
                                                  tables[i],
                                                  script_index,
                                                  lang_index,
                                                  0,
                                                  &count,
                                                  features);

          for (j = 0; j < count; j++)
            {
              for (k = 0; k < num_features; k++)
                {
                  if (hb_tag_from_string (feature_names[k], -1) == features[j])
                    ctk_widget_set_opacity (icon[k], 0.5);
                }
            }
        }
    }

  g_object_unref (pango_font);
}

static void
font_changed (void)
{
  update_script_combo ();
}

static void
script_changed (void)
{
  update_features ();
  update_display ();
}

static void
reset_features (void)
{
  int i;

  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (numcasedefault), TRUE);
  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (numspacedefault), TRUE);
  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (fractiondefault), TRUE);
  for (i = 0; i < num_features; i++)
    {
      if (!CTK_IS_RADIO_BUTTON (toggle[i]))
        {
          ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (toggle[i]), FALSE);
          ctk_widget_set_sensitive (toggle[i], FALSE);
        }
    }
}

static char *text;

static void
switch_to_entry (void)
{
  text = g_strdup (ctk_entry_get_text (CTK_ENTRY (entry)));
  ctk_stack_set_visible_child_name (CTK_STACK (stack), "entry");
}

static void
switch_to_label (void)
{
  g_free (text);
  text = NULL;
  ctk_stack_set_visible_child_name (CTK_STACK (stack), "label");
  update_display ();
}

static gboolean
entry_key_press (CtkEntry *entry, CdkEventKey *event)
{
  if (event->keyval == CDK_KEY_Escape)
    {
      ctk_entry_set_text (CTK_ENTRY (entry), text);
      switch_to_label ();
      return CDK_EVENT_STOP;
    }

  return CDK_EVENT_PROPAGATE;
}

CtkWidget *
do_font_features (CtkWidget *do_widget G_GNUC_UNUSED)
{
  static CtkWidget *window = NULL;

  if (!window)
    {
      CtkBuilder *builder;
      int i;

      builder = ctk_builder_new_from_resource ("/font_features/font-features.ui");

      ctk_builder_add_callback_symbol (builder, "update_display", update_display);
      ctk_builder_add_callback_symbol (builder, "font_changed", font_changed);
      ctk_builder_add_callback_symbol (builder, "script_changed", script_changed);
      ctk_builder_add_callback_symbol (builder, "reset", reset_features);
      ctk_builder_add_callback_symbol (builder, "switch_to_entry", switch_to_entry);
      ctk_builder_add_callback_symbol (builder, "switch_to_label", switch_to_label);
      ctk_builder_add_callback_symbol (builder, "entry_key_press", G_CALLBACK (entry_key_press));
      ctk_builder_connect_signals (builder, NULL);

      window = CTK_WIDGET (ctk_builder_get_object (builder, "window"));
      label = CTK_WIDGET (ctk_builder_get_object (builder, "label"));
      settings = CTK_WIDGET (ctk_builder_get_object (builder, "settings"));
      resetbutton = CTK_WIDGET (ctk_builder_get_object (builder, "reset"));
      font = CTK_WIDGET (ctk_builder_get_object (builder, "font"));
      script_lang = CTK_WIDGET (ctk_builder_get_object (builder, "script_lang"));
      numcasedefault = CTK_WIDGET (ctk_builder_get_object (builder, "numcasedefault"));
      numspacedefault = CTK_WIDGET (ctk_builder_get_object (builder, "numspacedefault"));
      fractiondefault = CTK_WIDGET (ctk_builder_get_object (builder, "fractiondefault"));
      stack = CTK_WIDGET (ctk_builder_get_object (builder, "stack"));
      entry = CTK_WIDGET (ctk_builder_get_object (builder, "entry"));

      for (i = 0; i < num_features; i++)
        {
          char *iname;

          toggle[i] = CTK_WIDGET (ctk_builder_get_object (builder, feature_names[i]));
          iname = g_strconcat (feature_names[i], "_pres", NULL);
          icon[i] = CTK_WIDGET (ctk_builder_get_object (builder, iname));
          g_free (iname);
        }

      font_changed ();

      g_signal_connect (window, "destroy",
                        G_CALLBACK (ctk_widget_destroyed), &window);

      g_object_unref (builder);
    }

  if (!ctk_widget_get_visible (window))
    ctk_window_present (CTK_WINDOW (window));
  else
    ctk_widget_destroy (window);

  return window;
}
