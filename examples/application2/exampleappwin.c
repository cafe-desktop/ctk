#include <ctk/ctk.h>

#include "exampleapp.h"
#include "exampleappwin.h"

struct _ExampleAppWindow
{
  CtkApplicationWindow parent;
};

G_DEFINE_TYPE(ExampleAppWindow, example_app_window, CTK_TYPE_APPLICATION_WINDOW);

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
}

ExampleAppWindow *
example_app_window_new (ExampleApp *app)
{
  return g_object_new (EXAMPLE_APP_WINDOW_TYPE, "application", app, NULL);
}

void
example_app_window_open (ExampleAppWindow *win G_GNUC_UNUSED,
                         GFile            *file G_GNUC_UNUSED)
{
}
