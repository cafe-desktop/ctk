/*
 * Copyright (c) 2013 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#include "css-editor.h"

#include "ctkcssprovider.h"
#include "ctkstyleprovider.h"
#include "ctkstylecontext.h"
#include "ctktextview.h"
#include "ctkmessagedialog.h"
#include "ctkfilechooserdialog.h"
#include "ctktogglebutton.h"
#include "ctklabel.h"
#include "ctktooltip.h"
#include "ctktextiter.h"


struct _CtkInspectorCssEditorPrivate
{
  CtkWidget *view;
  CtkTextBuffer *text;
  CtkCssProvider *provider;
  CtkToggleButton *disable_button;
  guint timeout;
  GList *errors;
};

typedef struct {
  GError *error;
  CtkTextIter start;
  CtkTextIter end;
} CssError;

static void
css_error_free (gpointer data)
{
  CssError *error = data;
  g_error_free (error->error);
  g_free (error);
}

static gboolean
query_tooltip_cb (CtkWidget             *widget G_GNUC_UNUSED,
                  gint                   x,
                  gint                   y,
                  gboolean               keyboard_tip,
                  CtkTooltip            *tooltip,
                  CtkInspectorCssEditor *ce)
{
  CtkTextIter iter;
  GList *l;

  if (keyboard_tip)
    {
      gint offset;

      g_object_get (ce->priv->text, "cursor-position", &offset, NULL);
      ctk_text_buffer_get_iter_at_offset (ce->priv->text, &iter, offset);
    }
  else
    {
      gint bx, by, trailing;

      ctk_text_view_window_to_buffer_coords (CTK_TEXT_VIEW (ce->priv->view), CTK_TEXT_WINDOW_TEXT,
                                             x, y, &bx, &by);
      ctk_text_view_get_iter_at_position (CTK_TEXT_VIEW (ce->priv->view), &iter, &trailing, bx, by);
    }

  for (l = ce->priv->errors; l; l = l->next)
    {
      CssError *css_error = l->data;

      if (ctk_text_iter_in_range (&iter, &css_error->start, &css_error->end))
        {
          ctk_tooltip_set_text (tooltip, css_error->error->message);
          return TRUE;
        }
    }

  return FALSE;
}

G_DEFINE_TYPE_WITH_PRIVATE (CtkInspectorCssEditor, ctk_inspector_css_editor, CTK_TYPE_BOX)

static void
set_initial_text (CtkInspectorCssEditor *ce)
{
  gchar *initial_text;
  initial_text = g_strconcat ("/*\n",
                              _("You can type here any CSS rule recognized by CTK+."), "\n",
                              _("You can temporarily disable this custom CSS by clicking on the “Pause” button above."), "\n\n",
                              _("Changes are applied instantly and globally, for the whole application."), "\n",
                              "*/\n\n", NULL);
  ctk_text_buffer_set_text (CTK_TEXT_BUFFER (ce->priv->text), initial_text, -1);
  g_free (initial_text);
}

static void
disable_toggled (CtkToggleButton       *button,
                 CtkInspectorCssEditor *ce)
{
  if (ctk_toggle_button_get_active (button))
    ctk_style_context_remove_provider_for_screen (cdk_screen_get_default (),
                                                  CTK_STYLE_PROVIDER (ce->priv->provider));
  else
    ctk_style_context_add_provider_for_screen (cdk_screen_get_default (),
                                               CTK_STYLE_PROVIDER (ce->priv->provider),
                                               CTK_STYLE_PROVIDER_PRIORITY_USER);
}

static gchar *
get_current_text (CtkTextBuffer *buffer)
{
  CtkTextIter start, end;

  ctk_text_buffer_get_start_iter (buffer, &start);
  ctk_text_buffer_get_end_iter (buffer, &end);
  ctk_text_buffer_remove_all_tags (buffer, &start, &end);

  return ctk_text_buffer_get_text (buffer, &start, &end, FALSE);
}

static void
save_to_file (CtkInspectorCssEditor *ce,
              const gchar           *filename)
{
  gchar *text;
  GError *error = NULL;

  text = get_current_text (ce->priv->text);

  if (!g_file_set_contents (filename, text, -1, &error))
    {
      CtkWidget *dialog;

      dialog = ctk_message_dialog_new (CTK_WINDOW (ctk_widget_get_toplevel (CTK_WIDGET (ce))),
                                       CTK_DIALOG_MODAL|CTK_DIALOG_DESTROY_WITH_PARENT,
                                       CTK_MESSAGE_INFO,
                                       CTK_BUTTONS_OK,
                                       _("Saving CSS failed"));
      ctk_message_dialog_format_secondary_text (CTK_MESSAGE_DIALOG (dialog),
                                                "%s", error->message);
      g_signal_connect (dialog, "response", G_CALLBACK (ctk_widget_destroy), NULL);
      ctk_widget_show (dialog);
      g_error_free (error);
    }

  g_free (text);
}

static void
save_response (CtkWidget             *dialog,
               gint                   response,
               CtkInspectorCssEditor *ce)
{
  ctk_widget_hide (dialog);

  if (response == CTK_RESPONSE_ACCEPT)
    {
      gchar *filename;

      filename = ctk_file_chooser_get_filename (CTK_FILE_CHOOSER (dialog));
      save_to_file (ce, filename);
      g_free (filename);
    }

  ctk_widget_destroy (dialog);
}

static void
save_clicked (CtkButton             *button G_GNUC_UNUSED,
              CtkInspectorCssEditor *ce)
{
  CtkWidget *dialog;

  dialog = ctk_file_chooser_dialog_new ("",
                                        CTK_WINDOW (ctk_widget_get_toplevel (CTK_WIDGET (ce))),
                                        CTK_FILE_CHOOSER_ACTION_SAVE,
                                        _("_Cancel"), CTK_RESPONSE_CANCEL,
                                        _("_Save"), CTK_RESPONSE_ACCEPT,
                                        NULL);
  ctk_file_chooser_set_current_name (CTK_FILE_CHOOSER (dialog), "custom.css");
  ctk_dialog_set_default_response (CTK_DIALOG (dialog), CTK_RESPONSE_ACCEPT);
  ctk_window_set_modal (CTK_WINDOW (dialog), TRUE);
  ctk_file_chooser_set_do_overwrite_confirmation (CTK_FILE_CHOOSER (dialog), TRUE);
  g_signal_connect (dialog, "response", G_CALLBACK (save_response), ce);
  ctk_widget_show (dialog);
}

static void
update_style (CtkInspectorCssEditor *ce)
{
  gchar *text;

  g_list_free_full (ce->priv->errors, css_error_free);
  ce->priv->errors = NULL;

  text = get_current_text (ce->priv->text);
  ctk_css_provider_load_from_data (ce->priv->provider, text, -1, NULL);
  g_free (text);
}

static gboolean
update_timeout (gpointer data)
{
  CtkInspectorCssEditor *ce = data;

  ce->priv->timeout = 0;

  update_style (ce);

  return G_SOURCE_REMOVE;
}

static void
text_changed (CtkTextBuffer         *buffer G_GNUC_UNUSED,
              CtkInspectorCssEditor *ce)
{
  if (ce->priv->timeout != 0)
    g_source_remove (ce->priv->timeout);

  ce->priv->timeout = g_timeout_add (100, update_timeout, ce); 

  g_list_free_full (ce->priv->errors, css_error_free);
  ce->priv->errors = NULL;
}

static void
show_parsing_error (CtkCssProvider        *provider G_GNUC_UNUSED,
                    CtkCssSection         *section,
                    const GError          *error,
                    CtkInspectorCssEditor *ce)
{
  const char *tag_name;
  CtkTextBuffer *buffer = CTK_TEXT_BUFFER (ce->priv->text);
  CssError *css_error;

  css_error = g_new (CssError, 1);
  css_error->error = g_error_copy (error);

  ctk_text_buffer_get_iter_at_line_index (buffer,
                                          &css_error->start,
                                          ctk_css_section_get_start_line (section),
                                          ctk_css_section_get_start_position (section));
  ctk_text_buffer_get_iter_at_line_index (buffer,
                                          &css_error->end,
                                          ctk_css_section_get_end_line (section),
                                          ctk_css_section_get_end_position (section));

  if (g_error_matches (error, CTK_CSS_PROVIDER_ERROR, CTK_CSS_PROVIDER_ERROR_DEPRECATED))
    tag_name = "warning";
  else
    tag_name = "error";

  if (ctk_text_iter_equal (&css_error->start, &css_error->end))
    ctk_text_iter_forward_char (&css_error->end);

  ctk_text_buffer_apply_tag_by_name (buffer, tag_name, &css_error->start, &css_error->end);

  ce->priv->errors = g_list_prepend (ce->priv->errors, css_error);
}

static void
create_provider (CtkInspectorCssEditor *ce)
{
  ce->priv->provider = ctk_css_provider_new ();
  ctk_style_context_add_provider_for_screen (cdk_screen_get_default (),
                                             CTK_STYLE_PROVIDER (ce->priv->provider),
                                             CTK_STYLE_PROVIDER_PRIORITY_USER);

  g_signal_connect (ce->priv->provider, "parsing-error",
                    G_CALLBACK (show_parsing_error), ce);
}

static void
destroy_provider (CtkInspectorCssEditor *ce)
{
  ctk_style_context_remove_provider_for_screen (cdk_screen_get_default (),
                                                CTK_STYLE_PROVIDER (ce->priv->provider));
  g_clear_object (&ce->priv->provider);
}

static void
ctk_inspector_css_editor_init (CtkInspectorCssEditor *ce)
{
  ce->priv = ctk_inspector_css_editor_get_instance_private (ce);
  ctk_widget_init_template (CTK_WIDGET (ce));
}

static void
constructed (GObject *object)
{
  CtkInspectorCssEditor *ce = CTK_INSPECTOR_CSS_EDITOR (object);

  create_provider (ce);
  set_initial_text (ce);
}

static void
finalize (GObject *object)
{
  CtkInspectorCssEditor *ce = CTK_INSPECTOR_CSS_EDITOR (object);

  if (ce->priv->timeout != 0)
    g_source_remove (ce->priv->timeout);

  destroy_provider (ce);

  g_list_free_full (ce->priv->errors, css_error_free);

  G_OBJECT_CLASS (ctk_inspector_css_editor_parent_class)->finalize (object);
}

static void
ctk_inspector_css_editor_class_init (CtkInspectorCssEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);

  object_class->constructed = constructed;
  object_class->finalize = finalize;

  ctk_widget_class_set_template_from_resource (widget_class, "/org/ctk/libctk/inspector/css-editor.ui");
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorCssEditor, text);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorCssEditor, view);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorCssEditor, disable_button);
  ctk_widget_class_bind_template_callback (widget_class, disable_toggled);
  ctk_widget_class_bind_template_callback (widget_class, save_clicked);
  ctk_widget_class_bind_template_callback (widget_class, text_changed);
  ctk_widget_class_bind_template_callback (widget_class, query_tooltip_cb);
}

// vim: set et sw=2 ts=2:
