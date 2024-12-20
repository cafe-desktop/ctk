#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#include <ctk/ctk.h>
#include <glib/gstdio.h>

#include "demos.h"

static CtkWidget *info_view;
static CtkWidget *source_view;

static gchar *current_file = NULL;

static CtkWidget *notebook;
static CtkWidget *treeview;
static CtkWidget *headerbar;

enum {
  NAME_COLUMN,
  TITLE_COLUMN,
  FILENAME_COLUMN,
  FUNC_COLUMN,
  STYLE_COLUMN,
  NUM_COLUMNS
};

typedef struct _CallbackData CallbackData;
struct _CallbackData
{
  CtkTreeModel *model;
  CtkTreePath *path;
};

static void
activate_about (GSimpleAction *action G_GNUC_UNUSED,
                GVariant      *parameter G_GNUC_UNUSED,
                gpointer       user_data)
{
  CtkApplication *app = user_data;
  const gchar *authors[] = {
    "The CTK+ Team",
    NULL
  };

  ctk_show_about_dialog (CTK_WINDOW (ctk_application_get_active_window (app)),
                         "program-name", "CTK+ Demo",
                         "version", g_strdup_printf ("%s,\nRunning against CTK+ %d.%d.%d",
                                                     PACKAGE_VERSION,
                                                     ctk_get_major_version (),
                                                     ctk_get_minor_version (),
                                                     ctk_get_micro_version ()),
                         "copyright", "(C) 1997-2013 The CTK+ Team",
                         "license-type", CTK_LICENSE_LGPL_2_1,
                         "website", "http://github.com/cafe-desktop/ctk",
                         "comments", "Program to demonstrate CTK+ widgets",
                         "authors", authors,
                         "logo-icon-name", "ctk3-demo",
                         "title", "About CTK+ Demo",
                         NULL);
}

static void
activate_quit (GSimpleAction *action G_GNUC_UNUSED,
               GVariant      *parameter G_GNUC_UNUSED,
               gpointer       user_data)
{
  CtkApplication *app = user_data;
  GList *list, *next;

  list = ctk_application_get_windows (app);
  while (list)
    {
      CtkWidget *win;

      win = list->data;
      next = list->next;

      ctk_widget_destroy (CTK_WIDGET (win));

      list = next;
    }
}

static void
window_closed_cb (CtkWidget *window G_GNUC_UNUSED,
		  gpointer   data)
{
  CallbackData *cbdata = data;
  CtkTreeIter iter;
  PangoStyle style;

  ctk_tree_model_get_iter (cbdata->model, &iter, cbdata->path);
  ctk_tree_model_get (CTK_TREE_MODEL (cbdata->model), &iter,
                      STYLE_COLUMN, &style,
                      -1);
  if (style == PANGO_STYLE_ITALIC)
    ctk_tree_store_set (CTK_TREE_STORE (cbdata->model), &iter,
                        STYLE_COLUMN, PANGO_STYLE_NORMAL,
                        -1);

  ctk_tree_path_free (cbdata->path);
  g_free (cbdata);
}

static void
run_example_for_row (CtkWidget    *window,
                     CtkTreeModel *model,
                     CtkTreeIter  *iter)
{
  PangoStyle style;
  GDoDemoFunc func;

  ctk_tree_model_get (CTK_TREE_MODEL (model),
                      iter,
                      FUNC_COLUMN, &func,
                      STYLE_COLUMN, &style,
                      -1);

  if (func)
    {
      CtkWidget *demo;

      ctk_tree_store_set (CTK_TREE_STORE (model),
                          iter,
                          STYLE_COLUMN, (style == PANGO_STYLE_ITALIC ? PANGO_STYLE_NORMAL : PANGO_STYLE_ITALIC),
                          -1);
      demo = (func) (window);

      if (demo != NULL)
        {
          CallbackData *cbdata;

          cbdata = g_new (CallbackData, 1);
          cbdata->model = model;
          cbdata->path = ctk_tree_model_get_path (model, iter);

          if (ctk_widget_is_toplevel (demo))
            {
              ctk_window_set_transient_for (CTK_WINDOW (demo), CTK_WINDOW (window));
              ctk_window_set_modal (CTK_WINDOW (demo), TRUE);
            }

          g_signal_connect (demo, "destroy",
                            G_CALLBACK (window_closed_cb), cbdata);
        }
    }
}

static void
activate_run (GSimpleAction *action G_GNUC_UNUSED,
              GVariant      *parameter G_GNUC_UNUSED,
              gpointer       user_data)
{
  CtkTreeSelection *selection;
  CtkTreeModel *model;
  CtkTreeIter iter;

  selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (treeview));

  if (ctk_tree_selection_get_selected (selection, &model, &iter)) {
    CtkWidget *window = user_data;
    run_example_for_row (window, model, &iter);
  }
}

/* Stupid syntax highlighting.
 *
 * No regex was used in the making of this highlighting.
 * It should only work for simple cases.  This is good, as
 * that's all we should have in the demos.
 */
/* This code should not be used elsewhere, except perhaps as an example of how
 * to iterate through a text buffer.
 */
enum {
  STATE_NORMAL,
  STATE_IN_COMMENT
};

static gchar *tokens[] =
{
  "/*",
  "\"",
  NULL
};

static gchar *types[] =
{
  "static",
  "const ",
  "void",
  "gint",
  " int ",
  " char ",
  "gchar ",
  "gfloat",
  "float",
  "double",
  "gint8",
  "gint16",
  "gint32",
  "guint",
  "guint8",
  "guint16",
  "guint32",
  "guchar",
  "glong",
  "gboolean" ,
  "gshort",
  "gushort",
  "gulong",
  "gdouble",
  "gldouble",
  "gpointer",
  "NULL",
  "GList",
  "GSList",
  "FALSE",
  "TRUE",
  "FILE ",
  "CtkColorSelection ",
  "CtkWidget ",
  "CtkButton ",
  "CdkColor ",
  "CdkRectangle ",
  "CdkEventExpose ",
  "CdkGC ",
  "GdkPixbufLoader ",
  "GdkPixbuf ",
  "GError",
  "size_t",
  "CtkAboutDialog ",
  "CtkAction ",
  "CtkActionEntry ",
  "CtkRadioActionEntry ",
  "CtkIconFactory ",
  "CtkIconSet ",
  "CtkTextBuffer ",
  "CtkStatusbar ",
  "CtkTextIter ",
  "CtkTextMark ",
  "CdkEventWindowState ",
  "CtkActionGroup ",
  "CtkUIManager ",
  "CtkRadioAction ",
  "CtkActionClass ",
  "CtkToggleActionEntry ",
  "CtkAssistant ",
  "CtkBuilder ",
  "CtkSizeGroup ",
  "CtkTreeModel ",
  "CtkTreeSelection ",
  "CdkDisplay ",
  "CdkScreen ",
  "CdkWindow ",
  "CdkEventButton ",
  "CdkCursor ",
  "CtkTreeIter ",
  "CtkTreeViewColumn ",
  "CdkDisplayManager ",
  "CtkClipboard ",
  "CtkIconSize ",
  "CtkImage ",
  "CdkDragContext ",
  "CtkSelectionData ",
  "CtkDialog ",
  "CtkMenuItem ",
  "CtkListStore ",
  "CtkCellLayout ",
  "CtkCellRenderer ",
  "CtkTreePath ",
  "CtkTreeStore ",
  "CtkEntry ",
  "CtkEditable ",
  "CtkEditableInterface ",
  "CdkPixmap ",
  "CdkEventConfigure ",
  "CdkEventMotion ",
  "CdkModifierType ",
  "CtkEntryCompletion ",
  "CtkToolItem ",
  "GDir ",
  "CtkIconView ",
  "CtkCellRendererText ",
  "CtkContainer ",
  "CtkAccelGroup ",
  "CtkPaned ",
  "CtkPrintOperation ",
  "CtkPrintContext ",
  "cairo_t ",
  "PangoLayout "
  "PangoFontDescription ",
  "PangoRenderer ",
  "PangoMatrix ",
  "PangoContext ",
  "PangoLayout ",
  "CtkTable ",
  "CtkToggleButton ",
  "GString ",
  "CtkIconSize ",
  "CtkTreeView ",
  "CtkTextTag ",
  "CdkEvent ",
  "CdkEventKey ",
  "CtkTextView ",
  "CdkEventVisibility ",
  "CdkBitmap ",
  "CtkTextChildAnchor ",
  "GArray ",
  "CtkCellEditable ",
  "CtkCellRendererToggle ",
  NULL
};

static gchar *control[] =
{
  " if ",
  " while ",
  " else",
  " do ",
  " for ",
  "?",
  ":",
  "return ",
  "goto ",
  NULL
};
void
parse_chars (gchar     *text,
             gchar    **end_ptr,
             gint      *state,
             gchar    **tag,
             gboolean   start)
{
  gint i;
  gchar *next_token;

  /* Handle comments first */
  if (*state == STATE_IN_COMMENT)
    {
      *end_ptr = strstr (text, "*/");
      if (*end_ptr)
        {
          *end_ptr += 2;
          *state = STATE_NORMAL;
          *tag = "comment";
        }
      return;
    }

  *tag = NULL;
  *end_ptr = NULL;

  /* check for comment */
  if (!strncmp (text, "/*", 2))
    {
      *end_ptr = strstr (text, "*/");
      if (*end_ptr)
        *end_ptr += 2;
      else
        *state = STATE_IN_COMMENT;
      *tag = "comment";
      return;
    }

  /* check for preprocessor defines */
  if (*text == '#' && start)
    {
      *end_ptr = NULL;
      *tag = "preprocessor";
      return;
    }

  /* functions */
  if (start && * text != '\t' && *text != ' ' && *text != '{' && *text != '}')
    {
      if (strstr (text, "("))
        {
          *end_ptr = strstr (text, "(");
          *tag = "function";
          return;
        }
    }
  /* check for types */
  for (i = 0; types[i] != NULL; i++)
    if (!strncmp (text, types[i], strlen (types[i])) ||
        (start && types[i][0] == ' ' && !strncmp (text, types[i] + 1, strlen (types[i]) - 1)))
      {
        *end_ptr = text + strlen (types[i]);
        *tag = "type";
        return;
      }

  /* check for control */
  for (i = 0; control[i] != NULL; i++)
    if (!strncmp (text, control[i], strlen (control[i])))
      {
        *end_ptr = text + strlen (control[i]);
        *tag = "control";
        return;
      }

  /* check for string */
  if (text[0] == '"')
    {
      gint maybe_escape = FALSE;

      *end_ptr = text + 1;
      *tag = "string";
      while (**end_ptr != '\000')
        {
          if (**end_ptr == '\"' && !maybe_escape)
            {
              *end_ptr += 1;
              return;
            }
          if (**end_ptr == '\\')
            maybe_escape = TRUE;
          else
            maybe_escape = FALSE;
          *end_ptr += 1;
        }
      return;
    }

  /* not at the start of a tag.  Find the next one. */
  for (i = 0; tokens[i] != NULL; i++)
    {
      next_token = strstr (text, tokens[i]);
      if (next_token)
        {
          if (*end_ptr)
            *end_ptr = (*end_ptr<next_token)?*end_ptr:next_token;
          else
            *end_ptr = next_token;
        }
    }

  for (i = 0; types[i] != NULL; i++)
    {
      next_token = strstr (text, types[i]);
      if (next_token)
        {
          if (*end_ptr)
            *end_ptr = (*end_ptr<next_token)?*end_ptr:next_token;
          else
            *end_ptr = next_token;
        }
    }

  for (i = 0; control[i] != NULL; i++)
    {
      next_token = strstr (text, control[i]);
      if (next_token)
        {
          if (*end_ptr)
            *end_ptr = (*end_ptr<next_token)?*end_ptr:next_token;
          else
            *end_ptr = next_token;
        }
    }
}

/* While not as cool as c-mode, this will do as a quick attempt at highlighting */
static void
fontify (CtkTextBuffer *source_buffer)
{
  CtkTextIter start_iter, next_iter, tmp_iter;
  gint state;
  gchar *text;
  gchar *tag;

  ctk_text_buffer_create_tag (source_buffer, "source",
                              "font", "monospace",
                              NULL);
  ctk_text_buffer_create_tag (source_buffer, "comment",
                              "foreground", "DodgerBlue",
                              NULL);
  ctk_text_buffer_create_tag (source_buffer, "type",
                              "foreground", "ForestGreen",
                              NULL);
  ctk_text_buffer_create_tag (source_buffer, "string",
                              "foreground", "RosyBrown",
                              "weight", PANGO_WEIGHT_BOLD,
                              NULL);
  ctk_text_buffer_create_tag (source_buffer, "control",
                              "foreground", "purple",
                              NULL);
  ctk_text_buffer_create_tag (source_buffer, "preprocessor",
                              "style", PANGO_STYLE_OBLIQUE,
                              "foreground", "burlywood4",
                              NULL);
  ctk_text_buffer_create_tag (source_buffer, "function",
                              "weight", PANGO_WEIGHT_BOLD,
                              "foreground", "DarkGoldenrod4",
                              NULL);

  ctk_text_buffer_get_bounds (source_buffer, &start_iter, &tmp_iter);
  ctk_text_buffer_apply_tag_by_name (source_buffer, "source", &start_iter, &tmp_iter);

  state = STATE_NORMAL;

  ctk_text_buffer_get_iter_at_offset (source_buffer, &start_iter, 0);

  next_iter = start_iter;
  while (ctk_text_iter_forward_line (&next_iter))
    {
      gchar *start_ptr, *end_ptr;
      gboolean start = TRUE;

      start_ptr = text = ctk_text_iter_get_text (&start_iter, &next_iter);

      do
        {
          parse_chars (start_ptr, &end_ptr, &state, &tag, start);

          start = FALSE;
          if (end_ptr)
            {
              tmp_iter = start_iter;
              ctk_text_iter_forward_chars (&tmp_iter, end_ptr - start_ptr);
            }
          else
            {
              tmp_iter = next_iter;
            }
          if (tag)
            ctk_text_buffer_apply_tag_by_name (source_buffer, tag, &start_iter, &tmp_iter);

          start_iter = tmp_iter;
          start_ptr = end_ptr;
        }
      while (end_ptr);

      g_free (text);
      start_iter = next_iter;
    }
}

static CtkWidget *create_text (CtkWidget **text_view, gboolean is_source);

static void
add_data_tab (const gchar *demoname)
{
  gchar *resource_dir;
  gchar **resources;
  guint i;

  resource_dir = g_strconcat ("/", demoname, NULL);
  resources = g_resources_enumerate_children (resource_dir, 0, NULL);
  if (resources == NULL)
    {
      g_free (resource_dir);
      return;
    }

  for (i = 0; resources[i]; i++)
    {
      gchar *resource_name;
      CtkWidget *widget, *label;

      resource_name = g_strconcat (resource_dir, "/", resources[i], NULL);

      widget = ctk_image_new_from_resource (resource_name);
      if (ctk_image_get_pixbuf (CTK_IMAGE (widget)) == NULL &&
          ctk_image_get_animation (CTK_IMAGE (widget)) == NULL)
        {
          GBytes *bytes;

          /* So we've used the best API available to figure out it's
           * not an image. Let's try something else then.
           */
          g_object_ref_sink (widget);
          g_object_unref (widget);

          bytes = g_resources_lookup_data (resource_name, 0, NULL);
          g_assert (bytes);

          if (g_utf8_validate (g_bytes_get_data (bytes, NULL), g_bytes_get_size (bytes), NULL))
            {
              /* Looks like it parses as text. Dump it into a textview then! */
              CtkTextBuffer *buffer;
              CtkWidget *textview;

              widget = create_text (&textview, FALSE);
              buffer = ctk_text_buffer_new (NULL);
              ctk_text_buffer_set_text (buffer, g_bytes_get_data (bytes, NULL), g_bytes_get_size (bytes));
              if (g_str_has_suffix (resource_name, ".c"))
                fontify (buffer);
              ctk_text_view_set_buffer (CTK_TEXT_VIEW (textview), buffer);
            }
          else
            {
              g_warning ("Don't know how to display resource '%s'", resource_name);
              widget = NULL;
            }

          g_bytes_unref (bytes);
        }

      ctk_widget_show_all (widget);
      label = ctk_label_new (resources[i]);
      ctk_widget_show (label);
      ctk_notebook_append_page (CTK_NOTEBOOK (notebook), widget, label);
      ctk_container_child_set (CTK_CONTAINER (notebook),
                               CTK_WIDGET (widget),
                               "tab-expand", TRUE,
                               NULL);

      g_free (resource_name);
    }

  g_strfreev (resources);
  g_free (resource_dir);
}

static void
remove_data_tabs (void)
{
  gint i;

  for (i = ctk_notebook_get_n_pages (CTK_NOTEBOOK (notebook)) - 1; i > 1; i--)
    ctk_notebook_remove_page (CTK_NOTEBOOK (notebook), i);
}

void
load_file (const gchar *demoname,
           const gchar *filename)
{
  CtkTextBuffer *info_buffer, *source_buffer;
  CtkTextIter start, end;
  char *resource_filename;
  GError *err = NULL;
  int state = 0;
  gboolean in_para = 0;
  gchar **lines;
  GBytes *bytes;
  gint i;

  if (!g_strcmp0 (current_file, filename))
    return;

  remove_data_tabs ();

  add_data_tab (demoname);

  g_free (current_file);
  current_file = g_strdup (filename);

  info_buffer = ctk_text_buffer_new (NULL);
  ctk_text_buffer_create_tag (info_buffer, "title",
                              "font", "Sans 18",
                              "pixels-below-lines", 10,
                              NULL);

  source_buffer = ctk_text_buffer_new (NULL);

  resource_filename = g_strconcat ("/sources/", filename, NULL);
  bytes = g_resources_lookup_data (resource_filename, 0, &err);
  g_free (resource_filename);

  if (bytes == NULL)
    {
      g_warning ("Cannot open source for %s: %s", filename, err->message);
      g_error_free (err);
      return;
    }

  lines = g_strsplit (g_bytes_get_data (bytes, NULL), "\n", -1);
  g_bytes_unref (bytes);

  ctk_text_buffer_get_iter_at_offset (info_buffer, &start, 0);
  for (i = 0; lines[i] != NULL; i++)
    {
      gchar *p;
      gchar *q;
      gchar *r;

      /* Make sure \r is stripped at the end for the poor windows people */
      lines[i] = g_strchomp (lines[i]);

      p = lines[i];
      switch (state)
        {
        case 0:
          /* Reading title */
          while (*p == '/' || *p == '*' || g_ascii_isspace (*p))
            p++;
          r = p;
          while (*r != '\0')
            {
              while (*r != '/' && *r != ':' && *r != '\0')
                r++;
              if (*r == '/')
                {
                  r++;
                  p = r;
                }
              if (r[0] == ':' && r[1] == ':')
                *r = '\0';
            }
          q = p + strlen (p);
          while (q > p && g_ascii_isspace (*(q - 1)))
            q--;


          if (q > p)
            {
              int len_chars = g_utf8_pointer_to_offset (p, q);

              end = start;

              g_assert (strlen (p) >= q - p);
              ctk_text_buffer_insert (info_buffer, &end, p, q - p);
              start = end;

              ctk_text_iter_backward_chars (&start, len_chars);
              ctk_text_buffer_apply_tag_by_name (info_buffer, "title", &start, &end);

              start = end;

              while (*p && *p != '\n') p++;

              state++;
            }
          break;

        case 1:
          /* Reading body of info section */
          while (g_ascii_isspace (*p))
            p++;
          if (*p == '*' && *(p + 1) == '/')
            {
              ctk_text_buffer_get_iter_at_offset (source_buffer, &start, 0);
              state++;
            }
          else
            {
              int len;

              while (*p == '*' || g_ascii_isspace (*p))
                p++;

              len = strlen (p);
              while (g_ascii_isspace (*(p + len - 1)))
                len--;

              if (len > 0)
                {
                  if (in_para)
                    ctk_text_buffer_insert (info_buffer, &start, " ", 1);

                  g_assert (strlen (p) >= len);
                  ctk_text_buffer_insert (info_buffer, &start, p, len);
                  in_para = 1;
                }
              else
                {
                  ctk_text_buffer_insert (info_buffer, &start, "\n", 1);
                  in_para = 0;
                }
            }
          break;

        case 2:
          /* Skipping blank lines */
          while (g_ascii_isspace (*p))
            p++;

          if (!*p)
            break;

          p = lines[i];
          state++;
          /* Fall through */

        case 3:
          /* Reading program body */
          ctk_text_buffer_insert (source_buffer, &start, p, -1);
          if (lines[i+1] != NULL)
            ctk_text_buffer_insert (source_buffer, &start, "\n", 1);
          break;
        }
    }

  g_strfreev (lines);

  fontify (source_buffer);

  ctk_text_view_set_buffer (CTK_TEXT_VIEW (source_view), source_buffer);
  g_object_unref (source_buffer);

  ctk_text_view_set_buffer (CTK_TEXT_VIEW (info_view), info_buffer);
  g_object_unref (info_buffer);
}

static void
selection_cb (CtkTreeSelection *selection,
              CtkTreeModel     *model)
{
  CtkTreeIter iter;
  char *name;
  char *filename;
  char *title;

  if (! ctk_tree_selection_get_selected (selection, NULL, &iter))
    return;

  ctk_tree_model_get (model, &iter,
                      NAME_COLUMN, &name,
                      TITLE_COLUMN, &title,
                      FILENAME_COLUMN, &filename,
                      -1);

  if (filename)
    load_file (name, filename);

  ctk_header_bar_set_title (CTK_HEADER_BAR (headerbar), title);

  g_free (name);
  g_free (title);
  g_free (filename);
}

static CtkWidget *
create_text (CtkWidget **view,
             gboolean    is_source)
{
  CtkWidget *scrolled_window;
  CtkWidget *text_view;

  scrolled_window = ctk_scrolled_window_new (NULL, NULL);
  ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (scrolled_window),
                                  CTK_POLICY_AUTOMATIC,
                                  CTK_POLICY_AUTOMATIC);
  ctk_scrolled_window_set_shadow_type (CTK_SCROLLED_WINDOW (scrolled_window),
                                       CTK_SHADOW_NONE);

  *view = text_view = ctk_text_view_new ();
  g_object_set (text_view,
                "left-margin", 20,
                "right-margin", 20,
                "top-margin", 20,
                "bottom-margin", 20,
                NULL);

  ctk_text_view_set_editable (CTK_TEXT_VIEW (text_view), FALSE);
  ctk_text_view_set_cursor_visible (CTK_TEXT_VIEW (text_view), FALSE);

  ctk_container_add (CTK_CONTAINER (scrolled_window), text_view);

  if (is_source)
    {
      ctk_text_view_set_monospace (CTK_TEXT_VIEW (text_view), TRUE);
      ctk_text_view_set_wrap_mode (CTK_TEXT_VIEW (text_view), CTK_WRAP_NONE);
    }
  else
    {
      /* Make it a bit nicer for text. */
      ctk_text_view_set_wrap_mode (CTK_TEXT_VIEW (text_view), CTK_WRAP_WORD);
      ctk_text_view_set_pixels_above_lines (CTK_TEXT_VIEW (text_view), 2);
      ctk_text_view_set_pixels_below_lines (CTK_TEXT_VIEW (text_view), 2);
    }

  return scrolled_window;
}

static void
populate_model (CtkTreeModel *model)
{
  Demo *d = ctk_demos;

  /* this code only supports 1 level of children. If we
   * want more we probably have to use a recursing function.
   */
  while (d->title)
    {
      Demo *children = d->children;
      CtkTreeIter iter;

      ctk_tree_store_append (CTK_TREE_STORE (model), &iter, NULL);

      ctk_tree_store_set (CTK_TREE_STORE (model),
                          &iter,
                          NAME_COLUMN, d->name,
                          TITLE_COLUMN, d->title,
                          FILENAME_COLUMN, d->filename,
                          FUNC_COLUMN, d->func,
                          STYLE_COLUMN, PANGO_STYLE_NORMAL,
                          -1);

      d++;

      if (!children)
        continue;

      while (children->title)
        {
          CtkTreeIter child_iter;

          ctk_tree_store_append (CTK_TREE_STORE (model), &child_iter, &iter);

          ctk_tree_store_set (CTK_TREE_STORE (model),
                              &child_iter,
                              NAME_COLUMN, children->name,
                              TITLE_COLUMN, children->title,
                              FILENAME_COLUMN, children->filename,
                              FUNC_COLUMN, children->func,
                              STYLE_COLUMN, PANGO_STYLE_NORMAL,
                              -1);

          children++;
        }
    }

}

static void
startup (GApplication *app)
{
  CtkBuilder *builder;
  GMenuModel *appmenu;
  gchar *ids[] = { "appmenu", NULL };

  builder = ctk_builder_new ();
  ctk_builder_add_objects_from_resource (builder, "/ui/appmenu.ui", ids, NULL);

  appmenu = (GMenuModel *)ctk_builder_get_object (builder, "appmenu");

  ctk_application_set_app_menu (CTK_APPLICATION (app), appmenu);

  g_object_unref (builder);
}

static void
row_activated_cb (CtkWidget         *tree_view,
                  CtkTreePath       *path,
                  CtkTreeViewColumn *column G_GNUC_UNUSED)
{
  CtkTreeIter iter;
  CtkWidget *window;
  CtkTreeModel *model;

  window = ctk_widget_get_toplevel (tree_view);
  model = ctk_tree_view_get_model (CTK_TREE_VIEW (tree_view));
  ctk_tree_model_get_iter (model, &iter, path);

  run_example_for_row (window, model, &iter);
}

static void
start_cb (CtkMenuItem *item G_GNUC_UNUSED,
	  CtkWidget   *scrollbar)
{
  CtkAdjustment *adj;

  adj = ctk_range_get_adjustment (CTK_RANGE (scrollbar));
  ctk_adjustment_set_value (adj, ctk_adjustment_get_lower (adj));
}

static void
end_cb (CtkMenuItem *item G_GNUC_UNUSED,
	CtkWidget   *scrollbar)
{
  CtkAdjustment *adj;

  adj = ctk_range_get_adjustment (CTK_RANGE (scrollbar));
  ctk_adjustment_set_value (adj, ctk_adjustment_get_upper (adj) - ctk_adjustment_get_page_size (adj));
}

static gboolean
scrollbar_popup (CtkWidget *scrollbar G_GNUC_UNUSED,
		 CtkWidget *menu)
{
  ctk_menu_popup_at_pointer (CTK_MENU (menu), NULL);

  return TRUE;
}

static void
activate (GApplication *app)
{
  CtkBuilder *builder;
  CtkWindow *window;
  CtkWidget *widget;
  CtkTreeModel *model;
  CtkTreeIter iter;
  GError *error = NULL;
  CtkWidget *sw;
  CtkWidget *scrollbar;
  CtkWidget *menu;
  CtkWidget *item;

  static GActionEntry win_entries[] = {
    { .name = "run", .activate = activate_run }
  };

  builder = ctk_builder_new ();
  ctk_builder_add_from_resource (builder, "/ui/main.ui", &error);
  if (error != NULL)
    {
      g_critical ("%s", error->message);
      exit (1);
    }

  window = (CtkWindow *)ctk_builder_get_object (builder, "window");
  ctk_application_add_window (CTK_APPLICATION (app), window);
  g_action_map_add_action_entries (G_ACTION_MAP (window),
                                   win_entries, G_N_ELEMENTS (win_entries),
                                   window);

  notebook = (CtkWidget *)ctk_builder_get_object (builder, "notebook");

  info_view = (CtkWidget *)ctk_builder_get_object (builder, "info-textview");
  source_view = (CtkWidget *)ctk_builder_get_object (builder, "source-textview");
  headerbar = (CtkWidget *)ctk_builder_get_object (builder, "headerbar");
  treeview = (CtkWidget *)ctk_builder_get_object (builder, "treeview");
  model = ctk_tree_view_get_model (CTK_TREE_VIEW (treeview));

  sw = (CtkWidget *)ctk_builder_get_object (builder, "source-scrolledwindow");
  scrollbar = ctk_scrolled_window_get_vscrollbar (CTK_SCROLLED_WINDOW (sw));

  menu = ctk_menu_new ();

  item = ctk_menu_item_new_with_label ("Start");
  g_signal_connect (item, "activate", G_CALLBACK (start_cb), scrollbar);
  ctk_menu_shell_append (CTK_MENU_SHELL (menu), item);

  item = ctk_menu_item_new_with_label ("End");
  g_signal_connect (item, "activate", G_CALLBACK (end_cb), scrollbar);
  ctk_menu_shell_append (CTK_MENU_SHELL (menu), item);

  ctk_widget_show_all (menu);

  g_signal_connect (scrollbar, "popup-menu", G_CALLBACK (scrollbar_popup), menu);

  load_file (ctk_demos[0].name, ctk_demos[0].filename);

  populate_model (model);

  g_signal_connect (treeview, "row-activated", G_CALLBACK (row_activated_cb), model);

  widget = (CtkWidget *)ctk_builder_get_object (builder, "treeview-selection");
  g_signal_connect (widget, "changed", G_CALLBACK (selection_cb), model);

  ctk_tree_model_get_iter_first (ctk_tree_view_get_model (CTK_TREE_VIEW (treeview)), &iter);
  ctk_tree_selection_select_iter (CTK_TREE_SELECTION (widget), &iter);

  ctk_tree_view_collapse_all (CTK_TREE_VIEW (treeview));

  ctk_widget_show_all (CTK_WIDGET (window));

  g_object_unref (builder);
}

static gboolean
auto_quit (gpointer data)
{
  g_application_quit (G_APPLICATION (data));
  return G_SOURCE_REMOVE;
}

static void
list_demos (void)
{
  Demo *d;

  d = ctk_demos;

  while (d->title)
    {
      Demo *c;

      c = d->children;
      if (d->name)
        g_print ("%s\n", d->name);
      d++;
      while (c && c->title)
        {
          if (c->name)
            g_print ("%s\n", c->name);
          c++;
        }
    }
}

static gint
command_line (GApplication            *app,
              GApplicationCommandLine *cmdline)
{
  GVariantDict *options;
  const gchar *name = NULL;
  gboolean autoquit = FALSE;
  gboolean list = FALSE;
  Demo *d, *c;
  GDoDemoFunc func = 0;
  CtkWidget *window, *demo;

  activate (app);

  options = g_application_command_line_get_options_dict (cmdline);
  g_variant_dict_lookup (options, "run", "&s", &name);
  g_variant_dict_lookup (options, "autoquit", "b", &autoquit);
  g_variant_dict_lookup (options, "list", "b", &list);

  if (list)
    {
      list_demos ();
      g_application_quit (app);
      return 0;
    }

  if (name == NULL)
    goto out;

  window = ctk_application_get_windows (CTK_APPLICATION (app))->data;

  d = ctk_demos;

  while (d->title)
    {
      c = d->children;
      if (g_strcmp0 (d->name, name) == 0)
        {
          func = d->func;
          goto out;
        }
      d++;
      while (c && c->title)
        {
          if (g_strcmp0 (c->name, name) == 0)
            {
              func = c->func;
              goto out;
            }
          c++;
        }
    }

out:
  if (func)
    {
      demo = (func) (window);

      ctk_window_set_transient_for (CTK_WINDOW (demo), CTK_WINDOW (window));
      ctk_window_set_modal (CTK_WINDOW (demo), TRUE);
    }

  if (autoquit)
    g_timeout_add_seconds (1, auto_quit, app);

  return 0;
}

static void
print_version (void)
{
  g_print ("ctk3-demo %d.%d.%d\n",
           ctk_get_major_version (),
           ctk_get_minor_version (),
           ctk_get_micro_version ());
}

static int
local_options (GApplication *app G_GNUC_UNUSED,
               GVariantDict *options,
               gpointer      data G_GNUC_UNUSED)
{
  gboolean version = FALSE;

  g_variant_dict_lookup (options, "version", "b", &version);

  if (version)
    {
      print_version ();
      return 0;
    }

  return -1;
}

int
main (int argc, char **argv)
{
  CtkApplication *app;
  static GActionEntry app_entries[] = {
    { .name = "about", .activate = activate_about },
    { .name = "quit", .activate = activate_quit },
  };

  /* Most code in ctk-demo is intended to be exemplary, but not
   * these few lines, which are just a hack so ctk-demo will work
   * in the CTK tree without installing it.
   */
  if (g_file_test ("../../modules/input/immodules.cache", G_FILE_TEST_EXISTS))
    {
      g_setenv ("CTK_IM_MODULE_FILE", "../../modules/input/immodules.cache", TRUE);
    }
  /* -- End of hack -- */

  app = ctk_application_new ("org.ctk.Demo", G_APPLICATION_NON_UNIQUE|G_APPLICATION_HANDLES_COMMAND_LINE);

  g_action_map_add_action_entries (G_ACTION_MAP (app),
                                   app_entries, G_N_ELEMENTS (app_entries),
                                   app);

  g_application_add_main_option (G_APPLICATION (app), "version", 0, 0, G_OPTION_ARG_NONE, "Show program version", NULL);
  g_application_add_main_option (G_APPLICATION (app), "run", 0, 0, G_OPTION_ARG_STRING, "Run an example", "EXAMPLE");
  g_application_add_main_option (G_APPLICATION (app), "list", 0, 0, G_OPTION_ARG_NONE, "List examples", NULL);
  g_application_add_main_option (G_APPLICATION (app), "autoquit", 0, 0, G_OPTION_ARG_NONE, "Quit after a delay", NULL);

  g_signal_connect (app, "startup", G_CALLBACK (startup), NULL);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  g_signal_connect (app, "command-line", G_CALLBACK (command_line), NULL);
  g_signal_connect (app, "handle-local-options", G_CALLBACK (local_options), NULL);

  g_application_run (G_APPLICATION (app), argc, argv);

  return 0;
}
