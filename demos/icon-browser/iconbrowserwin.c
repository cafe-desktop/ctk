#include <string.h>
#include "iconbrowserapp.h"
#include "iconbrowserwin.h"
#include "iconstore.h"
#include <ctk/ctk.h>

/* Drag 'n Drop */
static CtkTargetEntry target_table[] = {
  { "text/uri-list", 0, 0 },
};

typedef struct
{
  gchar *id;
  gchar *name;
  gchar *description;
} Context;

static void
context_free (gpointer data)
{
  Context *context = data;

  g_free (context->id);
  g_free (context->name);
  g_free (context->description);
  g_free (context);
}

struct _IconBrowserWindow
{
  CtkApplicationWindow parent;
  GHashTable *contexts;

  CtkWidget *context_list;
  Context *current_context;
  gboolean symbolic;
  CtkWidget *symbolic_radio;
  CtkTreeModelFilter *filter_model;
  CtkWidget *details;

  CtkListStore *store;
  CtkCellRenderer *cell;
  CtkCellRenderer *text_cell;
  CtkWidget *search;
  CtkWidget *searchbar;
  CtkWidget *searchentry;
  CtkWidget *list;
  CtkWidget *image1;
  CtkWidget *image2;
  CtkWidget *image3;
  CtkWidget *image4;
  CtkWidget *image5;
  CtkWidget *image6;
  CtkWidget *label6;
  CtkWidget *description;
};

struct _IconBrowserWindowClass
{
  CtkApplicationWindowClass parent_class;
};

G_DEFINE_TYPE(IconBrowserWindow, icon_browser_window, CTK_TYPE_APPLICATION_WINDOW);

static void
search_text_changed (CtkEntry *entry, IconBrowserWindow *win)
{
  const gchar *text;

  text = ctk_entry_get_text (entry);

  if (text[0] == '\0')
    return;

  ctk_tree_model_filter_refilter (win->filter_model);
}

static CdkPixbuf *
get_icon (CtkWidget *image, const gchar *name, gint size)
{
  CtkIconInfo *info;
  CtkStyleContext *context;
  CdkPixbuf *pixbuf;

  context = ctk_widget_get_style_context (image);
  info = ctk_icon_theme_lookup_icon (ctk_icon_theme_get_default (), name, size, 0);
  pixbuf = ctk_icon_info_load_symbolic_for_context (info, context, NULL, NULL);
  g_object_unref (info);

  return pixbuf;
}

static void
set_image (CtkWidget *image, const gchar *name, gint size)
{
  CdkPixbuf *pixbuf;

  ctk_image_set_from_icon_name (CTK_IMAGE (image), name, 1);
  ctk_image_set_pixel_size (CTK_IMAGE (image), size);
  pixbuf = get_icon (image, name, size);
  ctk_drag_source_set_icon_pixbuf (ctk_widget_get_parent (image), pixbuf);
  g_object_unref (pixbuf);
}

static void
item_activated (CtkIconView *icon_view, CtkTreePath *path, IconBrowserWindow *win)
{
  CtkTreeIter iter;
  gchar *name;
  gchar *description;
  gint column;

  ctk_tree_model_get_iter (CTK_TREE_MODEL (win->filter_model), &iter, path);

  if (win->symbolic)
    column = ICON_STORE_SYMBOLIC_NAME_COLUMN;
  else
    column = ICON_STORE_NAME_COLUMN;
  ctk_tree_model_get (CTK_TREE_MODEL (win->filter_model), &iter,
                      column, &name,
                      ICON_STORE_DESCRIPTION_COLUMN, &description,
                      -1);

  if (name == NULL || !ctk_icon_theme_has_icon (ctk_icon_theme_get_default (), name))
    {
      g_free (description);
      return;
    }

  ctk_window_set_title (CTK_WINDOW (win->details), name);
  set_image (win->image1, name, 16);
  set_image (win->image2, name, 24);
  set_image (win->image3, name, 32);
  set_image (win->image4, name, 48);
  set_image (win->image5, name, 64);
  if (win->symbolic)
    {
      ctk_widget_show (win->image6);
      ctk_widget_show (win->label6);
      ctk_widget_show (ctk_widget_get_parent (win->image6));
      set_image (win->image6, name, 64);
    }
  else
    {
      ctk_widget_hide (win->image6);
      ctk_widget_hide (win->label6);
      ctk_widget_hide (ctk_widget_get_parent (win->image6));
    }
  if (description && description[0])
    {
      ctk_label_set_text (CTK_LABEL (win->description), description);
      ctk_widget_show (win->description);
    }
  else
    {
      ctk_widget_hide (win->description);
    }

  ctk_window_present (CTK_WINDOW (win->details));

  g_free (name);
  g_free (description);
}

static void
add_icon (IconBrowserWindow *win,
          const gchar       *name,
          const gchar       *description,
          const gchar       *context)
{
  gchar *regular_name;
  gchar *symbolic_name;

  regular_name = g_strdup (name);
  if (!ctk_icon_theme_has_icon (ctk_icon_theme_get_default (), regular_name))
    {
      g_free (regular_name);
      regular_name = NULL;
    }

  symbolic_name = g_strconcat (name, "-symbolic", NULL);
  if (!ctk_icon_theme_has_icon (ctk_icon_theme_get_default (), symbolic_name))
    {
      g_free (symbolic_name);
      symbolic_name = NULL;
    }

  ctk_list_store_insert_with_values (win->store, NULL, -1,
                                     ICON_STORE_NAME_COLUMN, regular_name,
                                     ICON_STORE_SYMBOLIC_NAME_COLUMN, symbolic_name,
                                     ICON_STORE_DESCRIPTION_COLUMN, description,
                                     ICON_STORE_CONTEXT_COLUMN, context,
                                     -1);
}

static void
add_context (IconBrowserWindow *win,
             const gchar       *id,
             const gchar       *name,
             const gchar       *description)
{
  Context *c;
  CtkWidget *row;

  c = g_new (Context, 1);
  c->id = g_strdup (id);
  c->name = g_strdup (name);
  c->description = g_strdup (description);

  g_hash_table_insert (win->contexts, c->id, c);

  row = ctk_label_new (name);
  g_object_set_data (G_OBJECT (row), "context", c);
  ctk_widget_show (row);
  g_object_set (row, "margin", 10, NULL);

  ctk_list_box_insert (CTK_LIST_BOX (win->context_list), row, -1);

  /* set the tooltip on the list box row */
  row = ctk_widget_get_parent (row);
  ctk_widget_set_tooltip_text (row, description);

  if (win->current_context == NULL)
    win->current_context = c;
}

static void
selected_context_changed (CtkListBox *list, IconBrowserWindow *win)
{
  CtkWidget *row;
  CtkWidget *label;

  row = CTK_WIDGET (ctk_list_box_get_selected_row (list));
  if (row == NULL)
    return;

  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (win->search), FALSE);

  label = ctk_bin_get_child (CTK_BIN (row));
  win->current_context = g_object_get_data (G_OBJECT (label), "context");
  ctk_tree_model_filter_refilter (win->filter_model);
}

static void
populate (IconBrowserWindow *win)
{
  GFile *file;
  GKeyFile *kf;
  char *data;
  gsize length;
  char **groups;
  int i;

  file = g_file_new_for_uri ("resource:/org/ctk/iconbrowser/ctk/icon.list");
  g_file_load_contents (file, NULL, &data, &length, NULL, NULL);

  kf = g_key_file_new ();
  g_key_file_load_from_data (kf, data, length, G_KEY_FILE_NONE, NULL);

  groups = g_key_file_get_groups (kf, &length);
  for (i = 0; i < length; i++)
    {
      const char *context;
      const char *name;
      const char *description;
      char **keys;
      gsize len;
      int j;

      context = groups[i];
      name = g_key_file_get_string (kf, context, "Name", NULL);
      description = g_key_file_get_string (kf, context, "Description", NULL);
      add_context (win, context, name, description);

      keys = g_key_file_get_keys (kf, context, &len, NULL);
      for (j = 0; j < len; j++)
        {
          const char *key = keys[j];
          const char *value;

          if (strcmp (key, "Name") == 0 || strcmp (key, "Description") == 0)
            continue;

          value = g_key_file_get_string (kf, context, key, NULL);

          add_icon (win, key, value, context);
        }
      g_strfreev (keys);
    }
  g_strfreev (groups);
}

static gboolean
key_press_event_cb (CtkWidget *widget,
                    CdkEvent  *event,
                    gpointer   data)
{
  IconBrowserWindow *win = data;

  return ctk_search_bar_handle_event (CTK_SEARCH_BAR (win->searchbar), event);
}

static void
copy_to_clipboard (CtkButton         *button,
                   IconBrowserWindow *win)
{
  CtkClipboard *clipboard;

  clipboard = ctk_clipboard_get_default (cdk_display_get_default ());
  ctk_clipboard_set_text (clipboard, ctk_window_get_title (CTK_WINDOW (win->details)), -1);
}

static gboolean
icon_visible_func (CtkTreeModel *model,
                   CtkTreeIter  *iter,
                   gpointer      data)
{
  IconBrowserWindow *win = data;
  gchar *context;
  gchar *name;
  gint column;
  gboolean search;
  const gchar *search_text;
  gboolean visible;

  search = ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (win->search));
  search_text = ctk_entry_get_text (CTK_ENTRY (win->searchentry));

  if (win->symbolic)
    column = ICON_STORE_SYMBOLIC_NAME_COLUMN;
  else
    column = ICON_STORE_NAME_COLUMN;

  ctk_tree_model_get (model, iter,
                      column, &name,
                      ICON_STORE_CONTEXT_COLUMN, &context,
                      -1);
  if (!name)
    visible = FALSE;
  else if (search)
    visible = strstr (name, search_text) != NULL;
  else
    visible = win->current_context != NULL && g_strcmp0 (context, win->current_context->id) == 0;

  g_free (name);
  g_free (context);

  return visible;
}

static void
symbolic_toggled (CtkToggleButton *toggle, IconBrowserWindow *win)
{
  gint column;

  win->symbolic = ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (toggle));

  if (win->symbolic)
    column = ICON_STORE_SYMBOLIC_NAME_COLUMN;
  else
    column = ICON_STORE_NAME_COLUMN;

  icon_store_set_text_column (ICON_STORE (win->store), column);

  ctk_cell_layout_set_attributes (CTK_CELL_LAYOUT (win->list), win->cell, "icon-name", column, NULL);
  ctk_cell_layout_set_attributes (CTK_CELL_LAYOUT (win->list), win->text_cell, "text", column, NULL);

  ctk_tree_model_filter_refilter (win->filter_model);
  ctk_widget_queue_draw (win->list);
}

static void
search_mode_toggled (GObject *searchbar, GParamSpec *pspec, IconBrowserWindow *win)
{
  if (ctk_search_bar_get_search_mode (CTK_SEARCH_BAR (searchbar)))
    ctk_list_box_unselect_all (CTK_LIST_BOX (win->context_list));
}

static void
get_image_data (CtkWidget        *widget,
                CdkDragContext   *context,
                CtkSelectionData *selection,
                guint             target_info,
                guint             time,
                gpointer          data)
{
  CtkWidget *image;
  const gchar *name;
  gint size;
  CdkPixbuf *pixbuf;

  image = ctk_bin_get_child (CTK_BIN (widget));

  ctk_image_get_icon_name (CTK_IMAGE (image), &name, NULL);
  size = ctk_image_get_pixel_size (CTK_IMAGE (image));

  pixbuf = get_icon (image, name, size);
  ctk_selection_data_set_pixbuf (selection, pixbuf);
  g_object_unref (pixbuf);
}

static void
get_scalable_image_data (CtkWidget        *widget,
                         CdkDragContext   *context,
                         CtkSelectionData *selection,
                         guint             target_info,
                         guint             time,
                         gpointer          data)
{
  gchar *uris[2];
  CtkIconInfo *info;
  CtkWidget *image;
  GFile *file;
  const gchar *name;

  image = ctk_bin_get_child (CTK_BIN (widget));
  ctk_image_get_icon_name (CTK_IMAGE (image), &name, NULL);

  info = ctk_icon_theme_lookup_icon (ctk_icon_theme_get_default (), name, -1, 0);
  file = g_file_new_for_path (ctk_icon_info_get_filename (info));
  uris[0] = g_file_get_uri (file);
  uris[1] = NULL;

  ctk_selection_data_set_uris (selection, uris);

  g_free (uris[0]);
  g_object_unref (info);
  g_object_unref (file);
}

static void
setup_image_dnd (CtkWidget *image)
{
  CtkWidget *parent;

  parent = ctk_widget_get_parent (image);
  ctk_drag_source_set (parent, GDK_BUTTON1_MASK, NULL, 0, GDK_ACTION_COPY);
  ctk_drag_source_add_image_targets (parent);
  g_signal_connect (parent, "drag-data-get", G_CALLBACK (get_image_data), NULL);
}

static void
setup_scalable_image_dnd (CtkWidget *image)
{
  CtkWidget *parent;

  parent = ctk_widget_get_parent (image);
  ctk_drag_source_set (parent, GDK_BUTTON1_MASK,
                       target_table, G_N_ELEMENTS (target_table),
                       GDK_ACTION_COPY);

  g_signal_connect (parent, "drag-data-get", G_CALLBACK (get_scalable_image_data), NULL);
}

static void
icon_browser_window_init (IconBrowserWindow *win)
{
  CtkTargetList *list;
  CtkTargetEntry *targets;
  gint n_targets;

  ctk_widget_init_template (CTK_WIDGET (win));

  list = ctk_target_list_new (NULL, 0);
  ctk_target_list_add_text_targets (list, 0);
  targets = ctk_target_table_new_from_list (list, &n_targets);
  ctk_target_list_unref (list);

  ctk_icon_view_enable_model_drag_source (CTK_ICON_VIEW (win->list),
                                          GDK_BUTTON1_MASK,
                                          targets, n_targets,
                                          GDK_ACTION_COPY);

  ctk_target_table_free (targets, n_targets);

  setup_image_dnd (win->image1);
  setup_image_dnd (win->image2);
  setup_image_dnd (win->image3);
  setup_image_dnd (win->image4);
  setup_image_dnd (win->image5);
  setup_scalable_image_dnd (win->image6);

  win->contexts = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, context_free);

  ctk_tree_model_filter_set_visible_func (win->filter_model, icon_visible_func, win, NULL);
  ctk_window_set_transient_for (CTK_WINDOW (win->details), CTK_WINDOW (win));

  g_signal_connect (win->searchbar, "notify::search-mode-enabled",
                    G_CALLBACK (search_mode_toggled), win);

  symbolic_toggled (CTK_TOGGLE_BUTTON (win->symbolic_radio), win);

  populate (win);
}

static void
icon_browser_window_class_init (IconBrowserWindowClass *class)
{
  g_type_ensure (ICON_STORE_TYPE);

  ctk_widget_class_set_template_from_resource (CTK_WIDGET_CLASS (class),
                                               "/org/ctk/iconbrowser/ctk/window.ui");

  ctk_widget_class_bind_template_child (CTK_WIDGET_CLASS (class), IconBrowserWindow, context_list);
  ctk_widget_class_bind_template_child (CTK_WIDGET_CLASS (class), IconBrowserWindow, filter_model);
  ctk_widget_class_bind_template_child (CTK_WIDGET_CLASS (class), IconBrowserWindow, symbolic_radio);
  ctk_widget_class_bind_template_child (CTK_WIDGET_CLASS (class), IconBrowserWindow, details);

  ctk_widget_class_bind_template_child (CTK_WIDGET_CLASS (class), IconBrowserWindow, store);
  ctk_widget_class_bind_template_child (CTK_WIDGET_CLASS (class), IconBrowserWindow, cell);
  ctk_widget_class_bind_template_child (CTK_WIDGET_CLASS (class), IconBrowserWindow, text_cell);
  ctk_widget_class_bind_template_child (CTK_WIDGET_CLASS (class), IconBrowserWindow, search);
  ctk_widget_class_bind_template_child (CTK_WIDGET_CLASS (class), IconBrowserWindow, searchbar);
  ctk_widget_class_bind_template_child (CTK_WIDGET_CLASS (class), IconBrowserWindow, searchentry);
  ctk_widget_class_bind_template_child (CTK_WIDGET_CLASS (class), IconBrowserWindow, list);
  ctk_widget_class_bind_template_child (CTK_WIDGET_CLASS (class), IconBrowserWindow, image1);
  ctk_widget_class_bind_template_child (CTK_WIDGET_CLASS (class), IconBrowserWindow, image2);
  ctk_widget_class_bind_template_child (CTK_WIDGET_CLASS (class), IconBrowserWindow, image3);
  ctk_widget_class_bind_template_child (CTK_WIDGET_CLASS (class), IconBrowserWindow, image4);
  ctk_widget_class_bind_template_child (CTK_WIDGET_CLASS (class), IconBrowserWindow, image5);
  ctk_widget_class_bind_template_child (CTK_WIDGET_CLASS (class), IconBrowserWindow, image6);
  ctk_widget_class_bind_template_child (CTK_WIDGET_CLASS (class), IconBrowserWindow, label6);
  ctk_widget_class_bind_template_child (CTK_WIDGET_CLASS (class), IconBrowserWindow, description);

  ctk_widget_class_bind_template_callback (CTK_WIDGET_CLASS (class), search_text_changed);
  ctk_widget_class_bind_template_callback (CTK_WIDGET_CLASS (class), item_activated);
  ctk_widget_class_bind_template_callback (CTK_WIDGET_CLASS (class), selected_context_changed);
  ctk_widget_class_bind_template_callback (CTK_WIDGET_CLASS (class), symbolic_toggled);
  ctk_widget_class_bind_template_callback (CTK_WIDGET_CLASS (class), key_press_event_cb);
  ctk_widget_class_bind_template_callback (CTK_WIDGET_CLASS (class), copy_to_clipboard);
}

IconBrowserWindow *
icon_browser_window_new (IconBrowserApp *app)
{
  return g_object_new (ICON_BROWSER_WINDOW_TYPE, "application", app, NULL);
}
