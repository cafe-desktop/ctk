#include <string.h>
#include <ctk/ctk.h>

/* test that margins and non-zero allocation x/y
 * of the main widget are handled correctly
 */
static CtkWidget *
test_nonzerox (void)
{
  CtkWidget *win;
  CtkWidget *grid;
  CtkWidget *overlay;
  CtkWidget *text;
  CtkWidget *child;
  GdkRGBA color;

  win = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_title (CTK_WINDOW (win), "Non-zero X");

  grid = ctk_grid_new ();
  g_object_set (grid, "margin", 5, NULL);
  ctk_container_add (CTK_CONTAINER (win), grid);
  ctk_grid_attach (CTK_GRID (grid), ctk_label_new ("Above"), 1, 0, 1, 1);
  ctk_grid_attach (CTK_GRID (grid), ctk_label_new ("Below"), 1, 2, 1, 1);
  ctk_grid_attach (CTK_GRID (grid), ctk_label_new ("Left"), 0, 1, 1, 1);
  ctk_grid_attach (CTK_GRID (grid), ctk_label_new ("Right"), 2, 1, 1, 1);

  overlay = ctk_overlay_new ();
  cdk_rgba_parse (&color, "red");
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  ctk_widget_override_background_color (overlay, 0, &color);
G_GNUC_END_IGNORE_DEPRECATIONS
  ctk_grid_attach (CTK_GRID (grid), overlay, 1, 1, 1, 1);

  text = ctk_text_view_new ();
  ctk_widget_set_size_request (text, 200, 200);
  ctk_widget_set_hexpand (text, TRUE);
  ctk_widget_set_vexpand (text, TRUE);
  ctk_container_add (CTK_CONTAINER (overlay), text);

  child = ctk_label_new ("I'm the overlay");
  ctk_widget_set_halign (child, CTK_ALIGN_START);
  ctk_widget_set_valign (child, CTK_ALIGN_START);
  g_object_set (child, "margin", 3, NULL);
  ctk_overlay_add_overlay (CTK_OVERLAY (overlay), child);

  child = ctk_label_new ("No, I'm the overlay");
  ctk_widget_set_halign (child, CTK_ALIGN_END);
  ctk_widget_set_valign (child, CTK_ALIGN_END);
  ctk_overlay_add_overlay (CTK_OVERLAY (overlay), child);
  g_object_set (child, "margin", 3, NULL);

  return win;
}

static gboolean
get_child_position (CtkOverlay    *overlay,
                    CtkWidget     *widget,
                    CtkAllocation *alloc,
                    CtkWidget     *relative)
{
  CtkRequisition req;
  CtkWidget *child;
  CtkAllocation main_alloc;
  gint x, y;

  child = ctk_bin_get_child (CTK_BIN (overlay));

  ctk_widget_translate_coordinates (relative, child, 0, 0, &x, &y);
  main_alloc.x = x;
  main_alloc.y = y;
  main_alloc.width = ctk_widget_get_allocated_width (relative);
  main_alloc.height = ctk_widget_get_allocated_height (relative);

  ctk_widget_get_preferred_size (widget, NULL, &req);

  alloc->x = main_alloc.x;
  alloc->width = MIN (main_alloc.width, req.width);
  if (ctk_widget_get_halign (widget) == CTK_ALIGN_END)
    alloc->x += main_alloc.width - req.width;

  alloc->y = main_alloc.y;
  alloc->height = MIN (main_alloc.height, req.height);
  if (ctk_widget_get_valign (widget) == CTK_ALIGN_END)
    alloc->y += main_alloc.height - req.height;

  return TRUE;
}

/* test custom positioning */
static CtkWidget *
test_relative (void)
{
  CtkWidget *win;
  CtkWidget *grid;
  CtkWidget *overlay;
  CtkWidget *text;
  CtkWidget *child;
  GdkRGBA color;

  win = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_title (CTK_WINDOW (win), "Custom positioning");

  overlay = ctk_overlay_new ();
  cdk_rgba_parse (&color, "yellow");
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  ctk_widget_override_background_color (overlay, 0, &color);
G_GNUC_END_IGNORE_DEPRECATIONS
  ctk_container_add (CTK_CONTAINER (win), overlay);

  grid = ctk_grid_new ();
  ctk_container_add (CTK_CONTAINER (overlay), grid);
  ctk_grid_attach (CTK_GRID (grid), ctk_label_new ("Above"), 1, 0, 1, 1);
  ctk_grid_attach (CTK_GRID (grid), ctk_label_new ("Below"), 1, 2, 1, 1);
  ctk_grid_attach (CTK_GRID (grid), ctk_label_new ("Left"), 0, 1, 1, 1);
  ctk_grid_attach (CTK_GRID (grid), ctk_label_new ("Right"), 2, 1, 1, 1);

  text = ctk_text_view_new ();
  ctk_widget_set_size_request (text, 200, 200);
  g_object_set (text, "margin", 5, NULL);
  ctk_widget_set_hexpand (text, TRUE);
  ctk_widget_set_vexpand (text, TRUE);
  ctk_grid_attach (CTK_GRID (grid), text, 1, 1, 1, 1);
  g_signal_connect (overlay, "get-child-position",
                    G_CALLBACK (get_child_position), text);

  child = ctk_label_new ("Top left overlay");
  ctk_widget_set_halign (child, CTK_ALIGN_START);
  ctk_widget_set_valign (child, CTK_ALIGN_START);
  ctk_overlay_add_overlay (CTK_OVERLAY (overlay), child);
  g_object_set (child, "margin", 1, NULL);

  child = ctk_label_new ("Bottom right overlay");
  ctk_widget_set_halign (child, CTK_ALIGN_END);
  ctk_widget_set_valign (child, CTK_ALIGN_END);
  ctk_overlay_add_overlay (CTK_OVERLAY (overlay), child);
  g_object_set (child, "margin", 1, NULL);

  return win;
}

/* test CTK_ALIGN_FILL handling */
static CtkWidget *
test_fullwidth (void)
{
  CtkWidget *win;
  CtkWidget *overlay;
  CtkWidget *text;
  CtkWidget *child;

  win = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_title (CTK_WINDOW (win), "Full-width");

  overlay = ctk_overlay_new ();
  ctk_container_add (CTK_CONTAINER (win), overlay);

  text = ctk_text_view_new ();
  ctk_widget_set_size_request (text, 200, 200);
  ctk_widget_set_hexpand (text, TRUE);
  ctk_widget_set_vexpand (text, TRUE);
  ctk_container_add (CTK_CONTAINER (overlay), text);

  child = ctk_label_new ("Fullwidth top overlay");
  ctk_widget_set_halign (child, CTK_ALIGN_FILL);
  ctk_widget_set_valign (child, CTK_ALIGN_START);
  ctk_overlay_add_overlay (CTK_OVERLAY (overlay), child);
  g_object_set (child, "margin", 4, NULL);

  return win;
}

/* test that scrolling works as expected */
static CtkWidget *
test_scrolling (void)
{
  CtkWidget *win;
  CtkWidget *overlay;
  CtkWidget *sw;
  CtkWidget *text;
  CtkWidget *child;
  CtkTextBuffer *buffer;
  gchar *contents;
  gsize len;

  win = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_title (CTK_WINDOW (win), "Scrolling");

  overlay = ctk_overlay_new ();
  ctk_container_add (CTK_CONTAINER (win), overlay);

  sw = ctk_scrolled_window_new (NULL, NULL);
  ctk_scrolled_window_set_min_content_width (CTK_SCROLLED_WINDOW (sw), 200);
  ctk_scrolled_window_set_min_content_height (CTK_SCROLLED_WINDOW (sw), 200);
  ctk_container_add (CTK_CONTAINER (overlay), sw);

  text = ctk_text_view_new ();
  buffer = ctk_text_buffer_new (NULL);
  if (!g_file_get_contents ("testoverlay.c", &contents, &len, NULL))
    {
      contents = g_strdup ("Text should go here...");
      len = strlen (contents);
    }
  ctk_text_buffer_set_text (buffer, contents, len);
  g_free (contents);
  ctk_text_view_set_buffer (CTK_TEXT_VIEW (text), buffer);

  ctk_widget_set_hexpand (text, TRUE);
  ctk_widget_set_vexpand (text, TRUE);
  ctk_container_add (CTK_CONTAINER (sw), text);

  child = ctk_label_new ("This should be visible");
  ctk_widget_set_halign (child, CTK_ALIGN_CENTER);
  ctk_widget_set_valign (child, CTK_ALIGN_END);
  ctk_overlay_add_overlay (CTK_OVERLAY (overlay), child);
  g_object_set (child, "margin", 4, NULL);

  return win;
}

static const gchar *buffer =
"<interface>"
"  <object class='CtkWindow' id='window'>"
"    <property name='title'>CtkBuilder support</property>"
"    <child>"
"      <object class='CtkOverlay' id='overlay'>"
"        <child type='overlay'>"
"          <object class='CtkLabel' id='overlay-child'>"
"            <property name='label'>Witty remark goes here</property>"
"            <property name='halign'>end</property>"
"            <property name='valign'>end</property>"
"            <property name='margin'>4</property>"
"          </object>"
"        </child>"
"        <child>"
"          <object class='CtkGrid' id='grid'>"
"            <child>"
"              <object class='CtkLabel' id='left'>"
"                <property name='label'>Left</property>"
"              </object>"
"              <packing>"
"                <property name='left_attach'>0</property>"
"                <property name='top_attach'>0</property>"
"              </packing>"
"            </child>"
"            <child>"
"              <object class='CtkLabel' id='right'>"
"                <property name='label'>Right</property>"
"              </object>"
"              <packing>"
"                <property name='left_attach'>2</property>"
"                <property name='top_attach'>0</property>"
"              </packing>"
"            </child>"
"            <child>"
"              <object class='CtkTextView' id='text'>"
"                 <property name='width-request'>200</property>"
"                 <property name='height-request'>200</property>"
"                 <property name='hexpand'>True</property>"
"                 <property name='vexpand'>True</property>"
"              </object>"
"              <packing>"
"                <property name='left_attach'>1</property>"
"                <property name='top_attach'>0</property>"
"              </packing>"
"            </child>"
"          </object>"
"        </child>"
"      </object>"
"    </child>"
"  </object>"
"</interface>";

/* test that overlays can be constructed with CtkBuilder */
static CtkWidget *
test_builder (void)
{
  CtkBuilder *builder;
  CtkWidget *win;
  GError *error;

  builder = ctk_builder_new ();

  error = NULL;
  if (!ctk_builder_add_from_string (builder, buffer, -1, &error))
    {
      g_warning ("%s", error->message);
      g_error_free (error);
      return NULL;
    }

  win = (CtkWidget *)ctk_builder_get_object (builder, "window");
  g_object_ref (win);

  g_object_unref (builder);

  return win;
}

static void
on_enter (CtkWidget *overlay, GdkEventCrossing *event, CtkWidget *child)
{
  if (event->window != ctk_widget_get_window (child))
    return;

  if (ctk_widget_get_halign (child) == CTK_ALIGN_START)
    ctk_widget_set_halign (child, CTK_ALIGN_END);
  else
    ctk_widget_set_halign (child, CTK_ALIGN_START);

  ctk_widget_queue_resize (overlay);
}

static CtkWidget *
test_chase (void)
{
  CtkWidget *win;
  CtkWidget *overlay;
  CtkWidget *sw;
  CtkWidget *text;
  CtkWidget *child;
  CtkTextBuffer *buffer;
  gchar *contents;
  gsize len;

  win = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_title (CTK_WINDOW (win), "Chase");

  overlay = ctk_overlay_new ();
  ctk_widget_set_events (overlay, GDK_ENTER_NOTIFY_MASK|GDK_LEAVE_NOTIFY_MASK);
  ctk_container_add (CTK_CONTAINER (win), overlay);

  sw = ctk_scrolled_window_new (NULL, NULL);
  ctk_scrolled_window_set_min_content_width (CTK_SCROLLED_WINDOW (sw), 200);
  ctk_scrolled_window_set_min_content_height (CTK_SCROLLED_WINDOW (sw), 200);
  ctk_container_add (CTK_CONTAINER (overlay), sw);

  text = ctk_text_view_new ();
  buffer = ctk_text_buffer_new (NULL);
  if (!g_file_get_contents ("testoverlay.c", &contents, &len, NULL))
    {
      contents = g_strdup ("Text should go here...");
      len = strlen (contents);
    }
  ctk_text_buffer_set_text (buffer, contents, len);
  g_free (contents);
  ctk_text_view_set_buffer (CTK_TEXT_VIEW (text), buffer);

  ctk_widget_set_hexpand (text, TRUE);
  ctk_widget_set_vexpand (text, TRUE);
  ctk_container_add (CTK_CONTAINER (sw), text);

  child = ctk_label_new ("Try to enter");
  ctk_widget_set_halign (child, CTK_ALIGN_START);
  ctk_widget_set_valign (child, CTK_ALIGN_END);
  ctk_overlay_add_overlay (CTK_OVERLAY (overlay), child);
  g_object_set (child, "margin", 4, NULL);

  g_signal_connect (overlay, "enter-notify-event",
                    G_CALLBACK (on_enter), child);
  return win;
}

static CtkWidget *
test_stacking (void)
{
  CtkWidget *win;
  CtkWidget *overlay;
  CtkWidget *main_child;
  CtkWidget *label;
  CtkWidget *child;
  CtkWidget *grid;
  CtkWidget *check1;
  CtkWidget *check2;
  GdkRGBA color;

  win = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_title (CTK_WINDOW (win), "Stacking");

  grid = ctk_grid_new ();
  overlay = ctk_overlay_new ();
  main_child = ctk_event_box_new ();
  cdk_rgba_parse (&color, "green");
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  ctk_widget_override_background_color (main_child, 0, &color);
G_GNUC_END_IGNORE_DEPRECATIONS
  ctk_widget_set_hexpand (main_child, TRUE);
  ctk_widget_set_vexpand (main_child, TRUE);
  label = ctk_label_new ("Main child");
  child = ctk_label_new ("Overlay");
  ctk_widget_set_halign (child, CTK_ALIGN_END);
  ctk_widget_set_valign (child, CTK_ALIGN_END);

  check1 = ctk_check_button_new_with_label ("Show main");
  g_object_bind_property (main_child, "visible", check1, "active", G_BINDING_BIDIRECTIONAL);

  check2 = ctk_check_button_new_with_label ("Show overlay");
  g_object_bind_property (child, "visible", check2, "active", G_BINDING_BIDIRECTIONAL);
  ctk_container_add (CTK_CONTAINER (main_child), label);
  ctk_container_add (CTK_CONTAINER (overlay), main_child);
  ctk_overlay_add_overlay (CTK_OVERLAY (overlay), child);
  ctk_grid_attach (CTK_GRID (grid), overlay, 1, 0, 1, 3);
  ctk_container_add (CTK_CONTAINER (win), grid);

  ctk_grid_attach (CTK_GRID (grid), check1, 0, 0, 1, 1);
  ctk_grid_attach (CTK_GRID (grid), check2, 0, 1, 1, 1);
  child = ctk_label_new ("");
  ctk_widget_set_vexpand (child, TRUE);
  ctk_grid_attach (CTK_GRID (grid), child, 0, 2, 1, 1);

  return win;
}

static CtkWidget *
test_input_stacking (void)
{
  CtkWidget *win;
  CtkWidget *overlay;
  CtkWidget *label, *entry;
  CtkWidget *grid;
  CtkWidget *button;
  CtkWidget *vbox;
  int i,j;

  win = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_title (CTK_WINDOW (win), "Input Stacking");

  overlay = ctk_overlay_new ();
  grid = ctk_grid_new ();
  ctk_container_add (CTK_CONTAINER (overlay), grid);

  for (j = 0; j < 5; j++)
    {
      for (i = 0; i < 5; i++)
	{
	  button = ctk_button_new_with_label ("     ");
	  ctk_widget_set_hexpand (button, TRUE);
	  ctk_widget_set_vexpand (button, TRUE);
	  ctk_grid_attach (CTK_GRID (grid), button, i, j, 1, 1);
	}
    }

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 10);
  ctk_overlay_add_overlay (CTK_OVERLAY (overlay), vbox);
  ctk_overlay_set_overlay_pass_through (CTK_OVERLAY (overlay), vbox, TRUE);
  ctk_widget_set_halign (vbox, CTK_ALIGN_CENTER);
  ctk_widget_set_valign (vbox, CTK_ALIGN_CENTER);

  label = ctk_label_new ("This is some overlaid text\n"
			 "It does not get input\n"
			 "But the entry does");
  ctk_box_pack_start (CTK_BOX (vbox), label, FALSE, FALSE, 8);

  entry = ctk_entry_new ();
  ctk_box_pack_start (CTK_BOX (vbox), entry, FALSE, FALSE, 8);


  ctk_container_add (CTK_CONTAINER (win), overlay);

  return win;
}

static void
reorder_overlay (CtkButton *button, CtkOverlay *overlay)
{
  ctk_overlay_reorder_overlay (overlay, ctk_widget_get_parent (CTK_WIDGET (button)), -1);
}

static CtkWidget *
test_child_order (void)
{
  CtkWidget *win;
  CtkWidget *overlay;
  CtkWidget *button;
  CtkWidget *label;
  CtkWidget *ebox;
  GdkRGBA color;
  int i;

  win = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_title (CTK_WINDOW (win), "Child Order");

  overlay = ctk_overlay_new ();
  ctk_container_add (CTK_CONTAINER (win), overlay);

  for (i = 0; i < 4; i++)
    {
      char *colors[] = {
	"rgba(255,0,0,0.8)", "rgba(0,255,0,0.8)", "rgba(0,0,255,0.8)", "rgba(255,0,255,0.8)"
      };
      ebox = ctk_event_box_new ();
      button = ctk_button_new_with_label (g_strdup_printf ("Child %d", i));
      g_signal_connect (button, "clicked", G_CALLBACK (reorder_overlay), overlay);
      ctk_widget_set_margin_start (button, 20);
      ctk_widget_set_margin_end (button, 20);
      ctk_widget_set_margin_top (button, 10);
      ctk_widget_set_margin_bottom (button, 10);

      ctk_container_add (CTK_CONTAINER (ebox), button);

      cdk_rgba_parse (&color, colors[i]);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      ctk_widget_override_background_color (ebox, 0, &color);
 G_GNUC_END_IGNORE_DEPRECATIONS
      ctk_widget_set_halign (ebox, (i == 0 || i == 3) ? CTK_ALIGN_START : CTK_ALIGN_END);
      ctk_widget_set_valign (ebox, i < 2 ? CTK_ALIGN_START : CTK_ALIGN_END);
      ctk_overlay_add_overlay (CTK_OVERLAY (overlay), ebox);
    }

  ebox = ctk_event_box_new ();
  cdk_rgba_parse (&color, "white");
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  ctk_widget_override_background_color (ebox, 0, &color);
G_GNUC_END_IGNORE_DEPRECATIONS

  label = ctk_label_new ("Main\n"
			 "Main\n"
			 "Main\n"
			 "Main\n");
  ctk_container_add (CTK_CONTAINER (ebox), label);
  ctk_container_add (CTK_CONTAINER (overlay), ebox);

  return win;
}


int
main (int argc, char *argv[])
{
  CtkWidget *win1;
  CtkWidget *win2;
  CtkWidget *win3;
  CtkWidget *win4;
  CtkWidget *win5;
  CtkWidget *win6;
  CtkWidget *win7;
  CtkWidget *win8;
  CtkWidget *win9;

  ctk_init (&argc, &argv);

  if (g_getenv ("RTL"))
    ctk_widget_set_default_direction (CTK_TEXT_DIR_RTL);

  win1 = test_nonzerox ();
  ctk_widget_show_all (win1);

  win2 = test_relative ();
  ctk_widget_show_all (win2);

  win3 = test_fullwidth ();
  ctk_widget_show_all (win3);

  win4 = test_scrolling ();
  ctk_widget_show_all (win4);

  win5 = test_builder ();
  ctk_widget_show_all (win5);

  win6 = test_chase ();
  ctk_widget_show_all (win6);

  win7 = test_stacking ();
  ctk_widget_show_all (win7);

  win8 = test_input_stacking ();
  ctk_widget_show_all (win8);

  win9 = test_child_order ();
  ctk_widget_show_all (win9);

  ctk_main ();

  return 0;
}
