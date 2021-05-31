/* Icon View/Icon View Basics
 *
 * The CtkIconView widget is used to display and manipulate icons.
 * It uses a CtkTreeModel for data storage, so the list store
 * example might be helpful.
 */

#include <glib/gi18n.h>
#include <ctk/ctk.h>
#include <string.h>

static CtkWidget *window = NULL;

#define FOLDER_NAME "/iconview/gnome-fs-directory.png"
#define FILE_NAME "/iconview/gnome-fs-regular.png"

enum
{
  COL_PATH,
  COL_DISPLAY_NAME,
  COL_PIXBUF,
  COL_IS_DIRECTORY,
  NUM_COLS
};


static GdkPixbuf *file_pixbuf, *folder_pixbuf;
gchar *parent;
CtkToolItem *up_button;

/* Loads the images for the demo and returns whether the operation succeeded */
static void
load_pixbufs (void)
{
  if (file_pixbuf)
    return; /* already loaded earlier */

  file_pixbuf = gdk_pixbuf_new_from_resource (FILE_NAME, NULL);
  /* resources must load successfully */
  g_assert (file_pixbuf);

  folder_pixbuf = gdk_pixbuf_new_from_resource (FOLDER_NAME, NULL);
  g_assert (folder_pixbuf);
}

static void
fill_store (CtkListStore *store)
{
  GDir *dir;
  const gchar *name;
  CtkTreeIter iter;

  /* First clear the store */
  ctk_list_store_clear (store);

  /* Now go through the directory and extract all the file
   * information */
  dir = g_dir_open (parent, 0, NULL);
  if (!dir)
    return;

  name = g_dir_read_name (dir);
  while (name != NULL)
    {
      gchar *path, *display_name;
      gboolean is_dir;

      /* We ignore hidden files that start with a '.' */
      if (name[0] != '.')
        {
          path = g_build_filename (parent, name, NULL);

          is_dir = g_file_test (path, G_FILE_TEST_IS_DIR);

          display_name = g_filename_to_utf8 (name, -1, NULL, NULL, NULL);

          ctk_list_store_append (store, &iter);
          ctk_list_store_set (store, &iter,
                              COL_PATH, path,
                              COL_DISPLAY_NAME, display_name,
                              COL_IS_DIRECTORY, is_dir,
                              COL_PIXBUF, is_dir ? folder_pixbuf : file_pixbuf,
                              -1);
          g_free (path);
          g_free (display_name);
        }

      name = g_dir_read_name (dir);
    }
  g_dir_close (dir);
}

static gint
sort_func (CtkTreeModel *model,
           CtkTreeIter  *a,
           CtkTreeIter  *b,
           gpointer      user_data)
{
  gboolean is_dir_a, is_dir_b;
  gchar *name_a, *name_b;
  int ret;

  /* We need this function because we want to sort
   * folders before files.
   */


  ctk_tree_model_get (model, a,
                      COL_IS_DIRECTORY, &is_dir_a,
                      COL_DISPLAY_NAME, &name_a,
                      -1);

  ctk_tree_model_get (model, b,
                      COL_IS_DIRECTORY, &is_dir_b,
                      COL_DISPLAY_NAME, &name_b,
                      -1);

  if (!is_dir_a && is_dir_b)
    ret = 1;
  else if (is_dir_a && !is_dir_b)
    ret = -1;
  else
    {
      ret = g_utf8_collate (name_a, name_b);
    }

  g_free (name_a);
  g_free (name_b);

  return ret;
}

static CtkListStore *
create_store (void)
{
  CtkListStore *store;

  store = ctk_list_store_new (NUM_COLS,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              GDK_TYPE_PIXBUF,
                              G_TYPE_BOOLEAN);

  /* Set sort column and function */
  ctk_tree_sortable_set_default_sort_func (CTK_TREE_SORTABLE (store),
                                           sort_func,
                                           NULL, NULL);
  ctk_tree_sortable_set_sort_column_id (CTK_TREE_SORTABLE (store),
                                        CTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
                                        CTK_SORT_ASCENDING);

  return store;
}

static void
item_activated (CtkIconView *icon_view,
                CtkTreePath *tree_path,
                gpointer     user_data)
{
  CtkListStore *store;
  gchar *path;
  CtkTreeIter iter;
  gboolean is_dir;

  store = CTK_LIST_STORE (user_data);

  ctk_tree_model_get_iter (CTK_TREE_MODEL (store),
                           &iter, tree_path);
  ctk_tree_model_get (CTK_TREE_MODEL (store), &iter,
                      COL_PATH, &path,
                      COL_IS_DIRECTORY, &is_dir,
                      -1);

  if (!is_dir)
    {
      g_free (path);
      return;
    }

  /* Replace parent with path and re-fill the model*/
  g_free (parent);
  parent = path;

  fill_store (store);

  /* Sensitize the up button */
  ctk_widget_set_sensitive (CTK_WIDGET (up_button), TRUE);
}

static void
up_clicked (CtkToolItem *item,
            gpointer     user_data)
{
  CtkListStore *store;
  gchar *dir_name;

  store = CTK_LIST_STORE (user_data);

  dir_name = g_path_get_dirname (parent);
  g_free (parent);

  parent = dir_name;

  fill_store (store);

  /* Maybe de-sensitize the up button */
  ctk_widget_set_sensitive (CTK_WIDGET (up_button),
                            strcmp (parent, "/") != 0);
}

static void
home_clicked (CtkToolItem *item,
              gpointer     user_data)
{
  CtkListStore *store;

  store = CTK_LIST_STORE (user_data);

  g_free (parent);
  parent = g_strdup (g_get_home_dir ());

  fill_store (store);

  /* Sensitize the up button */
  ctk_widget_set_sensitive (CTK_WIDGET (up_button),
                            TRUE);
}

static void close_window(void)
{
  ctk_widget_destroy (window);
  window = NULL;

  g_object_unref (file_pixbuf);
  file_pixbuf = NULL;

  g_object_unref (folder_pixbuf);
  folder_pixbuf = NULL;
}

CtkWidget *
do_iconview (CtkWidget *do_widget)
{
  if (!window)
    {
      CtkWidget *sw;
      CtkWidget *icon_view;
      CtkListStore *store;
      CtkWidget *vbox;
      CtkWidget *tool_bar;
      CtkToolItem *home_button;

      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_default_size (CTK_WINDOW (window), 650, 400);

      ctk_window_set_screen (CTK_WINDOW (window),
                             ctk_widget_get_screen (do_widget));
      ctk_window_set_title (CTK_WINDOW (window), "Icon View Basics");

      g_signal_connect (window, "destroy",
                        G_CALLBACK (close_window), NULL);

      load_pixbufs ();

      vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
      ctk_container_add (CTK_CONTAINER (window), vbox);

      tool_bar = ctk_toolbar_new ();
      ctk_box_pack_start (CTK_BOX (vbox), tool_bar, FALSE, FALSE, 0);

      up_button = ctk_tool_button_new (NULL, NULL);
      ctk_tool_button_set_label (CTK_TOOL_BUTTON (up_button), _("_Up"));
      ctk_tool_button_set_use_underline (CTK_TOOL_BUTTON (up_button), TRUE);
      ctk_tool_button_set_icon_name (CTK_TOOL_BUTTON (up_button), "go-up");
      ctk_tool_item_set_is_important (up_button, TRUE);
      ctk_widget_set_sensitive (CTK_WIDGET (up_button), FALSE);
      ctk_toolbar_insert (CTK_TOOLBAR (tool_bar), up_button, -1);

      home_button = ctk_tool_button_new (NULL, NULL);
      ctk_tool_button_set_label (CTK_TOOL_BUTTON (home_button), _("_Home"));
      ctk_tool_button_set_use_underline (CTK_TOOL_BUTTON (home_button), TRUE);
      ctk_tool_button_set_icon_name (CTK_TOOL_BUTTON (home_button), "go-home");
      ctk_tool_item_set_is_important (home_button, TRUE);
      ctk_toolbar_insert (CTK_TOOLBAR (tool_bar), home_button, -1);


      sw = ctk_scrolled_window_new (NULL, NULL);
      ctk_scrolled_window_set_shadow_type (CTK_SCROLLED_WINDOW (sw),
                                           CTK_SHADOW_ETCHED_IN);
      ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (sw),
                                      CTK_POLICY_AUTOMATIC,
                                      CTK_POLICY_AUTOMATIC);

      ctk_box_pack_start (CTK_BOX (vbox), sw, TRUE, TRUE, 0);

      /* Create the store and fill it with the contents of '/' */
      parent = g_strdup ("/");
      store = create_store ();
      fill_store (store);

      icon_view = ctk_icon_view_new_with_model (CTK_TREE_MODEL (store));
      ctk_icon_view_set_selection_mode (CTK_ICON_VIEW (icon_view),
                                        CTK_SELECTION_MULTIPLE);
      g_object_unref (store);

      /* Connect to the "clicked" signal of the "Up" tool button */
      g_signal_connect (up_button, "clicked",
                        G_CALLBACK (up_clicked), store);

      /* Connect to the "clicked" signal of the "Home" tool button */
      g_signal_connect (home_button, "clicked",
                        G_CALLBACK (home_clicked), store);

      /* We now set which model columns that correspond to the text
       * and pixbuf of each item
       */
      ctk_icon_view_set_text_column (CTK_ICON_VIEW (icon_view), COL_DISPLAY_NAME);
      ctk_icon_view_set_pixbuf_column (CTK_ICON_VIEW (icon_view), COL_PIXBUF);

      /* Connect to the "item-activated" signal */
      g_signal_connect (icon_view, "item-activated",
                        G_CALLBACK (item_activated), store);
      ctk_container_add (CTK_CONTAINER (sw), icon_view);

      ctk_widget_grab_focus (icon_view);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);

  return window;
}
