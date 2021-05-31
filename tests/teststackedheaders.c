#include <ctk/ctk.h>

static CtkWidget *header_stack;
static CtkWidget *page_stack;

static void
back_to_main (CtkButton *button)
{
  ctk_stack_set_visible_child_name (CTK_STACK (header_stack), "main");
  ctk_stack_set_visible_child_name (CTK_STACK (page_stack), "page1");
}

static void
go_to_secondary (CtkButton *button)
{
  ctk_stack_set_visible_child_name (CTK_STACK (header_stack), "secondary");
  ctk_stack_set_visible_child_name (CTK_STACK (page_stack), "secondary");
}

int
main (int argc, char *argv[])
{
  CtkBuilder *builder;
  CtkWidget *win;

  ctk_init (NULL, NULL);

  builder = ctk_builder_new ();
  ctk_builder_add_callback_symbol (builder, "back_to_main", G_CALLBACK (back_to_main));
  ctk_builder_add_callback_symbol (builder, "go_to_secondary", G_CALLBACK (go_to_secondary));
  ctk_builder_add_from_file (builder, "teststackedheaders.ui", NULL);
  ctk_builder_connect_signals (builder, NULL);

  win = (CtkWidget *)ctk_builder_get_object (builder, "window");
  header_stack = (CtkWidget *)ctk_builder_get_object (builder, "header_stack");
  page_stack = (CtkWidget *)ctk_builder_get_object (builder, "page_stack");

  ctk_window_present (CTK_WINDOW (win));

  ctk_main ();

  return 0;
}
