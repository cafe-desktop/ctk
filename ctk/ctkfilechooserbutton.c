/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */

/* CTK+: ctkfilechooserbutton.c
 *
 * Copyright (c) 2004 James M. Cape <jcape@ignore-your.tv>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <string.h>

#include "ctkintl.h"
#include "ctkbutton.h"
#include "ctkcelllayout.h"
#include "ctkcellrenderertext.h"
#include "ctkcellrendererpixbuf.h"
#include "ctkcombobox.h"
#include "ctkcssiconthemevalueprivate.h"
#include "ctkdnd.h"
#include "ctkdragdest.h"
#include "ctkicontheme.h"
#include "deprecated/ctkiconfactory.h"
#include "ctkimage.h"
#include "ctklabel.h"
#include "ctkliststore.h"
#include "deprecated/ctkstock.h"
#include "ctktreemodelfilter.h"
#include "ctkseparator.h"
#include "ctkfilechooserdialog.h"
#include "ctkfilechoosernative.h"
#include "ctkfilechooserprivate.h"
#include "ctkfilechooserutils.h"
#include "ctkmarshalers.h"

#include "ctkfilechooserbutton.h"

#include "ctkorientable.h"

#include "ctktypebuiltins.h"
#include "ctkprivate.h"
#include "ctksettings.h"
#include "ctkstylecontextprivate.h"
#include "ctkbitmaskprivate.h"

/**
 * SECTION:ctkfilechooserbutton
 * @Short_description: A button to launch a file selection dialog
 * @Title: CtkFileChooserButton
 * @See_also:#CtkFileChooserDialog
 *
 * The #CtkFileChooserButton is a widget that lets the user select a
 * file.  It implements the #CtkFileChooser interface.  Visually, it is a
 * file name with a button to bring up a #CtkFileChooserDialog.
 * The user can then use that dialog to change the file associated with
 * that button.  This widget does not support setting the
 * #CtkFileChooser:select-multiple property to %TRUE.
 *
 * ## Create a button to let the user select a file in /etc
 *
 * |[<!-- language="C" -->
 * {
 *   CtkWidget *button;
 *
 *   button = ctk_file_chooser_button_new (_("Select a file"),
 *                                         CTK_FILE_CHOOSER_ACTION_OPEN);
 *   ctk_file_chooser_set_current_folder (CTK_FILE_CHOOSER (button),
 *                                        "/etc");
 * }
 * ]|
 *
 * The #CtkFileChooserButton supports the #CtkFileChooserActions
 * %CTK_FILE_CHOOSER_ACTION_OPEN and %CTK_FILE_CHOOSER_ACTION_SELECT_FOLDER.
 *
 * > The #CtkFileChooserButton will ellipsize the label, and will thus
 * > request little horizontal space.  To give the button more space,
 * > you should call ctk_widget_get_preferred_size(),
 * > ctk_file_chooser_button_set_width_chars(), or pack the button in
 * > such a way that other interface elements give space to the
 * > widget.
 *
 * # CSS nodes
 *
 * CtkFileChooserButton has a CSS node with name “filechooserbutton”, containing
 * a subnode for the internal button with name “button” and style class “.file”.
 */


/* **************** *
 *  Private Macros  *
 * **************** */

#define FALLBACK_ICON_SIZE	16
#define DEFAULT_TITLE		N_("Select a File")
#define DESKTOP_DISPLAY_NAME	N_("Desktop")
#define FALLBACK_DISPLAY_NAME	N_("(None)") /* this string is used in ctk+/ctk/tests/filechooser.c - change it there if you change it here */


/* ********************** *
 *  Private Enumerations  *
 * ********************** */

/* Property IDs */
enum
{
  PROP_0,

  PROP_DIALOG,
  PROP_TITLE,
  PROP_WIDTH_CHARS
};

/* Signals */
enum
{
  FILE_SET,
  LAST_SIGNAL
};

/* TreeModel Columns
 *
 * keep in line with the store defined in ctkfilechooserbutton.ui
 */
enum
{
  ICON_COLUMN,
  DISPLAY_NAME_COLUMN,
  TYPE_COLUMN,
  DATA_COLUMN,
  IS_FOLDER_COLUMN,
  CANCELLABLE_COLUMN,
  NUM_COLUMNS
};

/* TreeModel Row Types */
typedef enum
{
  ROW_TYPE_SPECIAL,
  ROW_TYPE_VOLUME,
  ROW_TYPE_SHORTCUT,
  ROW_TYPE_BOOKMARK_SEPARATOR,
  ROW_TYPE_BOOKMARK,
  ROW_TYPE_CURRENT_FOLDER_SEPARATOR,
  ROW_TYPE_CURRENT_FOLDER,
  ROW_TYPE_OTHER_SEPARATOR,
  ROW_TYPE_OTHER,
  ROW_TYPE_EMPTY_SELECTION,

  ROW_TYPE_INVALID = -1
}
RowType;


/* ******************** *
 *  Private Structures  *
 * ******************** */

struct _CtkFileChooserButtonPrivate
{
  CtkFileChooser *chooser;      /* Points to either dialog or native, depending on which is set */
  CtkWidget *dialog;            /* Set if you explicitly enable */
  CtkFileChooserNative *native; /* Otherwise this is set */
  CtkWidget *button;
  CtkWidget *image;
  CtkWidget *label;
  CtkWidget *combo_box;
  CtkCellRenderer *icon_cell;
  CtkCellRenderer *name_cell;

  CtkTreeModel *model;
  CtkTreeModel *filter_model;

  CtkFileSystem *fs;
  GFile *selection_while_inactive;
  GFile *current_folder_while_inactive;

  gulong fs_volumes_changed_id;

  GCancellable *dnd_select_folder_cancellable;
  GCancellable *update_button_cancellable;
  GSList *change_icon_theme_cancellables;

  CtkBookmarksManager *bookmarks_manager;

  gint icon_size;

  guint8 n_special;
  guint8 n_volumes;
  guint8 n_shortcuts;
  guint8 n_bookmarks;
  guint  has_bookmark_separator       : 1;
  guint  has_current_folder_separator : 1;
  guint  has_current_folder           : 1;
  guint  has_other_separator          : 1;

  /* Used for hiding/showing the dialog when the button is hidden */
  guint  active                       : 1;

  /* Whether the next async callback from GIO should emit the "selection-changed" signal */
  guint  is_changing_selection        : 1;
};


/* ************* *
 *  DnD Support  *
 * ************* */

enum
{
  TEXT_PLAIN,
  TEXT_URI_LIST
};


/* ********************* *
 *  Function Prototypes  *
 * ********************* */

/* CtkFileChooserIface Functions */
static void     ctk_file_chooser_button_file_chooser_iface_init (CtkFileChooserIface *iface);
static gboolean ctk_file_chooser_button_set_current_folder (CtkFileChooser    *chooser,
							    GFile             *file,
							    GError           **error);
static GFile *ctk_file_chooser_button_get_current_folder (CtkFileChooser    *chooser);
static gboolean ctk_file_chooser_button_select_file (CtkFileChooser *chooser,
						     GFile          *file,
						     GError        **error);
static void ctk_file_chooser_button_unselect_file (CtkFileChooser *chooser,
						   GFile          *file);
static void ctk_file_chooser_button_unselect_all (CtkFileChooser *chooser);
static GSList *ctk_file_chooser_button_get_files (CtkFileChooser *chooser);
static gboolean ctk_file_chooser_button_add_shortcut_folder     (CtkFileChooser      *chooser,
								 GFile               *file,
								 GError             **error);
static gboolean ctk_file_chooser_button_remove_shortcut_folder  (CtkFileChooser      *chooser,
								 GFile               *file,
								 GError             **error);

/* GObject Functions */
static void     ctk_file_chooser_button_constructed        (GObject          *object);
static void     ctk_file_chooser_button_set_property       (GObject          *object,
							    guint             param_id,
							    const GValue     *value,
							    GParamSpec       *pspec);
static void     ctk_file_chooser_button_get_property       (GObject          *object,
							    guint             param_id,
							    GValue           *value,
							    GParamSpec       *pspec);
static void     ctk_file_chooser_button_finalize           (GObject          *object);

/* CtkWidget Functions */
static void     ctk_file_chooser_button_destroy            (CtkWidget        *widget);
static void     ctk_file_chooser_button_drag_data_received (CtkWidget        *widget,
							    GdkDragContext   *context,
							    gint              x,
							    gint              y,
							    CtkSelectionData *data,
							    guint             type,
							    guint             drag_time);
static void     ctk_file_chooser_button_show_all           (CtkWidget        *widget);
static void     ctk_file_chooser_button_show               (CtkWidget        *widget);
static void     ctk_file_chooser_button_hide               (CtkWidget        *widget);
static void     ctk_file_chooser_button_map                (CtkWidget        *widget);
static gboolean ctk_file_chooser_button_mnemonic_activate  (CtkWidget        *widget,
							    gboolean          group_cycling);
static void     ctk_file_chooser_button_style_updated      (CtkWidget        *widget);
static void     ctk_file_chooser_button_screen_changed     (CtkWidget        *widget,
							    GdkScreen        *old_screen);
static void     ctk_file_chooser_button_state_flags_changed (CtkWidget       *widget,
                                                             CtkStateFlags    previous_state);

/* Utility Functions */
static CtkIconTheme *get_icon_theme               (CtkWidget            *widget);
static void          set_info_for_file_at_iter         (CtkFileChooserButton *fs,
							GFile                *file,
							CtkTreeIter          *iter);

static gint          model_get_type_position      (CtkFileChooserButton *button,
						   RowType               row_type);
static void          model_free_row_data          (CtkFileChooserButton *button,
						   CtkTreeIter          *iter);
static void          model_add_special            (CtkFileChooserButton *button);
static void          model_add_other              (CtkFileChooserButton *button);
static void          model_add_empty_selection    (CtkFileChooserButton *button);
static void          model_add_volumes            (CtkFileChooserButton *button,
						   GSList               *volumes);
static void          model_add_bookmarks          (CtkFileChooserButton *button,
						   GSList               *bookmarks);
static void          model_update_current_folder  (CtkFileChooserButton *button,
						   GFile                *file);
static void          model_remove_rows            (CtkFileChooserButton *button,
						   gint                  pos,
						   gint                  n_rows);

static gboolean      filter_model_visible_func    (CtkTreeModel         *model,
						   CtkTreeIter          *iter,
						   gpointer              user_data);

static gboolean      combo_box_row_separator_func (CtkTreeModel         *model,
						   CtkTreeIter          *iter,
						   gpointer              user_data);
static void          name_cell_data_func          (CtkCellLayout        *layout,
						   CtkCellRenderer      *cell,
						   CtkTreeModel         *model,
						   CtkTreeIter          *iter,
						   gpointer              user_data);
static void          open_dialog                  (CtkFileChooserButton *button);
static void          update_combo_box             (CtkFileChooserButton *button);
static void          update_label_and_image       (CtkFileChooserButton *button);

/* Child Object Callbacks */
static void     fs_volumes_changed_cb            (CtkFileSystem  *fs,
						  gpointer        user_data);
static void     bookmarks_changed_cb             (gpointer        user_data);

static void     combo_box_changed_cb             (CtkComboBox    *combo_box,
						  gpointer        user_data);
static void     combo_box_notify_popup_shown_cb  (GObject        *object,
						  GParamSpec     *pspec,
						  gpointer        user_data);

static void     button_clicked_cb                (CtkButton      *real_button,
						  gpointer        user_data);

static void     chooser_update_preview_cb        (CtkFileChooser *dialog,
						  gpointer        user_data);
static void     chooser_notify_cb                (GObject        *dialog,
						  GParamSpec     *pspec,
						  gpointer        user_data);
static gboolean dialog_delete_event_cb           (CtkWidget      *dialog,
						  GdkEvent       *event,
						  gpointer        user_data);
static void     dialog_response_cb               (CtkDialog      *dialog,
						  gint            response,
						  gpointer        user_data);
static void     native_response_cb               (CtkFileChooserNative *native,
                                                  gint            response,
                                                  gpointer        user_data);

static guint file_chooser_button_signals[LAST_SIGNAL] = { 0 };

/* ******************* *
 *  GType Declaration  *
 * ******************* */

G_DEFINE_TYPE_WITH_CODE (CtkFileChooserButton, ctk_file_chooser_button, CTK_TYPE_BOX,
                         G_ADD_PRIVATE (CtkFileChooserButton)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_FILE_CHOOSER,
                                                ctk_file_chooser_button_file_chooser_iface_init))


/* ***************** *
 *  GType Functions  *
 * ***************** */

static void
ctk_file_chooser_button_class_init (CtkFileChooserButtonClass * class)
{
  GObjectClass *gobject_class;
  CtkWidgetClass *widget_class;

  gobject_class = G_OBJECT_CLASS (class);
  widget_class = CTK_WIDGET_CLASS (class);

  gobject_class->constructed = ctk_file_chooser_button_constructed;
  gobject_class->set_property = ctk_file_chooser_button_set_property;
  gobject_class->get_property = ctk_file_chooser_button_get_property;
  gobject_class->finalize = ctk_file_chooser_button_finalize;

  widget_class->destroy = ctk_file_chooser_button_destroy;
  widget_class->drag_data_received = ctk_file_chooser_button_drag_data_received;
  widget_class->show_all = ctk_file_chooser_button_show_all;
  widget_class->show = ctk_file_chooser_button_show;
  widget_class->hide = ctk_file_chooser_button_hide;
  widget_class->map = ctk_file_chooser_button_map;
  widget_class->style_updated = ctk_file_chooser_button_style_updated;
  widget_class->screen_changed = ctk_file_chooser_button_screen_changed;
  widget_class->mnemonic_activate = ctk_file_chooser_button_mnemonic_activate;
  widget_class->state_flags_changed = ctk_file_chooser_button_state_flags_changed;

  /**
   * CtkFileChooserButton::file-set:
   * @widget: the object which received the signal.
   *
   * The ::file-set signal is emitted when the user selects a file.
   *
   * Note that this signal is only emitted when the user
   * changes the file.
   *
   * Since: 2.12
   */
  file_chooser_button_signals[FILE_SET] =
    g_signal_new (I_("file-set"),
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (CtkFileChooserButtonClass, file_set),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);

  /**
   * CtkFileChooserButton:dialog:
   *
   * Instance of the #CtkFileChooserDialog associated with the button.
   *
   * Since: 2.6
   */
  g_object_class_install_property (gobject_class, PROP_DIALOG,
				   g_param_spec_object ("dialog",
							P_("Dialog"),
							P_("The file chooser dialog to use."),
							CTK_TYPE_FILE_CHOOSER,
							(CTK_PARAM_WRITABLE |
							 G_PARAM_CONSTRUCT_ONLY)));

  /**
   * CtkFileChooserButton:title:
   *
   * Title to put on the #CtkFileChooserDialog associated with the button.
   *
   * Since: 2.6
   */
  g_object_class_install_property (gobject_class, PROP_TITLE,
				   g_param_spec_string ("title",
							P_("Title"),
							P_("The title of the file chooser dialog."),
							_(DEFAULT_TITLE),
							CTK_PARAM_READWRITE));

  /**
   * CtkFileChooserButton:width-chars:
   *
   * The width of the entry and label inside the button, in characters.
   *
   * Since: 2.6
   */
  g_object_class_install_property (gobject_class, PROP_WIDTH_CHARS,
				   g_param_spec_int ("width-chars",
						     P_("Width In Characters"),
						     P_("The desired width of the button widget, in characters."),
						     -1, G_MAXINT, -1,
						     CTK_PARAM_READWRITE));

  _ctk_file_chooser_install_properties (gobject_class);

  /* Bind class to template
   */
  ctk_widget_class_set_template_from_resource (widget_class,
					       "/org/ctk/libctk/ui/ctkfilechooserbutton.ui");

  ctk_widget_class_bind_template_child_private (widget_class, CtkFileChooserButton, model);
  ctk_widget_class_bind_template_child_private (widget_class, CtkFileChooserButton, button);
  ctk_widget_class_bind_template_child_private (widget_class, CtkFileChooserButton, image);
  ctk_widget_class_bind_template_child_private (widget_class, CtkFileChooserButton, label);
  ctk_widget_class_bind_template_child_private (widget_class, CtkFileChooserButton, combo_box);
  ctk_widget_class_bind_template_child_private (widget_class, CtkFileChooserButton, icon_cell);
  ctk_widget_class_bind_template_child_private (widget_class, CtkFileChooserButton, name_cell);

  ctk_widget_class_bind_template_callback (widget_class, button_clicked_cb);
  ctk_widget_class_bind_template_callback (widget_class, combo_box_changed_cb);
  ctk_widget_class_bind_template_callback (widget_class, combo_box_notify_popup_shown_cb);

  ctk_widget_class_set_css_name (widget_class, "filechooserbutton");
}

static void
ctk_file_chooser_button_init (CtkFileChooserButton *button)
{
  CtkFileChooserButtonPrivate *priv;
  CtkTargetList *target_list;

  priv = button->priv = ctk_file_chooser_button_get_instance_private (button);

  priv->icon_size = FALLBACK_ICON_SIZE;

  ctk_widget_init_template (CTK_WIDGET (button));

  /* Bookmarks manager */
  priv->bookmarks_manager = _ctk_bookmarks_manager_new (bookmarks_changed_cb, button);
  ctk_cell_layout_set_cell_data_func (CTK_CELL_LAYOUT (priv->combo_box),
				      priv->name_cell, name_cell_data_func,
				      NULL, NULL);

  /* DnD */
  ctk_drag_dest_set (CTK_WIDGET (button),
                     (CTK_DEST_DEFAULT_ALL),
		     NULL, 0,
		     GDK_ACTION_COPY);
  target_list = ctk_target_list_new (NULL, 0);
  ctk_target_list_add_uri_targets (target_list, TEXT_URI_LIST);
  ctk_target_list_add_text_targets (target_list, TEXT_PLAIN);
  ctk_drag_dest_set_target_list (CTK_WIDGET (button), target_list);
  ctk_target_list_unref (target_list);
}


/* ******************************* *
 *  CtkFileChooserIface Functions  *
 * ******************************* */
static void
ctk_file_chooser_button_file_chooser_iface_init (CtkFileChooserIface *iface)
{
  _ctk_file_chooser_delegate_iface_init (iface);

  iface->set_current_folder = ctk_file_chooser_button_set_current_folder;
  iface->get_current_folder = ctk_file_chooser_button_get_current_folder;
  iface->select_file = ctk_file_chooser_button_select_file;
  iface->unselect_file = ctk_file_chooser_button_unselect_file;
  iface->unselect_all = ctk_file_chooser_button_unselect_all;
  iface->get_files = ctk_file_chooser_button_get_files;
  iface->add_shortcut_folder = ctk_file_chooser_button_add_shortcut_folder;
  iface->remove_shortcut_folder = ctk_file_chooser_button_remove_shortcut_folder;
}

static void
emit_selection_changed_if_changing_selection (CtkFileChooserButton *button)
{
  CtkFileChooserButtonPrivate *priv = button->priv;

  if (priv->is_changing_selection)
    {
      priv->is_changing_selection = FALSE;
      g_signal_emit_by_name (button, "selection-changed");
    }
}

static gboolean
ctk_file_chooser_button_set_current_folder (CtkFileChooser    *chooser,
					    GFile             *file,
					    GError           **error)
{
  CtkFileChooserButton *button = CTK_FILE_CHOOSER_BUTTON (chooser);
  CtkFileChooserButtonPrivate *priv = button->priv;

  if (priv->current_folder_while_inactive)
    g_object_unref (priv->current_folder_while_inactive);

  priv->current_folder_while_inactive = g_object_ref (file);

  update_combo_box (button);

  g_signal_emit_by_name (button, "current-folder-changed");

  if (priv->active)
    ctk_file_chooser_set_current_folder_file (CTK_FILE_CHOOSER (priv->chooser), file, NULL);

  return TRUE;
}

static GFile *
ctk_file_chooser_button_get_current_folder (CtkFileChooser *chooser)
{
  CtkFileChooserButton *button = CTK_FILE_CHOOSER_BUTTON (chooser);
  CtkFileChooserButtonPrivate *priv = button->priv;

  if (priv->current_folder_while_inactive)
    return g_object_ref (priv->current_folder_while_inactive);
  else
    return NULL;
}

static gboolean
ctk_file_chooser_button_select_file (CtkFileChooser *chooser,
				     GFile          *file,
				     GError        **error)
{
  CtkFileChooserButton *button = CTK_FILE_CHOOSER_BUTTON (chooser);
  CtkFileChooserButtonPrivate *priv = button->priv;

  if (priv->selection_while_inactive)
    g_object_unref (priv->selection_while_inactive);

  priv->selection_while_inactive = g_object_ref (file);

  priv->is_changing_selection = TRUE;

  update_label_and_image (button);
  update_combo_box (button);

  if (priv->active)
    ctk_file_chooser_select_file (CTK_FILE_CHOOSER (priv->chooser), file, NULL);

  return TRUE;
}

static void
unselect_current_file (CtkFileChooserButton *button)
{
  CtkFileChooserButtonPrivate *priv = button->priv;

  if (priv->selection_while_inactive)
    {
      g_object_unref (priv->selection_while_inactive);
      priv->selection_while_inactive = NULL;
      priv->is_changing_selection = TRUE;
    }

  update_label_and_image (button);
  update_combo_box (button);
}

static void
ctk_file_chooser_button_unselect_file (CtkFileChooser *chooser,
				       GFile          *file)
{
  CtkFileChooserButton *button = CTK_FILE_CHOOSER_BUTTON (chooser);
  CtkFileChooserButtonPrivate *priv = button->priv;

  if (g_file_equal (priv->selection_while_inactive, file))
    unselect_current_file (button);

  if (priv->active)
    ctk_file_chooser_unselect_file (CTK_FILE_CHOOSER (priv->chooser), file);
}

static void
ctk_file_chooser_button_unselect_all (CtkFileChooser *chooser)
{
  CtkFileChooserButton *button = CTK_FILE_CHOOSER_BUTTON (chooser);
  CtkFileChooserButtonPrivate *priv = button->priv;

  unselect_current_file (button);

  if (priv->active)
    ctk_file_chooser_unselect_all (CTK_FILE_CHOOSER (priv->chooser));
}

static GFile *
get_selected_file (CtkFileChooserButton *button)
{
  CtkFileChooserButtonPrivate *priv = button->priv;
  GFile *retval;

  retval = NULL;

  if (priv->selection_while_inactive)
    retval = priv->selection_while_inactive;
  else if (priv->chooser && ctk_file_chooser_get_action (priv->chooser) == CTK_FILE_CHOOSER_ACTION_SELECT_FOLDER)
    {
      /* If there is no "real" selection in SELECT_FOLDER mode, then we'll just return
       * the current folder, since that is what CtkFileChooserWidget would do.
       */
      if (priv->current_folder_while_inactive)
	retval = priv->current_folder_while_inactive;
    }

  if (retval)
    return g_object_ref (retval);
  else
    return NULL;
}

static GSList *
ctk_file_chooser_button_get_files (CtkFileChooser *chooser)
{
  CtkFileChooserButton *button = CTK_FILE_CHOOSER_BUTTON (chooser);
  GFile *file;

  file = get_selected_file (button);
  if (file)
    return g_slist_prepend (NULL, file);
  else
    return NULL;
}

static gboolean
ctk_file_chooser_button_add_shortcut_folder (CtkFileChooser  *chooser,
					     GFile           *file,
					     GError         **error)
{
  CtkFileChooser *delegate;
  gboolean retval;

  delegate = g_object_get_qdata (G_OBJECT (chooser),
				 CTK_FILE_CHOOSER_DELEGATE_QUARK);
  retval = _ctk_file_chooser_add_shortcut_folder (delegate, file, error);

  if (retval)
    {
      CtkFileChooserButton *button = CTK_FILE_CHOOSER_BUTTON (chooser);
      CtkFileChooserButtonPrivate *priv = button->priv;
      CtkTreeIter iter;
      gint pos;

      pos = model_get_type_position (button, ROW_TYPE_SHORTCUT);
      pos += priv->n_shortcuts;

      ctk_list_store_insert (CTK_LIST_STORE (priv->model), &iter, pos);
      ctk_list_store_set (CTK_LIST_STORE (priv->model), &iter,
			  ICON_COLUMN, NULL,
			  DISPLAY_NAME_COLUMN, _(FALLBACK_DISPLAY_NAME),
			  TYPE_COLUMN, ROW_TYPE_SHORTCUT,
			  DATA_COLUMN, g_object_ref (file),
			  IS_FOLDER_COLUMN, FALSE,
			  -1);
      set_info_for_file_at_iter (button, file, &iter);
      priv->n_shortcuts++;

      ctk_tree_model_filter_refilter (CTK_TREE_MODEL_FILTER (priv->filter_model));
    }

  return retval;
}

static gboolean
ctk_file_chooser_button_remove_shortcut_folder (CtkFileChooser  *chooser,
						GFile           *file,
						GError         **error)
{
  CtkFileChooser *delegate;
  gboolean retval;

  delegate = g_object_get_qdata (G_OBJECT (chooser),
				 CTK_FILE_CHOOSER_DELEGATE_QUARK);

  retval = _ctk_file_chooser_remove_shortcut_folder (delegate, file, error);

  if (retval)
    {
      CtkFileChooserButton *button = CTK_FILE_CHOOSER_BUTTON (chooser);
      CtkFileChooserButtonPrivate *priv = button->priv;
      CtkTreeIter iter;
      gint pos;
      gchar type;

      pos = model_get_type_position (button, ROW_TYPE_SHORTCUT);
      ctk_tree_model_iter_nth_child (priv->model, &iter, NULL, pos);

      do
	{
	  gpointer data;

	  ctk_tree_model_get (priv->model, &iter,
			      TYPE_COLUMN, &type,
			      DATA_COLUMN, &data,
			      -1);

	  if (type == ROW_TYPE_SHORTCUT &&
	      data && g_file_equal (data, file))
	    {
	      model_free_row_data (CTK_FILE_CHOOSER_BUTTON (chooser), &iter);
	      ctk_list_store_remove (CTK_LIST_STORE (priv->model), &iter);
	      priv->n_shortcuts--;
	      ctk_tree_model_filter_refilter (CTK_TREE_MODEL_FILTER (priv->filter_model));
	      update_combo_box (CTK_FILE_CHOOSER_BUTTON (chooser));
	      break;
	    }
	}
      while (type == ROW_TYPE_SHORTCUT &&
	     ctk_tree_model_iter_next (priv->model, &iter));
    }

  return retval;
}


/* ******************* *
 *  GObject Functions  *
 * ******************* */

static void
ctk_file_chooser_button_constructed (GObject *object)
{
  CtkFileChooserButton *button = CTK_FILE_CHOOSER_BUTTON (object);
  CtkFileChooserButtonPrivate *priv = button->priv;
  GSList *list;

  G_OBJECT_CLASS (ctk_file_chooser_button_parent_class)->constructed (object);

  if (!priv->dialog)
    {
      priv->native = ctk_file_chooser_native_new (NULL,
                                                  NULL,
						  CTK_FILE_CHOOSER_ACTION_OPEN,
						  NULL,
						  NULL);
      priv->chooser = CTK_FILE_CHOOSER (priv->native);
      ctk_file_chooser_button_set_title (button, _(DEFAULT_TITLE));

      g_signal_connect (priv->native, "response",
                        G_CALLBACK (native_response_cb), object);
    }
  else /* dialog set */
    {
      priv->chooser = CTK_FILE_CHOOSER (priv->dialog);

      if (!ctk_window_get_title (CTK_WINDOW (priv->dialog)))
        ctk_file_chooser_button_set_title (button, _(DEFAULT_TITLE));

      g_signal_connect (priv->dialog, "delete-event",
                        G_CALLBACK (dialog_delete_event_cb), object);
      g_signal_connect (priv->dialog, "response",
                        G_CALLBACK (dialog_response_cb), object);

      g_object_add_weak_pointer (G_OBJECT (priv->dialog),
                                 (gpointer) (&priv->dialog));
    }

  g_signal_connect (priv->chooser, "notify",
                    G_CALLBACK (chooser_notify_cb), object);

  /* This is used, instead of the standard delegate, to ensure that signals are only
   * delegated when the OK button is pressed. */
  g_object_set_qdata (object, CTK_FILE_CHOOSER_DELEGATE_QUARK, priv->chooser);

  priv->fs =
    g_object_ref (_ctk_file_chooser_get_file_system (priv->chooser));

  model_add_special (button);

  list = _ctk_file_system_list_volumes (priv->fs);
  model_add_volumes (button, list);
  g_slist_free (list);

  list = _ctk_bookmarks_manager_list_bookmarks (priv->bookmarks_manager);
  model_add_bookmarks (button, list);
  g_slist_free_full (list, g_object_unref);

  model_add_other (button);

  model_add_empty_selection (button);

  priv->filter_model = ctk_tree_model_filter_new (priv->model, NULL);
  ctk_tree_model_filter_set_visible_func (CTK_TREE_MODEL_FILTER (priv->filter_model),
					  filter_model_visible_func,
					  object, NULL);

  ctk_combo_box_set_model (CTK_COMBO_BOX (priv->combo_box), priv->filter_model);
  ctk_combo_box_set_row_separator_func (CTK_COMBO_BOX (priv->combo_box),
					combo_box_row_separator_func,
					NULL, NULL);

  /* set up the action for a user-provided dialog, this also updates
   * the label, image and combobox
   */
  g_object_set (object,
		"action", ctk_file_chooser_get_action (CTK_FILE_CHOOSER (priv->chooser)),
		NULL);

  priv->fs_volumes_changed_id =
    g_signal_connect (priv->fs, "volumes-changed",
		      G_CALLBACK (fs_volumes_changed_cb), object);

  update_label_and_image (button);
  update_combo_box (button);
}

static void
ctk_file_chooser_button_set_property (GObject      *object,
				      guint         param_id,
				      const GValue *value,
				      GParamSpec   *pspec)
{
  CtkFileChooserButton *button = CTK_FILE_CHOOSER_BUTTON (object);
  CtkFileChooserButtonPrivate *priv = button->priv;

  switch (param_id)
    {
    case PROP_DIALOG:
      /* Construct-only */
      priv->dialog = g_value_get_object (value);
      break;
    case PROP_WIDTH_CHARS:
      ctk_file_chooser_button_set_width_chars (CTK_FILE_CHOOSER_BUTTON (object),
					       g_value_get_int (value));
      break;
    case CTK_FILE_CHOOSER_PROP_ACTION:
      switch (g_value_get_enum (value))
	{
	case CTK_FILE_CHOOSER_ACTION_CREATE_FOLDER:
	case CTK_FILE_CHOOSER_ACTION_SAVE:
	  {
	    GEnumClass *eclass;
	    GEnumValue *eval;

	    eclass = g_type_class_peek (CTK_TYPE_FILE_CHOOSER_ACTION);
	    eval = g_enum_get_value (eclass, g_value_get_enum (value));
	    g_warning ("%s: Choosers of type '%s' do not support '%s'.",
		       G_STRFUNC, G_OBJECT_TYPE_NAME (object), eval->value_name);

	    g_value_set_enum ((GValue *) value, CTK_FILE_CHOOSER_ACTION_OPEN);
	  }
	  break;
	}

      g_object_set_property (G_OBJECT (priv->chooser), pspec->name, value);
      update_label_and_image (CTK_FILE_CHOOSER_BUTTON (object));
      update_combo_box (CTK_FILE_CHOOSER_BUTTON (object));

      switch (g_value_get_enum (value))
	{
	case CTK_FILE_CHOOSER_ACTION_OPEN:
	  ctk_widget_hide (priv->combo_box);
	  ctk_widget_show (priv->button);
	  break;
	case CTK_FILE_CHOOSER_ACTION_SELECT_FOLDER:
	  ctk_widget_hide (priv->button);
	  ctk_widget_show (priv->combo_box);
	  break;
	default:
	  g_assert_not_reached ();
	  break;
	}
      break;

    case PROP_TITLE:
    case CTK_FILE_CHOOSER_PROP_FILTER:
    case CTK_FILE_CHOOSER_PROP_PREVIEW_WIDGET:
    case CTK_FILE_CHOOSER_PROP_PREVIEW_WIDGET_ACTIVE:
    case CTK_FILE_CHOOSER_PROP_USE_PREVIEW_LABEL:
    case CTK_FILE_CHOOSER_PROP_EXTRA_WIDGET:
    case CTK_FILE_CHOOSER_PROP_SHOW_HIDDEN:
    case CTK_FILE_CHOOSER_PROP_DO_OVERWRITE_CONFIRMATION:
    case CTK_FILE_CHOOSER_PROP_CREATE_FOLDERS:
      g_object_set_property (G_OBJECT (priv->chooser), pspec->name, value);
      break;

    case CTK_FILE_CHOOSER_PROP_LOCAL_ONLY:
      g_object_set_property (G_OBJECT (priv->chooser), pspec->name, value);
      fs_volumes_changed_cb (priv->fs, button);
      bookmarks_changed_cb (button);
      break;

    case CTK_FILE_CHOOSER_PROP_SELECT_MULTIPLE:
      g_warning ("%s: Choosers of type '%s' do not support selecting multiple files.",
		 G_STRFUNC, G_OBJECT_TYPE_NAME (object));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
ctk_file_chooser_button_get_property (GObject    *object,
				      guint       param_id,
				      GValue     *value,
				      GParamSpec *pspec)
{
  CtkFileChooserButton *button = CTK_FILE_CHOOSER_BUTTON (object);
  CtkFileChooserButtonPrivate *priv = button->priv;

  switch (param_id)
    {
    case PROP_WIDTH_CHARS:
      g_value_set_int (value,
		       ctk_label_get_width_chars (CTK_LABEL (priv->label)));
      break;

    case PROP_TITLE:
    case CTK_FILE_CHOOSER_PROP_ACTION:
    case CTK_FILE_CHOOSER_PROP_FILTER:
    case CTK_FILE_CHOOSER_PROP_LOCAL_ONLY:
    case CTK_FILE_CHOOSER_PROP_PREVIEW_WIDGET:
    case CTK_FILE_CHOOSER_PROP_PREVIEW_WIDGET_ACTIVE:
    case CTK_FILE_CHOOSER_PROP_USE_PREVIEW_LABEL:
    case CTK_FILE_CHOOSER_PROP_EXTRA_WIDGET:
    case CTK_FILE_CHOOSER_PROP_SELECT_MULTIPLE:
    case CTK_FILE_CHOOSER_PROP_SHOW_HIDDEN:
    case CTK_FILE_CHOOSER_PROP_DO_OVERWRITE_CONFIRMATION:
    case CTK_FILE_CHOOSER_PROP_CREATE_FOLDERS:
      g_object_get_property (G_OBJECT (priv->chooser), pspec->name, value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
ctk_file_chooser_button_finalize (GObject *object)
{
  CtkFileChooserButton *button = CTK_FILE_CHOOSER_BUTTON (object);
  CtkFileChooserButtonPrivate *priv = button->priv;

  if (priv->selection_while_inactive)
    g_object_unref (priv->selection_while_inactive);

  if (priv->current_folder_while_inactive)
    g_object_unref (priv->current_folder_while_inactive);

  if (priv->model)
    {
      model_remove_rows (button, 0, ctk_tree_model_iter_n_children (priv->model, NULL));
      g_object_unref (priv->model);
    }

  G_OBJECT_CLASS (ctk_file_chooser_button_parent_class)->finalize (object);
}

/* ********************* *
 *  CtkWidget Functions  *
 * ********************* */

static void
ctk_file_chooser_button_state_flags_changed (CtkWidget     *widget,
                                             CtkStateFlags  previous_state)
{
  CtkFileChooserButton *button = CTK_FILE_CHOOSER_BUTTON (widget);
  CtkFileChooserButtonPrivate *priv = button->priv;
  CtkWidget *child;

  if (ctk_widget_get_visible (priv->button))
    child = priv->button;
  else
    child = priv->combo_box;

  if (ctk_widget_get_state_flags (widget) & CTK_STATE_FLAG_DROP_ACTIVE)
    ctk_widget_set_state_flags (child, CTK_STATE_FLAG_DROP_ACTIVE, FALSE);
  else
    ctk_widget_unset_state_flags (child, CTK_STATE_FLAG_DROP_ACTIVE);

  CTK_WIDGET_CLASS (ctk_file_chooser_button_parent_class)->state_flags_changed (widget, previous_state);
}

static void
ctk_file_chooser_button_destroy (CtkWidget *widget)
{
  CtkFileChooserButton *button = CTK_FILE_CHOOSER_BUTTON (widget);
  CtkFileChooserButtonPrivate *priv = button->priv;
  CtkTreeIter iter;
  GSList *l;

  if (priv->dialog != NULL)
    {
      ctk_widget_destroy (priv->dialog);
      priv->dialog = NULL;
    }

  if (priv->native)
    {
      ctk_native_dialog_destroy (CTK_NATIVE_DIALOG (priv->native));
      g_clear_object (&priv->native);
    }

  priv->chooser = NULL;

  if (priv->model && ctk_tree_model_get_iter_first (priv->model, &iter))
    {
      do
        model_free_row_data (button, &iter);
      while (ctk_tree_model_iter_next (priv->model, &iter));
    }

  if (priv->dnd_select_folder_cancellable)
    {
      g_cancellable_cancel (priv->dnd_select_folder_cancellable);
      priv->dnd_select_folder_cancellable = NULL;
    }

  if (priv->update_button_cancellable)
    {
      g_cancellable_cancel (priv->update_button_cancellable);
      priv->update_button_cancellable = NULL;
    }

  if (priv->change_icon_theme_cancellables)
    {
      for (l = priv->change_icon_theme_cancellables; l; l = l->next)
        {
	  GCancellable *cancellable = G_CANCELLABLE (l->data);
	  g_cancellable_cancel (cancellable);
        }
      g_slist_free (priv->change_icon_theme_cancellables);
      priv->change_icon_theme_cancellables = NULL;
    }

  if (priv->filter_model)
    {
      g_object_unref (priv->filter_model);
      priv->filter_model = NULL;
    }

  if (priv->fs)
    {
      g_signal_handler_disconnect (priv->fs, priv->fs_volumes_changed_id);
      g_object_unref (priv->fs);
      priv->fs = NULL;
    }

  if (priv->bookmarks_manager)
    {
      _ctk_bookmarks_manager_free (priv->bookmarks_manager);
      priv->bookmarks_manager = NULL;
    }

  CTK_WIDGET_CLASS (ctk_file_chooser_button_parent_class)->destroy (widget);
}

struct DndSelectFolderData
{
  CtkFileSystem *file_system;
  CtkFileChooserButton *button;
  CtkFileChooserAction action;
  GFile *file;
  gchar **uris;
  guint i;
  gboolean selected;
};

static void
dnd_select_folder_get_info_cb (GCancellable *cancellable,
			       GFileInfo    *info,
			       const GError *error,
			       gpointer      user_data)
{
  gboolean cancelled = g_cancellable_is_cancelled (cancellable);
  struct DndSelectFolderData *data = user_data;

  if (cancellable != data->button->priv->dnd_select_folder_cancellable)
    {
      g_object_unref (data->button);
      g_object_unref (data->file);
      g_strfreev (data->uris);
      g_free (data);

      g_object_unref (cancellable);
      return;
    }

  data->button->priv->dnd_select_folder_cancellable = NULL;

  if (!cancelled && !error && info != NULL)
    {
      gboolean is_folder;

      is_folder = _ctk_file_info_consider_as_directory (info);

      data->selected =
	(((data->action == CTK_FILE_CHOOSER_ACTION_SELECT_FOLDER && is_folder) ||
	  (data->action == CTK_FILE_CHOOSER_ACTION_OPEN && !is_folder)) &&
	 ctk_file_chooser_select_file (CTK_FILE_CHOOSER (data->button), data->file, NULL));
    }
  else
    data->selected = FALSE;

  if (data->selected || data->uris[++data->i] == NULL)
    {
      g_signal_emit (data->button, file_chooser_button_signals[FILE_SET], 0);

      g_object_unref (data->button);
      g_object_unref (data->file);
      g_strfreev (data->uris);
      g_free (data);

      g_object_unref (cancellable);
      return;
    }

  if (data->file)
    g_object_unref (data->file);

  data->file = g_file_new_for_uri (data->uris[data->i]);

  data->button->priv->dnd_select_folder_cancellable =
    _ctk_file_system_get_info (data->file_system, data->file,
			       "standard::type",
			       dnd_select_folder_get_info_cb, user_data);

  g_object_unref (cancellable);
}

static void
ctk_file_chooser_button_drag_data_received (CtkWidget	     *widget,
					    GdkDragContext   *context,
					    gint	      x,
					    gint	      y,
					    CtkSelectionData *data,
					    guint	      type,
					    guint	      drag_time)
{
  CtkFileChooserButton *button = CTK_FILE_CHOOSER_BUTTON (widget);
  CtkFileChooserButtonPrivate *priv = button->priv;
  GFile *file;
  gchar *text;

  if (CTK_WIDGET_CLASS (ctk_file_chooser_button_parent_class)->drag_data_received != NULL)
    CTK_WIDGET_CLASS (ctk_file_chooser_button_parent_class)->drag_data_received (widget,
										 context,
										 x, y,
										 data, type,
										 drag_time);

  if (widget == NULL || context == NULL || data == NULL || ctk_selection_data_get_length (data) < 0)
    return;

  switch (type)
    {
    case TEXT_URI_LIST:
      {
	gchar **uris;
	struct DndSelectFolderData *info;

	uris = ctk_selection_data_get_uris (data);

	if (uris == NULL)
	  break;

	info = g_new0 (struct DndSelectFolderData, 1);
	info->button = g_object_ref (button);
	info->i = 0;
	info->uris = uris;
	info->selected = FALSE;
	info->file_system = priv->fs;
	g_object_get (priv->chooser, "action", &info->action, NULL);

	info->file = g_file_new_for_uri (info->uris[info->i]);

	if (priv->dnd_select_folder_cancellable)
	  g_cancellable_cancel (priv->dnd_select_folder_cancellable);

	priv->dnd_select_folder_cancellable =
	  _ctk_file_system_get_info (priv->fs, info->file,
				     "standard::type",
				     dnd_select_folder_get_info_cb, info);
      }
      break;

    case TEXT_PLAIN:
      text = (char*) ctk_selection_data_get_text (data);
      file = g_file_new_for_uri (text);
      ctk_file_chooser_select_file (CTK_FILE_CHOOSER (priv->chooser), file, NULL);
      g_object_unref (file);
      g_free (text);
      g_signal_emit (button, file_chooser_button_signals[FILE_SET], 0);
      break;

    default:
      break;
    }

  ctk_drag_finish (context, TRUE, FALSE, drag_time);
}

static void
ctk_file_chooser_button_show_all (CtkWidget *widget)
{
  ctk_widget_show (widget);
}

static void
ctk_file_chooser_button_show (CtkWidget *widget)
{
  CtkFileChooserButton *button = CTK_FILE_CHOOSER_BUTTON (widget);
  CtkFileChooserButtonPrivate *priv = button->priv;

  if (CTK_WIDGET_CLASS (ctk_file_chooser_button_parent_class)->show)
    CTK_WIDGET_CLASS (ctk_file_chooser_button_parent_class)->show (widget);

  if (priv->active)
    open_dialog (CTK_FILE_CHOOSER_BUTTON (widget));
}

static void
ctk_file_chooser_button_hide (CtkWidget *widget)
{
  CtkFileChooserButton *button = CTK_FILE_CHOOSER_BUTTON (widget);
  CtkFileChooserButtonPrivate *priv = button->priv;

  if (priv->dialog)
    ctk_widget_hide (priv->dialog);
  else
    ctk_native_dialog_hide (CTK_NATIVE_DIALOG (priv->native));

  if (CTK_WIDGET_CLASS (ctk_file_chooser_button_parent_class)->hide)
    CTK_WIDGET_CLASS (ctk_file_chooser_button_parent_class)->hide (widget);
}

static void
ctk_file_chooser_button_map (CtkWidget *widget)
{
  CTK_WIDGET_CLASS (ctk_file_chooser_button_parent_class)->map (widget);
}

static gboolean
ctk_file_chooser_button_mnemonic_activate (CtkWidget *widget,
					   gboolean   group_cycling)
{
  CtkFileChooserButton *button = CTK_FILE_CHOOSER_BUTTON (widget);
  CtkFileChooserButtonPrivate *priv = button->priv;

  switch (ctk_file_chooser_get_action (CTK_FILE_CHOOSER (priv->chooser)))
    {
    case CTK_FILE_CHOOSER_ACTION_OPEN:
      ctk_widget_grab_focus (priv->button);
      break;
    case CTK_FILE_CHOOSER_ACTION_SELECT_FOLDER:
      return ctk_widget_mnemonic_activate (priv->combo_box, group_cycling);
      break;
    default:
      g_assert_not_reached ();
      break;
    }

  return TRUE;
}

/* Changes the icons wherever it is needed */
struct ChangeIconThemeData
{
  CtkFileChooserButton *button;
  CtkTreeRowReference *row_ref;
};

static void
change_icon_theme_get_info_cb (GCancellable *cancellable,
			       GFileInfo    *info,
			       const GError *error,
			       gpointer      user_data)
{
  gboolean cancelled = g_cancellable_is_cancelled (cancellable);
  cairo_surface_t *surface;
  struct ChangeIconThemeData *data = user_data;

  if (!g_slist_find (data->button->priv->change_icon_theme_cancellables, cancellable))
    goto out;

  data->button->priv->change_icon_theme_cancellables =
    g_slist_remove (data->button->priv->change_icon_theme_cancellables, cancellable);

  if (cancelled || error)
    goto out;

  surface = _ctk_file_info_render_icon (info, CTK_WIDGET (data->button), data->button->priv->icon_size);

  if (surface)
    {
      gint width = 0;
      CtkTreeIter iter;
      CtkTreePath *path;

      width = MAX (width, data->button->priv->icon_size);

      path = ctk_tree_row_reference_get_path (data->row_ref);
      if (path)
        {
          ctk_tree_model_get_iter (data->button->priv->model, &iter, path);
          ctk_tree_path_free (path);

          ctk_list_store_set (CTK_LIST_STORE (data->button->priv->model), &iter,
	  		      ICON_COLUMN, surface,
			      -1);

          g_object_set (data->button->priv->icon_cell,
		        "width", width,
		        NULL);
        }
      cairo_surface_destroy (surface);
    }

out:
  g_object_unref (data->button);
  ctk_tree_row_reference_free (data->row_ref);
  g_free (data);

  g_object_unref (cancellable);
}

static void
change_icon_theme (CtkFileChooserButton *button)
{
  CtkFileChooserButtonPrivate *priv = button->priv;
  CtkIconTheme *theme;
  CtkTreeIter iter;
  GSList *l;
  gint width = 0, height = 0;

  for (l = button->priv->change_icon_theme_cancellables; l; l = l->next)
    {
      GCancellable *cancellable = G_CANCELLABLE (l->data);
      g_cancellable_cancel (cancellable);
    }
  g_slist_free (button->priv->change_icon_theme_cancellables);
  button->priv->change_icon_theme_cancellables = NULL;

  if (ctk_icon_size_lookup (CTK_ICON_SIZE_MENU, &width, &height))
    priv->icon_size = MAX (width, height);
  else
    priv->icon_size = FALLBACK_ICON_SIZE;

  update_label_and_image (button);

  ctk_tree_model_get_iter_first (priv->model, &iter);

  theme = get_icon_theme (CTK_WIDGET (button));

  do
    {
      cairo_surface_t *surface = NULL;
      gchar type;
      gpointer data;

      type = ROW_TYPE_INVALID;
      ctk_tree_model_get (priv->model, &iter,
			  TYPE_COLUMN, &type,
			  DATA_COLUMN, &data,
			  -1);

      switch (type)
	{
	case ROW_TYPE_SPECIAL:
	case ROW_TYPE_SHORTCUT:
	case ROW_TYPE_BOOKMARK:
	case ROW_TYPE_CURRENT_FOLDER:
	  if (data)
	    {
	      if (g_file_is_native (G_FILE (data)))
		{
		  CtkTreePath *path;
		  GCancellable *cancellable;
		  struct ChangeIconThemeData *info;

		  info = g_new0 (struct ChangeIconThemeData, 1);
		  info->button = g_object_ref (button);
		  path = ctk_tree_model_get_path (priv->model, &iter);
		  info->row_ref = ctk_tree_row_reference_new (priv->model, path);
		  ctk_tree_path_free (path);

		  cancellable =
		    _ctk_file_system_get_info (priv->fs, data,
					       "standard::icon",
					       change_icon_theme_get_info_cb,
					       info);
		  button->priv->change_icon_theme_cancellables =
		    g_slist_append (button->priv->change_icon_theme_cancellables, cancellable);
		  surface = NULL;
		}
	      else
                {
                  /* Don't call get_info for remote paths to avoid latency and
                   * auth dialogs.
                   * If we switch to a better bookmarks file format (XBEL), we
                   * should use mime info to get a better icon.
                   */
                  surface = ctk_icon_theme_load_surface (theme, "folder-remote",
                                                         priv->icon_size, 
                                                         ctk_widget_get_scale_factor (CTK_WIDGET (button)),
                                                         ctk_widget_get_window (CTK_WIDGET (button)),
                                                         0, NULL);
                }
	    }
	  break;
	case ROW_TYPE_VOLUME:
	  if (data)
            {
              surface = _ctk_file_system_volume_render_icon (data,
                                                             CTK_WIDGET (button),
                                                             priv->icon_size,
                                                             NULL);
            }

	  break;
	default:
	  continue;
	  break;
	}

      if (surface)
	width = MAX (width, priv->icon_size);

      ctk_list_store_set (CTK_LIST_STORE (priv->model), &iter,
			  ICON_COLUMN, surface,
			  -1);

      if (surface)
	cairo_surface_destroy (surface);
    }
  while (ctk_tree_model_iter_next (priv->model, &iter));

  g_object_set (button->priv->icon_cell,
		"width", width,
		NULL);
}

static void
ctk_file_chooser_button_style_updated (CtkWidget *widget)
{
  CTK_WIDGET_CLASS (ctk_file_chooser_button_parent_class)->style_updated (widget);

  if (ctk_widget_has_screen (widget))
    {
      /* We need to update the icon surface, but only in case
       * the icon theme really changed. */
      CtkStyleContext *context = ctk_widget_get_style_context (widget);
      CtkCssStyleChange *change = ctk_style_context_get_change (context);
      if (!change || ctk_css_style_change_changes_property (change, CTK_CSS_PROPERTY_ICON_THEME))
        change_icon_theme (CTK_FILE_CHOOSER_BUTTON (widget));
    }
}

static void
ctk_file_chooser_button_screen_changed (CtkWidget *widget,
					GdkScreen *old_screen)
{
  if (CTK_WIDGET_CLASS (ctk_file_chooser_button_parent_class)->screen_changed)
    CTK_WIDGET_CLASS (ctk_file_chooser_button_parent_class)->screen_changed (widget,
									     old_screen);

  change_icon_theme (CTK_FILE_CHOOSER_BUTTON (widget));
}


/* ******************* *
 *  Utility Functions  *
 * ******************* */

/* General */
static CtkIconTheme *
get_icon_theme (CtkWidget *widget)
{
  return ctk_css_icon_theme_value_get_icon_theme
    (_ctk_style_context_peek_property (ctk_widget_get_style_context (widget), CTK_CSS_PROPERTY_ICON_THEME));
}


struct SetDisplayNameData
{
  CtkFileChooserButton *button;
  char *label;
  CtkTreeRowReference *row_ref;
};

static void
set_info_get_info_cb (GCancellable *cancellable,
		      GFileInfo    *info,
		      const GError *error,
		      gpointer      callback_data)
{
  gboolean cancelled = g_cancellable_is_cancelled (cancellable);
  cairo_surface_t *surface;
  CtkTreePath *path;
  CtkTreeIter iter;
  GCancellable *model_cancellable = NULL;
  struct SetDisplayNameData *data = callback_data;
  gboolean is_folder;

  if (!data->button->priv->model)
    /* button got destroyed */
    goto out;

  path = ctk_tree_row_reference_get_path (data->row_ref);
  if (!path)
    /* Cancellable doesn't exist anymore in the model */
    goto out;

  ctk_tree_model_get_iter (data->button->priv->model, &iter, path);
  ctk_tree_path_free (path);

  /* Validate the cancellable */
  ctk_tree_model_get (data->button->priv->model, &iter,
		      CANCELLABLE_COLUMN, &model_cancellable,
		      -1);
  if (cancellable != model_cancellable)
    goto out;

  ctk_list_store_set (CTK_LIST_STORE (data->button->priv->model), &iter,
		      CANCELLABLE_COLUMN, NULL,
		      -1);

  if (cancelled || error)
    /* There was an error, leave the fallback name in there */
    goto out;

  surface = _ctk_file_info_render_icon (info, CTK_WIDGET (data->button), data->button->priv->icon_size);

  if (!data->label)
    data->label = g_strdup (g_file_info_get_display_name (info));

  is_folder = _ctk_file_info_consider_as_directory (info);

  ctk_list_store_set (CTK_LIST_STORE (data->button->priv->model), &iter,
		      ICON_COLUMN, surface,
		      DISPLAY_NAME_COLUMN, data->label,
		      IS_FOLDER_COLUMN, is_folder,
		      -1);

  if (surface)
    cairo_surface_destroy (surface);

out:
  g_object_unref (data->button);
  g_free (data->label);
  ctk_tree_row_reference_free (data->row_ref);
  g_free (data);

  g_object_unref (cancellable);
}

static void
set_info_for_file_at_iter (CtkFileChooserButton *button,
			   GFile                *file,
			   CtkTreeIter          *iter)
{
  struct SetDisplayNameData *data;
  CtkTreePath *tree_path;
  GCancellable *cancellable;

  data = g_new0 (struct SetDisplayNameData, 1);
  data->button = g_object_ref (button);
  data->label = _ctk_bookmarks_manager_get_bookmark_label (button->priv->bookmarks_manager, file);

  tree_path = ctk_tree_model_get_path (button->priv->model, iter);
  data->row_ref = ctk_tree_row_reference_new (button->priv->model, tree_path);
  ctk_tree_path_free (tree_path);

  cancellable = _ctk_file_system_get_info (button->priv->fs, file,
					   "standard::type,standard::icon,standard::display-name",
					   set_info_get_info_cb, data);

  ctk_list_store_set (CTK_LIST_STORE (button->priv->model), iter,
		      CANCELLABLE_COLUMN, cancellable,
		      -1);
}

/* Shortcuts Model */
static gint
model_get_type_position (CtkFileChooserButton *button,
			 RowType               row_type)
{
  gint retval = 0;

  if (row_type == ROW_TYPE_SPECIAL)
    return retval;

  retval += button->priv->n_special;

  if (row_type == ROW_TYPE_VOLUME)
    return retval;

  retval += button->priv->n_volumes;

  if (row_type == ROW_TYPE_SHORTCUT)
    return retval;

  retval += button->priv->n_shortcuts;

  if (row_type == ROW_TYPE_BOOKMARK_SEPARATOR)
    return retval;

  retval += button->priv->has_bookmark_separator;

  if (row_type == ROW_TYPE_BOOKMARK)
    return retval;

  retval += button->priv->n_bookmarks;

  if (row_type == ROW_TYPE_CURRENT_FOLDER_SEPARATOR)
    return retval;

  retval += button->priv->has_current_folder_separator;

  if (row_type == ROW_TYPE_CURRENT_FOLDER)
    return retval;

  retval += button->priv->has_current_folder;

  if (row_type == ROW_TYPE_OTHER_SEPARATOR)
    return retval;

  retval += button->priv->has_other_separator;

  if (row_type == ROW_TYPE_OTHER)
    return retval;

  retval++;

  if (row_type == ROW_TYPE_EMPTY_SELECTION)
    return retval;

  g_assert_not_reached ();
  return -1;
}

static void
model_free_row_data (CtkFileChooserButton *button,
		     CtkTreeIter          *iter)
{
  gchar type;
  gpointer data;
  GCancellable *cancellable;

  ctk_tree_model_get (button->priv->model, iter,
		      TYPE_COLUMN, &type,
		      DATA_COLUMN, &data,
		      CANCELLABLE_COLUMN, &cancellable,
		      -1);

  if (cancellable)
    g_cancellable_cancel (cancellable);

  switch (type)
    {
    case ROW_TYPE_SPECIAL:
    case ROW_TYPE_SHORTCUT:
    case ROW_TYPE_BOOKMARK:
    case ROW_TYPE_CURRENT_FOLDER:
      g_object_unref (data);
      break;
    case ROW_TYPE_VOLUME:
      _ctk_file_system_volume_unref (data);
      break;
    default:
      break;
    }
}

static void
model_add_special_get_info_cb (GCancellable *cancellable,
			       GFileInfo    *info,
			       const GError *error,
			       gpointer      user_data)
{
  gboolean cancelled = g_cancellable_is_cancelled (cancellable);
  CtkTreeIter iter;
  CtkTreePath *path;
  cairo_surface_t *surface;
  GCancellable *model_cancellable = NULL;
  struct ChangeIconThemeData *data = user_data;
  gchar *name;

  if (!data->button->priv->model)
    /* button got destroyed */
    goto out;

  path = ctk_tree_row_reference_get_path (data->row_ref);
  if (!path)
    /* Cancellable doesn't exist anymore in the model */
    goto out;

  ctk_tree_model_get_iter (data->button->priv->model, &iter, path);
  ctk_tree_path_free (path);

  ctk_tree_model_get (data->button->priv->model, &iter,
		      CANCELLABLE_COLUMN, &model_cancellable,
		      -1);
  if (cancellable != model_cancellable)
    goto out;

  ctk_list_store_set (CTK_LIST_STORE (data->button->priv->model), &iter,
		      CANCELLABLE_COLUMN, NULL,
		      -1);

  if (cancelled || error)
    goto out;

  surface = _ctk_file_info_render_icon (info, CTK_WIDGET (data->button), data->button->priv->icon_size);
  if (surface)
    {
      ctk_list_store_set (CTK_LIST_STORE (data->button->priv->model), &iter,
			  ICON_COLUMN, surface,
			  -1);
      cairo_surface_destroy (surface);
    }

  ctk_tree_model_get (data->button->priv->model, &iter,
                      DISPLAY_NAME_COLUMN, &name,
                      -1);
  if (!name)
    ctk_list_store_set (CTK_LIST_STORE (data->button->priv->model), &iter,
  		        DISPLAY_NAME_COLUMN, g_file_info_get_display_name (info),
		        -1);
  g_free (name);

out:
  g_object_unref (data->button);
  ctk_tree_row_reference_free (data->row_ref);
  g_free (data);

  g_object_unref (cancellable);
}

static void
model_add_special (CtkFileChooserButton *button)
{
  const gchar *homedir;
  const gchar *desktopdir;
  CtkListStore *store;
  CtkTreeIter iter;
  GFile *file;
  gint pos;

  store = CTK_LIST_STORE (button->priv->model);
  pos = model_get_type_position (button, ROW_TYPE_SPECIAL);

  homedir = g_get_home_dir ();

  if (homedir)
    {
      CtkTreePath *tree_path;
      GCancellable *cancellable;
      struct ChangeIconThemeData *info;

      file = g_file_new_for_path (homedir);
      ctk_list_store_insert (store, &iter, pos);
      pos++;

      info = g_new0 (struct ChangeIconThemeData, 1);
      info->button = g_object_ref (button);
      tree_path = ctk_tree_model_get_path (CTK_TREE_MODEL (store), &iter);
      info->row_ref = ctk_tree_row_reference_new (CTK_TREE_MODEL (store),
						  tree_path);
      ctk_tree_path_free (tree_path);

      cancellable = _ctk_file_system_get_info (button->priv->fs, file,
					       "standard::icon,standard::display-name",
					       model_add_special_get_info_cb, info);

      ctk_list_store_set (store, &iter,
			  ICON_COLUMN, NULL,
			  DISPLAY_NAME_COLUMN, NULL,
			  TYPE_COLUMN, ROW_TYPE_SPECIAL,
			  DATA_COLUMN, file,
			  IS_FOLDER_COLUMN, TRUE,
			  CANCELLABLE_COLUMN, cancellable,
			  -1);

      button->priv->n_special++;
    }

  desktopdir = g_get_user_special_dir (G_USER_DIRECTORY_DESKTOP);

  /* "To disable a directory, point it to the homedir."
   * See http://freedesktop.org/wiki/Software/xdg-user-dirs
   */
  if (g_strcmp0 (desktopdir, g_get_home_dir ()) != 0)
    {
      CtkTreePath *tree_path;
      GCancellable *cancellable;
      struct ChangeIconThemeData *info;

      file = g_file_new_for_path (desktopdir);
      ctk_list_store_insert (store, &iter, pos);
      pos++;

      info = g_new0 (struct ChangeIconThemeData, 1);
      info->button = g_object_ref (button);
      tree_path = ctk_tree_model_get_path (CTK_TREE_MODEL (store), &iter);
      info->row_ref = ctk_tree_row_reference_new (CTK_TREE_MODEL (store),
						  tree_path);
      ctk_tree_path_free (tree_path);

      cancellable = _ctk_file_system_get_info (button->priv->fs, file,
					       "standard::icon,standard::display-name",
					       model_add_special_get_info_cb, info);

      ctk_list_store_set (store, &iter,
			  TYPE_COLUMN, ROW_TYPE_SPECIAL,
			  ICON_COLUMN, NULL,
			  DISPLAY_NAME_COLUMN, _(DESKTOP_DISPLAY_NAME),
			  DATA_COLUMN, file,
			  IS_FOLDER_COLUMN, TRUE,
			  CANCELLABLE_COLUMN, cancellable,
			  -1);

      button->priv->n_special++;
    }
}

static void
model_add_volumes (CtkFileChooserButton *button,
                   GSList               *volumes)
{
  CtkListStore *store;
  gint pos;
  gboolean local_only;
  GSList *l;

  if (!volumes)
    return;

  store = CTK_LIST_STORE (button->priv->model);
  pos = model_get_type_position (button, ROW_TYPE_VOLUME);
  local_only = ctk_file_chooser_get_local_only (CTK_FILE_CHOOSER (button->priv->chooser));

  for (l = volumes; l; l = l->next)
    {
      CtkFileSystemVolume *volume;
      CtkTreeIter iter;
      cairo_surface_t *surface;
      gchar *display_name;

      volume = l->data;

      if (local_only)
        {
          if (_ctk_file_system_volume_is_mounted (volume))
            {
              GFile *base_file;

              base_file = _ctk_file_system_volume_get_root (volume);
              if (base_file != NULL)
                {
                  if (!_ctk_file_has_native_path (base_file))
                    {
                      g_object_unref (base_file);
                      continue;
                    }
                  else
                    g_object_unref (base_file);
                }
            }
        }

      surface = _ctk_file_system_volume_render_icon (volume,
                                                     CTK_WIDGET (button),
                                                     button->priv->icon_size,
                                                     NULL);
      display_name = _ctk_file_system_volume_get_display_name (volume);

      ctk_list_store_insert (store, &iter, pos);
      ctk_list_store_set (store, &iter,
                          ICON_COLUMN, surface,
                          DISPLAY_NAME_COLUMN, display_name,
                          TYPE_COLUMN, ROW_TYPE_VOLUME,
                          DATA_COLUMN, _ctk_file_system_volume_ref (volume),
                          IS_FOLDER_COLUMN, TRUE,
                          -1);

      if (surface)
        cairo_surface_destroy (surface);
      g_free (display_name);

      button->priv->n_volumes++;
      pos++;
    }
}

extern gchar * _ctk_file_chooser_label_for_file (GFile *file);

static void
model_add_bookmarks (CtkFileChooserButton *button,
		     GSList               *bookmarks)
{
  CtkListStore *store;
  CtkTreeIter iter;
  gint pos;
  gboolean local_only;
  GSList *l;

  if (!bookmarks)
    return;

  store = CTK_LIST_STORE (button->priv->model);
  pos = model_get_type_position (button, ROW_TYPE_BOOKMARK);
  local_only = ctk_file_chooser_get_local_only (CTK_FILE_CHOOSER (button->priv->chooser));

  for (l = bookmarks; l; l = l->next)
    {
      GFile *file;

      file = l->data;

      if (_ctk_file_has_native_path (file))
	{
	  ctk_list_store_insert (store, &iter, pos);
	  ctk_list_store_set (store, &iter,
			      ICON_COLUMN, NULL,
			      DISPLAY_NAME_COLUMN, _(FALLBACK_DISPLAY_NAME),
			      TYPE_COLUMN, ROW_TYPE_BOOKMARK,
			      DATA_COLUMN, g_object_ref (file),
			      IS_FOLDER_COLUMN, FALSE,
			      -1);
	  set_info_for_file_at_iter (button, file, &iter);
	}
      else
	{
	  gchar *label;
	  CtkIconTheme *icon_theme;
	  cairo_surface_t *surface = NULL;

	  if (local_only)
	    continue;

	  /* Don't call get_info for remote paths to avoid latency and
	   * auth dialogs.
	   * If we switch to a better bookmarks file format (XBEL), we
	   * should use mime info to get a better icon.
	   */
	  label = _ctk_bookmarks_manager_get_bookmark_label (button->priv->bookmarks_manager, file);
	  if (!label)
	    label = _ctk_file_chooser_label_for_file (file);

          icon_theme = get_icon_theme (CTK_WIDGET (button));
	  surface = ctk_icon_theme_load_surface (icon_theme, "folder-remote",
						 button->priv->icon_size, 
						 ctk_widget_get_scale_factor (CTK_WIDGET (button)),
						 ctk_widget_get_window (CTK_WIDGET (button)),
						 0, NULL);

	  ctk_list_store_insert (store, &iter, pos);
	  ctk_list_store_set (store, &iter,
			      ICON_COLUMN, surface,
			      DISPLAY_NAME_COLUMN, label,
			      TYPE_COLUMN, ROW_TYPE_BOOKMARK,
			      DATA_COLUMN, g_object_ref (file),
			      IS_FOLDER_COLUMN, TRUE,
			      -1);

	  g_free (label);
	  if (surface)
	    cairo_surface_destroy (surface);
	}

      button->priv->n_bookmarks++;
      pos++;
    }

  if (button->priv->n_bookmarks > 0 &&
      !button->priv->has_bookmark_separator)
    {
      pos = model_get_type_position (button, ROW_TYPE_BOOKMARK_SEPARATOR);

      ctk_list_store_insert (store, &iter, pos);
      ctk_list_store_set (store, &iter,
			  ICON_COLUMN, NULL,
			  DISPLAY_NAME_COLUMN, NULL,
			  TYPE_COLUMN, ROW_TYPE_BOOKMARK_SEPARATOR,
			  DATA_COLUMN, NULL,
			  IS_FOLDER_COLUMN, FALSE,
			  -1);
      button->priv->has_bookmark_separator = TRUE;
    }
}

static void
model_update_current_folder (CtkFileChooserButton *button,
			     GFile                *file)
{
  CtkListStore *store;
  CtkTreeIter iter;
  gint pos;

  if (!file)
    return;

  store = CTK_LIST_STORE (button->priv->model);

  if (!button->priv->has_current_folder_separator)
    {
      pos = model_get_type_position (button, ROW_TYPE_CURRENT_FOLDER_SEPARATOR);
      ctk_list_store_insert (store, &iter, pos);
      ctk_list_store_set (store, &iter,
			  ICON_COLUMN, NULL,
			  DISPLAY_NAME_COLUMN, NULL,
			  TYPE_COLUMN, ROW_TYPE_CURRENT_FOLDER_SEPARATOR,
			  DATA_COLUMN, NULL,
			  IS_FOLDER_COLUMN, FALSE,
			  -1);
      button->priv->has_current_folder_separator = TRUE;
    }

  pos = model_get_type_position (button, ROW_TYPE_CURRENT_FOLDER);
  if (!button->priv->has_current_folder)
    {
      ctk_list_store_insert (store, &iter, pos);
      button->priv->has_current_folder = TRUE;
    }
  else
    {
      ctk_tree_model_iter_nth_child (button->priv->model, &iter, NULL, pos);
      model_free_row_data (button, &iter);
    }

  if (g_file_is_native (file))
    {
      ctk_list_store_set (store, &iter,
			  ICON_COLUMN, NULL,
			  DISPLAY_NAME_COLUMN, _(FALLBACK_DISPLAY_NAME),
			  TYPE_COLUMN, ROW_TYPE_CURRENT_FOLDER,
			  DATA_COLUMN, g_object_ref (file),
			  IS_FOLDER_COLUMN, FALSE,
			  -1);
      set_info_for_file_at_iter (button, file, &iter);
    }
  else
    {
      gchar *label;
      CtkIconTheme *icon_theme;
      cairo_surface_t *surface;

      /* Don't call get_info for remote paths to avoid latency and
       * auth dialogs.
       * If we switch to a better bookmarks file format (XBEL), we
       * should use mime info to get a better icon.
       */
      label = _ctk_bookmarks_manager_get_bookmark_label (button->priv->bookmarks_manager, file);
      if (!label)
	label = _ctk_file_chooser_label_for_file (file);

      icon_theme = get_icon_theme (CTK_WIDGET (button));

      if (g_file_is_native (file))
	  surface = ctk_icon_theme_load_surface (icon_theme, "folder",
						 button->priv->icon_size, 
						 ctk_widget_get_scale_factor (CTK_WIDGET (button)),
						 ctk_widget_get_window (CTK_WIDGET (button)),
						 0, NULL);
      else
	  surface = ctk_icon_theme_load_surface (icon_theme, "folder-remote",
						 button->priv->icon_size, 
						 ctk_widget_get_scale_factor (CTK_WIDGET (button)),
						 ctk_widget_get_window (CTK_WIDGET (button)),
						 0, NULL);

      ctk_list_store_set (store, &iter,
			  ICON_COLUMN, surface,
			  DISPLAY_NAME_COLUMN, label,
			  TYPE_COLUMN, ROW_TYPE_CURRENT_FOLDER,
			  DATA_COLUMN, g_object_ref (file),
			  IS_FOLDER_COLUMN, TRUE,
			  -1);

      g_free (label);
      if (surface)
	cairo_surface_destroy (surface);
    }
}

static void
model_add_other (CtkFileChooserButton *button)
{
  CtkListStore *store;
  CtkTreeIter iter;
  gint pos;

  store = CTK_LIST_STORE (button->priv->model);
  pos = model_get_type_position (button, ROW_TYPE_OTHER_SEPARATOR);

  ctk_list_store_insert (store, &iter, pos);
  ctk_list_store_set (store, &iter,
		      ICON_COLUMN, NULL,
		      DISPLAY_NAME_COLUMN, NULL,
		      TYPE_COLUMN, ROW_TYPE_OTHER_SEPARATOR,
		      DATA_COLUMN, NULL,
		      IS_FOLDER_COLUMN, FALSE,
		      -1);
  button->priv->has_other_separator = TRUE;
  pos++;

  ctk_list_store_insert (store, &iter, pos);
  ctk_list_store_set (store, &iter,
		      ICON_COLUMN, NULL,
		      DISPLAY_NAME_COLUMN, _("Other…"),
		      TYPE_COLUMN, ROW_TYPE_OTHER,
		      DATA_COLUMN, NULL,
		      IS_FOLDER_COLUMN, FALSE,
		      -1);
}

static void
model_add_empty_selection (CtkFileChooserButton *button)
{
  CtkListStore *store;
  CtkTreeIter iter;
  gint pos;

  store = CTK_LIST_STORE (button->priv->model);
  pos = model_get_type_position (button, ROW_TYPE_EMPTY_SELECTION);

  ctk_list_store_insert (store, &iter, pos);
  ctk_list_store_set (store, &iter,
		      ICON_COLUMN, NULL,
		      DISPLAY_NAME_COLUMN, _(FALLBACK_DISPLAY_NAME),
		      TYPE_COLUMN, ROW_TYPE_EMPTY_SELECTION,
		      DATA_COLUMN, NULL,
		      IS_FOLDER_COLUMN, FALSE,
		      -1);
}

static void
model_remove_rows (CtkFileChooserButton *button,
		   gint                  pos,
		   gint                  n_rows)
{
  CtkListStore *store;

  if (!n_rows)
    return;

  store = CTK_LIST_STORE (button->priv->model);

  do
    {
      CtkTreeIter iter;

      if (!ctk_tree_model_iter_nth_child (button->priv->model, &iter, NULL, pos))
	g_assert_not_reached ();

      model_free_row_data (button, &iter);
      ctk_list_store_remove (store, &iter);
      n_rows--;
    }
  while (n_rows);
}

/* Filter Model */
static gboolean
test_if_file_is_visible (CtkFileSystem *fs,
			 GFile         *file,
			 gboolean       local_only,
			 gboolean       is_folder)
{
  if (!file)
    return FALSE;

  if (local_only && !_ctk_file_has_native_path (file))
    return FALSE;

  if (!is_folder)
    return FALSE;

  return TRUE;
}

static gboolean
filter_model_visible_func (CtkTreeModel *model,
			   CtkTreeIter  *iter,
			   gpointer      user_data)
{
  CtkFileChooserButton *button = CTK_FILE_CHOOSER_BUTTON (user_data);
  CtkFileChooserButtonPrivate *priv = button->priv;
  gchar type;
  gpointer data;
  gboolean local_only, retval, is_folder;

  type = ROW_TYPE_INVALID;
  data = NULL;
  local_only = ctk_file_chooser_get_local_only (CTK_FILE_CHOOSER (priv->chooser));

  ctk_tree_model_get (model, iter,
		      TYPE_COLUMN, &type,
		      DATA_COLUMN, &data,
		      IS_FOLDER_COLUMN, &is_folder,
		      -1);

  switch (type)
    {
    case ROW_TYPE_CURRENT_FOLDER:
      retval = TRUE;
      break;
    case ROW_TYPE_SPECIAL:
    case ROW_TYPE_SHORTCUT:
    case ROW_TYPE_BOOKMARK:
      retval = test_if_file_is_visible (priv->fs, data, local_only, is_folder);
      break;
    case ROW_TYPE_VOLUME:
      {
	retval = TRUE;
	if (local_only)
	  {
	    if (_ctk_file_system_volume_is_mounted (data))
	      {
		GFile *base_file;

		base_file = _ctk_file_system_volume_get_root (data);

		if (base_file)
		  {
		    if (!_ctk_file_has_native_path (base_file))
		      retval = FALSE;
                    g_object_unref (base_file);
		  }
		else
		  retval = FALSE;
	      }
	  }
      }
      break;
    case ROW_TYPE_EMPTY_SELECTION:
      {
	gboolean popup_shown;

	g_object_get (priv->combo_box,
		      "popup-shown", &popup_shown,
		      NULL);

	if (popup_shown)
	  retval = FALSE;
	else
	  {
	    GFile *selected;

	    /* When the combo box is not popped up... */

	    selected = get_selected_file (button);
	    if (selected)
	      retval = FALSE; /* ... nonempty selection means the ROW_TYPE_EMPTY_SELECTION is *not* visible... */
	    else
	      retval = TRUE;  /* ... and empty selection means the ROW_TYPE_EMPTY_SELECTION *is* visible */

	    if (selected)
	      g_object_unref (selected);
	  }

	break;
      }
    default:
      retval = TRUE;
      break;
    }

  return retval;
}

/* Combo Box */
static void
name_cell_data_func (CtkCellLayout   *layout,
		     CtkCellRenderer *cell,
		     CtkTreeModel    *model,
		     CtkTreeIter     *iter,
		     gpointer         user_data)
{
  gchar type;

  type = 0;
  ctk_tree_model_get (model, iter,
		      TYPE_COLUMN, &type,
		      -1);

  if (type == ROW_TYPE_CURRENT_FOLDER)
    g_object_set (cell, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  else if (type == ROW_TYPE_BOOKMARK || type == ROW_TYPE_SHORTCUT)
    g_object_set (cell, "ellipsize", PANGO_ELLIPSIZE_MIDDLE, NULL);
  else
    g_object_set (cell, "ellipsize", PANGO_ELLIPSIZE_NONE, NULL);
}

static gboolean
combo_box_row_separator_func (CtkTreeModel *model,
			      CtkTreeIter  *iter,
			      gpointer      user_data)
{
  gchar type = ROW_TYPE_INVALID;

  ctk_tree_model_get (model, iter, TYPE_COLUMN, &type, -1);

  return (type == ROW_TYPE_BOOKMARK_SEPARATOR ||
	  type == ROW_TYPE_CURRENT_FOLDER_SEPARATOR ||
	  type == ROW_TYPE_OTHER_SEPARATOR);
}

static void
select_combo_box_row_no_notify (CtkFileChooserButton *button, int pos)
{
  CtkFileChooserButtonPrivate *priv = button->priv;
  CtkTreeIter iter, filter_iter;

  ctk_tree_model_iter_nth_child (priv->model, &iter, NULL, pos);
  ctk_tree_model_filter_convert_child_iter_to_iter (CTK_TREE_MODEL_FILTER (priv->filter_model),
						    &filter_iter, &iter);

  g_signal_handlers_block_by_func (priv->combo_box, combo_box_changed_cb, button);
  ctk_combo_box_set_active_iter (CTK_COMBO_BOX (priv->combo_box), &filter_iter);
  g_signal_handlers_unblock_by_func (priv->combo_box, combo_box_changed_cb, button);
}

static void
update_combo_box (CtkFileChooserButton *button)
{
  CtkFileChooserButtonPrivate *priv = button->priv;
  GFile *file;
  CtkTreeIter iter;
  gboolean row_found;

  file = get_selected_file (button);

  row_found = FALSE;

  ctk_tree_model_get_iter_first (priv->filter_model, &iter);

  do
    {
      gchar type;
      gpointer data;

      type = ROW_TYPE_INVALID;
      data = NULL;

      ctk_tree_model_get (priv->filter_model, &iter,
			  TYPE_COLUMN, &type,
			  DATA_COLUMN, &data,
			  -1);

      switch (type)
	{
	case ROW_TYPE_SPECIAL:
	case ROW_TYPE_SHORTCUT:
	case ROW_TYPE_BOOKMARK:
	case ROW_TYPE_CURRENT_FOLDER:
	  row_found = (file && g_file_equal (data, file));
	  break;
	case ROW_TYPE_VOLUME:
	  {
	    GFile *base_file;

	    base_file = _ctk_file_system_volume_get_root (data);
            if (base_file)
              {
	        row_found = (file && g_file_equal (base_file, file));
		g_object_unref (base_file);
              }
	  }
	  break;
	default:
	  row_found = FALSE;
	  break;
	}

      if (row_found)
	{
	  g_signal_handlers_block_by_func (priv->combo_box, combo_box_changed_cb, button);
	  ctk_combo_box_set_active_iter (CTK_COMBO_BOX (priv->combo_box),
					 &iter);
	  g_signal_handlers_unblock_by_func (priv->combo_box, combo_box_changed_cb, button);
	}
    }
  while (!row_found && ctk_tree_model_iter_next (priv->filter_model, &iter));

  if (!row_found)
    {
      gint pos;

      /* If it hasn't been found already, update & select the current-folder row. */
      if (file)
	{
	  model_update_current_folder (button, file);
	  pos = model_get_type_position (button, ROW_TYPE_CURRENT_FOLDER);
	}
      else
	{
	  /* No selection; switch to that row */

	  pos = model_get_type_position (button, ROW_TYPE_EMPTY_SELECTION);
	}

      ctk_tree_model_filter_refilter (CTK_TREE_MODEL_FILTER (priv->filter_model));

      select_combo_box_row_no_notify (button, pos);
    }

  if (file)
    g_object_unref (file);
}

/* Button */
static void
update_label_get_info_cb (GCancellable *cancellable,
			  GFileInfo    *info,
			  const GError *error,
			  gpointer      data)
{
  gboolean cancelled = g_cancellable_is_cancelled (cancellable);
  cairo_surface_t *surface;
  CtkFileChooserButton *button = data;
  CtkFileChooserButtonPrivate *priv = button->priv;

  if (cancellable != priv->update_button_cancellable)
    goto out;

  priv->update_button_cancellable = NULL;

  if (cancelled || error)
    goto out;

  ctk_label_set_text (CTK_LABEL (priv->label), g_file_info_get_display_name (info));

  surface = _ctk_file_info_render_icon (info, CTK_WIDGET (priv->image), priv->icon_size);
  ctk_image_set_from_surface (CTK_IMAGE (priv->image), surface);
  if (surface)
    cairo_surface_destroy (surface);

out:
  emit_selection_changed_if_changing_selection (button);

  g_object_unref (button);
  g_object_unref (cancellable);
}

static void
update_label_and_image (CtkFileChooserButton *button)
{
  CtkFileChooserButtonPrivate *priv = button->priv;
  gchar *label_text;
  GFile *file;
  gboolean done_changing_selection;

  file = get_selected_file (button);

  label_text = NULL;
  done_changing_selection = FALSE;

  if (priv->update_button_cancellable)
    {
      g_cancellable_cancel (priv->update_button_cancellable);
      priv->update_button_cancellable = NULL;
    }

  if (file)
    {
      CtkFileSystemVolume *volume = NULL;

      volume = _ctk_file_system_get_volume_for_file (priv->fs, file);
      if (volume)
        {
          GFile *base_file;

          base_file = _ctk_file_system_volume_get_root (volume);
          if (base_file && g_file_equal (base_file, file))
            {
              cairo_surface_t *surface;

              label_text = _ctk_file_system_volume_get_display_name (volume);
              surface = _ctk_file_system_volume_render_icon (volume,
							     CTK_WIDGET (button),
							     priv->icon_size,
							     NULL);
              ctk_image_set_from_surface (CTK_IMAGE (priv->image), surface);
              if (surface)
                cairo_surface_destroy (surface);
            }

          if (base_file)
            g_object_unref (base_file);

          _ctk_file_system_volume_unref (volume);

          if (label_text)
	    {
	      done_changing_selection = TRUE;
	      goto out;
	    }
        }

      if (g_file_is_native (file))
        {
          priv->update_button_cancellable =
            _ctk_file_system_get_info (priv->fs, file,
                                       "standard::icon,standard::display-name",
                                       update_label_get_info_cb,
                                       g_object_ref (button));
        }
      else
        {
          cairo_surface_t *surface;

          label_text = _ctk_bookmarks_manager_get_bookmark_label (button->priv->bookmarks_manager, file);
          surface = ctk_icon_theme_load_surface (get_icon_theme (CTK_WIDGET (priv->image)),
						 "text-x-generic",
						 priv->icon_size, 
						 ctk_widget_get_scale_factor (CTK_WIDGET (button)),
						 ctk_widget_get_window (CTK_WIDGET (button)),
						 0, NULL);
          ctk_image_set_from_surface (CTK_IMAGE (priv->image), surface);
          if (surface)
            cairo_surface_destroy (surface);

	  done_changing_selection = TRUE;
        }
    }
  else
    {
      /* We know the selection is empty */
      done_changing_selection = TRUE;
    }

out:

  if (file)
    g_object_unref (file);

  if (label_text)
    {
      ctk_label_set_text (CTK_LABEL (priv->label), label_text);
      g_free (label_text);
    }
  else
    {
      ctk_label_set_text (CTK_LABEL (priv->label), _(FALLBACK_DISPLAY_NAME));
      ctk_image_set_from_surface (CTK_IMAGE (priv->image), NULL);
    }

  if (done_changing_selection)
    emit_selection_changed_if_changing_selection (button);
}


/* ************************ *
 *  Child Object Callbacks  *
 * ************************ */

/* File System */
static void
fs_volumes_changed_cb (CtkFileSystem *fs,
		       gpointer       user_data)
{
  CtkFileChooserButton *button = CTK_FILE_CHOOSER_BUTTON (user_data);
  CtkFileChooserButtonPrivate *priv = button->priv;
  GSList *volumes;

  model_remove_rows (user_data,
		     model_get_type_position (user_data, ROW_TYPE_VOLUME),
		     priv->n_volumes);

  priv->n_volumes = 0;

  volumes = _ctk_file_system_list_volumes (fs);
  model_add_volumes (user_data, volumes);
  g_slist_free (volumes);

  ctk_tree_model_filter_refilter (CTK_TREE_MODEL_FILTER (priv->filter_model));

  update_label_and_image (user_data);
  update_combo_box (user_data);
}

static void
bookmarks_changed_cb (gpointer user_data)
{
  CtkFileChooserButton *button = CTK_FILE_CHOOSER_BUTTON (user_data);
  CtkFileChooserButtonPrivate *priv = button->priv;
  GSList *bookmarks;

  bookmarks = _ctk_bookmarks_manager_list_bookmarks (priv->bookmarks_manager);
  model_remove_rows (user_data,
		     model_get_type_position (user_data, ROW_TYPE_BOOKMARK_SEPARATOR),
		     priv->n_bookmarks + priv->has_bookmark_separator);
  priv->has_bookmark_separator = FALSE;
  priv->n_bookmarks = 0;
  model_add_bookmarks (user_data, bookmarks);
  g_slist_free_full (bookmarks, g_object_unref);

  ctk_tree_model_filter_refilter (CTK_TREE_MODEL_FILTER (priv->filter_model));

  update_label_and_image (user_data);
  update_combo_box (user_data);
}

static void
save_inactive_state (CtkFileChooserButton *button)
{
  CtkFileChooserButtonPrivate *priv = button->priv;

  if (priv->current_folder_while_inactive)
    g_object_unref (priv->current_folder_while_inactive);

  if (priv->selection_while_inactive)
    g_object_unref (priv->selection_while_inactive);

  priv->current_folder_while_inactive = ctk_file_chooser_get_current_folder_file (CTK_FILE_CHOOSER (priv->chooser));
  priv->selection_while_inactive = ctk_file_chooser_get_file (CTK_FILE_CHOOSER (priv->chooser));
}

static void
restore_inactive_state (CtkFileChooserButton *button)
{
  CtkFileChooserButtonPrivate *priv = button->priv;

  if (priv->current_folder_while_inactive)
    ctk_file_chooser_set_current_folder_file (CTK_FILE_CHOOSER (priv->chooser), priv->current_folder_while_inactive, NULL);

  if (priv->selection_while_inactive)
    ctk_file_chooser_select_file (CTK_FILE_CHOOSER (priv->chooser), priv->selection_while_inactive, NULL);
  else
    ctk_file_chooser_unselect_all (CTK_FILE_CHOOSER (priv->chooser));
}

/* Dialog */
static void
open_dialog (CtkFileChooserButton *button)
{
  CtkFileChooserButtonPrivate *priv = button->priv;
  CtkWidget *toplevel;

  toplevel = ctk_widget_get_toplevel (CTK_WIDGET (button));

  /* Setup the dialog parent to be chooser button's toplevel, and be modal
     as needed. */
  if (priv->dialog != NULL)
    {
      if (!ctk_widget_get_visible (priv->dialog))
        {
          if (ctk_widget_is_toplevel (toplevel) && CTK_IS_WINDOW (toplevel))
            {
              if (CTK_WINDOW (toplevel) != ctk_window_get_transient_for (CTK_WINDOW (priv->dialog)))
                ctk_window_set_transient_for (CTK_WINDOW (priv->dialog),
                                              CTK_WINDOW (toplevel));

              ctk_window_set_modal (CTK_WINDOW (priv->dialog),
                                    ctk_window_get_modal (CTK_WINDOW (toplevel)));
            }
        }
    }
  else
    {
      if (!ctk_native_dialog_get_visible (CTK_NATIVE_DIALOG (priv->native)))
        {
          if (ctk_widget_is_toplevel (toplevel) && CTK_IS_WINDOW (toplevel))
            {
              if (CTK_WINDOW (toplevel) != ctk_native_dialog_get_transient_for (CTK_NATIVE_DIALOG (priv->native)))
                ctk_native_dialog_set_transient_for (CTK_NATIVE_DIALOG (priv->native),
                                                     CTK_WINDOW (toplevel));

              ctk_native_dialog_set_modal (CTK_NATIVE_DIALOG (priv->native),
                                           ctk_window_get_modal (CTK_WINDOW (toplevel)));
            }
        }
    }

  if (!priv->active)
    {
      restore_inactive_state (button);
      priv->active = TRUE;

      /* Only handle update-preview handler if it is handled on the button */
      if (g_signal_has_handler_pending (button,
                                        g_signal_lookup ("update-preview", CTK_TYPE_FILE_CHOOSER),
                                        0, TRUE))
        {
          g_signal_connect (priv->chooser, "update-preview",
                            G_CALLBACK (chooser_update_preview_cb), button);
        }
    }

  ctk_widget_set_sensitive (priv->combo_box, FALSE);
  if (priv->dialog)
    {
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      ctk_window_present (CTK_WINDOW (priv->dialog));
      G_GNUC_END_IGNORE_DEPRECATIONS
    }
  else
    ctk_native_dialog_show (CTK_NATIVE_DIALOG (priv->native));
}

/* Combo Box */
static void
combo_box_changed_cb (CtkComboBox *combo_box,
		      gpointer     user_data)
{
  CtkFileChooserButton *button = CTK_FILE_CHOOSER_BUTTON (user_data);
  CtkFileChooserButtonPrivate *priv = button->priv;
  CtkTreeIter iter;
  gboolean file_was_set;

  file_was_set = FALSE;

  if (ctk_combo_box_get_active_iter (combo_box, &iter))
    {
      gchar type;
      gpointer data;

      type = ROW_TYPE_INVALID;
      data = NULL;

      ctk_tree_model_get (priv->filter_model, &iter,
			  TYPE_COLUMN, &type,
			  DATA_COLUMN, &data,
			  -1);

      switch (type)
	{
	case ROW_TYPE_SPECIAL:
	case ROW_TYPE_SHORTCUT:
	case ROW_TYPE_BOOKMARK:
	case ROW_TYPE_CURRENT_FOLDER:
	  if (data)
	    {
	      ctk_file_chooser_button_select_file (CTK_FILE_CHOOSER (button), data, NULL);
	      file_was_set = TRUE;
	    }
	  break;
	case ROW_TYPE_VOLUME:
	  {
	    GFile *base_file;

	    base_file = _ctk_file_system_volume_get_root (data);
	    if (base_file)
	      {
		ctk_file_chooser_button_select_file (CTK_FILE_CHOOSER (button), base_file, NULL);
		file_was_set = TRUE;
		g_object_unref (base_file);
	      }
	  }
	  break;
	case ROW_TYPE_OTHER:
	  open_dialog (user_data);
	  break;
	default:
	  break;
	}
    }

  if (file_was_set)
    g_signal_emit (button, file_chooser_button_signals[FILE_SET], 0);
}

/* Calback for the "notify::popup-shown" signal on the combo box.
 * When the combo is popped up, we don’t want the ROW_TYPE_EMPTY_SELECTION to be visible
 * at all; otherwise we would be showing a “(None)” item in the combo box’s popup.
 *
 * However, when the combo box is *not* popped up, we want the empty-selection row
 * to be visible depending on the selection.
 *
 * Since all that is done through the filter_model_visible_func(), this means
 * that we need to refilter the model when the combo box pops up - hence the
 * present signal handler.
 */
static void
combo_box_notify_popup_shown_cb (GObject    *object,
				 GParamSpec *pspec,
				 gpointer    user_data)
{
  CtkFileChooserButton *button = CTK_FILE_CHOOSER_BUTTON (user_data);
  CtkFileChooserButtonPrivate *priv = button->priv;
  gboolean popup_shown;

  g_object_get (priv->combo_box,
		"popup-shown", &popup_shown,
		NULL);

  /* Indicate that the ROW_TYPE_EMPTY_SELECTION will change visibility... */
  ctk_tree_model_filter_refilter (CTK_TREE_MODEL_FILTER (priv->filter_model));

  /* If the combo box popup got dismissed, go back to showing the ROW_TYPE_EMPTY_SELECTION if needed */
  if (!popup_shown)

    {
      GFile *selected = get_selected_file (button);

      if (!selected)
	{
	  int pos;

	  pos = model_get_type_position (button, ROW_TYPE_EMPTY_SELECTION);
	  select_combo_box_row_no_notify (button, pos);
	}
      else
	g_object_unref (selected);
    }
}

/* Button */
static void
button_clicked_cb (CtkButton *real_button,
		   gpointer   user_data)
{
  open_dialog (user_data);
}

/* Dialog */

static void
chooser_update_preview_cb (CtkFileChooser *dialog,
                           gpointer        user_data)
{
  g_signal_emit_by_name (user_data, "update-preview");
}

static void
chooser_notify_cb (GObject    *dialog,
                   GParamSpec *pspec,
                   gpointer    user_data)
{
  gpointer iface;

  iface = g_type_interface_peek (g_type_class_peek (G_OBJECT_TYPE (dialog)),
				 CTK_TYPE_FILE_CHOOSER);
  if (g_object_interface_find_property (iface, pspec->name))
    g_object_notify (user_data, pspec->name);

  if (g_ascii_strcasecmp (pspec->name, "local-only") == 0)
    {
      CtkFileChooserButton *button = CTK_FILE_CHOOSER_BUTTON (user_data);
      CtkFileChooserButtonPrivate *priv = button->priv;

      if (priv->has_current_folder)
	{
	  CtkTreeIter iter;
	  gint pos;
	  gpointer data;

	  pos = model_get_type_position (user_data,
					 ROW_TYPE_CURRENT_FOLDER);
	  ctk_tree_model_iter_nth_child (priv->model, &iter, NULL, pos);

	  data = NULL;
	  ctk_tree_model_get (priv->model, &iter, DATA_COLUMN, &data, -1);

	  /* If the path isn't local but we're in local-only mode now, remove
	   * the custom-folder row */
	  if (data && _ctk_file_has_native_path (G_FILE (data)) &&
	      ctk_file_chooser_get_local_only (CTK_FILE_CHOOSER (priv->chooser)))
	    {
	      pos--;
	      model_remove_rows (user_data, pos, 2);
	    }
	}

      ctk_tree_model_filter_refilter (CTK_TREE_MODEL_FILTER (priv->filter_model));
      update_combo_box (user_data);
    }
}

static gboolean
dialog_delete_event_cb (CtkWidget *dialog,
			GdkEvent  *event,
		        gpointer   user_data)
{
  g_signal_emit_by_name (dialog, "response", CTK_RESPONSE_DELETE_EVENT);

  return TRUE;
}

static void
common_response_cb (CtkFileChooserButton *button,
		    gint       response)
{
  CtkFileChooserButtonPrivate *priv = button->priv;

  if (response == CTK_RESPONSE_ACCEPT ||
      response == CTK_RESPONSE_OK)
    {
      save_inactive_state (button);

      g_signal_emit_by_name (button, "current-folder-changed");
      g_signal_emit_by_name (button, "selection-changed");
    }
  else
    {
      restore_inactive_state (button);
    }

  if (priv->active)
    {
      priv->active = FALSE;

      g_signal_handlers_disconnect_by_func (priv->chooser, chooser_update_preview_cb, button);
    }

  update_label_and_image (button);
  update_combo_box (button);

  ctk_widget_set_sensitive (priv->combo_box, TRUE);
}


static void
dialog_response_cb (CtkDialog *dialog,
		    gint       response,
		    gpointer   user_data)
{
  CtkFileChooserButton *button = CTK_FILE_CHOOSER_BUTTON (user_data);
  CtkFileChooserButtonPrivate *priv = button->priv;

  common_response_cb (button, response);

  ctk_widget_hide (priv->dialog);

  if (response == CTK_RESPONSE_ACCEPT ||
      response == CTK_RESPONSE_OK)
    g_signal_emit (button, file_chooser_button_signals[FILE_SET], 0);
}

static void
native_response_cb (CtkFileChooserNative *native,
		    gint       response,
		    gpointer   user_data)
{
  CtkFileChooserButton *button = CTK_FILE_CHOOSER_BUTTON (user_data);

  common_response_cb (button, response);

  /* dialog already hidden */

  if (response == CTK_RESPONSE_ACCEPT ||
      response == CTK_RESPONSE_OK)
    g_signal_emit (button, file_chooser_button_signals[FILE_SET], 0);
}


/* ************************************************************************** *
 *  Public API                                                                *
 * ************************************************************************** */

/**
 * ctk_file_chooser_button_new:
 * @title: the title of the browse dialog.
 * @action: the open mode for the widget.
 *
 * Creates a new file-selecting button widget.
 *
 * Returns: a new button widget.
 *
 * Since: 2.6
 */
CtkWidget *
ctk_file_chooser_button_new (const gchar          *title,
			     CtkFileChooserAction  action)
{
  g_return_val_if_fail (action == CTK_FILE_CHOOSER_ACTION_OPEN ||
			action == CTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, NULL);

  return g_object_new (CTK_TYPE_FILE_CHOOSER_BUTTON,
		       "action", action,
		       "title", (title ? title : _(DEFAULT_TITLE)),
		       NULL);
}

/**
 * ctk_file_chooser_button_new_with_dialog:
 * @dialog: (type Ctk.Dialog): the widget to use as dialog
 *
 * Creates a #CtkFileChooserButton widget which uses @dialog as its
 * file-picking window.
 *
 * Note that @dialog must be a #CtkDialog (or subclass) which
 * implements the #CtkFileChooser interface and must not have
 * %CTK_DIALOG_DESTROY_WITH_PARENT set.
 *
 * Also note that the dialog needs to have its confirmative button
 * added with response %CTK_RESPONSE_ACCEPT or %CTK_RESPONSE_OK in
 * order for the button to take over the file selected in the dialog.
 *
 * Returns: a new button widget.
 *
 * Since: 2.6
 */
CtkWidget *
ctk_file_chooser_button_new_with_dialog (CtkWidget *dialog)
{
  g_return_val_if_fail (CTK_IS_FILE_CHOOSER (dialog) && CTK_IS_DIALOG (dialog), NULL);

  return g_object_new (CTK_TYPE_FILE_CHOOSER_BUTTON,
		       "dialog", dialog,
		       NULL);
}

/**
 * ctk_file_chooser_button_set_title:
 * @button: the button widget to modify.
 * @title: the new browse dialog title.
 *
 * Modifies the @title of the browse dialog used by @button.
 *
 * Since: 2.6
 */
void
ctk_file_chooser_button_set_title (CtkFileChooserButton *button,
				   const gchar          *title)
{
  g_return_if_fail (CTK_IS_FILE_CHOOSER_BUTTON (button));

  if (button->priv->dialog)
    ctk_window_set_title (CTK_WINDOW (button->priv->dialog), title);
  else
    ctk_native_dialog_set_title (CTK_NATIVE_DIALOG (button->priv->native), title);
  g_object_notify (G_OBJECT (button), "title");
}

/**
 * ctk_file_chooser_button_get_title:
 * @button: the button widget to examine.
 *
 * Retrieves the title of the browse dialog used by @button. The returned value
 * should not be modified or freed.
 *
 * Returns: a pointer to the browse dialog’s title.
 *
 * Since: 2.6
 */
const gchar *
ctk_file_chooser_button_get_title (CtkFileChooserButton *button)
{
  g_return_val_if_fail (CTK_IS_FILE_CHOOSER_BUTTON (button), NULL);

  if (button->priv->dialog)
    return ctk_window_get_title (CTK_WINDOW (button->priv->dialog));
  else
    return ctk_native_dialog_get_title (CTK_NATIVE_DIALOG (button->priv->native));
}

/**
 * ctk_file_chooser_button_get_width_chars:
 * @button: the button widget to examine.
 *
 * Retrieves the width in characters of the @button widget’s entry and/or label.
 *
 * Returns: an integer width (in characters) that the button will use to size itself.
 *
 * Since: 2.6
 */
gint
ctk_file_chooser_button_get_width_chars (CtkFileChooserButton *button)
{
  g_return_val_if_fail (CTK_IS_FILE_CHOOSER_BUTTON (button), -1);

  return ctk_label_get_width_chars (CTK_LABEL (button->priv->label));
}

/**
 * ctk_file_chooser_button_set_width_chars:
 * @button: the button widget to examine.
 * @n_chars: the new width, in characters.
 *
 * Sets the width (in characters) that @button will use to @n_chars.
 *
 * Since: 2.6
 */
void
ctk_file_chooser_button_set_width_chars (CtkFileChooserButton *button,
					 gint                  n_chars)
{
  g_return_if_fail (CTK_IS_FILE_CHOOSER_BUTTON (button));

  ctk_label_set_width_chars (CTK_LABEL (button->priv->label), n_chars);
  g_object_notify (G_OBJECT (button), "width-chars");
}

/**
 * ctk_file_chooser_button_set_focus_on_click:
 * @button: a #CtkFileChooserButton
 * @focus_on_click: whether the button grabs focus when clicked with the mouse
 *
 * Sets whether the button will grab focus when it is clicked with the mouse.
 * Making mouse clicks not grab focus is useful in places like toolbars where
 * you don’t want the keyboard focus removed from the main area of the
 * application.
 *
 * Since: 2.10
 *
 * Deprecated: 3.20: Use ctk_widget_set_focus_on_click() instead
 */
void
ctk_file_chooser_button_set_focus_on_click (CtkFileChooserButton *button,
					    gboolean              focus_on_click)
{
  g_return_if_fail (CTK_IS_FILE_CHOOSER_BUTTON (button));

  ctk_widget_set_focus_on_click (CTK_WIDGET (button), focus_on_click);
}

/**
 * ctk_file_chooser_button_get_focus_on_click:
 * @button: a #CtkFileChooserButton
 *
 * Returns whether the button grabs focus when it is clicked with the mouse.
 * See ctk_file_chooser_button_set_focus_on_click().
 *
 * Returns: %TRUE if the button grabs focus when it is clicked with
 *               the mouse.
 *
 * Since: 2.10
 *
 * Deprecated: 3.20: Use ctk_widget_get_focus_on_click() instead
 */
gboolean
ctk_file_chooser_button_get_focus_on_click (CtkFileChooserButton *button)
{
  g_return_val_if_fail (CTK_IS_FILE_CHOOSER_BUTTON (button), FALSE);

  return ctk_widget_get_focus_on_click (CTK_WIDGET (button));
}
