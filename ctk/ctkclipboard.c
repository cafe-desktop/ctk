/* CTK - The GIMP Toolkit
 * Copyright (C) 2000 Red Hat, Inc.
 * Copyright (C) 2004 Nokia Corporation
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
 *
 * Global clipboard abstraction. 
 */

#include "config.h"
#include <string.h>

#include "ctkclipboard.h"
#include "ctkclipboardprivate.h"
#include "ctkinvisible.h"
#include "ctkmain.h"
#include "ctkmarshalers.h"
#include "ctktextbufferrichtext.h"
#include "ctkintl.h"

#ifdef CDK_WINDOWING_X11
#include "x11/cdkx.h"
#endif

#ifdef CDK_WINDOWING_BROADWAY
#include "broadway/cdkbroadway.h"
#endif

#ifdef CDK_WINDOWING_WIN32
#include "win32/cdkwin32.h"
#endif


/**
 * SECTION:ctkclipboard
 * @Short_description: Storing data on clipboards
 * @Title: Clipboards
 * @See_also: #CtkSelectionData
 *
 * The #CtkClipboard object represents a clipboard of data shared
 * between different processes or between different widgets in
 * the same process. Each clipboard is identified by a name encoded as a
 * #CdkAtom. (Conversion to and from strings can be done with
 * cdk_atom_intern() and cdk_atom_name().) The default clipboard
 * corresponds to the “CLIPBOARD” atom; another commonly used clipboard
 * is the “PRIMARY” clipboard, which, in X, traditionally contains
 * the currently selected text.
 *
 * To support having a number of different formats on the clipboard
 * at the same time, the clipboard mechanism allows providing
 * callbacks instead of the actual data.  When you set the contents
 * of the clipboard, you can either supply the data directly (via
 * functions like ctk_clipboard_set_text()), or you can supply a
 * callback to be called at a later time when the data is needed (via
 * ctk_clipboard_set_with_data() or ctk_clipboard_set_with_owner().)
 * Providing a callback also avoids having to make copies of the data
 * when it is not needed.
 *
 * ctk_clipboard_set_with_data() and ctk_clipboard_set_with_owner()
 * are quite similar; the choice between the two depends mostly on
 * which is more convenient in a particular situation.
 * The former is most useful when you want to have a blob of data
 * with callbacks to convert it into the various data types that you
 * advertise. When the @clear_func you provided is called, you
 * simply free the data blob. The latter is more useful when the
 * contents of clipboard reflect the internal state of a #GObject
 * (As an example, for the PRIMARY clipboard, when an entry widget
 * provides the clipboard’s contents the contents are simply the
 * text within the selected region.) If the contents change, the
 * entry widget can call ctk_clipboard_set_with_owner() to update
 * the timestamp for clipboard ownership, without having to worry
 * about @clear_func being called.
 *
 * Requesting the data from the clipboard is essentially
 * asynchronous. If the contents of the clipboard are provided within
 * the same process, then a direct function call will be made to
 * retrieve the data, but if they are provided by another process,
 * then the data needs to be retrieved from the other process, which
 * may take some time. To avoid blocking the user interface, the call
 * to request the selection, ctk_clipboard_request_contents() takes a
 * callback that will be called when the contents are received (or
 * when the request fails.) If you don’t want to deal with providing
 * a separate callback, you can also use ctk_clipboard_wait_for_contents().
 * What this does is run the GLib main loop recursively waiting for
 * the contents. This can simplify the code flow, but you still have
 * to be aware that other callbacks in your program can be called
 * while this recursive mainloop is running.
 *
 * Along with the functions to get the clipboard contents as an
 * arbitrary data chunk, there are also functions to retrieve
 * it as text, ctk_clipboard_request_text() and
 * ctk_clipboard_wait_for_text(). These functions take care of
 * determining which formats are advertised by the clipboard
 * provider, asking for the clipboard in the best available format
 * and converting the results into the UTF-8 encoding. (The standard
 * form for representing strings in CTK+.)
 */


enum {
  OWNER_CHANGE,
  LAST_SIGNAL
};

typedef struct _RequestContentsInfo RequestContentsInfo;
typedef struct _RequestTextInfo RequestTextInfo;
typedef struct _RequestRichTextInfo RequestRichTextInfo;
typedef struct _RequestImageInfo RequestImageInfo;
typedef struct _RequestURIInfo RequestURIInfo;
typedef struct _RequestTargetsInfo RequestTargetsInfo;

struct _RequestContentsInfo
{
  CtkClipboardReceivedFunc callback;
  gpointer user_data;
};

struct _RequestTextInfo
{
  CtkClipboardTextReceivedFunc callback;
  gpointer user_data;
};

struct _RequestRichTextInfo
{
  CtkClipboardRichTextReceivedFunc callback;
  CdkAtom *atoms;
  gint     n_atoms;
  gint     current_atom;
  gpointer user_data;
};

struct _RequestImageInfo
{
  CtkClipboardImageReceivedFunc callback;
  gpointer user_data;
};

struct _RequestURIInfo
{
  CtkClipboardURIReceivedFunc callback;
  gpointer user_data;
};

struct _RequestTargetsInfo
{
  CtkClipboardTargetsReceivedFunc callback;
  gpointer user_data;
};

static void ctk_clipboard_class_init   (CtkClipboardClass   *class);
static void ctk_clipboard_finalize     (GObject             *object);
static void ctk_clipboard_owner_change (CtkClipboard        *clipboard,
					CdkEventOwnerChange *event);
static gboolean ctk_clipboard_set_contents      (CtkClipboard                   *clipboard,
                                                 const CtkTargetEntry           *targets,
                                                 guint                           n_targets,
                                                 CtkClipboardGetFunc             get_func,
                                                 CtkClipboardClearFunc           clear_func,
                                                 gpointer                        user_data,
                                                 gboolean                        have_owner);
static void ctk_clipboard_real_clear            (CtkClipboard                   *clipboard);
static void ctk_clipboard_real_request_contents (CtkClipboard                   *clipboard,
                                                 CdkAtom                         target,
                                                 CtkClipboardReceivedFunc        callback,
                                                 gpointer                        user_data);
static void ctk_clipboard_real_set_can_store    (CtkClipboard                   *clipboard,
                                                 const CtkTargetEntry           *targets,
                                                 gint                            n_targets);
static void ctk_clipboard_real_store            (CtkClipboard                   *clipboard);


static void          clipboard_unset      (CtkClipboard     *clipboard);
static void          selection_received   (CtkWidget        *widget,
					   CtkSelectionData *selection_data,
					   guint             time);
static CtkClipboard *clipboard_peek       (CdkDisplay       *display,
					   CdkAtom           selection,
					   gboolean          only_if_exists);
static CtkWidget *   get_clipboard_widget (CdkDisplay       *display);


enum {
  TARGET_STRING,
  TARGET_TEXT,
  TARGET_COMPOUND_TEXT,
  TARGET_UTF8_STRING,
  TARGET_SAVE_TARGETS
};

static const gchar request_contents_key[] = "ctk-request-contents";
static GQuark request_contents_key_id = 0;

static const gchar clipboards_owned_key[] = "ctk-clipboards-owned";
static GQuark clipboards_owned_key_id = 0;

static guint         clipboard_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (CtkClipboard, ctk_clipboard, G_TYPE_OBJECT)

static void
ctk_clipboard_init (CtkClipboard *object)
{
}

static void
ctk_clipboard_class_init (CtkClipboardClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

  gobject_class->finalize = ctk_clipboard_finalize;

  class->set_contents = ctk_clipboard_set_contents;
  class->clear = ctk_clipboard_real_clear;
  class->request_contents = ctk_clipboard_real_request_contents;
  class->set_can_store = ctk_clipboard_real_set_can_store;
  class->store = ctk_clipboard_real_store;
  class->owner_change = ctk_clipboard_owner_change;

  /**
   * CtkClipboard::owner-change:
   * @clipboard: the #CtkClipboard on which the signal is emitted
   * @event: (type Cdk.EventOwnerChange): the @CdkEventOwnerChange event
   *
   * The ::owner-change signal is emitted when CTK+ receives an
   * event that indicates that the ownership of the selection
   * associated with @clipboard has changed.
   *
   * Since: 2.6
   */
  clipboard_signals[OWNER_CHANGE] =
    g_signal_new (I_("owner-change"),
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (CtkClipboardClass, owner_change),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 1,
		  CDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
}

static void
ctk_clipboard_finalize (GObject *object)
{
  CtkClipboard *clipboard;
  CtkWidget *clipboard_widget = NULL;
  GSList *clipboards = NULL;

  clipboard = CTK_CLIPBOARD (object);

  if (clipboard->display)
    {
      clipboards = g_object_get_data (G_OBJECT (clipboard->display), "ctk-clipboard-list");

      if (g_slist_index (clipboards, clipboard) >= 0)
        g_warning ("CtkClipboard prematurely finalized");

      clipboards = g_slist_remove (clipboards, clipboard);

      g_object_set_data (G_OBJECT (clipboard->display), "ctk-clipboard-list", 
                         clipboards);

      /* don't use get_clipboard_widget() here because it would create the
       * widget if it doesn't exist.
       */
      clipboard_widget = g_object_get_data (G_OBJECT (clipboard->display),
                                            "ctk-clipboard-widget");
    }

  clipboard_unset (clipboard);

  if (clipboard->store_loop && g_main_loop_is_running (clipboard->store_loop))
    g_main_loop_quit (clipboard->store_loop);

  if (clipboard->store_timeout != 0)
    g_source_remove (clipboard->store_timeout);

  if (clipboard_widget != NULL && clipboard->notify_signal_id != 0)
    g_signal_handler_disconnect (clipboard_widget, clipboard->notify_signal_id);

  g_free (clipboard->storable_targets);
  g_free (clipboard->cached_targets);

  G_OBJECT_CLASS (ctk_clipboard_parent_class)->finalize (object);
}

static void
clipboard_display_closed (CdkDisplay   *display,
			  gboolean      is_error,
			  CtkClipboard *clipboard)
{
  GSList *clipboards;

  clipboards = g_object_get_data (G_OBJECT (display), "ctk-clipboard-list");
  g_object_run_dispose (G_OBJECT (clipboard));
  clipboards = g_slist_remove (clipboards, clipboard);
  g_object_set_data (G_OBJECT (display), I_("ctk-clipboard-list"), clipboards);
  g_object_unref (clipboard);
}

/**
 * ctk_clipboard_get_for_display:
 * @display: the #CdkDisplay for which the clipboard is to be retrieved or created.
 * @selection: a #CdkAtom which identifies the clipboard to use.
 *
 * Returns the clipboard object for the given selection.
 * Cut/copy/paste menu items and keyboard shortcuts should use
 * the default clipboard, returned by passing %CDK_SELECTION_CLIPBOARD for @selection.
 * (%CDK_NONE is supported as a synonym for CDK_SELECTION_CLIPBOARD
 * for backwards compatibility reasons.)
 * The currently-selected object or text should be provided on the clipboard
 * identified by #CDK_SELECTION_PRIMARY. Cut/copy/paste menu items
 * conceptually copy the contents of the #CDK_SELECTION_PRIMARY clipboard
 * to the default clipboard, i.e. they copy the selection to what the
 * user sees as the clipboard.
 *
 * (Passing #CDK_NONE is the same as using `cdk_atom_intern
 * ("CLIPBOARD", FALSE)`.
 *
 * See the
 * [FreeDesktop Clipboard Specification](http://www.freedesktop.org/Standards/clipboards-spec)
 * for a detailed discussion of the “CLIPBOARD” vs. “PRIMARY”
 * selections under the X window system. On Win32 the
 * #CDK_SELECTION_PRIMARY clipboard is essentially ignored.)
 *
 * It’s possible to have arbitrary named clipboards; if you do invent
 * new clipboards, you should prefix the selection name with an
 * underscore (because the ICCCM requires that nonstandard atoms are
 * underscore-prefixed), and namespace it as well. For example,
 * if your application called “Foo” has a special-purpose
 * clipboard, you might call it “_FOO_SPECIAL_CLIPBOARD”.
 *
 * Returns: (transfer none): the appropriate clipboard object. If no
 *   clipboard already exists, a new one will be created. Once a clipboard
 *   object has been created, it is persistent and, since it is owned by
 *   CTK+, must not be freed or unrefd.
 *
 * Since: 2.2
 **/
CtkClipboard *
ctk_clipboard_get_for_display (CdkDisplay *display, 
			       CdkAtom     selection)
{
  g_return_val_if_fail (display != NULL, NULL); /* See bgo#463773; this is needed because Flash Player sucks */
  g_return_val_if_fail (CDK_IS_DISPLAY (display), NULL);
  g_return_val_if_fail (!cdk_display_is_closed (display), NULL);

  return clipboard_peek (display, selection, FALSE);
}


/**
 * ctk_clipboard_get:
 * @selection: a #CdkAtom which identifies the clipboard to use
 *
 * Returns the clipboard object for the given selection.
 * See ctk_clipboard_get_for_display() for complete details.
 *
 * Returns: (transfer none): the appropriate clipboard object. If no clipboard
 *     already exists, a new one will be created. Once a clipboard
 *     object has been created, it is persistent and, since it is
 *     owned by CTK+, must not be freed or unreffed.
 */
CtkClipboard *
ctk_clipboard_get (CdkAtom selection)
{
  return ctk_clipboard_get_for_display (cdk_display_get_default (), selection);
}

/**
 * ctk_clipboard_get_default:
 * @display: the #CdkDisplay for which the clipboard is to be retrieved.
 *
 * Returns the default clipboard object for use with cut/copy/paste menu items
 * and keyboard shortcuts.
 *
 * Return value: (transfer none): the default clipboard object.
 *
 * Since: 3.16
 **/
CtkClipboard *
ctk_clipboard_get_default (CdkDisplay *display)
{
  g_return_val_if_fail (display != NULL, NULL);
  g_return_val_if_fail (CDK_IS_DISPLAY (display), NULL);

  return ctk_clipboard_get_for_display (display, CDK_SELECTION_CLIPBOARD);
}

static void 
selection_get_cb (CtkWidget          *widget,
		  CtkSelectionData   *selection_data,
		  guint               info,
		  guint               time)
{
  CtkClipboard *clipboard;

  clipboard = ctk_widget_get_clipboard (widget,
                                        ctk_selection_data_get_selection (selection_data));

  if (clipboard && clipboard->get_func)
    clipboard->get_func (clipboard, selection_data, info, clipboard->user_data);
}

static gboolean
selection_clear_event_cb (CtkWidget	    *widget,
			  CdkEventSelection *event)
{
  CtkClipboard *clipboard = ctk_widget_get_clipboard (widget, event->selection);

  if (clipboard)
    {
      clipboard_unset (clipboard);

      return TRUE;
    }

  return FALSE;
}

static CtkWidget *
make_clipboard_widget (CdkDisplay *display, 
		       gboolean    provider)
{
  CtkWidget *widget = ctk_invisible_new_for_screen (cdk_display_get_default_screen (display));

  g_signal_connect (widget, "selection-received",
		    G_CALLBACK (selection_received), NULL);

  if (provider)
    {
      /* We need this for cdk_x11_get_server_time() */
      ctk_widget_add_events (widget, CDK_PROPERTY_CHANGE_MASK);
      
      g_signal_connect (widget, "selection-get",
			G_CALLBACK (selection_get_cb), NULL);
      g_signal_connect (widget, "selection-clear-event",
			G_CALLBACK (selection_clear_event_cb), NULL);
    }

  return widget;
}

static CtkWidget *
get_clipboard_widget (CdkDisplay *display)
{
  CtkWidget *clip_widget = g_object_get_data (G_OBJECT (display), "ctk-clipboard-widget");
  if (!clip_widget)
    {
      clip_widget = make_clipboard_widget (display, TRUE);
      g_object_set_data (G_OBJECT (display), I_("ctk-clipboard-widget"), clip_widget);
    }

  return clip_widget;
}

/* This function makes a very good guess at what the correct
 * timestamp for a selection request should be. If there is
 * a currently processed event, it uses the timestamp for that
 * event, otherwise it uses the current server time. However,
 * if the time resulting from that is older than the time used
 * last time, it uses the time used last time instead.
 *
 * In order implement this correctly, we never use CurrentTime,
 * but actually retrieve the actual timestamp from the server.
 * This is a little slower but allows us to make the guarantee
 * that the times used by this application will always ascend
 * and we won’t get selections being rejected just because
 * we are using a correct timestamp from an event, but used
 * CurrentTime previously.
 */
static guint32
clipboard_get_timestamp (CtkClipboard *clipboard)
{
  CtkWidget *clipboard_widget = get_clipboard_widget (clipboard->display);
  guint32 timestamp = ctk_get_current_event_time ();
  CdkWindow *window;

  if (timestamp == CDK_CURRENT_TIME)
    {
      window = ctk_widget_get_window (clipboard_widget);
#ifdef CDK_WINDOWING_X11
      if (CDK_IS_X11_WINDOW (window))
	{
	  timestamp = cdk_x11_get_server_time (ctk_widget_get_window (clipboard_widget));
	}
      else
#endif
#if defined CDK_WINDOWING_WIN32
      if (CDK_IS_WIN32_WINDOW (window))
	{
	  timestamp = GetMessageTime ();
	}
      else
#endif
#if defined CDK_WINDOWING_BROADWAY
      if (CDK_IS_BROADWAY_WINDOW (window))
	{
	  timestamp = cdk_broadway_get_last_seen_time (window);
	}
      else
#endif
	{
	  /* No implementation */
	}
    }
  else
    {
      if (clipboard->timestamp != CDK_CURRENT_TIME)
	{
	  /* Check to see if clipboard->timestamp is newer than
	   * timestamp, accounting for wraparound.
	   */

	  guint32 max = timestamp + 0x80000000;

	  if ((max > timestamp &&
	       (clipboard->timestamp > timestamp &&
		clipboard->timestamp <= max)) ||
	      (max <= timestamp &&
	       (clipboard->timestamp > timestamp ||
		clipboard->timestamp <= max)))
	    {
	      timestamp = clipboard->timestamp;
	    }
	}
    }

  clipboard->timestamp = timestamp;

  return timestamp;
}

static void
clipboard_owner_destroyed (gpointer data)
{
  GSList *clipboards = data;
  GSList *tmp_list;

  tmp_list = clipboards;
  while (tmp_list)
    {
      CtkClipboard *clipboard = tmp_list->data;

      clipboard->get_func = NULL;
      clipboard->clear_func = NULL;
      clipboard->user_data = NULL;
      clipboard->have_owner = FALSE;

      ctk_clipboard_clear (clipboard);

      tmp_list = tmp_list->next;
    }
  
  g_slist_free (clipboards);
}

static void
clipboard_add_owner_notify (CtkClipboard *clipboard)
{
  if (!clipboards_owned_key_id)
    clipboards_owned_key_id = g_quark_from_static_string (clipboards_owned_key);
  
  if (clipboard->have_owner)
    g_object_set_qdata_full (clipboard->user_data, clipboards_owned_key_id,
			     g_slist_prepend (g_object_steal_qdata (clipboard->user_data,
								    clipboards_owned_key_id),
					      clipboard),
			     clipboard_owner_destroyed);
}

static void
clipboard_remove_owner_notify (CtkClipboard *clipboard)
{
  if (clipboard->have_owner)
     g_object_set_qdata_full (clipboard->user_data, clipboards_owned_key_id,
			      g_slist_remove (g_object_steal_qdata (clipboard->user_data,
								    clipboards_owned_key_id),
					      clipboard),
			      clipboard_owner_destroyed);
}
	  
static gboolean
ctk_clipboard_set_contents (CtkClipboard         *clipboard,
			    const CtkTargetEntry *targets,
			    guint                 n_targets,
			    CtkClipboardGetFunc   get_func,
			    CtkClipboardClearFunc clear_func,
			    gpointer              user_data,
			    gboolean              have_owner)
{
  CtkWidget *clipboard_widget = get_clipboard_widget (clipboard->display);

  if (ctk_selection_owner_set_for_display (clipboard->display,
					   clipboard_widget,
					   clipboard->selection,
					   clipboard_get_timestamp (clipboard)))
    {
      clipboard->have_selection = TRUE;

      if (clipboard->n_cached_targets != -1)
        {
          g_free (clipboard->cached_targets);
	  clipboard->cached_targets = NULL;
          clipboard->n_cached_targets = -1;
        }

      if (!(clipboard->have_owner && have_owner) ||
	  clipboard->user_data != user_data)
	{
	  clipboard_unset (clipboard);

	  clipboard->user_data = user_data;
	  clipboard->have_owner = have_owner;
	  if (have_owner)
	      clipboard_add_owner_notify (clipboard);
 	}

      clipboard->get_func = get_func;
      clipboard->clear_func = clear_func;

      ctk_selection_clear_targets (clipboard_widget, clipboard->selection);
      ctk_selection_add_targets (clipboard_widget, clipboard->selection,
				 targets, n_targets);

      return TRUE;
    }
  else
    return FALSE;
}

/**
 * ctk_clipboard_set_with_data: (skip)
 * @clipboard: a #CtkClipboard
 * @targets: (array length=n_targets): array containing information
 *     about the available forms for the clipboard data
 * @n_targets: number of elements in @targets
 * @get_func: (scope async): function to call to get the actual clipboard data
 * @clear_func: (scope async): when the clipboard contents are set again,
 *     this function will be called, and @get_func will not be subsequently
 *     called.
 * @user_data: user data to pass to @get_func and @clear_func.
 *
 * Virtually sets the contents of the specified clipboard by providing
 * a list of supported formats for the clipboard data and a function
 * to call to get the actual data when it is requested.
 *
 * Returns: %TRUE if setting the clipboard data succeeded.
 *    If setting the clipboard data failed the provided callback
 *    functions will be ignored.
 **/
gboolean
ctk_clipboard_set_with_data (CtkClipboard          *clipboard,
			     const CtkTargetEntry  *targets,
			     guint                  n_targets,
			     CtkClipboardGetFunc    get_func,
			     CtkClipboardClearFunc  clear_func,
			     gpointer               user_data)
{
  g_return_val_if_fail (clipboard != NULL, FALSE);
  g_return_val_if_fail (targets != NULL, FALSE);
  g_return_val_if_fail (get_func != NULL, FALSE);

  return CTK_CLIPBOARD_GET_CLASS (clipboard)->set_contents (clipboard,
                                                            targets,
                                                            n_targets,
				                            get_func,
                                                            clear_func,
                                                            user_data,
				                            FALSE);
}

/**
 * ctk_clipboard_set_with_owner: (skip)
 * @clipboard: a #CtkClipboard
 * @targets: (array length=n_targets): array containing information
 *     about the available forms for the clipboard data
 * @n_targets: number of elements in @targets
 * @get_func: (scope async): function to call to get the actual clipboard data
 * @clear_func: (scope async): when the clipboard contents are set again,
 *     this function will be called, and @get_func will not be subsequently
 *     called
 * @owner: an object that “owns” the data. This object will be passed
 *     to the callbacks when called
 *
 * Virtually sets the contents of the specified clipboard by providing
 * a list of supported formats for the clipboard data and a function
 * to call to get the actual data when it is requested.
 *
 * The difference between this function and ctk_clipboard_set_with_data()
 * is that instead of an generic @user_data pointer, a #GObject is passed
 * in.
 *
 * Returns: %TRUE if setting the clipboard data succeeded.
 *     If setting the clipboard data failed the provided callback
 *     functions will be ignored.
 **/
gboolean
ctk_clipboard_set_with_owner (CtkClipboard          *clipboard,
			      const CtkTargetEntry  *targets,
			      guint                  n_targets,
			      CtkClipboardGetFunc    get_func,
			      CtkClipboardClearFunc  clear_func,
			      GObject               *owner)
{
  g_return_val_if_fail (clipboard != NULL, FALSE);
  g_return_val_if_fail (targets != NULL, FALSE);
  g_return_val_if_fail (get_func != NULL, FALSE);
  g_return_val_if_fail (G_IS_OBJECT (owner), FALSE);

  return CTK_CLIPBOARD_GET_CLASS (clipboard)->set_contents (clipboard,
                                                            targets,
                                                            n_targets,
				                            get_func,
                                                            clear_func,
                                                            owner,
				                            TRUE);
}

/**
 * ctk_clipboard_get_owner:
 * @clipboard: a #CtkClipboard
 *
 * If the clipboard contents callbacks were set with
 * ctk_clipboard_set_with_owner(), and the ctk_clipboard_set_with_data() or
 * ctk_clipboard_clear() has not subsequently called, returns the owner set
 * by ctk_clipboard_set_with_owner().
 *
 * Returns: (nullable) (transfer none): the owner of the clipboard, if any;
 *     otherwise %NULL.
 **/
GObject *
ctk_clipboard_get_owner (CtkClipboard *clipboard)
{
  g_return_val_if_fail (clipboard != NULL, NULL);

  if (clipboard->have_owner)
    return clipboard->user_data;
  else
    return NULL;
}

static void
clipboard_unset (CtkClipboard *clipboard)
{
  CtkClipboardClearFunc old_clear_func;
  gpointer old_data;
  gboolean old_have_owner;
  gint old_n_storable_targets;
  
  old_clear_func = clipboard->clear_func;
  old_data = clipboard->user_data;
  old_have_owner = clipboard->have_owner;
  old_n_storable_targets = clipboard->n_storable_targets;
  
  if (old_have_owner)
    {
      clipboard_remove_owner_notify (clipboard);
      clipboard->have_owner = FALSE;
    }

  clipboard->n_storable_targets = -1;
  g_free (clipboard->storable_targets);
  clipboard->storable_targets = NULL;
      
  clipboard->get_func = NULL;
  clipboard->clear_func = NULL;
  clipboard->user_data = NULL;
  
  if (old_clear_func)
    old_clear_func (clipboard, old_data);

  /* If we've transferred the clipboard data to the manager,
   * unref the owner
   */
  if (old_have_owner &&
      old_n_storable_targets != -1)
    g_object_unref (old_data);
}

/**
 * ctk_clipboard_clear:
 * @clipboard:  a #CtkClipboard
 * 
 * Clears the contents of the clipboard. Generally this should only
 * be called between the time you call ctk_clipboard_set_with_owner()
 * or ctk_clipboard_set_with_data(),
 * and when the @clear_func you supplied is called. Otherwise, the
 * clipboard may be owned by someone else.
 **/
void
ctk_clipboard_clear (CtkClipboard *clipboard)
{
  g_return_if_fail (clipboard != NULL);

  CTK_CLIPBOARD_GET_CLASS (clipboard)->clear (clipboard);
}

static void
ctk_clipboard_real_clear (CtkClipboard *clipboard)
{
  if (clipboard->have_selection)
    ctk_selection_owner_set_for_display (clipboard->display, 
					 NULL,
					 clipboard->selection,
					 clipboard_get_timestamp (clipboard));
}

static void 
text_get_func (CtkClipboard     *clipboard,
	       CtkSelectionData *selection_data,
	       guint             info,
	       gpointer          data)
{
  ctk_selection_data_set_text (selection_data, data, -1);
}

static void 
text_clear_func (CtkClipboard *clipboard,
		 gpointer      data)
{
  g_free (data);
}


/**
 * ctk_clipboard_set_text:
 * @clipboard: a #CtkClipboard object
 * @text:      a UTF-8 string.
 * @len:       length of @text, in bytes, or -1, in which case
 *             the length will be determined with strlen().
 * 
 * Sets the contents of the clipboard to the given UTF-8 string. CTK+ will
 * make a copy of the text and take responsibility for responding
 * for requests for the text, and for converting the text into
 * the requested format.
 **/
void 
ctk_clipboard_set_text (CtkClipboard *clipboard,
			const gchar  *text,
			gint          len)
{
  CtkTargetList *list;
  CtkTargetEntry *targets;
  gint n_targets;

  g_return_if_fail (clipboard != NULL);
  g_return_if_fail (text != NULL);

  list = ctk_target_list_new (NULL, 0);
  ctk_target_list_add_text_targets (list, 0);

  targets = ctk_target_table_new_from_list (list, &n_targets);
  
  if (len < 0)
    len = strlen (text);
  
  ctk_clipboard_set_with_data (clipboard, 
			       targets, n_targets,
			       text_get_func, text_clear_func,
			       g_strndup (text, len));
  ctk_clipboard_set_can_store (clipboard, NULL, 0);

  ctk_target_table_free (targets, n_targets);
  ctk_target_list_unref (list);
}

static void 
pixbuf_get_func (CtkClipboard     *clipboard,
		 CtkSelectionData *selection_data,
		 guint             info,
		 gpointer          data)
{
  ctk_selection_data_set_pixbuf (selection_data, data);
}

static void 
pixbuf_clear_func (CtkClipboard *clipboard,
		   gpointer      data)
{
  g_object_unref (data);
}

/**
 * ctk_clipboard_set_image:
 * @clipboard: a #CtkClipboard object
 * @pixbuf:    a #CdkPixbuf 
 * 
 * Sets the contents of the clipboard to the given #CdkPixbuf. 
 * CTK+ will take responsibility for responding for requests 
 * for the image, and for converting the image into the 
 * requested format.
 * 
 * Since: 2.6
 **/
void
ctk_clipboard_set_image (CtkClipboard *clipboard,
			  CdkPixbuf    *pixbuf)
{
  CtkTargetList *list;
  CtkTargetEntry *targets;
  gint n_targets;

  g_return_if_fail (clipboard != NULL);
  g_return_if_fail (CDK_IS_PIXBUF (pixbuf));

  list = ctk_target_list_new (NULL, 0);
  ctk_target_list_add_image_targets (list, 0, TRUE);

  targets = ctk_target_table_new_from_list (list, &n_targets);

  ctk_clipboard_set_with_data (clipboard, 
			       targets, n_targets,
			       pixbuf_get_func, pixbuf_clear_func,
			       g_object_ref (pixbuf));
  ctk_clipboard_set_can_store (clipboard, NULL, 0);

  ctk_target_table_free (targets, n_targets);
  ctk_target_list_unref (list);
}

static void
set_request_contents_info (CtkWidget           *widget,
			   RequestContentsInfo *info)
{
  if (!request_contents_key_id)
    request_contents_key_id = g_quark_from_static_string (request_contents_key);

  g_object_set_qdata (G_OBJECT (widget), request_contents_key_id, info);
}

static RequestContentsInfo *
get_request_contents_info (CtkWidget *widget)
{
  if (!request_contents_key_id)
    return NULL;
  else
    return g_object_get_qdata (G_OBJECT (widget), request_contents_key_id);
}

static void 
selection_received (CtkWidget            *widget,
		    CtkSelectionData     *selection_data,
		    guint                 time)
{
  RequestContentsInfo *request_info = get_request_contents_info (widget);
  set_request_contents_info (widget, NULL);

  request_info->callback (ctk_widget_get_clipboard (widget, ctk_selection_data_get_selection (selection_data)),
			  selection_data,
			  request_info->user_data);

  g_free (request_info);

  if (widget != get_clipboard_widget (ctk_widget_get_display (widget)))
    ctk_widget_destroy (widget);
}

/**
 * ctk_clipboard_request_contents:
 * @clipboard: a #CtkClipboard
 * @target: an atom representing the form into which the clipboard
 *     owner should convert the selection.
 * @callback: (scope async): A function to call when the results are received
 *     (or the retrieval fails). If the retrieval fails the length field of
 *     @selection_data will be negative.
 * @user_data: user data to pass to @callback
 *
 * Requests the contents of clipboard as the given target.
 * When the results of the result are later received the supplied callback
 * will be called.
 **/
void 
ctk_clipboard_request_contents (CtkClipboard            *clipboard,
				CdkAtom                  target,
				CtkClipboardReceivedFunc callback,
				gpointer                 user_data)
{
  g_return_if_fail (clipboard != NULL);
  g_return_if_fail (target != CDK_NONE);
  g_return_if_fail (callback != NULL);

  CTK_CLIPBOARD_GET_CLASS (clipboard)->request_contents (clipboard,
                                                         target,
                                                         callback,
                                                         user_data);
}

static void 
ctk_clipboard_real_request_contents (CtkClipboard            *clipboard,
                                     CdkAtom                  target,
                                     CtkClipboardReceivedFunc callback,
                                     gpointer                 user_data)
{
  RequestContentsInfo *info;
  CtkWidget *widget;
  CtkWidget *clipboard_widget;

  clipboard_widget = get_clipboard_widget (clipboard->display);

  if (get_request_contents_info (clipboard_widget))
    widget = make_clipboard_widget (clipboard->display, FALSE);
  else
    widget = clipboard_widget;

  info = g_new (RequestContentsInfo, 1);
  info->callback = callback;
  info->user_data = user_data;

  set_request_contents_info (widget, info);

  ctk_selection_convert (widget, clipboard->selection, target,
			 clipboard_get_timestamp (clipboard));
}

static void 
request_text_received_func (CtkClipboard     *clipboard,
			    CtkSelectionData *selection_data,
			    gpointer          data)
{
  RequestTextInfo *info = data;
  gchar *result = NULL;

  result = (gchar *) ctk_selection_data_get_text (selection_data);

  if (!result)
    {
      /* If we asked for UTF8 and didn't get it, try compound_text;
       * if we asked for compound_text and didn't get it, try string;
       * If we asked for anything else and didn't get it, give up.
       */
      CdkAtom target = ctk_selection_data_get_target (selection_data);

      if (target == cdk_atom_intern_static_string ("text/plain;charset=utf-8"))
        {
          ctk_clipboard_request_contents (clipboard,
                                          cdk_atom_intern_static_string ("UTF8_STRING"),
                                          request_text_received_func, info);
          return;
        }
      else if (target == cdk_atom_intern_static_string ("UTF8_STRING"))
	{
	  ctk_clipboard_request_contents (clipboard,
					  cdk_atom_intern_static_string ("COMPOUND_TEXT"), 
					  request_text_received_func, info);
	  return;
	}
      else if (target == cdk_atom_intern_static_string ("COMPOUND_TEXT"))
	{
	  ctk_clipboard_request_contents (clipboard,
					  CDK_TARGET_STRING, 
					  request_text_received_func, info);
	  return;
	}
    }

  info->callback (clipboard, result, info->user_data);
  g_free (info);
  g_free (result);
}

/**
 * ctk_clipboard_request_text:
 * @clipboard: a #CtkClipboard
 * @callback: (scope async): a function to call when the text is received,
 *     or the retrieval fails. (It will always be called one way or the other.)
 * @user_data: user data to pass to @callback.
 *
 * Requests the contents of the clipboard as text. When the text is
 * later received, it will be converted to UTF-8 if necessary, and
 * @callback will be called.
 *
 * The @text parameter to @callback will contain the resulting text if
 * the request succeeded, or %NULL if it failed. This could happen for
 * various reasons, in particular if the clipboard was empty or if the
 * contents of the clipboard could not be converted into text form.
 **/
void 
ctk_clipboard_request_text (CtkClipboard                *clipboard,
			    CtkClipboardTextReceivedFunc callback,
			    gpointer                     user_data)
{
  RequestTextInfo *info;
  
  g_return_if_fail (clipboard != NULL);
  g_return_if_fail (callback != NULL);
  
  info = g_new (RequestTextInfo, 1);
  info->callback = callback;
  info->user_data = user_data;

  ctk_clipboard_request_contents (clipboard, cdk_atom_intern_static_string ("text/plain;charset=utf-8"),
				  request_text_received_func,
				  info);
}

static void
request_rich_text_received_func (CtkClipboard     *clipboard,
                                 CtkSelectionData *selection_data,
                                 gpointer          data)
{
  RequestRichTextInfo *info = data;
  guint8 *result = NULL;
  gsize length = 0;

  result = (guint8 *) ctk_selection_data_get_data (selection_data);
  length = ctk_selection_data_get_length (selection_data);

  info->current_atom++;

  if ((!result || length < 1) && (info->current_atom < info->n_atoms))
    {
      ctk_clipboard_request_contents (clipboard, info->atoms[info->current_atom],
                                      request_rich_text_received_func,
                                      info);
      return;
    }

  info->callback (clipboard, ctk_selection_data_get_target (selection_data),
                  result, length,
                  info->user_data);
  g_free (info->atoms);
  g_free (info);
}

/**
 * ctk_clipboard_request_rich_text:
 * @clipboard: a #CtkClipboard
 * @buffer: a #CtkTextBuffer
 * @callback: (scope async): a function to call when the text is received,
 *     or the retrieval fails. (It will always be called one way or the other.)
 * @user_data: user data to pass to @callback.
 *
 * Requests the contents of the clipboard as rich text. When the rich
 * text is later received, @callback will be called.
 *
 * The @text parameter to @callback will contain the resulting rich
 * text if the request succeeded, or %NULL if it failed. The @length
 * parameter will contain @text’s length. This function can fail for
 * various reasons, in particular if the clipboard was empty or if the
 * contents of the clipboard could not be converted into rich text form.
 *
 * Since: 2.10
 **/
void
ctk_clipboard_request_rich_text (CtkClipboard                    *clipboard,
                                 CtkTextBuffer                   *buffer,
                                 CtkClipboardRichTextReceivedFunc callback,
                                 gpointer                         user_data)
{
  RequestRichTextInfo *info;

  g_return_if_fail (clipboard != NULL);
  g_return_if_fail (CTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (callback != NULL);

  info = g_new (RequestRichTextInfo, 1);
  info->callback = callback;
  info->atoms = NULL;
  info->n_atoms = 0;
  info->current_atom = 0;
  info->user_data = user_data;

  info->atoms = ctk_text_buffer_get_deserialize_formats (buffer, &info->n_atoms);

  ctk_clipboard_request_contents (clipboard, info->atoms[info->current_atom],
				  request_rich_text_received_func,
				  info);
}

static void 
request_image_received_func (CtkClipboard     *clipboard,
			     CtkSelectionData *selection_data,
			     gpointer          data)
{
  RequestImageInfo *info = data;
  CdkPixbuf *result = NULL;

  result = ctk_selection_data_get_pixbuf (selection_data);

  if (!result)
    {
      /* If we asked for image/png and didn't get it, try image/jpeg;
       * if we asked for image/jpeg and didn't get it, try image/gif;
       * if we asked for image/gif and didn't get it, try image/bmp;
       * If we asked for anything else and didn't get it, give up.
       */
      CdkAtom target = ctk_selection_data_get_target (selection_data);

      if (target == cdk_atom_intern_static_string ("image/png"))
	{
	  ctk_clipboard_request_contents (clipboard,
					  cdk_atom_intern_static_string ("image/jpeg"), 
					  request_image_received_func, info);
	  return;
	}
      else if (target == cdk_atom_intern_static_string ("image/jpeg"))
	{
	  ctk_clipboard_request_contents (clipboard,
					  cdk_atom_intern_static_string ("image/gif"), 
					  request_image_received_func, info);
	  return;
	}
      else if (target == cdk_atom_intern_static_string ("image/gif"))
	{
	  ctk_clipboard_request_contents (clipboard,
					  cdk_atom_intern_static_string ("image/bmp"), 
					  request_image_received_func, info);
	  return;
	}
    }

  info->callback (clipboard, result, info->user_data);
  g_free (info);

  if (result)
    g_object_unref (result);
}

/**
 * ctk_clipboard_request_image:
 * @clipboard: a #CtkClipboard
 * @callback: (scope async): a function to call when the image is received,
 *     or the retrieval fails. (It will always be called one way or the other.)
 * @user_data: user data to pass to @callback.
 *
 * Requests the contents of the clipboard as image. When the image is
 * later received, it will be converted to a #CdkPixbuf, and
 * @callback will be called.
 *
 * The @pixbuf parameter to @callback will contain the resulting
 * #CdkPixbuf if the request succeeded, or %NULL if it failed. This
 * could happen for various reasons, in particular if the clipboard
 * was empty or if the contents of the clipboard could not be
 * converted into an image.
 *
 * Since: 2.6
 **/
void 
ctk_clipboard_request_image (CtkClipboard                  *clipboard,
			     CtkClipboardImageReceivedFunc  callback,
			     gpointer                       user_data)
{
  RequestImageInfo *info;
  
  g_return_if_fail (clipboard != NULL);
  g_return_if_fail (callback != NULL);
  
  info = g_new (RequestImageInfo, 1);
  info->callback = callback;
  info->user_data = user_data;

  ctk_clipboard_request_contents (clipboard, 
				  cdk_atom_intern_static_string ("image/png"),
				  request_image_received_func,
				  info);
}

static void 
request_uris_received_func (CtkClipboard     *clipboard,
			    CtkSelectionData *selection_data,
			    gpointer          data)
{
  RequestURIInfo *info = data;
  gchar **uris;

  uris = ctk_selection_data_get_uris (selection_data);
  info->callback (clipboard, uris, info->user_data);
  g_strfreev (uris);

  g_slice_free (RequestURIInfo, info);
}

/**
 * ctk_clipboard_request_uris:
 * @clipboard: a #CtkClipboard
 * @callback: (scope async): a function to call when the URIs are received,
 *     or the retrieval fails. (It will always be called one way or the other.)
 * @user_data: user data to pass to @callback.
 *
 * Requests the contents of the clipboard as URIs. When the URIs are
 * later received @callback will be called.
 *
 * The @uris parameter to @callback will contain the resulting array of
 * URIs if the request succeeded, or %NULL if it failed. This could happen
 * for various reasons, in particular if the clipboard was empty or if the
 * contents of the clipboard could not be converted into URI form.
 *
 * Since: 2.14
 **/
void 
ctk_clipboard_request_uris (CtkClipboard                *clipboard,
			    CtkClipboardURIReceivedFunc  callback,
			    gpointer                     user_data)
{
  RequestURIInfo *info;
  
  g_return_if_fail (clipboard != NULL);
  g_return_if_fail (callback != NULL);
  
  info = g_slice_new (RequestURIInfo);
  info->callback = callback;
  info->user_data = user_data;

  ctk_clipboard_request_contents (clipboard, cdk_atom_intern_static_string ("text/uri-list"),
				  request_uris_received_func,
				  info);
}

static void 
request_targets_received_func (CtkClipboard     *clipboard,
			       CtkSelectionData *selection_data,
			       gpointer          data)
{
  RequestTargetsInfo *info = data;
  CdkAtom *targets = NULL;
  gint n_targets = 0;

  ctk_selection_data_get_targets (selection_data, &targets, &n_targets);

  info->callback (clipboard, targets, n_targets, info->user_data);

  g_free (info);
  g_free (targets);
}

/**
 * ctk_clipboard_request_targets:
 * @clipboard: a #CtkClipboard
 * @callback: (scope async): a function to call when the targets are
 *     received, or the retrieval fails. (It will always be called
 *     one way or the other.)
 * @user_data: user data to pass to @callback.
 *
 * Requests the contents of the clipboard as list of supported targets.
 * When the list is later received, @callback will be called.
 *
 * The @targets parameter to @callback will contain the resulting targets if
 * the request succeeded, or %NULL if it failed.
 *
 * Since: 2.4
 **/
void 
ctk_clipboard_request_targets (CtkClipboard                *clipboard,
			       CtkClipboardTargetsReceivedFunc callback,
			       gpointer                     user_data)
{
  RequestTargetsInfo *info;
  
  g_return_if_fail (clipboard != NULL);
  g_return_if_fail (callback != NULL);

  /* If the display supports change notification we cache targets */
  if (cdk_display_supports_selection_notification (ctk_clipboard_get_display (clipboard)) &&
      clipboard->n_cached_targets != -1)
    {
      (* callback) (clipboard, clipboard->cached_targets, clipboard->n_cached_targets, user_data);
      return;
    }
  
  info = g_new (RequestTargetsInfo, 1);
  info->callback = callback;
  info->user_data = user_data;

  ctk_clipboard_request_contents (clipboard, cdk_atom_intern_static_string ("TARGETS"),
				  request_targets_received_func,
				  info);
}

typedef struct
{
  GMainLoop *loop;
  gpointer data;
  CdkAtom format; /* used by rich text */
  gsize length; /* used by rich text */
} WaitResults;

static void 
clipboard_received_func (CtkClipboard     *clipboard,
			 CtkSelectionData *selection_data,
			 gpointer          data)
{
  WaitResults *results = data;

  if (ctk_selection_data_get_length (selection_data) >= 0)
    results->data = ctk_selection_data_copy (selection_data);
  
  g_main_loop_quit (results->loop);
}

/**
 * ctk_clipboard_wait_for_contents:
 * @clipboard: a #CtkClipboard
 * @target: an atom representing the form into which the clipboard
 *          owner should convert the selection.
 * 
 * Requests the contents of the clipboard using the given target.
 * This function waits for the data to be received using the main 
 * loop, so events, timeouts, etc, may be dispatched during the wait.
 * 
 * Returns: (nullable): a newly-allocated #CtkSelectionData object or %NULL
 *               if retrieving the given target failed. If non-%NULL,
 *               this value must be freed with ctk_selection_data_free() 
 *               when you are finished with it.
 **/
CtkSelectionData *
ctk_clipboard_wait_for_contents (CtkClipboard *clipboard,
				 CdkAtom       target)
{
  WaitResults results;

  g_return_val_if_fail (clipboard != NULL, NULL);
  g_return_val_if_fail (target != CDK_NONE, NULL);
  
  results.data = NULL;
  results.loop = g_main_loop_new (NULL, TRUE);

  ctk_clipboard_request_contents (clipboard, target, 
				  clipboard_received_func,
				  &results);

  if (g_main_loop_is_running (results.loop))
    {
      cdk_threads_leave ();
      g_main_loop_run (results.loop);
      cdk_threads_enter ();
    }

  g_main_loop_unref (results.loop);

  return results.data;
}

static void 
clipboard_text_received_func (CtkClipboard *clipboard,
			      const gchar  *text,
			      gpointer      data)
{
  WaitResults *results = data;

  results->data = g_strdup (text);
  g_main_loop_quit (results->loop);
}

/**
 * ctk_clipboard_wait_for_text:
 * @clipboard: a #CtkClipboard
 * 
 * Requests the contents of the clipboard as text and converts
 * the result to UTF-8 if necessary. This function waits for
 * the data to be received using the main loop, so events,
 * timeouts, etc, may be dispatched during the wait.
 * 
 * Returns: (nullable): a newly-allocated UTF-8 string which must
 *               be freed with g_free(), or %NULL if retrieving
 *               the selection data failed. (This could happen
 *               for various reasons, in particular if the
 *               clipboard was empty or if the contents of the
 *               clipboard could not be converted into text form.)
 **/
gchar *
ctk_clipboard_wait_for_text (CtkClipboard *clipboard)
{
  WaitResults results;

  g_return_val_if_fail (clipboard != NULL, NULL);
  
  results.data = NULL;
  results.loop = g_main_loop_new (NULL, TRUE);

  ctk_clipboard_request_text (clipboard,
			      clipboard_text_received_func,
			      &results);

  if (g_main_loop_is_running (results.loop))
    {
      cdk_threads_leave ();
      g_main_loop_run (results.loop);
      cdk_threads_enter ();
    }

  g_main_loop_unref (results.loop);

  return results.data;
}

static void
clipboard_rich_text_received_func (CtkClipboard *clipboard,
                                   CdkAtom       format,
                                   const guint8 *text,
                                   gsize         length,
                                   gpointer      data)
{
  WaitResults *results = data;

  results->data = g_memdup (text, length);
  results->format = format;
  results->length = length;
  g_main_loop_quit (results->loop);
}

/**
 * ctk_clipboard_wait_for_rich_text:
 * @clipboard: a #CtkClipboard
 * @buffer: a #CtkTextBuffer
 * @format: (out): return location for the format of the returned data
 * @length: (out): return location for the length of the returned data
 *
 * Requests the contents of the clipboard as rich text.  This function
 * waits for the data to be received using the main loop, so events,
 * timeouts, etc, may be dispatched during the wait.
 *
 * Returns: (nullable) (array length=length) (transfer full): a
 *               newly-allocated binary block of data which must be
 *               freed with g_free(), or %NULL if retrieving the
 *               selection data failed. (This could happen for various
 *               reasons, in particular if the clipboard was empty or
 *               if the contents of the clipboard could not be
 *               converted into text form.)
 *
 * Since: 2.10
 **/
guint8 *
ctk_clipboard_wait_for_rich_text (CtkClipboard  *clipboard,
                                  CtkTextBuffer *buffer,
                                  CdkAtom       *format,
                                  gsize         *length)
{
  WaitResults results;

  g_return_val_if_fail (clipboard != NULL, NULL);
  g_return_val_if_fail (CTK_IS_TEXT_BUFFER (buffer), NULL);
  g_return_val_if_fail (format != NULL, NULL);
  g_return_val_if_fail (length != NULL, NULL);

  results.data = NULL;
  results.loop = g_main_loop_new (NULL, TRUE);

  ctk_clipboard_request_rich_text (clipboard, buffer,
                                   clipboard_rich_text_received_func,
                                   &results);

  if (g_main_loop_is_running (results.loop))
    {
      cdk_threads_leave ();
      g_main_loop_run (results.loop);
      cdk_threads_enter ();
    }

  g_main_loop_unref (results.loop);

  *format = results.format;
  *length = results.length;

  return results.data;
}

static void 
clipboard_image_received_func (CtkClipboard *clipboard,
			       CdkPixbuf    *pixbuf,
			       gpointer      data)
{
  WaitResults *results = data;

  if (pixbuf)
    results->data = g_object_ref (pixbuf);

  g_main_loop_quit (results->loop);
}

/**
 * ctk_clipboard_wait_for_image:
 * @clipboard: a #CtkClipboard
 *
 * Requests the contents of the clipboard as image and converts
 * the result to a #CdkPixbuf. This function waits for
 * the data to be received using the main loop, so events,
 * timeouts, etc, may be dispatched during the wait.
 *
 * Returns: (nullable) (transfer full): a newly-allocated #CdkPixbuf
 *     object which must be disposed with g_object_unref(), or
 *     %NULL if retrieving the selection data failed. (This could
 *     happen for various reasons, in particular if the clipboard
 *     was empty or if the contents of the clipboard could not be
 *     converted into an image.)
 *
 * Since: 2.6
 **/
CdkPixbuf *
ctk_clipboard_wait_for_image (CtkClipboard *clipboard)
{
  WaitResults results;

  g_return_val_if_fail (clipboard != NULL, NULL);
  
  results.data = NULL;
  results.loop = g_main_loop_new (NULL, TRUE);

  ctk_clipboard_request_image (clipboard,
			       clipboard_image_received_func,
			       &results);

  if (g_main_loop_is_running (results.loop))
    {
      cdk_threads_leave ();
      g_main_loop_run (results.loop);
      cdk_threads_enter ();
    }

  g_main_loop_unref (results.loop);

  return results.data;
}

static void 
clipboard_uris_received_func (CtkClipboard *clipboard,
			      gchar       **uris,
			      gpointer      data)
{
  WaitResults *results = data;

  results->data = g_strdupv (uris);
  g_main_loop_quit (results->loop);
}

/**
 * ctk_clipboard_wait_for_uris:
 * @clipboard: a #CtkClipboard
 * 
 * Requests the contents of the clipboard as URIs. This function waits
 * for the data to be received using the main loop, so events,
 * timeouts, etc, may be dispatched during the wait.
 *
 * Returns: (nullable) (array zero-terminated=1) (element-type utf8) (transfer full):
 *     a newly-allocated %NULL-terminated array of strings which must
 *     be freed with g_strfreev(), or %NULL if retrieving the
 *     selection data failed. (This could happen for various reasons,
 *     in particular if the clipboard was empty or if the contents of
 *     the clipboard could not be converted into URI form.)
 *
 * Since: 2.14
 **/
gchar **
ctk_clipboard_wait_for_uris (CtkClipboard *clipboard)
{
  WaitResults results;

  g_return_val_if_fail (clipboard != NULL, NULL);
  
  results.data = NULL;
  results.loop = g_main_loop_new (NULL, TRUE);

  ctk_clipboard_request_uris (clipboard,
			      clipboard_uris_received_func,
			      &results);

  if (g_main_loop_is_running (results.loop))
    {
      cdk_threads_leave ();
      g_main_loop_run (results.loop);
      cdk_threads_enter ();
    }

  g_main_loop_unref (results.loop);

  return results.data;
}

/**
 * ctk_clipboard_get_display:
 * @clipboard: a #CtkClipboard
 *
 * Gets the #CdkDisplay associated with @clipboard
 *
 * Returns: (transfer none): the #CdkDisplay associated with @clipboard
 *
 * Since: 2.2
 **/
CdkDisplay *
ctk_clipboard_get_display (CtkClipboard *clipboard)
{
  g_return_val_if_fail (clipboard != NULL, NULL);

  return clipboard->display;
}

/**
 * ctk_clipboard_wait_is_text_available:
 * @clipboard: a #CtkClipboard
 * 
 * Test to see if there is text available to be pasted
 * This is done by requesting the TARGETS atom and checking
 * if it contains any of the supported text targets. This function 
 * waits for the data to be received using the main loop, so events, 
 * timeouts, etc, may be dispatched during the wait.
 *
 * This function is a little faster than calling
 * ctk_clipboard_wait_for_text() since it doesn’t need to retrieve
 * the actual text.
 * 
 * Returns: %TRUE is there is text available, %FALSE otherwise.
 **/
gboolean
ctk_clipboard_wait_is_text_available (CtkClipboard *clipboard)
{
  CtkSelectionData *data;
  gboolean result = FALSE;

  data = ctk_clipboard_wait_for_contents (clipboard, cdk_atom_intern_static_string ("TARGETS"));
  if (data)
    {
      result = ctk_selection_data_targets_include_text (data);
      ctk_selection_data_free (data);
    }

  return result;
}

/**
 * ctk_clipboard_wait_is_rich_text_available:
 * @clipboard: a #CtkClipboard
 * @buffer: a #CtkTextBuffer
 *
 * Test to see if there is rich text available to be pasted
 * This is done by requesting the TARGETS atom and checking
 * if it contains any of the supported rich text targets. This function
 * waits for the data to be received using the main loop, so events,
 * timeouts, etc, may be dispatched during the wait.
 *
 * This function is a little faster than calling
 * ctk_clipboard_wait_for_rich_text() since it doesn’t need to retrieve
 * the actual text.
 *
 * Returns: %TRUE is there is rich text available, %FALSE otherwise.
 *
 * Since: 2.10
 **/
gboolean
ctk_clipboard_wait_is_rich_text_available (CtkClipboard  *clipboard,
                                           CtkTextBuffer *buffer)
{
  CtkSelectionData *data;
  gboolean result = FALSE;

  g_return_val_if_fail (CTK_IS_CLIPBOARD (clipboard), FALSE);
  g_return_val_if_fail (CTK_IS_TEXT_BUFFER (buffer), FALSE);

  data = ctk_clipboard_wait_for_contents (clipboard, cdk_atom_intern_static_string ("TARGETS"));
  if (data)
    {
      result = ctk_selection_data_targets_include_rich_text (data, buffer);
      ctk_selection_data_free (data);
    }

  return result;
}

/**
 * ctk_clipboard_wait_is_image_available:
 * @clipboard: a #CtkClipboard
 * 
 * Test to see if there is an image available to be pasted
 * This is done by requesting the TARGETS atom and checking
 * if it contains any of the supported image targets. This function 
 * waits for the data to be received using the main loop, so events, 
 * timeouts, etc, may be dispatched during the wait.
 *
 * This function is a little faster than calling
 * ctk_clipboard_wait_for_image() since it doesn’t need to retrieve
 * the actual image data.
 * 
 * Returns: %TRUE is there is an image available, %FALSE otherwise.
 *
 * Since: 2.6
 **/
gboolean
ctk_clipboard_wait_is_image_available (CtkClipboard *clipboard)
{
  CtkSelectionData *data;
  gboolean result = FALSE;

  data = ctk_clipboard_wait_for_contents (clipboard, 
					  cdk_atom_intern_static_string ("TARGETS"));
  if (data)
    {
      result = ctk_selection_data_targets_include_image (data, FALSE);
      ctk_selection_data_free (data);
    }

  return result;
}

/**
 * ctk_clipboard_wait_is_uris_available:
 * @clipboard: a #CtkClipboard
 * 
 * Test to see if there is a list of URIs available to be pasted
 * This is done by requesting the TARGETS atom and checking
 * if it contains the URI targets. This function
 * waits for the data to be received using the main loop, so events, 
 * timeouts, etc, may be dispatched during the wait.
 *
 * This function is a little faster than calling
 * ctk_clipboard_wait_for_uris() since it doesn’t need to retrieve
 * the actual URI data.
 * 
 * Returns: %TRUE is there is an URI list available, %FALSE otherwise.
 *
 * Since: 2.14
 **/
gboolean
ctk_clipboard_wait_is_uris_available (CtkClipboard *clipboard)
{
  CtkSelectionData *data;
  gboolean result = FALSE;

  data = ctk_clipboard_wait_for_contents (clipboard, 
					  cdk_atom_intern_static_string ("TARGETS"));
  if (data)
    {
      result = ctk_selection_data_targets_include_uri (data);
      ctk_selection_data_free (data);
    }

  return result;
}

/**
 * ctk_clipboard_wait_for_targets:
 * @clipboard: a #CtkClipboard
 * @targets: (out) (array length=n_targets) (transfer container): location
 *           to store an array of targets. The result stored here must
 *           be freed with g_free().
 * @n_targets: (out): location to store number of items in @targets.
 *
 * Returns a list of targets that are present on the clipboard, or %NULL
 * if there aren’t any targets available. The returned list must be
 * freed with g_free().
 * This function waits for the data to be received using the main
 * loop, so events, timeouts, etc, may be dispatched during the wait.
 *
 * Returns: %TRUE if any targets are present on the clipboard,
 *               otherwise %FALSE.
 *
 * Since: 2.4
 */
gboolean
ctk_clipboard_wait_for_targets (CtkClipboard  *clipboard, 
				CdkAtom      **targets,
				gint          *n_targets)
{
  CtkSelectionData *data;
  gboolean result = FALSE;
  
  g_return_val_if_fail (clipboard != NULL, FALSE);

  /* If the display supports change notification we cache targets */
  if (cdk_display_supports_selection_notification (ctk_clipboard_get_display (clipboard)) &&
      clipboard->n_cached_targets != -1)
    {
      if (n_targets)
 	*n_targets = clipboard->n_cached_targets;
 
      if (targets)
 	*targets = g_memdup (clipboard->cached_targets,
 			     clipboard->n_cached_targets * sizeof (CdkAtom));

       return TRUE;
    }
  
  if (n_targets)
    *n_targets = 0;
      
  if (targets)
    *targets = NULL;      

  data = ctk_clipboard_wait_for_contents (clipboard, cdk_atom_intern_static_string ("TARGETS"));

  if (data)
    {
      CdkAtom *tmp_targets;
      gint tmp_n_targets;
       
      result = ctk_selection_data_get_targets (data, &tmp_targets, &tmp_n_targets);
 
      if (cdk_display_supports_selection_notification (ctk_clipboard_get_display (clipboard)))
 	{
 	  clipboard->n_cached_targets = tmp_n_targets;
 	  clipboard->cached_targets = g_memdup (tmp_targets,
 						tmp_n_targets * sizeof (CdkAtom));
 	}
 
      if (n_targets)
 	*n_targets = tmp_n_targets;
 
      if (targets)
 	*targets = tmp_targets;
      else
 	g_free (tmp_targets);
      
      ctk_selection_data_free (data);
    }

  return result;
}

static CtkClipboard *
clipboard_peek (CdkDisplay *display, 
		CdkAtom     selection,
		gboolean    only_if_exists)
{
  CtkClipboard *clipboard = NULL;
  GSList *clipboards;
  GSList *tmp_list;

  if (selection == CDK_NONE)
    selection = CDK_SELECTION_CLIPBOARD;

  clipboards = g_object_get_data (G_OBJECT (display), "ctk-clipboard-list");

  tmp_list = clipboards;
  while (tmp_list)
    {
      clipboard = tmp_list->data;
      if (clipboard->selection == selection)
	break;

      tmp_list = tmp_list->next;
    }

  if (!tmp_list && !only_if_exists)
    {
      clipboard = g_object_new (CTK_TYPE_CLIPBOARD, NULL);

      clipboard->selection = selection;
      clipboard->display = display;
      clipboard->n_cached_targets = -1;
      clipboard->n_storable_targets = -1;
      clipboards = g_slist_prepend (clipboards, clipboard);
      g_object_set_data (G_OBJECT (display), I_("ctk-clipboard-list"), clipboards);
      g_signal_connect (display, "closed",
			G_CALLBACK (clipboard_display_closed), clipboard);
      cdk_display_request_selection_notification (display, selection);
    }
  
  return clipboard;
}

static void
ctk_clipboard_owner_change (CtkClipboard        *clipboard,
			    CdkEventOwnerChange *event)
{
  if (clipboard->n_cached_targets != -1)
    {
      g_free (clipboard->cached_targets);
      clipboard->cached_targets = NULL;
      clipboard->n_cached_targets = -1;
    }
}

/**
 * ctk_clipboard_wait_is_target_available:
 * @clipboard: a #CtkClipboard
 * @target:    A #CdkAtom indicating which target to look for.
 *
 * Checks if a clipboard supports pasting data of a given type. This
 * function can be used to determine if a “Paste” menu item should be
 * insensitive or not.
 *
 * If you want to see if there’s text available on the clipboard, use
 * ctk_clipboard_wait_is_text_available () instead.
 *
 * Returns: %TRUE if the target is available, %FALSE otherwise.
 *
 * Since: 2.6
 */
gboolean
ctk_clipboard_wait_is_target_available (CtkClipboard *clipboard,
					CdkAtom       target)
{
  CdkAtom *targets;
  gint i, n_targets;
  gboolean retval = FALSE;
    
  if (!ctk_clipboard_wait_for_targets (clipboard, &targets, &n_targets))
    return FALSE;

  for (i = 0; i < n_targets; i++)
    {
      if (targets[i] == target)
	{
	  retval = TRUE;
	  break;
	}
    }

  g_free (targets);
  
  return retval;
}

/**
 * _ctk_clipboard_handle_event:
 * @event: a owner change event
 * 
 * Emits the #CtkClipboard::owner-change signal on the appropriate @clipboard.
 *
 * Since: 2.6
 **/
void 
_ctk_clipboard_handle_event (CdkEventOwnerChange *event)
{
  CdkDisplay *display;
  CtkClipboard *clipboard;
  
  display = cdk_window_get_display (event->window);
  clipboard = clipboard_peek (display, event->selection, TRUE);
      
  if (clipboard)
    g_signal_emit (clipboard, 
		   clipboard_signals[OWNER_CHANGE], 0, event, NULL);
}

static gboolean
ctk_clipboard_store_timeout (CtkClipboard *clipboard)
{
  g_main_loop_quit (clipboard->store_loop);
  
  return G_SOURCE_REMOVE;
}

/**
 * ctk_clipboard_set_can_store:
 * @clipboard: a #CtkClipboard
 * @targets: (allow-none) (array length=n_targets): array containing
 *           information about which forms should be stored or %NULL
 *           to indicate that all forms should be stored.
 * @n_targets: number of elements in @targets
 *
 * Hints that the clipboard data should be stored somewhere when the
 * application exits or when ctk_clipboard_store () is called.
 *
 * This value is reset when the clipboard owner changes.
 * Where the clipboard data is stored is platform dependent,
 * see cdk_display_store_clipboard () for more information.
 * 
 * Since: 2.6
 */
void
ctk_clipboard_set_can_store (CtkClipboard         *clipboard,
 			     const CtkTargetEntry *targets,
 			     gint                  n_targets)
{
  g_return_if_fail (CTK_IS_CLIPBOARD (clipboard));
  g_return_if_fail (n_targets >= 0);

  CTK_CLIPBOARD_GET_CLASS (clipboard)->set_can_store (clipboard,
                                                      targets,
                                                      n_targets);
}

static void
ctk_clipboard_real_set_can_store (CtkClipboard         *clipboard,
                                  const CtkTargetEntry *targets,
                                  gint                  n_targets)
{
  CtkWidget *clipboard_widget;
  int i;
  static const CtkTargetEntry save_targets[] = {
    { "SAVE_TARGETS", 0, TARGET_SAVE_TARGETS }
  };
  
  if (clipboard->selection != CDK_SELECTION_CLIPBOARD)
    return;
  
  g_free (clipboard->storable_targets);
  
  clipboard_widget = get_clipboard_widget (clipboard->display);

  /* n_storable_targets being -1 means that
   * ctk_clipboard_set_can_store hasn't been called since the
   * clipboard owner changed. We only want to add SAVE_TARGETS and 
   * ref the owner once , so we do that here
   */  
  if (clipboard->n_storable_targets == -1)
    {
      ctk_selection_add_targets (clipboard_widget, clipboard->selection,
				 save_targets, 1);

      /* Ref the owner so it won't go away */
      if (clipboard->have_owner)
	g_object_ref (clipboard->user_data);
    }
  
  clipboard->n_storable_targets = n_targets;
  clipboard->storable_targets = g_new (CdkAtom, n_targets);
  for (i = 0; i < n_targets; i++)
    clipboard->storable_targets[i] = cdk_atom_intern (targets[i].target, FALSE);
}

static gboolean
ctk_clipboard_selection_notify (CtkWidget         *widget,
				CdkEventSelection *event,
				CtkClipboard      *clipboard)
{
  if (event->selection == cdk_atom_intern_static_string ("CLIPBOARD_MANAGER") &&
      clipboard->storing_selection)
    g_main_loop_quit (clipboard->store_loop);

  return FALSE;
}

/**
 * ctk_clipboard_store:
 * @clipboard: a #CtkClipboard
 *
 * Stores the current clipboard data somewhere so that it will stay
 * around after the application has quit.
 *
 * Since: 2.6
 */
void
ctk_clipboard_store (CtkClipboard *clipboard)
{
  g_return_if_fail (CTK_IS_CLIPBOARD (clipboard));

  CTK_CLIPBOARD_GET_CLASS (clipboard)->store (clipboard);
}

static void
ctk_clipboard_real_store (CtkClipboard *clipboard)
{
  CtkWidget *clipboard_widget;

  if (clipboard->n_storable_targets < 0)
    return;
  
  if (!cdk_display_supports_clipboard_persistence (clipboard->display))
    return;

  g_object_ref (clipboard);

  clipboard_widget = get_clipboard_widget (clipboard->display);
  clipboard->notify_signal_id = g_signal_connect (clipboard_widget,
						  "selection-notify-event",
						  G_CALLBACK (ctk_clipboard_selection_notify),
						  clipboard);

  cdk_display_store_clipboard (clipboard->display,
                               ctk_widget_get_window (clipboard_widget),
			       clipboard_get_timestamp (clipboard),
			       clipboard->storable_targets,
			       clipboard->n_storable_targets);

  clipboard->storing_selection = TRUE;

  clipboard->store_loop = g_main_loop_new (NULL, TRUE);
  clipboard->store_timeout = g_timeout_add_seconds (10, (GSourceFunc) ctk_clipboard_store_timeout, clipboard);
  g_source_set_name_by_id (clipboard->store_timeout, "[ctk+] ctk_clipboard_store_timeout");

  if (g_main_loop_is_running (clipboard->store_loop))
    {
      cdk_threads_leave ();
      g_main_loop_run (clipboard->store_loop);
      cdk_threads_enter ();
    }
  
  g_main_loop_unref (clipboard->store_loop);
  clipboard->store_loop = NULL;
  
  g_source_remove (clipboard->store_timeout);
  clipboard->store_timeout = 0;
  g_signal_handler_disconnect (clipboard_widget, clipboard->notify_signal_id);
  clipboard->notify_signal_id = 0;
  
  clipboard->storing_selection = FALSE;

  g_object_unref (clipboard);
}

/* Stores all clipboard selections on all displays, called from
 * ctk_main_quit ().
 */
void
_ctk_clipboard_store_all (void)
{
  CtkClipboard *clipboard;
  GSList *displays, *list;
  
  displays = cdk_display_manager_list_displays (cdk_display_manager_get ());

  list = displays;
  while (list)
    {
      CdkDisplay *display = list->data;

      clipboard = clipboard_peek (display, CDK_SELECTION_CLIPBOARD, TRUE);

      if (clipboard)
	ctk_clipboard_store (clipboard);
      
      list = list->next;
    }
  g_slist_free (displays);
  
}

/**
 * ctk_clipboard_get_selection:
 * @clipboard: a #CtkClipboard
 *
 * Gets the selection that this clipboard is for.
 *
 * Returns: the selection
 *
 * Since: 3.22
 */
CdkAtom
ctk_clipboard_get_selection (CtkClipboard *clipboard)
{
  g_return_val_if_fail (CTK_IS_CLIPBOARD (clipboard), CDK_NONE);

  return clipboard->selection;
}
