#include "config.h"

#define CDK_DISABLE_DEPRECATION_WARNINGS
#undef CTK_DISABLE_DEPRECATED

#include <ctk/ctkunixprint.h>
#include <cdk/cdkkeysyms.h>
#include <X11/Xatom.h>
#include <cdkx.h>
#include "widgets.h"
#include "ctkgears.h"

#define SMALL_WIDTH  240
#define SMALL_HEIGHT 75
#define MEDIUM_WIDTH 240
#define MEDIUM_HEIGHT 165
#define LARGE_WIDTH 240
#define LARGE_HEIGHT 240

static WidgetInfo *
new_widget_info (const char *name,
		 CtkWidget  *widget,
		 WidgetSize  size)
{
  WidgetInfo *info;

  info = g_new0 (WidgetInfo, 1);
  info->name = g_strdup (name);
  info->size = size;
  if (CTK_IS_WINDOW (widget))
    {
      info->window = widget;
      ctk_window_set_resizable (CTK_WINDOW (info->window), FALSE);
      info->include_decorations = TRUE;
    }
  else
    {
      info->window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_container_set_border_width (CTK_CONTAINER (info->window), 12);
      info->include_decorations = FALSE;
      ctk_widget_show_all (widget);
      ctk_container_add (CTK_CONTAINER (info->window), widget);
    }
  info->no_focus = TRUE;

  g_signal_connect (info->window, "focus", G_CALLBACK (ctk_true), NULL);

  switch (size)
    {
    case SMALL:
      ctk_widget_set_size_request (info->window,
				   240, 75);
      break;
    case MEDIUM:
      ctk_widget_set_size_request (info->window,
				   240, 165);
      break;
    case LARGE:
      ctk_widget_set_size_request (info->window,
				   240, 240);
      break;
    default:
	break;
    }

  return info;
}

static WidgetInfo *
create_button (void)
{
  CtkWidget *widget;
  CtkWidget *align;

  widget = ctk_button_new_with_mnemonic ("_Button");
  align = ctk_alignment_new (0.5, 0.5, 0.0, 0.0);
  ctk_container_add (CTK_CONTAINER (align), widget);

  return new_widget_info ("button", align, SMALL);
}

static WidgetInfo *
create_switch (void)
{
  CtkWidget *widget;
  CtkWidget *align;
  CtkWidget *sw;

  widget = ctk_box_new (CTK_ORIENTATION_VERTICAL, 3);
  sw = ctk_switch_new ();
  ctk_switch_set_active (CTK_SWITCH (sw), TRUE);
  ctk_box_pack_start (CTK_BOX (widget), sw, TRUE, TRUE, 0);
  sw = ctk_switch_new ();
  ctk_box_pack_start (CTK_BOX (widget), sw, TRUE, TRUE, 0);

  align = ctk_alignment_new (0.5, 0.5, 0.0, 0.0);
  ctk_container_add (CTK_CONTAINER (align), widget);

  return new_widget_info ("switch", align, SMALL);
}

static WidgetInfo *
create_toggle_button (void)
{
  CtkWidget *widget;
  CtkWidget *align;

  widget = ctk_toggle_button_new_with_mnemonic ("_Toggle Button");
  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (widget), FALSE);
  align = ctk_alignment_new (0.5, 0.5, 0.0, 0.0);
  ctk_container_add (CTK_CONTAINER (align), widget);

  return new_widget_info ("toggle-button", align, SMALL);
}

static WidgetInfo *
create_check_button (void)
{
  CtkWidget *widget;
  CtkWidget *align;

  widget = ctk_check_button_new_with_mnemonic ("_Check Button");
  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (widget), TRUE);
  align = ctk_alignment_new (0.5, 0.5, 0.0, 0.0);
  ctk_container_add (CTK_CONTAINER (align), widget);

  return new_widget_info ("check-button", align, SMALL);
}

static WidgetInfo *
create_link_button (void)
{
  CtkWidget *widget;
  CtkWidget *align;

  widget = ctk_link_button_new_with_label ("http://www.ctk.org", "Link Button");
  align = ctk_alignment_new (0.5, 0.5, 0.0, 0.0);
  ctk_container_add (CTK_CONTAINER (align), widget);

  return new_widget_info ("link-button", align, SMALL);
}

static WidgetInfo *
create_menu_button (void)
{
  CtkWidget *widget;
  CtkWidget *image;
  CtkWidget *menu;
  CtkWidget *vbox;

  widget = ctk_menu_button_new ();
  image = ctk_image_new ();
  ctk_image_set_from_icon_name (CTK_IMAGE (image), "emblem-system-symbolic", CTK_ICON_SIZE_MENU);
  ctk_button_set_image (CTK_BUTTON (widget), image);
  menu = ctk_menu_new ();
  ctk_menu_button_set_popup (CTK_MENU_BUTTON (widget), menu);

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 3);
  ctk_box_pack_start (CTK_BOX (vbox), widget, FALSE, FALSE, 0);
  ctk_widget_set_halign (widget, CTK_ALIGN_CENTER);
  ctk_widget_set_valign (widget, CTK_ALIGN_CENTER);

  ctk_box_pack_start (CTK_BOX (vbox), ctk_label_new ("Menu Button"), TRUE, TRUE, 0);

  return new_widget_info ("menu-button", vbox, SMALL);
}

#define G_TYPE_TEST_PERMISSION      (g_test_permission_get_type ())
#define G_TEST_PERMISSION(inst)     (G_TYPE_CHECK_INSTANCE_CAST ((inst), \
                                     G_TYPE_TEST_PERMISSION,             \
                                     GTestPermission))
#define G_IS_TEST_PERMISSION(inst)  (G_TYPE_CHECK_INSTANCE_TYPE ((inst), \
                                     G_TYPE_TEST_PERMISSION))

typedef struct _GTestPermission GTestPermission;
typedef struct _GTestPermissionClass GTestPermissionClass;

struct _GTestPermission
{
  GPermission parent;

  gboolean success;
};

struct _GTestPermissionClass
{
  GPermissionClass parent_class;
};

G_DEFINE_TYPE (GTestPermission, g_test_permission, G_TYPE_PERMISSION)

static void
g_test_permission_init (GTestPermission *test)
{
  g_permission_impl_update (G_PERMISSION (test), FALSE, TRUE, TRUE);
}

static void
g_test_permission_class_init (GTestPermissionClass *class)
{
}

static WidgetInfo *
create_lockbutton (void)
{
  CtkWidget *vbox;
  CtkWidget *widget;

  widget = ctk_lock_button_new (g_object_new (G_TYPE_TEST_PERMISSION, NULL));

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 3);
  ctk_box_pack_start (CTK_BOX (vbox), widget, FALSE, FALSE, 0);
  ctk_box_pack_start (CTK_BOX (vbox),
		      ctk_label_new ("Lock Button"),
		      FALSE, FALSE, 0);
  ctk_widget_set_halign (vbox, CTK_ALIGN_CENTER);
  ctk_widget_set_valign (vbox, CTK_ALIGN_CENTER);

  return new_widget_info ("lock-button", vbox, SMALL);
}

static WidgetInfo *
create_entry (void)
{
  CtkWidget *widget;
  CtkWidget *align;

  widget = ctk_entry_new ();
  ctk_entry_set_text (CTK_ENTRY (widget), "Entry");
  ctk_editable_set_position (CTK_EDITABLE (widget), -1);
  align = ctk_alignment_new (0.5, 0.5, 1.0, 0.0);
  ctk_container_add (CTK_CONTAINER (align), widget);

  return  new_widget_info ("entry", align, SMALL);
}

static WidgetInfo *
create_search_entry (void)
{
  CtkWidget *widget;
  CtkWidget *align;

  widget = ctk_search_entry_new ();
  ctk_entry_set_placeholder_text (CTK_ENTRY (widget), "Search...");
  align = ctk_alignment_new (0.5, 0.5, 1.0, 0.0);
  ctk_container_add (CTK_CONTAINER (align), widget);

  return  new_widget_info ("search-entry", align, SMALL);
}

static WidgetInfo *
create_radio (void)
{
  CtkWidget *widget;
  CtkWidget *radio;
  CtkWidget *align;

  widget = ctk_box_new (CTK_ORIENTATION_VERTICAL, 3);
  radio = ctk_radio_button_new_with_mnemonic (NULL, "Radio Button _One");
  ctk_box_pack_start (CTK_BOX (widget), radio, FALSE, FALSE, 0);
  radio = ctk_radio_button_new_with_mnemonic_from_widget (CTK_RADIO_BUTTON (radio), "Radio Button _Two");
  ctk_box_pack_start (CTK_BOX (widget), radio, FALSE, FALSE, 0);
  radio = ctk_radio_button_new_with_mnemonic_from_widget (CTK_RADIO_BUTTON (radio), "Radio Button T_hree");
  ctk_box_pack_start (CTK_BOX (widget), radio, FALSE, FALSE, 0);
  align = ctk_alignment_new (0.5, 0.5, 0.0, 0.0);
  ctk_container_add (CTK_CONTAINER (align), widget);

  return new_widget_info ("radio-group", align, MEDIUM);
}

static WidgetInfo *
create_label (void)
{
  CtkWidget *widget;
  CtkWidget *align;

  widget = ctk_label_new ("Label");
  align = ctk_alignment_new (0.5, 0.5, 0.0, 0.0);
  ctk_container_add (CTK_CONTAINER (align), widget);

  return new_widget_info ("label", align, SMALL);
}

static WidgetInfo *
create_accel_label (void)
{
  WidgetInfo *info;
  CtkWidget *widget, *button, *box;
  CtkAccelGroup *accel_group;

  widget = ctk_accel_label_new ("Accel Label");

  button = ctk_button_new_with_label ("Quit");
  ctk_accel_label_set_accel_widget (CTK_ACCEL_LABEL (widget), button);
  ctk_widget_set_no_show_all (button, TRUE);

  box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  ctk_container_add (CTK_CONTAINER (box), widget);
  ctk_container_add (CTK_CONTAINER (box), button);

  ctk_accel_label_set_accel_widget (CTK_ACCEL_LABEL (widget), button);
  accel_group = ctk_accel_group_new();

  info = new_widget_info ("accel-label", box, SMALL);

  ctk_widget_add_accelerator (button, "activate", accel_group, CDK_KEY_Q, CDK_CONTROL_MASK,
			      CTK_ACCEL_VISIBLE | CTK_ACCEL_LOCKED);

  return info;
}

static WidgetInfo *
create_combo_box_entry (void)
{
  CtkWidget *widget;
  CtkWidget *align;
  CtkWidget *child;
  CtkTreeModel *model;

  model = (CtkTreeModel *)ctk_list_store_new (1, G_TYPE_STRING);
  widget = g_object_new (CTK_TYPE_COMBO_BOX,
			 "has-entry", TRUE,
			 "model", model,
			 "entry-text-column", 0,
			 NULL);
  g_object_unref (model);

  child = ctk_bin_get_child (CTK_BIN (widget));
  ctk_entry_set_text (CTK_ENTRY (child), "Combo Box Entry");
  align = ctk_alignment_new (0.5, 0.5, 0.0, 0.0);
  ctk_container_add (CTK_CONTAINER (align), widget);

  return new_widget_info ("combo-box-entry", align, SMALL);
}

static WidgetInfo *
create_combo_box (void)
{
  CtkWidget *widget;
  CtkWidget *align;
  CtkCellRenderer *cell;
  CtkListStore *store;

  widget = ctk_combo_box_new ();
  ctk_cell_layout_clear (CTK_CELL_LAYOUT (widget));
  cell = ctk_cell_renderer_text_new ();
  ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (widget), cell, FALSE);
  ctk_cell_layout_set_attributes (CTK_CELL_LAYOUT (widget), cell, "text", 0, NULL);

  store = ctk_list_store_new (1, G_TYPE_STRING);
  ctk_list_store_insert_with_values (store, NULL, -1, 0, "Combo Box", -1);
  ctk_combo_box_set_model (CTK_COMBO_BOX (widget), CTK_TREE_MODEL (store));

  ctk_combo_box_set_active (CTK_COMBO_BOX (widget), 0);
  align = ctk_alignment_new (0.5, 0.5, 0.0, 0.0);
  ctk_container_add (CTK_CONTAINER (align), widget);

  return new_widget_info ("combo-box", align, SMALL);
}

static WidgetInfo *
create_combo_box_text (void)
{
  CtkWidget *widget;
  CtkWidget *align;

  widget = ctk_combo_box_text_new ();

  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (widget), "Combo Box Text");
  ctk_combo_box_set_active (CTK_COMBO_BOX (widget), 0);
  align = ctk_alignment_new (0.5, 0.5, 0.0, 0.0);
  ctk_container_add (CTK_CONTAINER (align), widget);

  return new_widget_info ("combo-box-text", align, SMALL);
}

static WidgetInfo *
create_info_bar (void)
{
  CtkWidget *widget;
  CtkWidget *align;
  WidgetInfo *info;

  widget = ctk_info_bar_new ();
  ctk_info_bar_set_show_close_button (CTK_INFO_BAR (widget), TRUE);
  ctk_info_bar_set_message_type (CTK_INFO_BAR (widget), CTK_MESSAGE_INFO);
  ctk_container_add (CTK_CONTAINER (ctk_info_bar_get_content_area (CTK_INFO_BAR (widget))),
                     ctk_label_new ("Info Bar"));

  align = ctk_alignment_new (0.5, 0, 1.0, 0.0);
  ctk_container_add (CTK_CONTAINER (align), widget);

  info = new_widget_info ("info-bar", align, SMALL);
  ctk_container_set_border_width (CTK_CONTAINER (info->window), 0);

  return info;
}

static WidgetInfo *
create_search_bar (void)
{
  CtkWidget *widget;
  CtkWidget *entry;
  WidgetInfo *info;
  CtkWidget *view;
  CtkWidget *box;

  box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  widget = ctk_search_bar_new ();

  entry = ctk_search_entry_new ();
  ctk_entry_set_text (CTK_ENTRY (entry), "Search Bar");
  ctk_container_add (CTK_CONTAINER (widget), entry);
  ctk_widget_show (entry);

  ctk_search_bar_set_show_close_button (CTK_SEARCH_BAR (widget), TRUE);
  ctk_search_bar_set_search_mode (CTK_SEARCH_BAR (widget), TRUE);

  ctk_container_add (CTK_CONTAINER (box), widget);

  view = ctk_text_view_new ();
  ctk_widget_show (view);
  ctk_box_pack_start (CTK_BOX (box), view, TRUE, TRUE, 0);

  info = new_widget_info ("search-bar", box, SMALL);
  ctk_container_set_border_width (CTK_CONTAINER (info->window), 0);

  return info;
}

static WidgetInfo *
create_action_bar (void)
{
  CtkWidget *widget;
  CtkWidget *button;
  WidgetInfo *info;
  CtkWidget *view;
  CtkWidget *box;

  box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  view = ctk_text_view_new ();
  ctk_widget_show (view);
  ctk_box_pack_start (CTK_BOX (box), view, TRUE, TRUE, 0);

  widget = ctk_action_bar_new ();

  button = ctk_button_new_from_icon_name ("object-select-symbolic", CTK_ICON_SIZE_MENU);
  ctk_widget_show (button);
  ctk_container_add (CTK_CONTAINER (widget), button);
  button = ctk_button_new_from_icon_name ("call-start-symbolic", CTK_ICON_SIZE_MENU);
  ctk_widget_show (button);
  ctk_container_add (CTK_CONTAINER (widget), button);
  g_object_set (ctk_widget_get_parent (button), "margin", 6, "spacing", 6, NULL);

  ctk_widget_show (widget);

  ctk_container_add (CTK_CONTAINER (box), widget);

  info = new_widget_info ("action-bar", box, SMALL);
  ctk_container_set_border_width (CTK_CONTAINER (info->window), 0);

  return info;
}

static WidgetInfo *
create_recent_chooser_dialog (void)
{
  WidgetInfo *info;
  CtkWidget *widget;

  widget = ctk_recent_chooser_dialog_new ("Recent Chooser Dialog",
					  NULL,
					  "Cancel", CTK_RESPONSE_CANCEL,
					  "Open", CTK_RESPONSE_ACCEPT,
					  NULL); 
  ctk_window_set_default_size (CTK_WINDOW (widget), 505, 305);
  
  info = new_widget_info ("recentchooserdialog", widget, ASIS);
  info->include_decorations = TRUE;

  return info;
}

static WidgetInfo *
create_text_view (void)
{
  CtkWidget *widget;
  CtkWidget *text_view;

  widget = ctk_frame_new (NULL);
  ctk_frame_set_shadow_type (CTK_FRAME (widget), CTK_SHADOW_IN);
  text_view = ctk_text_view_new ();
  ctk_container_add (CTK_CONTAINER (widget), text_view);
  /* Bad hack to add some size to the widget */
  ctk_text_buffer_set_text (ctk_text_view_get_buffer (CTK_TEXT_VIEW (text_view)),
			    "Multiline\nText\n\n", -1);
  ctk_text_view_set_cursor_visible (CTK_TEXT_VIEW (text_view), FALSE);

  return new_widget_info ("multiline-text", widget, MEDIUM);
}

static WidgetInfo *
create_tree_view (void)
{
  CtkWidget *widget;
  CtkWidget *tree_view;
  CtkTreeStore *store;
  CtkTreeIter iter;
  WidgetInfo *info;

  widget = ctk_frame_new (NULL);
  ctk_frame_set_shadow_type (CTK_FRAME (widget), CTK_SHADOW_IN);
  store = ctk_tree_store_new (3, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_STRING);
  ctk_tree_store_append (store, &iter, NULL);
  ctk_tree_store_set (store, &iter, 0, "Line One", 1, FALSE, 2, "A", -1);
  ctk_tree_store_append (store, &iter, NULL);
  ctk_tree_store_set (store, &iter, 0, "Line Two", 1, TRUE, 2, "B", -1);
  ctk_tree_store_append (store, &iter, &iter);
  ctk_tree_store_set (store, &iter, 0, "Line Three", 1, FALSE, 2, "C", -1);

  tree_view = ctk_tree_view_new_with_model (CTK_TREE_MODEL (store));
  ctk_tree_view_set_enable_tree_lines (CTK_TREE_VIEW (tree_view), TRUE);
  ctk_tree_view_insert_column_with_attributes (CTK_TREE_VIEW (tree_view),
					       0, "List",
					       ctk_cell_renderer_text_new (),
					       "text", 0, NULL);
  ctk_tree_view_insert_column_with_attributes (CTK_TREE_VIEW (tree_view),
					       1, "and",
					       ctk_cell_renderer_toggle_new (),
                                               "active", 1, NULL);
  ctk_tree_view_insert_column_with_attributes (CTK_TREE_VIEW (tree_view),
					       2, "Tree",
					       g_object_new (CTK_TYPE_CELL_RENDERER_TEXT, "xalign", 0.5, NULL),
					       "text", 2, NULL);
  ctk_tree_view_expand_all (CTK_TREE_VIEW (tree_view));
  ctk_container_add (CTK_CONTAINER (widget), tree_view);

  info = new_widget_info ("list-and-tree", widget, MEDIUM);
  info->no_focus = FALSE;

  return info;
}

static WidgetInfo *
create_icon_view (void)
{
  CtkWidget *widget;
  CtkWidget *vbox;
  CtkWidget *align;
  CtkWidget *icon_view;
  CtkListStore *list_store;
  CtkTreeIter iter;
  CdkPixbuf *pixbuf;
  WidgetInfo *info;

  widget = ctk_frame_new (NULL);
  ctk_frame_set_shadow_type (CTK_FRAME (widget), CTK_SHADOW_IN);
  list_store = ctk_list_store_new (2, G_TYPE_STRING, CDK_TYPE_PIXBUF);
  ctk_list_store_append (list_store, &iter);
  pixbuf = cdk_pixbuf_new_from_file ("folder.png", NULL);
  ctk_list_store_set (list_store, &iter, 0, "One", 1, pixbuf, -1);
  ctk_list_store_append (list_store, &iter);
  pixbuf = cdk_pixbuf_new_from_file ("gnome.png", NULL);
  ctk_list_store_set (list_store, &iter, 0, "Two", 1, pixbuf, -1);

  icon_view = ctk_icon_view_new();

  ctk_icon_view_set_model (CTK_ICON_VIEW (icon_view), CTK_TREE_MODEL (list_store));
  ctk_icon_view_set_text_column (CTK_ICON_VIEW (icon_view), 0);
  ctk_icon_view_set_pixbuf_column (CTK_ICON_VIEW (icon_view), 1);

  ctk_container_add (CTK_CONTAINER (widget), icon_view);

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 3);
  align = ctk_alignment_new (0.5, 0.5, 1.0, 1.0);
  ctk_container_add (CTK_CONTAINER (align), widget);
  ctk_box_pack_start (CTK_BOX (vbox), align, TRUE, TRUE, 0);
  ctk_box_pack_start (CTK_BOX (vbox),
		      ctk_label_new ("Icon View"),
		      FALSE, FALSE, 0);

  info = new_widget_info ("icon-view", vbox, MEDIUM);
  info->no_focus = FALSE;

  return info;
}

static WidgetInfo *
create_color_button (void)
{
  CtkWidget *vbox;
  CtkWidget *picker;
  CtkWidget *align;
  CdkColor color;

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 3);
  align = ctk_alignment_new (0.5, 0.5, 0.0, 0.0);
  color.red = 0x1e<<8;  /* Go Gagne! */
  color.green = 0x90<<8;
  color.blue = 0xff<<8;
  picker = ctk_color_button_new_with_color (&color);
  ctk_container_add (CTK_CONTAINER (align), picker);
  ctk_box_pack_start (CTK_BOX (vbox), align, FALSE, FALSE, 0);
  ctk_box_pack_start (CTK_BOX (vbox),
		      ctk_label_new ("Color Button"),
		      FALSE, FALSE, 0);

  return new_widget_info ("color-button", vbox, SMALL);
}

static WidgetInfo *
create_font_button (void)
{
  CtkWidget *vbox;
  CtkWidget *picker;
  CtkWidget *align;

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 3);
  align = ctk_alignment_new (0.5, 0.5, 0.0, 0.0);
  picker = ctk_font_button_new_with_font ("Sans Serif 10");
  ctk_container_add (CTK_CONTAINER (align), picker);
  ctk_box_pack_start (CTK_BOX (vbox), align, FALSE, FALSE, 0);
  ctk_box_pack_start (CTK_BOX (vbox),
		      ctk_label_new ("Font Button"),
		      FALSE, FALSE, 0);

  return new_widget_info ("font-button", vbox, SMALL);
}

static WidgetInfo *
create_file_button (void)
{
  CtkWidget *vbox;
  CtkWidget *vbox2;
  CtkWidget *picker;
  CtkWidget *align;
  char *path;

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 12);
  vbox2 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 3);
  align = ctk_alignment_new (0.5, 0.5, 0.0, 0.0);
  picker = ctk_file_chooser_button_new ("File Chooser Button",
		  			CTK_FILE_CHOOSER_ACTION_OPEN);
  ctk_widget_set_size_request (picker, 150, -1);
  ctk_container_add (CTK_CONTAINER (align), picker);
  ctk_box_pack_start (CTK_BOX (vbox2), align, FALSE, FALSE, 0);
  ctk_box_pack_start (CTK_BOX (vbox2),
		      ctk_label_new ("File Button (Files)"),
		      FALSE, FALSE, 0);

  ctk_box_pack_start (CTK_BOX (vbox),
		      vbox2, TRUE, TRUE, 0);
  ctk_box_pack_start (CTK_BOX (vbox),
		      ctk_separator_new (CTK_ORIENTATION_HORIZONTAL),
		      FALSE, FALSE, 0);

  vbox2 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 3);
  align = ctk_alignment_new (0.5, 0.5, 0.0, 0.0);
  picker = ctk_file_chooser_button_new ("File Chooser Button",
		  			CTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
  ctk_widget_set_size_request (picker, 150, -1);
  path = g_build_filename (g_get_home_dir (), "Documents", NULL);
  ctk_file_chooser_set_filename (CTK_FILE_CHOOSER (picker), path);
  g_free (path);
  ctk_container_add (CTK_CONTAINER (align), picker);
  ctk_box_pack_start (CTK_BOX (vbox2), align, FALSE, FALSE, 0);
  ctk_box_pack_start (CTK_BOX (vbox2),
		      ctk_label_new ("File Button (Select Folder)"),
		      FALSE, FALSE, 0);
  ctk_box_pack_start (CTK_BOX (vbox),
		      vbox2, TRUE, TRUE, 0);

  return new_widget_info ("file-button", vbox, MEDIUM);
}

static WidgetInfo *
create_separator (void)
{
  CtkWidget *hbox;
  CtkWidget *vbox;

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 3);
  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
  ctk_box_set_homogeneous (CTK_BOX (hbox), TRUE);
  ctk_box_pack_start (CTK_BOX (hbox),
		      ctk_separator_new (CTK_ORIENTATION_HORIZONTAL),
		      TRUE, TRUE, 0);
  ctk_box_pack_start (CTK_BOX (hbox),
		      ctk_separator_new (CTK_ORIENTATION_VERTICAL),
		      TRUE, TRUE, 0);
  ctk_box_pack_start (CTK_BOX (vbox), hbox, TRUE, TRUE, 0);
  ctk_box_pack_start (CTK_BOX (vbox),
		      g_object_new (CTK_TYPE_LABEL,
				    "label", "Horizontal and Vertical\nSeparators",
				    "justify", CTK_JUSTIFY_CENTER,
				    NULL),
		      FALSE, FALSE, 0);
  return new_widget_info ("separator", vbox, MEDIUM);
}

static WidgetInfo *
create_panes (void)
{
  CtkWidget *hbox;
  CtkWidget *vbox;
  CtkWidget *pane;

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 3);
  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 12);
  ctk_box_set_homogeneous (CTK_BOX (hbox), TRUE);
  pane = ctk_paned_new (CTK_ORIENTATION_HORIZONTAL);
  ctk_paned_pack1 (CTK_PANED (pane),
		   g_object_new (CTK_TYPE_FRAME,
				 "shadow-type", CTK_SHADOW_IN,
				 NULL),
		   FALSE, FALSE);
  ctk_paned_pack2 (CTK_PANED (pane),
		   g_object_new (CTK_TYPE_FRAME,
				 "shadow-type", CTK_SHADOW_IN,
				 NULL),
		   FALSE, FALSE);
  ctk_box_pack_start (CTK_BOX (hbox),
		      pane,
		      TRUE, TRUE, 0);
  pane = ctk_paned_new (CTK_ORIENTATION_VERTICAL);
  ctk_paned_pack1 (CTK_PANED (pane),
		   g_object_new (CTK_TYPE_FRAME,
				 "shadow-type", CTK_SHADOW_IN,
				 NULL),
		   FALSE, FALSE);
  ctk_paned_pack2 (CTK_PANED (pane),
		   g_object_new (CTK_TYPE_FRAME,
				 "shadow-type", CTK_SHADOW_IN,
				 NULL),
		   FALSE, FALSE);
  ctk_box_pack_start (CTK_BOX (hbox),
		      pane,
		      TRUE, TRUE, 0);
  ctk_box_pack_start (CTK_BOX (vbox), hbox, TRUE, TRUE, 0);
  ctk_box_pack_start (CTK_BOX (vbox),
		      g_object_new (CTK_TYPE_LABEL,
				    "label", "Horizontal and Vertical\nPanes",
				    "justify", CTK_JUSTIFY_CENTER,
				    NULL),
		      FALSE, FALSE, 0);
  return new_widget_info ("panes", vbox, MEDIUM);
}

static WidgetInfo *
create_frame (void)
{
  CtkWidget *widget;

  widget = ctk_frame_new ("Frame");

  return new_widget_info ("frame", widget, MEDIUM);
}

static WidgetInfo *
create_window (void)
{
  WidgetInfo *info;
  CtkWidget *widget;

  widget = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  info = new_widget_info ("window", widget, MEDIUM);
  info->include_decorations = TRUE;
  ctk_window_set_title (CTK_WINDOW (info->window), "Window");

  return info;
}

static WidgetInfo *
create_filesel (void)
{
  WidgetInfo *info;
  CtkWidget *widget;

  widget = ctk_file_chooser_dialog_new ("File Chooser Dialog",
					NULL,
					CTK_FILE_CHOOSER_ACTION_OPEN,
					"Cancel", CTK_RESPONSE_CANCEL,
					"Open", CTK_RESPONSE_ACCEPT,
					NULL); 
  ctk_window_set_default_size (CTK_WINDOW (widget), 505, 305);
  
  info = new_widget_info ("filechooser", widget, ASIS);
  info->include_decorations = TRUE;

  return info;
}

static WidgetInfo *
create_print_dialog (void)
{
  WidgetInfo *info;
  CtkWidget *widget;

  widget = ctk_print_unix_dialog_new ("Print Dialog", NULL);   
  ctk_widget_set_size_request (widget, 505, 350);
  info = new_widget_info ("printdialog", widget, ASIS);
  info->include_decorations = TRUE;

  return info;
}

static WidgetInfo *
create_page_setup_dialog (void)
{
  WidgetInfo *info;
  CtkWidget *widget;
  CtkPageSetup *page_setup;
  CtkPrintSettings *settings;

  page_setup = ctk_page_setup_new ();
  settings = ctk_print_settings_new ();
  widget = ctk_page_setup_unix_dialog_new ("Page Setup Dialog", NULL);   
  ctk_page_setup_unix_dialog_set_page_setup (CTK_PAGE_SETUP_UNIX_DIALOG (widget),
					     page_setup);
  ctk_page_setup_unix_dialog_set_print_settings (CTK_PAGE_SETUP_UNIX_DIALOG (widget),
						 settings);

  info = new_widget_info ("pagesetupdialog", widget, ASIS);
  ctk_widget_set_app_paintable (info->window, FALSE);
  info->include_decorations = TRUE;

  return info;
}

static WidgetInfo *
create_toolbar (void)
{
  CtkWidget *widget;
  CtkToolItem *item;

  widget = ctk_toolbar_new ();

  item = ctk_tool_button_new (NULL, NULL);
  ctk_tool_button_set_icon_name (CTK_TOOL_BUTTON (item), "document-new");
  ctk_toolbar_insert (CTK_TOOLBAR (widget), item, -1);

  item = ctk_tool_button_new (NULL, NULL);
  ctk_tool_button_set_icon_name (CTK_TOOL_BUTTON (item), "document-open");
  ctk_toolbar_insert (CTK_TOOLBAR (widget), item, -1);

  item = ctk_tool_button_new (NULL, NULL);
  ctk_tool_button_set_icon_name (CTK_TOOL_BUTTON (item), "view-refresh");
  ctk_toolbar_insert (CTK_TOOLBAR (widget), item, -1);

  ctk_toolbar_set_show_arrow (CTK_TOOLBAR (widget), FALSE);

  return new_widget_info ("toolbar", widget, SMALL);
}

static WidgetInfo *
create_toolpalette (void)
{
  CtkWidget *widget, *group;
  CtkToolItem *item;

  widget = ctk_tool_palette_new ();
  group = ctk_tool_item_group_new ("Tools");
  ctk_container_add (CTK_CONTAINER (widget), group);
  item = ctk_tool_button_new (NULL, NULL);
  ctk_tool_button_set_icon_name (CTK_TOOL_BUTTON (item), "help-about");
  ctk_tool_item_group_insert (CTK_TOOL_ITEM_GROUP (group), item, -1);
  item = ctk_tool_button_new (NULL, NULL);
  ctk_tool_button_set_icon_name (CTK_TOOL_BUTTON (item), "document-new");
  ctk_tool_item_group_insert (CTK_TOOL_ITEM_GROUP (group), item, -1);
  item = ctk_tool_button_new (NULL, NULL);
  ctk_tool_button_set_icon_name (CTK_TOOL_BUTTON (item), "folder");
  ctk_tool_item_group_insert (CTK_TOOL_ITEM_GROUP (group), item, -1);

  group = ctk_tool_item_group_new ("More tools");
  ctk_container_add (CTK_CONTAINER (widget), group);
  item = ctk_tool_button_new (NULL, NULL);
  ctk_tool_button_set_icon_name (CTK_TOOL_BUTTON (item), "edit-cut");
  ctk_tool_item_group_insert (CTK_TOOL_ITEM_GROUP (group), item, -1);
  item = ctk_tool_button_new (NULL, NULL);
  ctk_tool_button_set_icon_name (CTK_TOOL_BUTTON (item), "edit-find");
  ctk_tool_item_group_insert (CTK_TOOL_ITEM_GROUP (group), item, -1);
  item = ctk_tool_button_new (NULL, NULL);
  ctk_tool_button_set_icon_name (CTK_TOOL_BUTTON (item), "document-properties");
  ctk_tool_item_group_insert (CTK_TOOL_ITEM_GROUP (group), item, -1);

  return new_widget_info ("toolpalette", widget, MEDIUM);
}

static WidgetInfo *
create_menubar (void)
{
  CtkWidget *widget, *vbox, *align, *item;

  widget = ctk_menu_bar_new ();

  item = ctk_menu_item_new_with_mnemonic ("_File");
  ctk_menu_shell_append (CTK_MENU_SHELL (widget), item);

  item = ctk_menu_item_new_with_mnemonic ("_Edit");
  ctk_menu_shell_append (CTK_MENU_SHELL (widget), item);

  item = ctk_menu_item_new_with_mnemonic ("_Help");
  ctk_menu_shell_append (CTK_MENU_SHELL (widget), item);

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 3);
  align = ctk_alignment_new (0.5, 0.5, 1.0, 0.0);
  ctk_container_add (CTK_CONTAINER (align), widget);
  ctk_box_pack_start (CTK_BOX (vbox), align, FALSE, FALSE, 0);
  ctk_box_pack_start (CTK_BOX (vbox),
		      ctk_label_new ("Menu Bar"),
		      FALSE, FALSE, 0);

  return new_widget_info ("menubar", vbox, SMALL);
}

static WidgetInfo *
create_message_dialog (void)
{
  CtkWidget *widget;

  widget = ctk_message_dialog_new (NULL,
				   0,
				   CTK_MESSAGE_INFO,
				   CTK_BUTTONS_OK,
				   NULL);
  ctk_window_set_icon_name (CTK_WINDOW (widget), "edit-copy");
  ctk_message_dialog_set_markup (CTK_MESSAGE_DIALOG (widget), "Message Dialog");
  ctk_message_dialog_format_secondary_text (CTK_MESSAGE_DIALOG (widget), "%s", "With secondary text");
  return new_widget_info ("messagedialog", widget, ASIS);
}

static WidgetInfo *
create_about_dialog (void)
{
  CtkWidget *widget;
  const gchar *authors[] = {
    "Peter Mattis",
    "Spencer Kimball",
    "Josh MacDonald",
    "and many more...",
    NULL
  };

  widget = ctk_about_dialog_new ();
  g_object_set (widget,
                "program-name", "CTK+ Code Demos",
                "version", PACKAGE_VERSION,
                "copyright", "© 1997-2013 The CTK+ Team",
                "website", "http://www.ctk.org",
                "comments", "Program to demonstrate CTK+ functions.",
                "logo-icon-name", "help-about",
                "title", "About CTK+ Code Demos",
                "authors", authors,
		NULL);
  ctk_window_set_icon_name (CTK_WINDOW (widget), "help-about");
  return new_widget_info ("aboutdialog", widget, ASIS);
}

static WidgetInfo *
create_notebook (void)
{
  CtkWidget *widget;

  widget = ctk_notebook_new ();

  ctk_notebook_append_page (CTK_NOTEBOOK (widget), 
			    ctk_label_new ("Notebook"),
			    NULL);
  ctk_notebook_append_page (CTK_NOTEBOOK (widget), ctk_event_box_new (), NULL);
  ctk_notebook_append_page (CTK_NOTEBOOK (widget), ctk_event_box_new (), NULL);

  return new_widget_info ("notebook", widget, MEDIUM);
}

static WidgetInfo *
create_progressbar (void)
{
  CtkWidget *vbox;
  CtkWidget *widget;
  CtkWidget *align;

  widget = ctk_progress_bar_new ();
  ctk_progress_bar_set_fraction (CTK_PROGRESS_BAR (widget), 0.5);

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 3);
  align = ctk_alignment_new (0.5, 0.5, 1.0, 0.0);
  ctk_container_add (CTK_CONTAINER (align), widget);
  ctk_box_pack_start (CTK_BOX (vbox), align, FALSE, FALSE, 0);
  ctk_box_pack_start (CTK_BOX (vbox),
		      ctk_label_new ("Progress Bar"),
		      FALSE, FALSE, 0);

  return new_widget_info ("progressbar", vbox, SMALL);
}

static WidgetInfo *
create_level_bar (void)
{
  CtkWidget *vbox;
  CtkWidget *widget;

  widget = ctk_level_bar_new ();
  ctk_level_bar_set_value (CTK_LEVEL_BAR (widget), 0.333);

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 3);
  ctk_box_pack_start (CTK_BOX (vbox), widget, TRUE, FALSE, 0);
  ctk_box_pack_start (CTK_BOX (vbox),
		      ctk_label_new ("Level Bar"),
		      FALSE, FALSE, 0);

  return new_widget_info ("levelbar", vbox, SMALL);
}

static WidgetInfo *
create_scrolledwindow (void)
{
  CtkWidget *scrolledwin, *label;

  scrolledwin = ctk_scrolled_window_new (NULL, NULL);
  ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (scrolledwin),
                                  CTK_POLICY_NEVER, CTK_POLICY_ALWAYS);
  label = ctk_label_new ("Scrolled Window");

  ctk_container_add (CTK_CONTAINER (scrolledwin), label);

  return new_widget_info ("scrolledwindow", scrolledwin, MEDIUM);
}

static WidgetInfo *
create_scrollbar (void)
{
  CtkWidget *widget;
  CtkWidget *vbox, *align;

  widget = ctk_scrollbar_new (CTK_ORIENTATION_HORIZONTAL, NULL);
  ctk_widget_set_size_request (widget, 100, -1);

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 3);
  align = ctk_alignment_new (0.5, 0.5, 1.0, 0.0);
  ctk_container_add (CTK_CONTAINER (align), widget);
  ctk_box_pack_start (CTK_BOX (vbox), align, FALSE, FALSE, 0);
  ctk_box_pack_start (CTK_BOX (vbox),
		      ctk_label_new ("Scrollbar"),
		      FALSE, FALSE, 0);

  return new_widget_info ("scrollbar", vbox, SMALL);
}

static WidgetInfo *
create_spinbutton (void)
{
  CtkWidget *widget;
  CtkWidget *vbox, *align;

  widget = ctk_spin_button_new_with_range (0.0, 100.0, 1.0);

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 3);
  align = ctk_alignment_new (0.5, 0.5, 0.0, 0.0);
  ctk_container_add (CTK_CONTAINER (align), widget);
  ctk_box_pack_start (CTK_BOX (vbox), align, FALSE, FALSE, 0);
  ctk_box_pack_start (CTK_BOX (vbox),
		      ctk_label_new ("Spin Button"),
		      FALSE, FALSE, 0);

  return new_widget_info ("spinbutton", vbox, SMALL);
}

static WidgetInfo *
create_statusbar (void)
{
  WidgetInfo *info;
  CtkWidget *widget;
  CtkWidget *vbox, *align;

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  align = ctk_alignment_new (0.5, 0.5, 0.0, 0.0);
  ctk_container_add (CTK_CONTAINER (align), ctk_label_new ("Status Bar"));
  ctk_box_pack_start (CTK_BOX (vbox),
		      align,
		      FALSE, FALSE, 0);
  widget = ctk_statusbar_new ();
  align = ctk_alignment_new (0.5, 1.0, 1.0, 0.0);
  ctk_container_add (CTK_CONTAINER (align), widget);
  ctk_statusbar_push (CTK_STATUSBAR (widget), 0, "Hold on...");

  ctk_box_pack_end (CTK_BOX (vbox), align, FALSE, FALSE, 0);

  info = new_widget_info ("statusbar", vbox, SMALL);
  ctk_container_set_border_width (CTK_CONTAINER (info->window), 0);

  return info;
}

static WidgetInfo *
create_scales (void)
{
  CtkWidget *hbox;
  CtkWidget *vbox;

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 3);
  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
  ctk_box_set_homogeneous (CTK_BOX (hbox), TRUE);
  ctk_box_pack_start (CTK_BOX (hbox),
		      ctk_scale_new_with_range (CTK_ORIENTATION_HORIZONTAL,
                                                0.0, 100.0, 1.0),
		      TRUE, TRUE, 0);
  ctk_box_pack_start (CTK_BOX (hbox),
		      ctk_scale_new_with_range (CTK_ORIENTATION_VERTICAL,
                                                0.0, 100.0, 1.0),
		      TRUE, TRUE, 0);
  ctk_box_pack_start (CTK_BOX (vbox), hbox, TRUE, TRUE, 0);
  ctk_box_pack_start (CTK_BOX (vbox),
		      g_object_new (CTK_TYPE_LABEL,
				    "label", "Horizontal and Vertical\nScales",
				    "justify", CTK_JUSTIFY_CENTER,
				    NULL),
		      FALSE, FALSE, 0);
  return new_widget_info ("scales", vbox, MEDIUM);}

static WidgetInfo *
create_image (void)
{
  CtkWidget *widget;
  CtkWidget *align, *vbox;

  widget = ctk_image_new_from_icon_name ("applications-graphics",
                                         CTK_ICON_SIZE_DIALOG);

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 3);
  align = ctk_alignment_new (0.5, 0.5, 0.0, 0.0);
  ctk_container_add (CTK_CONTAINER (align), widget);
  ctk_box_pack_start (CTK_BOX (vbox), align, FALSE, FALSE, 0);
  ctk_box_pack_start (CTK_BOX (vbox),
		      ctk_label_new ("Image"),
		      FALSE, FALSE, 0);

  return new_widget_info ("image", vbox, SMALL);
}

static WidgetInfo *
create_spinner (void)
{
  CtkWidget *widget;
  CtkWidget *align, *vbox;

  widget = ctk_spinner_new ();
  ctk_widget_set_size_request (widget, 24, 24);
  ctk_spinner_start (CTK_SPINNER (widget));

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 3);
  align = ctk_alignment_new (0.5, 0.5, 0.0, 0.0);
  ctk_container_add (CTK_CONTAINER (align), widget);
  ctk_box_pack_start (CTK_BOX (vbox), align, FALSE, FALSE, 0);
  ctk_box_pack_start (CTK_BOX (vbox),
		      ctk_label_new ("Spinner"),
		      FALSE, FALSE, 0);

  return new_widget_info ("spinner", vbox, SMALL);
}

static WidgetInfo *
create_volume_button (void)
{
  CtkWidget *button, *box;
  CtkWidget *widget;
  CtkWidget *popup;

  widget = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_widget_set_size_request (widget, 100, 250);

  box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  ctk_container_add (CTK_CONTAINER (widget), box);

  button = ctk_volume_button_new ();
  ctk_box_pack_end (CTK_BOX (box), button, FALSE, FALSE, 0);

  ctk_scale_button_set_value (CTK_SCALE_BUTTON (button), 33);
  popup = ctk_scale_button_get_popup (CTK_SCALE_BUTTON (button));
  ctk_widget_realize (widget);
  ctk_widget_show (box);
  ctk_widget_show (popup);

  return new_widget_info ("volumebutton", widget, ASIS);
}

static WidgetInfo *
create_assistant (void)
{
  CtkWidget *widget;
  CtkWidget *page1, *page2;
  WidgetInfo *info;

  widget = ctk_assistant_new ();
  ctk_window_set_title (CTK_WINDOW (widget), "Assistant");

  page1 = ctk_label_new ("Assistant");
  ctk_widget_show (page1);
  ctk_widget_set_size_request (page1, 300, 140);
  ctk_assistant_prepend_page (CTK_ASSISTANT (widget), page1);
  ctk_assistant_set_page_title (CTK_ASSISTANT (widget), page1, "Assistant page");
  ctk_assistant_set_page_complete (CTK_ASSISTANT (widget), page1, TRUE);

  page2 = ctk_label_new (NULL);
  ctk_widget_show (page2);
  ctk_assistant_append_page (CTK_ASSISTANT (widget), page2);
  ctk_assistant_set_page_type (CTK_ASSISTANT (widget), page2, CTK_ASSISTANT_PAGE_CONFIRM);

  info = new_widget_info ("assistant", widget, ASIS);
  info->include_decorations = TRUE;

  return info;
}

static WidgetInfo *
create_appchooserbutton (void)
{
  CtkWidget *picker;
  CtkWidget *align, *vbox;

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 3);
  align = ctk_alignment_new (0.5, 0.5, 0.0, 0.0);
  picker = ctk_app_chooser_button_new ("text/plain");
  ctk_container_add (CTK_CONTAINER (align), picker);
  ctk_box_pack_start (CTK_BOX (vbox), align, FALSE, FALSE, 0);
  ctk_box_pack_start (CTK_BOX (vbox),
                      ctk_label_new ("Application Button"),
                      FALSE, FALSE, 0);

  return new_widget_info ("appchooserbutton", vbox, SMALL);
}

static WidgetInfo *
create_appchooserdialog (void)
{
  WidgetInfo *info;
  CtkWidget *widget;

  widget = ctk_app_chooser_dialog_new_for_content_type (NULL, 0, "image/png");
  ctk_window_set_default_size (CTK_WINDOW (widget), 200, 300);

  info = new_widget_info ("appchooserdialog", widget, ASIS);
  info->include_decorations = TRUE;

  return info;
}

static WidgetInfo *
create_fontchooserdialog (void)
{
  WidgetInfo *info;
  CtkWidget *widget;

  widget = ctk_font_chooser_dialog_new ("Font Chooser Dialog", NULL);
  ctk_window_set_default_size (CTK_WINDOW (widget), 200, 300);
  info = new_widget_info ("fontchooser", widget, ASIS);
  info->include_decorations = TRUE;

  return info;
}

static WidgetInfo *
create_colorchooserdialog (void)
{
  WidgetInfo *info;
  CtkWidget *widget;

  widget = ctk_color_chooser_dialog_new ("Color Chooser Dialog", NULL);
  info = new_widget_info ("colorchooser", widget, ASIS);
  info->include_decorations = TRUE;

  return info;
}

static WidgetInfo *
create_headerbar (void)
{
  CtkWidget *window;
  CtkWidget *bar;
  CtkWidget *view;
  CtkWidget *button;

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_container_set_border_width (CTK_CONTAINER (window), 0);
  view = ctk_text_view_new ();
  ctk_widget_show (view);
  ctk_widget_set_size_request (window, 220, 150);
  ctk_container_add (CTK_CONTAINER (window), view);
  bar = ctk_header_bar_new ();
  ctk_header_bar_set_title (CTK_HEADER_BAR (bar), "Header Bar");
  ctk_header_bar_set_subtitle (CTK_HEADER_BAR (bar), "(subtitle)");
  ctk_window_set_titlebar (CTK_WINDOW (window), bar);
  button = ctk_button_new ();
  ctk_container_add (CTK_CONTAINER (button), ctk_image_new_from_icon_name ("bookmark-new-symbolic", CTK_ICON_SIZE_BUTTON));
  ctk_header_bar_pack_end (CTK_HEADER_BAR (bar), button);
  ctk_widget_show_all (bar);

  return new_widget_info ("headerbar", window, ASIS);
}

static WidgetInfo *
create_placessidebar (void)
{
  CtkWidget *bar;
  CtkWidget *vbox;
  CtkWidget *align;

  bar = ctk_places_sidebar_new ();
  ctk_widget_set_size_request (bar, 150, 300);
  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 3);
  align = ctk_alignment_new (0.5, 0.5, 0.0, 0.0);

  ctk_container_add (CTK_CONTAINER (align), bar);
  ctk_box_pack_start (CTK_BOX (vbox), align, FALSE, FALSE, 0);
  ctk_box_pack_start (CTK_BOX (vbox),
                      ctk_label_new ("Places Sidebar"),
                      FALSE, FALSE, 0);

  return new_widget_info ("placessidebar", vbox, ASIS);
}

static WidgetInfo *
create_stack (void)
{
  CtkWidget *stack;
  CtkWidget *switcher;
  CtkWidget *vbox;
  CtkWidget *view;

  stack = ctk_stack_new ();
  ctk_widget_set_margin_top (stack, 10);
  ctk_widget_set_margin_bottom (stack, 10);
  ctk_widget_set_size_request (stack, 120, 120);
  view = ctk_text_view_new ();
  ctk_widget_show (view);
  ctk_stack_add_titled (CTK_STACK (stack), view, "page1", "Page 1");
  view = ctk_text_view_new ();
  ctk_widget_show (view);
  ctk_stack_add_titled (CTK_STACK (stack), view, "page2", "Page 2");

  switcher = ctk_stack_switcher_new ();
  ctk_stack_switcher_set_stack (CTK_STACK_SWITCHER (switcher), CTK_STACK (stack));
  ctk_widget_set_halign (switcher, CTK_ALIGN_CENTER);

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);

  ctk_box_pack_start (CTK_BOX (vbox), switcher, FALSE, FALSE, 0);
  ctk_box_pack_start (CTK_BOX (vbox), stack, FALSE, FALSE, 0);
  ctk_box_pack_start (CTK_BOX (vbox),
                      ctk_label_new ("Stack"),
                      FALSE, FALSE, 0);

  return new_widget_info ("stack", vbox, ASIS);
}

static WidgetInfo *
create_stack_switcher (void)
{
  CtkWidget *stack;
  CtkWidget *switcher;
  CtkWidget *vbox;
  CtkWidget *view;

  stack = ctk_stack_new ();
  ctk_widget_set_margin_top (stack, 10);
  ctk_widget_set_margin_bottom (stack, 10);
  ctk_widget_set_size_request (stack, 120, 120);
  view = ctk_text_view_new ();
  ctk_widget_show (view);
  ctk_stack_add_titled (CTK_STACK (stack), view, "page1", "Page 1");
  view = ctk_text_view_new ();
  ctk_widget_show (view);
  ctk_stack_add_titled (CTK_STACK (stack), view, "page2", "Page 2");

  switcher = ctk_stack_switcher_new ();
  ctk_stack_switcher_set_stack (CTK_STACK_SWITCHER (switcher), CTK_STACK (stack));
  ctk_widget_set_halign (switcher, CTK_ALIGN_CENTER);

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);

  ctk_box_pack_start (CTK_BOX (vbox), switcher, FALSE, FALSE, 0);
  ctk_box_pack_start (CTK_BOX (vbox), stack, FALSE, FALSE, 0);
  ctk_box_pack_start (CTK_BOX (vbox),
                      ctk_label_new ("Stack Switcher"),
                      FALSE, FALSE, 0);

  return new_widget_info ("stackswitcher", vbox, ASIS);
}

static WidgetInfo *
create_sidebar (void)
{
  CtkWidget *stack;
  CtkWidget *sidebar;
  CtkWidget *hbox;
  CtkWidget *view;
  CtkWidget *frame;

  stack = ctk_stack_new ();
  ctk_widget_set_size_request (stack, 120, 120);
  view = ctk_label_new ("Sidebar");
  ctk_style_context_add_class (ctk_widget_get_style_context (view), "view");
  ctk_widget_set_halign (view, CTK_ALIGN_FILL);
  ctk_widget_set_valign (view, CTK_ALIGN_FILL);
  ctk_widget_show (view);
  ctk_stack_add_titled (CTK_STACK (stack), view, "page1", "Page 1");
  view = ctk_text_view_new ();
  ctk_widget_show (view);
  ctk_stack_add_titled (CTK_STACK (stack), view, "page2", "Page 2");

  sidebar = ctk_stack_sidebar_new ();
  ctk_stack_sidebar_set_stack (CTK_STACK_SIDEBAR (sidebar), CTK_STACK (stack));

  frame = ctk_frame_new (NULL);
  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);

  ctk_box_pack_start (CTK_BOX (hbox), sidebar, FALSE, FALSE, 0);
  ctk_box_pack_start (CTK_BOX (hbox), ctk_separator_new (CTK_ORIENTATION_VERTICAL), FALSE, FALSE, 0);
  ctk_box_pack_start (CTK_BOX (hbox), stack, TRUE, TRUE, 0);
  ctk_container_add (CTK_CONTAINER (frame), hbox);

  return new_widget_info ("sidebar", frame, ASIS);
}

static WidgetInfo *
create_list_box (void)
{
  CtkWidget *widget;
  CtkWidget *list;
  CtkWidget *row;
  CtkWidget *button;
  WidgetInfo *info;

  widget = ctk_frame_new (NULL);
  ctk_frame_set_shadow_type (CTK_FRAME (widget), CTK_SHADOW_IN);

  list = ctk_list_box_new ();
  ctk_list_box_set_selection_mode (CTK_LIST_BOX (list), CTK_SELECTION_BROWSE);
  row = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 10);
  button = ctk_label_new ("List Box");
  ctk_widget_set_hexpand (button, TRUE);
  ctk_widget_set_halign (button, CTK_ALIGN_CENTER);
  ctk_container_add (CTK_CONTAINER (row), button);
  ctk_container_add (CTK_CONTAINER (list), row);
  row = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 10);
  ctk_container_add (CTK_CONTAINER (row), ctk_label_new ("Line One"));
  button = ctk_check_button_new ();
  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (button), TRUE);
  ctk_widget_set_hexpand (button, TRUE);
  ctk_widget_set_halign (button, CTK_ALIGN_END);
  ctk_container_add (CTK_CONTAINER (row), button);
  ctk_container_add (CTK_CONTAINER (list), row);
  ctk_list_box_select_row (CTK_LIST_BOX (list), CTK_LIST_BOX_ROW (ctk_widget_get_parent (row)));
  row = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 10);
  ctk_container_add (CTK_CONTAINER (row), ctk_label_new ("Line Two"));
  button = ctk_button_new_with_label ("2");
  ctk_widget_set_hexpand (button, TRUE);
  ctk_widget_set_halign (button, CTK_ALIGN_END);
  ctk_container_add (CTK_CONTAINER (row), button);
  ctk_container_add (CTK_CONTAINER (list), row);
  row = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 10);
  ctk_container_add (CTK_CONTAINER (row), ctk_label_new ("Line Three"));
  button = ctk_entry_new ();
  ctk_widget_set_hexpand (button, TRUE);
  ctk_widget_set_halign (button, CTK_ALIGN_END);
  ctk_container_add (CTK_CONTAINER (row), button);
  ctk_container_add (CTK_CONTAINER (list), row);

  ctk_container_add (CTK_CONTAINER (widget), list);

  info = new_widget_info ("list-box", widget, MEDIUM);
  info->no_focus = FALSE;

  return info;
}

static WidgetInfo *
create_flow_box (void)
{
  CtkWidget *widget;
  CtkWidget *box;
  CtkWidget *vbox;
  CtkWidget *child;
  CtkWidget *button;
  WidgetInfo *info;

  widget = ctk_frame_new (NULL);
  ctk_frame_set_shadow_type (CTK_FRAME (widget), CTK_SHADOW_IN);

  box = ctk_flow_box_new ();
  ctk_flow_box_set_min_children_per_line (CTK_FLOW_BOX (box), 2);
  ctk_flow_box_set_max_children_per_line (CTK_FLOW_BOX (box), 2);
  ctk_flow_box_set_selection_mode (CTK_FLOW_BOX (box), CTK_SELECTION_BROWSE);
  button = ctk_label_new ("Child One");
  ctk_container_add (CTK_CONTAINER (box), button);
  button = ctk_button_new_with_label ("Child Two");
  ctk_container_add (CTK_CONTAINER (box), button);
  child = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 6);
  ctk_container_add (CTK_CONTAINER (child), ctk_label_new ("Child Three"));
  button = ctk_check_button_new ();
  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (button), TRUE);
  ctk_container_add (CTK_CONTAINER (child), button);
  ctk_container_add (CTK_CONTAINER (box), child);
  ctk_flow_box_select_child (CTK_FLOW_BOX (box),
                             CTK_FLOW_BOX_CHILD (ctk_widget_get_parent (child)));

  ctk_container_add (CTK_CONTAINER (widget), box);

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);

  ctk_box_pack_start (CTK_BOX (vbox), widget, FALSE, FALSE, 0);
  ctk_box_pack_start (CTK_BOX (vbox), ctk_label_new ("Flow Box"),
                      FALSE, FALSE, 0);
  info = new_widget_info ("flow-box", vbox, ASIS);
  info->no_focus = FALSE;

  return info;
}

static WidgetInfo *
create_gl_area (void)
{
  WidgetInfo *info;
  CtkWidget *widget;
  CtkWidget *gears;

  widget = ctk_frame_new (NULL);
  ctk_frame_set_shadow_type (CTK_FRAME (widget), CTK_SHADOW_IN);

  gears = ctk_gears_new ();
  ctk_container_add (CTK_CONTAINER (widget), gears);
 
  info = new_widget_info ("glarea", widget, MEDIUM);

  return info;
}

GList *
get_all_widgets (void)
{
  GList *retval = NULL;

  retval = g_list_prepend (retval, create_search_bar ());
  retval = g_list_prepend (retval, create_action_bar ());
  retval = g_list_prepend (retval, create_list_box());
  retval = g_list_prepend (retval, create_flow_box());
  retval = g_list_prepend (retval, create_headerbar ());
  retval = g_list_prepend (retval, create_placessidebar ());
  retval = g_list_prepend (retval, create_stack ());
  retval = g_list_prepend (retval, create_stack_switcher ());
  retval = g_list_prepend (retval, create_toolpalette ());
  retval = g_list_prepend (retval, create_spinner ());
  retval = g_list_prepend (retval, create_about_dialog ());
  retval = g_list_prepend (retval, create_accel_label ());
  retval = g_list_prepend (retval, create_button ());
  retval = g_list_prepend (retval, create_check_button ());
  retval = g_list_prepend (retval, create_color_button ());
  retval = g_list_prepend (retval, create_combo_box ());
  retval = g_list_prepend (retval, create_combo_box_entry ());
  retval = g_list_prepend (retval, create_combo_box_text ());
  retval = g_list_prepend (retval, create_entry ());
  retval = g_list_prepend (retval, create_file_button ());
  retval = g_list_prepend (retval, create_font_button ());
  retval = g_list_prepend (retval, create_frame ());
  retval = g_list_prepend (retval, create_icon_view ());
  retval = g_list_prepend (retval, create_image ());
  retval = g_list_prepend (retval, create_label ());
  retval = g_list_prepend (retval, create_link_button ());
  retval = g_list_prepend (retval, create_menubar ());
  retval = g_list_prepend (retval, create_message_dialog ());
  retval = g_list_prepend (retval, create_notebook ());
  retval = g_list_prepend (retval, create_panes ());
  retval = g_list_prepend (retval, create_progressbar ());
  retval = g_list_prepend (retval, create_radio ());
  retval = g_list_prepend (retval, create_scales ());
  retval = g_list_prepend (retval, create_scrolledwindow ());
  retval = g_list_prepend (retval, create_scrollbar ());
  retval = g_list_prepend (retval, create_separator ());
  retval = g_list_prepend (retval, create_spinbutton ());
  retval = g_list_prepend (retval, create_statusbar ());
  retval = g_list_prepend (retval, create_text_view ());
  retval = g_list_prepend (retval, create_toggle_button ());
  retval = g_list_prepend (retval, create_toolbar ());
  retval = g_list_prepend (retval, create_tree_view ());
  retval = g_list_prepend (retval, create_window ());
  retval = g_list_prepend (retval, create_filesel ());
  retval = g_list_prepend (retval, create_assistant ());
  retval = g_list_prepend (retval, create_recent_chooser_dialog ());
  retval = g_list_prepend (retval, create_page_setup_dialog ());
  retval = g_list_prepend (retval, create_print_dialog ());
  retval = g_list_prepend (retval, create_volume_button ());
  retval = g_list_prepend (retval, create_switch ());
  retval = g_list_prepend (retval, create_appchooserbutton ());
  retval = g_list_prepend (retval, create_appchooserdialog ());
  retval = g_list_prepend (retval, create_lockbutton ());
  retval = g_list_prepend (retval, create_fontchooserdialog ());
  retval = g_list_prepend (retval, create_colorchooserdialog ());
  retval = g_list_prepend (retval, create_menu_button ());
  retval = g_list_prepend (retval, create_search_entry ());
  retval = g_list_prepend (retval, create_level_bar ());
  retval = g_list_prepend (retval, create_info_bar ());
  retval = g_list_prepend (retval, create_gl_area ());
  retval = g_list_prepend (retval, create_sidebar ());

  return retval;
}
