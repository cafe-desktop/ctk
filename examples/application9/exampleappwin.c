#include <gtk/gtk.h>

#include "exampleapp.h"
#include "exampleappwin.h"

struct _ExampleAppWindow
{
  GtkApplicationWindow parent;
};

typedef struct _ExampleAppWindowPrivate ExampleAppWindowPrivate;

struct _ExampleAppWindowPrivate
{
  GSettings *settings;
  GtkWidget *stack;
  GtkWidget *search;
  GtkWidget *searchbar;
  GtkWidget *searchentry;
  GtkWidget *gears;
  GtkWidget *sidebar;
  GtkWidget *words;
  GtkWidget *lines;
  GtkWidget *lines_label;
};

G_DEFINE_TYPE_WITH_PRIVATE(ExampleAppWindow, example_app_window, GTK_TYPE_APPLICATION_WINDOW);

static void
search_text_changed (GtkEntry *entry)
{
  ExampleAppWindow *win;
  ExampleAppWindowPrivate *priv;
  const gchar *text;
  GtkWidget *tab;
  GtkWidget *view;
  GtkTextBuffer *buffer;
  GtkTextIter start, match_start, match_end;

  text = ctk_entry_get_text (entry);

  if (text[0] == '\0')
    return;

  win = EXAMPLE_APP_WINDOW (ctk_widget_get_toplevel (GTK_WIDGET (entry)));
  priv = example_app_window_get_instance_private (win);

  tab = ctk_stack_get_visible_child (GTK_STACK (priv->stack));
  view = ctk_bin_get_child (GTK_BIN (tab));
  buffer = ctk_text_view_get_buffer (GTK_TEXT_VIEW (view));

  /* Very simple-minded search implementation */
  ctk_text_buffer_get_start_iter (buffer, &start);
  if (ctk_text_iter_forward_search (&start, text, GTK_TEXT_SEARCH_CASE_INSENSITIVE,
                                    &match_start, &match_end, NULL))
    {
      ctk_text_buffer_select_range (buffer, &match_start, &match_end);
      ctk_text_view_scroll_to_iter (GTK_TEXT_VIEW (view), &match_start,
                                    0.0, FALSE, 0.0, 0.0);
    }
}

static void
find_word (GtkButton        *button,
           ExampleAppWindow *win)
{
  ExampleAppWindowPrivate *priv;
  const gchar *word;

  priv = example_app_window_get_instance_private (win);

  word = ctk_button_get_label (button);
  ctk_entry_set_text (GTK_ENTRY (priv->searchentry), word);
}

static void
update_words (ExampleAppWindow *win)
{
  ExampleAppWindowPrivate *priv;
  GHashTable *strings;
  GHashTableIter iter;
  GtkWidget *tab, *view, *row;
  GtkTextBuffer *buffer;
  GtkTextIter start, end;
  GList *children, *l;
  gchar *word, *key;

  priv = example_app_window_get_instance_private (win);

  tab = ctk_stack_get_visible_child (GTK_STACK (priv->stack));

  if (tab == NULL)
    return;

  view = ctk_bin_get_child (GTK_BIN (tab));
  buffer = ctk_text_view_get_buffer (GTK_TEXT_VIEW (view));

  strings = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

  ctk_text_buffer_get_start_iter (buffer, &start);
  while (!ctk_text_iter_is_end (&start))
    {
      while (!ctk_text_iter_starts_word (&start))
        {
          if (!ctk_text_iter_forward_char (&start))
            goto done;
        }
      end = start;
      if (!ctk_text_iter_forward_word_end (&end))
        goto done;
      word = ctk_text_buffer_get_text (buffer, &start, &end, FALSE);
      g_hash_table_add (strings, g_utf8_strdown (word, -1));
      g_free (word);
      start = end;
    }

done:
  children = ctk_container_get_children (GTK_CONTAINER (priv->words));
  for (l = children; l; l = l->next)
    ctk_container_remove (GTK_CONTAINER (priv->words), GTK_WIDGET (l->data));
  g_list_free (children);

  g_hash_table_iter_init (&iter, strings);
  while (g_hash_table_iter_next (&iter, (gpointer *)&key, NULL))
    {
      row = ctk_button_new_with_label (key);
      g_signal_connect (row, "clicked",
                        G_CALLBACK (find_word), win);
      ctk_widget_show (row);
      ctk_container_add (GTK_CONTAINER (priv->words), row);
    }

  g_hash_table_unref (strings);
}

static void
update_lines (ExampleAppWindow *win)
{
  ExampleAppWindowPrivate *priv;
  GtkWidget *tab, *view;
  GtkTextBuffer *buffer;
  GtkTextIter iter;
  int count;
  gchar *lines;

  priv = example_app_window_get_instance_private (win);

  tab = ctk_stack_get_visible_child (GTK_STACK (priv->stack));

  if (tab == NULL)
    return;

  view = ctk_bin_get_child (GTK_BIN (tab));
  buffer = ctk_text_view_get_buffer (GTK_TEXT_VIEW (view));

  count = 0;

  ctk_text_buffer_get_start_iter (buffer, &iter);
  while (!ctk_text_iter_is_end (&iter))
    {
      count++;
      if (!ctk_text_iter_forward_line (&iter))
        break;
    }

  lines = g_strdup_printf ("%d", count);
  ctk_label_set_text (GTK_LABEL (priv->lines), lines);
  g_free (lines);
}

static void
visible_child_changed (GObject    *stack,
                       GParamSpec *pspec)
{
  ExampleAppWindow *win;
  ExampleAppWindowPrivate *priv;

  if (ctk_widget_in_destruction (GTK_WIDGET (stack)))
    return;

  win = EXAMPLE_APP_WINDOW (ctk_widget_get_toplevel (GTK_WIDGET (stack)));
  priv = example_app_window_get_instance_private (win);
  ctk_search_bar_set_search_mode (GTK_SEARCH_BAR (priv->searchbar), FALSE);
  update_words (win);
  update_lines (win);
}

static void
words_changed (GObject          *sidebar,
               GParamSpec       *pspec,
               ExampleAppWindow *win)
{
  update_words (win);
}

static void
example_app_window_init (ExampleAppWindow *win)
{
  ExampleAppWindowPrivate *priv;
  GtkBuilder *builder;
  GMenuModel *menu;
  GAction *action;

  priv = example_app_window_get_instance_private (win);
  ctk_widget_init_template (GTK_WIDGET (win));
  priv->settings = g_settings_new ("org.gtk.exampleapp");

  g_settings_bind (priv->settings, "transition",
                   priv->stack, "transition-type",
                   G_SETTINGS_BIND_DEFAULT);

  g_settings_bind (priv->settings, "show-words",
                   priv->sidebar, "reveal-child",
                   G_SETTINGS_BIND_DEFAULT);

  g_object_bind_property (priv->search, "active",
                          priv->searchbar, "search-mode-enabled",
                          G_BINDING_BIDIRECTIONAL);

  g_signal_connect (priv->sidebar, "notify::reveal-child",
                    G_CALLBACK (words_changed), win);

  builder = ctk_builder_new_from_resource ("/org/gtk/exampleapp/gears-menu.ui");
  menu = G_MENU_MODEL (ctk_builder_get_object (builder, "menu"));
  ctk_menu_button_set_menu_model (GTK_MENU_BUTTON (priv->gears), menu);
  g_object_unref (builder);

  action = g_settings_create_action (priv->settings, "show-words");
  g_action_map_add_action (G_ACTION_MAP (win), action);
  g_object_unref (action);

  action = (GAction*) g_property_action_new ("show-lines", priv->lines, "visible");
  g_action_map_add_action (G_ACTION_MAP (win), action);
  g_object_unref (action);

  g_object_bind_property (priv->lines, "visible",
                          priv->lines_label, "visible",
                          G_BINDING_DEFAULT);
}

static void
example_app_window_dispose (GObject *object)
{
  ExampleAppWindow *win;
  ExampleAppWindowPrivate *priv;

  win = EXAMPLE_APP_WINDOW (object);
  priv = example_app_window_get_instance_private (win);

  g_clear_object (&priv->settings);

  G_OBJECT_CLASS (example_app_window_parent_class)->dispose (object);
}

static void
example_app_window_class_init (ExampleAppWindowClass *class)
{
  G_OBJECT_CLASS (class)->dispose = example_app_window_dispose;

  ctk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (class),
                                               "/org/gtk/exampleapp/window.ui");

  ctk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class), ExampleAppWindow, stack);
  ctk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class), ExampleAppWindow, search);
  ctk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class), ExampleAppWindow, searchbar);
  ctk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class), ExampleAppWindow, searchentry);
  ctk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class), ExampleAppWindow, gears);
  ctk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class), ExampleAppWindow, words);
  ctk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class), ExampleAppWindow, sidebar);
  ctk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class), ExampleAppWindow, lines);
  ctk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class), ExampleAppWindow, lines_label);

  ctk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (class), search_text_changed);
  ctk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (class), visible_child_changed);
}

ExampleAppWindow *
example_app_window_new (ExampleApp *app)
{
  return g_object_new (EXAMPLE_APP_WINDOW_TYPE, "application", app, NULL);
}

void
example_app_window_open (ExampleAppWindow *win,
                         GFile            *file)
{
  ExampleAppWindowPrivate *priv;
  gchar *basename;
  GtkWidget *scrolled, *view;
  gchar *contents;
  gsize length;
  GtkTextBuffer *buffer;
  GtkTextTag *tag;
  GtkTextIter start_iter, end_iter;

  priv = example_app_window_get_instance_private (win);
  basename = g_file_get_basename (file);

  scrolled = ctk_scrolled_window_new (NULL, NULL);
  ctk_widget_show (scrolled);
  ctk_widget_set_hexpand (scrolled, TRUE);
  ctk_widget_set_vexpand (scrolled, TRUE);
  view = ctk_text_view_new ();
  ctk_text_view_set_editable (GTK_TEXT_VIEW (view), FALSE);
  ctk_text_view_set_cursor_visible (GTK_TEXT_VIEW (view), FALSE);
  ctk_widget_show (view);
  ctk_container_add (GTK_CONTAINER (scrolled), view);
  ctk_stack_add_titled (GTK_STACK (priv->stack), scrolled, basename, basename);

  buffer = ctk_text_view_get_buffer (GTK_TEXT_VIEW (view));

  if (g_file_load_contents (file, NULL, &contents, &length, NULL, NULL))
    {
      ctk_text_buffer_set_text (buffer, contents, length);
      g_free (contents);
    }

  tag = ctk_text_buffer_create_tag (buffer, NULL, NULL);
  g_settings_bind (priv->settings, "font",
                   tag, "font",
                   G_SETTINGS_BIND_DEFAULT);

  ctk_text_buffer_get_start_iter (buffer, &start_iter);
  ctk_text_buffer_get_end_iter (buffer, &end_iter);
  ctk_text_buffer_apply_tag (buffer, tag, &start_iter, &end_iter);

  g_free (basename);

  ctk_widget_set_sensitive (priv->search, TRUE);

  update_words (win);
  update_lines (win);
}
