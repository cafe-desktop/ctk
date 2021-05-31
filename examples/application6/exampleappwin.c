#include <ctk/ctk.h>

#include "exampleapp.h"
#include "exampleappwin.h"

struct _ExampleAppWindow
{
  CtkApplicationWindow parent;
};

typedef struct ExampleAppWindowPrivate ExampleAppWindowPrivate;

struct ExampleAppWindowPrivate
{
  GSettings *settings;
  CtkWidget *stack;
};

G_DEFINE_TYPE_WITH_PRIVATE(ExampleAppWindow, example_app_window, CTK_TYPE_APPLICATION_WINDOW);

static void
example_app_window_init (ExampleAppWindow *win)
{
  ExampleAppWindowPrivate *priv;

  priv = example_app_window_get_instance_private (win);
  ctk_widget_init_template (CTK_WIDGET (win));
  priv->settings = g_settings_new ("org.ctk.exampleapp");

  g_settings_bind (priv->settings, "transition",
                   priv->stack, "transition-type",
                   G_SETTINGS_BIND_DEFAULT);
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

  ctk_widget_class_set_template_from_resource (CTK_WIDGET_CLASS (class),
                                               "/org/ctk/exampleapp/window.ui");
  ctk_widget_class_bind_template_child_private (CTK_WIDGET_CLASS (class), ExampleAppWindow, stack);
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
  CtkWidget *scrolled, *view;
  gchar *contents;
  gsize length;
  CtkTextBuffer *buffer;
  CtkTextTag *tag;
  CtkTextIter start_iter, end_iter;

  priv = example_app_window_get_instance_private (win);
  basename = g_file_get_basename (file);

  scrolled = ctk_scrolled_window_new (NULL, NULL);
  ctk_widget_show (scrolled);
  ctk_widget_set_hexpand (scrolled, TRUE);
  ctk_widget_set_vexpand (scrolled, TRUE);
  view = ctk_text_view_new ();
  ctk_text_view_set_editable (CTK_TEXT_VIEW (view), FALSE);
  ctk_text_view_set_cursor_visible (CTK_TEXT_VIEW (view), FALSE);
  ctk_widget_show (view);
  ctk_container_add (CTK_CONTAINER (scrolled), view);
  ctk_stack_add_titled (CTK_STACK (priv->stack), scrolled, basename, basename);

  buffer = ctk_text_view_get_buffer (CTK_TEXT_VIEW (view));

  if (g_file_load_contents (file, NULL, &contents, &length, NULL, NULL))
    {
      ctk_text_buffer_set_text (buffer, contents, length);
      g_free (contents);
    }

  tag = ctk_text_buffer_create_tag (buffer, NULL, NULL);
  g_settings_bind (priv->settings, "font", tag, "font", G_SETTINGS_BIND_DEFAULT);

  ctk_text_buffer_get_start_iter (buffer, &start_iter);
  ctk_text_buffer_get_end_iter (buffer, &end_iter);
  ctk_text_buffer_apply_tag (buffer, tag, &start_iter, &end_iter);

  g_free (basename);
}
