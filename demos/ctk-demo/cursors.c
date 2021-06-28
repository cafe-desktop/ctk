/* Cursors
 *
 * Demonstrates a useful set of available cursors.
 */
#include <ctk/ctk.h>

static void
set_cursor (CtkWidget *button, gpointer data)
{
  CtkWidget *toplevel;
  CdkCursor *cursor = data;
  CdkWindow *window;

  toplevel = ctk_widget_get_toplevel (button);
  window = ctk_widget_get_window (toplevel);
  cdk_window_set_cursor (window, cursor);
}

static CtkWidget *
add_section (CtkWidget   *box,
             const gchar *heading)
{
  CtkWidget *label;
  CtkWidget *section;

  label = ctk_label_new (heading);
  ctk_label_set_xalign (CTK_LABEL (label), 0.0);
  ctk_widget_set_margin_top (label, 10);
  ctk_widget_set_margin_bottom (label, 10);
  ctk_box_pack_start (CTK_BOX (box), label, FALSE, TRUE, 0);
  section = ctk_flow_box_new ();
  ctk_widget_set_halign (section, CTK_ALIGN_START);
  ctk_flow_box_set_selection_mode (CTK_FLOW_BOX (section), CTK_SELECTION_NONE);
  ctk_flow_box_set_min_children_per_line (CTK_FLOW_BOX (section), 2);
  ctk_flow_box_set_max_children_per_line (CTK_FLOW_BOX (section), 20);
  ctk_box_pack_start (CTK_BOX (box), section, FALSE, TRUE, 0);

  return section;
}

static void
add_button (CtkWidget   *section,
            const gchar *css_name)
{
  CtkWidget *image, *button;
  CdkDisplay *display;
  CdkCursor *cursor;

  display = ctk_widget_get_display (section);
  cursor = cdk_cursor_new_from_name (display, css_name);
  if (cursor == NULL)
    image = ctk_image_new_from_icon_name ("image-missing", CTK_ICON_SIZE_MENU);
  else
    {
      gchar *path;

      path = g_strdup_printf ("/cursors/%s_cursor.png", css_name);
      g_strdelimit (path, "-", '_');
      image = ctk_image_new_from_resource (path);
      g_free (path);
    }
  ctk_widget_set_size_request (image, 32, 32);
  button = ctk_button_new ();
  ctk_container_add (CTK_CONTAINER (button), image);
  ctk_style_context_add_class (ctk_widget_get_style_context (button), "image-button");
  g_signal_connect (button, "clicked", G_CALLBACK (set_cursor), cursor);

  ctk_widget_set_tooltip_text (button, css_name);
  ctk_container_add (CTK_CONTAINER (section), button);
}

CtkWidget *
do_cursors (CtkWidget *do_widget)
{
  static CtkWidget *window = NULL;

  if (!window)
    {
      CtkWidget *sw;
      CtkWidget *box;
      CtkWidget *section;

      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_screen (CTK_WINDOW (window),
                             ctk_widget_get_screen (do_widget));
      ctk_window_set_title (CTK_WINDOW (window), "Cursors");
      ctk_window_set_default_size (CTK_WINDOW (window), 500, 500);

      g_signal_connect (window, "destroy",
                        G_CALLBACK (ctk_widget_destroyed),
                        &window);

      sw = ctk_scrolled_window_new (NULL, NULL);
      ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (sw),
                                      CTK_POLICY_NEVER,
                                      CTK_POLICY_AUTOMATIC);
      ctk_container_add (CTK_CONTAINER (window), sw);
      box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
      g_object_set (box,
                    "margin-start", 20,
                    "margin-end", 20,
                    "margin-bottom", 10,
                    NULL);
      ctk_container_add (CTK_CONTAINER (sw), box);

      section = add_section (box, "General");
      add_button (section, "default");
      add_button (section, "none");

      section = add_section (box, "Link & Status");
      add_button (section, "context-menu");
      add_button (section, "help");
      add_button (section, "pointer");
      add_button (section, "progress");
      add_button (section, "wait");

      section = add_section (box, "Selection");
      add_button (section, "cell");
      add_button (section, "crosshair");
      add_button (section, "text");
      add_button (section, "vertical-text");

      section = add_section (box, "Drag & Drop");
      add_button (section, "alias");
      add_button (section, "copy");
      add_button (section, "move");
      add_button (section, "no-drop");
      add_button (section, "not-allowed");
      add_button (section, "grab");
      add_button (section, "grabbing");

      section = add_section (box, "Resize & Scrolling");
      add_button (section, "all-scroll");
      add_button (section, "col-resize");
      add_button (section, "row-resize");
      add_button (section, "n-resize");
      add_button (section, "e-resize");
      add_button (section, "s-resize");
      add_button (section, "w-resize");
      add_button (section, "ne-resize");
      add_button (section, "nw-resize");
      add_button (section, "se-resize");
      add_button (section, "sw-resize");
      add_button (section, "ew-resize");
      add_button (section, "ns-resize");
      add_button (section, "nesw-resize");
      add_button (section, "nwse-resize");

      section = add_section (box, "Zoom");
      add_button (section, "zoom-in");
      add_button (section, "zoom-out");
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);


  return window;
}
