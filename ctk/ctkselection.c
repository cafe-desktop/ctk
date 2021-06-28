/* CTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

/* This file implements most of the work of the ICCCM selection protocol.
 * The code was written after an intensive study of the equivalent part
 * of John Ousterhout’s Tk toolkit, and does many things in much the 
 * same way.
 *
 * The one thing in the ICCCM that isn’t fully supported here (or in Tk)
 * is side effects targets. For these to be handled properly, MULTIPLE
 * targets need to be done in the order specified. This cannot be
 * guaranteed with the way we do things, since if we are doing INCR
 * transfers, the order will depend on the timing of the requestor.
 *
 * By Owen Taylor <owt1@cornell.edu>	      8/16/97
 */

/* Terminology note: when not otherwise specified, the term "incr" below
 * refers to the _sending_ part of the INCR protocol. The receiving
 * portion is referred to just as “retrieval”. (Terminology borrowed
 * from Tk, because there is no good opposite to “retrieval” in English.
 * “send” can’t be made into a noun gracefully and we’re already using
 * “emission” for something else ....)
 */

/* The MOTIF entry widget seems to ask for the TARGETS target, then
   (regardless of the reply) ask for the TEXT target. It's slightly
   possible though that it somehow thinks we are responding negatively
   to the TARGETS request, though I don't really think so ... */

/*
 * Modified by the CTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/. 
 */

/**
 * SECTION:ctkselection
 * @Title: Selections
 * @Short_description: Functions for handling inter-process communication
 *     via selections
 * @See_also: #CtkWidget - Much of the operation of selections happens via
 *     signals for #CtkWidget. In particular, if you are using the functions
 *     in this section, you may need to pay attention to
 *     #CtkWidget::selection-get, #CtkWidget::selection-received and
 *     #CtkWidget::selection-clear-event signals
 *
 * The selection mechanism provides the basis for different types
 * of communication between processes. In particular, drag and drop and
 * #CtkClipboard work via selections. You will very seldom or
 * never need to use most of the functions in this section directly;
 * #CtkClipboard provides a nicer interface to the same functionality.
 *
 * If an application is expected to exchange image data and work
 * on Windows, it is highly advised to support at least "image/bmp" target
 * for the widest possible compatibility with third-party applications.
 * #CtkClipboard already does that by using ctk_target_list_add_image_targets()
 * and ctk_selection_data_set_pixbuf() or ctk_selection_data_get_pixbuf(),
 * which is one of the reasons why it is advised to use #CtkClipboard.
 *
 * Some of the datatypes defined this section are used in
 * the #CtkClipboard and drag-and-drop API’s as well. The
 * #CtkTargetEntry and #CtkTargetList objects represent
 * lists of data types that are supported when sending or
 * receiving data. The #CtkSelectionData object is used to
 * store a chunk of data along with the data type and other
 * associated information.
 */

/* We are using deprecated API, here, and we know that */
#define CDK_DISABLE_DEPRECATION_WARNINGS

#include "config.h"

#include "ctkselection.h"
#include "ctkselectionprivate.h"

#include <stdarg.h>
#include <string.h>
#include "cdk.h"

#include "ctkmain.h"
#include "ctkdebug.h"
#include "ctktextbufferrichtext.h"
#include "ctkintl.h"
#include "cdk-pixbuf/cdk-pixbuf.h"

#ifdef CDK_WINDOWING_X11
#include "x11/cdkx.h"
#endif

#ifdef CDK_WINDOWING_WIN32
#include "win32/cdkwin32.h"
#endif

#ifdef CDK_WINDOWING_WAYLAND
#include <cdk/wayland/cdkwayland.h>
#endif

#ifdef CDK_WINDOWING_BROADWAY
#include "broadway/cdkbroadway.h"
#endif

#undef DEBUG_SELECTION

/* Maximum size of a sent chunk, in bytes. Also the default size of
   our buffers */
#ifdef CDK_WINDOWING_X11
#define CTK_SELECTION_MAX_SIZE(display)                                 \
  CDK_IS_X11_DISPLAY (display) ?                                        \
  MIN(262144,                                                           \
      XExtendedMaxRequestSize (CDK_DISPLAY_XDISPLAY (display)) == 0     \
       ? XMaxRequestSize (CDK_DISPLAY_XDISPLAY (display)) - 100         \
       : XExtendedMaxRequestSize (CDK_DISPLAY_XDISPLAY (display)) - 100)\
  : G_MAXINT
#else
/* No chunks on Win32 */
#define CTK_SELECTION_MAX_SIZE(display) G_MAXINT
#endif

#define IDLE_ABORT_TIME 30

enum {
  INCR,
  MULTIPLE,
  TARGETS,
  TIMESTAMP,
  SAVE_TARGETS,
  LAST_ATOM
};

typedef struct _CtkSelectionInfo CtkSelectionInfo;
typedef struct _CtkIncrConversion CtkIncrConversion;
typedef struct _CtkIncrInfo CtkIncrInfo;
typedef struct _CtkRetrievalInfo CtkRetrievalInfo;

struct _CtkSelectionInfo
{
  CdkAtom	 selection;
  CtkWidget	*widget;	/* widget that owns selection */
  guint32	 time;		/* time used to acquire selection */
  CdkDisplay	*display;	/* needed in ctk_selection_remove_all */    
};

struct _CtkIncrConversion 
{
  CdkAtom	    target;	/* Requested target */
  CdkAtom	    property;	/* Property to store in */
  CtkSelectionData  data;	/* The data being supplied */
  gint		    offset;	/* Current offset in sent selection.
				 *  -1 => All done
				 *  -2 => Only the final (empty) portion
				 *	  left to send */
};

struct _CtkIncrInfo
{
  CdkWindow *requestor;		/* Requestor window - we create a CdkWindow
				   so we can receive events */
  CdkAtom    selection;		/* Selection we're sending */
  
  CtkIncrConversion *conversions; /* Information about requested conversions -
				   * With MULTIPLE requests (benighted 1980's
				   * hardware idea), there can be more than
				   * one */
  gint num_conversions;
  gint num_incrs;		/* number of remaining INCR style transactions */
  guint32 idle_time;
};


struct _CtkRetrievalInfo
{
  CtkWidget *widget;
  CdkAtom selection;		/* Selection being retrieved. */
  CdkAtom target;		/* Form of selection that we requested */
  guint32 idle_time;		/* Number of seconds since we last heard
				   from selection owner */
  guchar   *buffer;		/* Buffer in which to accumulate results */
  gint	   offset;		/* Current offset in buffer, -1 indicates
				   not yet started */
  guint32 notify_time;		/* Timestamp from SelectionNotify */
};

/* Local Functions */
static void ctk_selection_init              (void);
static gboolean ctk_selection_incr_timeout      (CtkIncrInfo      *info);
static gboolean ctk_selection_retrieval_timeout (CtkRetrievalInfo *info);
static void ctk_selection_retrieval_report  (CtkRetrievalInfo *info,
					     CdkAtom           type,
					     gint              format,
					     guchar           *buffer,
					     gint              length,
					     guint32           time);
static void ctk_selection_invoke_handler    (CtkWidget        *widget,
					     CtkSelectionData *data,
					     guint             time);
static void ctk_selection_default_handler   (CtkWidget        *widget,
					     CtkSelectionData *data);
static int  ctk_selection_bytes_per_item    (gint              format);

/* Local Data */
static gint initialize = TRUE;
static GList *current_retrievals = NULL;
static GList *current_incrs = NULL;
static GList *current_selections = NULL;

static CdkAtom ctk_selection_atoms[LAST_ATOM];
static const char ctk_selection_handler_key[] = "ctk-selection-handlers";

/****************
 * Target Lists *
 ****************/

/*
 * Target lists
 */


/**
 * ctk_target_list_new:
 * @targets: (array length=ntargets) (allow-none): Pointer to an array
 *   of #CtkTargetEntry
 * @ntargets: number of entries in @targets.
 * 
 * Creates a new #CtkTargetList from an array of #CtkTargetEntry.
 * 
 * Returns: (transfer full): the new #CtkTargetList.
 **/
CtkTargetList *
ctk_target_list_new (const CtkTargetEntry *targets,
		     guint                 ntargets)
{
  CtkTargetList *result = g_slice_new (CtkTargetList);
  result->list = NULL;
  result->ref_count = 1;

  if (targets)
    ctk_target_list_add_table (result, targets, ntargets);
  
  return result;
}

/**
 * ctk_target_list_ref:
 * @list:  a #CtkTargetList
 * 
 * Increases the reference count of a #CtkTargetList by one.
 *
 * Returns: the passed in #CtkTargetList.
 **/
CtkTargetList *
ctk_target_list_ref (CtkTargetList *list)
{
  g_return_val_if_fail (list != NULL, NULL);

  list->ref_count++;

  return list;
}

/**
 * ctk_target_list_unref:
 * @list: a #CtkTargetList
 * 
 * Decreases the reference count of a #CtkTargetList by one.
 * If the resulting reference count is zero, frees the list.
 **/
void               
ctk_target_list_unref (CtkTargetList *list)
{
  g_return_if_fail (list != NULL);
  g_return_if_fail (list->ref_count > 0);

  list->ref_count--;
  if (list->ref_count == 0)
    {
      GList *tmp_list = list->list;
      while (tmp_list)
	{
	  CtkTargetPair *pair = tmp_list->data;
	  g_slice_free (CtkTargetPair, pair);

	  tmp_list = tmp_list->next;
	}
      
      g_list_free (list->list);
      g_slice_free (CtkTargetList, list);
    }
}

/**
 * ctk_target_list_add:
 * @list:  a #CtkTargetList
 * @target: the interned atom representing the target
 * @flags: the flags for this target
 * @info: an ID that will be passed back to the application
 * 
 * Appends another target to a #CtkTargetList.
 **/
void 
ctk_target_list_add (CtkTargetList *list,
		     CdkAtom        target,
		     guint          flags,
		     guint          info)
{
  CtkTargetPair *pair;

  g_return_if_fail (list != NULL);
  
  pair = g_slice_new (CtkTargetPair);
  pair->target = target;
  pair->flags = flags;
  pair->info = info;

  list->list = g_list_append (list->list, pair);
}

static CdkAtom utf8_atom;
static CdkAtom text_atom;
static CdkAtom ctext_atom;
static CdkAtom text_plain_atom;
static CdkAtom text_plain_utf8_atom;
static CdkAtom text_plain_locale_atom;
static CdkAtom text_uri_list_atom;

static void 
init_atoms (void)
{
  gchar *tmp;
  const gchar *charset;

  if (!utf8_atom)
    {
      utf8_atom = cdk_atom_intern_static_string ("UTF8_STRING");
      text_atom = cdk_atom_intern_static_string ("TEXT");
      ctext_atom = cdk_atom_intern_static_string ("COMPOUND_TEXT");
      text_plain_atom = cdk_atom_intern_static_string ("text/plain");
      text_plain_utf8_atom = cdk_atom_intern_static_string ("text/plain;charset=utf-8");
      g_get_charset (&charset);
      tmp = g_strdup_printf ("text/plain;charset=%s", charset);
      text_plain_locale_atom = cdk_atom_intern (tmp, FALSE);
      g_free (tmp);

      text_uri_list_atom = cdk_atom_intern_static_string ("text/uri-list");
    }
}

/**
 * ctk_target_list_add_text_targets:
 * @list: a #CtkTargetList
 * @info: an ID that will be passed back to the application
 * 
 * Appends the text targets supported by #CtkSelectionData to
 * the target list. All targets are added with the same @info.
 * 
 * Since: 2.6
 **/
void 
ctk_target_list_add_text_targets (CtkTargetList *list,
				  guint          info)
{
  g_return_if_fail (list != NULL);
  
  init_atoms ();

  /* Keep in sync with ctk_selection_data_targets_include_text()
   */
  ctk_target_list_add (list, utf8_atom, 0, info);  
  ctk_target_list_add (list, ctext_atom, 0, info);  
  ctk_target_list_add (list, text_atom, 0, info);  
  ctk_target_list_add (list, CDK_TARGET_STRING, 0, info);  
  ctk_target_list_add (list, text_plain_utf8_atom, 0, info);  
  if (!g_get_charset (NULL))
    ctk_target_list_add (list, text_plain_locale_atom, 0, info);  
  ctk_target_list_add (list, text_plain_atom, 0, info);  
}

/**
 * ctk_target_list_add_rich_text_targets:
 * @list: a #CtkTargetList
 * @info: an ID that will be passed back to the application
 * @deserializable: if %TRUE, then deserializable rich text formats
 *                  will be added, serializable formats otherwise.
 * @buffer: a #CtkTextBuffer.
 *
 * Appends the rich text targets registered with
 * ctk_text_buffer_register_serialize_format() or
 * ctk_text_buffer_register_deserialize_format() to the target list. All
 * targets are added with the same @info.
 *
 * Since: 2.10
 **/
void
ctk_target_list_add_rich_text_targets (CtkTargetList  *list,
                                       guint           info,
                                       gboolean        deserializable,
                                       CtkTextBuffer  *buffer)
{
  CdkAtom *atoms;
  gint     n_atoms;
  gint     i;

  g_return_if_fail (list != NULL);
  g_return_if_fail (CTK_IS_TEXT_BUFFER (buffer));

  if (deserializable)
    atoms = ctk_text_buffer_get_deserialize_formats (buffer, &n_atoms);
  else
    atoms = ctk_text_buffer_get_serialize_formats (buffer, &n_atoms);

  for (i = 0; i < n_atoms; i++)
    ctk_target_list_add (list, atoms[i], 0, info);

  g_free (atoms);
}

/**
 * ctk_target_list_add_image_targets:
 * @list: a #CtkTargetList
 * @info: an ID that will be passed back to the application
 * @writable: whether to add only targets for which CTK+ knows
 *   how to convert a pixbuf into the format
 * 
 * Appends the image targets supported by #CtkSelectionData to
 * the target list. All targets are added with the same @info.
 * 
 * Since: 2.6
 **/
void 
ctk_target_list_add_image_targets (CtkTargetList *list,
				   guint          info,
				   gboolean       writable)
{
  GSList *formats, *f;
  gchar **mimes, **m;
  CdkAtom atom;

  g_return_if_fail (list != NULL);

  formats = gdk_pixbuf_get_formats ();

  /* Make sure png comes first */
  for (f = formats; f; f = f->next)
    {
      GdkPixbufFormat *fmt = f->data;
      gchar *name; 
 
      name = gdk_pixbuf_format_get_name (fmt);
      if (strcmp (name, "png") == 0)
	{
	  formats = g_slist_delete_link (formats, f);
	  formats = g_slist_prepend (formats, fmt);

	  g_free (name);

	  break;
	}

      g_free (name);
    }  

  for (f = formats; f; f = f->next)
    {
      GdkPixbufFormat *fmt = f->data;

      if (writable && !gdk_pixbuf_format_is_writable (fmt))
	continue;
      
      mimes = gdk_pixbuf_format_get_mime_types (fmt);
      for (m = mimes; *m; m++)
	{
	  atom = cdk_atom_intern (*m, FALSE);
	  ctk_target_list_add (list, atom, 0, info);  
	}
      g_strfreev (mimes);
    }

  g_slist_free (formats);
}

/**
 * ctk_target_list_add_uri_targets:
 * @list: a #CtkTargetList
 * @info: an ID that will be passed back to the application
 * 
 * Appends the URI targets supported by #CtkSelectionData to
 * the target list. All targets are added with the same @info.
 * 
 * Since: 2.6
 **/
void 
ctk_target_list_add_uri_targets (CtkTargetList *list,
				 guint          info)
{
  g_return_if_fail (list != NULL);
  
  init_atoms ();

  ctk_target_list_add (list, text_uri_list_atom, 0, info);  
}

/**
 * ctk_target_list_add_table:
 * @list: a #CtkTargetList
 * @targets: (array length=ntargets): the table of #CtkTargetEntry
 * @ntargets: number of targets in the table
 * 
 * Prepends a table of #CtkTargetEntry to a target list.
 **/
void               
ctk_target_list_add_table (CtkTargetList        *list,
			   const CtkTargetEntry *targets,
			   guint                 ntargets)
{
  gint i;

  for (i=ntargets-1; i >= 0; i--)
    {
      CtkTargetPair *pair = g_slice_new (CtkTargetPair);
      pair->target = cdk_atom_intern (targets[i].target, FALSE);
      pair->flags = targets[i].flags;
      pair->info = targets[i].info;
      
      list->list = g_list_prepend (list->list, pair);
    }
}

/**
 * ctk_target_list_remove:
 * @list: a #CtkTargetList
 * @target: the interned atom representing the target
 * 
 * Removes a target from a target list.
 **/
void 
ctk_target_list_remove (CtkTargetList *list,
			CdkAtom            target)
{
  GList *tmp_list;

  g_return_if_fail (list != NULL);

  tmp_list = list->list;
  while (tmp_list)
    {
      CtkTargetPair *pair = tmp_list->data;
      
      if (pair->target == target)
	{
	  g_slice_free (CtkTargetPair, pair);

	  list->list = g_list_remove_link (list->list, tmp_list);
	  g_list_free_1 (tmp_list);

	  return;
	}
      
      tmp_list = tmp_list->next;
    }
}

/**
 * ctk_target_list_find:
 * @list: a #CtkTargetList
 * @target: an interned atom representing the target to search for
 * @info: (out) (allow-none): a pointer to the location to store
 *        application info for target, or %NULL
 *
 * Looks up a given target in a #CtkTargetList.
 *
 * Returns: %TRUE if the target was found, otherwise %FALSE
 **/
gboolean
ctk_target_list_find (CtkTargetList *list,
		      CdkAtom        target,
		      guint         *info)
{
  GList *tmp_list;

  g_return_val_if_fail (list != NULL, FALSE);

  tmp_list = list->list;
  while (tmp_list)
    {
      CtkTargetPair *pair = tmp_list->data;

      if (pair->target == target)
	{
          if (info)
            *info = pair->info;

	  return TRUE;
	}

      tmp_list = tmp_list->next;
    }

  return FALSE;
}

/**
 * ctk_target_table_new_from_list:
 * @list: a #CtkTargetList
 * @n_targets: (out): return location for the number ot targets in the table
 *
 * This function creates an #CtkTargetEntry array that contains the
 * same targets as the passed %list. The returned table is newly
 * allocated and should be freed using ctk_target_table_free() when no
 * longer needed.
 *
 * Returns: (array length=n_targets) (transfer full): the new table.
 *
 * Since: 2.10
 **/
CtkTargetEntry *
ctk_target_table_new_from_list (CtkTargetList *list,
                                gint          *n_targets)
{
  CtkTargetEntry *targets;
  GList          *tmp_list;
  gint            i;

  g_return_val_if_fail (list != NULL, NULL);
  g_return_val_if_fail (n_targets != NULL, NULL);

  *n_targets = g_list_length (list->list);
  targets = g_new0 (CtkTargetEntry, *n_targets);

  for (tmp_list = list->list, i = 0; tmp_list; tmp_list = tmp_list->next, i++)
    {
      CtkTargetPair *pair = tmp_list->data;

      targets[i].target = cdk_atom_name (pair->target);
      targets[i].flags  = pair->flags;
      targets[i].info   = pair->info;
    }

  return targets;
}

/**
 * ctk_target_table_free:
 * @targets: (array length=n_targets): a #CtkTargetEntry array
 * @n_targets: the number of entries in the array
 *
 * This function frees a target table as returned by
 * ctk_target_table_new_from_list()
 *
 * Since: 2.10
 **/
void
ctk_target_table_free (CtkTargetEntry *targets,
                       gint            n_targets)
{
  gint i;

  g_return_if_fail (targets == NULL || n_targets > 0);

  for (i = 0; i < n_targets; i++)
    g_free (targets[i].target);

  g_free (targets);
}

/**
 * ctk_selection_owner_set_for_display:
 * @display: the #CdkDisplay where the selection is set
 * @widget: (allow-none): new selection owner (a #CtkWidget), or %NULL.
 * @selection: an interned atom representing the selection to claim.
 * @time_: timestamp with which to claim the selection
 *
 * Claim ownership of a given selection for a particular widget, or,
 * if @widget is %NULL, release ownership of the selection.
 *
 * Returns: TRUE if the operation succeeded 
 * 
 * Since: 2.2
 */
gboolean
ctk_selection_owner_set_for_display (CdkDisplay   *display,
				     CtkWidget    *widget,
				     CdkAtom       selection,
				     guint32       time)
{
  GList *tmp_list;
  CtkWidget *old_owner;
  CtkSelectionInfo *selection_info = NULL;
  CdkWindow *window;

  g_return_val_if_fail (CDK_IS_DISPLAY (display), FALSE);
  g_return_val_if_fail (selection != CDK_NONE, FALSE);
  g_return_val_if_fail (widget == NULL || ctk_widget_get_realized (widget), FALSE);
  g_return_val_if_fail (widget == NULL || ctk_widget_get_display (widget) == display, FALSE);
  
  if (widget == NULL)
    window = NULL;
  else
    window = ctk_widget_get_window (widget);

  tmp_list = current_selections;
  while (tmp_list)
    {
      if (((CtkSelectionInfo *)tmp_list->data)->selection == selection)
	{
	  selection_info = tmp_list->data;
	  break;
	}
      
      tmp_list = tmp_list->next;
    }
  
  if (cdk_selection_owner_set_for_display (display, window, selection, time, TRUE))
    {
      old_owner = NULL;
      
      if (widget == NULL)
	{
	  if (selection_info)
	    {
	      old_owner = selection_info->widget;
	      current_selections = g_list_remove_link (current_selections,
						       tmp_list);
	      g_list_free (tmp_list);
	      g_slice_free (CtkSelectionInfo, selection_info);
	    }
	}
      else
	{
	  if (selection_info == NULL)
	    {
	      selection_info = g_slice_new (CtkSelectionInfo);
	      selection_info->selection = selection;
	      selection_info->widget = widget;
	      selection_info->time = time;
	      selection_info->display = display;
	      current_selections = g_list_prepend (current_selections,
						   selection_info);
	    }
	  else
	    {
	      old_owner = selection_info->widget;
	      selection_info->widget = widget;
	      selection_info->time = time;
	      selection_info->display = display;
	    }
	}
      /* If another widget in the application lost the selection,
       *  send it a CDK_SELECTION_CLEAR event.
       */
      if (old_owner && old_owner != widget)
	{
	  CdkEvent *event = cdk_event_new (CDK_SELECTION_CLEAR);

          event->selection.window = g_object_ref (ctk_widget_get_window (old_owner));
	  event->selection.selection = selection;
	  event->selection.time = time;
	  
	  ctk_widget_event (old_owner, event);

	  cdk_event_free (event);
	}
      return TRUE;
    }
  else
    return FALSE;
}

/**
 * ctk_selection_owner_set:
 * @widget: (allow-none):  a #CtkWidget, or %NULL.
 * @selection:  an interned atom representing the selection to claim
 * @time_: timestamp with which to claim the selection
 * 
 * Claims ownership of a given selection for a particular widget,
 * or, if @widget is %NULL, release ownership of the selection.
 * 
 * Returns: %TRUE if the operation succeeded
 **/
gboolean
ctk_selection_owner_set (CtkWidget *widget,
			 CdkAtom    selection,
			 guint32    time)
{
  CdkDisplay *display;
  
  g_return_val_if_fail (widget == NULL || ctk_widget_get_realized (widget), FALSE);
  g_return_val_if_fail (selection != CDK_NONE, FALSE);

  if (widget)
    display = ctk_widget_get_display (widget);
  else
    {
      CTK_NOTE (MULTIHEAD,
		g_warning ("ctk_selection_owner_set (NULL,...) is not multihead safe"));
		 
      display = cdk_display_get_default ();
    }
  
  return ctk_selection_owner_set_for_display (display, widget,
					      selection, time);
}

typedef struct _CtkSelectionTargetList CtkSelectionTargetList;

struct _CtkSelectionTargetList {
  CdkAtom selection;
  CtkTargetList *list;
};

static CtkTargetList *
ctk_selection_target_list_get (CtkWidget    *widget,
			       CdkAtom       selection)
{
  CtkSelectionTargetList *sellist;
  GList *tmp_list;
  GList *lists;

  lists = g_object_get_data (G_OBJECT (widget), ctk_selection_handler_key);
  
  tmp_list = lists;
  while (tmp_list)
    {
      sellist = tmp_list->data;
      if (sellist->selection == selection)
	return sellist->list;
      tmp_list = tmp_list->next;
    }

  sellist = g_slice_new (CtkSelectionTargetList);
  sellist->selection = selection;
  sellist->list = ctk_target_list_new (NULL, 0);

  lists = g_list_prepend (lists, sellist);
  g_object_set_data (G_OBJECT (widget), I_(ctk_selection_handler_key), lists);

  return sellist->list;
}

static void
ctk_selection_target_list_remove (CtkWidget    *widget)
{
  CtkSelectionTargetList *sellist;
  GList *tmp_list;
  GList *lists;

  lists = g_object_get_data (G_OBJECT (widget), ctk_selection_handler_key);
  
  tmp_list = lists;
  while (tmp_list)
    {
      sellist = tmp_list->data;

      ctk_target_list_unref (sellist->list);

      g_slice_free (CtkSelectionTargetList, sellist);
      tmp_list = tmp_list->next;
    }

  g_list_free (lists);
  g_object_set_data (G_OBJECT (widget), I_(ctk_selection_handler_key), NULL);
}

/**
 * ctk_selection_clear_targets:
 * @widget:    a #CtkWidget
 * @selection: an atom representing a selection
 *
 * Remove all targets registered for the given selection for the
 * widget.
 **/
void 
ctk_selection_clear_targets (CtkWidget *widget,
			     CdkAtom    selection)
{
  CtkSelectionTargetList *sellist;
  GList *tmp_list;
  GList *lists;

  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (selection != CDK_NONE);

#ifdef CDK_WINDOWING_WAYLAND
  if (CDK_IS_WAYLAND_DISPLAY (ctk_widget_get_display (widget)))
    cdk_wayland_selection_clear_targets (ctk_widget_get_display (widget), selection);
#endif
#ifdef CDK_WINDOWING_WIN32
  if (CDK_IS_WIN32_DISPLAY (ctk_widget_get_display (widget)))
    cdk_win32_selection_clear_targets (ctk_widget_get_display (widget), selection);
#endif

  lists = g_object_get_data (G_OBJECT (widget), ctk_selection_handler_key);
  
  tmp_list = lists;
  while (tmp_list)
    {
      sellist = tmp_list->data;
      if (sellist->selection == selection)
	{
	  lists = g_list_delete_link (lists, tmp_list);
	  ctk_target_list_unref (sellist->list);
	  g_slice_free (CtkSelectionTargetList, sellist);

	  break;
	}
      
      tmp_list = tmp_list->next;
    }
  
  g_object_set_data (G_OBJECT (widget), I_(ctk_selection_handler_key), lists);
}

/**
 * ctk_selection_add_target:
 * @widget:  a #CtkWidget
 * @selection: the selection
 * @target: target to add.
 * @info: A unsigned integer which will be passed back to the application.
 * 
 * Appends a specified target to the list of supported targets for a 
 * given widget and selection.
 **/
void 
ctk_selection_add_target (CtkWidget	    *widget, 
			  CdkAtom	     selection,
			  CdkAtom	     target,
			  guint              info)
{
  CtkTargetList *list;

  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (selection != CDK_NONE);

  list = ctk_selection_target_list_get (widget, selection);
  ctk_target_list_add (list, target, 0, info);
#ifdef CDK_WINDOWING_WAYLAND
  if (CDK_IS_WAYLAND_DISPLAY (ctk_widget_get_display (widget)))
    cdk_wayland_selection_add_targets (ctk_widget_get_window (widget), selection, 1, &target);
#endif
#ifdef CDK_WINDOWING_WIN32
  if (CDK_IS_WIN32_DISPLAY (ctk_widget_get_display (widget)))
    cdk_win32_selection_add_targets (ctk_widget_get_window (widget), selection, 1, &target);
#endif
}

/**
 * ctk_selection_add_targets:
 * @widget: a #CtkWidget
 * @selection: the selection
 * @targets: (array length=ntargets): a table of targets to add
 * @ntargets:  number of entries in @targets
 * 
 * Prepends a table of targets to the list of supported targets
 * for a given widget and selection.
 **/
void 
ctk_selection_add_targets (CtkWidget            *widget, 
			   CdkAtom               selection,
			   const CtkTargetEntry *targets,
			   guint                 ntargets)
{
  CtkTargetList *list;

  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (selection != CDK_NONE);
  g_return_if_fail (targets != NULL);
  
  list = ctk_selection_target_list_get (widget, selection);
  ctk_target_list_add_table (list, targets, ntargets);

#ifdef CDK_WINDOWING_WAYLAND
  if (CDK_IS_WAYLAND_DISPLAY (ctk_widget_get_display (widget)))
    {
      CdkAtom *atoms = g_new (CdkAtom, ntargets);
      guint i;

      for (i = 0; i < ntargets; i++)
        atoms[i] = cdk_atom_intern (targets[i].target, FALSE);

      cdk_wayland_selection_add_targets (ctk_widget_get_window (widget), selection, ntargets, atoms);
      g_free (atoms);
    }
#endif

#ifdef CDK_WINDOWING_WIN32
  if (CDK_IS_WIN32_DISPLAY (ctk_widget_get_display (widget)))
    {
      int i;
      CdkAtom *atoms = g_new (CdkAtom, ntargets);

      for (i = 0; i < ntargets; ++i)
        atoms[i] = cdk_atom_intern (targets[i].target, FALSE);
      cdk_win32_selection_add_targets (ctk_widget_get_window (widget), selection, ntargets, atoms);
      g_free (atoms);
    }
#endif
}


/**
 * ctk_selection_remove_all:
 * @widget: a #CtkWidget 
 * 
 * Removes all handlers and unsets ownership of all 
 * selections for a widget. Called when widget is being
 * destroyed. This function will not generally be
 * called by applications.
 **/
void
ctk_selection_remove_all (CtkWidget *widget)
{
  GList *tmp_list;
  GList *next;
  CtkSelectionInfo *selection_info;

  g_return_if_fail (CTK_IS_WIDGET (widget));

  /* Remove pending requests/incrs for this widget */
  
  tmp_list = current_retrievals;
  while (tmp_list)
    {
      next = tmp_list->next;
      if (((CtkRetrievalInfo *)tmp_list->data)->widget == widget)
	{
	  current_retrievals = g_list_remove_link (current_retrievals, 
						   tmp_list);
	  /* structure will be freed in timeout */
	  g_list_free (tmp_list);
	}
      tmp_list = next;
    }
  
  /* Disclaim ownership of any selections */
  
  tmp_list = current_selections;
  while (tmp_list)
    {
      next = tmp_list->next;
      selection_info = (CtkSelectionInfo *)tmp_list->data;
      
      if (selection_info->widget == widget)
	{	
	  cdk_selection_owner_set_for_display (selection_info->display,
					       NULL, 
					       selection_info->selection,
				               CDK_CURRENT_TIME, FALSE);
	  current_selections = g_list_remove_link (current_selections,
						   tmp_list);
	  g_list_free (tmp_list);
	  g_slice_free (CtkSelectionInfo, selection_info);
	}
      
      tmp_list = next;
    }

  /* Remove all selection lists */
  ctk_selection_target_list_remove (widget);
}


/**
 * ctk_selection_convert:
 * @widget: The widget which acts as requestor
 * @selection: Which selection to get
 * @target: Form of information desired (e.g., STRING)
 * @time_: Time of request (usually of triggering event)
       In emergency, you could use #CDK_CURRENT_TIME
 * 
 * Requests the contents of a selection. When received, 
 * a “selection-received” signal will be generated.
 * 
 * Returns: %TRUE if requested succeeded. %FALSE if we could not process
 *          request. (e.g., there was already a request in process for
 *          this widget).
 **/
gboolean
ctk_selection_convert (CtkWidget *widget, 
		       CdkAtom	  selection, 
		       CdkAtom	  target,
		       guint32	  time_)
{
  CtkRetrievalInfo *info;
  GList *tmp_list;
  CdkWindow *owner_window;
  CdkDisplay *display;
  guint id;
  
  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);
  g_return_val_if_fail (selection != CDK_NONE, FALSE);
  
  if (initialize)
    ctk_selection_init ();
  
  if (!ctk_widget_get_realized (widget))
    ctk_widget_realize (widget);
  
  /* Check to see if there are already any retrievals in progress for
     this widget. If we changed CDK to use the selection for the 
     window property in which to store the retrieved information, then
     we could support multiple retrievals for different selections.
     This might be useful for DND. */
  
  tmp_list = current_retrievals;
  while (tmp_list)
    {
      info = (CtkRetrievalInfo *)tmp_list->data;
      if (info->widget == widget)
	return FALSE;
      tmp_list = tmp_list->next;
    }
  
  info = g_slice_new (CtkRetrievalInfo);
  
  info->widget = widget;
  info->selection = selection;
  info->target = target;
  info->idle_time = 0;
  info->buffer = NULL;
  info->offset = -1;
  
  /* Check if this process has current owner. If so, call handler
     procedure directly to avoid deadlocks with INCR. */

  display = ctk_widget_get_display (widget);
  owner_window = cdk_selection_owner_get_for_display (display, selection);
  
#ifdef CDK_WINDOWING_WIN32
  /* Special handling for DELETE requests,
   * make sure this goes down into CDK layer.
   */
  if (CDK_IS_WIN32_DISPLAY (display) &&
      target == cdk_atom_intern_static_string ("DELETE"))
    owner_window = NULL;
#endif

  if (owner_window != NULL)
    {
      CtkWidget *owner_widget;
      gpointer owner_widget_ptr;
      CtkSelectionData selection_data = {0};
      
      selection_data.selection = selection;
      selection_data.target = target;
      selection_data.length = -1;
      selection_data.display = display;
      
      cdk_window_get_user_data (owner_window, &owner_widget_ptr);
      owner_widget = owner_widget_ptr;
      
      if (owner_widget != NULL)
	{
	  ctk_selection_invoke_handler (owner_widget, 
					&selection_data,
					time_);
	  
	  ctk_selection_retrieval_report (info,
					  selection_data.type, 
					  selection_data.format,
					  selection_data.data,
					  selection_data.length,
					  time_);
	  
	  g_free (selection_data.data);
          selection_data.data = NULL;
          selection_data.length = -1;
	  
	  g_slice_free (CtkRetrievalInfo, info);
	  return TRUE;
	}
    }

#if defined CDK_WINDOWING_BROADWAY
  /* This patch is a workaround to circumvent unimplemented
     clipboard functionality in broadwayd. It eliminates
     35s delay on popup menu before first clipboard copy,
     by preventing conversion to be started.
   
     https://gitlab.gnome.org/GNOME/ctk/issues/1630
  */ 
  if (CDK_IS_BROADWAY_DISPLAY (display))
  {
      g_debug("ctk_selection_convert: disabled for broadway backend");

      ctk_selection_retrieval_report (
          info, CDK_NONE, 0, NULL, -1, CDK_CURRENT_TIME);

      return FALSE;
  }
#endif
  
  /* Otherwise, we need to go through X */
  
  current_retrievals = g_list_append (current_retrievals, info);
  cdk_selection_convert (ctk_widget_get_window (widget), selection, target, time_);
  id = cdk_threads_add_timeout (1000,
      (GSourceFunc) ctk_selection_retrieval_timeout, info);
  g_source_set_name_by_id (id, "[ctk+] ctk_selection_retrieval_timeout");
  
  return TRUE;
}

/**
 * ctk_selection_data_get_selection:
 * @selection_data: a pointer to a #CtkSelectionData-struct.
 *
 * Retrieves the selection #CdkAtom of the selection data.
 *
 * Returns: (transfer none): the selection #CdkAtom of the selection data.
 *
 * Since: 2.16
 **/
CdkAtom
ctk_selection_data_get_selection (const CtkSelectionData *selection_data)
{
  g_return_val_if_fail (selection_data != NULL, 0);

  return selection_data->selection;
}

/**
 * ctk_selection_data_get_target:
 * @selection_data: a pointer to a #CtkSelectionData-struct.
 *
 * Retrieves the target of the selection.
 *
 * Returns: (transfer none): the target of the selection.
 *
 * Since: 2.14
 **/
CdkAtom
ctk_selection_data_get_target (const CtkSelectionData *selection_data)
{
  g_return_val_if_fail (selection_data != NULL, 0);

  return selection_data->target;
}

/**
 * ctk_selection_data_get_data_type:
 * @selection_data: a pointer to a #CtkSelectionData-struct.
 *
 * Retrieves the data type of the selection.
 *
 * Returns: (transfer none): the data type of the selection.
 *
 * Since: 2.14
 **/
CdkAtom
ctk_selection_data_get_data_type (const CtkSelectionData *selection_data)
{
  g_return_val_if_fail (selection_data != NULL, 0);

  return selection_data->type;
}

/**
 * ctk_selection_data_get_format:
 * @selection_data: a pointer to a #CtkSelectionData-struct.
 *
 * Retrieves the format of the selection.
 *
 * Returns: the format of the selection.
 *
 * Since: 2.14
 **/
gint
ctk_selection_data_get_format (const CtkSelectionData *selection_data)
{
  g_return_val_if_fail (selection_data != NULL, 0);

  return selection_data->format;
}

/**
 * ctk_selection_data_get_data: (skip)
 * @selection_data: a pointer to a
 *   #CtkSelectionData-struct.
 *
 * Retrieves the raw data of the selection.
 *
 * Returns: (array) (element-type guint8): the raw data of the selection.
 *
 * Since: 2.14
 **/
const guchar*
ctk_selection_data_get_data (const CtkSelectionData *selection_data)
{
  g_return_val_if_fail (selection_data != NULL, NULL);

  return selection_data->data;
}

/**
 * ctk_selection_data_get_length:
 * @selection_data: a pointer to a #CtkSelectionData-struct.
 *
 * Retrieves the length of the raw data of the selection.
 *
 * Returns: the length of the data of the selection.
 *
 * Since: 2.14
 */
gint
ctk_selection_data_get_length (const CtkSelectionData *selection_data)
{
  g_return_val_if_fail (selection_data != NULL, -1);

  return selection_data->length;
}

/**
 * ctk_selection_data_get_data_with_length: (rename-to ctk_selection_data_get_data)
 * @selection_data: a pointer to a #CtkSelectionData-struct.
 * @length: (out): return location for length of the data segment
 *
 * Retrieves the raw data of the selection along with its length.
 *
 * Returns: (array length=length): the raw data of the selection
 *
 * Since: 3.0
 */
const guchar*
ctk_selection_data_get_data_with_length (const CtkSelectionData *selection_data,
                                         gint                   *length)
{
  g_return_val_if_fail (selection_data != NULL, NULL);

  *length = selection_data->length;

  return selection_data->data;
}

/**
 * ctk_selection_data_get_display:
 * @selection_data: a pointer to a #CtkSelectionData-struct.
 *
 * Retrieves the display of the selection.
 *
 * Returns: (transfer none): the display of the selection.
 *
 * Since: 2.14
 **/
CdkDisplay *
ctk_selection_data_get_display (const CtkSelectionData *selection_data)
{
  g_return_val_if_fail (selection_data != NULL, NULL);

  return selection_data->display;
}

/**
 * ctk_selection_data_set:
 * @selection_data: a pointer to a #CtkSelectionData-struct.
 * @type: the type of selection data
 * @format: format (number of bits in a unit)
 * @data: (array length=length): pointer to the data (will be copied)
 * @length: length of the data
 * 
 * Stores new data into a #CtkSelectionData object. Should
 * only be called from a selection handler callback.
 * Zero-terminates the stored data.
 **/
void 
ctk_selection_data_set (CtkSelectionData *selection_data,
			CdkAtom		  type,
			gint		  format,
			const guchar	 *data,
			gint		  length)
{
  g_return_if_fail (selection_data != NULL);

  g_free (selection_data->data);
  
  selection_data->type = type;
  selection_data->format = format;
  
  if (data)
    {
      selection_data->data = g_new (guchar, length+1);
      memcpy (selection_data->data, data, length);
      selection_data->data[length] = 0;
    }
  else
    {
      g_return_if_fail (length <= 0);
      
      if (length < 0)
	selection_data->data = NULL;
      else
	selection_data->data = (guchar *) g_strdup ("");
    }
  
  selection_data->length = length;
}

static gboolean
selection_set_string (CtkSelectionData *selection_data,
		      const gchar      *str,
		      gint              len)
{
  gchar *tmp = g_strndup (str, len);
  gchar *latin1 = cdk_utf8_to_string_target (tmp);
  g_free (tmp);
  
  if (latin1)
    {
      ctk_selection_data_set (selection_data,
			      CDK_SELECTION_TYPE_STRING,
			      8, (guchar *) latin1, strlen (latin1));
      g_free (latin1);
      
      return TRUE;
    }
  else
    return FALSE;
}

static gboolean
selection_set_compound_text (CtkSelectionData *selection_data,
			     const gchar      *str,
			     gint              len)
{
  gboolean result = FALSE;

#ifdef CDK_WINDOWING_X11
  gchar *tmp;
  guchar *text;
  CdkAtom encoding;
  gint format;
  gint new_length;

  if (CDK_IS_X11_DISPLAY (selection_data->display))
    {
      tmp = g_strndup (str, len);
      if (cdk_x11_display_utf8_to_compound_text (selection_data->display, tmp,
                                                 &encoding, &format, &text, &new_length))
        {
          ctk_selection_data_set (selection_data, encoding, format, text, new_length);
          cdk_x11_free_compound_text (text);

          result = TRUE;
        }
      g_free (tmp);
    }
#endif

  return result;
}

/* Normalize \r and \n into \r\n
 */
static gchar *
normalize_to_crlf (const gchar *str, 
		   gint         len)
{
  GString *result = g_string_sized_new (len);
  const gchar *p = str;
  const gchar *end = str + len;

  while (p < end)
    {
      if (*p == '\n')
	g_string_append_c (result, '\r');

      if (*p == '\r')
	{
	  g_string_append_c (result, *p);
	  p++;
	  if (p == end || *p != '\n')
	    g_string_append_c (result, '\n');
	  if (p == end)
	    break;
	}

      g_string_append_c (result, *p);
      p++;
    }

  return g_string_free (result, FALSE);  
}

/* Normalize \r and \r\n into \n
 */
static gchar *
normalize_to_lf (gchar *str, 
		 gint   len)
{
  GString *result = g_string_sized_new (len);
  const gchar *p = str;

  while (1)
    {
      if (*p == '\r')
	{
	  p++;
	  if (*p != '\n')
	    g_string_append_c (result, '\n');
	}

      if (*p == '\0')
	break;

      g_string_append_c (result, *p);
      p++;
    }

  return g_string_free (result, FALSE);  
}

static gboolean
selection_set_text_plain (CtkSelectionData *selection_data,
			  const gchar      *str,
			  gint              len)
{
  const gchar *charset = NULL;
  gchar *result;
  GError *error = NULL;

  result = normalize_to_crlf (str, len);
  if (selection_data->target == text_plain_atom)
    charset = "ASCII";
  else if (selection_data->target == text_plain_locale_atom)
    g_get_charset (&charset);

  if (charset)
    {
      gchar *tmp = result;
      result = g_convert_with_fallback (tmp, -1, 
					charset, "UTF-8", 
					NULL, NULL, NULL, &error);
      g_free (tmp);
    }

  if (!result)
    {
      g_warning ("Error converting from %s to %s: %s",
		 "UTF-8", charset, error->message);
      g_error_free (error);
      
      return FALSE;
    }
  
  ctk_selection_data_set (selection_data,
			  selection_data->target, 
			  8, (guchar *) result, strlen (result));
  g_free (result);
  
  return TRUE;
}

static guchar *
selection_get_text_plain (const CtkSelectionData *selection_data)
{
  const gchar *charset = NULL;
  gchar *str, *result;
  gsize len;
  GError *error = NULL;

  str = g_strdup ((const gchar *) selection_data->data);
  len = selection_data->length;
  
  if (selection_data->type == text_plain_atom)
    charset = "ISO-8859-1";
  else if (selection_data->type == text_plain_locale_atom)
    g_get_charset (&charset);

  if (charset)
    {
      gchar *tmp = str;
      str = g_convert_with_fallback (tmp, len, 
				     "UTF-8", charset,
				     NULL, NULL, &len, &error);
      g_free (tmp);

      if (!str)
	{
	  g_warning ("Error converting from %s to %s: %s",
		     charset, "UTF-8", error->message);
	  g_error_free (error);

	  return NULL;
	}
    }
  else if (!g_utf8_validate (str, -1, NULL))
    {
      g_warning ("Error converting from %s to %s: %s",
		 "text/plain;charset=utf-8", "UTF-8", "invalid UTF-8");
      g_free (str);

      return NULL;
    }

  result = normalize_to_lf (str, len);
  g_free (str);

  return (guchar *) result;
}

/**
 * ctk_selection_data_set_text:
 * @selection_data: a #CtkSelectionData
 * @str: a UTF-8 string
 * @len: the length of @str, or -1 if @str is nul-terminated.
 * 
 * Sets the contents of the selection from a UTF-8 encoded string.
 * The string is converted to the form determined by
 * @selection_data->target.
 * 
 * Returns: %TRUE if the selection was successfully set,
 *   otherwise %FALSE.
 **/
gboolean
ctk_selection_data_set_text (CtkSelectionData     *selection_data,
			     const gchar          *str,
			     gint                  len)
{
  g_return_val_if_fail (selection_data != NULL, FALSE);

  if (len < 0)
    len = strlen (str);
  
  init_atoms ();

  if (selection_data->target == utf8_atom)
    {
      ctk_selection_data_set (selection_data,
			      utf8_atom,
			      8, (guchar *)str, len);
      return TRUE;
    }
  else if (selection_data->target == CDK_TARGET_STRING)
    {
      return selection_set_string (selection_data, str, len);
    }
  else if (selection_data->target == ctext_atom ||
	   selection_data->target == text_atom)
    {
      if (selection_set_compound_text (selection_data, str, len))
	return TRUE;
      else if (selection_data->target == text_atom)
	return selection_set_string (selection_data, str, len);
    }
  else if (selection_data->target == text_plain_atom ||
	   selection_data->target == text_plain_utf8_atom ||
	   selection_data->target == text_plain_locale_atom)
    {
      return selection_set_text_plain (selection_data, str, len);
    }

  return FALSE;
}

/**
 * ctk_selection_data_get_text:
 * @selection_data: a #CtkSelectionData
 * 
 * Gets the contents of the selection data as a UTF-8 string.
 * 
 * Returns: (type utf8) (nullable) (transfer full): if the selection data contained a
 *   recognized text type and it could be converted to UTF-8, a newly
 *   allocated string containing the converted text, otherwise %NULL.
 *   If the result is non-%NULL it must be freed with g_free().
 **/
guchar *
ctk_selection_data_get_text (const CtkSelectionData *selection_data)
{
  guchar *result = NULL;

  g_return_val_if_fail (selection_data != NULL, NULL);

  init_atoms ();
  
  if (selection_data->length >= 0 &&
      (selection_data->type == CDK_TARGET_STRING ||
       selection_data->type == ctext_atom ||
       selection_data->type == utf8_atom))
    {
      gchar **list;
      gint i;
      gint count = cdk_text_property_to_utf8_list_for_display (selection_data->display,
      							       selection_data->type,
						   	       selection_data->format, 
						               selection_data->data,
						               selection_data->length,
						               &list);
      if (count > 0)
	result = (guchar *) list[0];

      for (i = 1; i < count; i++)
	g_free (list[i]);
      g_free (list);
    }
  else if (selection_data->length >= 0 &&
	   (selection_data->type == text_plain_atom ||
	    selection_data->type == text_plain_utf8_atom ||
	    selection_data->type == text_plain_locale_atom))
    {
      result = selection_get_text_plain (selection_data);
    }

  return result;
}

/**
 * ctk_selection_data_set_pixbuf:
 * @selection_data: a #CtkSelectionData
 * @pixbuf: a #GdkPixbuf
 * 
 * Sets the contents of the selection from a #GdkPixbuf
 * The pixbuf is converted to the form determined by
 * @selection_data->target.
 * 
 * Returns: %TRUE if the selection was successfully set,
 *   otherwise %FALSE.
 *
 * Since: 2.6
 **/
gboolean
ctk_selection_data_set_pixbuf (CtkSelectionData *selection_data,
			       GdkPixbuf        *pixbuf)
{
  GSList *formats, *f;
  gchar **mimes, **m;
  CdkAtom atom;
  gboolean result;
  gchar *str, *type;
  gsize len;

  g_return_val_if_fail (selection_data != NULL, FALSE);
  g_return_val_if_fail (CDK_IS_PIXBUF (pixbuf), FALSE);

  formats = gdk_pixbuf_get_formats ();

  for (f = formats; f; f = f->next)
    {
      GdkPixbufFormat *fmt = f->data;

      mimes = gdk_pixbuf_format_get_mime_types (fmt);
      for (m = mimes; *m; m++)
	{
	  atom = cdk_atom_intern (*m, FALSE);
	  if (selection_data->target == atom)
	    {
	      str = NULL;
	      type = gdk_pixbuf_format_get_name (fmt);
	      result = gdk_pixbuf_save_to_buffer (pixbuf, &str, &len,
						  type, NULL,
                                                  ((strcmp (type, "png") == 0) ?
                                                   "compression" : NULL), "2",
                                                  NULL);
	      if (result)
		ctk_selection_data_set (selection_data,
					atom, 8, (guchar *)str, len);
	      g_free (type);
	      g_free (str);
	      g_strfreev (mimes);
	      g_slist_free (formats);

	      return result;
	    }
	}

      g_strfreev (mimes);
    }

  g_slist_free (formats);
 
  return FALSE;
}

/**
 * ctk_selection_data_get_pixbuf:
 * @selection_data: a #CtkSelectionData
 * 
 * Gets the contents of the selection data as a #GdkPixbuf.
 * 
 * Returns: (nullable) (transfer full): if the selection data
 *   contained a recognized image type and it could be converted to a
 *   #GdkPixbuf, a newly allocated pixbuf is returned, otherwise
 *   %NULL.  If the result is non-%NULL it must be freed with
 *   g_object_unref().
 *
 * Since: 2.6
 **/
GdkPixbuf *
ctk_selection_data_get_pixbuf (const CtkSelectionData *selection_data)
{
  GdkPixbufLoader *loader;
  GdkPixbuf *result = NULL;

  g_return_val_if_fail (selection_data != NULL, NULL);

  if (selection_data->length > 0)
    {
      loader = gdk_pixbuf_loader_new ();
      
      gdk_pixbuf_loader_write (loader, 
			       selection_data->data,
			       selection_data->length,
			       NULL);
      gdk_pixbuf_loader_close (loader, NULL);
      result = gdk_pixbuf_loader_get_pixbuf (loader);
      
      if (result)
	g_object_ref (result);
      
      g_object_unref (loader);
    }

  return result;
}

/**
 * ctk_selection_data_set_uris:
 * @selection_data: a #CtkSelectionData
 * @uris: (array zero-terminated=1): a %NULL-terminated array of
 *     strings holding URIs
 * 
 * Sets the contents of the selection from a list of URIs.
 * The string is converted to the form determined by
 * @selection_data->target.
 * 
 * Returns: %TRUE if the selection was successfully set,
 *   otherwise %FALSE.
 *
 * Since: 2.6
 **/
gboolean
ctk_selection_data_set_uris (CtkSelectionData  *selection_data,
			     gchar            **uris)
{
  g_return_val_if_fail (selection_data != NULL, FALSE);
  g_return_val_if_fail (uris != NULL, FALSE);

  init_atoms ();

  if (selection_data->target == text_uri_list_atom)
    {
      GString *list;
      gint i;
      gchar *result;
      gsize length;
      
      list = g_string_new (NULL);
      for (i = 0; uris[i]; i++)
	{
	  g_string_append (list, uris[i]);
	  g_string_append (list, "\r\n");
	}

      result = g_convert (list->str, list->len,
			  "ASCII", "UTF-8", 
			  NULL, &length, NULL);
      g_string_free (list, TRUE);
      
      if (result)
	{
	  ctk_selection_data_set (selection_data,
				  text_uri_list_atom,
				  8, (guchar *)result, length);
	  
	  g_free (result);

	  return TRUE;
	}
    }

  return FALSE;
}

/**
 * ctk_selection_data_get_uris:
 * @selection_data: a #CtkSelectionData
 * 
 * Gets the contents of the selection data as array of URIs.
 *
 * Returns:  (array zero-terminated=1) (element-type utf8) (transfer full): if
 *   the selection data contains a list of
 *   URIs, a newly allocated %NULL-terminated string array
 *   containing the URIs, otherwise %NULL. If the result is
 *   non-%NULL it must be freed with g_strfreev().
 *
 * Since: 2.6
 **/
gchar **
ctk_selection_data_get_uris (const CtkSelectionData *selection_data)
{
  gchar **result = NULL;

  g_return_val_if_fail (selection_data != NULL, NULL);

  init_atoms ();
  
  if (selection_data->length >= 0 &&
      selection_data->type == text_uri_list_atom)
    {
      gchar **list;
      gint count = cdk_text_property_to_utf8_list_for_display (selection_data->display,
      							       utf8_atom,
						   	       selection_data->format, 
						               selection_data->data,
						               selection_data->length,
						               &list);
      if (count > 0)
	result = g_uri_list_extract_uris (list[0]);
      
      g_strfreev (list);
    }

  return result;
}


/**
 * ctk_selection_data_get_targets:
 * @selection_data: a #CtkSelectionData object
 * @targets: (out) (array length=n_atoms) (transfer container):
 *           location to store an array of targets. The result stored
 *           here must be freed with g_free().
 * @n_atoms: location to store number of items in @targets.
 * 
 * Gets the contents of @selection_data as an array of targets.
 * This can be used to interpret the results of getting
 * the standard TARGETS target that is always supplied for
 * any selection.
 * 
 * Returns: %TRUE if @selection_data contains a valid
 *    array of targets, otherwise %FALSE.
 **/
gboolean
ctk_selection_data_get_targets (const CtkSelectionData  *selection_data,
				CdkAtom                **targets,
				gint                    *n_atoms)
{
  g_return_val_if_fail (selection_data != NULL, FALSE);

  if (selection_data->length >= 0 &&
      selection_data->format == 32 &&
      selection_data->type == CDK_SELECTION_TYPE_ATOM)
    {
      if (targets)
	*targets = g_memdup (selection_data->data, selection_data->length);
      if (n_atoms)
	*n_atoms = selection_data->length / sizeof (CdkAtom);

      return TRUE;
    }
  else
    {
      if (targets)
	*targets = NULL;
      if (n_atoms)
	*n_atoms = -1;

      return FALSE;
    }
}

/**
 * ctk_targets_include_text:
 * @targets: (array length=n_targets): an array of #CdkAtoms
 * @n_targets: the length of @targets
 * 
 * Determines if any of the targets in @targets can be used to
 * provide text.
 * 
 * Returns: %TRUE if @targets include a suitable target for text,
 *   otherwise %FALSE.
 *
 * Since: 2.10
 **/
gboolean 
ctk_targets_include_text (CdkAtom *targets,
                          gint     n_targets)
{
  gint i;
  gboolean result = FALSE;

  g_return_val_if_fail (targets != NULL || n_targets == 0, FALSE);

  /* Keep in sync with ctk_target_list_add_text_targets()
   */
 
  init_atoms ();
 
  for (i = 0; i < n_targets; i++)
    {
      if (targets[i] == utf8_atom ||
	  targets[i] == text_atom ||
	  targets[i] == CDK_TARGET_STRING ||
	  targets[i] == ctext_atom ||
	  targets[i] == text_plain_atom ||
	  targets[i] == text_plain_utf8_atom ||
	  targets[i] == text_plain_locale_atom)
	{
	  result = TRUE;
	  break;
	}
    }
  
  return result;
}

/**
 * ctk_targets_include_rich_text:
 * @targets: (array length=n_targets): an array of #CdkAtoms
 * @n_targets: the length of @targets
 * @buffer: a #CtkTextBuffer
 *
 * Determines if any of the targets in @targets can be used to
 * provide rich text.
 *
 * Returns: %TRUE if @targets include a suitable target for rich text,
 *               otherwise %FALSE.
 *
 * Since: 2.10
 **/
gboolean
ctk_targets_include_rich_text (CdkAtom       *targets,
                               gint           n_targets,
                               CtkTextBuffer *buffer)
{
  CdkAtom *rich_targets;
  gint n_rich_targets;
  gint i, j;
  gboolean result = FALSE;

  g_return_val_if_fail (targets != NULL || n_targets == 0, FALSE);
  g_return_val_if_fail (CTK_IS_TEXT_BUFFER (buffer), FALSE);

  init_atoms ();

  rich_targets = ctk_text_buffer_get_deserialize_formats (buffer,
                                                          &n_rich_targets);

  for (i = 0; i < n_targets; i++)
    {
      for (j = 0; j < n_rich_targets; j++)
        {
          if (targets[i] == rich_targets[j])
            {
              result = TRUE;
              goto done;
            }
        }
    }

 done:
  g_free (rich_targets);

  return result;
}

/**
 * ctk_selection_data_targets_include_text:
 * @selection_data: a #CtkSelectionData object
 * 
 * Given a #CtkSelectionData object holding a list of targets,
 * determines if any of the targets in @targets can be used to
 * provide text.
 * 
 * Returns: %TRUE if @selection_data holds a list of targets,
 *   and a suitable target for text is included, otherwise %FALSE.
 **/
gboolean
ctk_selection_data_targets_include_text (const CtkSelectionData *selection_data)
{
  CdkAtom *targets;
  gint n_targets;
  gboolean result = FALSE;

  g_return_val_if_fail (selection_data != NULL, FALSE);

  init_atoms ();

  if (ctk_selection_data_get_targets (selection_data, &targets, &n_targets))
    {
      result = ctk_targets_include_text (targets, n_targets);
      g_free (targets);
    }

  return result;
}

/**
 * ctk_selection_data_targets_include_rich_text:
 * @selection_data: a #CtkSelectionData object
 * @buffer: a #CtkTextBuffer
 *
 * Given a #CtkSelectionData object holding a list of targets,
 * determines if any of the targets in @targets can be used to
 * provide rich text.
 *
 * Returns: %TRUE if @selection_data holds a list of targets,
 *               and a suitable target for rich text is included,
 *               otherwise %FALSE.
 *
 * Since: 2.10
 **/
gboolean
ctk_selection_data_targets_include_rich_text (const CtkSelectionData *selection_data,
                                              CtkTextBuffer          *buffer)
{
  CdkAtom *targets;
  gint n_targets;
  gboolean result = FALSE;

  g_return_val_if_fail (selection_data != NULL, FALSE);
  g_return_val_if_fail (CTK_IS_TEXT_BUFFER (buffer), FALSE);

  init_atoms ();

  if (ctk_selection_data_get_targets (selection_data, &targets, &n_targets))
    {
      result = ctk_targets_include_rich_text (targets, n_targets, buffer);
      g_free (targets);
    }

  return result;
}

/**
 * ctk_targets_include_image:
 * @targets: (array length=n_targets): an array of #CdkAtoms
 * @n_targets: the length of @targets
 * @writable: whether to accept only targets for which CTK+ knows
 *   how to convert a pixbuf into the format
 * 
 * Determines if any of the targets in @targets can be used to
 * provide a #GdkPixbuf.
 * 
 * Returns: %TRUE if @targets include a suitable target for images,
 *   otherwise %FALSE.
 *
 * Since: 2.10
 **/
gboolean 
ctk_targets_include_image (CdkAtom *targets,
			   gint     n_targets,
			   gboolean writable)
{
  CtkTargetList *list;
  GList *l;
  gint i;
  gboolean result = FALSE;

  g_return_val_if_fail (targets != NULL || n_targets == 0, FALSE);

  list = ctk_target_list_new (NULL, 0);
  ctk_target_list_add_image_targets (list, 0, writable);
  for (i = 0; i < n_targets && !result; i++)
    {
      for (l = list->list; l; l = l->next)
	{
	  CtkTargetPair *pair = (CtkTargetPair *)l->data;
	  if (pair->target == targets[i])
	    {
	      result = TRUE;
	      break;
	    }
	}
    }
  ctk_target_list_unref (list);

  return result;
}
				    
/**
 * ctk_selection_data_targets_include_image:
 * @selection_data: a #CtkSelectionData object
 * @writable: whether to accept only targets for which CTK+ knows
 *   how to convert a pixbuf into the format
 * 
 * Given a #CtkSelectionData object holding a list of targets,
 * determines if any of the targets in @targets can be used to
 * provide a #GdkPixbuf.
 * 
 * Returns: %TRUE if @selection_data holds a list of targets,
 *   and a suitable target for images is included, otherwise %FALSE.
 *
 * Since: 2.6
 **/
gboolean 
ctk_selection_data_targets_include_image (const CtkSelectionData *selection_data,
					  gboolean                writable)
{
  CdkAtom *targets;
  gint n_targets;
  gboolean result = FALSE;

  g_return_val_if_fail (selection_data != NULL, FALSE);

  init_atoms ();

  if (ctk_selection_data_get_targets (selection_data, &targets, &n_targets))
    {
      result = ctk_targets_include_image (targets, n_targets, writable);
      g_free (targets);
    }

  return result;
}

/**
 * ctk_targets_include_uri:
 * @targets: (array length=n_targets): an array of #CdkAtoms
 * @n_targets: the length of @targets
 * 
 * Determines if any of the targets in @targets can be used to
 * provide an uri list.
 * 
 * Returns: %TRUE if @targets include a suitable target for uri lists,
 *   otherwise %FALSE.
 *
 * Since: 2.10
 **/
gboolean 
ctk_targets_include_uri (CdkAtom *targets,
			 gint     n_targets)
{
  gint i;
  gboolean result = FALSE;

  g_return_val_if_fail (targets != NULL || n_targets == 0, FALSE);

  /* Keep in sync with ctk_target_list_add_uri_targets()
   */

  init_atoms ();

  for (i = 0; i < n_targets; i++)
    {
      if (targets[i] == text_uri_list_atom)
	{
	  result = TRUE;
	  break;
	}
    }
  
  return result;
}

/**
 * ctk_selection_data_targets_include_uri:
 * @selection_data: a #CtkSelectionData object
 * 
 * Given a #CtkSelectionData object holding a list of targets,
 * determines if any of the targets in @targets can be used to
 * provide a list or URIs.
 * 
 * Returns: %TRUE if @selection_data holds a list of targets,
 *   and a suitable target for URI lists is included, otherwise %FALSE.
 *
 * Since: 2.10
 **/
gboolean
ctk_selection_data_targets_include_uri (const CtkSelectionData *selection_data)
{
  CdkAtom *targets;
  gint n_targets;
  gboolean result = FALSE;

  g_return_val_if_fail (selection_data != NULL, FALSE);

  init_atoms ();

  if (ctk_selection_data_get_targets (selection_data, &targets, &n_targets))
    {
      result = ctk_targets_include_uri (targets, n_targets);
      g_free (targets);
    }

  return result;
}

	  
/*************************************************************
 * ctk_selection_init:
 *     Initialize local variables
 *   arguments:
 *     
 *   results:
 *************************************************************/

static void
ctk_selection_init (void)
{
  ctk_selection_atoms[INCR] = cdk_atom_intern_static_string ("INCR");
  ctk_selection_atoms[MULTIPLE] = cdk_atom_intern_static_string ("MULTIPLE");
  ctk_selection_atoms[TIMESTAMP] = cdk_atom_intern_static_string ("TIMESTAMP");
  ctk_selection_atoms[TARGETS] = cdk_atom_intern_static_string ("TARGETS");
  ctk_selection_atoms[SAVE_TARGETS] = cdk_atom_intern_static_string ("SAVE_TARGETS");

  initialize = FALSE;
}

/**
 * _ctk_selection_clear:
 * @widget: a #CtkWidget
 * @event: the event
 * 
 * The default handler for the #CtkWidget::selection-clear-event
 * signal. 
 * 
 * Returns: %TRUE if the event was handled, otherwise false
 **/
gboolean
_ctk_selection_clear (CtkWidget         *widget,
		     CdkEventSelection *event)
{
  /* Note that we filter clear events in cdkselection-x11.c, so
   * that we only will get here if the clear event actually
   * represents a change that we didn't do ourself.
   */
  GList *tmp_list;
  CtkSelectionInfo *selection_info = NULL;
  
  tmp_list = current_selections;
  while (tmp_list)
    {
      selection_info = (CtkSelectionInfo *)tmp_list->data;
      
      if ((selection_info->selection == event->selection) &&
	  (selection_info->widget == widget))
	break;
      
      tmp_list = tmp_list->next;
    }
  
  if (tmp_list)
    {
      current_selections = g_list_remove_link (current_selections, tmp_list);
      g_list_free (tmp_list);
      g_slice_free (CtkSelectionInfo, selection_info);
    }
  
  return TRUE;
}


/*************************************************************
 * _ctk_selection_request:
 *     Handler for “selection_request_event” 
 *   arguments:
 *     widget:
 *     event:
 *   results:
 *************************************************************/

gboolean
_ctk_selection_request (CtkWidget *widget,
			CdkEventSelection *event)
{
  CdkDisplay *display = ctk_widget_get_display (widget);
  CtkIncrInfo *info;
  GList *tmp_list;
  int i;
  gulong selection_max_size;

  if (event->requestor == NULL)
    return FALSE;

  if (initialize)
    ctk_selection_init ();
  
  selection_max_size = CTK_SELECTION_MAX_SIZE (display);

  /* Check if we own selection */
  
  tmp_list = current_selections;
  while (tmp_list)
    {
      CtkSelectionInfo *selection_info = (CtkSelectionInfo *)tmp_list->data;
      
      if ((selection_info->selection == event->selection) &&
	  (selection_info->widget == widget))
	break;
      
      tmp_list = tmp_list->next;
    }
  
  if (tmp_list == NULL)
    return FALSE;
  
  info = g_slice_new (CtkIncrInfo);

  g_object_ref (widget);

  info->selection = event->selection;
  info->num_incrs = 0;
  info->requestor = g_object_ref (event->requestor);

  /* Determine conversions we need to perform */
  if (event->target == ctk_selection_atoms[MULTIPLE])
    {
      CdkAtom  type;
      guchar  *mult_atoms;
      gint     format;
      gint     length;
      
      mult_atoms = NULL;
      
      cdk_error_trap_push ();
      if (!cdk_property_get (info->requestor, event->property, CDK_NONE, /* AnyPropertyType */
			     0, selection_max_size, FALSE,
			     &type, &format, &length, &mult_atoms))
	{
	  cdk_selection_send_notify_for_display (display,
						 event->requestor, 
						 event->selection,
						 event->target, 
						 CDK_NONE, 
						 event->time);
	  g_free (mult_atoms);
	  g_slice_free (CtkIncrInfo, info);
          cdk_error_trap_pop_ignored ();
	  return TRUE;
	}
      cdk_error_trap_pop_ignored ();

      /* This is annoying; the ICCCM doesn't specify the property type
       * used for the property contents, so the autoconversion for
       * ATOM / ATOM_PAIR in CDK doesn't work properly.
       */
#ifdef CDK_WINDOWING_X11
      if (type != CDK_SELECTION_TYPE_ATOM &&
	  type != cdk_atom_intern_static_string ("ATOM_PAIR"))
	{
	  info->num_conversions = length / (2*sizeof (glong));
	  info->conversions = g_new (CtkIncrConversion, info->num_conversions);
	  
	  for (i=0; i<info->num_conversions; i++)
	    {
	      info->conversions[i].target = cdk_x11_xatom_to_atom_for_display (display,
									       ((glong *)mult_atoms)[2*i]);
	      info->conversions[i].property = cdk_x11_xatom_to_atom_for_display (display,
										 ((glong *)mult_atoms)[2*i + 1]);
	    }

	  g_free (mult_atoms);
	}
      else
#endif
	{
	  info->num_conversions = length / (2*sizeof (CdkAtom));
	  info->conversions = g_new (CtkIncrConversion, info->num_conversions);
	  
	  for (i=0; i<info->num_conversions; i++)
	    {
	      info->conversions[i].target = ((CdkAtom *)mult_atoms)[2*i];
	      info->conversions[i].property = ((CdkAtom *)mult_atoms)[2*i+1];
	    }

	  g_free (mult_atoms);
	}
    }
  else				/* only a single conversion */
    {
      info->conversions = g_new (CtkIncrConversion, 1);
      info->num_conversions = 1;
      info->conversions[0].target = event->target;
      info->conversions[0].property = event->property;
    }
  
  /* Loop through conversions and determine which of these are big
     enough to require doing them via INCR */
  for (i=0; i<info->num_conversions; i++)
    {
      CtkSelectionData data;
      glong items;
      
      data.selection = event->selection;
      data.target = info->conversions[i].target;
      data.data = NULL;
      data.length = -1;
      data.display = ctk_widget_get_display (widget);
      
#ifdef DEBUG_SELECTION
      g_message ("Selection %ld, target %ld (%s) requested by 0x%x (property = %ld)",
		 event->selection, 
		 info->conversions[i].target,
		 cdk_atom_name (info->conversions[i].target),
		 event->requestor, info->conversions[i].property);
#endif
      
      ctk_selection_invoke_handler (widget, &data, event->time);
      if (data.length < 0)
	{
	  info->conversions[i].property = CDK_NONE;
	  continue;
	}
      
      g_return_val_if_fail ((data.format >= 8) && (data.format % 8 == 0), FALSE);
      
      items = data.length / ctk_selection_bytes_per_item (data.format);
      
      if (data.length > selection_max_size)
	{
	  /* Sending via INCR */
#ifdef DEBUG_SELECTION
	  g_message ("Target larger (%d) than max. request size (%ld), sending incrementally\n",
		     data.length, selection_max_size);
#endif
	  
	  info->conversions[i].offset = 0;
	  info->conversions[i].data = data;
	  info->num_incrs++;
	  
	  cdk_error_trap_push ();
	  cdk_property_change (info->requestor, 
			       info->conversions[i].property,
			       ctk_selection_atoms[INCR],
			       32,
			       CDK_PROP_MODE_REPLACE,
			       (guchar *)&items, 1);
	  cdk_error_trap_pop_ignored ();
	}
      else
	{
	  info->conversions[i].offset = -1;
	  
	  cdk_error_trap_push ();
	  cdk_property_change (info->requestor, 
			       info->conversions[i].property,
			       data.type,
			       data.format,
			       CDK_PROP_MODE_REPLACE,
			       data.data, items);
	  cdk_error_trap_pop_ignored ();
	  
	  g_free (data.data);
	}
    }
  
  /* If we have some INCR's, we need to send the rest of the data in
     a callback */
  
  if (info->num_incrs > 0)
    {
      guint id;

      /* FIXME: this could be dangerous if window doesn't still
	 exist */
      
#ifdef DEBUG_SELECTION
      g_message ("Starting INCR...");
#endif
      
      cdk_error_trap_push ();
      cdk_window_set_events (info->requestor,
			     cdk_window_get_events (info->requestor) |
			     CDK_PROPERTY_CHANGE_MASK);
      cdk_error_trap_pop_ignored ();
      current_incrs = g_list_append (current_incrs, info);
      id = cdk_threads_add_timeout (1000, (GSourceFunc) ctk_selection_incr_timeout, info);
      g_source_set_name_by_id (id, "[ctk+] ctk_selection_incr_timeout");
    }
  
  /* If it was a MULTIPLE request, set the property to indicate which
     conversions succeeded */
  if (event->target == ctk_selection_atoms[MULTIPLE])
    {
      CdkAtom *mult_atoms = g_new (CdkAtom, 2 * info->num_conversions);
      for (i = 0; i < info->num_conversions; i++)
	{
	  mult_atoms[2*i] = info->conversions[i].target;
	  mult_atoms[2*i+1] = info->conversions[i].property;
	}

      cdk_error_trap_push ();
      cdk_property_change (info->requestor, event->property,
			   cdk_atom_intern_static_string ("ATOM_PAIR"), 32, 
			   CDK_PROP_MODE_REPLACE,
			   (guchar *)mult_atoms, 2*info->num_conversions);
      cdk_error_trap_pop_ignored ();
      g_free (mult_atoms);
    }

  if (info->num_conversions == 1 &&
      info->conversions[0].property == CDK_NONE)
    {
      /* Reject the entire conversion */
      cdk_selection_send_notify_for_display (ctk_widget_get_display (widget),
					     event->requestor, 
					     event->selection, 
					     event->target, 
					     CDK_NONE, 
					     event->time);
    }
  else
    {
      cdk_selection_send_notify_for_display (ctk_widget_get_display (widget),
					     event->requestor, 
					     event->selection,
					     event->target,
					     event->property, 
					     event->time);
    }

  if (info->num_incrs == 0)
    {
      g_free (info->conversions);
      g_slice_free (CtkIncrInfo, info);
    }

  g_object_unref (widget);
  
  return TRUE;
}

/*************************************************************
 * _ctk_selection_incr_event:
 *     Called whenever an PropertyNotify event occurs for an 
 *     CdkWindow with user_data == NULL. These will be notifications
 *     that a window we are sending the selection to via the
 *     INCR protocol has deleted a property and is ready for
 *     more data.
 *
 *   arguments:
 *     window:	the requestor window
 *     event:	the property event structure
 *
 *   results:
 *************************************************************/

gboolean
_ctk_selection_incr_event (CdkWindow	   *window,
			   CdkEventProperty *event)
{
  GList *tmp_list;
  CtkIncrInfo *info = NULL;
  gint num_bytes;
  guchar *buffer;
  gulong selection_max_size;
  
  int i;
  
  if (event->state != CDK_PROPERTY_DELETE)
    return FALSE;
  
#ifdef DEBUG_SELECTION
  g_message ("PropertyDelete, property %ld", event->atom);
#endif

  selection_max_size = CTK_SELECTION_MAX_SIZE (cdk_window_get_display (window));  

  /* Now find the appropriate ongoing INCR */
  tmp_list = current_incrs;
  while (tmp_list)
    {
      info = (CtkIncrInfo *)tmp_list->data;
      if (info->requestor == event->window)
	break;
      
      tmp_list = tmp_list->next;
    }
  
  if (tmp_list == NULL)
    return FALSE;
  
  /* Find out which target this is for */
  for (i=0; i<info->num_conversions; i++)
    {
      if (info->conversions[i].property == event->atom &&
	  info->conversions[i].offset != -1)
	{
	  int bytes_per_item;
	  
	  info->idle_time = 0;
	  
	  if (info->conversions[i].offset == -2) /* only the last 0-length
						    piece*/
	    {
	      num_bytes = 0;
	      buffer = NULL;
	    }
	  else
	    {
	      num_bytes = info->conversions[i].data.length -
		info->conversions[i].offset;
	      buffer = info->conversions[i].data.data + 
		info->conversions[i].offset;
	      
	      if (num_bytes > selection_max_size)
		{
		  num_bytes = selection_max_size;
		  info->conversions[i].offset += selection_max_size;
		}
	      else
		info->conversions[i].offset = -2;
	    }
#ifdef DEBUG_SELECTION
	  g_message ("INCR: put %d bytes (offset = %d) into window 0x%lx , property %ld",
		     num_bytes, info->conversions[i].offset, 
		     CDK_WINDOW_XID(info->requestor), event->atom);
#endif

	  bytes_per_item = ctk_selection_bytes_per_item (info->conversions[i].data.format);
	  cdk_error_trap_push ();
	  cdk_property_change (info->requestor, event->atom,
			       info->conversions[i].data.type,
			       info->conversions[i].data.format,
			       CDK_PROP_MODE_REPLACE,
			       buffer,
			       num_bytes / bytes_per_item);
	  cdk_error_trap_pop_ignored ();
	  
	  if (info->conversions[i].offset == -2)
	    {
	      g_free (info->conversions[i].data.data);
	      info->conversions[i].data.data = NULL;
	    }
	  
	  if (num_bytes == 0)
	    {
	      info->num_incrs--;
	      info->conversions[i].offset = -1;
	    }
	}
    }
  
  /* Check if we're finished with all the targets */
  
  if (info->num_incrs == 0)
    {
      current_incrs = g_list_remove_link (current_incrs, tmp_list);
      g_list_free (tmp_list);
      /* Let the timeout free it */
    }
  
  return TRUE;
}

/*************************************************************
 * ctk_selection_incr_timeout:
 *     Timeout callback for the sending portion of the INCR
 *     protocol
 *   arguments:
 *     info:	Information about this incr
 *   results:
 *************************************************************/

static gint
ctk_selection_incr_timeout (CtkIncrInfo *info)
{
  GList *tmp_list;
  gboolean retval;

  /* Determine if retrieval has finished by checking if it still in
     list of pending retrievals */
  
  tmp_list = current_incrs;
  while (tmp_list)
    {
      if (info == (CtkIncrInfo *)tmp_list->data)
	break;
      tmp_list = tmp_list->next;
    }
  
  /* If retrieval is finished */
  if (!tmp_list || info->idle_time >= IDLE_ABORT_TIME)
    {
      if (tmp_list && info->idle_time >= IDLE_ABORT_TIME)
	{
	  current_incrs = g_list_remove_link (current_incrs, tmp_list);
	  g_list_free (tmp_list);
	}
      
      g_free (info->conversions);
      /* FIXME: we should check if requestor window is still in use,
	 and if not, remove it? */
      
      g_slice_free (CtkIncrInfo, info);
      
      retval =  FALSE;		/* remove timeout */
    }
  else
    {
      info->idle_time++;
      
      retval = TRUE;		/* timeout will happen again */
    }
  
  return retval;
}

/*************************************************************
 * _ctk_selection_notify:
 *     Handler for “selection-notify-event” signals on windows
 *     where a retrieval is currently in process. The selection
 *     owner has responded to our conversion request.
 *   arguments:
 *     widget:		Widget getting signal
 *     event:		Selection event structure
 *     info:		Information about this retrieval
 *   results:
 *     was event handled?
 *************************************************************/

gboolean
_ctk_selection_notify (CtkWidget	 *widget,
		       CdkEventSelection *event)
{
  GList *tmp_list;
  CtkRetrievalInfo *info = NULL;
  CdkWindow *window;
  guchar  *buffer = NULL;
  gint length;
  CdkAtom type;
  gint	  format;

#ifdef DEBUG_SELECTION
  g_message ("Initial receipt of selection %ld, target %ld (property = %ld)",
	     event->selection, event->target, event->property);
#endif

  window = ctk_widget_get_window (widget);

  tmp_list = current_retrievals;
  while (tmp_list)
    {
      info = (CtkRetrievalInfo *)tmp_list->data;
      if (info->widget == widget && info->selection == event->selection)
	break;
      tmp_list = tmp_list->next;
    }
  
  if (!tmp_list)		/* no retrieval in progress */
    return FALSE;

  if (event->property != CDK_NONE)
    length = cdk_selection_property_get (window, &buffer,
					 &type, &format);
  else
    length = 0; /* silence gcc */
  
  if (event->property == CDK_NONE || buffer == NULL)
    {
      current_retrievals = g_list_remove_link (current_retrievals, tmp_list);
      g_list_free (tmp_list);
      /* structure will be freed in timeout */
      ctk_selection_retrieval_report (info,
				      CDK_NONE, 0, NULL, -1, event->time);
      
      return TRUE;
    }
  
  if (type == ctk_selection_atoms[INCR])
    {
      /* The remainder of the selection will come through PropertyNotify
	 events */

      info->notify_time = event->time;
      info->idle_time = 0;
      info->offset = 0;		/* Mark as OK to proceed */
      cdk_window_set_events (window,
                             cdk_window_get_events (window)
			     | CDK_PROPERTY_CHANGE_MASK);
    }
  else
    {
      /* We don't delete the info structure - that will happen in timeout */
      current_retrievals = g_list_remove_link (current_retrievals, tmp_list);
      g_list_free (tmp_list);
      
      info->offset = length;
      ctk_selection_retrieval_report (info,
				      type, format, 
				      buffer, length, event->time);
    }

  cdk_property_delete (window, event->property);

  g_free (buffer);
  
  return TRUE;
}

/*************************************************************
 * _ctk_selection_property_notify:
 *     Handler for “property-notify-event” signals on windows
 *     where a retrieval is currently in process. The selection
 *     owner has added more data.
 *   arguments:
 *     widget:		Widget getting signal
 *     event:		Property event structure
 *     info:		Information about this retrieval
 *   results:
 *     was event handled?
 *************************************************************/

gboolean
_ctk_selection_property_notify (CtkWidget	*widget,
				CdkEventProperty *event)
{
  GList *tmp_list;
  CtkRetrievalInfo *info = NULL;
  CdkWindow *window;
  guchar *new_buffer;
  int length;
  CdkAtom type;
  gint	  format;
  
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

#if defined(CDK_WINDOWING_WIN32) || defined(CDK_WINDOWING_X11)
  if ((event->state != CDK_PROPERTY_NEW_VALUE) ||  /* property was deleted */
      (event->atom != cdk_atom_intern_static_string ("CDK_SELECTION"))) /* not the right property */
#endif
    return FALSE;
  
#ifdef DEBUG_SELECTION
  g_message ("PropertyNewValue, property %ld",
	     event->atom);
#endif
  
  tmp_list = current_retrievals;
  while (tmp_list)
    {
      info = (CtkRetrievalInfo *)tmp_list->data;
      if (info->widget == widget)
	break;
      tmp_list = tmp_list->next;
    }
  
  if (!tmp_list)		/* No retrieval in progress */
    return FALSE;
  
  if (info->offset < 0)		/* We haven't got the SelectionNotify
				   for this retrieval yet */
    return FALSE;
  
  info->idle_time = 0;

  window = ctk_widget_get_window (widget);
  length = cdk_selection_property_get (window, &new_buffer,
				       &type, &format);
  cdk_property_delete (window, event->atom);

  /* We could do a lot better efficiency-wise by paying attention to
     what length was sent in the initial INCR transaction, instead of
     doing memory allocation at every step. But its only guaranteed to
     be a _lower bound_ (pretty useless!) */
  
  if (length == 0 || type == CDK_NONE)		/* final zero length portion */
    {
      /* Info structure will be freed in timeout */
      current_retrievals = g_list_remove_link (current_retrievals, tmp_list);
      g_list_free (tmp_list);
      ctk_selection_retrieval_report (info,
				      type, format, 
				      (type == CDK_NONE) ?  NULL : info->buffer,
				      (type == CDK_NONE) ?  -1 : info->offset,
				      info->notify_time);
    }
  else				/* append on newly arrived data */
    {
      if (!info->buffer)
	{
#ifdef DEBUG_SELECTION
	  g_message ("Start - Adding %d bytes at offset 0",
		     length);
#endif
	  info->buffer = new_buffer;
	  info->offset = length;
	}
      else
	{
	  
#ifdef DEBUG_SELECTION
	  g_message ("Appending %d bytes at offset %d",
		     length,info->offset);
#endif
	  /* We copy length+1 bytes to preserve guaranteed null termination */
	  info->buffer = g_realloc (info->buffer, info->offset+length+1);
	  memcpy (info->buffer + info->offset, new_buffer, length+1);
	  info->offset += length;
	  g_free (new_buffer);
	}
    }
  
  return TRUE;
}

/*************************************************************
 * ctk_selection_retrieval_timeout:
 *     Timeout callback while receiving a selection.
 *   arguments:
 *     info:	Information about this retrieval
 *   results:
 *************************************************************/

static gboolean
ctk_selection_retrieval_timeout (CtkRetrievalInfo *info)
{
  GList *tmp_list;
  gboolean retval;

  /* Determine if retrieval has finished by checking if it still in
     list of pending retrievals */
  
  tmp_list = current_retrievals;
  while (tmp_list)
    {
      if (info == (CtkRetrievalInfo *)tmp_list->data)
	break;
      tmp_list = tmp_list->next;
    }
  
  /* If retrieval is finished */
  if (!tmp_list || info->idle_time >= IDLE_ABORT_TIME)
    {
      if (tmp_list && info->idle_time >= IDLE_ABORT_TIME)
	{
	  current_retrievals = g_list_remove_link (current_retrievals, tmp_list);
	  g_list_free (tmp_list);
	  ctk_selection_retrieval_report (info, CDK_NONE, 0, NULL, -1, CDK_CURRENT_TIME);
	}
      
      g_free (info->buffer);
      g_slice_free (CtkRetrievalInfo, info);
      
      retval =  FALSE;		/* remove timeout */
    }
  else
    {
      info->idle_time++;
      
      retval =  TRUE;		/* timeout will happen again */
    }

  return retval;
}

/*************************************************************
 * ctk_selection_retrieval_report:
 *     Emits a “selection-received” signal.
 *   arguments:
 *     info:	  information about the retrieval that completed
 *     buffer:	  buffer containing data (NULL => errror)
 *     time:      timestamp for data in buffer
 *   results:
 *************************************************************/

static void
ctk_selection_retrieval_report (CtkRetrievalInfo *info,
				CdkAtom type, gint format, 
				guchar *buffer, gint length,
				guint32 time)
{
  CtkSelectionData data;
  
  data.selection = info->selection;
  data.target = info->target;
  data.type = type;
  data.format = format;
  
  data.length = length;
  data.data = buffer;
  data.display = ctk_widget_get_display (info->widget);
  
  g_signal_emit_by_name (info->widget,
			 "selection-received", 
			 &data, time);
}

/*************************************************************
 * ctk_selection_invoke_handler:
 *     Finds and invokes handler for specified
 *     widget/selection/target combination, calls 
 *     ctk_selection_default_handler if none exists.
 *
 *   arguments:
 *     widget:	    selection owner
 *     data:	    selection data [INOUT]
 *     time:        time from requeset
 *     
 *   results:
 *     Number of bytes written to buffer, -1 if error
 *************************************************************/

static void
ctk_selection_invoke_handler (CtkWidget	       *widget,
			      CtkSelectionData *data,
			      guint             time)
{
  CtkTargetList *target_list;
  guint info;
  

  g_return_if_fail (widget != NULL);

  target_list = ctk_selection_target_list_get (widget, data->selection);
  if (data->target != ctk_selection_atoms[SAVE_TARGETS] &&
      target_list &&
      ctk_target_list_find (target_list, data->target, &info))
    {
      g_signal_emit_by_name (widget,
			     "selection-get",
			     data,
			     info, time);
    }
  else
    ctk_selection_default_handler (widget, data);
}

/*************************************************************
 * ctk_selection_default_handler:
 *     Handles some default targets that exist for any widget
 *     If it can’t fit results into buffer, returns -1. This
 *     won’t happen in any conceivable case, since it would
 *     require 1000 selection targets!
 *
 *   arguments:
 *     widget:	    selection owner
 *     data:	    selection data [INOUT]
 *
 *************************************************************/

static void
ctk_selection_default_handler (CtkWidget	*widget,
			       CtkSelectionData *data)
{
  if (data->target == ctk_selection_atoms[TIMESTAMP])
    {
      /* Time which was used to obtain selection */
      GList *tmp_list;
      CtkSelectionInfo *selection_info;
      
      tmp_list = current_selections;
      while (tmp_list)
	{
	  selection_info = (CtkSelectionInfo *)tmp_list->data;
	  if ((selection_info->widget == widget) &&
	      (selection_info->selection == data->selection))
	    {
	      gulong time = selection_info->time;

	      ctk_selection_data_set (data,
				      CDK_SELECTION_TYPE_INTEGER,
				      32,
				      (guchar *)&time,
				      sizeof (time));
	      return;
	    }
	  
	  tmp_list = tmp_list->next;
	}
      
      data->length = -1;
    }
  else if (data->target == ctk_selection_atoms[TARGETS])
    {
      /* List of all targets supported for this widget/selection pair */
      CdkAtom *p;
      guint count;
      GList *tmp_list;
      CtkTargetList *target_list;
      CtkTargetPair *pair;
      
      target_list = ctk_selection_target_list_get (widget,
						   data->selection);
      count = g_list_length (target_list->list) + 3;
      
      data->type = CDK_SELECTION_TYPE_ATOM;
      data->format = 32;
      data->length = count * sizeof (CdkAtom);

      /* selection data is always terminated by a trailing \0
       */
      p = g_malloc (data->length + 1);
      data->data = (guchar *)p;
      data->data[data->length] = '\0';
      
      *p++ = ctk_selection_atoms[TIMESTAMP];
      *p++ = ctk_selection_atoms[TARGETS];
      *p++ = ctk_selection_atoms[MULTIPLE];
      
      tmp_list = target_list->list;
      while (tmp_list)
	{
	  pair = (CtkTargetPair *)tmp_list->data;
	  *p++ = pair->target;
	  
	  tmp_list = tmp_list->next;
	}
    }
  else if (data->target == ctk_selection_atoms[SAVE_TARGETS])
    {
      ctk_selection_data_set (data,
			      cdk_atom_intern_static_string ("NULL"),
			      32, NULL, 0);
    }
  else
    {
      data->length = -1;
    }
}


/**
 * ctk_selection_data_copy:
 * @data: a pointer to a #CtkSelectionData-struct.
 * 
 * Makes a copy of a #CtkSelectionData-struct and its data.
 * 
 * Returns: a pointer to a copy of @data.
 **/
CtkSelectionData*
ctk_selection_data_copy (const CtkSelectionData *data)
{
  CtkSelectionData *new_data;
  
  g_return_val_if_fail (data != NULL, NULL);
  
  new_data = g_slice_new (CtkSelectionData);
  *new_data = *data;

  if (data->data)
    {
      new_data->data = g_malloc (data->length + 1);
      memcpy (new_data->data, data->data, data->length + 1);
    }
  
  return new_data;
}

/**
 * ctk_selection_data_free:
 * @data: a pointer to a #CtkSelectionData-struct.
 * 
 * Frees a #CtkSelectionData-struct returned from
 * ctk_selection_data_copy().
 **/
void
ctk_selection_data_free (CtkSelectionData *data)
{
  g_return_if_fail (data != NULL);
  
  g_free (data->data);
  
  g_slice_free (CtkSelectionData, data);
}

/**
 * ctk_target_entry_new:
 * @target: String identifier for target
 * @flags: Set of flags, see #CtkTargetFlags
 * @info: an ID that will be passed back to the application
 *
 * Makes a new #CtkTargetEntry.
 *
 * Returns: a pointer to a new #CtkTargetEntry.
 *     Free with ctk_target_entry_free()
 **/
CtkTargetEntry *
ctk_target_entry_new (const char *target,
		      guint       flags,
		      guint       info)
{
  CtkTargetEntry entry = { (char *) target, flags, info };
  return ctk_target_entry_copy (&entry);
}

/**
 * ctk_target_entry_copy:
 * @data: a pointer to a #CtkTargetEntry
 *
 * Makes a copy of a #CtkTargetEntry and its data.
 *
 * Returns: a pointer to a copy of @data.
 *     Free with ctk_target_entry_free()
 **/
CtkTargetEntry *
ctk_target_entry_copy (CtkTargetEntry *data)
{
  CtkTargetEntry *new_data;

  g_return_val_if_fail (data != NULL, NULL);

  new_data = g_slice_new (CtkTargetEntry);
  new_data->target = g_strdup (data->target);
  new_data->flags = data->flags;
  new_data->info = data->info;

  return new_data;
}

/**
 * ctk_target_entry_free:
 * @data: a pointer to a #CtkTargetEntry.
 *
 * Frees a #CtkTargetEntry returned from
 * ctk_target_entry_new() or ctk_target_entry_copy().
 **/
void
ctk_target_entry_free (CtkTargetEntry *data)
{
  g_return_if_fail (data != NULL);

  g_free (data->target);

  g_slice_free (CtkTargetEntry, data);
}


G_DEFINE_BOXED_TYPE (CtkSelectionData, ctk_selection_data,
                     ctk_selection_data_copy,
                     ctk_selection_data_free)

G_DEFINE_BOXED_TYPE (CtkTargetList, ctk_target_list,
                     ctk_target_list_ref,
                     ctk_target_list_unref)

G_DEFINE_BOXED_TYPE (CtkTargetEntry, ctk_target_entry,
                     ctk_target_entry_copy,
                     ctk_target_entry_free)

static int
ctk_selection_bytes_per_item (gint format)
{
  switch (format)
    {
    case 8:
      return sizeof (char);
      break;
    case 16:
      return sizeof (short);
      break;
    case 32:
      return sizeof (long);
      break;
    default:
      g_assert_not_reached();
    }
  return 0;
}
