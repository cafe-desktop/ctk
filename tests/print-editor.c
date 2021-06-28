#include <math.h>
#include <pango/pangocairo.h>
#include <ctk/ctk.h>

static CtkWidget *main_window;
static char *filename = NULL;
static CtkPageSetup *page_setup = NULL;
static CtkPrintSettings *settings = NULL;
static gboolean file_changed = FALSE;
static CtkTextBuffer *buffer;
static CtkWidget *statusbar;
static GList *active_prints = NULL;

static void
update_title (CtkWindow *window)
{
  char *basename;
  char *title;

  if (filename == NULL)
    basename = g_strdup ("Untitled");
  else
    basename = g_path_get_basename (filename);

  title = g_strdup_printf ("Simple Editor with printing - %s", basename);
  g_free (basename);

  ctk_window_set_title (window, title);
  g_free (title);
}

static void
update_statusbar (void)
{
  gchar *msg;
  gint row, col;
  CtkTextIter iter;
  const char *print_str;

  ctk_statusbar_pop (CTK_STATUSBAR (statusbar), 0);
  
  ctk_text_buffer_get_iter_at_mark (buffer,
                                    &iter,
                                    ctk_text_buffer_get_insert (buffer));

  row = ctk_text_iter_get_line (&iter);
  col = ctk_text_iter_get_line_offset (&iter);

  print_str = "";
  if (active_prints)
    {
      CtkPrintOperation *op = active_prints->data;
      print_str = ctk_print_operation_get_status_string (op);
    }
  
  msg = g_strdup_printf ("%d, %d%s %s",
                         row, col,
			 file_changed?" - Modified":"",
			 print_str);

  ctk_statusbar_push (CTK_STATUSBAR (statusbar), 0, msg);

  g_free (msg);
}

static void
update_ui (void)
{
  update_title (CTK_WINDOW (main_window));
  update_statusbar ();
}

static char *
get_text (void)
{
  CtkTextIter start, end;

  ctk_text_buffer_get_start_iter (buffer, &start);
  ctk_text_buffer_get_end_iter (buffer, &end);
  return ctk_text_buffer_get_text (buffer, &start, &end, FALSE);
}

static void
set_text (const char *text, gsize len)
{
  ctk_text_buffer_set_text (buffer, text, len);
  file_changed = FALSE;
  update_ui ();
}

static void
load_file (const char *open_filename)
{
  CtkWidget *error_dialog;
  char *contents;
  GError *error;
  gsize len;

  error_dialog = NULL;
  error = NULL;
  if (g_file_get_contents (open_filename, &contents, &len, &error))
    {
      if (g_utf8_validate (contents, len, NULL))
	{
	  filename = g_strdup (open_filename);
	  set_text (contents, len);
	  g_free (contents);
	}
      else
	{
	  error_dialog = ctk_message_dialog_new (CTK_WINDOW (main_window),
						 CTK_DIALOG_DESTROY_WITH_PARENT,
						 CTK_MESSAGE_ERROR,
						 CTK_BUTTONS_CLOSE,
						 "Error loading file %s:\n%s",
						 open_filename,
						 "Not valid utf8");
	}
    }
  else
    {
      error_dialog = ctk_message_dialog_new (CTK_WINDOW (main_window),
					     CTK_DIALOG_DESTROY_WITH_PARENT,
					     CTK_MESSAGE_ERROR,
					     CTK_BUTTONS_CLOSE,
					     "Error loading file %s:\n%s",
					     open_filename,
					     error->message);
      
      g_error_free (error);
    }
  if (error_dialog)
    {
      g_signal_connect (error_dialog, "response", G_CALLBACK (ctk_widget_destroy), NULL);
      ctk_widget_show (error_dialog);
    }
}


static void
save_file (const char *save_filename)
{
  char *text = get_text ();
  CtkWidget *error_dialog;
  GError *error;

  error = NULL;
  if (g_file_set_contents (save_filename,
			   text, -1, &error))
    {
      if (save_filename != filename)
	{
	  g_free (filename);
	  filename = g_strdup (save_filename);
	}
      file_changed = FALSE;
      update_ui ();
    }
  else
    {
      error_dialog = ctk_message_dialog_new (CTK_WINDOW (main_window),
					     CTK_DIALOG_DESTROY_WITH_PARENT,
					     CTK_MESSAGE_ERROR,
					     CTK_BUTTONS_CLOSE,
					     "Error saving to file %s:\n%s",
					     filename,
					     error->message);
      
      g_signal_connect (error_dialog, "response", G_CALLBACK (ctk_widget_destroy), NULL);
      ctk_widget_show (error_dialog);
      
      g_error_free (error);
    }
}


typedef struct {
  char *text;
  PangoLayout *layout;
  GList *page_breaks;
  CtkWidget *font_button;
  char *font;
} PrintData;

static void
begin_print (CtkPrintOperation *operation,
	     CtkPrintContext *context,
	     PrintData *print_data)
{
  PangoFontDescription *desc;
  PangoLayoutLine *layout_line;
  double width, height;
  double page_height;
  GList *page_breaks;
  int num_lines;
  int line;

  width = ctk_print_context_get_width (context);
  height = ctk_print_context_get_height (context);

  print_data->layout = ctk_print_context_create_pango_layout (context);

  desc = pango_font_description_from_string (print_data->font);
  pango_layout_set_font_description (print_data->layout, desc);
  pango_font_description_free (desc);

  pango_layout_set_width (print_data->layout, width * PANGO_SCALE);
  
  pango_layout_set_text (print_data->layout, print_data->text, -1);

  num_lines = pango_layout_get_line_count (print_data->layout);

  page_breaks = NULL;
  page_height = 0;

  for (line = 0; line < num_lines; line++)
    {
      PangoRectangle ink_rect, logical_rect;
      double line_height;
      
      layout_line = pango_layout_get_line (print_data->layout, line);
      pango_layout_line_get_extents (layout_line, &ink_rect, &logical_rect);

      line_height = logical_rect.height / 1024.0;

      if (page_height + line_height > height)
	{
	  page_breaks = g_list_prepend (page_breaks, GINT_TO_POINTER (line));
	  page_height = 0;
	}

      page_height += line_height;
    }

  page_breaks = g_list_reverse (page_breaks);
  ctk_print_operation_set_n_pages (operation, g_list_length (page_breaks) + 1);
  
  print_data->page_breaks = page_breaks;
}

static void
draw_page (CtkPrintOperation *operation,
	   CtkPrintContext *context,
	   int page_nr,
	   PrintData *print_data)
{
  cairo_t *cr;
  GList *pagebreak;
  int start, end, i;
  PangoLayoutIter *iter;
  double start_pos;

  if (page_nr == 0)
    start = 0;
  else
    {
      pagebreak = g_list_nth (print_data->page_breaks, page_nr - 1);
      start = GPOINTER_TO_INT (pagebreak->data);
    }

  pagebreak = g_list_nth (print_data->page_breaks, page_nr);
  if (pagebreak == NULL)
    end = pango_layout_get_line_count (print_data->layout);
  else
    end = GPOINTER_TO_INT (pagebreak->data);
    
  cr = ctk_print_context_get_cairo_context (context);

  cairo_set_source_rgb (cr, 0, 0, 0);
  
  i = 0;
  start_pos = 0;
  iter = pango_layout_get_iter (print_data->layout);
  do
    {
      PangoRectangle   logical_rect;
      PangoLayoutLine *line;
      int              baseline;

      if (i >= start)
	{
	  line = pango_layout_iter_get_line (iter);

	  pango_layout_iter_get_line_extents (iter, NULL, &logical_rect);
	  baseline = pango_layout_iter_get_baseline (iter);
	  
	  if (i == start)
	    start_pos = logical_rect.y / 1024.0;
	  
	  cairo_move_to (cr, logical_rect.x / 1024.0, baseline / 1024.0 - start_pos);
	  
	  pango_cairo_show_layout_line  (cr, line);
	}
      i++;
    }
  while (i < end &&
	 pango_layout_iter_next_line (iter));

  pango_layout_iter_free (iter);
}

static void
status_changed_cb (CtkPrintOperation *op,
		   gpointer user_data)
{
  if (ctk_print_operation_is_finished (op))
    {
      active_prints = g_list_remove (active_prints, op);
      g_object_unref (op);
    }
  update_statusbar ();
}

static CtkWidget *
create_custom_widget (CtkPrintOperation *operation,
		      PrintData *data)
{
  CtkWidget *vbox, *hbox, *font, *label;

  ctk_print_operation_set_custom_tab_label (operation, "Other");
  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  ctk_container_set_border_width (CTK_CONTAINER (vbox), 12);

  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 8);
  ctk_box_pack_start (CTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  ctk_widget_show (hbox);

  label = ctk_label_new ("Font:");
  ctk_box_pack_start (CTK_BOX (hbox), label, FALSE, FALSE, 0);
  ctk_widget_show (label);
  
  font = ctk_font_button_new_with_font  (data->font);
  ctk_box_pack_start (CTK_BOX (hbox), font, FALSE, FALSE, 0);
  ctk_widget_show (font);
  data->font_button = font;

  return vbox;
}

static void
custom_widget_apply (CtkPrintOperation *operation,
		     CtkWidget *widget,
		     PrintData *data)
{
  const char *selected_font;
  selected_font = ctk_font_chooser_get_font (CTK_FONT_CHOOSER (data->font_button));

  g_free (data->font);
  data->font = g_strdup (selected_font);
}

typedef struct 
{
  CtkPrintOperation *op;
  CtkPrintOperationPreview *preview;
  CtkPrintContext   *context;
  CtkWidget         *spin;
  CtkWidget         *area;
  gint               page;
  PrintData *data;
  gdouble dpi_x, dpi_y;
} PreviewOp;

static gboolean
preview_draw (CtkWidget *widget,
              cairo_t   *cr,
              gpointer   data)
{
  PreviewOp *pop = data;
  cairo_t *prev_cr;
  double dpi_x, dpi_y;

  prev_cr = ctk_print_context_get_cairo_context (pop->context);
  cairo_reference (prev_cr);
  dpi_x = ctk_print_context_get_dpi_x (pop->context);
  dpi_y = ctk_print_context_get_dpi_y (pop->context);

  ctk_print_context_set_cairo_context (pop->context,
                                       cr, dpi_x, dpi_y);
  ctk_print_operation_preview_render_page (pop->preview,
					   pop->page - 1);
  ctk_print_context_set_cairo_context (pop->context,
                                       prev_cr, dpi_x, dpi_y);
  cairo_destroy (prev_cr);

  return TRUE;
}

static void
preview_ready (CtkPrintOperationPreview *preview,
	       CtkPrintContext          *context,
	       gpointer                  data)
{
  PreviewOp *pop = data;
  gint n_pages;

  g_object_get (pop->op, "n-pages", &n_pages, NULL);

  ctk_spin_button_set_range (CTK_SPIN_BUTTON (pop->spin), 
			     1.0, n_pages);

  g_signal_connect (pop->area, "draw",
		    G_CALLBACK (preview_draw),
		    pop);

  ctk_widget_queue_draw (pop->area);
}

static void
preview_got_page_size (CtkPrintOperationPreview *preview, 
		       CtkPrintContext          *context,
		       CtkPageSetup             *page_setup,
		       gpointer                  data)
{
  PreviewOp *pop = data;
  CtkAllocation allocation;
  CtkPaperSize *paper_size;
  double w, h;
  cairo_t *cr;
  gdouble dpi_x, dpi_y;

  paper_size = ctk_page_setup_get_paper_size (page_setup);

  w = ctk_paper_size_get_width (paper_size, CTK_UNIT_INCH);
  h = ctk_paper_size_get_height (paper_size, CTK_UNIT_INCH);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  cr = cdk_cairo_create (ctk_widget_get_window (pop->area));
G_GNUC_END_IGNORE_DEPRECATIONS

  ctk_widget_get_allocation (pop->area, &allocation);
  dpi_x = allocation.width/w;
  dpi_y = allocation.height/h;

  if (fabs (dpi_x - pop->dpi_x) > 0.001 ||
      fabs (dpi_y - pop->dpi_y) > 0.001)
    {
      ctk_print_context_set_cairo_context (context, cr, dpi_x, dpi_y);
      pop->dpi_x = dpi_x;
      pop->dpi_y = dpi_y;
    }

  pango_cairo_update_layout (cr, pop->data->layout);
  cairo_destroy (cr);
}

static void
update_page (CtkSpinButton *widget,
	     gpointer       data)
{
  PreviewOp *pop = data;

  pop->page = ctk_spin_button_get_value_as_int (widget);
  ctk_widget_queue_draw (pop->area);
}

static void
preview_destroy (CtkWindow *window, 
		 PreviewOp *pop)
{
  ctk_print_operation_preview_end_preview (pop->preview);
  g_object_unref (pop->op);

  g_free (pop);
}

static gboolean 
preview_cb (CtkPrintOperation        *op,
	    CtkPrintOperationPreview *preview,
	    CtkPrintContext          *context,
	    CtkWindow                *parent,
	    gpointer                  data)
{
  CtkWidget *window, *close, *page, *hbox, *vbox, *da;
  gdouble width, height;
  cairo_t *cr;
  PreviewOp *pop;
  PrintData *print_data = data;

  pop = g_new0 (PreviewOp, 1);

  pop->data = print_data;

  width = 200;
  height = 300;
  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_transient_for (CTK_WINDOW (window), 
				CTK_WINDOW (main_window));
  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  ctk_container_add (CTK_CONTAINER (window), vbox);
  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
  ctk_box_pack_start (CTK_BOX (vbox), hbox,
		      FALSE, FALSE, 0);
  page = ctk_spin_button_new_with_range (1, 100, 1);
  ctk_box_pack_start (CTK_BOX (hbox), page, FALSE, FALSE, 0);
  
  close = ctk_button_new_with_label ("Close");
  ctk_box_pack_start (CTK_BOX (hbox), close, FALSE, FALSE, 0);

  da = ctk_drawing_area_new ();
  ctk_widget_set_size_request (CTK_WIDGET (da), width, height);
  ctk_box_pack_start (CTK_BOX (vbox), da, TRUE, TRUE, 0);

  ctk_widget_realize (da);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  cr = cdk_cairo_create (ctk_widget_get_window (da));
G_GNUC_END_IGNORE_DEPRECATIONS

  /* TODO: What dpi to use here? This will be used for pagination.. */
  ctk_print_context_set_cairo_context (context, cr, 72, 72);
  cairo_destroy (cr);
  
  pop->op = g_object_ref (op);
  pop->preview = preview;
  pop->context = context;
  pop->spin = page;
  pop->area = da;
  pop->page = 1;

  g_signal_connect (page, "value-changed", 
		    G_CALLBACK (update_page), pop);
  g_signal_connect_swapped (close, "clicked", 
			    G_CALLBACK (ctk_widget_destroy), window);

  g_signal_connect (preview, "ready",
		    G_CALLBACK (preview_ready), pop);
  g_signal_connect (preview, "got-page-size",
		    G_CALLBACK (preview_got_page_size), pop);

  g_signal_connect (window, "destroy", 
                    G_CALLBACK (preview_destroy), pop);
                            
  ctk_widget_show_all (window);
  
  return TRUE;
}

static void
print_done (CtkPrintOperation *op,
	    CtkPrintOperationResult res,
	    PrintData *print_data)
{
  GError *error = NULL;

  if (res == CTK_PRINT_OPERATION_RESULT_ERROR)
    {

      CtkWidget *error_dialog;
      
      ctk_print_operation_get_error (op, &error);
      
      error_dialog = ctk_message_dialog_new (CTK_WINDOW (main_window),
					     CTK_DIALOG_DESTROY_WITH_PARENT,
					     CTK_MESSAGE_ERROR,
					     CTK_BUTTONS_CLOSE,
					     "Error printing file:\n%s",
					     error ? error->message : "no details");
      g_signal_connect (error_dialog, "response", G_CALLBACK (ctk_widget_destroy), NULL);
      ctk_widget_show (error_dialog);
    }
  else if (res == CTK_PRINT_OPERATION_RESULT_APPLY)
    {
      if (settings != NULL)
	g_object_unref (settings);
      settings = g_object_ref (ctk_print_operation_get_print_settings (op));
    }

  g_free (print_data->text);
  g_free (print_data->font);
  g_free (print_data);
  
  if (!ctk_print_operation_is_finished (op))
    {
      g_object_ref (op);
      active_prints = g_list_append (active_prints, op);
      update_statusbar ();
      
      /* This ref is unref:ed when we get the final state change */
      g_signal_connect (op, "status_changed",
			G_CALLBACK (status_changed_cb), NULL);
    }
}

static void
end_print (CtkPrintOperation *op, CtkPrintContext *context, PrintData *print_data)
{
  g_list_free (print_data->page_breaks);
  print_data->page_breaks = NULL;
  g_object_unref (print_data->layout);
  print_data->layout = NULL;
}

static void
print_or_preview (GSimpleAction *action, CtkPrintOperationAction print_action)
{
  CtkPrintOperation *print;
  PrintData *print_data;

  print_data = g_new0 (PrintData, 1);

  print_data->text = get_text ();
  print_data->font = g_strdup ("Sans 12");

  print = ctk_print_operation_new ();

  ctk_print_operation_set_track_print_status (print, TRUE);

  if (settings != NULL)
    ctk_print_operation_set_print_settings (print, settings);

  if (page_setup != NULL)
    ctk_print_operation_set_default_page_setup (print, page_setup);

  g_signal_connect (print, "begin_print", G_CALLBACK (begin_print), print_data);
  g_signal_connect (print, "end-print", G_CALLBACK (end_print), print_data);
  g_signal_connect (print, "draw_page", G_CALLBACK (draw_page), print_data);
  g_signal_connect (print, "create_custom_widget", G_CALLBACK (create_custom_widget), print_data);
  g_signal_connect (print, "custom_widget_apply", G_CALLBACK (custom_widget_apply), print_data);
  g_signal_connect (print, "preview", G_CALLBACK (preview_cb), print_data);

  g_signal_connect (print, "done", G_CALLBACK (print_done), print_data);

  ctk_print_operation_set_export_filename (print, "test.pdf");

#if 0
  ctk_print_operation_set_allow_async (print, TRUE);
#endif
  ctk_print_operation_run (print, print_action, CTK_WINDOW (main_window), NULL);

  g_object_unref (print);
}

static void
activate_page_setup (GSimpleAction *action,
                     GVariant      *parameter,
                     gpointer       user_data)
{
  CtkPageSetup *new_page_setup;

  new_page_setup = ctk_print_run_page_setup_dialog (CTK_WINDOW (main_window),
                                                    page_setup, settings);

  if (page_setup)
    g_object_unref (page_setup);

  page_setup = new_page_setup;
}

static void
activate_print (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       user_data)
{
  print_or_preview (action, CTK_PRINT_OPERATION_ACTION_PRINT_DIALOG);
}

static void
activate_preview (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       user_data)
{
  print_or_preview (action, CTK_PRINT_OPERATION_ACTION_PREVIEW);
}

static void
activate_save_as (GSimpleAction *action,
                  GVariant      *parameter,
                  gpointer       user_data)
{
  CtkWidget *dialog;
  gint response;
  char *save_filename;

  dialog = ctk_file_chooser_dialog_new ("Select file",
                                        CTK_WINDOW (main_window),
                                        CTK_FILE_CHOOSER_ACTION_SAVE,
                                        "_Cancel", CTK_RESPONSE_CANCEL,
                                        "_Save", CTK_RESPONSE_OK,
                                        NULL);
  ctk_dialog_set_default_response (CTK_DIALOG (dialog), CTK_RESPONSE_OK);
  response = ctk_dialog_run (CTK_DIALOG (dialog));

  if (response == CTK_RESPONSE_OK)
    {
      save_filename = ctk_file_chooser_get_filename (CTK_FILE_CHOOSER (dialog));
      save_file (save_filename);
      g_free (save_filename);
    }

  ctk_widget_destroy (dialog);
}

static void
activate_save (GSimpleAction *action,
               GVariant      *parameter,
               gpointer       user_data)
{
  if (filename == NULL)
    activate_save_as (action, NULL, NULL);
  else
    save_file (filename);
}

static void
activate_open (GSimpleAction *action,
               GVariant      *parameter,
               gpointer       user_data)
{
  CtkWidget *dialog;
  gint response;
  char *open_filename;

  dialog = ctk_file_chooser_dialog_new ("Select file",
                                        CTK_WINDOW (main_window),
                                        CTK_FILE_CHOOSER_ACTION_OPEN,
                                        "_Cancel", CTK_RESPONSE_CANCEL,
                                        "_Open", CTK_RESPONSE_OK,
                                        NULL);
  ctk_dialog_set_default_response (CTK_DIALOG (dialog), CTK_RESPONSE_OK);
  response = ctk_dialog_run (CTK_DIALOG (dialog));

  if (response == CTK_RESPONSE_OK)
    {
      open_filename = ctk_file_chooser_get_filename (CTK_FILE_CHOOSER (dialog));
      load_file (open_filename);
      g_free (open_filename);
    }

  ctk_widget_destroy (dialog);
}

static void
activate_new (GSimpleAction *action,
              GVariant      *parameter,
              gpointer       user_data)
{
  g_free (filename);
  filename = NULL;
  set_text ("", 0);
}

static void
activate_about (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       user_data)
{
  const gchar *authors[] = {
    "Alexander Larsson",
    NULL
  };
  ctk_show_about_dialog (CTK_WINDOW (main_window),
                         "name", "Print Test Editor",
                         "logo-icon-name", "text-editor",
                         "version", "0.1",
                         "copyright", "(C) Red Hat, Inc",
                         "comments", "Program to demonstrate CTK+ printing.",
                         "authors", authors,
                         NULL);
}

static void
activate_quit (GSimpleAction *action,
               GVariant      *parameter,
               gpointer       user_data)
{
  CtkApplication *app = user_data;
  CtkWidget *win;
  GList *list, *next;

  list = ctk_application_get_windows (app);
  while (list)
    {
      win = list->data;
      next = list->next;

      ctk_widget_destroy (CTK_WIDGET (win));

      list = next;
    }
}

static GActionEntry app_entries[] = {
  { "new", activate_new, NULL, NULL, NULL },
  { "open", activate_open, NULL, NULL, NULL },
  { "save", activate_save, NULL, NULL, NULL },
  { "save-as", activate_save_as, NULL, NULL, NULL },
  { "quit", activate_quit, NULL, NULL, NULL },
  { "about", activate_about, NULL, NULL, NULL },
  { "page-setup", activate_page_setup, NULL, NULL, NULL },
  { "preview", activate_preview, NULL, NULL, NULL },
  { "print", activate_print, NULL, NULL, NULL }
};

static const gchar ui_info[] =
  "<interface>"
  "  <menu id='appmenu'>"
  "    <section>"
  "      <item>"
  "        <attribute name='label'>_About</attribute>"
  "        <attribute name='action'>app.about</attribute>"
  "        <attribute name='accel'>&lt;Primary&gt;a</attribute>"
  "      </item>"
  "    </section>"
  "    <section>"
  "      <item>"
  "        <attribute name='label'>_Quit</attribute>"
  "        <attribute name='action'>app.quit</attribute>"
  "        <attribute name='accel'>&lt;Primary&gt;q</attribute>"
  "      </item>"
  "    </section>"
  "  </menu>"
  "  <menu id='menubar'>"
  "    <submenu>"
  "      <attribute name='label'>_File</attribute>"
  "      <section>"
  "        <item>"
  "          <attribute name='label'>_New</attribute>"
  "          <attribute name='action'>app.new</attribute>"
  "          <attribute name='accel'>&lt;Primary&gt;n</attribute>"
  "        </item>"
  "        <item>"
  "          <attribute name='label'>_Open</attribute>"
  "          <attribute name='action'>app.open</attribute>"
  "        </item>"
  "        <item>"
  "          <attribute name='label'>_Save</attribute>"
  "          <attribute name='action'>app.save</attribute>"
  "          <attribute name='accel'>&lt;Primary&gt;s</attribute>"
  "        </item>"
  "        <item>"
  "          <attribute name='label'>Save _As...</attribute>"
  "          <attribute name='action'>app.save-as</attribute>"
  "          <attribute name='accel'>&lt;Primary&gt;s</attribute>"
  "        </item>"
  "      </section>"
  "      <section>"
  "        <item>"
  "          <attribute name='label'>Page Setup</attribute>"
  "          <attribute name='action'>app.page-setup</attribute>"
  "        </item>"
  "        <item>"
  "          <attribute name='label'>Preview</attribute>"
  "          <attribute name='action'>app.preview</attribute>"
  "        </item>"
  "        <item>"
  "          <attribute name='label'>Print</attribute>"
  "          <attribute name='action'>app.print</attribute>"
  "        </item>"
  "      </section>"
  "    </submenu>"
  "  </menu>"
  "</interface>";

static void
buffer_changed_callback (CtkTextBuffer *buffer)
{
  file_changed = TRUE;
  update_statusbar ();
}

static void
mark_set_callback (CtkTextBuffer     *buffer,
                   const CtkTextIter *new_location,
                   CtkTextMark       *mark,
                   gpointer           data)
{
  update_statusbar ();
}

static gint
command_line (GApplication            *application,
              GApplicationCommandLine *command_line)
{
  int argc;
  char **argv;

  argv = g_application_command_line_get_arguments (command_line, &argc);

  if (argc == 2)
    load_file (argv[1]);

  return 0;
}

static void
startup (GApplication *app)
{
  CtkBuilder *builder;
  GMenuModel *appmenu;
  GMenuModel *menubar;

  builder = ctk_builder_new ();
  ctk_builder_add_from_string (builder, ui_info, -1, NULL);

  appmenu = (GMenuModel *)ctk_builder_get_object (builder, "appmenu");
  menubar = (GMenuModel *)ctk_builder_get_object (builder, "menubar");

  ctk_application_set_app_menu (CTK_APPLICATION (app), appmenu);
  ctk_application_set_menubar (CTK_APPLICATION (app), menubar);

  g_object_unref (builder);
}

static void
activate (GApplication *app)
{
  CtkWidget *box;
  CtkWidget *bar;
  CtkWidget *sw;
  CtkWidget *contents;

  main_window = ctk_application_window_new (CTK_APPLICATION (app));
  ctk_window_set_icon_name (CTK_WINDOW (main_window), "text-editor");
  ctk_window_set_default_size (CTK_WINDOW (main_window), 400, 600);
  update_title (CTK_WINDOW (main_window));

  box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  ctk_container_add (CTK_CONTAINER (main_window), box);

  bar = ctk_menu_bar_new ();
  ctk_widget_show (bar);
  ctk_container_add (CTK_CONTAINER (box), bar);

  /* Create document  */
  sw = ctk_scrolled_window_new (NULL, NULL);

  ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (sw),
                                  CTK_POLICY_AUTOMATIC,
                                  CTK_POLICY_AUTOMATIC);

  ctk_scrolled_window_set_shadow_type (CTK_SCROLLED_WINDOW (sw),
                                       CTK_SHADOW_IN);

  ctk_widget_set_vexpand (sw, TRUE);
  ctk_container_add (CTK_CONTAINER (box), sw);

  contents = ctk_text_view_new ();
  ctk_widget_grab_focus (contents);

  ctk_container_add (CTK_CONTAINER (sw),
                     contents);

  /* Create statusbar */
  statusbar = ctk_statusbar_new ();
  ctk_container_add (CTK_CONTAINER (box), statusbar);

  /* Show text widget info in the statusbar */
  buffer = ctk_text_view_get_buffer (CTK_TEXT_VIEW (contents));

  g_signal_connect_object (buffer,
                           "changed",
                           G_CALLBACK (buffer_changed_callback),
                           NULL,
                           0);

  g_signal_connect_object (buffer,
                           "mark_set", /* cursor moved */
                           G_CALLBACK (mark_set_callback),
                           NULL,
                           0);

  update_ui ();

  ctk_widget_show_all (main_window);
}

int
main (int argc, char **argv)
{
  CtkApplication *app;
  GError *error = NULL;

  ctk_init (NULL, NULL);

  settings = ctk_print_settings_new_from_file ("print-settings.ini", &error);
  if (error) {
    g_print ("Failed to load print settings: %s\n", error->message);
    g_clear_error (&error);

    settings = ctk_print_settings_new ();
  }
  g_assert (settings != NULL);

  page_setup = ctk_page_setup_new_from_file ("page-setup.ini", &error);
  if (error) {
    g_print ("Failed to load page setup: %s\n", error->message);
    g_clear_error (&error);
  }

  app = ctk_application_new ("org.ctk.PrintEditor", 0);

  g_action_map_add_action_entries (G_ACTION_MAP (app),
                                   app_entries, G_N_ELEMENTS (app_entries),
                                   app);

  g_signal_connect (app, "startup", G_CALLBACK (startup), NULL);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  g_signal_connect (app, "command-line", G_CALLBACK (command_line), NULL);

  g_application_run (G_APPLICATION (app), argc, argv);

  if (!ctk_print_settings_to_file (settings, "print-settings.ini", &error)) {
    g_print ("Failed to save print settings: %s\n", error->message);
    g_clear_error (&error);
  }
  if (page_setup &&
      !ctk_page_setup_to_file (page_setup, "page-setup.ini", &error)) {
    g_print ("Failed to save page setup: %s\n", error->message);
    g_clear_error (&error);
  }

  return 0;
}
