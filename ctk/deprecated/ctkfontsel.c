/* CTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * Massively updated for Pango by Owen Taylor, May 2000
 * CtkFontSelection widget for Ctk+, by Damon Chaplin, May 1998.
 * Based on the GnomeFontSelector widget, by Elliot Lee, but major changes.
 * The GnomeFontSelector was derived from app/text_tool.c in the GIMP.
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

/*
 * Modified by the CTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/. 
 */

#include "config.h"

#define CDK_DISABLE_DEPRECATION_WARNINGS

#include <stdlib.h>
#include <glib/gprintf.h>
#include <string.h>

#include <atk/atk.h>

#include "ctkbutton.h"
#include "ctkcellrenderertext.h"
#include "ctkentry.h"
#include "ctkframe.h"
#include "ctklabel.h"
#include "ctkliststore.h"
#include "ctkstock.h"
#include "ctktable.h"
#include "ctktreeselection.h"
#include "ctktreeview.h"
#include "ctkscrolledwindow.h"
#include "ctkintl.h"
#include "ctkaccessible.h"
#include "ctkbuildable.h"
#include "ctkorientable.h"
#include "ctkprivate.h"
#include "ctkfontsel.h"

/**
 * SECTION:ctkfontsel
 * @Short_description: Deprecated widget for selecting fonts
 * @Title: CtkFontSelection
 * @See_also: #CtkFontSelectionDialog, #CtkFontChooser
 *
 * The #CtkFontSelection widget lists the available fonts, styles and sizes,
 * allowing the user to select a font.
 * It is used in the #CtkFontSelectionDialog widget to provide a dialog box for
 * selecting fonts.
 *
 * To set the font which is initially selected, use
 * ctk_font_selection_set_font_name().
 *
 * To get the selected font use ctk_font_selection_get_font_name().
 *
 * To change the text which is shown in the preview area, use
 * ctk_font_selection_set_preview_text().
 *
 * In CTK+ 3.2, #CtkFontSelection has been deprecated in favor of
 * #CtkFontChooser.
 */


struct _CtkFontSelectionPrivate
{
  CtkWidget *font_entry;        /* Used _get_family_entry() for consistency, -mr */
  CtkWidget *font_style_entry;  /* Used _get_face_entry() for consistency, -mr */

  CtkWidget *size_entry;
  CtkWidget *preview_entry;

  CtkWidget *family_list;
  CtkWidget *face_list;
  CtkWidget *size_list;

  PangoFontFamily *family;      /* Current family */
  PangoFontFace *face;          /* Current face */

  gint size;
};


struct _CtkFontSelectionDialogPrivate
{
  CtkWidget *fontsel;

  CtkWidget *ok_button;
  CtkWidget *apply_button;
  CtkWidget *cancel_button;
};


/* We don't enable the font and style entries because they don't add
 * much in terms of visible effect and have a weird effect on keynav.
 * the Windows font selector has entries similarly positioned but they
 * act in conjunction with the associated lists to form a single focus
 * location.
 */
#undef INCLUDE_FONT_ENTRIES

/* This is the default text shown in the preview entry, though the user
   can set it. Remember that some fonts only have capital letters. */
#define PREVIEW_TEXT N_("abcdefghijk ABCDEFGHIJK")

#define DEFAULT_FONT_NAME "Sans 10"

/* This is the initial and maximum height of the preview entry (it expands
   when large font sizes are selected). Initial height is also the minimum. */
#define INITIAL_PREVIEW_HEIGHT 44
#define MAX_PREVIEW_HEIGHT 300

/* These are the sizes of the font, style & size lists. */
#define FONT_LIST_HEIGHT	136
#define FONT_LIST_WIDTH		190
#define FONT_STYLE_LIST_WIDTH	170
#define FONT_SIZE_LIST_WIDTH	60

/* These are what we use as the standard font sizes, for the size list.
 */
static const guint16 font_sizes[] = {
  6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 20, 22, 24, 26, 28,
  32, 36, 40, 48, 56, 64, 72
};

enum {
   PROP_0,
   PROP_FONT_NAME,
   PROP_PREVIEW_TEXT
};


enum {
  FAMILY_COLUMN,
  FAMILY_NAME_COLUMN
};

enum {
  FACE_COLUMN,
  FACE_NAME_COLUMN
};

enum {
  SIZE_COLUMN
};

static void    ctk_font_selection_set_property       (GObject         *object,
						      guint            prop_id,
						      const GValue    *value,
						      GParamSpec      *pspec);
static void    ctk_font_selection_get_property       (GObject         *object,
						      guint            prop_id,
						      GValue          *value,
						      GParamSpec      *pspec);
static void    ctk_font_selection_finalize	     (GObject         *object);
static void    ctk_font_selection_screen_changed     (CtkWidget	      *widget,
						      CdkScreen       *previous_screen);
static void    ctk_font_selection_style_updated      (CtkWidget      *widget);

/* These are the callbacks & related functions. */
static void     ctk_font_selection_select_font           (CtkTreeSelection *selection,
							  gpointer          data);
static void     ctk_font_selection_show_available_fonts  (CtkFontSelection *fs);

static void     ctk_font_selection_show_available_styles (CtkFontSelection *fs);
static void     ctk_font_selection_select_best_style     (CtkFontSelection *fs,
							  gboolean          use_first);
static void     ctk_font_selection_select_style          (CtkTreeSelection *selection,
							  gpointer          data);

static void     ctk_font_selection_select_best_size      (CtkFontSelection *fs);
static void     ctk_font_selection_show_available_sizes  (CtkFontSelection *fs,
							  gboolean          first_time);
static void     ctk_font_selection_size_activate         (CtkWidget        *w,
							  gpointer          data);
static gboolean ctk_font_selection_size_focus_out        (CtkWidget        *w,
							  CdkEventFocus    *event,
							  gpointer          data);
static void     ctk_font_selection_select_size           (CtkTreeSelection *selection,
							  gpointer          data);

static void     ctk_font_selection_scroll_on_map         (CtkWidget        *w,
							  gpointer          data);

static void     ctk_font_selection_preview_changed       (CtkWidget        *entry,
							  CtkFontSelection *fontsel);
static void     ctk_font_selection_scroll_to_selection   (CtkFontSelection *fontsel);


/* Misc. utility functions. */
static void    ctk_font_selection_load_font          (CtkFontSelection *fs);
static void    ctk_font_selection_update_preview     (CtkFontSelection *fs);

static PangoFontDescription *ctk_font_selection_get_font_description (CtkFontSelection *fontsel);
static gboolean ctk_font_selection_select_font_desc  (CtkFontSelection      *fontsel,
						      PangoFontDescription  *new_desc,
						      PangoFontFamily      **pfamily,
						      PangoFontFace        **pface);
static void     ctk_font_selection_reload_fonts          (CtkFontSelection *fontsel);
static void     ctk_font_selection_ref_family            (CtkFontSelection *fontsel,
							  PangoFontFamily  *family);
static void     ctk_font_selection_ref_face              (CtkFontSelection *fontsel,
							  PangoFontFace    *face);

G_DEFINE_TYPE_WITH_PRIVATE (CtkFontSelection, ctk_font_selection, CTK_TYPE_BOX)

static void
ctk_font_selection_class_init (CtkFontSelectionClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);

  gobject_class->finalize = ctk_font_selection_finalize;
  gobject_class->set_property = ctk_font_selection_set_property;
  gobject_class->get_property = ctk_font_selection_get_property;

  widget_class->screen_changed = ctk_font_selection_screen_changed;
  widget_class->style_updated = ctk_font_selection_style_updated;
   
  g_object_class_install_property (gobject_class,
                                   PROP_FONT_NAME,
                                   g_param_spec_string ("font-name",
                                                        P_("Font name"),
                                                        P_("The string that represents this font"),
                                                        DEFAULT_FONT_NAME,
                                                        CTK_PARAM_READWRITE));
  g_object_class_install_property (gobject_class,
                                   PROP_PREVIEW_TEXT,
                                   g_param_spec_string ("preview-text",
                                                        P_("Preview text"),
                                                        P_("The text to display in order to demonstrate the selected font"),
                                                        _(PREVIEW_TEXT),
                                                        CTK_PARAM_READWRITE));
}

static void 
ctk_font_selection_set_property (GObject         *object,
				 guint            prop_id,
				 const GValue    *value,
				 GParamSpec      *pspec)
{
  CtkFontSelection *fontsel;

  fontsel = CTK_FONT_SELECTION (object);

  switch (prop_id)
    {
    case PROP_FONT_NAME:
      ctk_font_selection_set_font_name (fontsel, g_value_get_string (value));
      break;
    case PROP_PREVIEW_TEXT:
      ctk_font_selection_set_preview_text (fontsel, g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void ctk_font_selection_get_property (GObject         *object,
					     guint            prop_id,
					     GValue          *value,
					     GParamSpec      *pspec)
{
  CtkFontSelection *fontsel;

  fontsel = CTK_FONT_SELECTION (object);

  switch (prop_id)
    {
    case PROP_FONT_NAME:
      g_value_take_string (value, ctk_font_selection_get_font_name (fontsel));
      break;
    case PROP_PREVIEW_TEXT:
      g_value_set_string (value, ctk_font_selection_get_preview_text (fontsel));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/* Handles key press events on the lists, so that we can trap Enter to
 * activate the default button on our own.
 */
static gboolean
list_row_activated (CtkWidget *widget)
{
  CtkWidget *default_widget, *focus_widget;
  CtkWindow *window;
  
  window = CTK_WINDOW (ctk_widget_get_toplevel (CTK_WIDGET (widget)));
  if (!ctk_widget_is_toplevel (CTK_WIDGET (window)))
    window = NULL;

  if (window)
    {
      default_widget = ctk_window_get_default_widget (window);
      focus_widget = ctk_window_get_focus (window);

      if (widget != default_widget &&
          !(widget == focus_widget && (!default_widget || !ctk_widget_get_sensitive (default_widget))))
        ctk_window_activate_default (window);
    }

  return TRUE;
}

static void
ctk_font_selection_init (CtkFontSelection *fontsel)
{
  CtkFontSelectionPrivate *priv;
  CtkWidget *scrolled_win;
  CtkWidget *text_box;
  CtkWidget *table, *label;
  CtkWidget *font_label, *style_label;
  CtkWidget *vbox;
  CtkListStore *model;
  CtkTreeViewColumn *column;
  GList *focus_chain = NULL;
  AtkObject *atk_obj;

  fontsel->priv = ctk_font_selection_get_instance_private (fontsel);
  priv = fontsel->priv;

  ctk_orientable_set_orientation (CTK_ORIENTABLE (fontsel),
                                  CTK_ORIENTATION_VERTICAL);

  ctk_widget_push_composite_child ();

  ctk_box_set_spacing (CTK_BOX (fontsel), 12);
  priv->size = 12 * PANGO_SCALE;
  
  /* Create the table of font, style & size. */
  table = ctk_table_new (3, 3, FALSE);
  ctk_widget_show (table);
  ctk_table_set_row_spacings (CTK_TABLE (table), 6);
  ctk_table_set_col_spacings (CTK_TABLE (table), 12);
  ctk_box_pack_start (CTK_BOX (fontsel), table, TRUE, TRUE, 0);

#ifdef INCLUDE_FONT_ENTRIES
  priv->font_entry = ctk_entry_new ();
  ctk_editable_set_editable (CTK_EDITABLE (priv->font_entry), FALSE);
  ctk_widget_set_size_request (priv->font_entry, 20, -1);
  ctk_widget_show (priv->font_entry);
  ctk_table_attach (CTK_TABLE (table), priv->font_entry, 0, 1, 1, 2,
		    CTK_FILL, 0, 0, 0);
  
  priv->font_style_entry = ctk_entry_new ();
  ctk_editable_set_editable (CTK_EDITABLE (priv->font_style_entry), FALSE);
  ctk_widget_set_size_request (priv->font_style_entry, 20, -1);
  ctk_widget_show (priv->font_style_entry);
  ctk_table_attach (CTK_TABLE (table), priv->font_style_entry, 1, 2, 1, 2,
		    CTK_FILL, 0, 0, 0);
#endif /* INCLUDE_FONT_ENTRIES */
  
  priv->size_entry = ctk_entry_new ();
  ctk_widget_set_size_request (priv->size_entry, 20, -1);
  ctk_widget_show (priv->size_entry);
  ctk_table_attach (CTK_TABLE (table), priv->size_entry, 2, 3, 1, 2,
		    CTK_FILL, 0, 0, 0);
  g_signal_connect (priv->size_entry, "activate",
		    G_CALLBACK (ctk_font_selection_size_activate),
		    fontsel);
  g_signal_connect_after (priv->size_entry, "focus-out-event",
			  G_CALLBACK (ctk_font_selection_size_focus_out),
			  fontsel);
  
  font_label = ctk_label_new_with_mnemonic (_("_Family:"));
  ctk_widget_set_halign (font_label, CTK_ALIGN_START);
  ctk_widget_set_valign (font_label, CTK_ALIGN_CENTER);
  ctk_widget_show (font_label);
  ctk_table_attach (CTK_TABLE (table), font_label, 0, 1, 0, 1,
		    CTK_FILL, 0, 0, 0);  

  style_label = ctk_label_new_with_mnemonic (_("_Style:"));
  ctk_widget_set_halign (style_label, CTK_ALIGN_START);
  ctk_widget_set_valign (style_label, CTK_ALIGN_CENTER);
  ctk_widget_show (style_label);
  ctk_table_attach (CTK_TABLE (table), style_label, 1, 2, 0, 1,
		    CTK_FILL, 0, 0, 0);
  
  label = ctk_label_new_with_mnemonic (_("Si_ze:"));
  ctk_label_set_mnemonic_widget (CTK_LABEL (label),
                                 priv->size_entry);
  ctk_widget_set_halign (label, CTK_ALIGN_START);
  ctk_widget_set_valign (label, CTK_ALIGN_CENTER);
  ctk_widget_show (label);
  ctk_table_attach (CTK_TABLE (table), label, 2, 3, 0, 1,
		    CTK_FILL, 0, 0, 0);
  
  
  /* Create the lists  */

  model = ctk_list_store_new (2,
			      G_TYPE_OBJECT,  /* FAMILY_COLUMN */
			      G_TYPE_STRING); /* FAMILY_NAME_COLUMN */
  priv->family_list = ctk_tree_view_new_with_model (CTK_TREE_MODEL (model));
  g_object_unref (model);

  g_signal_connect (priv->family_list, "row-activated",
		    G_CALLBACK (list_row_activated), fontsel);

  column = ctk_tree_view_column_new_with_attributes ("Family",
						     ctk_cell_renderer_text_new (),
						     "text", FAMILY_NAME_COLUMN,
						     NULL);
  ctk_tree_view_column_set_sizing (column, CTK_TREE_VIEW_COLUMN_AUTOSIZE);
  ctk_tree_view_append_column (CTK_TREE_VIEW (priv->family_list), column);

  ctk_tree_view_set_headers_visible (CTK_TREE_VIEW (priv->family_list), FALSE);
  ctk_tree_selection_set_mode (ctk_tree_view_get_selection (CTK_TREE_VIEW (priv->family_list)),
			       CTK_SELECTION_BROWSE);
  
  ctk_label_set_mnemonic_widget (CTK_LABEL (font_label), priv->family_list);

  scrolled_win = ctk_scrolled_window_new (NULL, NULL);
  ctk_scrolled_window_set_shadow_type (CTK_SCROLLED_WINDOW (scrolled_win), CTK_SHADOW_IN);
  ctk_widget_set_size_request (scrolled_win,
			       FONT_LIST_WIDTH, FONT_LIST_HEIGHT);
  ctk_container_add (CTK_CONTAINER (scrolled_win), priv->family_list);
  ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (scrolled_win),
				  CTK_POLICY_AUTOMATIC, CTK_POLICY_ALWAYS);
  ctk_widget_show (priv->family_list);
  ctk_widget_show (scrolled_win);

  ctk_table_attach (CTK_TABLE (table), scrolled_win, 0, 1, 1, 3,
		    CTK_EXPAND | CTK_FILL,
		    CTK_EXPAND | CTK_FILL, 0, 0);
  focus_chain = g_list_append (focus_chain, scrolled_win);
  
  model = ctk_list_store_new (2,
			      G_TYPE_OBJECT,  /* FACE_COLUMN */
			      G_TYPE_STRING); /* FACE_NAME_COLUMN */
  priv->face_list = ctk_tree_view_new_with_model (CTK_TREE_MODEL (model));
  g_object_unref (model);
  g_signal_connect (priv->face_list, "row-activated",
		    G_CALLBACK (list_row_activated), fontsel);

  ctk_label_set_mnemonic_widget (CTK_LABEL (style_label), priv->face_list);

  column = ctk_tree_view_column_new_with_attributes ("Face",
						     ctk_cell_renderer_text_new (),
						     "text", FACE_NAME_COLUMN,
						     NULL);
  ctk_tree_view_column_set_sizing (column, CTK_TREE_VIEW_COLUMN_AUTOSIZE);
  ctk_tree_view_append_column (CTK_TREE_VIEW (priv->face_list), column);

  ctk_tree_view_set_headers_visible (CTK_TREE_VIEW (priv->face_list), FALSE);
  ctk_tree_selection_set_mode (ctk_tree_view_get_selection (CTK_TREE_VIEW (priv->face_list)),
			       CTK_SELECTION_BROWSE);
  
  scrolled_win = ctk_scrolled_window_new (NULL, NULL);
  ctk_scrolled_window_set_shadow_type (CTK_SCROLLED_WINDOW (scrolled_win), CTK_SHADOW_IN);
  ctk_widget_set_size_request (scrolled_win,
			       FONT_STYLE_LIST_WIDTH, FONT_LIST_HEIGHT);
  ctk_container_add (CTK_CONTAINER (scrolled_win), priv->face_list);
  ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (scrolled_win),
				  CTK_POLICY_AUTOMATIC, CTK_POLICY_ALWAYS);
  ctk_widget_show (priv->face_list);
  ctk_widget_show (scrolled_win);
  ctk_table_attach (CTK_TABLE (table), scrolled_win, 1, 2, 1, 3,
		    CTK_EXPAND | CTK_FILL,
		    CTK_EXPAND | CTK_FILL, 0, 0);
  focus_chain = g_list_append (focus_chain, scrolled_win);
  
  focus_chain = g_list_append (focus_chain, priv->size_entry);

  model = ctk_list_store_new (1, G_TYPE_INT);
  priv->size_list = ctk_tree_view_new_with_model (CTK_TREE_MODEL (model));
  g_object_unref (model);
  g_signal_connect (priv->size_list, "row-activated",
		    G_CALLBACK (list_row_activated), fontsel);

  column = ctk_tree_view_column_new_with_attributes ("Size",
						     ctk_cell_renderer_text_new (),
						     "text", SIZE_COLUMN,
						     NULL);
  ctk_tree_view_column_set_sizing (column, CTK_TREE_VIEW_COLUMN_AUTOSIZE);
  ctk_tree_view_append_column (CTK_TREE_VIEW (priv->size_list), column);

  ctk_tree_view_set_headers_visible (CTK_TREE_VIEW (priv->size_list), FALSE);
  ctk_tree_selection_set_mode (ctk_tree_view_get_selection (CTK_TREE_VIEW (priv->size_list)),
			       CTK_SELECTION_BROWSE);
  
  scrolled_win = ctk_scrolled_window_new (NULL, NULL);
  ctk_scrolled_window_set_shadow_type (CTK_SCROLLED_WINDOW (scrolled_win), CTK_SHADOW_IN);
  ctk_container_add (CTK_CONTAINER (scrolled_win), priv->size_list);
  ctk_widget_set_size_request (scrolled_win, -1, FONT_LIST_HEIGHT);
  ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (scrolled_win),
				  CTK_POLICY_NEVER, CTK_POLICY_ALWAYS);
  ctk_widget_show (priv->size_list);
  ctk_widget_show (scrolled_win);
  ctk_table_attach (CTK_TABLE (table), scrolled_win, 2, 3, 2, 3,
		    CTK_FILL, CTK_EXPAND | CTK_FILL, 0, 0);
  focus_chain = g_list_append (focus_chain, scrolled_win);

  ctk_container_set_focus_chain (CTK_CONTAINER (table), focus_chain);
  g_list_free (focus_chain);
  
  /* Insert the fonts. */
  g_signal_connect (ctk_tree_view_get_selection (CTK_TREE_VIEW (priv->family_list)), "changed",
		    G_CALLBACK (ctk_font_selection_select_font), fontsel);

  g_signal_connect_after (priv->family_list, "map",
			  G_CALLBACK (ctk_font_selection_scroll_on_map),
			  fontsel);
  
  g_signal_connect (ctk_tree_view_get_selection (CTK_TREE_VIEW (priv->face_list)), "changed",
		    G_CALLBACK (ctk_font_selection_select_style), fontsel);

  g_signal_connect (ctk_tree_view_get_selection (CTK_TREE_VIEW (priv->size_list)), "changed",
		    G_CALLBACK (ctk_font_selection_select_size), fontsel);
  atk_obj = ctk_widget_get_accessible (priv->size_list);
  if (CTK_IS_ACCESSIBLE (atk_obj))
    {
      /* Accessibility support is enabled.
       * Make the label ATK_RELATON_LABEL_FOR for the size list as well.
       */
      AtkObject *atk_label;
      AtkRelationSet *relation_set;
      AtkRelation *relation;
      AtkObject *obj_array[1];

      atk_label = ctk_widget_get_accessible (label);
      relation_set = atk_object_ref_relation_set (atk_obj);
      relation = atk_relation_set_get_relation_by_type (relation_set, ATK_RELATION_LABELLED_BY);
      if (relation)
        {
          atk_relation_add_target (relation, atk_label);
        }
      else 
        {
          obj_array[0] = atk_label;
          relation = atk_relation_new (obj_array, 1, ATK_RELATION_LABELLED_BY);
          atk_relation_set_add (relation_set, relation);
        }
      g_object_unref (relation_set);

      relation_set = atk_object_ref_relation_set (atk_label);
      relation = atk_relation_set_get_relation_by_type (relation_set, ATK_RELATION_LABEL_FOR);
      if (relation)
        {
          atk_relation_add_target (relation, atk_obj);
        }
      else 
        {
          obj_array[0] = atk_obj;
          relation = atk_relation_new (obj_array, 1, ATK_RELATION_LABEL_FOR);
          atk_relation_set_add (relation_set, relation);
        }
      g_object_unref (relation_set);
    }

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 6);
  ctk_widget_show (vbox);
  ctk_box_pack_start (CTK_BOX (fontsel), vbox, FALSE, TRUE, 0);
  
  /* create the text entry widget */
  label = ctk_label_new_with_mnemonic (_("_Preview:"));
  ctk_widget_set_halign (label, CTK_ALIGN_START);
  ctk_widget_set_valign (label, CTK_ALIGN_CENTER);
  ctk_widget_show (label);
  ctk_box_pack_start (CTK_BOX (vbox), label, FALSE, TRUE, 0);

  text_box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
  ctk_widget_show (text_box);
  ctk_box_pack_start (CTK_BOX (vbox), text_box, FALSE, TRUE, 0);
  
  priv->preview_entry = ctk_entry_new ();
  ctk_label_set_mnemonic_widget (CTK_LABEL (label), priv->preview_entry);
  ctk_entry_set_text (CTK_ENTRY (priv->preview_entry), _(PREVIEW_TEXT));
  
  ctk_widget_show (priv->preview_entry);
  g_signal_connect (priv->preview_entry, "changed",
		    G_CALLBACK (ctk_font_selection_preview_changed), fontsel);
  ctk_widget_set_size_request (priv->preview_entry,
			       -1, INITIAL_PREVIEW_HEIGHT);
  ctk_box_pack_start (CTK_BOX (text_box), priv->preview_entry,
		      TRUE, TRUE, 0);
  ctk_widget_pop_composite_child();
}

/**
 * ctk_font_selection_new:
 *
 * Creates a new #CtkFontSelection.
 *
 * Returns: a new #CtkFontSelection
 *
 * Deprecated: 3.2: Use #CtkFontChooserWidget instead
 */
CtkWidget *
ctk_font_selection_new (void)
{
  CtkFontSelection *fontsel;
  
  fontsel = g_object_new (CTK_TYPE_FONT_SELECTION, NULL);
  
  return CTK_WIDGET (fontsel);
}

static void
ctk_font_selection_finalize (GObject *object)
{
  CtkFontSelection *fontsel = CTK_FONT_SELECTION (object);

  ctk_font_selection_ref_family (fontsel, NULL);
  ctk_font_selection_ref_face (fontsel, NULL);

  G_OBJECT_CLASS (ctk_font_selection_parent_class)->finalize (object);
}

static void
ctk_font_selection_ref_family (CtkFontSelection *fontsel,
			       PangoFontFamily  *family)
{
  CtkFontSelectionPrivate *priv = fontsel->priv;

  if (family)
    family = g_object_ref (family);
  if (priv->family)
    g_object_unref (priv->family);
  priv->family = family;
}

static void ctk_font_selection_ref_face (CtkFontSelection *fontsel,
					 PangoFontFace    *face)
{
  CtkFontSelectionPrivate *priv = fontsel->priv;

  if (face)
    face = g_object_ref (face);
  if (priv->face)
    g_object_unref (priv->face);
  priv->face = face;
}

static void
ctk_font_selection_reload_fonts (CtkFontSelection *fontsel)
{
  if (ctk_widget_has_screen (CTK_WIDGET (fontsel)))
    {
      PangoFontDescription *desc;
      desc = ctk_font_selection_get_font_description (fontsel);

      ctk_font_selection_show_available_fonts (fontsel);
      ctk_font_selection_show_available_sizes (fontsel, TRUE);
      ctk_font_selection_show_available_styles (fontsel);

      ctk_font_selection_select_font_desc (fontsel, desc, NULL, NULL);
      ctk_font_selection_scroll_to_selection (fontsel);

      pango_font_description_free (desc);
    }
}

static void
ctk_font_selection_screen_changed (CtkWidget *widget,
				   CdkScreen *previous_screen G_GNUC_UNUSED)
{
  ctk_font_selection_reload_fonts (CTK_FONT_SELECTION (widget));
}

static void
ctk_font_selection_style_updated (CtkWidget *widget)
{
  CTK_WIDGET_CLASS (ctk_font_selection_parent_class)->style_updated (widget);

  /* Maybe fonts where installed or removed... */
  ctk_font_selection_reload_fonts (CTK_FONT_SELECTION (widget));
}

static void
ctk_font_selection_preview_changed (CtkWidget        *entry G_GNUC_UNUSED,
				    CtkFontSelection *fontsel)
{
  g_object_notify (G_OBJECT (fontsel), "preview-text");
}

static void
scroll_to_selection (CtkTreeView *tree_view)
{
  CtkTreeSelection *selection = ctk_tree_view_get_selection (tree_view);
  CtkTreeModel *model;
  CtkTreeIter iter;

  if (ctk_tree_selection_get_selected (selection, &model, &iter))
    {
      CtkTreePath *path = ctk_tree_model_get_path (model, &iter);
      ctk_tree_view_scroll_to_cell (tree_view, path, NULL, TRUE, 0.5, 0.5);
      ctk_tree_path_free (path);
    }
}

static void
set_cursor_to_iter (CtkTreeView *view,
		    CtkTreeIter *iter)
{
  CtkTreeModel *model = ctk_tree_view_get_model (view);
  CtkTreePath *path = ctk_tree_model_get_path (model, iter);
  
  ctk_tree_view_set_cursor (view, path, NULL, FALSE);

  ctk_tree_path_free (path);
}

static void
ctk_font_selection_scroll_to_selection (CtkFontSelection *fontsel)
{
  CtkFontSelectionPrivate *priv = fontsel->priv;

  /* Try to scroll the font family list to the selected item */
  scroll_to_selection (CTK_TREE_VIEW (priv->family_list));

  /* Try to scroll the font family list to the selected item */
  scroll_to_selection (CTK_TREE_VIEW (priv->face_list));

  /* Try to scroll the font family list to the selected item */
  scroll_to_selection (CTK_TREE_VIEW (priv->size_list));
/* This is called when the list is mapped. Here we scroll to the current
   font if necessary. */
}

static void
ctk_font_selection_scroll_on_map (CtkWidget *widget G_GNUC_UNUSED,
                                  gpointer   data)
{
  ctk_font_selection_scroll_to_selection (CTK_FONT_SELECTION (data));
}

/* This is called when a family is selected in the list. */
static void
ctk_font_selection_select_font (CtkTreeSelection *selection,
				gpointer          data)
{
  CtkFontSelection *fontsel;
  CtkFontSelectionPrivate *priv;
  CtkTreeModel *model;
  CtkTreeIter iter;
#ifdef INCLUDE_FONT_ENTRIES
  const gchar *family_name;
#endif

  fontsel = CTK_FONT_SELECTION (data);
  priv = fontsel->priv;

  if (ctk_tree_selection_get_selected (selection, &model, &iter))
    {
      PangoFontFamily *family;

      ctk_tree_model_get (model, &iter, FAMILY_COLUMN, &family, -1);
      if (priv->family != family)
	{
	  ctk_font_selection_ref_family (fontsel, family);

#ifdef INCLUDE_FONT_ENTRIES
	  family_name = pango_font_family_get_name (priv->family);
	  ctk_entry_set_text (CTK_ENTRY (priv->font_entry), family_name);
#endif

	  ctk_font_selection_show_available_styles (fontsel);
	  ctk_font_selection_select_best_style (fontsel, TRUE);
	}

      g_object_unref (family);
    }
}

static int
cmp_families (const void *a, const void *b)
{
  const char *a_name = pango_font_family_get_name (*(PangoFontFamily **)a);
  const char *b_name = pango_font_family_get_name (*(PangoFontFamily **)b);
  
  return g_utf8_collate (a_name, b_name);
}

static void
ctk_font_selection_show_available_fonts (CtkFontSelection *fontsel)
{
  CtkFontSelectionPrivate *priv = fontsel->priv;
  CtkListStore *model;
  PangoFontFamily **families;
  PangoFontFamily *match_family = NULL;
  gint n_families, i;
  CtkTreeIter match_row;

  model = CTK_LIST_STORE (ctk_tree_view_get_model (CTK_TREE_VIEW (priv->family_list)));

  pango_context_list_families (ctk_widget_get_pango_context (CTK_WIDGET (fontsel)),
			       &families, &n_families);
  qsort (families, n_families, sizeof (PangoFontFamily *), cmp_families);

  ctk_list_store_clear (model);

  for (i=0; i<n_families; i++)
    {
      const gchar *name = pango_font_family_get_name (families[i]);
      CtkTreeIter iter;

      ctk_list_store_insert_with_values (model, &iter, -1,
                                         FAMILY_COLUMN, families[i],
                                         FAMILY_NAME_COLUMN, name,
                                         -1);

      if (i == 0 || !g_ascii_strcasecmp (name, "sans"))
	{
	  match_family = families[i];
	  match_row = iter;
	}
    }

  ctk_font_selection_ref_family (fontsel, match_family);
  if (match_family)
    {
      set_cursor_to_iter (CTK_TREE_VIEW (priv->family_list), &match_row);
#ifdef INCLUDE_FONT_ENTRIES
      ctk_entry_set_text (CTK_ENTRY (priv->font_entry), 
			  pango_font_family_get_name (match_family));
#endif /* INCLUDE_FONT_ENTRIES */
    }

  g_free (families);
}

static int
compare_font_descriptions (const PangoFontDescription *a, const PangoFontDescription *b)
{
  int val = strcmp (pango_font_description_get_family (a), pango_font_description_get_family (b));
  if (val != 0)
    return val;

  if (pango_font_description_get_weight (a) != pango_font_description_get_weight (b))
    return pango_font_description_get_weight (a) - pango_font_description_get_weight (b);

  if (pango_font_description_get_style (a) != pango_font_description_get_style (b))
    return pango_font_description_get_style (a) - pango_font_description_get_style (b);
  
  if (pango_font_description_get_stretch (a) != pango_font_description_get_stretch (b))
    return pango_font_description_get_stretch (a) - pango_font_description_get_stretch (b);

  if (pango_font_description_get_variant (a) != pango_font_description_get_variant (b))
    return pango_font_description_get_variant (a) - pango_font_description_get_variant (b);

  return 0;
}

static int
faces_sort_func (const void *a, const void *b)
{
  PangoFontDescription *desc_a = pango_font_face_describe (*(PangoFontFace **)a);
  PangoFontDescription *desc_b = pango_font_face_describe (*(PangoFontFace **)b);
  
  int ord = compare_font_descriptions (desc_a, desc_b);

  pango_font_description_free (desc_a);
  pango_font_description_free (desc_b);

  return ord;
}

static gboolean
font_description_style_equal (const PangoFontDescription *a,
			      const PangoFontDescription *b)
{
  return (pango_font_description_get_weight (a) == pango_font_description_get_weight (b) &&
	  pango_font_description_get_style (a) == pango_font_description_get_style (b) &&
	  pango_font_description_get_stretch (a) == pango_font_description_get_stretch (b) &&
	  pango_font_description_get_variant (a) == pango_font_description_get_variant (b));
}

/* This fills the font style list with all the possible style combinations
   for the current font family. */
static void
ctk_font_selection_show_available_styles (CtkFontSelection *fontsel)
{
  CtkFontSelectionPrivate *priv = fontsel->priv;
  gint n_faces, i;
  PangoFontFace **faces;
  PangoFontDescription *old_desc;
  CtkListStore *model;
  CtkTreeIter match_row;
  PangoFontFace *match_face = NULL;

  model = CTK_LIST_STORE (ctk_tree_view_get_model (CTK_TREE_VIEW (priv->face_list)));

  if (priv->face)
    old_desc = pango_font_face_describe (priv->face);
  else
    old_desc= NULL;

  pango_font_family_list_faces (priv->family, &faces, &n_faces);
  qsort (faces, n_faces, sizeof (PangoFontFace *), faces_sort_func);

  ctk_list_store_clear (model);

  for (i=0; i < n_faces; i++)
    {
      CtkTreeIter iter;
      const gchar *str = pango_font_face_get_face_name (faces[i]);

      ctk_list_store_insert_with_values (model, &iter, -1,
                                         FACE_COLUMN, faces[i],
                                         FACE_NAME_COLUMN, str,
                                         -1);

      if (i == 0)
	{
	  match_row = iter;
	  match_face = faces[i];
	}
      else if (old_desc)
	{
	  PangoFontDescription *tmp_desc = pango_font_face_describe (faces[i]);
	  
	  if (font_description_style_equal (tmp_desc, old_desc))
	    {
	      match_row = iter;
	      match_face = faces[i];
	    }
      
	  pango_font_description_free (tmp_desc);
	}
    }

  if (old_desc)
    pango_font_description_free (old_desc);

  ctk_font_selection_ref_face (fontsel, match_face);
  if (match_face)
    {
#ifdef INCLUDE_FONT_ENTRIES
      const gchar *str = pango_font_face_get_face_name (priv->face);

      ctk_entry_set_text (CTK_ENTRY (priv->font_style_entry), str);
#endif
      set_cursor_to_iter (CTK_TREE_VIEW (priv->face_list), &match_row);
    }

  g_free (faces);
}

/* This selects a style when the user selects a font. It just uses the first
   available style at present. I was thinking of trying to maintain the
   selected style, e.g. bold italic, when the user selects different fonts.
   However, the interface is so easy to use now I'm not sure it's worth it.
   Note: This will load a font. */
static void
ctk_font_selection_select_best_style (CtkFontSelection *fontsel,
				      gboolean          use_first G_GNUC_UNUSED)
{
  CtkFontSelectionPrivate *priv = fontsel->priv;
  CtkTreeIter iter;
  CtkTreeModel *model;

  model = ctk_tree_view_get_model (CTK_TREE_VIEW (priv->face_list));

  if (ctk_tree_model_get_iter_first (model, &iter))
    {
      set_cursor_to_iter (CTK_TREE_VIEW (priv->face_list), &iter);
      scroll_to_selection (CTK_TREE_VIEW (priv->face_list));
    }

  ctk_font_selection_show_available_sizes (fontsel, FALSE);
  ctk_font_selection_select_best_size (fontsel);
}


/* This is called when a style is selected in the list. */
static void
ctk_font_selection_select_style (CtkTreeSelection *selection,
				 gpointer          data)
{
  CtkFontSelection *fontsel = CTK_FONT_SELECTION (data);
  CtkTreeModel *model;
  CtkTreeIter iter;

  if (ctk_tree_selection_get_selected (selection, &model, &iter))
    {
      PangoFontFace *face;
      
      ctk_tree_model_get (model, &iter, FACE_COLUMN, &face, -1);
      ctk_font_selection_ref_face (fontsel, face);
      g_object_unref (face);
    }

  ctk_font_selection_show_available_sizes (fontsel, FALSE);
  ctk_font_selection_select_best_size (fontsel);
}

static void
ctk_font_selection_show_available_sizes (CtkFontSelection *fontsel,
					 gboolean          first_time)
{
  CtkFontSelectionPrivate *priv = fontsel->priv;
  gint i;
  CtkListStore *model;
  gchar buffer[128];
  gchar *p;

  model = CTK_LIST_STORE (ctk_tree_view_get_model (CTK_TREE_VIEW (priv->size_list)));

  /* Insert the standard font sizes */
  if (first_time)
    {
      ctk_list_store_clear (model);

      for (i = 0; i < G_N_ELEMENTS (font_sizes); i++)
	{
	  CtkTreeIter iter;

	  ctk_list_store_insert_with_values (model, &iter, -1,
	                                     SIZE_COLUMN, font_sizes[i], -1);

	  if (font_sizes[i] * PANGO_SCALE == priv->size)
	    set_cursor_to_iter (CTK_TREE_VIEW (priv->size_list), &iter);
	}
    }
  else
    {
      CtkTreeIter iter;
      gboolean found = FALSE;

      ctk_tree_model_get_iter_first (CTK_TREE_MODEL (model), &iter);
      for (i = 0; i < G_N_ELEMENTS (font_sizes) && !found; i++)
	{
	  if (font_sizes[i] * PANGO_SCALE == priv->size)
	    {
	      set_cursor_to_iter (CTK_TREE_VIEW (priv->size_list), &iter);
	      found = TRUE;
	    }

          if (!ctk_tree_model_iter_next (CTK_TREE_MODEL (model), &iter))
            break;
	}

      if (!found)
	{
	  CtkTreeSelection *selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (priv->size_list));
	  ctk_tree_selection_unselect_all (selection);
	}
    }

  /* Set the entry to the new size, rounding to 1 digit,
   * trimming of trailing 0's and a trailing period
   */
  g_snprintf (buffer, sizeof (buffer), "%.1f", priv->size / (1.0 * PANGO_SCALE));
  if (strchr (buffer, '.'))
    {
      p = buffer + strlen (buffer) - 1;
      while (*p == '0')
	p--;
      if (*p == '.')
	p--;
      p[1] = '\0';
    }

  /* Compare, to avoid moving the cursor unecessarily */
  if (strcmp (ctk_entry_get_text (CTK_ENTRY (priv->size_entry)), buffer) != 0)
    ctk_entry_set_text (CTK_ENTRY (priv->size_entry), buffer);
}

static void
ctk_font_selection_select_best_size (CtkFontSelection *fontsel)
{
  ctk_font_selection_load_font (fontsel);  
}

static void
ctk_font_selection_set_size (CtkFontSelection *fontsel,
			     gint              new_size)
{
  CtkFontSelectionPrivate *priv = fontsel->priv;

  if (priv->size != new_size)
    {
      priv->size = new_size;

      ctk_font_selection_show_available_sizes (fontsel, FALSE);      
      ctk_font_selection_load_font (fontsel);
    }
}

/* If the user hits return in the font size entry, we change to the new font
   size. */
static void
ctk_font_selection_size_activate (CtkWidget   *w,
                                  gpointer     data)
{
  CtkFontSelection *fontsel = CTK_FONT_SELECTION (data);
  CtkFontSelectionPrivate *priv = fontsel->priv;
  gint new_size;
  const gchar *text;

  text = ctk_entry_get_text (CTK_ENTRY (priv->size_entry));
  new_size = (int) MAX (0.1, atof (text) * PANGO_SCALE + 0.5);

  if (priv->size != new_size)
    ctk_font_selection_set_size (fontsel, new_size);
  else 
    list_row_activated (w);
}

static gboolean
ctk_font_selection_size_focus_out (CtkWidget     *w G_GNUC_UNUSED,
				   CdkEventFocus *event G_GNUC_UNUSED,
				   gpointer       data)
{
  CtkFontSelection *fontsel = CTK_FONT_SELECTION (data);
  CtkFontSelectionPrivate *priv = fontsel->priv;
  gint new_size;
  const gchar *text;

  text = ctk_entry_get_text (CTK_ENTRY (priv->size_entry));
  new_size = (int) MAX (0.1, atof (text) * PANGO_SCALE + 0.5);

  ctk_font_selection_set_size (fontsel, new_size);

  return TRUE;
}

/* This is called when a size is selected in the list. */
static void
ctk_font_selection_select_size (CtkTreeSelection *selection,
				gpointer          data)
{
  CtkFontSelection *fontsel = CTK_FONT_SELECTION (data);
  CtkTreeModel *model;
  CtkTreeIter iter;
  gint new_size;

  if (ctk_tree_selection_get_selected (selection, &model, &iter))
    {
      ctk_tree_model_get (model, &iter, SIZE_COLUMN, &new_size, -1);
      ctk_font_selection_set_size (fontsel, new_size * PANGO_SCALE);
    }
}

static void
ctk_font_selection_load_font (CtkFontSelection *fontsel)
{
  ctk_font_selection_update_preview (fontsel);
}

static PangoFontDescription *
ctk_font_selection_get_font_description (CtkFontSelection *fontsel)
{
  CtkFontSelectionPrivate *priv = fontsel->priv;
  PangoFontDescription *font_desc;

  if (priv->face)
    {
      font_desc = pango_font_face_describe (priv->face);
      pango_font_description_set_size (font_desc, priv->size);
    }
  else
    font_desc = pango_font_description_from_string (DEFAULT_FONT_NAME);

  return font_desc;
}

/* This sets the font in the preview entry to the selected font.
 */
static void
ctk_font_selection_update_preview (CtkFontSelection *fontsel)
{
  CtkFontSelectionPrivate *priv = fontsel->priv;
  CtkWidget *preview_entry = priv->preview_entry;
  const gchar *text;

  ctk_widget_override_font (preview_entry,
                            ctk_font_selection_get_font_description (fontsel));

  /* This sets the preview text, if it hasn't been set already. */
  text = ctk_entry_get_text (CTK_ENTRY (preview_entry));
  if (strlen (text) == 0)
    ctk_entry_set_text (CTK_ENTRY (preview_entry), _(PREVIEW_TEXT));
  ctk_editable_set_position (CTK_EDITABLE (preview_entry), 0);
}


/*****************************************************************************
 * These functions are the main public interface for getting/setting the font.
 *****************************************************************************/

/**
 * ctk_font_selection_get_family_list:
 * @fontsel: a #CtkFontSelection
 *
 * This returns the #CtkTreeView that lists font families, for
 * example, “Sans”, “Serif”, etc.
 *
 * Returns: (transfer none): A #CtkWidget that is part of @fontsel
 *
 * Since: 2.14
 *
 * Deprecated: 3.2: Use #CtkFontChooser
 */
CtkWidget *
ctk_font_selection_get_family_list (CtkFontSelection *fontsel)
{
  g_return_val_if_fail (CTK_IS_FONT_SELECTION (fontsel), NULL);

  return fontsel->priv->family_list;
}

/**
 * ctk_font_selection_get_face_list:
 * @fontsel: a #CtkFontSelection
 *
 * This returns the #CtkTreeView which lists all styles available for
 * the selected font. For example, “Regular”, “Bold”, etc.
 * 
 * Returns: (transfer none): A #CtkWidget that is part of @fontsel
 *
 * Since: 2.14
 *
 * Deprecated: 3.2: Use #CtkFontChooser
 */
CtkWidget *
ctk_font_selection_get_face_list (CtkFontSelection *fontsel)
{
  g_return_val_if_fail (CTK_IS_FONT_SELECTION (fontsel), NULL);

  return fontsel->priv->face_list;
}

/**
 * ctk_font_selection_get_size_entry:
 * @fontsel: a #CtkFontSelection
 *
 * This returns the #CtkEntry used to allow the user to edit the font
 * number manually instead of selecting it from the list of font sizes.
 *
 * Returns: (transfer none): A #CtkWidget that is part of @fontsel
 *
 * Since: 2.14
 *
 * Deprecated: 3.2: Use #CtkFontChooser
 */
CtkWidget *
ctk_font_selection_get_size_entry (CtkFontSelection *fontsel)
{
  g_return_val_if_fail (CTK_IS_FONT_SELECTION (fontsel), NULL);

  return fontsel->priv->size_entry;
}

/**
 * ctk_font_selection_get_size_list:
 * @fontsel: a #CtkFontSelection
 *
 * This returns the #CtkTreeView used to list font sizes.
 *
 * Returns: (transfer none): A #CtkWidget that is part of @fontsel
 *
 * Since: 2.14
 *
 * Deprecated: 3.2: Use #CtkFontChooser
 */
CtkWidget *
ctk_font_selection_get_size_list (CtkFontSelection *fontsel)
{
  g_return_val_if_fail (CTK_IS_FONT_SELECTION (fontsel), NULL);

  return fontsel->priv->size_list;
}

/**
 * ctk_font_selection_get_preview_entry:
 * @fontsel: a #CtkFontSelection
 *
 * This returns the #CtkEntry used to display the font as a preview.
 *
 * Returns: (transfer none): A #CtkWidget that is part of @fontsel
 *
 * Since: 2.14
 *
 * Deprecated: 3.2: Use #CtkFontChooser
 */
CtkWidget *
ctk_font_selection_get_preview_entry (CtkFontSelection *fontsel)
{
  g_return_val_if_fail (CTK_IS_FONT_SELECTION (fontsel), NULL);

  return fontsel->priv->preview_entry;
}

/**
 * ctk_font_selection_get_family:
 * @fontsel: a #CtkFontSelection
 *
 * Gets the #PangoFontFamily representing the selected font family.
 *
 * Returns: (transfer none): A #PangoFontFamily representing the
 *     selected font family. Font families are a collection of font
 *     faces. The returned object is owned by @fontsel and must not
 *     be modified or freed.
 *
 * Since: 2.14
 *
 * Deprecated: 3.2: Use #CtkFontChooser
 */
PangoFontFamily *
ctk_font_selection_get_family (CtkFontSelection *fontsel)
{
  g_return_val_if_fail (CTK_IS_FONT_SELECTION (fontsel), NULL);

  return fontsel->priv->family;
}

/**
 * ctk_font_selection_get_face:
 * @fontsel: a #CtkFontSelection
 *
 * Gets the #PangoFontFace representing the selected font group
 * details (i.e. family, slant, weight, width, etc).
 *
 * Returns: (transfer none): A #PangoFontFace representing the
 *     selected font group details. The returned object is owned by
 *     @fontsel and must not be modified or freed.
 *
 * Since: 2.14
 *
 * Deprecated: 3.2: Use #CtkFontChooser
 */
PangoFontFace *
ctk_font_selection_get_face (CtkFontSelection *fontsel)
{
  g_return_val_if_fail (CTK_IS_FONT_SELECTION (fontsel), NULL);

  return fontsel->priv->face;
}

/**
 * ctk_font_selection_get_size:
 * @fontsel: a #CtkFontSelection
 *
 * The selected font size.
 *
 * Returns: A n integer representing the selected font size,
 *     or -1 if no font size is selected.
 *
 * Since: 2.14
 *
 * Deprecated: 3.2: Use #CtkFontChooser
 **/
gint
ctk_font_selection_get_size (CtkFontSelection *fontsel)
{
  g_return_val_if_fail (CTK_IS_FONT_SELECTION (fontsel), -1);

  return fontsel->priv->size;
}

/**
 * ctk_font_selection_get_font_name:
 * @fontsel: a #CtkFontSelection
 * 
 * Gets the currently-selected font name. 
 *
 * Note that this can be a different string than what you set with 
 * ctk_font_selection_set_font_name(), as the font selection widget may 
 * normalize font names and thus return a string with a different structure. 
 * For example, “Helvetica Italic Bold 12” could be normalized to 
 * “Helvetica Bold Italic 12”. Use pango_font_description_equal()
 * if you want to compare two font descriptions.
 * 
 * Returns: A string with the name of the current font, or %NULL if 
 *     no font is selected. You must free this string with g_free().
 *
 * Deprecated: 3.2: Use #CtkFontChooser
 */
gchar *
ctk_font_selection_get_font_name (CtkFontSelection *fontsel)
{
  gchar *result;
  
  PangoFontDescription *font_desc = ctk_font_selection_get_font_description (fontsel);
  result = pango_font_description_to_string (font_desc);
  pango_font_description_free (font_desc);

  return result;
}

/* This selects the appropriate list rows.
   First we check the fontname is valid and try to find the font family
   - i.e. the name in the main list. If we can't find that, then just return.
   Next we try to set each of the properties according to the fontname.
   Finally we select the font family & style in the lists. */
static gboolean
ctk_font_selection_select_font_desc (CtkFontSelection      *fontsel,
				     PangoFontDescription  *new_desc,
				     PangoFontFamily      **pfamily,
				     PangoFontFace        **pface)
{
  CtkFontSelectionPrivate *priv = fontsel->priv;
  PangoFontFamily *new_family = NULL;
  PangoFontFace *new_face = NULL;
  PangoFontFace *fallback_face = NULL;
  CtkTreeModel *model;
  CtkTreeIter iter;
  CtkTreeIter match_iter;
  gboolean valid;
  const gchar *new_family_name;

  new_family_name = pango_font_description_get_family (new_desc);

  if (!new_family_name)
    return FALSE;

  /* Check to make sure that this is in the list of allowed fonts 
   */
  model = ctk_tree_view_get_model (CTK_TREE_VIEW (priv->family_list));
  for (valid = ctk_tree_model_get_iter_first (model, &iter);
       valid;
       valid = ctk_tree_model_iter_next (model, &iter))
    {
      PangoFontFamily *family;
      
      ctk_tree_model_get (model, &iter, FAMILY_COLUMN, &family, -1);
      
      if (g_ascii_strcasecmp (pango_font_family_get_name (family),
			      new_family_name) == 0)
	new_family = g_object_ref (family);

      g_object_unref (family);
      
      if (new_family)
	break;
    }

  if (!new_family)
    return FALSE;

  if (pfamily)
    *pfamily = new_family;
  else
    g_object_unref (new_family);
  set_cursor_to_iter (CTK_TREE_VIEW (priv->family_list), &iter);
  ctk_font_selection_show_available_styles (fontsel);

  model = ctk_tree_view_get_model (CTK_TREE_VIEW (priv->face_list));
  for (valid = ctk_tree_model_get_iter_first (model, &iter);
       valid;
       valid = ctk_tree_model_iter_next (model, &iter))
    {
      PangoFontFace *face;
      PangoFontDescription *tmp_desc;
      
      ctk_tree_model_get (model, &iter, FACE_COLUMN, &face, -1);
      tmp_desc = pango_font_face_describe (face);
      
      if (font_description_style_equal (tmp_desc, new_desc))
	new_face = g_object_ref (face);
      
      if (!fallback_face)
	{
	  fallback_face = g_object_ref (face);
	  match_iter = iter;
	}
      
      pango_font_description_free (tmp_desc);
      g_object_unref (face);
      
      if (new_face)
	{
	  match_iter = iter;
	  break;
	}
    }

  if (!new_face)
    new_face = fallback_face;
  else if (fallback_face)
    g_object_unref (fallback_face);

  if (pface)
    *pface = new_face;
  else if (new_face)
    g_object_unref (new_face);
  set_cursor_to_iter (CTK_TREE_VIEW (priv->face_list), &match_iter);  

  ctk_font_selection_set_size (fontsel, pango_font_description_get_size (new_desc));

  return TRUE;
}


/* This sets the current font, then selecting the appropriate list rows. */

/**
 * ctk_font_selection_set_font_name:
 * @fontsel: a #CtkFontSelection
 * @fontname: a font name like “Helvetica 12” or “Times Bold 18”
 * 
 * Sets the currently-selected font. 
 *
 * Note that the @fontsel needs to know the screen in which it will appear 
 * for this to work; this can be guaranteed by simply making sure that the 
 * @fontsel is inserted in a toplevel window before you call this function.
 * 
 * Returns: %TRUE if the font could be set successfully; %FALSE if no 
 *     such font exists or if the @fontsel doesn’t belong to a particular 
 *     screen yet.
 *
 * Deprecated: 3.2: Use #CtkFontChooser
 */
gboolean
ctk_font_selection_set_font_name (CtkFontSelection *fontsel,
				  const gchar      *fontname)
{
  PangoFontFamily *family = NULL;
  PangoFontFace *face = NULL;
  PangoFontDescription *new_desc;
  
  g_return_val_if_fail (CTK_IS_FONT_SELECTION (fontsel), FALSE);

  if (!ctk_widget_has_screen (CTK_WIDGET (fontsel)))
    return FALSE;

  new_desc = pango_font_description_from_string (fontname);

  if (ctk_font_selection_select_font_desc (fontsel, new_desc, &family, &face))
    {
      ctk_font_selection_ref_family (fontsel, family);
      if (family)
        g_object_unref (family);

      ctk_font_selection_ref_face (fontsel, face);
      if (face)
        g_object_unref (face);
    }

  pango_font_description_free (new_desc);
  
  g_object_notify (G_OBJECT (fontsel), "font-name");

  return TRUE;
}

/**
 * ctk_font_selection_get_preview_text:
 * @fontsel: a #CtkFontSelection
 *
 * Gets the text displayed in the preview area.
 * 
 * Returns: the text displayed in the preview area. 
 *     This string is owned by the widget and should not be 
 *     modified or freed 
 *
 * Deprecated: 3.2: Use #CtkFontChooser
 */
const gchar*
ctk_font_selection_get_preview_text (CtkFontSelection *fontsel)
{
  CtkFontSelectionPrivate *priv;

  g_return_val_if_fail (CTK_IS_FONT_SELECTION (fontsel), NULL);

  priv = fontsel->priv;

  return ctk_entry_get_text (CTK_ENTRY (priv->preview_entry));
}


/**
 * ctk_font_selection_set_preview_text:
 * @fontsel: a #CtkFontSelection
 * @text: the text to display in the preview area 
 *
 * Sets the text displayed in the preview area.
 * The @text is used to show how the selected font looks.
 *
 * Deprecated: 3.2: Use #CtkFontChooser
 */
void
ctk_font_selection_set_preview_text  (CtkFontSelection *fontsel,
				      const gchar      *text)
{
  CtkFontSelectionPrivate *priv;

  g_return_if_fail (CTK_IS_FONT_SELECTION (fontsel));
  g_return_if_fail (text != NULL);

  priv = fontsel->priv;

  ctk_entry_set_text (CTK_ENTRY (priv->preview_entry), text);
}


/**
 * SECTION:ctkfontseldlg
 * @Short_description: Deprecated dialog box for selecting fonts
 * @Title: CtkFontSelectionDialog
 * @See_also: #CtkFontSelection, #CtkDialog, #CtkFontChooserDialog
 *
 * The #CtkFontSelectionDialog widget is a dialog box for selecting a font.
 *
 * To set the font which is initially selected, use
 * ctk_font_selection_dialog_set_font_name().
 *
 * To get the selected font use ctk_font_selection_dialog_get_font_name().
 *
 * To change the text which is shown in the preview area, use
 * ctk_font_selection_dialog_set_preview_text().
 *
 * In CTK+ 3.2, #CtkFontSelectionDialog has been deprecated in favor of
 * #CtkFontChooserDialog.
 *
 * # CtkFontSelectionDialog as CtkBuildable # {#CtkFontSelectionDialog-BUILDER-UI}
 *
 * The CtkFontSelectionDialog implementation of the CtkBuildable interface
 * exposes the embedded #CtkFontSelection as internal child with the
 * name “font_selection”. It also exposes the buttons with the names
 * “ok_button”, “cancel_button” and “apply_button”.
 */

static void ctk_font_selection_dialog_buildable_interface_init     (CtkBuildableIface *iface);
static GObject * ctk_font_selection_dialog_buildable_get_internal_child (CtkBuildable *buildable,
									  CtkBuilder   *builder,
									  const gchar  *childname);

G_DEFINE_TYPE_WITH_CODE (CtkFontSelectionDialog, ctk_font_selection_dialog,
			 CTK_TYPE_DIALOG,
                         G_ADD_PRIVATE (CtkFontSelectionDialog)
			 G_IMPLEMENT_INTERFACE (CTK_TYPE_BUILDABLE,
						ctk_font_selection_dialog_buildable_interface_init))

static CtkBuildableIface *parent_buildable_iface;

static void
ctk_font_selection_dialog_class_init (CtkFontSelectionDialogClass *klass)
{
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);

  ctk_widget_class_set_accessible_role (widget_class, ATK_ROLE_FONT_CHOOSER);
}

static void
ctk_font_selection_dialog_init (CtkFontSelectionDialog *fontseldiag)
{
  CtkFontSelectionDialogPrivate *priv;
  CtkDialog *dialog = CTK_DIALOG (fontseldiag);
  CtkWidget *action_area, *content_area;

  fontseldiag->priv = ctk_font_selection_dialog_get_instance_private (fontseldiag);
  priv = fontseldiag->priv;

  content_area = ctk_dialog_get_content_area (dialog);
  action_area = ctk_dialog_get_action_area (dialog);

  ctk_container_set_border_width (CTK_CONTAINER (dialog), 5);
  ctk_box_set_spacing (CTK_BOX (content_area), 2); /* 2 * 5 + 2 = 12 */
  ctk_container_set_border_width (CTK_CONTAINER (action_area), 5);
  ctk_box_set_spacing (CTK_BOX (action_area), 6);

  ctk_widget_push_composite_child ();

  ctk_window_set_resizable (CTK_WINDOW (fontseldiag), TRUE);

  /* Create the content area */
  priv->fontsel = ctk_font_selection_new ();
  ctk_container_set_border_width (CTK_CONTAINER (priv->fontsel), 5);
  ctk_widget_show (priv->fontsel);
  ctk_box_pack_start (CTK_BOX (content_area),
		      priv->fontsel, TRUE, TRUE, 0);

  /* Create the action area */
  priv->cancel_button = ctk_dialog_add_button (dialog,
                                               _("_Cancel"),
                                               CTK_RESPONSE_CANCEL);

  priv->apply_button = ctk_dialog_add_button (dialog,
                                              _("_Apply"),
                                              CTK_RESPONSE_APPLY);
  ctk_widget_hide (priv->apply_button);

  priv->ok_button = ctk_dialog_add_button (dialog,
                                           _("_OK"),
                                           CTK_RESPONSE_OK);
  ctk_widget_grab_default (priv->ok_button);

  ctk_dialog_set_alternative_button_order (CTK_DIALOG (fontseldiag),
					   CTK_RESPONSE_OK,
					   CTK_RESPONSE_APPLY,
					   CTK_RESPONSE_CANCEL,
					   -1);

  ctk_window_set_title (CTK_WINDOW (fontseldiag),
                        _("Font Selection"));

  ctk_widget_pop_composite_child ();
}

/**
 * ctk_font_selection_dialog_new:
 * @title: the title of the dialog window 
 *
 * Creates a new #CtkFontSelectionDialog.
 *
 * Returns: a new #CtkFontSelectionDialog
 *
 * Deprecated: 3.2: Use #CtkFontChooserDialog
 */
CtkWidget*
ctk_font_selection_dialog_new (const gchar *title)
{
  CtkFontSelectionDialog *fontseldiag;
  
  fontseldiag = g_object_new (CTK_TYPE_FONT_SELECTION_DIALOG, NULL);

  if (title)
    ctk_window_set_title (CTK_WINDOW (fontseldiag), title);
  
  return CTK_WIDGET (fontseldiag);
}

/**
 * ctk_font_selection_dialog_get_font_selection:
 * @fsd: a #CtkFontSelectionDialog
 *
 * Retrieves the #CtkFontSelection widget embedded in the dialog.
 *
 * Returns: (transfer none): the embedded #CtkFontSelection
 *
 * Since: 2.22
 *
 * Deprecated: 3.2: Use #CtkFontChooserDialog
 **/
CtkWidget*
ctk_font_selection_dialog_get_font_selection (CtkFontSelectionDialog *fsd)
{
  g_return_val_if_fail (CTK_IS_FONT_SELECTION_DIALOG (fsd), NULL);

  return fsd->priv->fontsel;
}


/**
 * ctk_font_selection_dialog_get_ok_button:
 * @fsd: a #CtkFontSelectionDialog
 *
 * Gets the “OK” button.
 *
 * Returns: (transfer none): the #CtkWidget used in the dialog
 *     for the “OK” button.
 *
 * Since: 2.14
 *
 * Deprecated: 3.2: Use #CtkFontChooserDialog
 */
CtkWidget *
ctk_font_selection_dialog_get_ok_button (CtkFontSelectionDialog *fsd)
{
  g_return_val_if_fail (CTK_IS_FONT_SELECTION_DIALOG (fsd), NULL);

  return fsd->priv->ok_button;
}

/**
 * ctk_font_selection_dialog_get_cancel_button:
 * @fsd: a #CtkFontSelectionDialog
 *
 * Gets the “Cancel” button.
 *
 * Returns: (transfer none): the #CtkWidget used in the dialog
 *     for the “Cancel” button.
 *
 * Since: 2.14
 *
 * Deprecated: 3.2: Use #CtkFontChooserDialog
 */
CtkWidget *
ctk_font_selection_dialog_get_cancel_button (CtkFontSelectionDialog *fsd)
{
  g_return_val_if_fail (CTK_IS_FONT_SELECTION_DIALOG (fsd), NULL);

  return fsd->priv->cancel_button;
}

static void
ctk_font_selection_dialog_buildable_interface_init (CtkBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);
  iface->get_internal_child = ctk_font_selection_dialog_buildable_get_internal_child;
}

static GObject *
ctk_font_selection_dialog_buildable_get_internal_child (CtkBuildable *buildable,
							CtkBuilder   *builder,
							const gchar  *childname)
{
  CtkFontSelectionDialogPrivate *priv;

  priv = CTK_FONT_SELECTION_DIALOG (buildable)->priv;

  if (g_strcmp0 (childname, "ok_button") == 0)
    return G_OBJECT (priv->ok_button);
  else if (g_strcmp0 (childname, "cancel_button") == 0)
    return G_OBJECT (priv->cancel_button);
  else if (g_strcmp0 (childname, "apply_button") == 0)
    return G_OBJECT (priv->apply_button);
  else if (g_strcmp0 (childname, "font_selection") == 0)
    return G_OBJECT (priv->fontsel);

  return parent_buildable_iface->get_internal_child (buildable, builder, childname);
}

/**
 * ctk_font_selection_dialog_get_font_name:
 * @fsd: a #CtkFontSelectionDialog
 * 
 * Gets the currently-selected font name.
 *
 * Note that this can be a different string than what you set with 
 * ctk_font_selection_dialog_set_font_name(), as the font selection widget
 * may normalize font names and thus return a string with a different 
 * structure. For example, “Helvetica Italic Bold 12” could be normalized 
 * to “Helvetica Bold Italic 12”.  Use pango_font_description_equal()
 * if you want to compare two font descriptions.
 * 
 * Returns: A string with the name of the current font, or %NULL if no 
 *     font is selected. You must free this string with g_free().
 *
 * Deprecated: 3.2: Use #CtkFontChooserDialog
 */
gchar*
ctk_font_selection_dialog_get_font_name (CtkFontSelectionDialog *fsd)
{
  CtkFontSelectionDialogPrivate *priv;

  g_return_val_if_fail (CTK_IS_FONT_SELECTION_DIALOG (fsd), NULL);

  priv = fsd->priv;

  return ctk_font_selection_get_font_name (CTK_FONT_SELECTION (priv->fontsel));
}

/**
 * ctk_font_selection_dialog_set_font_name:
 * @fsd: a #CtkFontSelectionDialog
 * @fontname: a font name like “Helvetica 12” or “Times Bold 18”
 *
 * Sets the currently selected font. 
 * 
 * Returns: %TRUE if the font selected in @fsd is now the
 *     @fontname specified, %FALSE otherwise. 
 *
 * Deprecated: 3.2: Use #CtkFontChooserDialog
 */
gboolean
ctk_font_selection_dialog_set_font_name (CtkFontSelectionDialog *fsd,
					 const gchar	        *fontname)
{
  CtkFontSelectionDialogPrivate *priv;

  g_return_val_if_fail (CTK_IS_FONT_SELECTION_DIALOG (fsd), FALSE);
  g_return_val_if_fail (fontname, FALSE);

  priv = fsd->priv;

  return ctk_font_selection_set_font_name (CTK_FONT_SELECTION (priv->fontsel), fontname);
}

/**
 * ctk_font_selection_dialog_get_preview_text:
 * @fsd: a #CtkFontSelectionDialog
 *
 * Gets the text displayed in the preview area.
 * 
 * Returns: the text displayed in the preview area. 
 *     This string is owned by the widget and should not be 
 *     modified or freed 
 *
 * Deprecated: 3.2: Use #CtkFontChooserDialog
 */
const gchar*
ctk_font_selection_dialog_get_preview_text (CtkFontSelectionDialog *fsd)
{
  CtkFontSelectionDialogPrivate *priv;

  g_return_val_if_fail (CTK_IS_FONT_SELECTION_DIALOG (fsd), NULL);

  priv = fsd->priv;

  return ctk_font_selection_get_preview_text (CTK_FONT_SELECTION (priv->fontsel));
}

/**
 * ctk_font_selection_dialog_set_preview_text:
 * @fsd: a #CtkFontSelectionDialog
 * @text: the text to display in the preview area
 *
 * Sets the text displayed in the preview area. 
 *
 * Deprecated: 3.2: Use #CtkFontChooserDialog
 */
void
ctk_font_selection_dialog_set_preview_text (CtkFontSelectionDialog *fsd,
					    const gchar	           *text)
{
  CtkFontSelectionDialogPrivate *priv;

  g_return_if_fail (CTK_IS_FONT_SELECTION_DIALOG (fsd));
  g_return_if_fail (text != NULL);

  priv = fsd->priv;

  ctk_font_selection_set_preview_text (CTK_FONT_SELECTION (priv->fontsel), text);
}
