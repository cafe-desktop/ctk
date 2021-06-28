/* CTK - The GIMP Toolkit
 * Copyright (C) 2000 Red Hat, Inc.
 * Copyright (C) 2004 Nokia Corporation
 * Copyright (C) 2006-2008 Imendio AB
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
 */

#include "config.h"
#include <string.h>

#import <Cocoa/Cocoa.h>

#include "ctkclipboard.h"
#include "ctkinvisible.h"
#include "ctkmain.h"
#include "ctkmarshalers.h"
#include "ctkintl.h"
#include "ctktextbuffer.h"
#include "ctkselectionprivate.h"
#include "ctkquartz.h"
#include "quartz/cdkquartz-ctk-only.h"

enum {
  OWNER_CHANGE,
  LAST_SIGNAL
};

@interface CtkClipboardOwner : NSObject {
  CtkClipboard *clipboard;
  @public
  gboolean setting_same_owner;
}

@end

typedef struct _CtkClipboardClass CtkClipboardClass;

struct _CtkClipboard
{
  GObject parent_instance;

  NSPasteboard *pasteboard;
  CtkClipboardOwner *owner;
  NSInteger change_count;

  CdkAtom selection;

  CtkClipboardGetFunc get_func;
  CtkClipboardClearFunc clear_func;
  gpointer user_data;
  gboolean have_owner;
  CtkTargetList *target_list;

  gboolean have_selection;
  CdkDisplay *display;

  CdkAtom *cached_targets;
  gint     n_cached_targets;

  guint      notify_signal_id;
  gboolean   storing_selection;
  GMainLoop *store_loop;
  guint      store_timeout;
  gint       n_storable_targets;
  CdkAtom   *storable_targets;
};

struct _CtkClipboardClass
{
  GObjectClass parent_class;

  void (*owner_change) (CtkClipboard        *clipboard,
			CdkEventOwnerChange *event);
};

static void ctk_clipboard_class_init   (CtkClipboardClass   *class);
static void ctk_clipboard_finalize     (GObject             *object);
static void ctk_clipboard_owner_change (CtkClipboard        *clipboard,
					CdkEventOwnerChange *event);

static void          clipboard_unset      (CtkClipboard     *clipboard);
static CtkClipboard *clipboard_peek       (CdkDisplay       *display,
					   CdkAtom           selection,
					   gboolean          only_if_exists);

@implementation CtkClipboardOwner
-(void)pasteboard:(NSPasteboard *)sender provideDataForType:(NSString *)type
{
  CtkSelectionData selection_data;
  guint info;

  if (!clipboard->target_list)
    return;

  memset (&selection_data, 0, sizeof (CtkSelectionData));

  selection_data.selection = clipboard->selection;
  selection_data.target = cdk_quartz_pasteboard_type_to_atom_libctk_only (type);
  selection_data.display = cdk_display_get_default ();
  selection_data.length = -1;

  if (ctk_target_list_find (clipboard->target_list, selection_data.target, &info))
    {
      clipboard->get_func (clipboard, &selection_data,
                           info,
                           clipboard->user_data);

      if (selection_data.length >= 0)
        _ctk_quartz_set_selection_data_for_pasteboard (clipboard->pasteboard,
                                                       &selection_data);

      g_free (selection_data.data);
    }
}

/*  pasteboardChangedOwner is not called immediately, and it's not called
 *  reliably. It is somehow documented in the apple api docs, but the docs
 *  suck and don’t really give clear instructions. Therefore we track
 *  changeCount in several places below and clear the clipboard if it
 *  changed.
 */
- (void)pasteboardChangedOwner:(NSPasteboard *)sender
{
  if (! setting_same_owner)
    clipboard_unset (clipboard);
}

- (id)initWithClipboard:(CtkClipboard *)aClipboard
{
  self = [super init];

  if (self)
    {
      clipboard = aClipboard;
      setting_same_owner = FALSE;
    }

  return self;
}

@end


static const gchar clipboards_owned_key[] = "ctk-clipboards-owned";
static GQuark clipboards_owned_key_id = 0;

static GObjectClass *parent_class;
static guint         clipboard_signals[LAST_SIGNAL] = { 0 };

GType
ctk_clipboard_get_type (void)
{
  static GType clipboard_type = 0;

  if (!clipboard_type)
    {
      const GTypeInfo clipboard_info =
      {
	sizeof (CtkClipboardClass),
	NULL,           /* base_init */
	NULL,           /* base_finalize */
	(GClassInitFunc) ctk_clipboard_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data */
	sizeof (CtkClipboard),
	0,              /* n_preallocs */
	(GInstanceInitFunc) NULL,
      };

      clipboard_type = g_type_register_static (G_TYPE_OBJECT, I_("CtkClipboard"),
					       &clipboard_info, 0);
    }

  return clipboard_type;
}

static void
ctk_clipboard_class_init (CtkClipboardClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

  parent_class = g_type_class_peek_parent (class);

  gobject_class->finalize = ctk_clipboard_finalize;

  class->owner_change = ctk_clipboard_owner_change;

  /**
   * CtkClipboard::owner-change:
   * @clipboard:
   * @event: (type CdkEventOwnerChange):
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
  GSList *clipboards;

  clipboard = CTK_CLIPBOARD (object);

  clipboards = g_object_get_data (G_OBJECT (clipboard->display), "ctk-clipboard-list");
  if (g_slist_index (clipboards, clipboard) >= 0)
    g_warning ("CtkClipboard prematurely finalized");

  clipboard_unset (clipboard);

  clipboards = g_object_get_data (G_OBJECT (clipboard->display), "ctk-clipboard-list");
  clipboards = g_slist_remove (clipboards, clipboard);
  g_object_set_data (G_OBJECT (clipboard->display), I_("ctk-clipboard-list"), clipboards);

  if (clipboard->store_loop && g_main_loop_is_running (clipboard->store_loop))
    g_main_loop_quit (clipboard->store_loop);

  if (clipboard->store_timeout != 0)
    g_source_remove (clipboard->store_timeout);

  g_free (clipboard->storable_targets);

  G_OBJECT_CLASS (parent_class)->finalize (object);
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
 * @display:
 * @selection:
 *
 * Returns: (transfer none):
 */
CtkClipboard *
ctk_clipboard_get_for_display (CdkDisplay *display,
			       CdkAtom     selection)
{
  g_return_val_if_fail (CDK_IS_DISPLAY (display), NULL);
  g_return_val_if_fail (!cdk_display_is_closed (display), NULL);

  return clipboard_peek (display, selection, FALSE);
}

/**
 * ctk_clipboard_get:
 * @selection:
 *
 * Returns: (transfer none):
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

      if (clipboard->target_list)
        {
          ctk_target_list_unref (clipboard->target_list);
          clipboard->target_list = NULL;
        }

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
  NSSet *types;
  NSAutoreleasePool *pool;

  if (!(clipboard->have_owner && have_owner) ||
      clipboard->user_data != user_data)
    {
      clipboard_unset (clipboard);

      if (clipboard->get_func)
        {
          /* Calling unset() caused the clipboard contents to be reset!
           * Avoid leaking and return
           */
          if (!(clipboard->have_owner && have_owner) ||
              clipboard->user_data != user_data)
            {
              (*clear_func) (clipboard, user_data);
              return FALSE;
            }
          else
            {
              return TRUE;
            }
        }
    }

  pool = [[NSAutoreleasePool alloc] init];

  types = _ctk_quartz_target_entries_to_pasteboard_types (targets, n_targets);

  /*  call declareTypes before setting the clipboard members because
   *  declareTypes might clear the clipboard
   */
  if (user_data && user_data == clipboard->user_data)
    {
      clipboard->owner->setting_same_owner = TRUE;
      clipboard->change_count = [clipboard->pasteboard declareTypes: [types allObjects]
                                                              owner: clipboard->owner];
      clipboard->owner->setting_same_owner = FALSE;
    }
  else
    {
      CtkClipboardOwner *new_owner;

      /* We do not set the new owner on clipboard->owner immediately,
       * because declareTypes could (but not always does -- see also the
       * comment at pasteboardChangedOwner above) cause clipboard_unset
       * to be called, which releases clipboard->owner.
       */
      new_owner = [[CtkClipboardOwner alloc] initWithClipboard:clipboard];
      clipboard->change_count = [clipboard->pasteboard declareTypes: [types allObjects]
                                                              owner: new_owner];

      /* In case pasteboardChangedOwner was not triggered, check to see
       * whether the previous owner still needs to be released.
       */
      if (clipboard->owner)
        [clipboard->owner release];
      clipboard->owner = new_owner;
    }

  [types release];
  [pool release];

  clipboard->user_data = user_data;
  clipboard->have_owner = have_owner;
  if (have_owner)
    clipboard_add_owner_notify (clipboard);
  clipboard->get_func = get_func;
  clipboard->clear_func = clear_func;

  if (clipboard->target_list)
    ctk_target_list_unref (clipboard->target_list);
  clipboard->target_list = ctk_target_list_new (targets, n_targets);

  return TRUE;
}

/**
 * ctk_clipboard_set_with_data: (skip)
 * @clipboard:
 * @targets: (array length=n_targets):
 * @n_targets:
 * @get_func: (scope async):
 * @clear_func: (scope async):
 * @user_data:
 */
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

  return ctk_clipboard_set_contents (clipboard, targets, n_targets,
				     get_func, clear_func, user_data,
				     FALSE);
}

/**
 * ctk_clipboard_set_with_owner: (skip)
 * @clipboard:
 * @targets: (array length=n_targets):
 * @get_func: (scope async):
 * @clear_func: (scope async):
 * @owner:
 */
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

  return ctk_clipboard_set_contents (clipboard, targets, n_targets,
				     get_func, clear_func, owner,
				     TRUE);
}

/**
 * ctk_clipboard_get_owner:
 * @clipboard:
 *
 * Returns: (transfer none):
 */
GObject *
ctk_clipboard_get_owner (CtkClipboard *clipboard)
{
  g_return_val_if_fail (clipboard != NULL, NULL);

  if (clipboard->change_count < [clipboard->pasteboard changeCount])
    {
      clipboard_unset (clipboard);
      clipboard->change_count = [clipboard->pasteboard changeCount];
    }

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

  if (clipboard->owner)
    [clipboard->owner release];
  clipboard->owner = NULL;
  clipboard->get_func = NULL;
  clipboard->clear_func = NULL;
  clipboard->user_data = NULL;

  if (old_clear_func)
    old_clear_func (clipboard, old_data);

  if (clipboard->target_list)
    {
      ctk_target_list_unref (clipboard->target_list);
      clipboard->target_list = NULL;
    }

  /* If we've transferred the clipboard data to the manager,
   * unref the owner
   */
  if (old_have_owner &&
      old_n_storable_targets != -1)
    g_object_unref (old_data);
}

void
ctk_clipboard_clear (CtkClipboard *clipboard)
{
  clipboard_unset (clipboard);
#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1060
  if (cdk_quartz_osx_version() >= CDK_OSX_SNOW_LEOPARD)
    [clipboard->pasteboard clearContents];
#if MAC_OS_X_VERSION_MIN_REQUIRED < 1060
  else
#endif
#endif
#if MAC_OS_X_VERSION_MIN_REQUIRED < 1060
    [clipboard->pasteboard declareTypes:nil owner:nil];
#endif
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

void
ctk_clipboard_set_text (CtkClipboard *clipboard,
			const gchar  *text,
			gint          len)
{
  CtkTargetEntry target = { "UTF8_STRING", 0, 0 };

  g_return_if_fail (clipboard != NULL);
  g_return_if_fail (text != NULL);

  if (len < 0)
    len = strlen (text);

  ctk_clipboard_set_with_data (clipboard,
			       &target, 1,
			       text_get_func, text_clear_func,
			       g_strndup (text, len));
  ctk_clipboard_set_can_store (clipboard, NULL, 0);
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

void
ctk_clipboard_set_image (CtkClipboard *clipboard,
			 GdkPixbuf    *pixbuf)
{
  CtkTargetList *list;
  GList *l;
  CtkTargetEntry *targets;
  gint n_targets, i;

  g_return_if_fail (clipboard != NULL);
  g_return_if_fail (GDK_IS_PIXBUF (pixbuf));

  list = ctk_target_list_new (NULL, 0);
  ctk_target_list_add_image_targets (list, 0, TRUE);

  n_targets = g_list_length (list->list);
  targets = g_new0 (CtkTargetEntry, n_targets);
  for (l = list->list, i = 0; l; l = l->next, i++)
    {
      CtkTargetPair *pair = (CtkTargetPair *)l->data;
      targets[i].target = cdk_atom_name (pair->target);
    }

  ctk_clipboard_set_with_data (clipboard,
			       targets, n_targets,
			       pixbuf_get_func, pixbuf_clear_func,
			       g_object_ref (pixbuf));
  ctk_clipboard_set_can_store (clipboard, NULL, 0);

  for (i = 0; i < n_targets; i++)
    g_free (targets[i].target);
  g_free (targets);
  ctk_target_list_unref (list);
}

/**
 * ctk_clipboard_request_contents:
 * @clipboard:
 * @target:
 * @callback: (scope async):
 * @user_data:
 */
void
ctk_clipboard_request_contents (CtkClipboard            *clipboard,
				CdkAtom                  target,
				CtkClipboardReceivedFunc callback,
				gpointer                 user_data)
{
  CtkSelectionData *data;

  data = ctk_clipboard_wait_for_contents (clipboard, target);

  callback (clipboard, data, user_data);

  ctk_selection_data_free (data);
}

/**
 * ctk_clipboard_request_text:
 * @clipboard:
 * @callback: (scope async):
 * @user_data:
 */
void
ctk_clipboard_request_text (CtkClipboard                *clipboard,
			    CtkClipboardTextReceivedFunc callback,
			    gpointer                     user_data)
{
  gchar *data = ctk_clipboard_wait_for_text (clipboard);

  callback (clipboard, data, user_data);

  g_free (data);
}

/**
 * ctk_clipboard_request_rich_text:
 * @clipboard:
 * @buffer:
 * @callback: (scope async):
 * @user_data:
 */
void
ctk_clipboard_request_rich_text (CtkClipboard                    *clipboard,
                                 CtkTextBuffer                   *buffer,
                                 CtkClipboardRichTextReceivedFunc callback,
                                 gpointer                         user_data)
{
  /* FIXME: Implement */
}


/**
 * ctk_clipboard_wait_for_rich_text:
 * @clipboard:
 * @buffer:
 * @format: (out):
 * @length: (out):
 *
 * Returns: (nullable):
 */
guint8 *
ctk_clipboard_wait_for_rich_text (CtkClipboard  *clipboard,
                                  CtkTextBuffer *buffer,
                                  CdkAtom       *format,
                                  gsize         *length)
{
  /* FIXME: Implement */
  return NULL;
}

/**
 * ctk_clipboard_request_image:
 * @clipboard:
 * @callback: (scope async):
 * @user_data:
 */
void
ctk_clipboard_request_image (CtkClipboard                  *clipboard,
			     CtkClipboardImageReceivedFunc  callback,
			     gpointer                       user_data)
{
  GdkPixbuf *pixbuf = ctk_clipboard_wait_for_image (clipboard);

  callback (clipboard, pixbuf, user_data);

  if (pixbuf)
    g_object_unref (pixbuf);
}

/**
 * ctk_clipboard_request_uris:
 * @clipboard:
 * @callback: (scope async):
 * @user_data:
 */
void
ctk_clipboard_request_uris (CtkClipboard                *clipboard,
			    CtkClipboardURIReceivedFunc  callback,
			    gpointer                     user_data)
{
  gchar **uris = ctk_clipboard_wait_for_uris (clipboard);

  callback (clipboard, uris, user_data);

  g_strfreev (uris);
}

/**
 * ctk_clipboard_request_targets:
 * @clipboard:
 * @callback: (scope async):
 * @user_data:
 */
void
ctk_clipboard_request_targets (CtkClipboard                *clipboard,
			       CtkClipboardTargetsReceivedFunc callback,
			       gpointer                     user_data)
{
  CdkAtom *targets;
  gint n_targets;

  ctk_clipboard_wait_for_targets (clipboard, &targets, &n_targets);

  callback (clipboard, targets, n_targets, user_data);
}

/**
 * ctk_clipboard_wait_for_contents:
 * @clipboard:
 * @target:
 *
 * Returns: (nullable):
 */
CtkSelectionData *
ctk_clipboard_wait_for_contents (CtkClipboard *clipboard,
				 CdkAtom       target)
{
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  CtkSelectionData *selection_data = NULL;

  if (clipboard->change_count < [clipboard->pasteboard changeCount])
    {
      clipboard_unset (clipboard);
      clipboard->change_count = [clipboard->pasteboard changeCount];
    }

  if (target == cdk_atom_intern_static_string ("TARGETS"))
    {
      NSArray *types = [clipboard->pasteboard types];
      int i, length;
      GList *atom_list, *l;
      CdkAtom *atoms;

      length = [types count] * sizeof (CdkAtom);

      selection_data = g_slice_new0 (CtkSelectionData);
      selection_data->selection = clipboard->selection;
      selection_data->target = target;
      if (!selection_data->display)
	selection_data->display = cdk_display_get_default ();

      atoms = g_malloc (length);

      atom_list = _ctk_quartz_pasteboard_types_to_atom_list (types);
      for (l = atom_list, i = 0; l ; l = l->next, i++)
	atoms[i] = CDK_POINTER_TO_ATOM (l->data);
      g_list_free (atom_list);

      ctk_selection_data_set (selection_data,
                              CDK_SELECTION_TYPE_ATOM, 32,
                              (guchar *)atoms, length);

      [pool release];

      return selection_data;
    }

  selection_data = _ctk_quartz_get_selection_data_from_pasteboard (clipboard->pasteboard,
								   target,
								   clipboard->selection);

  [pool release];

  return selection_data;
}

/**
 * ctk_clipboard_wait_for_text:
 * @clipboard:
 *
 * Returns: (nullable):
 */
gchar *
ctk_clipboard_wait_for_text (CtkClipboard *clipboard)
{
  CtkSelectionData *data;
  gchar *result;

  data = ctk_clipboard_wait_for_contents (clipboard,
					  cdk_atom_intern_static_string ("UTF8_STRING"));

  result = (gchar *)ctk_selection_data_get_text (data);

  ctk_selection_data_free (data);

  return result;
}

/**
 * ctk_clipboard_wait_for_image:
 * @clipboard:
 *
 * Returns: (nullable) (transfer full):
 */
GdkPixbuf *
ctk_clipboard_wait_for_image (CtkClipboard *clipboard)
{
  CdkAtom target = cdk_atom_intern_static_string("image/tiff");
  CtkSelectionData *data;

  data = ctk_clipboard_wait_for_contents (clipboard, target);

  if (data && data->data)
    {
      GdkPixbuf *pixbuf = ctk_selection_data_get_pixbuf (data);
      ctk_selection_data_free (data);
      return pixbuf;
    }

  return NULL;
}

/**
 * ctk_clipboard_wait_for_uris:
 * @clipboard:
 *
 * Returns: (nullable) (array zero-terminated=1) (transfer full) (element-type utf8):
 */
gchar **
ctk_clipboard_wait_for_uris (CtkClipboard *clipboard)
{
  CtkSelectionData *data;

  data = ctk_clipboard_wait_for_contents (clipboard, cdk_atom_intern_static_string ("text/uri-list"));
  if (data)
    {
      gchar **uris;

      uris = ctk_selection_data_get_uris (data);
      ctk_selection_data_free (data);

      return uris;
    }

  return NULL;
}

/**
 * ctk_clipboard_get_display:
 * @clipboard:
 *
 * Returns: (transfer none):
 */
CdkDisplay *
ctk_clipboard_get_display (CtkClipboard *clipboard)
{
  g_return_val_if_fail (clipboard != NULL, NULL);

  return clipboard->display;
}

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
      NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
      NSString *pasteboard_name;
      clipboard = g_object_new (CTK_TYPE_CLIPBOARD, NULL);

      if (selection == CDK_SELECTION_CLIPBOARD)
	pasteboard_name = NSGeneralPboard;
      else
	{
	  char *atom_string = cdk_atom_name (selection);

	  pasteboard_name = [NSString stringWithFormat:@"_CTK_%@",
			     [NSString stringWithUTF8String:atom_string]];
	  g_free (atom_string);
	}

      clipboard->pasteboard = [NSPasteboard pasteboardWithName:pasteboard_name];

      [pool release];

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
      clipboard->n_cached_targets = -1;
      g_free (clipboard->cached_targets);
    }
}

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

void
_ctk_clipboard_handle_event (CdkEventOwnerChange *event)
{
}

/**
 * ctk_clipboard_set_can_store:
 * @clipboard:
 * @targets: (allow-none) (array length=n_targets):
 * @n_targets:
 */
void
ctk_clipboard_set_can_store (CtkClipboard         *clipboard,
 			     const CtkTargetEntry *targets,
 			     gint                  n_targets)
{
  /* FIXME: Implement */
}

void
ctk_clipboard_store (CtkClipboard *clipboard)
{
  int i;
  int n_targets = 0;
  CtkTargetEntry *targets;

  g_return_if_fail (CTK_IS_CLIPBOARD (clipboard));

  if (!clipboard->target_list || !clipboard->get_func)
    return;

  /* We simply store all targets into the OS X clipboard. We should be
   * using the functions cdk_display_supports_clipboard_persistence() and
   * cdk_display_store_clipboard(), but since for OS X the clipboard support
   * was implemented in CTK+ and not through CdkSelections, we do it this
   * way. Doing this properly could be worthwhile to implement in the future.
   */

  targets = ctk_target_table_new_from_list (clipboard->target_list,
                                            &n_targets);
  for (i = 0; i < n_targets; i++)
    {
      CtkSelectionData selection_data;

      /* in each loop iteration, check if the content is still
       * there, because calling get_func() can do anything to
       * the clipboard
       */
      if (!clipboard->target_list || !clipboard->get_func)
        break;

      memset (&selection_data, 0, sizeof (CtkSelectionData));

      selection_data.selection = clipboard->selection;
      selection_data.target = cdk_atom_intern_static_string (targets[i].target);
      selection_data.display = cdk_display_get_default ();
      selection_data.length = -1;

      clipboard->get_func (clipboard, &selection_data,
                           targets[i].info, clipboard->user_data);

      if (selection_data.length >= 0)
        _ctk_quartz_set_selection_data_for_pasteboard (clipboard->pasteboard,
                                                       &selection_data);

      g_free (selection_data.data);
    }

  if (targets)
    ctk_target_table_free (targets, n_targets);
}

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
}

/**
 * ctk_clipboard_get_selection:
 * @clipboard:
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
