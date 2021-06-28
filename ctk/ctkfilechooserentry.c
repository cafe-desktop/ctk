/* CTK - The GIMP Toolkit
 * ctkfilechooserentry.c: Entry with filename completion
 * Copyright (C) 2003, Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "ctkfilechooserentry.h"

#include <string.h>

#include "ctkcelllayout.h"
#include "ctkcellrenderertext.h"
#include "ctkentry.h"
#include "ctkfilesystemmodel.h"
#include "ctklabel.h"
#include "ctkmain.h"
#include "ctksizerequest.h"
#include "ctkwindow.h"
#include "ctkintl.h"
#include "ctkmarshalers.h"
#include "ctkfilefilterprivate.h"

typedef struct _CtkFileChooserEntryClass CtkFileChooserEntryClass;

#define CTK_FILE_CHOOSER_ENTRY_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_FILE_CHOOSER_ENTRY, CtkFileChooserEntryClass))
#define CTK_IS_FILE_CHOOSER_ENTRY_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_FILE_CHOOSER_ENTRY))
#define CTK_FILE_CHOOSER_ENTRY_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_FILE_CHOOSER_ENTRY, CtkFileChooserEntryClass))

struct _CtkFileChooserEntryClass
{
  CtkEntryClass parent_class;
};

struct _CtkFileChooserEntry
{
  CtkEntry parent_instance;

  CtkFileChooserAction action;

  GFile *base_folder;
  GFile *current_folder_file;
  gchar *dir_part;
  gchar *file_part;

  CtkTreeModel *completion_store;
  CtkFileFilter *current_filter;

  guint current_folder_loaded : 1;
  guint complete_on_load : 1;
  guint eat_tabs       : 1;
  guint eat_escape     : 1;
  guint local_only     : 1;
};

enum
{
  DISPLAY_NAME_COLUMN,
  FULL_PATH_COLUMN,
  N_COLUMNS
};

enum
{
  HIDE_ENTRY,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static void     ctk_file_chooser_entry_finalize       (GObject          *object);
static void     ctk_file_chooser_entry_dispose        (GObject          *object);
static void     ctk_file_chooser_entry_grab_focus     (CtkWidget        *widget);
static gboolean ctk_file_chooser_entry_tab_handler    (CtkWidget *widget,
						       CdkEventKey *event);
static gboolean ctk_file_chooser_entry_focus_out_event (CtkWidget       *widget,
							CdkEventFocus   *event);

#ifdef G_OS_WIN32
static gint     insert_text_callback      (CtkFileChooserEntry *widget,
					   const gchar         *new_text,
					   gint                 new_text_length,
					   gint                *position,
					   gpointer             user_data);
static void     delete_text_callback      (CtkFileChooserEntry *widget,
					   gint                 start_pos,
					   gint                 end_pos,
					   gpointer             user_data);
#endif

static gboolean match_selected_callback   (CtkEntryCompletion  *completion,
					   CtkTreeModel        *model,
					   CtkTreeIter         *iter,
					   CtkFileChooserEntry *chooser_entry);

static void set_complete_on_load (CtkFileChooserEntry *chooser_entry,
                                  gboolean             complete_on_load);
static void refresh_current_folder_and_file_part (CtkFileChooserEntry *chooser_entry);
static void set_completion_folder (CtkFileChooserEntry *chooser_entry,
                                   GFile               *folder,
				   char                *dir_part);
static void finished_loading_cb (CtkFileSystemModel  *model,
                                 GError              *error,
		                 CtkFileChooserEntry *chooser_entry);

G_DEFINE_TYPE (CtkFileChooserEntry, _ctk_file_chooser_entry, CTK_TYPE_ENTRY)

static char *
ctk_file_chooser_entry_get_completion_text (CtkFileChooserEntry *chooser_entry)
{
  CtkEditable *editable = CTK_EDITABLE (chooser_entry);
  int start, end;

  ctk_editable_get_selection_bounds (editable, &start, &end);
  return ctk_editable_get_chars (editable, 0, MIN (start, end));
}

static void
ctk_file_chooser_entry_dispatch_properties_changed (GObject     *object,
                                                    guint        n_pspecs,
                                                    GParamSpec **pspecs)
{
  CtkFileChooserEntry *chooser_entry = CTK_FILE_CHOOSER_ENTRY (object);
  guint i;

  G_OBJECT_CLASS (_ctk_file_chooser_entry_parent_class)->dispatch_properties_changed (object, n_pspecs, pspecs);

  /* Don't do this during or after disposal */
  if (ctk_widget_get_parent (CTK_WIDGET (object)) != NULL)
    {
      /* What we are after: The text in front of the cursor was modified.
       * Unfortunately, there's no other way to catch this.
       */
      for (i = 0; i < n_pspecs; i++)
        {
          if (pspecs[i]->name == I_("cursor-position") ||
              pspecs[i]->name == I_("selection-bound") ||
              pspecs[i]->name == I_("text"))
            {
              set_complete_on_load (chooser_entry, FALSE);
              refresh_current_folder_and_file_part (chooser_entry);
              break;
            }
        }
    }
}

static void
_ctk_file_chooser_entry_class_init (CtkFileChooserEntryClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (class);

  gobject_class->finalize = ctk_file_chooser_entry_finalize;
  gobject_class->dispose = ctk_file_chooser_entry_dispose;
  gobject_class->dispatch_properties_changed = ctk_file_chooser_entry_dispatch_properties_changed;

  widget_class->grab_focus = ctk_file_chooser_entry_grab_focus;
  widget_class->focus_out_event = ctk_file_chooser_entry_focus_out_event;

  signals[HIDE_ENTRY] =
    g_signal_new (I_("hide-entry"),
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);
}

static gboolean
match_func (CtkEntryCompletion *compl,
            const gchar        *key,
            CtkTreeIter        *iter,
            gpointer            user_data)
{
  CtkFileChooserEntry *chooser_entry = user_data;

  /* If we arrive here, the CtkFileSystemModel's CtkFileFilter already filtered out all
   * files that don't start with the current prefix, so we manually apply the CtkFileChooser's
   * current file filter (e.g. just jpg files) here. */
  if (chooser_entry->current_filter != NULL)
    {
      char *mime_type = NULL;
      gboolean matches;
      GFile *file;
      GFileInfo *file_info;
      CtkFileFilterInfo filter_info;
      CtkFileFilterFlags needed_flags;

      file = _ctk_file_system_model_get_file (CTK_FILE_SYSTEM_MODEL (chooser_entry->completion_store),
                                              iter);
      file_info = _ctk_file_system_model_get_info (CTK_FILE_SYSTEM_MODEL (chooser_entry->completion_store),
                                                   iter);

      /* We always allow navigating into subfolders, so don't ever filter directories */
      if (g_file_info_get_file_type (file_info) != G_FILE_TYPE_REGULAR)
        return TRUE;

      needed_flags = ctk_file_filter_get_needed (chooser_entry->current_filter);

      filter_info.display_name = g_file_info_get_display_name (file_info);
      filter_info.contains = CTK_FILE_FILTER_DISPLAY_NAME;

      if (needed_flags & CTK_FILE_FILTER_MIME_TYPE)
        {
          const char *s = g_file_info_get_content_type (file_info);
          if (s != NULL)
            {
              mime_type = g_content_type_get_mime_type (s);
              if (mime_type != NULL)
                {
                  filter_info.mime_type = mime_type;
                  filter_info.contains |= CTK_FILE_FILTER_MIME_TYPE;
                }
            }
        }

      if (needed_flags & CTK_FILE_FILTER_FILENAME)
        {
          const char *path = g_file_get_path (file);
          if (path != NULL)
            {
              filter_info.filename = path;
              filter_info.contains |= CTK_FILE_FILTER_FILENAME;
            }
        }

      if (needed_flags & CTK_FILE_FILTER_URI)
        {
          const char *uri = g_file_get_uri (file);
          if (uri)
            {
              filter_info.uri = uri;
              filter_info.contains |= CTK_FILE_FILTER_URI;
            }
        }

      matches = ctk_file_filter_filter (chooser_entry->current_filter, &filter_info);

      g_free (mime_type);
      return matches;
    }

  return TRUE;
}

static void
_ctk_file_chooser_entry_init (CtkFileChooserEntry *chooser_entry)
{
  CtkEntryCompletion *comp;
  CtkCellRenderer *cell;

  chooser_entry->local_only = TRUE;

  g_object_set (chooser_entry, "truncate-multiline", TRUE, NULL);

  comp = ctk_entry_completion_new ();
  ctk_entry_completion_set_popup_single_match (comp, FALSE);
  ctk_entry_completion_set_minimum_key_length (comp, 0);
  /* see docs for ctk_entry_completion_set_text_column() */
  g_object_set (comp, "text-column", FULL_PATH_COLUMN, NULL);

  /* Need a match func here or entry completion uses a wrong one.
   * We do our own filtering after all. */
  ctk_entry_completion_set_match_func (comp,
                                       match_func,
                                       chooser_entry,
                                       NULL);

  cell = ctk_cell_renderer_text_new ();
  ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (comp),
                              cell, TRUE);
  ctk_cell_layout_add_attribute (CTK_CELL_LAYOUT (comp),
                                 cell,
                                 "text", DISPLAY_NAME_COLUMN);

  g_signal_connect (comp, "match-selected",
		    G_CALLBACK (match_selected_callback), chooser_entry);

  ctk_entry_set_completion (CTK_ENTRY (chooser_entry), comp);
  g_object_unref (comp);
  /* NB: This needs to happen after the completion is set, so this handler
   * runs before the handler installed by entrycompletion */
  g_signal_connect (chooser_entry, "key-press-event",
                    G_CALLBACK (ctk_file_chooser_entry_tab_handler), NULL);

#ifdef G_OS_WIN32
  g_signal_connect (chooser_entry, "insert-text",
		    G_CALLBACK (insert_text_callback), NULL);
  g_signal_connect (chooser_entry, "delete-text",
		    G_CALLBACK (delete_text_callback), NULL);
#endif
}

static void
ctk_file_chooser_entry_finalize (GObject *object)
{
  CtkFileChooserEntry *chooser_entry = CTK_FILE_CHOOSER_ENTRY (object);

  if (chooser_entry->base_folder)
    g_object_unref (chooser_entry->base_folder);

  if (chooser_entry->current_folder_file)
    g_object_unref (chooser_entry->current_folder_file);

  g_free (chooser_entry->dir_part);
  g_free (chooser_entry->file_part);

  G_OBJECT_CLASS (_ctk_file_chooser_entry_parent_class)->finalize (object);
}

static void
ctk_file_chooser_entry_dispose (GObject *object)
{
  CtkFileChooserEntry *chooser_entry = CTK_FILE_CHOOSER_ENTRY (object);

  set_completion_folder (chooser_entry, NULL, NULL);

  G_OBJECT_CLASS (_ctk_file_chooser_entry_parent_class)->dispose (object);
}

/* Match functions for the CtkEntryCompletion */
static gboolean
match_selected_callback (CtkEntryCompletion  *completion,
                         CtkTreeModel        *model,
                         CtkTreeIter         *iter,
                         CtkFileChooserEntry *chooser_entry)
{
  char *path;
  gint pos;

  ctk_tree_model_get (model, iter,
                      FULL_PATH_COLUMN, &path,
                      -1);

  ctk_editable_delete_text (CTK_EDITABLE (chooser_entry),
                            0,
                            ctk_editable_get_position (CTK_EDITABLE (chooser_entry)));
  pos = 0;
  ctk_editable_insert_text (CTK_EDITABLE (chooser_entry),
                            path,
                            -1,
                            &pos);

  ctk_editable_set_position (CTK_EDITABLE (chooser_entry), pos);

  g_free (path);

  return TRUE;
}

static void
set_complete_on_load (CtkFileChooserEntry *chooser_entry,
                      gboolean             complete_on_load)
{
  /* a completion was triggered, but we couldn't do it.
   * So no text was inserted when pressing tab, so we beep
   */
  if (chooser_entry->complete_on_load && !complete_on_load)
    ctk_widget_error_bell (CTK_WIDGET (chooser_entry));

  chooser_entry->complete_on_load = complete_on_load;
}

static gboolean
is_valid_scheme_character (char c)
{
  return g_ascii_isalnum (c) || c == '+' || c == '-' || c == '.';
}

static gboolean
has_uri_scheme (const char *str)
{
  const char *p;

  p = str;

  if (!is_valid_scheme_character (*p))
    return FALSE;

  do
    p++;
  while (is_valid_scheme_character (*p));

  return (strncmp (p, "://", 3) == 0);
}

static GFile *
ctk_file_chooser_get_file_for_text (CtkFileChooserEntry *chooser_entry,
                                    const gchar         *str)
{
  GFile *file;

  if (str[0] == '~' || g_path_is_absolute (str) || has_uri_scheme (str))
    file = g_file_parse_name (str);
  else if (chooser_entry->base_folder != NULL)
    file = g_file_resolve_relative_path (chooser_entry->base_folder, str);
  else
    file = NULL;

  return file;
}

static gboolean
is_directory_shortcut (const char *text)
{
  return strcmp (text, ".") == 0 ||
         strcmp (text, "..") == 0 ||
         strcmp (text, "~" ) == 0;
}

static GFile *
ctk_file_chooser_get_directory_for_text (CtkFileChooserEntry *chooser_entry,
                                         const char *         text)
{
  GFile *file, *parent;

  file = ctk_file_chooser_get_file_for_text (chooser_entry, text);

  if (file == NULL)
    return NULL;

  if (text[0] == 0 || text[strlen (text) - 1] == G_DIR_SEPARATOR ||
      is_directory_shortcut (text))
    return file;

  parent = g_file_get_parent (file);
  g_object_unref (file);

  return parent;
}

/* Finds a common prefix based on the contents of the entry
 * and mandatorily appends it
 */
static void
explicitly_complete (CtkFileChooserEntry *chooser_entry)
{
  chooser_entry->complete_on_load = FALSE;

  if (chooser_entry->completion_store)
    {
      char *completion, *text;
      gsize completion_len, text_len;

      text = ctk_file_chooser_entry_get_completion_text (chooser_entry);
      text_len = strlen (text);
      completion = ctk_entry_completion_compute_prefix (ctk_entry_get_completion (CTK_ENTRY (chooser_entry)), text);
      completion_len = completion ? strlen (completion) : 0;

      if (completion_len > text_len)
        {
          CtkEditable *editable = CTK_EDITABLE (chooser_entry);
          int pos = ctk_editable_get_position (editable);

          ctk_editable_insert_text (editable,
                                    completion + text_len,
                                    completion_len - text_len,
                                    &pos);
          ctk_editable_set_position (editable, pos);
          return;
        }
    }

  ctk_widget_error_bell (CTK_WIDGET (chooser_entry));
}

static void
ctk_file_chooser_entry_grab_focus (CtkWidget *widget)
{
  CTK_WIDGET_CLASS (_ctk_file_chooser_entry_parent_class)->grab_focus (widget);
  _ctk_file_chooser_entry_select_filename (CTK_FILE_CHOOSER_ENTRY (widget));
}

static void
start_explicit_completion (CtkFileChooserEntry *chooser_entry)
{
  if (chooser_entry->current_folder_loaded)
    explicitly_complete (chooser_entry);
  else
    set_complete_on_load (chooser_entry, TRUE);
}

static gboolean
ctk_file_chooser_entry_tab_handler (CtkWidget *widget,
				    CdkEventKey *event)
{
  CtkFileChooserEntry *chooser_entry;
  CtkEditable *editable;
  CdkModifierType state;
  gint start, end;

  chooser_entry = CTK_FILE_CHOOSER_ENTRY (widget);
  editable = CTK_EDITABLE (widget);

  if (event->keyval == GDK_KEY_Escape &&
      chooser_entry->eat_escape)
    {
      g_signal_emit (widget, signals[HIDE_ENTRY], 0);
      return TRUE;
    }

  if (!chooser_entry->eat_tabs)
    return FALSE;

  if (event->keyval != GDK_KEY_Tab)
    return FALSE;

  if (ctk_get_current_event_state (&state) &&
      (state & GDK_CONTROL_MASK) == GDK_CONTROL_MASK)
    return FALSE;

  /* This is a bit evil -- it makes Tab never leave the entry. It basically
   * makes it 'safe' for people to hit. */
  ctk_editable_get_selection_bounds (editable, &start, &end);
      
  if (start != end)
    ctk_editable_set_position (editable, MAX (start, end));
  else
    start_explicit_completion (chooser_entry);

  return TRUE;
}

static gboolean
ctk_file_chooser_entry_focus_out_event (CtkWidget     *widget,
					CdkEventFocus *event)
{
  CtkFileChooserEntry *chooser_entry = CTK_FILE_CHOOSER_ENTRY (widget);

  set_complete_on_load (chooser_entry, FALSE);
 
  return CTK_WIDGET_CLASS (_ctk_file_chooser_entry_parent_class)->focus_out_event (widget, event);
}

static void
update_inline_completion (CtkFileChooserEntry *chooser_entry)
{
  CtkEntryCompletion *completion = ctk_entry_get_completion (CTK_ENTRY (chooser_entry));

  if (!chooser_entry->current_folder_loaded)
    {
      ctk_entry_completion_set_inline_completion (completion, FALSE);
      return;
    }

  switch (chooser_entry->action)
    {
    case CTK_FILE_CHOOSER_ACTION_OPEN:
    case CTK_FILE_CHOOSER_ACTION_SELECT_FOLDER:
      ctk_entry_completion_set_inline_completion (completion, TRUE);
      break;
    case CTK_FILE_CHOOSER_ACTION_SAVE:
    case CTK_FILE_CHOOSER_ACTION_CREATE_FOLDER:
      ctk_entry_completion_set_inline_completion (completion, FALSE);
      break;
    }
}

static void
discard_completion_store (CtkFileChooserEntry *chooser_entry)
{
  if (!chooser_entry->completion_store)
    return;

  ctk_entry_completion_set_model (ctk_entry_get_completion (CTK_ENTRY (chooser_entry)), NULL);
  update_inline_completion (chooser_entry);
  g_object_unref (chooser_entry->completion_store);
  chooser_entry->completion_store = NULL;
}

static gboolean
completion_store_set (CtkFileSystemModel  *model,
                      GFile               *file,
                      GFileInfo           *info,
                      int                  column,
                      GValue              *value,
                      gpointer             data)
{
  CtkFileChooserEntry *chooser_entry = data;

  const char *prefix = "";
  const char *suffix = "";

  switch (column)
    {
    case FULL_PATH_COLUMN:
      prefix = chooser_entry->dir_part;
      /* fall through */
    case DISPLAY_NAME_COLUMN:
      if (_ctk_file_info_consider_as_directory (info))
        suffix = G_DIR_SEPARATOR_S;

      g_value_take_string (value,
			   g_strconcat (prefix,
					g_file_info_get_display_name (info),
					suffix,
					NULL));
      break;
    default:
      g_assert_not_reached ();
      break;
    }

  return TRUE;
}

/* Fills the completion store from the contents of the current folder */
static void
populate_completion_store (CtkFileChooserEntry *chooser_entry)
{
  chooser_entry->completion_store = CTK_TREE_MODEL (
      _ctk_file_system_model_new_for_directory (chooser_entry->current_folder_file,
                                                "standard::name,standard::display-name,standard::type,"
                                                "standard::content-type",
                                                completion_store_set,
                                                chooser_entry,
                                                N_COLUMNS,
                                                G_TYPE_STRING,
                                                G_TYPE_STRING));
  g_signal_connect (chooser_entry->completion_store, "finished-loading",
		    G_CALLBACK (finished_loading_cb), chooser_entry);

  _ctk_file_system_model_set_filter_folders (CTK_FILE_SYSTEM_MODEL (chooser_entry->completion_store),
                                             TRUE);
  _ctk_file_system_model_set_show_files (CTK_FILE_SYSTEM_MODEL (chooser_entry->completion_store),
                                         chooser_entry->action == CTK_FILE_CHOOSER_ACTION_OPEN ||
                                         chooser_entry->action == CTK_FILE_CHOOSER_ACTION_SAVE);
  ctk_tree_sortable_set_sort_column_id (CTK_TREE_SORTABLE (chooser_entry->completion_store),
					DISPLAY_NAME_COLUMN, CTK_SORT_ASCENDING);

  ctk_entry_completion_set_model (ctk_entry_get_completion (CTK_ENTRY (chooser_entry)),
				  chooser_entry->completion_store);
}

/* Callback when the current folder finishes loading */
static void
finished_loading_cb (CtkFileSystemModel  *model,
                     GError              *error,
		     CtkFileChooserEntry *chooser_entry)
{
  CtkEntryCompletion *completion;

  chooser_entry->current_folder_loaded = TRUE;

  if (error)
    {
      discard_completion_store (chooser_entry);
      set_complete_on_load (chooser_entry, FALSE);
      return;
    }

  if (chooser_entry->complete_on_load)
    explicitly_complete (chooser_entry);

  ctk_widget_set_tooltip_text (CTK_WIDGET (chooser_entry), NULL);

  completion = ctk_entry_get_completion (CTK_ENTRY (chooser_entry));
  update_inline_completion (chooser_entry);

  if (ctk_widget_has_focus (CTK_WIDGET (chooser_entry)))
    {
      ctk_entry_completion_complete (completion);
      ctk_entry_completion_insert_prefix (completion);
    }
}

static void
set_completion_folder (CtkFileChooserEntry *chooser_entry,
                       GFile               *folder_file,
		       char                *dir_part)
{
  if (folder_file &&
      chooser_entry->local_only
      && !_ctk_file_has_native_path (folder_file))
    folder_file = NULL;

  if (((chooser_entry->current_folder_file
	&& folder_file
	&& g_file_equal (folder_file, chooser_entry->current_folder_file))
       || chooser_entry->current_folder_file == folder_file)
      && g_strcmp0 (dir_part, chooser_entry->dir_part) == 0)
    {
      return;
    }

  if (chooser_entry->current_folder_file)
    {
      g_object_unref (chooser_entry->current_folder_file);
      chooser_entry->current_folder_file = NULL;
    }

  g_free (chooser_entry->dir_part);
  chooser_entry->dir_part = g_strdup (dir_part);
  
  chooser_entry->current_folder_loaded = FALSE;

  discard_completion_store (chooser_entry);

  if (folder_file)
    {
      chooser_entry->current_folder_file = g_object_ref (folder_file);
      populate_completion_store (chooser_entry);
    }
}

static void
refresh_current_folder_and_file_part (CtkFileChooserEntry *chooser_entry)
{
  GFile *folder_file;
  char *text, *last_slash, *old_file_part;
  char *dir_part;

  old_file_part = chooser_entry->file_part;

  text = ctk_file_chooser_entry_get_completion_text (chooser_entry);

  last_slash = strrchr (text, G_DIR_SEPARATOR);
  if (last_slash)
    {
      dir_part = g_strndup (text, last_slash - text + 1);
      chooser_entry->file_part = g_strdup (last_slash + 1);
    }
  else
    {
      dir_part = g_strdup ("");
      chooser_entry->file_part = g_strdup (text);
    }

  folder_file = ctk_file_chooser_get_directory_for_text (chooser_entry, text);

  set_completion_folder (chooser_entry, folder_file, dir_part);

  if (folder_file)
    g_object_unref (folder_file);

  g_free (dir_part);

  if (chooser_entry->completion_store &&
      (g_strcmp0 (old_file_part, chooser_entry->file_part) != 0))
    {
      CtkFileFilter *filter;
      char *pattern;

      filter = ctk_file_filter_new ();
      pattern = g_strconcat (chooser_entry->file_part, "*", NULL);
      ctk_file_filter_add_pattern (filter, pattern);

      _ctk_file_system_model_set_filter (CTK_FILE_SYSTEM_MODEL (chooser_entry->completion_store),
                                         filter);

      g_free (pattern);
      g_object_unref (filter);
    }

  g_free (text);
  g_free (old_file_part);
}

#ifdef G_OS_WIN32
static gint
insert_text_callback (CtkFileChooserEntry *chooser_entry,
		      const gchar	  *new_text,
		      gint       	   new_text_length,
		      gint       	  *position,
		      gpointer   	   user_data)
{
  const gchar *colon = memchr (new_text, ':', new_text_length);
  gint i;

  /* Disallow these characters altogether */
  for (i = 0; i < new_text_length; i++)
    {
      if (new_text[i] == '<' ||
	  new_text[i] == '>' ||
	  new_text[i] == '"' ||
	  new_text[i] == '|' ||
	  new_text[i] == '*' ||
	  new_text[i] == '?')
	break;
    }

  if (i < new_text_length ||
      /* Disallow entering text that would cause a colon to be anywhere except
       * after a drive letter.
       */
      (colon != NULL &&
       *position + (colon - new_text) != 1) ||
      (new_text_length > 0 &&
       *position <= 1 &&
       ctk_entry_get_text_length (CTK_ENTRY (chooser_entry)) >= 2 &&
       ctk_entry_get_text (CTK_ENTRY (chooser_entry))[1] == ':'))
    {
      ctk_widget_error_bell (CTK_WIDGET (chooser_entry));
      g_signal_stop_emission_by_name (chooser_entry, "insert_text");
      return FALSE;
    }

  return TRUE;
}

static void
delete_text_callback (CtkFileChooserEntry *chooser_entry,
		      gint                 start_pos,
		      gint                 end_pos,
		      gpointer             user_data)
{
  /* If deleting a drive letter, delete the colon, too */
  if (start_pos == 0 && end_pos == 1 &&
      ctk_entry_get_text_length (CTK_ENTRY (chooser_entry)) >= 2 &&
      ctk_entry_get_text (CTK_ENTRY (chooser_entry))[1] == ':')
    {
      g_signal_handlers_block_by_func (chooser_entry,
				       G_CALLBACK (delete_text_callback),
				       user_data);
      ctk_editable_delete_text (CTK_EDITABLE (chooser_entry), 0, 1);
      g_signal_handlers_unblock_by_func (chooser_entry,
					 G_CALLBACK (delete_text_callback),
					 user_data);
    }
}
#endif

/**
 * _ctk_file_chooser_entry_new:
 * @eat_tabs: If %FALSE, allow focus navigation with the tab key.
 * @eat_escape: If %TRUE, capture Escape key presses and emit ::hide-entry
 *
 * Creates a new #CtkFileChooserEntry object. #CtkFileChooserEntry
 * is an internal implementation widget for the CTK+ file chooser
 * which is an entry with completion with respect to a
 * #CtkFileSystem object.
 *
 * Returns: the newly created #CtkFileChooserEntry
 **/
CtkWidget *
_ctk_file_chooser_entry_new (gboolean eat_tabs,
                             gboolean eat_escape)
{
  CtkFileChooserEntry *chooser_entry;

  chooser_entry = g_object_new (CTK_TYPE_FILE_CHOOSER_ENTRY, NULL);
  chooser_entry->eat_tabs = (eat_tabs != FALSE);
  chooser_entry->eat_escape = (eat_escape != FALSE);

  return CTK_WIDGET (chooser_entry);
}

/**
 * _ctk_file_chooser_entry_set_base_folder:
 * @chooser_entry: a #CtkFileChooserEntry
 * @file: file for a folder in the chooser entries current file system.
 *
 * Sets the folder with respect to which completions occur.
 **/
void
_ctk_file_chooser_entry_set_base_folder (CtkFileChooserEntry *chooser_entry,
					 GFile               *file)
{
  g_return_if_fail (CTK_IS_FILE_CHOOSER_ENTRY (chooser_entry));
  g_return_if_fail (file == NULL || G_IS_FILE (file));

  if (chooser_entry->base_folder == file ||
      (file != NULL && chooser_entry->base_folder != NULL 
       && g_file_equal (chooser_entry->base_folder, file)))
    return;

  if (file)
    g_object_ref (file);

  if (chooser_entry->base_folder)
    g_object_unref (chooser_entry->base_folder);

  chooser_entry->base_folder = file;

  refresh_current_folder_and_file_part (chooser_entry);
}

/**
 * _ctk_file_chooser_entry_get_current_folder:
 * @chooser_entry: a #CtkFileChooserEntry
 *
 * Gets the current folder for the #CtkFileChooserEntry. If the
 * user has only entered a filename, this will be in the base folder
 * (see _ctk_file_chooser_entry_set_base_folder()), but if the
 * user has entered a relative or absolute path, then it will
 * be different.  If the user has entered unparsable text, or text which
 * the entry cannot handle, this will return %NULL.
 *
 * Returns: the file for the current folder - you must g_object_unref()
 *   the value after use.
 **/
GFile *
_ctk_file_chooser_entry_get_current_folder (CtkFileChooserEntry *chooser_entry)
{
  g_return_val_if_fail (CTK_IS_FILE_CHOOSER_ENTRY (chooser_entry), NULL);

  return ctk_file_chooser_get_directory_for_text (chooser_entry,
                                                  ctk_entry_get_text (CTK_ENTRY (chooser_entry)));
}

/**
 * _ctk_file_chooser_entry_get_file_part:
 * @chooser_entry: a #CtkFileChooserEntry
 *
 * Gets the non-folder portion of whatever the user has entered
 * into the file selector. What is returned is a UTF-8 string,
 * and if a filename path is needed, g_file_get_child_for_display_name()
 * must be used
  *
 * Returns: the entered filename - this value is owned by the
 *  chooser entry and must not be modified or freed.
 **/
const gchar *
_ctk_file_chooser_entry_get_file_part (CtkFileChooserEntry *chooser_entry)
{
  const char *last_slash, *text;

  g_return_val_if_fail (CTK_IS_FILE_CHOOSER_ENTRY (chooser_entry), NULL);

  text = ctk_entry_get_text (CTK_ENTRY (chooser_entry));
  last_slash = strrchr (text, G_DIR_SEPARATOR);
  if (last_slash)
    return last_slash + 1;
  else if (is_directory_shortcut (text))
    return "";
  else
    return text;
}

/**
 * _ctk_file_chooser_entry_set_action:
 * @chooser_entry: a #CtkFileChooserEntry
 * @action: the action which is performed by the file selector using this entry
 *
 * Sets action which is performed by the file selector using this entry. 
 * The #CtkFileChooserEntry will use different completion strategies for 
 * different actions.
 **/
void
_ctk_file_chooser_entry_set_action (CtkFileChooserEntry *chooser_entry,
				    CtkFileChooserAction action)
{
  g_return_if_fail (CTK_IS_FILE_CHOOSER_ENTRY (chooser_entry));
  
  if (chooser_entry->action != action)
    {
      CtkEntryCompletion *comp;

      chooser_entry->action = action;

      comp = ctk_entry_get_completion (CTK_ENTRY (chooser_entry));

      /* FIXME: do we need to actually set the following? */

      switch (action)
	{
	case CTK_FILE_CHOOSER_ACTION_OPEN:
	case CTK_FILE_CHOOSER_ACTION_SELECT_FOLDER:
	  ctk_entry_completion_set_popup_single_match (comp, FALSE);
	  break;
	case CTK_FILE_CHOOSER_ACTION_SAVE:
	case CTK_FILE_CHOOSER_ACTION_CREATE_FOLDER:
	  ctk_entry_completion_set_popup_single_match (comp, TRUE);
	  break;
	}

      if (chooser_entry->completion_store)
        _ctk_file_system_model_set_show_files (CTK_FILE_SYSTEM_MODEL (chooser_entry->completion_store),
                                               action == CTK_FILE_CHOOSER_ACTION_OPEN ||
                                               action == CTK_FILE_CHOOSER_ACTION_SAVE);

      update_inline_completion (chooser_entry);
    }
}


/**
 * _ctk_file_chooser_entry_get_action:
 * @chooser_entry: a #CtkFileChooserEntry
 *
 * Gets the action for this entry. 
 *
 * Returns: the action
 **/
CtkFileChooserAction
_ctk_file_chooser_entry_get_action (CtkFileChooserEntry *chooser_entry)
{
  g_return_val_if_fail (CTK_IS_FILE_CHOOSER_ENTRY (chooser_entry),
			CTK_FILE_CHOOSER_ACTION_OPEN);
  
  return chooser_entry->action;
}

gboolean
_ctk_file_chooser_entry_get_is_folder (CtkFileChooserEntry *chooser_entry,
				       GFile               *file)
{
  CtkTreeIter iter;
  GFileInfo *info;

  if (chooser_entry->completion_store == NULL ||
      !_ctk_file_system_model_get_iter_for_file (CTK_FILE_SYSTEM_MODEL (chooser_entry->completion_store),
                                                 &iter,
                                                 file))
    return FALSE;

  info = _ctk_file_system_model_get_info (CTK_FILE_SYSTEM_MODEL (chooser_entry->completion_store),
                                          &iter);

  return _ctk_file_info_consider_as_directory (info);
}


/*
 * _ctk_file_chooser_entry_select_filename:
 * @chooser_entry: a #CtkFileChooserEntry
 *
 * Selects the filename (without the extension) for user edition.
 */
void
_ctk_file_chooser_entry_select_filename (CtkFileChooserEntry *chooser_entry)
{
  const gchar *str, *ext;
  glong len = -1;

  if (chooser_entry->action == CTK_FILE_CHOOSER_ACTION_SAVE)
    {
      str = ctk_entry_get_text (CTK_ENTRY (chooser_entry));
      ext = g_strrstr (str, ".");

      if (ext)
       len = g_utf8_pointer_to_offset (str, ext);
    }

  ctk_editable_select_region (CTK_EDITABLE (chooser_entry), 0, (gint) len);
}

void
_ctk_file_chooser_entry_set_local_only (CtkFileChooserEntry *chooser_entry,
                                        gboolean             local_only)
{
  chooser_entry->local_only = local_only;
  refresh_current_folder_and_file_part (chooser_entry);
}

gboolean
_ctk_file_chooser_entry_get_local_only (CtkFileChooserEntry *chooser_entry)
{
  return chooser_entry->local_only;
}

void
_ctk_file_chooser_entry_set_file_filter (CtkFileChooserEntry *chooser_entry,
                                         CtkFileFilter       *filter)
{
  chooser_entry->current_filter = filter;
}
