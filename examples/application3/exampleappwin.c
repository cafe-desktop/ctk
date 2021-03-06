#include <ctk/ctk.h>

#include "exampleapp.h"
#include "exampleappwin.h"

struct _ExampleAppWindow
{
  CtkApplicationWindow parent;
};

typedef struct _ExampleAppWindowPrivate ExampleAppWindowPrivate;

struct _ExampleAppWindowPrivate
{
  CtkWidget *stack;
};

G_DEFINE_TYPE_WITH_PRIVATE(ExampleAppWindow, example_app_window, CTK_TYPE_APPLICATION_WINDOW);

static void
example_app_window_init (ExampleAppWindow *win)
{
  ctk_widget_init_template (CTK_WIDGET (win));
}

static void
example_app_window_class_init (ExampleAppWindowClass *class)
{
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

  if (g_file_load_contents (file, NULL, &contents, &length, NULL, NULL))
    {
      CtkTextBuffer *buffer;

      buffer = ctk_text_view_get_buffer (CTK_TEXT_VIEW (view));
      ctk_text_buffer_set_text (buffer, contents, length);
      g_free (contents);
    }

  g_free (basename);
}
