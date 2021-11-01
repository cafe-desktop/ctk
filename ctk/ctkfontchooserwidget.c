/* CTK - The GIMP Toolkit
 * Copyright (C) 2011 Alberto Ruiz <aruiz@gnome.org>
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

#include <stdlib.h>
#include <glib/gprintf.h>
#include <string.h>

#include <atk/atk.h>

#include "ctkfontchooserwidget.h"
#include "ctkfontchooserwidgetprivate.h"

#include "ctkadjustment.h"
#include "ctkbuildable.h"
#include "ctkbox.h"
#include "ctkcellrenderertext.h"
#include "ctkentry.h"
#include "ctksearchentry.h"
#include "ctkgrid.h"
#include "ctkfontchooser.h"
#include "ctkfontchooserutils.h"
#include "ctkintl.h"
#include "ctklabel.h"
#include "ctkliststore.h"
#include "ctkstack.h"
#include "ctkprivate.h"
#include "ctkscale.h"
#include "ctkscrolledwindow.h"
#include "ctkspinbutton.h"
#include "ctktextview.h"
#include "ctktreeselection.h"
#include "ctktreeview.h"
#include "ctkwidget.h"
#include "ctksettings.h"
#include "ctkdialog.h"
#include "ctkradiobutton.h"
#include "ctkcombobox.h"
#include "ctkgesturemultipress.h"

#include <hb-ot.h>

#include "language-names.h"
#include "script-names.h"

#include "open-type-layout.h"

/**
 * SECTION:ctkfontchooserwidget
 * @Short_description: A widget for selecting fonts
 * @Title: CtkFontChooserWidget
 * @See_also: #CtkFontChooserDialog
 *
 * The #CtkFontChooserWidget widget lists the available fonts,
 * styles and sizes, allowing the user to select a font. It is
 * used in the #CtkFontChooserDialog widget to provide a
 * dialog box for selecting fonts.
 *
 * To set the font which is initially selected, use
 * ctk_font_chooser_set_font() or ctk_font_chooser_set_font_desc().
 *
 * To get the selected font use ctk_font_chooser_get_font() or
 * ctk_font_chooser_get_font_desc().
 *
 * To change the text which is shown in the preview area, use
 * ctk_font_chooser_set_preview_text().
 *
 * # CSS nodes
 *
 * CtkFontChooserWidget has a single CSS node with name fontchooser.
 *
 * Since: 3.2
 */


struct _CtkFontChooserWidgetPrivate
{
  CtkWidget    *stack;
  CtkWidget    *search_entry;
  CtkWidget    *family_face_list;
  CtkTreeViewColumn *family_face_column;
  CtkCellRenderer *family_face_cell;
  CtkWidget    *list_scrolled_window;
  CtkWidget    *list_stack;
  CtkTreeModel *model;
  CtkTreeModel *filter_model;

  CtkWidget       *preview;
  CtkWidget       *preview2;
  CtkWidget       *font_name_label;
  gchar           *preview_text;
  gboolean         show_preview_entry;

  CtkWidget *size_label;
  CtkWidget *size_spin;
  CtkWidget *size_slider;
  CtkWidget *size_slider2;

  CtkWidget *axis_grid;
  CtkWidget       *feature_box;

  PangoFontMap         *font_map;

  PangoFontDescription *font_desc;
  char                 *font_features;
  PangoLanguage        *language;
  CtkTreeIter           font_iter;      /* invalid if font not available or pointer into model
                                           (not filter_model) to the row containing font */
  CtkFontFilterFunc filter_func;
  gpointer          filter_data;
  GDestroyNotify    filter_data_destroy;

  guint last_fontconfig_timestamp;

  CtkFontChooserLevel level;

  GHashTable *axes;
  gboolean updating_variations;

  GList *feature_items;

  GAction *tweak_action;
};


/* This is the initial fixed height and the top padding of the preview entry */
#define PREVIEW_HEIGHT 72
#define PREVIEW_TOP_PADDING 6

/* These are the sizes of the font, style & size lists. */
#define FONT_LIST_HEIGHT  136
#define FONT_LIST_WIDTH   190
#define FONT_STYLE_LIST_WIDTH 170
#define FONT_SIZE_LIST_WIDTH  60

enum {
  PROP_ZERO,
  PROP_TWEAK_ACTION
};

/* Keep in line with CtkTreeStore defined in ctkfontchooserwidget.ui */
enum {
  FAMILY_COLUMN,
  FACE_COLUMN,
  FONT_DESC_COLUMN,
  PREVIEW_TITLE_COLUMN
};

static void ctk_font_chooser_widget_set_property         (GObject         *object,
                                                          guint            prop_id,
                                                          const GValue    *value,
                                                          GParamSpec      *pspec);
static void ctk_font_chooser_widget_get_property         (GObject         *object,
                                                          guint            prop_id,
                                                          GValue          *value,
                                                          GParamSpec      *pspec);
static void ctk_font_chooser_widget_finalize             (GObject         *object);

static void ctk_font_chooser_widget_screen_changed       (CtkWidget       *widget,
                                                          CdkScreen       *previous_screen);

static gboolean ctk_font_chooser_widget_find_font        (CtkFontChooserWidget *fontchooser,
                                                          const PangoFontDescription *font_desc,
                                                          CtkTreeIter          *iter);
static void     ctk_font_chooser_widget_ensure_selection (CtkFontChooserWidget *fontchooser);

static gchar   *ctk_font_chooser_widget_get_font         (CtkFontChooserWidget *fontchooser);
static void     ctk_font_chooser_widget_set_font         (CtkFontChooserWidget *fontchooser,
                                                          const gchar          *fontname);

static PangoFontDescription *ctk_font_chooser_widget_get_font_desc  (CtkFontChooserWidget *fontchooser);
static void                  ctk_font_chooser_widget_merge_font_desc(CtkFontChooserWidget       *fontchooser,
                                                                     const PangoFontDescription *font_desc,
                                                                     CtkTreeIter                *iter);
static void                  ctk_font_chooser_widget_take_font_desc (CtkFontChooserWidget *fontchooser,
                                                                     PangoFontDescription *font_desc);


static const gchar *ctk_font_chooser_widget_get_preview_text (CtkFontChooserWidget *fontchooser);
static void         ctk_font_chooser_widget_set_preview_text (CtkFontChooserWidget *fontchooser,
                                                              const gchar          *text);

static gboolean ctk_font_chooser_widget_get_show_preview_entry (CtkFontChooserWidget *fontchooser);
static void     ctk_font_chooser_widget_set_show_preview_entry (CtkFontChooserWidget *fontchooser,
                                                                gboolean              show_preview_entry);

static void     ctk_font_chooser_widget_set_cell_size          (CtkFontChooserWidget *fontchooser);
static void     ctk_font_chooser_widget_load_fonts             (CtkFontChooserWidget *fontchooser,
                                                                gboolean              force);

static void     ctk_font_chooser_widget_populate_features      (CtkFontChooserWidget *fontchooser);
static gboolean visible_func                                   (CtkTreeModel *model,
								CtkTreeIter  *iter,
								gpointer      user_data);
static void     ctk_font_chooser_widget_cell_data_func         (CtkTreeViewColumn *column,
								CtkCellRenderer   *cell,
								CtkTreeModel      *tree_model,
								CtkTreeIter       *iter,
								gpointer           user_data);

static void selection_changed (CtkTreeSelection *selection,
                               CtkFontChooserWidget *fontchooser);
static void update_font_features (CtkFontChooserWidget *fontchooser);


static void                ctk_font_chooser_widget_set_level (CtkFontChooserWidget *fontchooser,
                                                              CtkFontChooserLevel   level);
static CtkFontChooserLevel ctk_font_chooser_widget_get_level (CtkFontChooserWidget *fontchooser);
static void                ctk_font_chooser_widget_set_language (CtkFontChooserWidget *fontchooser,
                                                                 const char           *language);
static void selection_changed (CtkTreeSelection *selection,
                               CtkFontChooserWidget *fontchooser);
static void update_font_features (CtkFontChooserWidget *fontchooser);

static void ctk_font_chooser_widget_iface_init (CtkFontChooserIface *iface);

G_DEFINE_TYPE_WITH_CODE (CtkFontChooserWidget, ctk_font_chooser_widget, CTK_TYPE_BOX,
                         G_ADD_PRIVATE (CtkFontChooserWidget)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_FONT_CHOOSER,
                                                ctk_font_chooser_widget_iface_init))

typedef struct _CtkDelayedFontDescription CtkDelayedFontDescription;
struct _CtkDelayedFontDescription {
  PangoFontFace        *face;
  PangoFontDescription *desc;
  guint                 ref_count;
};

static CtkDelayedFontDescription *
ctk_delayed_font_description_new (PangoFontFace *face)
{
  CtkDelayedFontDescription *result;
  
  result = g_slice_new0 (CtkDelayedFontDescription);

  result->face = g_object_ref (face);
  result->desc = NULL;
  result->ref_count = 1;

  return result;
}

static CtkDelayedFontDescription *
ctk_delayed_font_description_ref (CtkDelayedFontDescription *desc)
{
  desc->ref_count++;

  return desc;
}

static void
ctk_delayed_font_description_unref (CtkDelayedFontDescription *desc)
{
  desc->ref_count--;

  if (desc->ref_count > 0)
    return;

  g_object_unref (desc->face);
  if (desc->desc)
    pango_font_description_free (desc->desc);

  g_slice_free (CtkDelayedFontDescription, desc);
}

static const PangoFontDescription *
ctk_delayed_font_description_get (CtkDelayedFontDescription *desc)
{
  if (desc->desc == NULL)
    desc->desc = pango_font_face_describe (desc->face);

  return desc->desc;
}

#define CTK_TYPE_DELAYED_FONT_DESCRIPTION (ctk_delayed_font_description_get_type ())
G_DEFINE_BOXED_TYPE (CtkDelayedFontDescription, ctk_delayed_font_description,
                     ctk_delayed_font_description_ref,
                     ctk_delayed_font_description_unref)
static void
ctk_font_chooser_widget_set_property (GObject         *object,
                                      guint            prop_id,
                                      const GValue    *value,
                                      GParamSpec      *pspec)
{
  CtkFontChooserWidget *fontchooser = CTK_FONT_CHOOSER_WIDGET (object);

  switch (prop_id)
    {
    case CTK_FONT_CHOOSER_PROP_FONT:
      ctk_font_chooser_widget_set_font (fontchooser, g_value_get_string (value));
      break;
    case CTK_FONT_CHOOSER_PROP_FONT_DESC:
      ctk_font_chooser_widget_take_font_desc (fontchooser, g_value_dup_boxed (value));
      break;
    case CTK_FONT_CHOOSER_PROP_PREVIEW_TEXT:
      ctk_font_chooser_widget_set_preview_text (fontchooser, g_value_get_string (value));
      break;
    case CTK_FONT_CHOOSER_PROP_SHOW_PREVIEW_ENTRY:
      ctk_font_chooser_widget_set_show_preview_entry (fontchooser, g_value_get_boolean (value));
      break;
    case CTK_FONT_CHOOSER_PROP_LEVEL:
      ctk_font_chooser_widget_set_level (fontchooser, g_value_get_flags (value));
      break;
    case CTK_FONT_CHOOSER_PROP_LANGUAGE:
      ctk_font_chooser_widget_set_language (fontchooser, g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_font_chooser_widget_get_property (GObject         *object,
                                      guint            prop_id,
                                      GValue          *value,
                                      GParamSpec      *pspec)
{
  CtkFontChooserWidget *fontchooser = CTK_FONT_CHOOSER_WIDGET (object);

  switch (prop_id)
    {
    case PROP_TWEAK_ACTION:
      g_value_set_object (value, G_OBJECT (fontchooser->priv->tweak_action));
      break;
    case CTK_FONT_CHOOSER_PROP_FONT:
      g_value_take_string (value, ctk_font_chooser_widget_get_font (fontchooser));
      break;
    case CTK_FONT_CHOOSER_PROP_FONT_DESC:
      g_value_set_boxed (value, ctk_font_chooser_widget_get_font_desc (fontchooser));
      break;
    case CTK_FONT_CHOOSER_PROP_PREVIEW_TEXT:
      g_value_set_string (value, ctk_font_chooser_widget_get_preview_text (fontchooser));
      break;
    case CTK_FONT_CHOOSER_PROP_SHOW_PREVIEW_ENTRY:
      g_value_set_boolean (value, ctk_font_chooser_widget_get_show_preview_entry (fontchooser));
      break;
    case CTK_FONT_CHOOSER_PROP_LEVEL:
      g_value_set_flags (value, ctk_font_chooser_widget_get_level (fontchooser));
      break;
    case CTK_FONT_CHOOSER_PROP_FONT_FEATURES:
      g_value_set_string (value, fontchooser->priv->font_features);
      break;
    case CTK_FONT_CHOOSER_PROP_LANGUAGE:
      g_value_set_string (value, pango_language_to_string (fontchooser->priv->language));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_font_chooser_widget_refilter_font_list (CtkFontChooserWidget *fontchooser)
{
  ctk_tree_model_filter_refilter (CTK_TREE_MODEL_FILTER (fontchooser->priv->filter_model));
  ctk_font_chooser_widget_ensure_selection (fontchooser);
}

static void
text_changed_cb (CtkEntry             *entry,
                 CtkFontChooserWidget *fc)
{
  ctk_font_chooser_widget_refilter_font_list (fc);
}

static void
stop_search_cb (CtkEntry             *entry,
                CtkFontChooserWidget *fc)
{
  if (ctk_entry_get_text (entry)[0] != 0)
    ctk_entry_set_text (entry, "");
  else
    {
      CtkWidget *dialog;
      CtkWidget *button = NULL;

      dialog = ctk_widget_get_ancestor (CTK_WIDGET (fc), CTK_TYPE_DIALOG);
      if (dialog)
        button = ctk_dialog_get_widget_for_response (CTK_DIALOG (dialog), CTK_RESPONSE_CANCEL);

      if (button)
        ctk_widget_activate (button);
    }
}

static void
size_change_cb (CtkAdjustment *adjustment,
                gpointer       user_data)
{
  CtkFontChooserWidget *fontchooser = user_data;
  CtkFontChooserWidgetPrivate *priv = fontchooser->priv;
  PangoFontDescription *font_desc;
  gdouble size = ctk_adjustment_get_value (adjustment);

  font_desc = pango_font_description_new ();
  if (pango_font_description_get_size_is_absolute (priv->font_desc))
    pango_font_description_set_absolute_size (font_desc, size * PANGO_SCALE);
  else
    pango_font_description_set_size (font_desc, size * PANGO_SCALE);

  ctk_font_chooser_widget_take_font_desc (fontchooser, font_desc);
}

static gboolean
output_cb (CtkSpinButton *spin,
           gpointer       data)
{
  CtkAdjustment *adjustment;
  gchar *text;
  gdouble value;

  adjustment = ctk_spin_button_get_adjustment (spin);
  value = ctk_adjustment_get_value (adjustment);
  text = g_strdup_printf ("%2.4g", value);
  ctk_entry_set_text (CTK_ENTRY (spin), text);
  g_free (text);

  return TRUE;
}

static void
ctk_font_chooser_widget_update_marks (CtkFontChooserWidget *fontchooser)
{
  CtkFontChooserWidgetPrivate *priv = fontchooser->priv;
  CtkAdjustment *adj, *spin_adj;
  const int *sizes;
  gint *font_sizes;
  gint i, n_sizes;
  gdouble value, spin_value;

  if (ctk_list_store_iter_is_valid (CTK_LIST_STORE (priv->model), &priv->font_iter))
    {
      PangoFontFace *face;

      ctk_tree_model_get (priv->model, &priv->font_iter,
                          FACE_COLUMN, &face,
                          -1);

      pango_font_face_list_sizes (face, &font_sizes, &n_sizes);

      /* It seems not many fonts actually have a sane set of sizes */
      for (i = 0; i < n_sizes; i++)
        font_sizes[i] = font_sizes[i] / PANGO_SCALE;

      g_object_unref (face);
    }
  else
    {
      font_sizes = NULL;
      n_sizes = 0;
    }

  if (n_sizes < 2)
    {
      static const gint fallback_sizes[] = {
        6, 8, 9, 10, 11, 12, 13, 14, 16, 20, 24, 36, 48, 72
      };

      sizes = fallback_sizes;
      n_sizes = G_N_ELEMENTS (fallback_sizes);
    }
  else
    {
      sizes = font_sizes;
    }

  ctk_scale_clear_marks (CTK_SCALE (priv->size_slider));
  ctk_scale_clear_marks (CTK_SCALE (priv->size_slider2));

  adj        = ctk_range_get_adjustment (CTK_RANGE (priv->size_slider));
  spin_adj   = ctk_spin_button_get_adjustment (CTK_SPIN_BUTTON (priv->size_spin));
  spin_value = ctk_adjustment_get_value (spin_adj);

  if (spin_value < sizes[0])
    value = (gdouble) sizes[0];
  else if (spin_value > sizes[n_sizes - 1])
    value = (gdouble)sizes[n_sizes - 1];
  else
    value = (gdouble)spin_value;

  /* ensure clamping doesn't callback into font resizing code */
  g_signal_handlers_block_by_func (adj, size_change_cb, fontchooser);
  ctk_adjustment_configure (adj,
                            value,
                            sizes[0],
                            sizes[n_sizes - 1],
                            ctk_adjustment_get_step_increment (adj),
                            ctk_adjustment_get_page_increment (adj),
                            ctk_adjustment_get_page_size (adj));
  g_signal_handlers_unblock_by_func (adj, size_change_cb, fontchooser);

  for (i = 0; i < n_sizes; i++)
    {
      ctk_scale_add_mark (CTK_SCALE (priv->size_slider),
                          sizes[i],
                          CTK_POS_BOTTOM, NULL);
      ctk_scale_add_mark (CTK_SCALE (priv->size_slider2),
                          sizes[i],
                          CTK_POS_BOTTOM, NULL);
    }

  g_free (font_sizes);
}

static void
row_activated_cb (CtkTreeView       *view,
                  CtkTreePath       *path,
                  CtkTreeViewColumn *column,
                  gpointer           user_data)
{
  CtkFontChooserWidget *fontchooser = user_data;
  gchar *fontname;

  fontname = ctk_font_chooser_widget_get_font (fontchooser);
  _ctk_font_chooser_font_activated (CTK_FONT_CHOOSER (fontchooser), fontname);
  g_free (fontname);
}

static void
cursor_changed_cb (CtkTreeView *treeview,
                   gpointer     user_data)
{
  CtkFontChooserWidget *fontchooser = user_data;
  CtkFontChooserWidgetPrivate *priv = fontchooser->priv;
  CtkDelayedFontDescription *desc;
  CtkTreeIter filter_iter, iter;
  CtkTreePath *path = NULL;

  ctk_tree_view_get_cursor (treeview, &path, NULL);

  if (!path)
    return;

  if (!ctk_tree_model_get_iter (priv->filter_model, &filter_iter, path))
    {
      ctk_tree_path_free (path);
      return;
    }

  ctk_tree_path_free (path);

  ctk_tree_model_filter_convert_iter_to_child_iter (CTK_TREE_MODEL_FILTER (priv->filter_model),
                                                    &iter,
                                                    &filter_iter);
  ctk_tree_model_get (priv->model, &iter,
                      FONT_DESC_COLUMN, &desc,
                      -1);

  pango_font_description_set_variations (priv->font_desc, NULL);
  ctk_font_chooser_widget_merge_font_desc (fontchooser,
                                           ctk_delayed_font_description_get (desc),
                                           &iter);

  ctk_delayed_font_description_unref (desc);
}

static gboolean
resize_by_scroll_cb (CtkWidget      *scrolled_window,
                     CdkEventScroll *event,
                     gpointer        user_data)
{
  CtkFontChooserWidget *fc = user_data;
  CtkFontChooserWidgetPrivate *priv = fc->priv;
  CtkAdjustment *adj = ctk_spin_button_get_adjustment (CTK_SPIN_BUTTON (priv->size_spin));

  if (event->direction == CDK_SCROLL_UP || event->direction == CDK_SCROLL_RIGHT)
    ctk_adjustment_set_value (adj,
                              ctk_adjustment_get_value (adj) +
                              ctk_adjustment_get_step_increment (adj));
  else if (event->direction == CDK_SCROLL_DOWN || event->direction == CDK_SCROLL_LEFT)
    ctk_adjustment_set_value (adj,
                              ctk_adjustment_get_value (adj) -
                              ctk_adjustment_get_step_increment (adj));
  else if (event->direction == CDK_SCROLL_SMOOTH && event->delta_x != 0.0)
    ctk_adjustment_set_value (adj,
                              ctk_adjustment_get_value (adj) +
                              ctk_adjustment_get_step_increment (adj) * event->delta_x);
  else if (event->direction == CDK_SCROLL_SMOOTH && event->delta_y != 0.0)
    ctk_adjustment_set_value (adj,
                              ctk_adjustment_get_value (adj) -
                              ctk_adjustment_get_step_increment (adj) * event->delta_y);

  return TRUE;
}

static void
ctk_font_chooser_widget_update_preview_attributes (CtkFontChooserWidget *fontchooser)
{
  CtkFontChooserWidgetPrivate *priv;
  PangoAttrList *attrs;

  priv = fontchooser->priv;

  attrs = pango_attr_list_new ();

  /* Prevent font fallback */
  pango_attr_list_insert (attrs, pango_attr_fallback_new (FALSE));

  /* Force current font and features */
  pango_attr_list_insert (attrs, pango_attr_font_desc_new (priv->font_desc));
  if (priv->font_features)
    pango_attr_list_insert (attrs, pango_attr_font_features_new (priv->font_features));
  if (priv->language)
    pango_attr_list_insert (attrs, pango_attr_language_new (priv->language));

  ctk_entry_set_attributes (CTK_ENTRY (priv->preview), attrs);
  pango_attr_list_unref (attrs);
}

static void
row_inserted_cb (CtkTreeModel *model,
                 CtkTreePath  *path,
                 CtkTreeIter  *iter,
                 gpointer      user_data)
{
  CtkFontChooserWidget *fontchooser = user_data;
  CtkFontChooserWidgetPrivate *priv = fontchooser->priv;

  ctk_stack_set_visible_child_name (CTK_STACK (priv->list_stack), "list");
}

static void
row_deleted_cb  (CtkTreeModel *model,
                 CtkTreePath  *path,
                 gpointer      user_data)
{
  CtkFontChooserWidget *fontchooser = user_data;
  CtkFontChooserWidgetPrivate *priv = fontchooser->priv;

  if (ctk_tree_model_iter_n_children (model, NULL) == 0)
    ctk_stack_set_visible_child_name (CTK_STACK (priv->list_stack), "empty");
}

static void
ctk_font_chooser_widget_map (CtkWidget *widget)
{
  CtkFontChooserWidget *fontchooser = CTK_FONT_CHOOSER_WIDGET (widget);
  CtkFontChooserWidgetPrivate *priv = fontchooser->priv;

  ctk_entry_set_text (CTK_ENTRY (priv->search_entry), "");
  ctk_stack_set_visible_child_name (CTK_STACK (priv->stack), "list");
  g_simple_action_set_state (G_SIMPLE_ACTION (priv->tweak_action), g_variant_new_boolean (FALSE));

  CTK_WIDGET_CLASS (ctk_font_chooser_widget_parent_class)->map (widget);
}

static void
ctk_font_chooser_widget_class_init (CtkFontChooserWidgetClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);
  GParamSpec *pspec;

  g_type_ensure (CTK_TYPE_DELAYED_FONT_DESCRIPTION);
  g_type_ensure (G_TYPE_THEMED_ICON);

  widget_class->screen_changed = ctk_font_chooser_widget_screen_changed;
  widget_class->map = ctk_font_chooser_widget_map;

  gobject_class->finalize = ctk_font_chooser_widget_finalize;
  gobject_class->set_property = ctk_font_chooser_widget_set_property;
  gobject_class->get_property = ctk_font_chooser_widget_get_property;

  /**
   * CtkFontChooserWidget:tweak-action:
   *
   * A toggle action that can be used to switch to the tweak page
   * of the font chooser widget, which lets the user tweak the
   * OpenType features and variation axes of the selected font.
   *
   * The action will be enabled or disabled depending on whether
   * the selected font has any features or axes.
   */
  pspec = g_param_spec_object ("tweak-action",
                               P_("The tweak action"),
                               P_("The toggle action to switch to the tweak page"),
                               G_TYPE_ACTION,
                               G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (gobject_class, PROP_TWEAK_ACTION, pspec);

  _ctk_font_chooser_install_properties (gobject_class);

  /* Bind class to template */
  ctk_widget_class_set_template_from_resource (widget_class,
					       "/org/ctk/libctk/ui/ctkfontchooserwidget.ui");

  ctk_widget_class_bind_template_child_private (widget_class, CtkFontChooserWidget, search_entry);
  ctk_widget_class_bind_template_child_private (widget_class, CtkFontChooserWidget, family_face_list);
  ctk_widget_class_bind_template_child_private (widget_class, CtkFontChooserWidget, family_face_column);
  ctk_widget_class_bind_template_child_private (widget_class, CtkFontChooserWidget, family_face_cell);
  ctk_widget_class_bind_template_child_private (widget_class, CtkFontChooserWidget, list_scrolled_window);
  ctk_widget_class_bind_template_child_private (widget_class, CtkFontChooserWidget, list_stack);
  ctk_widget_class_bind_template_child_private (widget_class, CtkFontChooserWidget, model);
  ctk_widget_class_bind_template_child_private (widget_class, CtkFontChooserWidget, filter_model);
  ctk_widget_class_bind_template_child_private (widget_class, CtkFontChooserWidget, preview);
  ctk_widget_class_bind_template_child_private (widget_class, CtkFontChooserWidget, preview2);
  ctk_widget_class_bind_template_child_private (widget_class, CtkFontChooserWidget, size_label);
  ctk_widget_class_bind_template_child_private (widget_class, CtkFontChooserWidget, size_spin);
  ctk_widget_class_bind_template_child_private (widget_class, CtkFontChooserWidget, size_slider);
  ctk_widget_class_bind_template_child_private (widget_class, CtkFontChooserWidget, size_slider2);
  ctk_widget_class_bind_template_child_private (widget_class, CtkFontChooserWidget, stack);
  ctk_widget_class_bind_template_child_private (widget_class, CtkFontChooserWidget, font_name_label);
  ctk_widget_class_bind_template_child_private (widget_class, CtkFontChooserWidget, feature_box);
  ctk_widget_class_bind_template_child_private (widget_class, CtkFontChooserWidget, axis_grid);

  ctk_widget_class_bind_template_callback (widget_class, text_changed_cb);
  ctk_widget_class_bind_template_callback (widget_class, stop_search_cb);
  ctk_widget_class_bind_template_callback (widget_class, cursor_changed_cb);
  ctk_widget_class_bind_template_callback (widget_class, row_activated_cb);
  ctk_widget_class_bind_template_callback (widget_class, ctk_font_chooser_widget_set_cell_size);
  ctk_widget_class_bind_template_callback (widget_class, resize_by_scroll_cb);
  ctk_widget_class_bind_template_callback (widget_class, row_deleted_cb);
  ctk_widget_class_bind_template_callback (widget_class, row_inserted_cb);
  ctk_widget_class_bind_template_callback (widget_class, row_deleted_cb);
  ctk_widget_class_bind_template_callback (widget_class, size_change_cb);
  ctk_widget_class_bind_template_callback (widget_class, output_cb);
  ctk_widget_class_bind_template_callback (widget_class, selection_changed);

  ctk_widget_class_set_css_name (widget_class, I_("fontchooser"));
}

static void
change_tweak (GSimpleAction *action,
              GVariant      *state,
              gpointer       data)
{
  CtkFontChooserWidget *fontchooser = data;
  gboolean tweak = g_variant_get_boolean (state);

  if (tweak)
    {
      ctk_entry_grab_focus_without_selecting (CTK_ENTRY (fontchooser->priv->preview2));
      ctk_stack_set_visible_child_name (CTK_STACK (fontchooser->priv->stack), "tweaks");
    }
  else
    {
      ctk_entry_grab_focus_without_selecting (CTK_ENTRY (fontchooser->priv->search_entry));
      ctk_stack_set_visible_child_name (CTK_STACK (fontchooser->priv->stack), "list");
    }

  g_simple_action_set_state (action, state);
}

typedef struct {
  guint32 tag;
  CtkAdjustment *adjustment;
  CtkWidget *label;
  CtkWidget *scale;
  CtkWidget *spin;
  CtkWidget *fontchooser;
} Axis;

static guint
axis_hash (gconstpointer v)
{
  const Axis *a = v;

  return a->tag;
}

static gboolean
axis_equal (gconstpointer v1, gconstpointer v2)
{
  const Axis *a1 = v1;
  const Axis *a2 = v2;

  return a1->tag == a2->tag;
}

static void
axis_remove (gpointer key,
             gpointer value,
             gpointer data)
{
  Axis *a = value;

  ctk_widget_destroy (a->label);
  ctk_widget_destroy (a->scale);
  ctk_widget_destroy (a->spin);
}

static void
axis_free (gpointer v)
{
  Axis *a = v;

  g_free (a);
}

static void
ctk_font_chooser_widget_init (CtkFontChooserWidget *fontchooser)
{
  CtkFontChooserWidgetPrivate *priv;

  fontchooser->priv = ctk_font_chooser_widget_get_instance_private (fontchooser);
  priv = fontchooser->priv;

  ctk_widget_init_template (CTK_WIDGET (fontchooser));

  priv->axes = g_hash_table_new_full (axis_hash, axis_equal, NULL, axis_free);

  /* Default preview string  */
  priv->preview_text = g_strdup (pango_language_get_sample_string (NULL));
  priv->show_preview_entry = TRUE;
  priv->font_desc = pango_font_description_new ();
  priv->level = CTK_FONT_CHOOSER_LEVEL_FAMILY |
                CTK_FONT_CHOOSER_LEVEL_STYLE |
                CTK_FONT_CHOOSER_LEVEL_SIZE;
  priv->language = pango_language_get_default ();

  /* Set default preview text */
  ctk_entry_set_text (CTK_ENTRY (priv->preview), priv->preview_text);

  ctk_font_chooser_widget_update_preview_attributes (fontchooser);

  ctk_widget_add_events (priv->preview, CDK_SCROLL_MASK);

  /* Set the upper values of the spin/scale with G_MAXINT / PANGO_SCALE */
  ctk_spin_button_set_range (CTK_SPIN_BUTTON (priv->size_spin),
                             1.0, (gdouble)(G_MAXINT / PANGO_SCALE));
  ctk_adjustment_set_upper (ctk_range_get_adjustment (CTK_RANGE (priv->size_slider)),
                            (gdouble)(G_MAXINT / PANGO_SCALE));

  /* Setup treeview/model auxilary functions */
  ctk_tree_model_filter_set_visible_func (CTK_TREE_MODEL_FILTER (priv->filter_model),
                                          visible_func, (gpointer)priv, NULL);

  ctk_tree_view_column_set_cell_data_func (priv->family_face_column,
                                           priv->family_face_cell,
                                           ctk_font_chooser_widget_cell_data_func,
                                           fontchooser,
                                           NULL);

  priv->tweak_action = G_ACTION (g_simple_action_new_stateful ("tweak", NULL, g_variant_new_boolean (FALSE)));
  g_signal_connect (priv->tweak_action, "change-state", G_CALLBACK (change_tweak), fontchooser);

  /* Load data and set initial style-dependent parameters */
  ctk_font_chooser_widget_load_fonts (fontchooser, TRUE);
  ctk_font_chooser_widget_populate_features (fontchooser);

  ctk_font_chooser_widget_set_cell_size (fontchooser);
  ctk_font_chooser_widget_take_font_desc (fontchooser, NULL);
}

/**
 * ctk_font_chooser_widget_new:
 *
 * Creates a new #CtkFontChooserWidget.
 *
 * Returns: a new #CtkFontChooserWidget
 *
 * Since: 3.2
 */
CtkWidget *
ctk_font_chooser_widget_new (void)
{
  return g_object_new (CTK_TYPE_FONT_CHOOSER_WIDGET, NULL);
}

static int
cmp_families (const void *a,
              const void *b)
{
  const char *a_name = pango_font_family_get_name (*(PangoFontFamily **)a);
  const char *b_name = pango_font_family_get_name (*(PangoFontFamily **)b);

  return g_utf8_collate (a_name, b_name);
}

static void
ctk_font_chooser_widget_load_fonts (CtkFontChooserWidget *fontchooser,
                                    gboolean              force)
{
  CtkFontChooserWidgetPrivate *priv = fontchooser->priv;
  CtkListStore *list_store;
  gint n_families, i;
  PangoFontFamily **families;
  guint fontconfig_timestamp;
  gboolean need_reload;
  PangoFontMap *font_map;

  g_object_get (ctk_widget_get_settings (CTK_WIDGET (fontchooser)),
                "ctk-fontconfig-timestamp", &fontconfig_timestamp,
                NULL);

  /* The fontconfig timestamp is only set on systems with fontconfig; every
   * other platform will set it to 0. For those systems, we fall back to
   * reloading the fonts every time.
   */
  need_reload = fontconfig_timestamp == 0 ||
                fontconfig_timestamp != priv->last_fontconfig_timestamp;

  priv->last_fontconfig_timestamp = fontconfig_timestamp;

  if (!need_reload && !force)
    return;

  list_store = CTK_LIST_STORE (priv->model);

  if (priv->font_map)
    font_map = priv->font_map;
  else
    font_map = pango_cairo_font_map_get_default ();
  pango_font_map_list_families (font_map, &families, &n_families);

  qsort (families, n_families, sizeof (PangoFontFamily *), cmp_families);

  g_signal_handlers_block_by_func (priv->family_face_list, cursor_changed_cb, fontchooser);
  ctk_list_store_clear (list_store);
  g_signal_handlers_unblock_by_func (priv->family_face_list, cursor_changed_cb, fontchooser);

  /* Iterate over families and faces */
  for (i = 0; i < n_families; i++)
    {
      CtkTreeIter     iter;
      const gchar    *fam_name = pango_font_family_get_name (families[i]);

      if ((priv->level & CTK_FONT_CHOOSER_LEVEL_STYLE) == 0)
        {
          CtkDelayedFontDescription *desc;
          PangoFontFace *face;

#if PANGO_VERSION_CHECK(1,46,0)
          face = pango_font_family_get_face (families[i], NULL);
#else
          {
            PangoFontFace **faces;
            int j, n_faces;
            pango_font_family_list_faces (families[i], &faces, &n_faces);
            face = faces[0];
            for (j = 0; j < n_faces; j++)
              {
                if (strcmp (pango_font_face_get_face_name (faces[j]), "Regular") == 0)
                  {
                    face = faces[j];
                    break;
                  }
              }
            g_free (faces);
          }
#endif
          desc = ctk_delayed_font_description_new (face);

          ctk_list_store_insert_with_values (list_store, &iter, -1,
                                             FAMILY_COLUMN, families[i],
                                             FACE_COLUMN, face,
                                             FONT_DESC_COLUMN, desc,
                                             PREVIEW_TITLE_COLUMN, fam_name,
                                             -1);

          ctk_delayed_font_description_unref (desc);
        }
      else
        {
          PangoFontFace **faces;
          int j, n_faces;

          pango_font_family_list_faces (families[i], &faces, &n_faces);

          for (j = 0; j < n_faces; j++)
            {
              CtkDelayedFontDescription *desc;
              const gchar *face_name;
              char *title;

              face_name = pango_font_face_get_face_name (faces[j]);
              title = g_strconcat (fam_name, " ", face_name, NULL);
              desc = ctk_delayed_font_description_new (faces[j]);

              ctk_list_store_insert_with_values (list_store, &iter, -1,
                                                 FAMILY_COLUMN, families[i],
                                                 FACE_COLUMN, faces[j],
                                                 FONT_DESC_COLUMN, desc,
                                                 PREVIEW_TITLE_COLUMN, title,
                                                 -1);

              g_free (title);
              ctk_delayed_font_description_unref (desc);
            }

          g_free (faces);
        }
    }

  g_free (families);

  /* now make sure the font list looks right */
  if (!ctk_font_chooser_widget_find_font (fontchooser, priv->font_desc, &priv->font_iter))
    memset (&priv->font_iter, 0, sizeof (CtkTreeIter));

  ctk_font_chooser_widget_ensure_selection (fontchooser);
}

static gboolean
visible_func (CtkTreeModel *model,
              CtkTreeIter  *iter,
              gpointer      user_data)
{
  CtkFontChooserWidgetPrivate *priv = user_data;
  gboolean result = TRUE;
  const gchar *search_text;
  gchar **split_terms;
  gchar *font_name, *font_name_casefold;
  guint i;

  if (priv->filter_func != NULL)
    {
      PangoFontFamily *family;
      PangoFontFace *face;

      ctk_tree_model_get (model, iter,
                          FAMILY_COLUMN, &family,
                          FACE_COLUMN, &face,
                          -1);

      result = priv->filter_func (family, face, priv->filter_data);

      g_object_unref (family);
      g_object_unref (face);
      
      if (!result)
        return FALSE;
    }

  /* If there's no filter string we show the item */
  search_text = ctk_entry_get_text (CTK_ENTRY (priv->search_entry));
  if (strlen (search_text) == 0)
    return TRUE;

  ctk_tree_model_get (model, iter,
                      PREVIEW_TITLE_COLUMN, &font_name,
                      -1);

  if (font_name == NULL)
    return FALSE;

  split_terms = g_strsplit (search_text, " ", 0);
  font_name_casefold = g_utf8_casefold (font_name, -1);

  for (i = 0; split_terms[i] && result; i++)
    {
      gchar* term_casefold = g_utf8_casefold (split_terms[i], -1);

      if (!strstr (font_name_casefold, term_casefold))
        result = FALSE;

      g_free (term_casefold);
    }

  g_free (font_name_casefold);
  g_free (font_name);
  g_strfreev (split_terms);

  return result;
}

/* in pango units */
static int
ctk_font_chooser_widget_get_preview_text_height (CtkFontChooserWidget *fontchooser)
{
  CtkWidget *treeview = fontchooser->priv->family_face_list;
  double dpi, font_size;

  dpi = cdk_screen_get_resolution (ctk_widget_get_screen (treeview));
  ctk_style_context_get (ctk_widget_get_style_context (treeview),
                         ctk_widget_get_state_flags (treeview),
                         "font-size", &font_size,
                         NULL);

  return (dpi < 0.0 ? 96.0 : dpi) / 72.0 * PANGO_SCALE_X_LARGE * font_size * PANGO_SCALE;
}

static PangoAttrList *
ctk_font_chooser_widget_get_preview_attributes (CtkFontChooserWidget       *fontchooser,
                                                const PangoFontDescription *font_desc)
{
  PangoAttribute *attribute;
  PangoAttrList *attrs;

  attrs = pango_attr_list_new ();

  if (font_desc)
    {
      attribute = pango_attr_font_desc_new (font_desc);
      pango_attr_list_insert (attrs, attribute);
    }

  attribute = pango_attr_size_new_absolute (ctk_font_chooser_widget_get_preview_text_height (fontchooser));
  pango_attr_list_insert (attrs, attribute);

  return attrs;
}

static void
ctk_font_chooser_widget_cell_data_func (CtkTreeViewColumn *column,
                                        CtkCellRenderer   *cell,
                                        CtkTreeModel      *tree_model,
                                        CtkTreeIter       *iter,
                                        gpointer           user_data)
{
  CtkFontChooserWidget *fontchooser = user_data;
  CtkDelayedFontDescription *desc;
  PangoAttrList *attrs;
  char *preview_title;

  ctk_tree_model_get (tree_model, iter,
                      PREVIEW_TITLE_COLUMN, &preview_title,
                      FONT_DESC_COLUMN, &desc,
                      -1);

  attrs = ctk_font_chooser_widget_get_preview_attributes (fontchooser,
                                                          ctk_delayed_font_description_get (desc));

  g_object_set (cell,
                "xpad", 20,
                "ypad", 10,
                "attributes", attrs,
                "text", preview_title,
                NULL);

  ctk_delayed_font_description_unref (desc);
  pango_attr_list_unref (attrs);
  g_free (preview_title);
}

static void
ctk_font_chooser_widget_set_cell_size (CtkFontChooserWidget *fontchooser)
{
  CtkFontChooserWidgetPrivate *priv = fontchooser->priv;
  PangoAttrList *attrs;
  CtkRequisition size;

  ctk_cell_renderer_set_fixed_size (priv->family_face_cell, -1, -1);

  attrs = ctk_font_chooser_widget_get_preview_attributes (fontchooser, NULL);

  g_object_set (priv->family_face_cell,
                "xpad", 20,
                "ypad", 10,
                "attributes", attrs,
                "text", "x",
                NULL);

  pango_attr_list_unref (attrs);

  ctk_cell_renderer_get_preferred_size (priv->family_face_cell,
                                        priv->family_face_list,
                                        &size,
                                        NULL);
  ctk_cell_renderer_set_fixed_size (priv->family_face_cell, size.width, size.height);
}

static void
ctk_font_chooser_widget_finalize (GObject *object)
{
  CtkFontChooserWidget *fontchooser = CTK_FONT_CHOOSER_WIDGET (object);
  CtkFontChooserWidgetPrivate *priv = fontchooser->priv;

  if (priv->font_desc)
    pango_font_description_free (priv->font_desc);

  if (priv->filter_data_destroy)
    priv->filter_data_destroy (priv->filter_data);

  g_free (priv->preview_text);

  g_clear_object (&priv->font_map);

  g_object_unref (priv->tweak_action);

  g_list_free_full (priv->feature_items, g_free);

  if (priv->axes)
    g_hash_table_unref (priv->axes);

  g_free (priv->font_features);

  G_OBJECT_CLASS (ctk_font_chooser_widget_parent_class)->finalize (object);
}

static gboolean
my_pango_font_family_equal (const char *familya,
                            const char *familyb)
{
  return g_ascii_strcasecmp (familya, familyb) == 0;
}

static gboolean
ctk_font_chooser_widget_find_font (CtkFontChooserWidget        *fontchooser,
                                   const PangoFontDescription  *font_desc,
                                   /* out arguments */
                                   CtkTreeIter                 *iter)
{
  CtkFontChooserWidgetPrivate *priv = fontchooser->priv;
  gboolean valid;

  if (pango_font_description_get_family (font_desc) == NULL)
    return FALSE;

  for (valid = ctk_tree_model_get_iter_first (priv->model, iter);
       valid;
       valid = ctk_tree_model_iter_next (priv->model, iter))
    {
      CtkDelayedFontDescription *desc;
      PangoFontDescription *merged;
      PangoFontFamily *family;

      ctk_tree_model_get (priv->model, iter,
                          FAMILY_COLUMN, &family,
                          FONT_DESC_COLUMN, &desc,
                          -1);

      if (!my_pango_font_family_equal (pango_font_description_get_family (font_desc),
                                       pango_font_family_get_name (family)))
        {
          ctk_delayed_font_description_unref (desc);
          g_object_unref (family);
          continue;
        }

      merged = pango_font_description_copy_static (ctk_delayed_font_description_get (desc));

      pango_font_description_merge_static (merged, font_desc, FALSE);
      if (pango_font_description_equal (merged, font_desc))
        {
          ctk_delayed_font_description_unref (desc);
          pango_font_description_free (merged);
          g_object_unref (family);
          break;
        }

      ctk_delayed_font_description_unref (desc);
      pango_font_description_free (merged);
      g_object_unref (family);
    }
  
  return valid;
}

static void
fontconfig_changed (CtkFontChooserWidget *fontchooser)
{
  ctk_font_chooser_widget_load_fonts (fontchooser, TRUE);
}

static void
ctk_font_chooser_widget_screen_changed (CtkWidget *widget,
                                        CdkScreen *previous_screen)
{
  CtkFontChooserWidget *fontchooser = CTK_FONT_CHOOSER_WIDGET (widget);
  CtkSettings *settings;

  if (CTK_WIDGET_CLASS (ctk_font_chooser_widget_parent_class)->screen_changed)
    CTK_WIDGET_CLASS (ctk_font_chooser_widget_parent_class)->screen_changed (widget, previous_screen);

  if (previous_screen)
    {
      settings = ctk_settings_get_for_screen (previous_screen);
      g_signal_handlers_disconnect_by_func (settings, fontconfig_changed, widget);
    }
  settings = ctk_widget_get_settings (widget);
  g_signal_connect_object (settings, "notify::ctk-fontconfig-timestamp",
                           G_CALLBACK (fontconfig_changed), widget, G_CONNECT_SWAPPED);

  if (previous_screen == NULL)
    previous_screen = cdk_screen_get_default ();

  if (previous_screen == ctk_widget_get_screen (widget))
    return;

  ctk_font_chooser_widget_load_fonts (fontchooser, FALSE);
}

static PangoFontFamily *
ctk_font_chooser_widget_get_family (CtkFontChooser *chooser)
{
  CtkFontChooserWidget *fontchooser = CTK_FONT_CHOOSER_WIDGET (chooser);
  CtkFontChooserWidgetPrivate *priv = fontchooser->priv;
  PangoFontFamily *family;

  if (!ctk_list_store_iter_is_valid (CTK_LIST_STORE (priv->model), &priv->font_iter))
    return NULL;

  ctk_tree_model_get (priv->model, &priv->font_iter,
                      FAMILY_COLUMN, &family,
                      -1);
  g_object_unref (family);

  return family;
}

static PangoFontFace *
ctk_font_chooser_widget_get_face (CtkFontChooser *chooser)
{
  CtkFontChooserWidget *fontchooser = CTK_FONT_CHOOSER_WIDGET (chooser);
  CtkFontChooserWidgetPrivate *priv = fontchooser->priv;
  PangoFontFace *face;

  if (!ctk_list_store_iter_is_valid (CTK_LIST_STORE (priv->model), &priv->font_iter))
    return NULL;

  ctk_tree_model_get (priv->model, &priv->font_iter,
                      FACE_COLUMN, &face,
                      -1);
  g_object_unref (face);

  return face;
}

static gint
ctk_font_chooser_widget_get_size (CtkFontChooser *chooser)
{
  CtkFontChooserWidget *fontchooser = CTK_FONT_CHOOSER_WIDGET (chooser);
  PangoFontDescription *desc = ctk_font_chooser_widget_get_font_desc (fontchooser);

  if (desc)
    return pango_font_description_get_size (desc);

  return -1;
}

static gchar *
ctk_font_chooser_widget_get_font (CtkFontChooserWidget *fontchooser)
{
  PangoFontDescription *desc = ctk_font_chooser_widget_get_font_desc (fontchooser);

  if (desc)
    return pango_font_description_to_string (desc);

  return NULL;
}

static PangoFontDescription *
ctk_font_chooser_widget_get_font_desc (CtkFontChooserWidget *fontchooser)
{
  CtkFontChooserWidgetPrivate *priv = fontchooser->priv;
  CtkTreeSelection *selection;

  selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (priv->family_face_list));
  if (ctk_tree_selection_count_selected_rows (selection) > 0)
    return fontchooser->priv->font_desc;

  return NULL;
}

static void
ctk_font_chooser_widget_set_font (CtkFontChooserWidget *fontchooser,
                                  const gchar          *fontname)
{
  PangoFontDescription *font_desc;

  font_desc = pango_font_description_from_string (fontname);
  ctk_font_chooser_widget_take_font_desc (fontchooser, font_desc);
}

static void
ctk_font_chooser_widget_update_font_name (CtkFontChooserWidget *fontchooser,
                                          CtkTreeSelection     *selection)
{
  CtkFontChooserWidgetPrivate *priv = fontchooser->priv;
  CtkTreeModel *model;
  CtkTreeIter iter;
  PangoFontFamily *family;
  PangoFontFace *face;
  CtkDelayedFontDescription *desc;
  const PangoFontDescription *font_desc;
  PangoAttrList *attrs;
  const char *fam_name;
  const char *face_name;
  char *title;

  ctk_tree_selection_get_selected (selection, &model, &iter);
  ctk_tree_model_get (model, &iter,
                      FAMILY_COLUMN, &family,
                      FACE_COLUMN, &face,
                      FONT_DESC_COLUMN, &desc,
                      -1);

  fam_name = pango_font_family_get_name (family);
  face_name = pango_font_face_get_face_name (face);
  font_desc = ctk_delayed_font_description_get (desc);

  g_object_unref (family);
  g_object_unref (face);
  ctk_delayed_font_description_unref (desc);

  if (priv->level == CTK_FONT_CHOOSER_LEVEL_FAMILY)
    title = g_strdup (fam_name);
  else
    title = g_strconcat (fam_name, " ", face_name, NULL);

  attrs = ctk_font_chooser_widget_get_preview_attributes (fontchooser, font_desc);
  ctk_label_set_attributes (CTK_LABEL (priv->font_name_label), attrs);
  pango_attr_list_unref (attrs);

  ctk_label_set_label (CTK_LABEL (priv->font_name_label), title);
  g_free (title);
}

static void
selection_changed (CtkTreeSelection     *selection,
                   CtkFontChooserWidget *fontchooser)
{
  CtkFontChooserWidgetPrivate *priv = fontchooser->priv;

  g_object_notify (G_OBJECT (fontchooser), "font");
  g_object_notify (G_OBJECT (fontchooser), "font-desc");

  if (ctk_tree_selection_count_selected_rows (selection) > 0)
    {
      ctk_font_chooser_widget_update_font_name (fontchooser, selection);
      g_simple_action_set_enabled (G_SIMPLE_ACTION (priv->tweak_action), TRUE);
    }
  else
    {
      g_simple_action_set_state (G_SIMPLE_ACTION (priv->tweak_action), g_variant_new_boolean (FALSE));
      g_simple_action_set_enabled (G_SIMPLE_ACTION (priv->tweak_action), FALSE);
    }
}

static void
ctk_font_chooser_widget_ensure_selection (CtkFontChooserWidget *fontchooser)
{
  CtkFontChooserWidgetPrivate *priv = fontchooser->priv;
  CtkTreeSelection *selection;
  CtkTreeIter filter_iter;
  
  selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (priv->family_face_list));

  if (ctk_list_store_iter_is_valid (CTK_LIST_STORE (priv->model), &priv->font_iter) &&
      ctk_tree_model_filter_convert_child_iter_to_iter (CTK_TREE_MODEL_FILTER (priv->filter_model),
                                                        &filter_iter,
                                                        &priv->font_iter))
    {
      CtkTreePath *path = ctk_tree_model_get_path (CTK_TREE_MODEL (priv->filter_model),
                                                   &filter_iter);

      ctk_tree_selection_select_iter (selection, &filter_iter);
      ctk_tree_view_scroll_to_cell (CTK_TREE_VIEW (priv->family_face_list),
                                    path, NULL, FALSE, 0.0, 0.0);
      ctk_tree_path_free (path);
    }
  else
    {
      ctk_tree_selection_unselect_all (selection);
    }
}

/* OpenType variations */

static void
add_font_variations (CtkFontChooserWidget *fontchooser,
                     GString              *s)
{
  GHashTableIter iter;
  Axis *axis;
  const char *sep = "";
  char buf[G_ASCII_DTOSTR_BUF_SIZE];

  g_hash_table_iter_init (&iter, fontchooser->priv->axes);
  while (g_hash_table_iter_next (&iter, (gpointer *)NULL, (gpointer *)&axis))
    {
      char tag[5];
      double value;

      tag[0] = (axis->tag >> 24) & 0xff;
      tag[1] = (axis->tag >> 16) & 0xff;
      tag[2] = (axis->tag >> 8) & 0xff;
      tag[3] = (axis->tag >> 0) & 0xff;
      tag[4] = '\0';
      value = ctk_adjustment_get_value (axis->adjustment);
      g_string_append_printf (s, "%s%s=%s", sep, tag, g_ascii_dtostr (buf, sizeof(buf), value));
      sep = ",";
    }
}

static void
adjustment_changed (CtkAdjustment *adjustment,
                    Axis          *axis)
{
  CtkFontChooserWidget *fontchooser = CTK_FONT_CHOOSER_WIDGET (axis->fontchooser);
  CtkFontChooserWidgetPrivate *priv = fontchooser->priv;
  PangoFontDescription *font_desc;
  GString *s;

  priv->updating_variations = TRUE;

  s = g_string_new ("");
  add_font_variations (fontchooser, s);

  if (s->len > 0)
    {
      font_desc = pango_font_description_new ();
      pango_font_description_set_variations (font_desc, s->str);
      ctk_font_chooser_widget_take_font_desc (fontchooser, font_desc);
    }

  g_string_free (s, TRUE);

  priv->updating_variations = FALSE;
}

static gboolean
should_show_axis (hb_ot_var_axis_info_t *ax)
{
  if (ax->flags & HB_OT_VAR_AXIS_FLAG_HIDDEN)
    return FALSE;

  return TRUE;
}

static gboolean
is_named_instance (hb_font_t *font)
{
  /* FIXME */
  return FALSE;
}

static struct {
  guint32 tag;
  const char *name;
} axis_names[] = {
  { HB_OT_TAG_VAR_AXIS_WIDTH,        N_("Width") },
  { HB_OT_TAG_VAR_AXIS_WEIGHT,       N_("Weight") },
  { HB_OT_TAG_VAR_AXIS_ITALIC,       N_("Italic") },
  { HB_OT_TAG_VAR_AXIS_SLANT,        N_("Slant") },
  { HB_OT_TAG_VAR_AXIS_OPTICAL_SIZE, N_("Optical Size") },
};

static gboolean
add_axis (CtkFontChooserWidget  *fontchooser,
          hb_font_t             *hb_font,
          hb_ot_var_axis_info_t *ax,
          int                    value,
          int                    row)
{
  CtkFontChooserWidgetPrivate *priv = fontchooser->priv;
  hb_face_t *hb_face;
  Axis *axis;
  const char *name;
  char buffer[20];
  unsigned int buffer_len = 20;
  int i;

  hb_face = hb_font_get_face (hb_font);

  axis = g_new (Axis, 1);
  axis->tag = ax->tag;
  axis->fontchooser = CTK_WIDGET (fontchooser);

  hb_ot_name_get_utf8 (hb_face, ax->name_id, HB_LANGUAGE_INVALID, &buffer_len, buffer);
  name = buffer;

  for (i = 0; i < G_N_ELEMENTS (axis_names); i++)
    {
      if (axis_names[i].tag == ax->tag)
        {
          name = _(axis_names[i].name);
          break;
        }
    }

  axis->label = ctk_label_new (name);

  ctk_widget_show (axis->label);
  ctk_widget_set_halign (axis->label, CTK_ALIGN_START);
  ctk_widget_set_valign (axis->label, CTK_ALIGN_BASELINE);
  ctk_grid_attach (CTK_GRID (priv->axis_grid), axis->label, 0, row, 1, 1);
  axis->adjustment = ctk_adjustment_new ((double)value,
                                         (double)ax->min_value,
                                         (double)ax->max_value,
                                         1.0, 10.0, 0.0);
  axis->scale = ctk_scale_new (CTK_ORIENTATION_HORIZONTAL, axis->adjustment);
  ctk_widget_show (axis->scale);
  ctk_scale_add_mark (CTK_SCALE (axis->scale), (double)ax->default_value, CTK_POS_TOP, NULL);
  ctk_widget_set_valign (axis->scale, CTK_ALIGN_BASELINE);
  ctk_widget_set_hexpand (axis->scale, TRUE);
  ctk_widget_set_size_request (axis->scale, 100, -1);
  ctk_scale_set_draw_value (CTK_SCALE (axis->scale), FALSE);
  ctk_grid_attach (CTK_GRID (priv->axis_grid), axis->scale, 1, row, 1, 1);
  axis->spin = ctk_spin_button_new (axis->adjustment, 0, 0);
  ctk_widget_show (axis->spin);
  g_signal_connect (axis->spin, "output", G_CALLBACK (output_cb), fontchooser);
  ctk_widget_set_valign (axis->spin, CTK_ALIGN_BASELINE);
  ctk_grid_attach (CTK_GRID (priv->axis_grid), axis->spin, 2, row, 1, 1);

  g_hash_table_add (priv->axes, axis);

  adjustment_changed (axis->adjustment, axis);
  g_signal_connect (axis->adjustment, "value-changed", G_CALLBACK (adjustment_changed), axis);
  if (is_named_instance (hb_font) || !should_show_axis (ax))
    {
      ctk_widget_hide (axis->label);
      ctk_widget_hide (axis->scale);
      ctk_widget_hide (axis->spin);

      return FALSE;
    }

  return TRUE;
}

/* FIXME: This doesn't work if the font has an avar table */
static float
denorm_coord (hb_ot_var_axis_info_t *axis, int coord)
{
  float r = coord / 16384.0;

  if (coord < 0)
    return axis->default_value + r * (axis->default_value - axis->min_value);
  else
    return axis->default_value + r * (axis->max_value - axis->default_value);
}

static gboolean
ctk_font_chooser_widget_update_font_variations (CtkFontChooserWidget *fontchooser)
{
  CtkFontChooserWidgetPrivate *priv = fontchooser->priv;
  PangoFont *pango_font;
  hb_font_t *hb_font;
  hb_face_t *hb_face;
  const int *coords;
  unsigned int n_coords;
  unsigned int n_axes;
  hb_ot_var_axis_info_t *axes;
  gboolean has_axis = FALSE;
  int i;

  if (priv->updating_variations)
    return FALSE;

  g_hash_table_foreach (priv->axes, axis_remove, NULL);
  g_hash_table_remove_all (priv->axes);

  if ((priv->level & CTK_FONT_CHOOSER_LEVEL_VARIATIONS) == 0)
    return FALSE;

  pango_font = pango_context_load_font (ctk_widget_get_pango_context (CTK_WIDGET (fontchooser)),
                                        priv->font_desc);
  hb_font = pango_font_get_hb_font (pango_font);
  hb_face = hb_font_get_face (hb_font);

  if (!hb_ot_var_has_data (hb_face))
  return FALSE;

  coords = hb_font_get_var_coords_normalized (hb_font, &n_coords);

  n_axes = hb_ot_var_get_axis_count (hb_face);
  axes = g_new0 (hb_ot_var_axis_info_t, n_axes);
  hb_ot_var_get_axis_infos (hb_face, 0, &n_axes, axes);

  for (i = 0; i < n_axes; i++)
    {
      float value;
      if (coords && i < n_coords)
        value = denorm_coord (&axes[i], coords[i]);
      else
        value = axes[i].default_value;
      if (add_axis (fontchooser, hb_font, &axes[i], value, i + 4))
        has_axis = TRUE;
    }

  g_free (axes);
  g_object_unref (pango_font);

  return has_axis;
}

/* OpenType features */

/* look for a lang / script combination that matches the
 * language property and is supported by the hb_face. If
 * none is found, return the default lang / script tags.
 */
static void
find_language_and_script (CtkFontChooserWidget *fontchooser,
                          hb_face_t            *hb_face,
                          hb_tag_t             *lang_tag,
                          hb_tag_t             *script_tag)
{
  CtkFontChooserWidgetPrivate *priv = fontchooser->priv;
  gint i, j, k;
  hb_tag_t scripts[80];
  unsigned int n_scripts;
  unsigned int count;
  hb_tag_t table[2] = { HB_OT_TAG_GSUB, HB_OT_TAG_GPOS };
  hb_language_t lang;
  const char *langname, *p;

  langname = pango_language_to_string (priv->language);
  p = strchr (langname, '-');
  lang = hb_language_from_string (langname, p ? p - langname : -1);

  n_scripts = 0;
  for (i = 0; i < 2; i++)
    {
      count = G_N_ELEMENTS (scripts);
      hb_ot_layout_table_get_script_tags (hb_face, table[i], n_scripts, &count, scripts);
      n_scripts += count;
    }

  for (j = 0; j < n_scripts; j++)
    {
      hb_tag_t languages[80];
      unsigned int n_languages;

      n_languages = 0;
      for (i = 0; i < 2; i++)
        {
          count = G_N_ELEMENTS (languages);
          hb_ot_layout_script_get_language_tags (hb_face, table[i], j, n_languages, &count, languages);
          n_languages += count;
        }

      for (k = 0; k < n_languages; k++)
        {
          if (lang == hb_ot_tag_to_language (languages[k]))
            {
              *script_tag = scripts[j];
              *lang_tag = languages[k];
              return;
            }
        }
    }

  *lang_tag = HB_OT_TAG_DEFAULT_LANGUAGE;
  *script_tag = HB_OT_TAG_DEFAULT_SCRIPT;
}

typedef struct {
  hb_tag_t tag;
  const char *name;
  CtkWidget *top;
  CtkWidget *feat;
  CtkWidget *example;
} FeatureItem;

static const char *
get_feature_display_name (hb_tag_t tag)
{
  int i;

  for (i = 0; i < G_N_ELEMENTS (open_type_layout_features); i++)
    {
      if (tag == open_type_layout_features[i].tag)
        return g_dpgettext2 (NULL, "OpenType layout", open_type_layout_features[i].name);
    }

  return NULL;
}

static void
set_inconsistent (CtkCheckButton *button,
                  gboolean        inconsistent)
{
  if (inconsistent)
    ctk_widget_set_state_flags (CTK_WIDGET (button), CTK_STATE_FLAG_INCONSISTENT, FALSE);
  else
    ctk_widget_unset_state_flags (CTK_WIDGET (button), CTK_STATE_FLAG_INCONSISTENT);
//  ctk_widget_set_opacity (ctk_widget_get_first_child (CTK_WIDGET (button)), inconsistent ? 0.0 : 1.0);
}

static void
feat_clicked (CtkWidget *feat,
              gpointer   data)
{
  g_signal_handlers_block_by_func (feat, feat_clicked, NULL);

  if (ctk_widget_get_state_flags (feat) & CTK_STATE_FLAG_INCONSISTENT)
    {
      set_inconsistent (CTK_CHECK_BUTTON (feat), FALSE);
      ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (feat), TRUE);
    }

  g_signal_handlers_unblock_by_func (feat, feat_clicked, NULL);
}

static void
feat_pressed (CtkGesture *gesture,
              int         n_press,
              double      x,
              double      y,
              CtkWidget  *feat)
{
  gboolean inconsistent;

  inconsistent = (ctk_widget_get_state_flags (feat) & CTK_STATE_FLAG_INCONSISTENT) != 0;
  set_inconsistent (CTK_CHECK_BUTTON (feat), !inconsistent);
}

static char *
find_affected_text (hb_tag_t   feature_tag,
                    hb_font_t *hb_font,
                    hb_tag_t   script_tag,
                    hb_tag_t   lang_tag,
                    int        max_chars)
{
  hb_face_t *hb_face;
  unsigned int script_index = 0;
  unsigned int lang_index = 0;
  unsigned int feature_index = 0;
  GString *chars;

  hb_face = hb_font_get_face (hb_font);

  chars = g_string_new ("");

  hb_ot_layout_table_find_script (hb_face, HB_OT_TAG_GSUB, script_tag, &script_index);
  hb_ot_layout_script_find_language (hb_face, HB_OT_TAG_GSUB, script_index, lang_tag, &lang_index);
  if (hb_ot_layout_language_find_feature (hb_face, HB_OT_TAG_GSUB, script_index, lang_index, feature_tag, &feature_index))
    {
      unsigned int lookup_indexes[32];
      unsigned int lookup_count = 32;
      int count;
      int n_chars = 0;

      count  = hb_ot_layout_feature_get_lookups (hb_face,
                                                 HB_OT_TAG_GSUB,
                                                 feature_index,
                                                 0,
                                                 &lookup_count,
                                                 lookup_indexes);
      if (count > 0)
        {
          hb_set_t* glyphs_before = NULL;
          hb_set_t* glyphs_input  = NULL;
          hb_set_t* glyphs_after  = NULL;
          hb_set_t* glyphs_output = NULL;
          hb_codepoint_t gid;

          glyphs_input  = hb_set_create ();

          // XXX For now, just look at first index
          hb_ot_layout_lookup_collect_glyphs (hb_face,
                                              HB_OT_TAG_GSUB,
                                              lookup_indexes[0],
                                              glyphs_before,
                                              glyphs_input,
                                              glyphs_after,
                                              glyphs_output);

          gid = -1;
          while (hb_set_next (glyphs_input, &gid)) {
            hb_codepoint_t ch;
            if (n_chars == max_chars)
              {
                g_string_append (chars, "…");
                break;
              }
            for (ch = 0; ch < 0xffff; ch++) {
              hb_codepoint_t glyph = 0;
              hb_font_get_nominal_glyph (hb_font, ch, &glyph);
              if (glyph == gid) {
                g_string_append_unichar (chars, (gunichar)ch);
                n_chars++;
                break;
              }
            }
          }
          hb_set_destroy (glyphs_input);
        }
    }

  return g_string_free (chars, FALSE);
}

static void
update_feature_example (FeatureItem          *item,
                        hb_font_t            *hb_font,
                        hb_tag_t              script_tag,
                        hb_tag_t              lang_tag,
                        PangoFontDescription *font_desc)
{
  const char *letter_case[] = { "smcp", "c2sc", "pcap", "c2pc", "unic", "cpsp", "case", NULL };
  const char *number_case[] = { "xxxx", "lnum", "onum", NULL };
  const char *number_spacing[] = { "xxxx", "pnum", "tnum", NULL };
  const char *number_formatting[] = { "zero", "nalt", NULL };
  const char *char_variants[] = {
    "swsh", "cswh", "calt", "falt", "hist", "salt", "jalt", "titl", "rand",
    "ss01", "ss02", "ss03", "ss04", "ss05", "ss06", "ss07", "ss08", "ss09", "ss10",
    "ss11", "ss12", "ss13", "ss14", "ss15", "ss16", "ss17", "ss18", "ss19", "ss20",
    NULL };

  if (g_strv_contains (number_case, item->name) ||
      g_strv_contains (number_spacing, item->name))
    {
      PangoAttrList *attrs;
      PangoAttribute *attr;
      PangoFontDescription *desc;
      char *str;

      attrs = pango_attr_list_new ();

      desc = pango_font_description_copy (font_desc);
      pango_font_description_unset_fields (desc, PANGO_FONT_MASK_SIZE);
      pango_attr_list_insert (attrs, pango_attr_font_desc_new (desc));
      pango_font_description_free (desc);
      str = g_strconcat (item->name, " 1", NULL);
      attr = pango_attr_font_features_new (str);
      pango_attr_list_insert (attrs, attr);

      ctk_label_set_text (CTK_LABEL (item->example), "0123456789");
      ctk_label_set_attributes (CTK_LABEL (item->example), attrs);

      pango_attr_list_unref (attrs);
    }
  else if (g_strv_contains (letter_case, item->name) ||
           g_strv_contains (number_formatting, item->name) ||
           g_strv_contains (char_variants, item->name))
    {
      char *input = NULL;
      char *text;

      if (strcmp (item->name, "case") == 0)
        input = g_strdup ("A-B[Cq]");
      else if (g_strv_contains (letter_case, item->name))
        input = g_strdup ("AaBbCc…");
      else if (strcmp (item->name, "zero") == 0)
        input = g_strdup ("0");
      else if (strcmp (item->name, "nalt") == 0)
        input = find_affected_text (item->tag, hb_font, script_tag, lang_tag, 3);
      else
        input = find_affected_text (item->tag, hb_font, script_tag, lang_tag, 10);

      if (input[0] != '\0')
        {
          PangoAttrList *attrs;
          PangoAttribute *attr;
          PangoFontDescription *desc;
          char *str;

          text = g_strconcat (input, " ⟶ ", input, NULL);

          attrs = pango_attr_list_new ();

          desc = pango_font_description_copy (font_desc);
          pango_font_description_unset_fields (desc, PANGO_FONT_MASK_SIZE);
          pango_attr_list_insert (attrs, pango_attr_font_desc_new (desc));
          pango_font_description_free (desc);
          str = g_strconcat (item->name, " 0", NULL);
          attr = pango_attr_font_features_new (str);
          attr->start_index = 0;
          attr->end_index = strlen (input);
          pango_attr_list_insert (attrs, attr);
          str = g_strconcat (item->name, " 1", NULL);
          attr = pango_attr_font_features_new (str);
          attr->start_index = strlen (input) + strlen (" ⟶ ");
          attr->end_index = attr->start_index + strlen (input);
          pango_attr_list_insert (attrs, attr);

          ctk_label_set_text (CTK_LABEL (item->example), text);
          ctk_label_set_attributes (CTK_LABEL (item->example), attrs);

          g_free (text);
          pango_attr_list_unref (attrs);
        }
      else
        ctk_label_set_markup (CTK_LABEL (item->example), "");
      g_free (input);
    }
}

static void
add_check_group (CtkFontChooserWidget *fontchooser,
                 const char  *title,
                 const char **tags)
{
  CtkFontChooserWidgetPrivate *priv = fontchooser->priv;
  CtkWidget *label;
  CtkWidget *group;
  PangoAttrList *attrs;
  int i;

  group = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  ctk_widget_show (group);
  ctk_widget_set_halign (group, CTK_ALIGN_FILL);

  label = ctk_label_new (title);
  ctk_widget_show (label);
  ctk_label_set_xalign (CTK_LABEL (label), 0.0);
  ctk_widget_set_halign (label, CTK_ALIGN_START);
  g_object_set (label, "margin-top", 10, "margin-bottom", 10, NULL);
  attrs = pango_attr_list_new ();
  pango_attr_list_insert (attrs, pango_attr_weight_new (PANGO_WEIGHT_BOLD));
  ctk_label_set_attributes (CTK_LABEL (label), attrs);
  pango_attr_list_unref (attrs);
  ctk_container_add (CTK_CONTAINER (group), label);

  for (i = 0; tags[i]; i++)
    {
      hb_tag_t tag;
      CtkWidget *feat;
      FeatureItem *item;
      CtkGesture *gesture;
      CtkWidget *box;
      CtkWidget *example;

      tag = hb_tag_from_string (tags[i], -1);

      feat = ctk_check_button_new_with_label (get_feature_display_name (tag));
      ctk_widget_show (feat);
      set_inconsistent (CTK_CHECK_BUTTON (feat), TRUE);
      g_signal_connect_swapped (feat, "notify::active", G_CALLBACK (update_font_features), fontchooser);
      g_signal_connect_swapped (feat, "notify::inconsistent", G_CALLBACK (update_font_features), fontchooser);
      g_signal_connect (feat, "clicked", G_CALLBACK (feat_clicked), NULL);

      gesture = ctk_gesture_multi_press_new (feat);
      g_object_set_data_full (G_OBJECT (feat), "press", gesture, g_object_unref);

      ctk_gesture_single_set_button (CTK_GESTURE_SINGLE (gesture), CDK_BUTTON_SECONDARY);
      g_signal_connect (gesture, "pressed", G_CALLBACK (feat_pressed), feat);

      example = ctk_label_new ("");
      ctk_widget_show (example);
      ctk_label_set_selectable (CTK_LABEL (example), TRUE);
      ctk_widget_set_halign (example, CTK_ALIGN_START);

      box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 10);
      ctk_widget_show (box);
      ctk_box_set_homogeneous (CTK_BOX (box), TRUE);
      ctk_container_add (CTK_CONTAINER (box), feat);
      ctk_container_add (CTK_CONTAINER (box), example);
      ctk_container_add (CTK_CONTAINER (group), box);

      item = g_new (FeatureItem, 1);
      item->name = tags[i];
      item->tag = tag;
      item->top = box;
      item->feat = feat;
      item->example = example;

      priv->feature_items = g_list_prepend (priv->feature_items, item);
    }

  ctk_container_add (CTK_CONTAINER (priv->feature_box), group);
}

static void
add_radio_group (CtkFontChooserWidget *fontchooser,
                 const char  *title,
                 const char **tags)
{
  CtkFontChooserWidgetPrivate *priv = fontchooser->priv;
  CtkWidget *label;
  CtkWidget *group;
  int i;
  CtkWidget *group_button = NULL;
  PangoAttrList *attrs;

  group = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  ctk_widget_show (group);
  ctk_widget_set_halign (group, CTK_ALIGN_FILL);

  label = ctk_label_new (title);
  ctk_widget_show (label);
  ctk_label_set_xalign (CTK_LABEL (label), 0.0);
  ctk_widget_set_halign (label, CTK_ALIGN_START);
  g_object_set (label, "margin-top", 10, "margin-bottom", 10, NULL);
  attrs = pango_attr_list_new ();
  pango_attr_list_insert (attrs, pango_attr_weight_new (PANGO_WEIGHT_BOLD));
  ctk_label_set_attributes (CTK_LABEL (label), attrs);
  pango_attr_list_unref (attrs);
  ctk_container_add (CTK_CONTAINER (group), label);

  for (i = 0; tags[i]; i++)
    {
      hb_tag_t tag;
      CtkWidget *feat;
      FeatureItem *item;
      const char *name;
      CtkWidget *box;
      CtkWidget *example;

      tag = hb_tag_from_string (tags[i], -1);
      name = get_feature_display_name (tag);

      feat = ctk_radio_button_new_with_label_from_widget (CTK_RADIO_BUTTON (group_button),
                                                          name ? name : _("Default"));
      ctk_widget_show (feat);
      if (group_button == NULL)
        group_button = feat;

      g_signal_connect_swapped (feat, "notify::active", G_CALLBACK (update_font_features), fontchooser);
      g_object_set_data (G_OBJECT (feat), "default", group_button);

      example = ctk_label_new ("");
      ctk_widget_show (example);
      ctk_label_set_selectable (CTK_LABEL (example), TRUE);
      ctk_widget_set_halign (example, CTK_ALIGN_START);

      box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 10);
      ctk_widget_show (box);
      ctk_box_set_homogeneous (CTK_BOX (box), TRUE);
      ctk_container_add (CTK_CONTAINER (box), feat);
      ctk_container_add (CTK_CONTAINER (box), example);
      ctk_container_add (CTK_CONTAINER (group), box);

      item = g_new (FeatureItem, 1);
      item->name = tags[i];
      item->tag = tag;
      item->top = box;
      item->feat = feat;
      item->example = example;

      priv->feature_items = g_list_prepend (priv->feature_items, item);
    }

  ctk_container_add (CTK_CONTAINER (priv->feature_box), group);
}

static void
ctk_font_chooser_widget_populate_features (CtkFontChooserWidget *fontchooser)
{
  const char *ligatures[] = { "liga", "dlig", "hlig", "clig", NULL };
  const char *letter_case[] = { "smcp", "c2sc", "pcap", "c2pc", "unic", "cpsp", "case", NULL };
  const char *number_case[] = { "xxxx", "lnum", "onum", NULL };
  const char *number_spacing[] = { "xxxx", "pnum", "tnum", NULL };
  const char *number_formatting[] = { "zero", "nalt", NULL };
  const char *char_variants[] = {
    "swsh", "cswh", "calt", "falt", "hist", "salt", "jalt", "titl", "rand",
    "ss01", "ss02", "ss03", "ss04", "ss05", "ss06", "ss07", "ss08", "ss09", "ss10",
    "ss11", "ss12", "ss13", "ss14", "ss15", "ss16", "ss17", "ss18", "ss19", "ss20",
    NULL };

  add_check_group (fontchooser, _("Ligatures"), ligatures);
  add_check_group (fontchooser, _("Letter Case"), letter_case);
  add_radio_group (fontchooser, _("Number Case"), number_case);
  add_radio_group (fontchooser, _("Number Spacing"), number_spacing);
  add_check_group (fontchooser, _("Number Formatting"), number_formatting);
  add_check_group (fontchooser, _("Character Variants"), char_variants);

  update_font_features (fontchooser);
}

static gboolean
ctk_font_chooser_widget_update_font_features (CtkFontChooserWidget *fontchooser)
{
  CtkFontChooserWidgetPrivate *priv = fontchooser->priv;
  PangoFont *pango_font;
  hb_font_t *hb_font;
  hb_tag_t script_tag;
  hb_tag_t lang_tag;
  guint script_index = 0;
  guint lang_index = 0;
  int i, j;
  GList *l;
  gboolean has_feature = FALSE;

  for (l = priv->feature_items; l; l = l->next)
    {
      FeatureItem *item = l->data;
      ctk_widget_hide (item->top);
      ctk_widget_hide (ctk_widget_get_parent (item->top));
    }

  if ((priv->level & CTK_FONT_CHOOSER_LEVEL_FEATURES) == 0)
    return FALSE;

  pango_font = pango_context_load_font (ctk_widget_get_pango_context (CTK_WIDGET (fontchooser)),
                                        priv->font_desc);
  hb_font = pango_font_get_hb_font (pango_font);

  if (hb_font)
    {
      hb_tag_t table[2] = { HB_OT_TAG_GSUB, HB_OT_TAG_GPOS };
      hb_face_t *hb_face;
      hb_tag_t features[80];
      unsigned int count;
      unsigned int n_features;

      hb_face = hb_font_get_face (hb_font);

      find_language_and_script (fontchooser, hb_face, &lang_tag, &script_tag);

      n_features = 0;
      for (i = 0; i < 2; i++)
        {
          hb_ot_layout_table_find_script (hb_face, table[i], script_tag, &script_index);
          hb_ot_layout_script_find_language (hb_face, table[i], script_index, lang_tag, &lang_index);
          count = G_N_ELEMENTS (features);
          hb_ot_layout_language_get_feature_tags (hb_face,
                                                  table[i],
                                                  script_index,
                                                  lang_index,
                                                  n_features,
                                                  &count,
                                                  features);
          n_features += count;
        }

      for (j = 0; j < n_features; j++)
        {
          for (l = priv->feature_items; l; l = l->next)
            {
              FeatureItem *item = l->data;
              if (item->tag != features[j])
                continue;

              has_feature = TRUE;
              ctk_widget_show (item->top);
              ctk_widget_show (ctk_widget_get_parent (item->top));

              update_feature_example (item, hb_font, script_tag, lang_tag, priv->font_desc);

              if (CTK_IS_RADIO_BUTTON (item->feat))
                {
                  CtkWidget *def = CTK_WIDGET (g_object_get_data (G_OBJECT (item->feat), "default"));
                  ctk_widget_show (ctk_widget_get_parent (def));
                }
              else if (CTK_IS_CHECK_BUTTON (item->feat))
                {
                  set_inconsistent (CTK_CHECK_BUTTON (item->feat), TRUE);
                }
            }
        }
    }

  g_object_unref (pango_font);

  return has_feature;
}

static void
update_font_features (CtkFontChooserWidget *fontchooser)
{
  CtkFontChooserWidgetPrivate *priv = fontchooser->priv;
  GString *s;
  GList *l;

  s = g_string_new ("");

  for (l = priv->feature_items; l; l = l->next)
    {
      FeatureItem *item = l->data;

      if (!ctk_widget_is_sensitive (item->feat))
        continue;

      if (CTK_IS_RADIO_BUTTON (item->feat))
        {
          if (ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (item->feat)) &&
              strcmp (item->name, "xxxx") != 0)
            {
              g_string_append_printf (s, "%s\"%s\" %d", s->len > 0 ? ", " : "", item->name, 1);
            }
        }
      else if (CTK_IS_CHECK_BUTTON (item->feat))
        {
          if (ctk_widget_get_state_flags (item->feat) & CTK_STATE_FLAG_INCONSISTENT)
            continue;

          g_string_append_printf (s, "%s\"%s\" %d",
                                  s->len > 0 ? ", " : "", item->name,
                                  ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (item->feat)));
        }
    }

  if (g_strcmp0 (priv->font_features, s->str) != 0)
    {
      g_free (priv->font_features);
      priv->font_features = g_string_free (s, FALSE);
      g_object_notify (G_OBJECT (fontchooser), "font-features");
    }
  else
    g_string_free (s, TRUE);

  ctk_font_chooser_widget_update_preview_attributes (fontchooser);
}

static void
ctk_font_chooser_widget_merge_font_desc (CtkFontChooserWidget       *fontchooser,
                                         const PangoFontDescription *font_desc,
                                         CtkTreeIter                *iter)
{
  CtkFontChooserWidgetPrivate *priv = fontchooser->priv;
  PangoFontMask mask;

  g_assert (font_desc != NULL);
  /* iter may be NULL if the font doesn't exist on the list */

  mask = pango_font_description_get_set_fields (font_desc);

  /* sucky test, because we can't restrict the comparison to 
   * only the parts that actually do get merged */
  if (pango_font_description_equal (font_desc, priv->font_desc))
    return;

  pango_font_description_merge (priv->font_desc, font_desc, TRUE);
  
  if (mask & PANGO_FONT_MASK_SIZE)
    {
      double font_size = (double) pango_font_description_get_size (priv->font_desc) / PANGO_SCALE;
      /* XXX: This clamps, which can cause it to reloop into here, do we need
       * to block its signal handler? */
      ctk_range_set_value (CTK_RANGE (priv->size_slider), font_size);
      ctk_spin_button_set_value (CTK_SPIN_BUTTON (priv->size_spin), font_size);
    }
  if (mask & (PANGO_FONT_MASK_FAMILY | PANGO_FONT_MASK_STYLE | PANGO_FONT_MASK_VARIANT |
              PANGO_FONT_MASK_WEIGHT | PANGO_FONT_MASK_STRETCH))
    {
      gboolean has_tweak = FALSE;

      if (&priv->font_iter != iter)
        {
          if (iter == NULL)
            memset (&priv->font_iter, 0, sizeof (CtkTreeIter));
          else
            memcpy (&priv->font_iter, iter, sizeof (CtkTreeIter));
          
          ctk_font_chooser_widget_ensure_selection (fontchooser);
        }

      ctk_font_chooser_widget_update_marks (fontchooser);

      if (ctk_font_chooser_widget_update_font_features (fontchooser))
        has_tweak = TRUE;
      if (ctk_font_chooser_widget_update_font_variations (fontchooser))
        has_tweak = TRUE;

      g_simple_action_set_enabled (G_SIMPLE_ACTION (priv->tweak_action), has_tweak);
    }

  ctk_font_chooser_widget_update_preview_attributes (fontchooser);

  g_object_notify (G_OBJECT (fontchooser), "font");
  g_object_notify (G_OBJECT (fontchooser), "font-desc");
}

static void
ctk_font_chooser_widget_take_font_desc (CtkFontChooserWidget *fontchooser,
                                        PangoFontDescription *font_desc)
{
  CtkFontChooserWidgetPrivate *priv = fontchooser->priv;
  PangoFontMask mask;

  if (font_desc == NULL)
    font_desc = pango_font_description_from_string (CTK_FONT_CHOOSER_DEFAULT_FONT_NAME);

  mask = pango_font_description_get_set_fields (font_desc);
  if (mask & (PANGO_FONT_MASK_FAMILY | PANGO_FONT_MASK_STYLE | PANGO_FONT_MASK_VARIANT |
              PANGO_FONT_MASK_WEIGHT | PANGO_FONT_MASK_STRETCH))
    {
      CtkTreeIter iter;

      if (ctk_font_chooser_widget_find_font (fontchooser, font_desc, &iter))
        ctk_font_chooser_widget_merge_font_desc (fontchooser, font_desc, &iter);
      else
        ctk_font_chooser_widget_merge_font_desc (fontchooser, font_desc, NULL);
    }
  else
    {
      ctk_font_chooser_widget_merge_font_desc (fontchooser, font_desc, &priv->font_iter);
    }

  pango_font_description_free (font_desc);
}

static const gchar*
ctk_font_chooser_widget_get_preview_text (CtkFontChooserWidget *fontchooser)
{
  return fontchooser->priv->preview_text;
}

static void
ctk_font_chooser_widget_set_preview_text (CtkFontChooserWidget *fontchooser,
                                          const gchar          *text)
{
  CtkFontChooserWidgetPrivate *priv = fontchooser->priv;

  g_free (priv->preview_text);
  priv->preview_text = g_strdup (text);

  ctk_entry_set_text (CTK_ENTRY (priv->preview), text);

  g_object_notify (G_OBJECT (fontchooser), "preview-text");

  /* XXX: There's no API to tell the treeview that a column has changed,
   * so we just */
  ctk_widget_queue_draw (priv->family_face_list);
}

static gboolean
ctk_font_chooser_widget_get_show_preview_entry (CtkFontChooserWidget *fontchooser)
{
  return fontchooser->priv->show_preview_entry;
}

static void
ctk_font_chooser_widget_set_show_preview_entry (CtkFontChooserWidget *fontchooser,
                                                gboolean              show_preview_entry)
{
  CtkFontChooserWidgetPrivate *priv = fontchooser->priv;

  if (priv->show_preview_entry != show_preview_entry)
    {
      fontchooser->priv->show_preview_entry = show_preview_entry;

      if (show_preview_entry)
        ctk_widget_show (fontchooser->priv->preview);
      else
        ctk_widget_hide (fontchooser->priv->preview);

      g_object_notify (G_OBJECT (fontchooser), "show-preview-entry");
    }
}

static void
ctk_font_chooser_widget_set_font_map (CtkFontChooser *chooser,
                                      PangoFontMap   *fontmap)
{
  CtkFontChooserWidget *fontchooser = CTK_FONT_CHOOSER_WIDGET (chooser);
  CtkFontChooserWidgetPrivate *priv = fontchooser->priv;

  if (g_set_object (&priv->font_map, fontmap))
    {
      PangoContext *context;

      if (!fontmap)
        fontmap = pango_cairo_font_map_get_default ();

      context = ctk_widget_get_pango_context (priv->family_face_list);
      pango_context_set_font_map (context, fontmap);

      context = ctk_widget_get_pango_context (priv->preview);
      pango_context_set_font_map (context, fontmap);

      ctk_font_chooser_widget_load_fonts (fontchooser, TRUE);
    }
}

static PangoFontMap *
ctk_font_chooser_widget_get_font_map (CtkFontChooser *chooser)
{
  CtkFontChooserWidget *fontchooser = CTK_FONT_CHOOSER_WIDGET (chooser);
  CtkFontChooserWidgetPrivate *priv = fontchooser->priv;

  return priv->font_map;
}

static void
ctk_font_chooser_widget_set_filter_func (CtkFontChooser  *chooser,
                                         CtkFontFilterFunc filter,
                                         gpointer          data,
                                         GDestroyNotify    destroy)
{
  CtkFontChooserWidget *fontchooser = CTK_FONT_CHOOSER_WIDGET (chooser);
  CtkFontChooserWidgetPrivate *priv = fontchooser->priv;

  if (priv->filter_data_destroy)
    priv->filter_data_destroy (priv->filter_data);

  priv->filter_func = filter;
  priv->filter_data = data;
  priv->filter_data_destroy = destroy;

  ctk_font_chooser_widget_refilter_font_list (fontchooser);
}

static void
ctk_font_chooser_widget_set_level (CtkFontChooserWidget *fontchooser,
                                   CtkFontChooserLevel   level)
{
  CtkFontChooserWidgetPrivate *priv = fontchooser->priv;

  if (priv->level == level)
    return;

  priv->level = level;

  if ((level & CTK_FONT_CHOOSER_LEVEL_SIZE) != 0)
    {
      ctk_widget_show (priv->size_slider);
      ctk_widget_show (priv->size_spin);
      ctk_widget_show (priv->size_label);
    }
  else
   {
      ctk_widget_hide (priv->size_slider);
      ctk_widget_hide (priv->size_spin);
      ctk_widget_hide (priv->size_label);
   }

  ctk_font_chooser_widget_load_fonts (fontchooser, TRUE);

  g_object_notify (G_OBJECT (fontchooser), "level");
}

static CtkFontChooserLevel
ctk_font_chooser_widget_get_level (CtkFontChooserWidget *fontchooser)
{
  CtkFontChooserWidgetPrivate *priv = fontchooser->priv;

  return priv->level;
}

static void
ctk_font_chooser_widget_set_language (CtkFontChooserWidget *fontchooser,
                                      const char           *language)
{
  CtkFontChooserWidgetPrivate *priv = fontchooser->priv;
  PangoLanguage *lang;

  lang = pango_language_from_string (language);
  if (priv->language == lang)
    return;

  priv->language = lang;
  g_object_notify (G_OBJECT (fontchooser), "language");

  ctk_font_chooser_widget_update_preview_attributes (fontchooser);
}

static void
ctk_font_chooser_widget_iface_init (CtkFontChooserIface *iface)
{
  iface->get_font_family = ctk_font_chooser_widget_get_family;
  iface->get_font_face = ctk_font_chooser_widget_get_face;
  iface->get_font_size = ctk_font_chooser_widget_get_size;
  iface->set_filter_func = ctk_font_chooser_widget_set_filter_func;
  iface->set_font_map = ctk_font_chooser_widget_set_font_map;
  iface->get_font_map = ctk_font_chooser_widget_get_font_map;
}

gboolean
ctk_font_chooser_widget_handle_event (CtkWidget   *widget,
                                      CdkEventKey *key_event)
{
  CtkFontChooserWidget *fontchooser = CTK_FONT_CHOOSER_WIDGET (widget);
  CtkFontChooserWidgetPrivate *priv = fontchooser->priv;
  CdkEvent *event = (CdkEvent *)key_event;

  return ctk_search_entry_handle_event (CTK_SEARCH_ENTRY (priv->search_entry), event);
}

GAction *
ctk_font_chooser_widget_get_tweak_action (CtkWidget *widget)
{
  return CTK_FONT_CHOOSER_WIDGET (widget)->priv->tweak_action;
}
