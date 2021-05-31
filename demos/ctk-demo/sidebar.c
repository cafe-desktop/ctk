/* Stack Sidebar
 *
 * CtkStackSidebar provides an automatic sidebar widget to control
 * navigation of a CtkStack object. This widget automatically updates it
 * content based on what is presently available in the CtkStack object,
 * and using the "title" child property to set the display labels.
 */

#include <glib/gi18n.h>
#include <ctk/ctk.h>

CtkWidget *
do_sidebar (CtkWidget *do_widget)
{
  static CtkWidget *window = NULL;
  CtkWidget *sidebar;
  CtkWidget *stack;
  CtkWidget *box;
  CtkWidget *widget;
  CtkWidget *header;
  const gchar* pages[] = {
    "Welcome to CTK+",
    "CtkStackSidebar Widget",
    "Automatic navigation",
    "Consistent appearance",
    "Scrolling",
    "Page 6",
    "Page 7",
    "Page 8",
    "Page 9",
    NULL
  };
  const gchar *c = NULL;
  guint i;

  if (!window)
    {
      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_resizable (CTK_WINDOW (window), TRUE);
      ctk_widget_set_size_request (window, 500, 350);

      header = ctk_header_bar_new ();
      ctk_header_bar_set_show_close_button (CTK_HEADER_BAR(header), TRUE);
      ctk_window_set_titlebar (CTK_WINDOW(window), header);
      ctk_window_set_title (CTK_WINDOW(window), "Stack Sidebar");

      g_signal_connect (window, "destroy",
                        G_CALLBACK (ctk_widget_destroyed), &window);

      box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
      sidebar = ctk_stack_sidebar_new ();
      ctk_box_pack_start (CTK_BOX (box), sidebar, FALSE, FALSE, 0);

      stack = ctk_stack_new ();
      ctk_stack_set_transition_type (CTK_STACK (stack), CTK_STACK_TRANSITION_TYPE_SLIDE_UP_DOWN);
      ctk_stack_sidebar_set_stack (CTK_STACK_SIDEBAR (sidebar), CTK_STACK (stack));

      /* Separator between sidebar and stack */
      widget = ctk_separator_new (CTK_ORIENTATION_VERTICAL);
      ctk_box_pack_start (CTK_BOX(box), widget, FALSE, FALSE, 0);

      ctk_box_pack_start (CTK_BOX (box), stack, TRUE, TRUE, 0);

      for (i=0; (c = *(pages+i)) != NULL; i++ )
        {
          if (i == 0)
            {
              widget = ctk_image_new_from_icon_name ("help-about", CTK_ICON_SIZE_MENU);
              ctk_image_set_pixel_size (CTK_IMAGE (widget), 256);
            }
          else
            {
              widget = ctk_label_new (c);
            }
          ctk_stack_add_named (CTK_STACK (stack), widget, c);
          ctk_container_child_set (CTK_CONTAINER (stack), widget, "title", c, NULL);
        }

       ctk_container_add (CTK_CONTAINER (window), box);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);

  return window;
}
