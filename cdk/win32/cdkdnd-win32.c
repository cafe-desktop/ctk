/* CDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1999 Peter Mattis, Spencer Kimball and Josh MacDonald
 * Copyright (C) 2001 Archaeopteryx Software Inc.
 * Copyright (C) 1998-2002 Tor Lillqvist
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

/*
 * Modified by the CTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#include "config.h"
#include <string.h>

#include <io.h>
#include <fcntl.h>

/*
 * Support for OLE-2 drag and drop added at Archaeopteryx Software, 2001
 * For more information, contact Stephan R.A. Deibel (sdeibel@archaeopteryx.com)
 *
 * Notes on the implementation:
 *
 * Source drag context, IDragSource and IDataObject for it are created
 * (almost) simultaneously, whereas target drag context and IDropTarget
 * are separated in time - IDropTarget is created when a window is made
 * to accept drops, while target drag context is created when a dragging
 * cursor enters the window and is destroyed when that cursor leaves
 * the window.
 *
 * There's a mismatch between data types supported by W32 (W32 formats)
 * and by CTK+ (CDK targets).
 * To account for it the data is transmuted back and forth. There are two
 * main points of transmutation:
 * * CDK convert selection: transmute W32 data to CTK+ data
 * * CDK window property change: transmute CTK+ data to W32 data
 *
 * There are also two points where data formats are considered:
 * * When source drag context is created, it gets a list of CTK+ targets
 *   that it supports, these are matched to the W32 formats they
 *   correspond to (possibly with transmutation). New W32 formats for
 *   CTK+-specific formats are also created here (see below).
 * * When target drag context is created, it queries the IDataObject
 *   for the list of W32 formats it supports and matches these to
 *   corresponding CTK+ formats that it will be able to provide
 *   (possibly with transmutation) later. Missing CDK targets for
 *   W32-specific formats are also created here (see below).
 *
 * W32 formats and CTK+ targets are both integers (CLIPFORMAT and CdkAtom
 * respectively), but cannot be used interchangeably.
 *
 * To accommodate advanced CTK+ applications the code allows them to
 * register drop targets that accept W32 data formats, and to register
 * drag sources that provide W32 data formats. To do that they must
 * register either with the string name of the format in question
 * (for example, "Shell IDList Array") or, for unnamed pre-defined
 * formats, register with the stringified constant name of the format
 * in question (for example, "CF_UNICODETEXT").
 * If such target format is accepted/provided, CDK will not try to
 * transmute it to/from something else. Otherwise CDK will do the following
 * transmutation:
 * * If CTK+ application provides image/png, image/gif or image/jpeg,
 *   CDK will claim to also provide "PNG", "GIF" or "JFIF" respectively,
 *   and will pass these along verbatim.
 * * If CTK+ application provides any CdkPixbuf-compatible target,
 *   CDK will also offer "PNG" and CF_DIB W32 formats.
 * * If CTK+ application provides UTF8_STRING, CDK will also offer
 *   CF_UNICODETEXT (UTF-16-encoded) and CF_TEXT (encoded with thread-
 *   and locale-depenant codepage), and will do the conversion when such
 *   data is requested.
 * * If CTK+ application accepts image/png, image/gif or image/jpeg,
 *   CDK will claim to also accept "PNG", "GIF" or "JFIF" respectively,
 *   and will pass these along verbatim.
 * * If CTK+ application accepts image/bmp, CDK will
 *   claim to accept CF_DIB W32 format, and will convert
 *   it, changing the header, when such data is provided.
 * * If CTK+ application accepts UTF8_STRING, CDK will
 *   claim to accept CF_UNICODETEXT and CF_TEXT, and will do
 *   the conversion when such data is provided.
 * * If CTK+ application accepts text/uri-list, CDK will
 *   claim to accept "Shell IDList Array", and will do the
 *   conversion when such data is provided.
 *
 * Currently the conversion from text/uri-list to Shell IDList Array is not
 * implemented, so it's not possible to drag & drop files from CTK+
 * applications to non-CTK+ applications the same way one can drag files
 * from Windows Explorer.
 *
 * To accommodate CTK+ application compaibility the code allows
 * CTK+ applications to register drop targets that accept CTK+-specific
 * data formats, and to register drag sources that provide CTK+-specific
 * data formats. This is done by simply registering target atom names
 * as clipboard formats. This way two CTK+ applications can exchange
 * data in their native formats (both well-known ones, such as UTF8_STRING,
 * and special, known only to specific applications). This will work just
 * fine as long as both applications agree on what kind of data is stored
 * under such format exactly.
 *
 * Note that clipboard format space is limited, there can only be 16384
 * of them for a particular user session. Therefore it is highly inadvisable
 * to create and register such formats out of the whole cloth, dynamically.
 * If more flexibility is needed, register one format that has some
 * internal indicators of the kind of data it contains, then write the application
 * in such a way that it requests the data and inspects its header before deciding
 * whether to accept it or not. For details see CTK+ drag & drop documentation
 * on the "drag-motion" and "drag-data-received" signals.
 */

/* The mingw.org compiler does not export GUIDS in it's import library. To work
 * around that, define INITGUID to have the GUIDS declared. */
#if defined(__MINGW32__) && !defined(__MINGW64_VERSION_MAJOR)
#define INITGUID
#endif

/* For C-style COM wrapper macros */
#define COBJMACROS

#include "cdkdnd.h"
#include "cdkproperty.h"
#include "cdkinternals.h"
#include "cdkprivate-win32.h"
#include "cdkwin32.h"
#include "cdkwin32dnd.h"
#include "cdkdisplayprivate.h"
#include "cdk/cdkdndprivate.h"
#include "cdkwin32dnd-private.h"
#include "cdkdisplay-win32.h"
#include "cdkselection-win32.h"

#include <ole2.h>

#include <shlobj.h>
#include <shlguid.h>
#include <objidl.h>

#include <cdk/cdk.h>
#include <glib/gstdio.h>

/* from cdkselection-win32.c */
extern CdkAtom *known_pixbuf_formats;
extern int n_known_pixbuf_formats;


typedef enum {
  CDK_DRAG_STATUS_DRAG,
  CDK_DRAG_STATUS_MOTION_WAIT,
  CDK_DRAG_STATUS_ACTION_WAIT,
  CDK_DRAG_STATUS_DROP
} CdkDragStatus;

static GList *contexts;
static CdkDragContext *current_dest_drag = NULL;
static gboolean use_ole2_dnd = FALSE;

G_DEFINE_TYPE (CdkWin32DragContext, cdk_win32_drag_context, CDK_TYPE_DRAG_CONTEXT)

static void
move_drag_window (CdkDragContext *context,
                  guint           x_root,
                  guint           y_root)
{
  CdkWin32DragContext *context_win32 = CDK_WIN32_DRAG_CONTEXT (context);

  cdk_window_move (context_win32->drag_window,
                   x_root - context_win32->hot_x,
                   y_root - context_win32->hot_y);
  cdk_window_raise (context_win32->drag_window);
}

static void
cdk_win32_drag_context_init (CdkWin32DragContext *context)
{
  if (!use_ole2_dnd)
    {
      contexts = g_list_prepend (contexts, context);
    }
  else
    {
    }

  CDK_NOTE (DND, g_print ("cdk_drag_context_init %p\n", context));
}

static void
cdk_win32_drag_context_finalize (GObject *object)
{
  CdkDragContext *context;
  CdkWin32DragContext *context_win32;
  CdkWindow *drag_window;

  CDK_NOTE (DND, g_print ("cdk_drag_context_finalize %p\n", object));

  g_return_if_fail (CDK_IS_WIN32_DRAG_CONTEXT (object));

  context = CDK_DRAG_CONTEXT (object);
  context_win32 = CDK_WIN32_DRAG_CONTEXT (context);

  if (!use_ole2_dnd)
    {
      contexts = g_list_remove (contexts, context);

      if (context == current_dest_drag)
	current_dest_drag = NULL;
    }

  drag_window = context_win32->drag_window;

  g_array_unref (context_win32->droptarget_format_target_map);

  G_OBJECT_CLASS (cdk_win32_drag_context_parent_class)->finalize (object);

  if (drag_window)
    cdk_window_destroy (drag_window);
}

/* Drag Contexts */

static CdkDragContext *
cdk_drag_context_new (CdkDisplay *display)
{
  CdkWin32DragContext *context_win32;
  CdkWin32Display *win32_display = CDK_WIN32_DISPLAY (display);
  CdkDragContext *context;

  context_win32 = g_object_new (CDK_TYPE_WIN32_DRAG_CONTEXT, NULL);
  context = CDK_DRAG_CONTEXT(context_win32);
  context->display = display;

  cdk_drag_context_set_device (context, cdk_seat_get_pointer (cdk_display_get_default_seat (display)));

  if (win32_display->has_fixed_scale)
    context_win32->scale = win32_display->window_scale;
  else
    context_win32->scale = _cdk_win32_display_get_monitor_scale_factor (win32_display, NULL, NULL, NULL);

  context_win32->droptarget_format_target_map = g_array_new (FALSE, FALSE, sizeof (CdkSelTargetFormat));

  return context;
}

static CdkDragContext *
cdk_drag_context_find (gboolean   is_source,
		       CdkWindow *source,
		       CdkWindow *dest)
{
  GList *tmp_list = contexts;
  CdkDragContext *context;

  while (tmp_list)
    {
      context = (CdkDragContext *)tmp_list->data;

      if ((!context->is_source == !is_source) &&
	  ((source == NULL) || (context->source_window && (context->source_window == source))) &&
	  ((dest == NULL) || (context->dest_window && (context->dest_window == dest))))
	return context;

      tmp_list = tmp_list->next;
    }

  return NULL;
}

#define PRINT_GUID(guid) \
  g_print ("%.08lx-%.04x-%.04x-%.02x%.02x-%.02x%.02x%.02x%.02x%.02x%.02x", \
	   ((gulong *)  guid)[0], \
	   ((gushort *) guid)[2], \
	   ((gushort *) guid)[3], \
	   ((guchar *)  guid)[8], \
	   ((guchar *)  guid)[9], \
	   ((guchar *)  guid)[10], \
	   ((guchar *)  guid)[11], \
	   ((guchar *)  guid)[12], \
	   ((guchar *)  guid)[13], \
	   ((guchar *)  guid)[14], \
	   ((guchar *)  guid)[15]);

typedef struct {
  IDropTarget idt;
  CdkDragContext *context;

  gint ref_count;
  CdkWindow *dest_window;

} target_drag_context;

typedef struct {
  IDropSource ids;
  CdkDragContext *context;
  gint ref_count;
} source_drag_context;

typedef struct {
  IDataObject ido;
  int ref_count;
  CdkDragContext *context;
  GArray *formats;
} data_object;

typedef struct {
  IEnumFORMATETC ief;
  int ref_count;
  int ix;
  data_object *dataobj;
} enum_formats;

static source_drag_context *pending_src_context = NULL;
static source_drag_context *current_src_context = NULL;
static data_object         *current_src_object = NULL;

static enum_formats *enum_formats_new (data_object *dataobj);

/* map windows -> target drag contexts. The table
 * owns a ref to both objects.
 */
static GHashTable* target_ctx_for_window = NULL;

static ULONG STDMETHODCALLTYPE
idroptarget_addref (LPDROPTARGET This)
{
  target_drag_context *ctx = (target_drag_context *) This;

  int ref_count = ++ctx->ref_count;

  CDK_NOTE (DND, g_print ("idroptarget_addref %p %d\n", This, ref_count));

  return ref_count;
}

static HRESULT STDMETHODCALLTYPE
idroptarget_queryinterface (LPDROPTARGET This,
			    REFIID       riid,
			    LPVOID      *ppvObject)
{
  CDK_NOTE (DND, {
      g_print ("idroptarget_queryinterface %p ", This);
      PRINT_GUID (riid);
    });

  *ppvObject = NULL;

  if (IsEqualGUID (riid, &IID_IUnknown))
    {
      CDK_NOTE (DND, g_print ("...IUnknown S_OK\n"));
      idroptarget_addref (This);
      *ppvObject = This;
      return S_OK;
    }
  else if (IsEqualGUID (riid, &IID_IDropTarget))
    {
      CDK_NOTE (DND, g_print ("...IDropTarget S_OK\n"));
      idroptarget_addref (This);
      *ppvObject = This;
      return S_OK;
    }
  else
    {
      CDK_NOTE (DND, g_print ("...E_NOINTERFACE\n"));
      return E_NOINTERFACE;
    }
}

static ULONG STDMETHODCALLTYPE
idroptarget_release (LPDROPTARGET This)
{
  target_drag_context *ctx = (target_drag_context *) This;

  int ref_count = --ctx->ref_count;

  CDK_NOTE (DND, g_print ("idroptarget_release %p %d\n", This, ref_count));

  if (ref_count == 0)
    {
      g_object_unref (ctx->context);
      g_clear_object (&ctx->dest_window);
      g_free (This);
    }

  return ref_count;
}

static CdkDragAction
get_suggested_action (DWORD grfKeyState)
{
  CdkWin32Selection *sel_win32 = _cdk_win32_selection_get ();
  /* This is the yucky Windows standard: Force link action if both
   * Control and Alt are down, copy if Control is down alone, move if
   * Alt is down alone, or use default of move within the app or copy
   * when origin of the drag is in another app.
   */
  if (grfKeyState & MK_CONTROL && grfKeyState & MK_SHIFT)
    return CDK_ACTION_LINK; /* Link action not supported */
  else if (grfKeyState & MK_CONTROL)
    return CDK_ACTION_COPY;
  else if (grfKeyState & MK_ALT)
    return CDK_ACTION_MOVE;
  else if (sel_win32->dnd_source_state == CDK_WIN32_DND_DRAGGING)
    return CDK_ACTION_MOVE;
  else
    return CDK_ACTION_COPY;
  /* Any way to determine when to add in DROPEFFECT_SCROLL? */
}

/* Process pending events -- we don't want to service non-GUI events
 * forever so do one iteration and then do more only if there’s a
 * pending CDK event.
 */
static void
process_pending_events (CdkDisplay *display)
{
  g_main_context_iteration (NULL, FALSE);
  while (_cdk_event_queue_find_first (display))
    g_main_context_iteration (NULL, FALSE);
}

static DWORD
drop_effect_for_action (CdkDragAction action)
{
  switch (action)
    {
    case CDK_ACTION_MOVE:
      return DROPEFFECT_MOVE;
    case CDK_ACTION_LINK:
      return DROPEFFECT_LINK;
    case CDK_ACTION_COPY:
      return DROPEFFECT_COPY;
    default:
      return DROPEFFECT_NONE;
    }
}

static CdkDragAction
action_for_drop_effect (DWORD effect)
{
  switch (effect)
    {
    case DROPEFFECT_MOVE:
      return CDK_ACTION_MOVE;
    case DROPEFFECT_LINK:
      return CDK_ACTION_LINK;
    case DROPEFFECT_COPY:
      return CDK_ACTION_COPY;
    default:
      return 0;
    }
}

static void
dnd_event_put (CdkEventType    type,
	       CdkDragContext *context,
	       gint            pt_x,
	       gint            pt_y,
	       gboolean        to_dest_window)
{
  CdkEvent *e;

  e = cdk_event_new (type);

  if (to_dest_window)
    g_set_object (&e->dnd.window, context->dest_window);
  else
    g_set_object (&e->dnd.window, context->source_window);
  e->dnd.send_event = FALSE;
  g_set_object (&e->dnd.context, context);
  e->dnd.time = CDK_CURRENT_TIME;
  e->dnd.x_root = pt_x;
  e->dnd.y_root = pt_y;

  cdk_event_set_device (e, cdk_drag_context_get_device (context));
  cdk_event_set_seat (e, cdk_device_get_seat (cdk_drag_context_get_device (context)));

  CDK_NOTE (EVENTS, _cdk_win32_print_event (e));
  cdk_event_put (e);
  cdk_event_free (e);
}

static GList *
query_targets (LPDATAOBJECT  pDataObj,
               GArray       *format_target_map)
{
  IEnumFORMATETC *pfmt = NULL;
  FORMATETC fmt;
  GList *result = NULL;
  HRESULT hr;

  if ((LPDATAOBJECT) current_src_object == pDataObj)
    return g_list_copy (current_src_object->context->targets);

  hr = IDataObject_EnumFormatEtc (pDataObj, DATADIR_GET, &pfmt);

  if (SUCCEEDED (hr))
    hr = IEnumFORMATETC_Next (pfmt, 1, &fmt, NULL);

  while (SUCCEEDED (hr) && hr != S_FALSE)
  {
    gboolean is_predef;
    gchar *registered_name = _cdk_win32_get_clipboard_format_name (fmt.cfFormat, &is_predef);

    if (registered_name && is_predef)
      CDK_NOTE (DND, g_print ("supported built-in source format 0x%x %s\n", fmt.cfFormat, registered_name));
    else if (registered_name)
      CDK_NOTE (DND, g_print ("supported source format 0x%x %s\n", fmt.cfFormat, registered_name));
    else
      CDK_NOTE (DND, g_print ("supported unnamed? source format 0x%x\n", fmt.cfFormat));

    g_free (registered_name);

    _cdk_win32_add_format_to_targets (fmt.cfFormat, format_target_map, &result);
    hr = IEnumFORMATETC_Next (pfmt, 1, &fmt, NULL);
  }

  if (pfmt)
    IEnumFORMATETC_Release (pfmt);

  return g_list_reverse (result);
}

static void
set_data_object (LPDATAOBJECT *location, LPDATAOBJECT data_object)
{
  if (*location != NULL)
    IDataObject_Release (*location);

  *location = data_object;

  if (*location != NULL)
    IDataObject_AddRef (*location);
}

static HRESULT STDMETHODCALLTYPE
idroptarget_dragenter (LPDROPTARGET This,
		       LPDATAOBJECT pDataObj,
		       DWORD        grfKeyState,
		       POINTL       pt,
		       LPDWORD      pdwEffect)
{
  target_drag_context *ctx = (target_drag_context *) This;
  CdkWin32Selection *sel_win32 = _cdk_win32_selection_get ();
  CdkDragContext *context;
  CdkWin32DragContext *context_win32;
  gint pt_x;
  gint pt_y;

  CDK_NOTE (DND, g_print ("idroptarget_dragenter %p @ %ld : %ld for dest window 0x%p S_OK\n", This, pt.x, pt.y, ctx->dest_window));

  g_clear_object (&ctx->context);

  context = cdk_drag_context_new (cdk_window_get_display (ctx->dest_window));
  context_win32 = CDK_WIN32_DRAG_CONTEXT (context);
  ctx->context = context;
  g_set_object (&context->dest_window, ctx->dest_window);

  context->protocol = CDK_DRAG_PROTO_OLE2;
  context->is_source = FALSE;
  context->source_window = NULL;

  /* OLE2 DnD does not allow us to get the source window,
   * but we *can* find it if it's ours. This is needed to
   * support DnD within the same widget, for example.
   */
  if (current_src_context && current_src_context->context)
    g_set_object (&context->source_window, current_src_context->context->source_window);
  else
    g_set_object (&context->source_window, cdk_get_default_root_window ());

  g_set_object (&sel_win32->target_drag_context, context);
  context->actions = CDK_ACTION_DEFAULT | CDK_ACTION_COPY | CDK_ACTION_MOVE;
  context->suggested_action = CDK_ACTION_MOVE;
  context->action = CDK_ACTION_MOVE;

  g_array_set_size (context_win32->droptarget_format_target_map, 0);
  context->targets = query_targets (pDataObj, context_win32->droptarget_format_target_map);

  ctx->context->suggested_action = get_suggested_action (grfKeyState);
  set_data_object (&sel_win32->dnd_data_object_target, pDataObj);
  pt_x = pt.x / context_win32->scale + _cdk_offset_x;
  pt_y = pt.y / context_win32->scale + _cdk_offset_y;
  dnd_event_put (CDK_DRAG_ENTER, ctx->context, pt_x, pt_y, TRUE);
  dnd_event_put (CDK_DRAG_MOTION, ctx->context, pt_x, pt_y, TRUE);
  context_win32->last_key_state = grfKeyState;
  context_win32->last_x = pt_x;
  context_win32->last_y = pt_y;
  process_pending_events (cdk_device_get_display (cdk_drag_context_get_device (ctx->context)));
  *pdwEffect = drop_effect_for_action (ctx->context->action);

  CDK_NOTE (DND, g_print ("idroptarget_dragenter returns with action %d and drop effect %lu\n", ctx->context->action, *pdwEffect));

  return S_OK;
}

static HRESULT STDMETHODCALLTYPE
idroptarget_dragover (LPDROPTARGET This,
		      DWORD        grfKeyState,
		      POINTL       pt,
		      LPDWORD      pdwEffect)
{
  target_drag_context *ctx = (target_drag_context *) This;
  CdkWin32DragContext *context_win32 = CDK_WIN32_DRAG_CONTEXT (ctx->context);
  gint pt_x = pt.x / context_win32->scale + _cdk_offset_x;
  gint pt_y = pt.y / context_win32->scale + _cdk_offset_y;

  ctx->context->suggested_action = get_suggested_action (grfKeyState);

  CDK_NOTE (DND, g_print ("idroptarget_dragover %p @ %ld : %ld, suggests %d action S_OK\n", This, pt.x, pt.y, ctx->context->suggested_action));

  if (pt_x != context_win32->last_x ||
      pt_y != context_win32->last_y ||
      grfKeyState != context_win32->last_key_state)
    {
      dnd_event_put (CDK_DRAG_MOTION, ctx->context, pt_x, pt_y, TRUE);
      context_win32->last_key_state = grfKeyState;
      context_win32->last_x = pt_x;
      context_win32->last_y = pt_y;
    }

  process_pending_events (cdk_device_get_display (cdk_drag_context_get_device (ctx->context)));

  *pdwEffect = drop_effect_for_action (ctx->context->action);

  CDK_NOTE (DND, g_print ("idroptarget_dragover returns with action %d and effect %lu\n", ctx->context->action, *pdwEffect));

  return S_OK;
}

static HRESULT STDMETHODCALLTYPE
idroptarget_dragleave (LPDROPTARGET This)
{
  target_drag_context *ctx = (target_drag_context *) This;
  CdkWin32Selection *sel_win32 = _cdk_win32_selection_get ();

  CDK_NOTE (DND, g_print ("idroptarget_dragleave %p S_OK\n", This));

  dnd_event_put (CDK_DRAG_LEAVE, ctx->context, 0, 0, TRUE);
  process_pending_events (cdk_device_get_display (cdk_drag_context_get_device (ctx->context)));

  g_clear_object (&sel_win32->target_drag_context);
  g_clear_object (&ctx->context);
  set_data_object (&sel_win32->dnd_data_object_target, NULL);

  return S_OK;
}

static HRESULT STDMETHODCALLTYPE
idroptarget_drop (LPDROPTARGET This,
		  LPDATAOBJECT pDataObj,
		  DWORD        grfKeyState,
		  POINTL       pt,
		  LPDWORD      pdwEffect)
{
  target_drag_context *ctx = (target_drag_context *) This;
  CdkWin32DragContext *context_win32 = CDK_WIN32_DRAG_CONTEXT (ctx->context);
  gint pt_x = pt.x / context_win32->scale + _cdk_offset_x;
  gint pt_y = pt.y / context_win32->scale + _cdk_offset_y;
  CdkWin32Selection *sel_win32 = _cdk_win32_selection_get ();

  CDK_NOTE (DND, g_print ("idroptarget_drop %p ", This));

  if (pDataObj == NULL)
    {
      CDK_NOTE (DND, g_print ("E_POINTER\n"));
      g_clear_object (&ctx->context);
      return E_POINTER;
    }

  ctx->context->suggested_action = get_suggested_action (grfKeyState);

  dnd_event_put (CDK_DROP_START, ctx->context, pt_x, pt_y, TRUE);
  process_pending_events (cdk_device_get_display (cdk_drag_context_get_device (ctx->context)));

  /* Notify OLE of copy or move */
  if (sel_win32->dnd_target_state != CDK_WIN32_DND_DROPPED)
    *pdwEffect = DROPEFFECT_NONE;
  else
    *pdwEffect = drop_effect_for_action (ctx->context->action);

  g_clear_object (&sel_win32->target_drag_context);
  g_clear_object (&ctx->context);
  set_data_object (&sel_win32->dnd_data_object_target, NULL);

  CDK_NOTE (DND, g_print ("drop S_OK with effect %lx\n", *pdwEffect));

  return S_OK;
}

static ULONG STDMETHODCALLTYPE
idropsource_addref (LPDROPSOURCE This)
{
  source_drag_context *ctx = (source_drag_context *) This;

  int ref_count = ++ctx->ref_count;

  CDK_NOTE (DND, g_print ("idropsource_addref %p %d\n", This, ref_count));

  return ref_count;
}

static HRESULT STDMETHODCALLTYPE
idropsource_queryinterface (LPDROPSOURCE This,
			    REFIID       riid,
			    LPVOID      *ppvObject)
{
  CDK_NOTE (DND, {
      g_print ("idropsource_queryinterface %p ", This);
      PRINT_GUID (riid);
    });

  *ppvObject = NULL;

  if (IsEqualGUID (riid, &IID_IUnknown))
    {
      CDK_NOTE (DND, g_print ("...IUnknown S_OK\n"));
      idropsource_addref (This);
      *ppvObject = This;
      return S_OK;
    }
  else if (IsEqualGUID (riid, &IID_IDropSource))
    {
      CDK_NOTE (DND, g_print ("...IDropSource S_OK\n"));
      idropsource_addref (This);
      *ppvObject = This;
      return S_OK;
    }
  else
    {
      CDK_NOTE (DND, g_print ("...E_NOINTERFACE\n"));
      return E_NOINTERFACE;
    }
}

static ULONG STDMETHODCALLTYPE
idropsource_release (LPDROPSOURCE This)
{
  source_drag_context *ctx = (source_drag_context *) This;

  int ref_count = --ctx->ref_count;

  CDK_NOTE (DND, g_print ("idropsource_release %p %d\n", This, ref_count));

  if (ref_count == 0)
  {
    g_clear_object (&ctx->context);
    if (current_src_context == ctx)
      current_src_context = NULL;
    g_free (This);
  }

  return ref_count;
}

/* Emit CDK events for any changes in mouse events or control key
 * state since the last recorded state. Return true if any events
 * have been emitted and false otherwise.
 */
static gboolean
send_change_events (CdkDragContext *context,
		    DWORD           key_state,
		    gboolean        esc_pressed)
{
  CdkWin32DragContext *context_win32 = CDK_WIN32_DRAG_CONTEXT (context);
  POINT pt;
  POINT pt_client;
  gboolean changed = FALSE;
  HWND hwnd = CDK_WINDOW_HWND (context->source_window);
  LPARAM lparam;
  WPARAM wparam;
  gint pt_x;
  gint pt_y;

  if (!API_CALL (GetCursorPos, (&pt)))
    return FALSE;

  pt_client = pt;

  if (!API_CALL (ScreenToClient, (hwnd, &pt_client)))
    return FALSE;

  pt_x = pt.x / context_win32->scale + _cdk_offset_x;
  pt_y = pt.y / context_win32->scale + _cdk_offset_y;

  if (pt_x != context_win32->last_x || pt_y != context_win32->last_y ||
      key_state != context_win32->last_key_state)
    {
      lparam = MAKELPARAM (pt_client.x, pt_client.y);
      wparam = key_state;
      if (pt_x != context_win32->last_x || pt_y != context_win32->last_y)
	{
	  CDK_NOTE (DND, g_print ("Sending WM_MOUSEMOVE (%ld,%ld)\n", pt.x, pt.y));
      	  SendMessage (hwnd, WM_MOUSEMOVE, wparam, lparam);
	}

      if ((key_state & MK_LBUTTON) != (context_win32->last_key_state & MK_LBUTTON))
	{
	  if (key_state & MK_LBUTTON)
	    SendMessage (hwnd, WM_LBUTTONDOWN, wparam, lparam);
	  else
	    SendMessage (hwnd, WM_LBUTTONUP, wparam, lparam);
	}
      if ((key_state & MK_MBUTTON) != (context_win32->last_key_state & MK_MBUTTON))
	{
	  if (key_state & MK_MBUTTON)
	    SendMessage (hwnd, WM_MBUTTONDOWN, wparam, lparam);
	  else
	    SendMessage (hwnd, WM_MBUTTONUP, wparam, lparam);
	}
      if ((key_state & MK_RBUTTON) != (context_win32->last_key_state & MK_RBUTTON))
	{
	  if (key_state & MK_RBUTTON)
	    SendMessage (hwnd, WM_RBUTTONDOWN, wparam, lparam);
	  else
	    SendMessage (hwnd, WM_RBUTTONUP, wparam, lparam);
	}
      if ((key_state & MK_CONTROL) != (context_win32->last_key_state & MK_CONTROL))
	{
	  if (key_state & MK_CONTROL)
	    SendMessage (hwnd, WM_KEYDOWN, VK_CONTROL, 0);
	  else
	    SendMessage (hwnd, WM_KEYUP, VK_CONTROL, 0);
	}
      if ((key_state & MK_SHIFT) != (context_win32->last_key_state & MK_SHIFT))
	{
	  if (key_state & MK_SHIFT)
	    SendMessage (hwnd, WM_KEYDOWN, VK_SHIFT, 0);
	  else
	    SendMessage (hwnd, WM_KEYUP, VK_SHIFT, 0);
	}

      changed = TRUE;
      context_win32->last_key_state = key_state;
      context_win32->last_x = pt_x;
      context_win32->last_y = pt_y;
    }

  if (esc_pressed)
    {
      CDK_NOTE (DND, g_print ("Sending a escape key down message to %p\n", hwnd));
      SendMessage (hwnd, WM_KEYDOWN, VK_ESCAPE, 0);
      changed = TRUE;
    }

  return changed;
}

static HRESULT STDMETHODCALLTYPE
idropsource_querycontinuedrag (LPDROPSOURCE This,
			       BOOL         fEscapePressed,
			       DWORD        grfKeyState)
{
  source_drag_context *ctx = (source_drag_context *) This;
  CdkWin32Selection *sel_win32 = _cdk_win32_selection_get ();

  CDK_NOTE (DND, g_print ("idropsource_querycontinuedrag %p esc=%d keystate=0x%lx ", This, fEscapePressed, grfKeyState));

  if (send_change_events (ctx->context, grfKeyState, fEscapePressed))
    process_pending_events (cdk_device_get_display (cdk_drag_context_get_device (ctx->context)));

  if (sel_win32->dnd_source_state == CDK_WIN32_DND_DROPPED)
    {
      CDK_NOTE (DND, g_print ("DRAGDROP_S_DROP\n"));
      return DRAGDROP_S_DROP;
    }
  else if (sel_win32->dnd_source_state == CDK_WIN32_DND_NONE)
    {
      CDK_NOTE (DND, g_print ("DRAGDROP_S_CANCEL\n"));
      return DRAGDROP_S_CANCEL;
    }
  else
    {
      CDK_NOTE (DND, g_print ("S_OK\n"));
      return S_OK;
    }
}

static HRESULT STDMETHODCALLTYPE
idropsource_givefeedback (LPDROPSOURCE This,
			  DWORD        dwEffect)
{
  source_drag_context *ctx = (source_drag_context *) This;
  CdkWin32DragContext *context_win32 = CDK_WIN32_DRAG_CONTEXT (ctx->context);
  CdkDragAction suggested_action;
  CdkEvent *e;
  POINT pt;

  CDK_NOTE (DND, g_print ("idropsource_givefeedback %p with drop effect %lu S_OK\n", This, dwEffect));

  if (!API_CALL (GetCursorPos, (&pt)))
    return S_OK;

  suggested_action = action_for_drop_effect (dwEffect);

  ctx->context->action = suggested_action;

  if (dwEffect == DROPEFFECT_NONE)
    g_clear_object (&ctx->context->dest_window);
  else if (ctx->context->dest_window == NULL)
    ctx->context->dest_window = g_object_ref (cdk_get_default_root_window ());

  context_win32->last_x = pt.x / context_win32->scale + _cdk_offset_x;
  context_win32->last_y = pt.y / context_win32->scale + _cdk_offset_y;

  e = cdk_event_new (CDK_DRAG_STATUS);

  g_set_object (&e->dnd.window, ctx->context->source_window);
  e->dnd.send_event = FALSE;
  g_set_object (&e->dnd.context, ctx->context);
  e->dnd.time = CDK_CURRENT_TIME;
  e->dnd.x_root = context_win32->last_x;
  e->dnd.y_root = context_win32->last_y;

  cdk_event_set_device (e, cdk_drag_context_get_device (ctx->context));
  cdk_event_set_seat (e, cdk_device_get_seat (cdk_drag_context_get_device (ctx->context)));

  CDK_NOTE (EVENTS, _cdk_win32_print_event (e));
  cdk_event_put (e);
  cdk_event_free (e);
  process_pending_events (cdk_device_get_display (cdk_drag_context_get_device (ctx->context)));

  CDK_NOTE (DND, g_print ("idropsource_givefeedback %p returns\n", This));

  return S_OK;
}

static ULONG STDMETHODCALLTYPE
idataobject_addref (LPDATAOBJECT This)
{
  data_object *dobj = (data_object *) This;
  int ref_count = ++dobj->ref_count;

  CDK_NOTE (DND, g_print ("idataobject_addref %p %d\n", This, ref_count));

  return ref_count;
}

static HRESULT STDMETHODCALLTYPE
idataobject_queryinterface (LPDATAOBJECT This,
			    REFIID       riid,
			    LPVOID      *ppvObject)
{
  CDK_NOTE (DND, {
      g_print ("idataobject_queryinterface %p ", This);
      PRINT_GUID (riid);
    });

  *ppvObject = NULL;

  if (IsEqualGUID (riid, &IID_IUnknown))
    {
      CDK_NOTE (DND, g_print ("...IUnknown S_OK\n"));
      idataobject_addref (This);
      *ppvObject = This;
      return S_OK;
    }
  else if (IsEqualGUID (riid, &IID_IDataObject))
    {
      CDK_NOTE (DND, g_print ("...IDataObject S_OK\n"));
      idataobject_addref (This);
      *ppvObject = This;
      return S_OK;
    }
  else
    {
      CDK_NOTE (DND, g_print ("...E_NOINTERFACE\n"));
      return E_NOINTERFACE;
    }
}

static ULONG STDMETHODCALLTYPE
idataobject_release (LPDATAOBJECT This)
{
  data_object *dobj = (data_object *) This;
  int ref_count = --dobj->ref_count;

  CDK_NOTE (DND, g_print ("idataobject_release %p %d\n", This, ref_count));

  if (ref_count == 0)
    {
      g_array_free (dobj->formats, TRUE);
      g_free (This);
    }

  return ref_count;
}

static HRESULT
query (LPDATAOBJECT This,
       LPFORMATETC  pFormatEtc)
{
  data_object *ctx = (data_object *) This;
  gint i;

  if (!pFormatEtc)
    return DV_E_FORMATETC;

  if (pFormatEtc->lindex != -1)
    return DV_E_LINDEX;

  if ((pFormatEtc->tymed & TYMED_HGLOBAL) == 0)
    return DV_E_TYMED;

  if ((pFormatEtc->dwAspect & DVASPECT_CONTENT) == 0)
    return DV_E_DVASPECT;

  for (i = 0; i < ctx->formats->len; i++)
    if (pFormatEtc->cfFormat == g_array_index (ctx->formats, CdkSelTargetFormat, i).format)
      return S_OK;

  return DV_E_FORMATETC;
}

static HRESULT STDMETHODCALLTYPE
idataobject_getdata (LPDATAOBJECT This,
		     LPFORMATETC  pFormatEtc,
		     LPSTGMEDIUM  pMedium)
{
  data_object *ctx = (data_object *) This;
  CdkWin32Selection *win32_sel = _cdk_win32_selection_get ();
  HRESULT hr;
  CdkEvent e;
  gint i;
  CdkAtom target;
  gint64 loopend;

  CDK_NOTE (DND, g_print ("idataobject_getdata %p %s ",
			  This, _cdk_win32_cf_to_string (pFormatEtc->cfFormat)));

  /* Check whether we can provide requested format */
  hr = query (This, pFormatEtc);
  if (hr != S_OK)
    {
      CDK_NOTE (DND, g_print ("Unsupported format, returning 0x%lx\n", hr));
      return hr;
    }

  /* Append a CDK_SELECTION_REQUEST event and then hope the app sets the
   * property associated with the _cdk_ole2_dnd atom
   */
  win32_sel->property_change_format = pFormatEtc->cfFormat;
  win32_sel->property_change_data = pMedium;

  for (i = 0, target = NULL; i < ctx->formats->len; i++)
    {
      CdkSelTargetFormat *frec = &g_array_index (ctx->formats, CdkSelTargetFormat, i);

      if (frec->format == pFormatEtc->cfFormat)
        {
          target = frec->target;
          win32_sel->property_change_transmute = frec->transmute;
          win32_sel->property_change_target_atom = frec->target;
        }
    }

  if (target == NULL)
    {
      CDK_NOTE (EVENTS, g_print ("(target not found)"));
      return E_UNEXPECTED;
    }

  CDK_NOTE (DND, {
      gchar *target_name = cdk_atom_name (target);
      g_print ("idataobject_getdata will request target 0x%p (%s) ",
               target, target_name);
      g_free (target_name);
    });


  memset (&e, 0, sizeof (CdkEvent));
  e.type = CDK_SELECTION_REQUEST;
  g_set_object (&e.selection.window, ctx->context->source_window);
  e.selection.send_event = FALSE; /* ??? */
  /* Both selection and property are OLE2_DND, because change_property()
   * will only get the property and not the selection. Theoretically we
   * could use two different atoms (SELECTION_OLE2_DND and PROPERTY_OLE2_DND),
   * but there's little reason to do so.
   */
  e.selection.selection = _cdk_win32_selection_atom (CDK_WIN32_ATOM_INDEX_OLE2_DND);
  e.selection.target = target;
  /* Requestor here is fake, just to allow the event to be processed */
  g_set_object (&e.selection.requestor, ctx->context->source_window);
  e.selection.property = _cdk_win32_selection_atom (CDK_WIN32_ATOM_INDEX_OLE2_DND);
  e.selection.time = CDK_CURRENT_TIME;

  CDK_NOTE (EVENTS, _cdk_win32_print_event (&e));

  cdk_event_put (&e);

  /* Don't hold up longer than one second */
  loopend = g_get_monotonic_time () + 1000000000;

  while (win32_sel->property_change_data != 0 &&
         g_get_monotonic_time () < loopend)
    process_pending_events (cdk_device_get_display (cdk_drag_context_get_device (ctx->context)));

  if (pMedium->hGlobal == NULL) {
    CDK_NOTE (DND, g_print (" E_UNEXPECTED\n"));
    return E_UNEXPECTED;
  }

  CDK_NOTE (DND, g_print (" S_OK\n"));
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE
idataobject_getdatahere (LPDATAOBJECT This,
			 LPFORMATETC  pFormatEtc,
			 LPSTGMEDIUM  pMedium)
{
  CDK_NOTE (DND, g_print ("idataobject_getdatahere %p %s E_UNEXPECTED\n",
			  This, _cdk_win32_cf_to_string (pFormatEtc->cfFormat)));

  return E_UNEXPECTED;
}

static HRESULT STDMETHODCALLTYPE
idataobject_querygetdata (LPDATAOBJECT This,
			  LPFORMATETC  pFormatEtc)
{
  HRESULT hr;

  hr = query (This, pFormatEtc);

#define CASE(x) case x: g_print (#x "\n"); break
  CDK_NOTE (DND, {
      g_print ("idataobject_querygetdata %p %s ",
	       This, _cdk_win32_cf_to_string (pFormatEtc->cfFormat));
      switch (hr)
	{
	CASE (DV_E_FORMATETC);
	CASE (DV_E_LINDEX);
	CASE (DV_E_TYMED);
	CASE (DV_E_DVASPECT);
	CASE (S_OK);
	default: g_print ("%#lx", hr);
	}
    });

  return hr;
}

static HRESULT STDMETHODCALLTYPE
idataobject_getcanonicalformatetc (LPDATAOBJECT This,
				   LPFORMATETC  pFormatEtcIn,
				   LPFORMATETC  pFormatEtcOut)
{
  CDK_NOTE (DND, g_print ("idataobject_getcanonicalformatetc %p E_UNEXPECTED\n", This));

  return E_UNEXPECTED;
}

static HRESULT STDMETHODCALLTYPE
idataobject_setdata (LPDATAOBJECT This,
		     LPFORMATETC  pFormatEtc,
		     LPSTGMEDIUM  pMedium,
		     BOOL         fRelease)
{
  CDK_NOTE (DND, g_print ("idataobject_setdata %p %s E_UNEXPECTED\n",
			  This, _cdk_win32_cf_to_string (pFormatEtc->cfFormat)));

  return E_UNEXPECTED;
}

static HRESULT STDMETHODCALLTYPE
idataobject_enumformatetc (LPDATAOBJECT     This,
			   DWORD            dwDirection,
			   LPENUMFORMATETC *ppEnumFormatEtc)
{
  CDK_NOTE (DND, g_print ("idataobject_enumformatetc %p ", This));

  if (dwDirection != DATADIR_GET)
    {
      CDK_NOTE (DND, g_print ("E_NOTIMPL\n"));
      return E_NOTIMPL;
    }

  *ppEnumFormatEtc = &enum_formats_new ((data_object *) This)->ief;

  CDK_NOTE (DND, g_print (" %p S_OK\n", *ppEnumFormatEtc));

  return S_OK;
}

static HRESULT STDMETHODCALLTYPE
idataobject_dadvise (LPDATAOBJECT This,
		     LPFORMATETC  pFormatetc,
		     DWORD        advf,
		     LPADVISESINK pAdvSink,
		     DWORD       *pdwConnection)
{
  CDK_NOTE (DND, g_print ("idataobject_dadvise %p E_NOTIMPL\n", This));

  return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
idataobject_dunadvise (LPDATAOBJECT This,
		       DWORD         dwConnection)
{
  CDK_NOTE (DND, g_print ("idataobject_dunadvise %p E_NOTIMPL\n", This));

  return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
idataobject_enumdadvise (LPDATAOBJECT    This,
			 LPENUMSTATDATA *ppenumAdvise)
{
  CDK_NOTE (DND, g_print ("idataobject_enumdadvise %p OLE_E_ADVISENOTSUPPORTED\n", This));

  return OLE_E_ADVISENOTSUPPORTED;
}

static ULONG STDMETHODCALLTYPE
ienumformatetc_addref (LPENUMFORMATETC This)
{
  enum_formats *en = (enum_formats *) This;
  int ref_count = ++en->ref_count;

  CDK_NOTE (DND, g_print ("ienumformatetc_addref %p %d\n", This, ref_count));

  return ref_count;
}

static HRESULT STDMETHODCALLTYPE
ienumformatetc_queryinterface (LPENUMFORMATETC This,
			       REFIID          riid,
			       LPVOID         *ppvObject)
{
  CDK_NOTE (DND, {
      g_print ("ienumformatetc_queryinterface %p", This);
      PRINT_GUID (riid);
    });

  *ppvObject = NULL;

  if (IsEqualGUID (riid, &IID_IUnknown))
    {
      CDK_NOTE (DND, g_print ("...IUnknown S_OK\n"));
      ienumformatetc_addref (This);
      *ppvObject = This;
      return S_OK;
    }
  else if (IsEqualGUID (riid, &IID_IEnumFORMATETC))
    {
      CDK_NOTE (DND, g_print ("...IEnumFORMATETC S_OK\n"));
      ienumformatetc_addref (This);
      *ppvObject = This;
      return S_OK;
    }
  else
    {
      CDK_NOTE (DND, g_print ("...E_NOINTERFACE\n"));
      return E_NOINTERFACE;
    }
}

static ULONG STDMETHODCALLTYPE
ienumformatetc_release (LPENUMFORMATETC This)
{
  enum_formats *en = (enum_formats *) This;
  int ref_count = --en->ref_count;

  CDK_NOTE (DND, g_print ("ienumformatetc_release %p %d\n", This, ref_count));

  if (ref_count == 0)
    {
      idataobject_release ((LPDATAOBJECT) en->dataobj);
      g_free (This);
    }

  return ref_count;
}

static HRESULT STDMETHODCALLTYPE
ienumformatetc_next (LPENUMFORMATETC This,
		     ULONG	     celt,
		     LPFORMATETC     elts,
		     ULONG	    *nelt)
{
  enum_formats *en = (enum_formats *) This;
  ULONG i, n;
  ULONG formats_to_get = celt;

  CDK_NOTE (DND, g_print ("ienumformatetc_next %p %d %ld ", This, en->ix, celt));

  n = 0;
  for (i = 0; i < formats_to_get; i++)
    {
      UINT fmt;
      if (en->ix >= en->dataobj->formats->len)
	break;
      fmt = g_array_index (en->dataobj->formats, CdkSelTargetFormat, en->ix++).format;
      /* skip internals */
      if (fmt == 0 || fmt > 0xFFFF)
        {
          formats_to_get += 1;
          continue;
        }
      elts[n].cfFormat = fmt;
      elts[n].ptd = NULL;
      elts[n].dwAspect = DVASPECT_CONTENT;
      elts[n].lindex = -1;
      elts[n].tymed = TYMED_HGLOBAL;

      n++;
    }

  if (nelt != NULL)
    *nelt = n;

  CDK_NOTE (DND, g_print ("%s\n", (n == celt) ? "S_OK" : "S_FALSE"));

  if (n == celt)
    return S_OK;
  else
    return S_FALSE;
}

static HRESULT STDMETHODCALLTYPE
ienumformatetc_skip (LPENUMFORMATETC This,
		     ULONG	     celt)
{
  enum_formats *en = (enum_formats *) This;

  CDK_NOTE (DND, g_print ("ienumformatetc_skip %p %d %ld S_OK\n", This, en->ix, celt));

  en->ix += celt;

  return S_OK;
}

static HRESULT STDMETHODCALLTYPE
ienumformatetc_reset (LPENUMFORMATETC This)
{
  enum_formats *en = (enum_formats *) This;

  CDK_NOTE (DND, g_print ("ienumformatetc_reset %p S_OK\n", This));

  en->ix = 0;

  return S_OK;
}

static HRESULT STDMETHODCALLTYPE
ienumformatetc_clone (LPENUMFORMATETC  This,
		      LPENUMFORMATETC *ppEnumFormatEtc)
{
  enum_formats *en = (enum_formats *) This;
  enum_formats *new;

  CDK_NOTE (DND, g_print ("ienumformatetc_clone %p S_OK\n", This));

  new = enum_formats_new (en->dataobj);

  new->ix = en->ix;

  *ppEnumFormatEtc = &new->ief;

  return S_OK;
}

static IDropTargetVtbl idt_vtbl = {
  idroptarget_queryinterface,
  idroptarget_addref,
  idroptarget_release,
  idroptarget_dragenter,
  idroptarget_dragover,
  idroptarget_dragleave,
  idroptarget_drop
};

static IDropSourceVtbl ids_vtbl = {
  idropsource_queryinterface,
  idropsource_addref,
  idropsource_release,
  idropsource_querycontinuedrag,
  idropsource_givefeedback
};

static IDataObjectVtbl ido_vtbl = {
  idataobject_queryinterface,
  idataobject_addref,
  idataobject_release,
  idataobject_getdata,
  idataobject_getdatahere,
  idataobject_querygetdata,
  idataobject_getcanonicalformatetc,
  idataobject_setdata,
  idataobject_enumformatetc,
  idataobject_dadvise,
  idataobject_dunadvise,
  idataobject_enumdadvise
};

static IEnumFORMATETCVtbl ief_vtbl = {
  ienumformatetc_queryinterface,
  ienumformatetc_addref,
  ienumformatetc_release,
  ienumformatetc_next,
  ienumformatetc_skip,
  ienumformatetc_reset,
  ienumformatetc_clone
};


static target_drag_context *
target_context_new (CdkWindow *window)
{
  target_drag_context *result;

  result = g_new0 (target_drag_context, 1);
  result->idt.lpVtbl = &idt_vtbl;
  result->ref_count = 0;

  result->dest_window = g_object_ref (window);

  idroptarget_addref (&result->idt);

  CDK_NOTE (DND, g_print ("target_context_new: %p (window %p)\n", result, result->dest_window));

  return result;
}

static source_drag_context *
source_context_new (CdkWindow *window,
		    GList     *targets)
{
  CdkDragContext *context;
  CdkWin32DragContext *context_win32;
  source_drag_context *result;
  GList *p;

  context = cdk_drag_context_new (cdk_window_get_display (window));
  context_win32 = CDK_WIN32_DRAG_CONTEXT (context);

  result = g_new0 (source_drag_context, 1);
  result->context = context;
  result->ids.lpVtbl = &ids_vtbl;
  result->ref_count = 0;

  result->context->protocol = CDK_DRAG_PROTO_OLE2;
  result->context->is_source = TRUE;

  result->context->source_window = g_object_ref (window);

  result->context->dest_window = NULL;
  result->context->targets = g_list_copy (targets);
  context_win32->has_image_format = FALSE;
  context_win32->has_cf_png = FALSE;
  context_win32->has_cf_dib = FALSE;
  context_win32->has_text_uri_list = FALSE;
  context_win32->has_shell_id_list = FALSE;
  context_win32->has_unicodetext = FALSE;

  for (p = result->context->targets; p; p = g_list_next (p))
    {
      gint j;
      CdkAtom target = p->data;

      if (target == _cdk_win32_selection_atom (CDK_WIN32_ATOM_INDEX_TEXT_URI_LIST))
        context_win32->has_text_uri_list = TRUE;
      else if (target == _cdk_win32_selection_atom (CDK_WIN32_ATOM_INDEX_CFSTR_SHELLIDLIST))
        context_win32->has_shell_id_list = TRUE;
      else if (target == _cdk_win32_selection_atom (CDK_WIN32_ATOM_INDEX_PNG))
        context_win32->has_cf_png = TRUE;
      else if (target == _cdk_win32_selection_atom (CDK_WIN32_ATOM_INDEX_CF_DIB))
        context_win32->has_cf_dib = TRUE;
      else if (target == _cdk_win32_selection_atom (CDK_WIN32_ATOM_INDEX_GIF))
        context_win32->has_gif = TRUE;
      else if (target == _cdk_win32_selection_atom (CDK_WIN32_ATOM_INDEX_JFIF))
        context_win32->has_jfif = TRUE;
      else if (target == _cdk_win32_selection_atom (CDK_WIN32_ATOM_INDEX_CF_UNICODETEXT))
        context_win32->has_unicodetext = TRUE;

      for (j = 0; !context_win32->has_image_format && j < _cdk_win32_selection_get ()->n_known_pixbuf_formats; j++)
        if (target == _cdk_win32_selection_get ()->known_pixbuf_formats[j])
          context_win32->has_image_format = TRUE;
    }

  idropsource_addref (&result->ids);

  CDK_NOTE (DND, g_print ("source_context_new: %p (drag context %p)\n", result, result->context));

  if (current_src_context == NULL)
    current_src_context = result;

  return result;
}

static data_object *
data_object_new (CdkDragContext *context)
{
  data_object *result;
  GList *p;

  result = g_new0 (data_object, 1);

  result->ido.lpVtbl = &ido_vtbl;
  result->ref_count = 1;
  result->context = context;
  result->formats = g_array_new (FALSE, FALSE, sizeof (CdkSelTargetFormat));

  for (p = context->targets; p; p = p->next)
    {
      CdkAtom target = p->data;
      gchar *target_name = cdk_atom_name (target);
      gint added_count = 0;
      gint i;

      CDK_NOTE (DND, g_print ("DataObject supports target 0x%p (%s)\n", target, target_name));
      g_free (target_name);

      added_count = _cdk_win32_add_target_to_selformats (target, result->formats);

      for (i = 0; i < added_count && result->formats->len - 1 - i >= 0; i++)
        CDK_NOTE (DND, g_print ("DataObject will support format 0x%x\n", g_array_index (result->formats, CdkSelTargetFormat, i).format));
    }

  CDK_NOTE (DND, g_print ("data_object_new: %p\n", result));

  return result;
}

static enum_formats *
enum_formats_new (data_object *dataobj)
{
  enum_formats *result;

  result = g_new0 (enum_formats, 1);

  result->ief.lpVtbl = &ief_vtbl;
  result->ref_count = 1;
  result->ix = 0;
  result->dataobj = dataobj;
  idataobject_addref ((LPDATAOBJECT) dataobj);

  return result;
}

/* From MS Knowledge Base article Q130698 */

static gboolean
resolve_link (HWND     hWnd,
	      wchar_t *link,
	      gchar  **lpszPath)
{
  WIN32_FILE_ATTRIBUTE_DATA wfad;
  HRESULT hr;
  IShellLinkW *pslW = NULL;
  IPersistFile *ppf = NULL;

  /* Check if the file is empty first because IShellLink::Resolve for
   * some reason succeeds with an empty file and returns an empty
   * "link target". (#524151)
   */
    if (!GetFileAttributesExW (link, GetFileExInfoStandard, &wfad) ||
	(wfad.nFileSizeHigh == 0 && wfad.nFileSizeLow == 0))
      return FALSE;

  /* Assume failure to start with: */
  *lpszPath = 0;

  /* Call CoCreateInstance to obtain the IShellLink interface
   * pointer. This call fails if CoInitialize is not called, so it is
   * assumed that CoInitialize has been called.
   */

  hr = CoCreateInstance (&CLSID_ShellLink,
			 NULL,
			 CLSCTX_INPROC_SERVER,
			 &IID_IShellLinkW,
			 (LPVOID *)&pslW);

  if (SUCCEEDED (hr))
   {
     /* The IShellLink interface supports the IPersistFile
      * interface. Get an interface pointer to it.
      */
     hr = pslW->lpVtbl->QueryInterface (pslW,
					&IID_IPersistFile,
					(LPVOID *) &ppf);
   }

  if (SUCCEEDED (hr))
    {
      /* Load the file. */
      hr = ppf->lpVtbl->Load (ppf, link, STGM_READ);
    }

  if (SUCCEEDED (hr))
    {
      /* Resolve the link by calling the Resolve()
       * interface function.
       */
      hr = pslW->lpVtbl->Resolve (pslW, hWnd, SLR_ANY_MATCH | SLR_NO_UI);
    }

  if (SUCCEEDED (hr))
    {
      wchar_t wtarget[MAX_PATH];

      hr = pslW->lpVtbl->GetPath (pslW, wtarget, MAX_PATH, NULL, 0);
      if (SUCCEEDED (hr))
	*lpszPath = g_utf16_to_utf8 (wtarget, -1, NULL, NULL, NULL);
    }

  if (ppf)
    ppf->lpVtbl->Release (ppf);

  if (pslW)
    pslW->lpVtbl->Release (pslW);

  return SUCCEEDED (hr);
}

#if 0

/* Check for filenames like C:\Users\tml\AppData\Local\Temp\d5qtkvvs.bmp */
static gboolean
filename_looks_tempish (const char *filename)
{
  char *dirname;
  char *p;
  const char *q;
  gboolean retval = FALSE;

  dirname = g_path_get_dirname (filename);

  p = dirname;
  q = g_get_tmp_dir ();

  while (*p && *q &&
	 ((G_IS_DIR_SEPARATOR (*p) && G_IS_DIR_SEPARATOR (*q)) ||
	  g_ascii_tolower (*p) == g_ascii_tolower (*q)))
    p++, q++;

  if (!*p && !*q)
    retval = TRUE;

  g_free (dirname);

  return retval;
}

static gboolean
close_it (gpointer data)
{
  close (GPOINTER_TO_INT (data));

  return FALSE;
}

#endif

static CdkFilterReturn
cdk_dropfiles_filter (CdkXEvent *xev,
		      CdkEvent  *event,
		      gpointer   data)
{
  CdkDragContext *context;
  CdkWin32DragContext *context_win32;
  GString *result;
  MSG *msg = (MSG *) xev;
  HANDLE hdrop;
  POINT pt;
  gint nfiles, i;
  gchar *fileName, *linkedFile;

  if (msg->message == WM_DROPFILES)
    {
      CDK_NOTE (DND, g_print ("WM_DROPFILES: %p\n", msg->hwnd));

      context = cdk_drag_context_new (cdk_window_get_display (event->any.window));
      context_win32 = CDK_WIN32_DRAG_CONTEXT (context);
      context->protocol = CDK_DRAG_PROTO_WIN32_DROPFILES;
      context->is_source = FALSE;

      context->source_window = cdk_get_default_root_window ();
      g_object_ref (context->source_window);

      g_set_object (&context->dest_window, event->any.window);

      /* WM_DROPFILES drops are always file names */
      context->targets =
	g_list_append (NULL, _cdk_win32_selection_atom (CDK_WIN32_ATOM_INDEX_TEXT_URI_LIST));
      context->actions = CDK_ACTION_COPY;
      context->suggested_action = CDK_ACTION_COPY;
      current_dest_drag = context;

      event->dnd.type = CDK_DROP_START;
      event->dnd.context = current_dest_drag;
      cdk_event_set_device (event, cdk_drag_context_get_device (current_dest_drag));
      cdk_event_set_seat (event, cdk_device_get_seat (cdk_drag_context_get_device (current_dest_drag)));

      hdrop = (HANDLE) msg->wParam;
      DragQueryPoint (hdrop, &pt);
      ClientToScreen (msg->hwnd, &pt);

      event->dnd.x_root = pt.x / context_win32->scale + _cdk_offset_x;
      event->dnd.y_root = pt.y / context_win32->scale + _cdk_offset_y;
      event->dnd.time = _cdk_win32_get_next_tick (msg->time);

      nfiles = DragQueryFile (hdrop, 0xFFFFFFFF, NULL, 0);

      result = g_string_new (NULL);
      for (i = 0; i < nfiles; i++)
	{
	  gchar *uri;
	  wchar_t wfn[MAX_PATH];

	  DragQueryFileW (hdrop, i, wfn, MAX_PATH);
	  fileName = g_utf16_to_utf8 (wfn, -1, NULL, NULL, NULL);

	  /* Resolve shortcuts */
	  if (resolve_link (msg->hwnd, wfn, &linkedFile))
	    {
	      uri = g_filename_to_uri (linkedFile, NULL, NULL);
	      if (uri != NULL)
		{
		  g_string_append (result, uri);
		  CDK_NOTE (DND, g_print ("... %s link to %s: %s\n",
					  fileName, linkedFile, uri));
		  g_free (uri);
		}
	      g_free (fileName);
	      fileName = linkedFile;
	    }
	  else
	    {
	      uri = g_filename_to_uri (fileName, NULL, NULL);
	      if (uri != NULL)
		{
		  g_string_append (result, uri);
		  CDK_NOTE (DND, g_print ("... %s: %s\n", fileName, uri));
		  g_free (uri);
		}
	    }

#if 0
	  /* Awful hack to recognize temp files corresponding to
	   * images dragged from Firefox... Open the file right here
	   * so that it is less likely that Firefox manages to delete
	   * it before the CTK+-using app (typically GIMP) has opened
	   * it.
	   *
	   * Not compiled in for now, because it means images dragged
	   * from Firefox would stay around in the temp folder which
	   * is not what Firefox intended. I don't feel comfortable
	   * with that, both from a geenral sanity point of view, and
	   * from a privacy point of view. It's better to wait for
	   * Firefox to fix the problem, for instance by deleting the
	   * temp file after a longer delay, or to wait until we
	   * implement the OLE2_DND...
	   */
	  if (filename_looks_tempish (fileName))
	    {
	      int fd = g_open (fileName, _O_RDONLY|_O_BINARY, 0);
	      if (fd == -1)
		{
		  CDK_NOTE (DND, g_print ("Could not open %s, maybe an image dragged from Firefox that it already deleted\n", fileName));
		}
	      else
		{
		  CDK_NOTE (DND, g_print ("Opened %s as %d so that Firefox won't delete it\n", fileName, fd));
		  g_timeout_add_seconds (1, close_it, GINT_TO_POINTER (fd));
		}
	    }
#endif

	  g_free (fileName);
	  g_string_append (result, "\015\012");
	}
      _cdk_dropfiles_store (result->str);
      g_string_free (result, FALSE);

      DragFinish (hdrop);

      return CDK_FILTER_TRANSLATE;
    }
  else
    return CDK_FILTER_CONTINUE;
}



void
_cdk_dnd_init (void)
{
  CoInitializeEx (NULL, COINIT_APARTMENTTHREADED);

  if (getenv ("CDK_WIN32_USE_EXPERIMENTAL_OLE2_DND"))
    use_ole2_dnd = TRUE;

  if (use_ole2_dnd)
    {
      HRESULT hr;

      hr = OleInitialize (NULL);

      if (! SUCCEEDED (hr))
	g_error ("OleInitialize failed");

      target_ctx_for_window = g_hash_table_new (g_direct_hash, g_direct_equal);
    }
}

void
_cdk_win32_dnd_exit (void)
{
  if (use_ole2_dnd)
    {
      OleUninitialize ();
    }

  CoUninitialize ();
}

/* Source side */

static void
local_send_leave (CdkDragContext *context,
		  guint32         time)
{
  CdkEvent *tmp_event;

  CDK_NOTE (DND, g_print ("local_send_leave: context=%p current_dest_drag=%p\n",
			  context,
			  current_dest_drag));

  if ((current_dest_drag != NULL) &&
      (current_dest_drag->protocol == CDK_DRAG_PROTO_LOCAL) &&
      (current_dest_drag->source_window == context->source_window))
    {
      tmp_event = cdk_event_new (CDK_DRAG_LEAVE);

      g_set_object (&tmp_event->dnd.window, context->dest_window);
      /* Pass ownership of context to the event */
      tmp_event->dnd.send_event = FALSE;
      g_set_object (&tmp_event->dnd.context, current_dest_drag);
      tmp_event->dnd.time = CDK_CURRENT_TIME; /* FIXME? */
      cdk_event_set_device (tmp_event, cdk_drag_context_get_device (context));
      cdk_event_set_seat (tmp_event, cdk_device_get_seat (cdk_drag_context_get_device (context)));

      current_dest_drag = NULL;

      CDK_NOTE (EVENTS, _cdk_win32_print_event (tmp_event));
      cdk_event_put (tmp_event);
      cdk_event_free (tmp_event);
    }
}

static void
local_send_enter (CdkDragContext *context,
		  guint32         time)
{
  CdkEvent *tmp_event;
  CdkDragContext *new_context;

  CDK_NOTE (DND, g_print ("local_send_enter: context=%p current_dest_drag=%p\n",
			  context,
			  current_dest_drag));

  if (current_dest_drag != NULL)
    {
      g_object_unref (G_OBJECT (current_dest_drag));
      current_dest_drag = NULL;
    }

  new_context = cdk_drag_context_new (cdk_window_get_display (context->source_window));
  new_context->protocol = CDK_DRAG_PROTO_LOCAL;
  new_context->is_source = FALSE;

  g_set_object (&new_context->source_window, context->source_window);
  g_set_object (&new_context->dest_window, context->dest_window);

  new_context->targets = g_list_copy (context->targets);

  cdk_window_set_events (new_context->source_window,
			 cdk_window_get_events (new_context->source_window) |
			 CDK_PROPERTY_CHANGE_MASK);
  new_context->actions = context->actions;

  tmp_event = cdk_event_new (CDK_DRAG_ENTER);
  g_set_object (&tmp_event->dnd.window, context->dest_window);
  tmp_event->dnd.send_event = FALSE;
  g_set_object (&tmp_event->dnd.context, new_context);
  tmp_event->dnd.time = CDK_CURRENT_TIME; /* FIXME? */
  cdk_event_set_device (tmp_event, cdk_drag_context_get_device (context));
  cdk_event_set_seat (tmp_event, cdk_device_get_seat (cdk_drag_context_get_device (context)));

  current_dest_drag = new_context;

  CDK_NOTE (EVENTS, _cdk_win32_print_event (tmp_event));
  cdk_event_put (tmp_event);
  cdk_event_free (tmp_event);
}

static void
local_send_motion (CdkDragContext *context,
		   gint            x_root,
		   gint            y_root,
		   CdkDragAction   action,
		   guint32         time)
{
  CdkEvent *tmp_event;
  CdkWin32DragContext *context_win32 = CDK_WIN32_DRAG_CONTEXT (context);

  CDK_NOTE (DND, g_print ("local_send_motion: context=%p (%d,%d) current_dest_drag=%p\n",
			  context, x_root, y_root,
			  current_dest_drag));

  if ((current_dest_drag != NULL) &&
      (current_dest_drag->protocol == CDK_DRAG_PROTO_LOCAL) &&
      (current_dest_drag->source_window == context->source_window))
    {
      CdkWin32DragContext *current_dest_drag_win32;

      tmp_event = cdk_event_new (CDK_DRAG_MOTION);
      g_set_object (&tmp_event->dnd.window, current_dest_drag->dest_window);
      tmp_event->dnd.send_event = FALSE;
      g_set_object (&tmp_event->dnd.context, current_dest_drag);
      tmp_event->dnd.time = time;
      cdk_event_set_device (tmp_event, cdk_drag_context_get_device (current_dest_drag));
      cdk_event_set_seat (tmp_event, cdk_device_get_seat (cdk_drag_context_get_device (current_dest_drag)));

      current_dest_drag->suggested_action = action;

      tmp_event->dnd.x_root = x_root;
      tmp_event->dnd.y_root = y_root;

      current_dest_drag_win32 = CDK_WIN32_DRAG_CONTEXT (current_dest_drag);
      current_dest_drag_win32->last_x = x_root;
      current_dest_drag_win32->last_y = y_root;

      context_win32->drag_status = CDK_DRAG_STATUS_MOTION_WAIT;

      CDK_NOTE (EVENTS, _cdk_win32_print_event (tmp_event));
      cdk_event_put (tmp_event);
      cdk_event_free (tmp_event);
    }
}

static void
local_send_drop (CdkDragContext *context,
		 guint32         time)
{
  CdkEvent *tmp_event;

  CDK_NOTE (DND, g_print ("local_send_drop: context=%p current_dest_drag=%p\n",
			  context,
			  current_dest_drag));

  if ((current_dest_drag != NULL) &&
      (current_dest_drag->protocol == CDK_DRAG_PROTO_LOCAL) &&
      (current_dest_drag->source_window == context->source_window))
    {
      CdkWin32DragContext *context_win32;

      /* Pass ownership of context to the event */
      tmp_event = cdk_event_new (CDK_DROP_START);
      g_set_object (&tmp_event->dnd.window, current_dest_drag->dest_window);
      tmp_event->dnd.send_event = FALSE;
      g_set_object (&tmp_event->dnd.context, current_dest_drag);
      tmp_event->dnd.time = CDK_CURRENT_TIME;
      cdk_event_set_device (tmp_event, cdk_drag_context_get_device (current_dest_drag));
      cdk_event_set_seat (tmp_event, cdk_device_get_seat (cdk_drag_context_get_device (current_dest_drag)));

      context_win32 = CDK_WIN32_DRAG_CONTEXT (current_dest_drag);
      tmp_event->dnd.x_root = context_win32->last_x;
      tmp_event->dnd.y_root = context_win32->last_y;

      current_dest_drag = NULL;

      CDK_NOTE (EVENTS, _cdk_win32_print_event (tmp_event));
      cdk_event_put (tmp_event);
      cdk_event_free (tmp_event);
    }

}

static void
cdk_drag_do_leave (CdkDragContext *context,
		   guint32         time)
{
  if (context->dest_window)
    {
      CDK_NOTE (DND, g_print ("cdk_drag_do_leave\n"));

      if (!use_ole2_dnd)
	{
	  if (context->protocol == CDK_DRAG_PROTO_LOCAL)
	    local_send_leave (context, time);
	}

      g_clear_object (&context->dest_window);
    }
}

static CdkWindow *
create_drag_window (CdkScreen *screen)
{
  CdkWindowAttr attrs = { 0 };
  guint mask;

  attrs.x = attrs.y = 0;
  attrs.width = attrs.height = 100;
  attrs.wclass = CDK_INPUT_OUTPUT;
  attrs.window_type = CDK_WINDOW_TEMP;
  attrs.type_hint = CDK_WINDOW_TYPE_HINT_DND;
  attrs.visual = cdk_screen_get_rgba_visual (screen);
  if (!attrs.visual)
    attrs.visual = cdk_screen_get_system_visual (screen);

  mask = CDK_WA_X | CDK_WA_Y | CDK_WA_VISUAL | CDK_WA_TYPE_HINT;

  return cdk_window_new (cdk_screen_get_root_window (screen), &attrs, mask);
}

CdkDragContext *
_cdk_win32_window_drag_begin (CdkWindow *window,
			      CdkDevice *device,
			      GList     *targets,
                              gint       x_root,
                              gint       y_root)
{
  CdkDragContext *new_context;
  CdkWin32DragContext *context_win32;
  BYTE kbd_state[256];
  CdkWin32Selection *sel_win32 = _cdk_win32_selection_get ();

  if (!use_ole2_dnd)
    {
      g_return_val_if_fail (window != NULL, NULL);

      new_context = cdk_drag_context_new (cdk_window_get_display (window));

      new_context->is_source = TRUE;
      g_set_object (&new_context->source_window, window);

      new_context->targets = g_list_copy (targets);
      new_context->actions = 0;
      context_win32 = CDK_WIN32_DRAG_CONTEXT (new_context);
    }
  else
    {
      source_drag_context *ctx;

      g_return_val_if_fail (window != NULL, NULL);

      CDK_NOTE (DND, g_print ("cdk_drag_begin\n"));

      ctx = source_context_new (window, targets);

      sel_win32->dnd_source_state = CDK_WIN32_DND_PENDING;

      pending_src_context = ctx;
      new_context = g_object_ref (ctx->context);

      context_win32 = CDK_WIN32_DRAG_CONTEXT (new_context);
    }

  context_win32->start_x = x_root;
  context_win32->start_y = y_root;
  context_win32->last_x = context_win32->start_x;
  context_win32->last_y = context_win32->start_y;

  context_win32->last_key_state = 0;
  API_CALL (GetKeyboardState, (kbd_state));

  if (kbd_state[VK_MENU] & 0x80)
    context_win32->last_key_state |= MK_ALT;
  if (kbd_state[VK_CONTROL] & 0x80)
    context_win32->last_key_state |= MK_CONTROL;
  if (kbd_state[VK_SHIFT] & 0x80)
    context_win32->last_key_state |= MK_SHIFT;
  if (kbd_state[VK_LBUTTON] & 0x80)
    context_win32->last_key_state |= MK_LBUTTON;
  if (kbd_state[VK_MBUTTON] & 0x80)
    context_win32->last_key_state |= MK_MBUTTON;
  if (kbd_state[VK_RBUTTON] & 0x80)
    context_win32->last_key_state |= MK_RBUTTON;

  context_win32->drag_window = create_drag_window (cdk_window_get_screen (window));

  return new_context;
}

void
_cdk_win32_dnd_do_dragdrop (void)
{
  CdkDragContext* drag_ctx;
  data_object *dobj;
  HRESULT hr;
  DWORD dwEffect;

  if (!use_ole2_dnd)
    return;

  if (pending_src_context == NULL)
    return;

  drag_ctx = pending_src_context->context;

  dobj = data_object_new (drag_ctx);
  current_src_object = dobj;

  /* Start dragging with mainloop inside the OLE2 API. Exits only when done */

  CDK_NOTE (DND, g_print ("Calling DoDragDrop\n"));

  _cdk_win32_begin_modal_call (CDK_WIN32_MODAL_OP_DND);
  hr = DoDragDrop (&dobj->ido, &pending_src_context->ids,
    	       DROPEFFECT_COPY | DROPEFFECT_MOVE,
    	       &dwEffect);
  _cdk_win32_end_modal_call (CDK_WIN32_MODAL_OP_DND);

  CDK_NOTE (DND, g_print ("DoDragDrop returned %s\n",
    		      (hr == DRAGDROP_S_DROP ? "DRAGDROP_S_DROP" :
    		       (hr == DRAGDROP_S_CANCEL ? "DRAGDROP_S_CANCEL" :
    			(hr == E_UNEXPECTED ? "E_UNEXPECTED" :
    			 g_strdup_printf ("%#.8lx", hr))))));

  /* Delete dnd selection after successful move */
  if (hr == DRAGDROP_S_DROP && dwEffect == DROPEFFECT_MOVE)
    {
      CdkWin32Selection *win32_sel = _cdk_win32_selection_get ();
      CdkEvent tmp_event;

      memset (&tmp_event, 0, sizeof (tmp_event));
      tmp_event.type = CDK_SELECTION_REQUEST;
      g_set_object (&tmp_event.selection.window, drag_ctx->source_window);
      tmp_event.selection.send_event = FALSE;
      tmp_event.selection.selection = _cdk_win32_selection_atom (CDK_WIN32_ATOM_INDEX_OLE2_DND);
      tmp_event.selection.target = _cdk_win32_selection_atom (CDK_WIN32_ATOM_INDEX_DELETE);
      win32_sel->property_change_target_atom = _cdk_win32_selection_atom (CDK_WIN32_ATOM_INDEX_DELETE);
      tmp_event.selection.property = _cdk_win32_selection_atom (CDK_WIN32_ATOM_INDEX_OLE2_DND);
      g_set_object (&tmp_event.selection.requestor, drag_ctx->source_window);
      tmp_event.selection.time = CDK_CURRENT_TIME; /* ??? */

      CDK_NOTE (EVENTS, _cdk_win32_print_event (&tmp_event));
      cdk_event_put (&tmp_event);
    }

  {
    CdkEvent *tmp_event;
    tmp_event = cdk_event_new (CDK_DROP_FINISHED);
    g_set_object (&tmp_event->dnd.window, drag_ctx->source_window);
    tmp_event->dnd.send_event = FALSE;
    g_set_object (&tmp_event->dnd.context, drag_ctx);
    cdk_event_set_device (tmp_event, cdk_drag_context_get_device (drag_ctx));
    cdk_event_set_seat (tmp_event, cdk_device_get_seat (cdk_drag_context_get_device (drag_ctx)));

    CDK_NOTE (EVENTS, _cdk_win32_print_event (tmp_event));
    cdk_event_put (tmp_event);
    cdk_event_free (tmp_event);
  }

  current_src_object = NULL;
  dobj->ido.lpVtbl->Release (&dobj->ido);
  if (pending_src_context != NULL)
    {
      pending_src_context->ids.lpVtbl->Release (&pending_src_context->ids);
      pending_src_context = NULL;
    }
}

/* Untested, may not work ...
 * ... but as of this writing is only used by exlusive X11 ctksocket.c
 */
CdkDragProtocol
_cdk_win32_window_get_drag_protocol (CdkWindow *window,
                                     CdkWindow **target)
{
  CdkDragProtocol protocol = CDK_DRAG_PROTO_NONE;

  if (cdk_window_get_window_type (window) != CDK_WINDOW_FOREIGN)
    {
      if (g_object_get_data (G_OBJECT (window), "cdk-dnd-registered") != NULL)
	{
	  if (use_ole2_dnd)
	    protocol = CDK_DRAG_PROTO_OLE2;
	  else
	    protocol = CDK_DRAG_PROTO_LOCAL;
	}
    }

  if (target)
    {
      *target = NULL;
    }

  return protocol;
}

typedef struct {
  gint x;
  gint y;
  HWND ignore;
  HWND result;
} find_window_enum_arg;

static BOOL CALLBACK
find_window_enum_proc (HWND   hwnd,
                       LPARAM lparam)
{
  RECT rect;
  POINT tl, br;
  find_window_enum_arg *a = (find_window_enum_arg *) lparam;

  if (hwnd == a->ignore)
    return TRUE;

  if (!IsWindowVisible (hwnd))
    return TRUE;

  tl.x = tl.y = 0;
  ClientToScreen (hwnd, &tl);
  GetClientRect (hwnd, &rect);
  br.x = rect.right;
  br.y = rect.bottom;
  ClientToScreen (hwnd, &br);

  if (a->x >= tl.x && a->y >= tl.y && a->x < br.x && a->y < br.y)
    {
      a->result = hwnd;
      return FALSE;
    }
  else
    return TRUE;
}

static CdkWindow *
cdk_win32_drag_context_find_window (CdkDragContext  *context,
				    CdkWindow       *drag_window,
				    CdkScreen       *screen,
				    gint             x_root,
				    gint             y_root,
				    CdkDragProtocol *protocol)
{
  CdkWin32DragContext *context_win32 = CDK_WIN32_DRAG_CONTEXT (context);
  CdkWindow *dest_window, *dw;
  find_window_enum_arg a;

  a.x = x_root * context_win32->scale - _cdk_offset_x;
  a.y = y_root * context_win32->scale - _cdk_offset_y;
  a.ignore = drag_window ? CDK_WINDOW_HWND (drag_window) : NULL;
  a.result = NULL;

  CDK_NOTE (DND,
	    g_print ("cdk_drag_find_window_real: %p %+d%+d\n",
		     (drag_window ? CDK_WINDOW_HWND (drag_window) : NULL),
		     a.x, a.y));

  EnumWindows (find_window_enum_proc, (LPARAM) &a);

  if (a.result == NULL)
    dest_window = NULL;
  else
    {
      dw = cdk_win32_handle_table_lookup (a.result);
      if (dw)
        {
          dest_window = cdk_window_get_toplevel (dw);
          g_object_ref (dest_window);
        }
      else
        dest_window = cdk_win32_window_foreign_new_for_display (cdk_screen_get_display (screen), a.result);

      if (use_ole2_dnd)
        *protocol = CDK_DRAG_PROTO_OLE2;
      else if (context->source_window)
        *protocol = CDK_DRAG_PROTO_LOCAL;
      else
        *protocol = CDK_DRAG_PROTO_WIN32_DROPFILES;
    }

  CDK_NOTE (DND,
	    g_print ("cdk_drag_find_window: %p %+d%+d: %p: %p %s\n",
		     (drag_window ? CDK_WINDOW_HWND (drag_window) : NULL),
		     x_root, y_root,
		     a.result,
		     (dest_window ? CDK_WINDOW_HWND (dest_window) : NULL),
		     _cdk_win32_drag_protocol_to_string (*protocol)));

  return dest_window;
}

static gboolean
cdk_win32_drag_context_drag_motion (CdkDragContext *context,
		 CdkWindow      *dest_window,
		 CdkDragProtocol protocol,
		 gint            x_root,
		 gint            y_root,
		 CdkDragAction   suggested_action,
		 CdkDragAction   possible_actions,
		 guint32         time)
{
  CdkWin32DragContext *context_win32;

  g_return_val_if_fail (context != NULL, FALSE);

  context->actions = possible_actions;

  CDK_NOTE (DND, g_print ("cdk_drag_motion: @ %+d:%+d %s suggested=%s, possible=%s\n"
			  " context=%p:{actions=%s,suggested=%s,action=%s}\n",
			  x_root, y_root,
			  _cdk_win32_drag_protocol_to_string (protocol),
			  _cdk_win32_drag_action_to_string (suggested_action),
			  _cdk_win32_drag_action_to_string (possible_actions),
			  context,
			  _cdk_win32_drag_action_to_string (context->actions),
			  _cdk_win32_drag_action_to_string (context->suggested_action),
			  _cdk_win32_drag_action_to_string (context->action)));

  context_win32 = CDK_WIN32_DRAG_CONTEXT (context);

  if (context_win32->drag_window)
    move_drag_window (context, x_root, y_root);

  if (!use_ole2_dnd)
    {
      if (context->dest_window == dest_window)
	{
	  CdkDragContext *dest_context;

	  dest_context = cdk_drag_context_find (FALSE,
						context->source_window,
						dest_window);

	  if (dest_context)
	    dest_context->actions = context->actions;

	  context->suggested_action = suggested_action;
	}
      else
	{
	  CdkEvent *tmp_event;

	  /* Send a leave to the last destination */
	  cdk_drag_do_leave (context, time);
	  context_win32->drag_status = CDK_DRAG_STATUS_DRAG;

	  /* Check if new destination accepts drags, and which protocol */
	  if (dest_window)
	    {
	      g_set_object (&context->dest_window, dest_window);
	      context->protocol = protocol;

	      switch (protocol)
		{
		case CDK_DRAG_PROTO_LOCAL:
		  local_send_enter (context, time);
		  break;

		default:
		  break;
		}
	      context->suggested_action = suggested_action;
	    }
	  else
	    {
	      context->dest_window = NULL;
	      context->action = 0;
	    }

	  /* Push a status event, to let the client know that
	   * the drag changed
	   */
	  tmp_event = cdk_event_new (CDK_DRAG_STATUS);
	  g_set_object (&tmp_event->dnd.window, context->source_window);
	  /* We use this to signal a synthetic status. Perhaps
	   * we should use an extra field...
	   */
	  tmp_event->dnd.send_event = TRUE;

	  g_set_object (&tmp_event->dnd.context, context);
	  tmp_event->dnd.time = time;
	  cdk_event_set_device (tmp_event, cdk_drag_context_get_device (context));
	  cdk_event_set_seat (tmp_event, cdk_device_get_seat (cdk_drag_context_get_device (context)));

	  CDK_NOTE (EVENTS, _cdk_win32_print_event (tmp_event));
	  cdk_event_put (tmp_event);
	  cdk_event_free (tmp_event);
	}

      /* Send a drag-motion event */

      context_win32->last_x = x_root;
      context_win32->last_y = y_root;

      if (context->dest_window)
	{
	  if (context_win32->drag_status == CDK_DRAG_STATUS_DRAG)
	    {
	      switch (context->protocol)
		{
		case CDK_DRAG_PROTO_LOCAL:
		  local_send_motion (context, x_root, y_root, suggested_action, time);
		  break;

		case CDK_DRAG_PROTO_NONE:
		  g_warning ("CDK_DRAG_PROTO_NONE is not valid in cdk_drag_motion()");
		  break;

		default:
		  break;
		}
	    }
	  else
	    {
	      CDK_NOTE (DND, g_print (" returning TRUE\n"
				      " context=%p:{actions=%s,suggested=%s,action=%s}\n",
				      context,
				      _cdk_win32_drag_action_to_string (context->actions),
				      _cdk_win32_drag_action_to_string (context->suggested_action),
				      _cdk_win32_drag_action_to_string (context->action)));
	      return TRUE;
	    }
	}
    }

  CDK_NOTE (DND, g_print (" returning FALSE\n"
			  " context=%p:{actions=%s,suggested=%s,action=%s}\n",
			  context,
			  _cdk_win32_drag_action_to_string (context->actions),
			  _cdk_win32_drag_action_to_string (context->suggested_action),
			  _cdk_win32_drag_action_to_string (context->action)));
  return FALSE;
}

static void
cdk_win32_drag_context_drag_drop (CdkDragContext *context,
	       guint32         time)
{
  CdkWin32Selection *sel_win32 = _cdk_win32_selection_get ();

  g_return_if_fail (context != NULL);

  CDK_NOTE (DND, g_print ("cdk_drag_drop\n"));

  if (!use_ole2_dnd)
    {
      if (context->dest_window &&
	  context->protocol == CDK_DRAG_PROTO_LOCAL)
	local_send_drop (context, time);
    }
  else
    {
      sel_win32->dnd_source_state = CDK_WIN32_DND_DROPPED;
    }
}

static void
cdk_win32_drag_context_drag_abort (CdkDragContext *context,
		guint32         time)
{
  CdkWin32Selection *sel_win32 = _cdk_win32_selection_get ();

  g_return_if_fail (context != NULL);

  CDK_NOTE (DND, g_print ("cdk_drag_abort\n"));

  if (use_ole2_dnd)
    sel_win32->dnd_source_state = CDK_WIN32_DND_NONE;
}

/* Destination side */

static void
cdk_win32_drag_context_drag_status (CdkDragContext *context,
		 CdkDragAction   action,
		 guint32         time)
{
  CdkDragContext *src_context;
  CdkEvent *tmp_event;

  g_return_if_fail (context != NULL);

  CDK_NOTE (DND, g_print ("cdk_drag_status: %s\n"
			  " context=%p:{actions=%s,suggested=%s,action=%s}\n",
			  _cdk_win32_drag_action_to_string (action),
			  context,
			  _cdk_win32_drag_action_to_string (context->actions),
			  _cdk_win32_drag_action_to_string (context->suggested_action),
			  _cdk_win32_drag_action_to_string (context->action)));

  context->action = action;

  if (!use_ole2_dnd)
    {
      src_context = cdk_drag_context_find (TRUE,
					   context->source_window,
					   context->dest_window);

      if (src_context)
	{
	  CdkWin32DragContext *src_context_win32 = CDK_WIN32_DRAG_CONTEXT (src_context);

	  if (src_context_win32->drag_status == CDK_DRAG_STATUS_MOTION_WAIT)
	    src_context_win32->drag_status = CDK_DRAG_STATUS_DRAG;

	  tmp_event = cdk_event_new (CDK_DRAG_STATUS);
	  g_set_object (&tmp_event->dnd.window, context->source_window);
	  tmp_event->dnd.send_event = FALSE;
	  g_set_object (&tmp_event->dnd.context, src_context);
	  tmp_event->dnd.time = CDK_CURRENT_TIME; /* FIXME? */
	  cdk_event_set_device (tmp_event, cdk_drag_context_get_device (src_context));
	  cdk_event_set_seat (tmp_event, cdk_device_get_seat (cdk_drag_context_get_device (src_context)));

	  if (action == CDK_ACTION_DEFAULT)
	    action = 0;

	  src_context->action = action;

	  CDK_NOTE (EVENTS, _cdk_win32_print_event (tmp_event));
	  cdk_event_put (tmp_event);
	  cdk_event_free (tmp_event);
	}
    }
}

static void
cdk_win32_drag_context_drop_reply (CdkDragContext *context,
		gboolean        ok,
		guint32         time)
{
  g_return_if_fail (context != NULL);

  CDK_NOTE (DND, g_print ("cdk_drop_reply\n"));

  if (!use_ole2_dnd)
    if (context->dest_window)
      {
	if (context->protocol == CDK_DRAG_PROTO_WIN32_DROPFILES)
	  _cdk_dropfiles_store (NULL);
      }
}

static void
cdk_win32_drag_context_drop_finish (CdkDragContext *context,
		 gboolean        success,
		 guint32         time)
{
  CdkDragContext *src_context;
  CdkEvent *tmp_event;
  CdkWin32Selection *sel_win32 = _cdk_win32_selection_get ();

  g_return_if_fail (context != NULL);

  CDK_NOTE (DND, g_print ("cdk_drop_finish\n"));

  if (!use_ole2_dnd)
    {
      src_context = cdk_drag_context_find (TRUE,
					   context->source_window,
					   context->dest_window);
      if (src_context)
	{
          if (cdk_drag_context_get_selected_action (src_context) == CDK_ACTION_MOVE)
            {
              tmp_event = cdk_event_new (CDK_SELECTION_REQUEST);
              g_set_object (&tmp_event->selection.window, src_context->source_window);
              tmp_event->selection.send_event = FALSE;
              tmp_event->selection.selection = _cdk_win32_selection_atom (CDK_WIN32_ATOM_INDEX_LOCAL_DND_SELECTION);
              tmp_event->selection.target = _cdk_win32_selection_atom (CDK_WIN32_ATOM_INDEX_DELETE);
              sel_win32->property_change_target_atom = _cdk_win32_selection_atom (CDK_WIN32_ATOM_INDEX_DELETE);
              tmp_event->selection.property = _cdk_win32_selection_atom (CDK_WIN32_ATOM_INDEX_LOCAL_DND_SELECTION);
              g_set_object (&tmp_event->selection.requestor, src_context->source_window);
              tmp_event->selection.time = CDK_CURRENT_TIME; /* ??? */

              CDK_NOTE (EVENTS, _cdk_win32_print_event (tmp_event));
              cdk_event_put (tmp_event);
              cdk_event_free (tmp_event);
            }

	  tmp_event = cdk_event_new (CDK_DROP_FINISHED);
	  g_set_object (&tmp_event->dnd.window, src_context->source_window);
	  tmp_event->dnd.send_event = FALSE;
	  g_set_object (&tmp_event->dnd.context, src_context);
	  cdk_event_set_device (tmp_event, cdk_drag_context_get_device (src_context));
	  cdk_event_set_seat (tmp_event, cdk_device_get_seat (cdk_drag_context_get_device (src_context)));

	  CDK_NOTE (EVENTS, _cdk_win32_print_event (tmp_event));
	  cdk_event_put (tmp_event);
	  cdk_event_free (tmp_event);
	}
    }
  else
    {
      cdk_drag_do_leave (context, time);

      if (success)
	sel_win32->dnd_target_state = CDK_WIN32_DND_DROPPED;
      else
	sel_win32->dnd_target_state = CDK_WIN32_DND_FAILED;
    }
}

#if 0

static CdkFilterReturn
cdk_destroy_filter (CdkXEvent *xev,
		    CdkEvent  *event,
		    gpointer   data)
{
  MSG *msg = (MSG *) xev;

  if (msg->message == WM_DESTROY)
    {
      IDropTarget *idtp = (IDropTarget *) data;

      CDK_NOTE (DND, g_print ("cdk_destroy_filter: WM_DESTROY: %p\n", msg->hwnd));
#if 0
      idtp->lpVtbl->Release (idtp);
#endif
      RevokeDragDrop (msg->hwnd);
      CoLockObjectExternal ((IUnknown*) idtp, FALSE, TRUE);
    }
  return CDK_FILTER_CONTINUE;
}

#endif

void
_cdk_win32_window_register_dnd (CdkWindow *window)
{
  target_drag_context *ctx;
  HRESULT hr;

  g_return_if_fail (window != NULL);

  if (cdk_window_get_window_type (window) == CDK_WINDOW_OFFSCREEN)
    return;

  if (g_object_get_data (G_OBJECT (window), "cdk-dnd-registered") != NULL)
    return;
  else
    g_object_set_data (G_OBJECT (window), "cdk-dnd-registered", GINT_TO_POINTER (TRUE));

  CDK_NOTE (DND, g_print ("cdk_window_register_dnd: %p\n", CDK_WINDOW_HWND (window)));

  if (!use_ole2_dnd)
    {
      /* We always claim to accept dropped files, but in fact we might not,
       * of course. This function is called in such a way that it cannot know
       * whether the window (widget) in question actually accepts files
       * (in ctk, data of type text/uri-list) or not.
       */
      cdk_window_add_filter (window, cdk_dropfiles_filter, NULL);
      DragAcceptFiles (CDK_WINDOW_HWND (window), TRUE);
    }
  else
    {
      /* Return if window is already setup for DND. */
      if (g_hash_table_lookup (target_ctx_for_window, CDK_WINDOW_HWND (window)) != NULL)
	return;

      ctx = target_context_new (window);

      hr = CoLockObjectExternal ((IUnknown *) &ctx->idt, TRUE, FALSE);
      if (!SUCCEEDED (hr))
	OTHER_API_FAILED ("CoLockObjectExternal");
      else
	{
	  hr = RegisterDragDrop (CDK_WINDOW_HWND (window), &ctx->idt);
	  if (hr == DRAGDROP_E_ALREADYREGISTERED)
	    {
	      g_print ("DRAGDROP_E_ALREADYREGISTERED\n");
	      CoLockObjectExternal ((IUnknown *) &ctx->idt, FALSE, FALSE);
	    }
	  else if (!SUCCEEDED (hr))
	    OTHER_API_FAILED ("RegisterDragDrop");
	  else
	    {
	      g_object_ref (window);
	      g_hash_table_insert (target_ctx_for_window, CDK_WINDOW_HWND (window), ctx);
	    }
	}
    }
}

static gboolean
cdk_win32_drag_context_drop_status (CdkDragContext *context)
{
  CdkWin32DragContext *context_win32 = CDK_WIN32_DRAG_CONTEXT (context);

  return ! context_win32->drop_failed;
}

static CdkAtom
cdk_win32_drag_context_get_selection (CdkDragContext *context)
{
  switch (context->protocol)
    {
    case CDK_DRAG_PROTO_LOCAL:
      return _cdk_win32_selection_atom (CDK_WIN32_ATOM_INDEX_LOCAL_DND_SELECTION);
    case CDK_DRAG_PROTO_WIN32_DROPFILES:
      return _cdk_win32_selection_atom (CDK_WIN32_ATOM_INDEX_DROPFILES_DND);
    case CDK_DRAG_PROTO_OLE2:
      return _cdk_win32_selection_atom (CDK_WIN32_ATOM_INDEX_OLE2_DND);
    default:
      return CDK_NONE;
    }
}

static void
cdk_win32_drag_context_set_cursor (CdkDragContext *context,
                                   CdkCursor      *cursor)
{
  CdkWin32DragContext *context_win32 = CDK_WIN32_DRAG_CONTEXT (context);

  CDK_NOTE (DND, g_print ("cdk_drag_context_set_cursor: 0x%p 0x%p\n", context, cursor));

  if (!g_set_object (&context_win32->cursor, cursor))
    return;

  if (context_win32->grab_seat)
    {
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      cdk_device_grab (cdk_seat_get_pointer (context_win32->grab_seat),
                       context_win32->ipc_window,
                       CDK_OWNERSHIP_APPLICATION, FALSE,
                       CDK_POINTER_MOTION_MASK | CDK_BUTTON_RELEASE_MASK,
                       cursor, CDK_CURRENT_TIME);
      G_GNUC_END_IGNORE_DEPRECATIONS;
    }
}

static double
ease_out_cubic (double t)
{
  double p = t - 1;
  return p * p * p + 1;
}

#define ANIM_TIME 500000 /* half a second */

typedef struct _CdkDragAnim CdkDragAnim;
struct _CdkDragAnim {
  CdkWin32DragContext *context;
  CdkFrameClock *frame_clock;
  gint64 start_time;
};

static void
cdk_drag_anim_destroy (CdkDragAnim *anim)
{
  g_object_unref (anim->context);
  g_slice_free (CdkDragAnim, anim);
}

static gboolean
cdk_drag_anim_timeout (gpointer data)
{
  CdkDragAnim *anim = data;
  CdkWin32DragContext *context = anim->context;
  CdkFrameClock *frame_clock = anim->frame_clock;
  gint64 current_time;
  double f;
  double t;

  if (!frame_clock)
    return G_SOURCE_REMOVE;

  current_time = cdk_frame_clock_get_frame_time (frame_clock);

  f = (current_time - anim->start_time) / (double) ANIM_TIME;

  if (f >= 1.0)
    return G_SOURCE_REMOVE;

  t = ease_out_cubic (f);

  cdk_window_show (context->drag_window);
  cdk_window_move (context->drag_window,
                   context->last_x + (context->start_x - context->last_x) * t - context->hot_x,
                   context->last_y + (context->start_y - context->last_y) * t - context->hot_y);
  cdk_window_set_opacity (context->drag_window, 1.0 - f);

  return G_SOURCE_CONTINUE;
}

static void
cdk_win32_drag_context_drop_done (CdkDragContext *context,
                                  gboolean        success)
{
  CdkWin32DragContext *win32_context = CDK_WIN32_DRAG_CONTEXT (context);
  CdkDragAnim *anim;
  cairo_surface_t *win_surface;
  cairo_surface_t *surface;
  cairo_pattern_t *pattern;
  cairo_t *cr;

  CDK_NOTE (DND, g_print ("cdk_drag_context_drop_done: 0x%p %s\n",
                          context,
                          success ? "dropped successfully" : "dropped unsuccessfully"));

  if (success)
    {
      cdk_window_hide (win32_context->drag_window);
      return;
    }

  win_surface = _cdk_window_ref_cairo_surface (win32_context->drag_window);
  surface = cdk_window_create_similar_surface (win32_context->drag_window,
                                               cairo_surface_get_content (win_surface),
                                               cdk_window_get_width (win32_context->drag_window),
                                               cdk_window_get_height (win32_context->drag_window));
  cr = cairo_create (surface);
  cairo_set_source_surface (cr, win_surface, 0, 0);
  cairo_paint (cr);
  cairo_destroy (cr);
  cairo_surface_destroy (win_surface);

  pattern = cairo_pattern_create_for_surface (surface);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  cdk_window_set_background_pattern (win32_context->drag_window, pattern);
G_GNUC_END_IGNORE_DEPRECATIONS

  cairo_pattern_destroy (pattern);
  cairo_surface_destroy (surface);

  anim = g_slice_new0 (CdkDragAnim);
  g_set_object (&anim->context, win32_context);
  anim->frame_clock = cdk_window_get_frame_clock (win32_context->drag_window);
  anim->start_time = cdk_frame_clock_get_frame_time (anim->frame_clock);

  cdk_threads_add_timeout_full (G_PRIORITY_DEFAULT, 17,
                                cdk_drag_anim_timeout, anim,
                                (GDestroyNotify) cdk_drag_anim_destroy);
}

static gboolean
drag_context_grab (CdkDragContext *context)
{
  CdkWin32DragContext *context_win32 = CDK_WIN32_DRAG_CONTEXT (context);
  CdkSeatCapabilities capabilities;
  CdkSeat *seat;
  CdkCursor *cursor;

  if (!context_win32->ipc_window)
    return FALSE;

  seat = cdk_device_get_seat (cdk_drag_context_get_device (context));

  capabilities = CDK_SEAT_CAPABILITY_ALL;

  cursor = cdk_drag_get_cursor (context, cdk_drag_context_get_selected_action (context));
  g_set_object (&context_win32->cursor, cursor);

  if (cdk_seat_grab (seat, context_win32->ipc_window,
                     capabilities, FALSE,
                     context_win32->cursor, NULL, NULL, NULL) != CDK_GRAB_SUCCESS)
    return FALSE;

  g_set_object (&context_win32->grab_seat, seat);

  /* TODO: Should be grabbing keys here, to support keynav. SetWindowsHookEx()? */

  return TRUE;
}

static void
drag_context_ungrab (CdkDragContext *context)
{
  CdkWin32DragContext *context_win32 = CDK_WIN32_DRAG_CONTEXT (context);

  if (!context_win32->grab_seat)
    return;

  cdk_seat_ungrab (context_win32->grab_seat);

  g_clear_object (&context_win32->grab_seat);

  /* TODO: Should be ungrabbing keys here */
}

static gboolean
cdk_win32_drag_context_manage_dnd (CdkDragContext *context,
                                   CdkWindow      *ipc_window,
                                   CdkDragAction   actions)
{
  CdkWin32DragContext *context_win32 = CDK_WIN32_DRAG_CONTEXT (context);

  if (context_win32->ipc_window)
    return FALSE;

  if (use_ole2_dnd)
    context->protocol = CDK_DRAG_PROTO_OLE2;
  else
    context->protocol = CDK_DRAG_PROTO_LOCAL;

  g_set_object (&context_win32->ipc_window, ipc_window);

  if (drag_context_grab (context))
    {
      context_win32->actions = actions;
      move_drag_window (context, context_win32->start_x, context_win32->start_y);
      return TRUE;
    }
  else
    {
      g_clear_object (&context_win32->ipc_window);
      return FALSE;
    }
}

static void
cdk_win32_drag_context_cancel (CdkDragContext      *context,
                               CdkDragCancelReason  reason)
{
  const gchar *reason_str = NULL;
  switch (reason)
    {
    case CDK_DRAG_CANCEL_NO_TARGET:
      reason_str = "no target";
      break;
    case CDK_DRAG_CANCEL_USER_CANCELLED:
      reason_str = "user cancelled";
      break;
    case CDK_DRAG_CANCEL_ERROR:
      reason_str = "error";
      break;
    default:
      reason_str = "<unknown>";
      break;
    }

  CDK_NOTE (DND, g_print ("cdk_drag_context_cancel: 0x%p %s\n",
                          context,
                          reason_str));
  drag_context_ungrab (context);
  cdk_drag_drop_done (context, FALSE);
}

static void
cdk_win32_drag_context_drop_performed (CdkDragContext *context,
                                       guint32         time_)
{
  CDK_NOTE (DND, g_print ("cdk_drag_context_drop_performed: 0x%p %u\n",
                          context,
                          time_));
  cdk_drag_drop (context, time_);
  drag_context_ungrab (context);
}

#define BIG_STEP 20
#define SMALL_STEP 1

static void
cdk_drag_get_current_actions (CdkModifierType  state,
                              gint             button,
                              CdkDragAction    actions,
                              CdkDragAction   *suggested_action,
                              CdkDragAction   *possible_actions)
{
  *suggested_action = 0;
  *possible_actions = 0;

  if ((button == CDK_BUTTON_MIDDLE || button == CDK_BUTTON_SECONDARY) && (actions & CDK_ACTION_ASK))
    {
      *suggested_action = CDK_ACTION_ASK;
      *possible_actions = actions;
    }
  else if (state & (CDK_SHIFT_MASK | CDK_CONTROL_MASK))
    {
      if ((state & CDK_SHIFT_MASK) && (state & CDK_CONTROL_MASK))
        {
          if (actions & CDK_ACTION_LINK)
            {
              *suggested_action = CDK_ACTION_LINK;
              *possible_actions = CDK_ACTION_LINK;
            }
        }
      else if (state & CDK_CONTROL_MASK)
        {
          if (actions & CDK_ACTION_COPY)
            {
              *suggested_action = CDK_ACTION_COPY;
              *possible_actions = CDK_ACTION_COPY;
            }
        }
      else
        {
          if (actions & CDK_ACTION_MOVE)
            {
              *suggested_action = CDK_ACTION_MOVE;
              *possible_actions = CDK_ACTION_MOVE;
            }
        }
    }
  else
    {
      *possible_actions = actions;

      if ((state & (CDK_MOD1_MASK)) && (actions & CDK_ACTION_ASK))
        *suggested_action = CDK_ACTION_ASK;
      else if (actions & CDK_ACTION_COPY)
        *suggested_action =  CDK_ACTION_COPY;
      else if (actions & CDK_ACTION_MOVE)
        *suggested_action = CDK_ACTION_MOVE;
      else if (actions & CDK_ACTION_LINK)
        *suggested_action = CDK_ACTION_LINK;
    }
}

static void
cdk_drag_update (CdkDragContext  *context,
                 gdouble          x_root,
                 gdouble          y_root,
                 CdkModifierType  mods,
                 guint32          evtime)
{
  CdkWin32DragContext *win32_context = CDK_WIN32_DRAG_CONTEXT (context);
  CdkDragAction action, possible_actions;
  CdkWindow *dest_window;
  CdkDragProtocol protocol;

  cdk_drag_get_current_actions (mods, CDK_BUTTON_PRIMARY, win32_context->actions,
                                &action, &possible_actions);

  cdk_drag_find_window_for_screen (context,
                                   win32_context->drag_window,
                                   cdk_display_get_default_screen (cdk_display_get_default ()),
                                   x_root, y_root, &dest_window, &protocol);

  cdk_drag_motion (context, dest_window, protocol, x_root, y_root,
                   action, possible_actions, evtime);
}

static gboolean
cdk_dnd_handle_motion_event (CdkDragContext       *context,
                             const CdkEventMotion *event)
{
  CdkModifierType state;

  if (!cdk_event_get_state ((CdkEvent *) event, &state))
    return FALSE;

  CDK_NOTE (DND, g_print ("gd_dnd_handle_motion_event: 0x%p\n",
                          context));

  cdk_drag_update (context, event->x_root, event->y_root, state,
                   cdk_event_get_time ((CdkEvent *) event));
  return TRUE;
}

static gboolean
cdk_dnd_handle_key_event (CdkDragContext    *context,
                          const CdkEventKey *event)
{
  CdkWin32DragContext *win32_context = CDK_WIN32_DRAG_CONTEXT (context);
  CdkModifierType state;
  CdkWindow *root_window;
  CdkDevice *pointer;
  gint dx, dy;

  CDK_NOTE (DND, g_print ("cdk_dnd_handle_key_event: 0x%p\n",
                          context));

  dx = dy = 0;
  state = event->state;
  pointer = cdk_device_get_associated_device (cdk_event_get_device ((CdkEvent *) event));

  if (event->type == CDK_KEY_PRESS)
    {
      switch (event->keyval)
        {
        case CDK_KEY_Escape:
          cdk_drag_context_cancel (context, CDK_DRAG_CANCEL_USER_CANCELLED);
          return TRUE;

        case CDK_KEY_space:
        case CDK_KEY_Return:
        case CDK_KEY_ISO_Enter:
        case CDK_KEY_KP_Enter:
        case CDK_KEY_KP_Space:
          if ((cdk_drag_context_get_selected_action (context) != 0) &&
              (cdk_drag_context_get_dest_window (context) != NULL))
            {
              g_signal_emit_by_name (context, "drop-performed",
                                     cdk_event_get_time ((CdkEvent *) event));
            }
          else
            cdk_drag_context_cancel (context, CDK_DRAG_CANCEL_NO_TARGET);

          return TRUE;

        case CDK_KEY_Up:
        case CDK_KEY_KP_Up:
          dy = (state & CDK_MOD1_MASK) ? -BIG_STEP : -SMALL_STEP;
          break;

        case CDK_KEY_Down:
        case CDK_KEY_KP_Down:
          dy = (state & CDK_MOD1_MASK) ? BIG_STEP : SMALL_STEP;
          break;

        case CDK_KEY_Left:
        case CDK_KEY_KP_Left:
          dx = (state & CDK_MOD1_MASK) ? -BIG_STEP : -SMALL_STEP;
          break;

        case CDK_KEY_Right:
        case CDK_KEY_KP_Right:
          dx = (state & CDK_MOD1_MASK) ? BIG_STEP : SMALL_STEP;
          break;
        }
    }

  /* The state is not yet updated in the event, so we need
   * to query it here.
   */
  root_window = cdk_screen_get_root_window (cdk_window_get_screen (win32_context->ipc_window));
  cdk_window_get_device_position (root_window, pointer, NULL, NULL, &state);

  if (dx != 0 || dy != 0)
    {
      win32_context->last_x += dx;
      win32_context->last_y += dy;
      cdk_device_warp (pointer,
                       cdk_window_get_screen (win32_context->ipc_window),
                       win32_context->last_x, win32_context->last_y);
    }

  cdk_drag_update (context, win32_context->last_x, win32_context->last_y, state,
                   cdk_event_get_time ((CdkEvent *) event));

  return TRUE;
}

static gboolean
cdk_dnd_handle_grab_broken_event (CdkDragContext           *context,
                                  const CdkEventGrabBroken *event)
{
  CdkWin32DragContext *win32_context = CDK_WIN32_DRAG_CONTEXT (context);

  CDK_NOTE (DND, g_print ("cdk_dnd_handle_grab_broken_event: 0x%p\n",
                          context));

  /* Don't cancel if we break the implicit grab from the initial button_press.
   * Also, don't cancel if we re-grab on the widget or on our IPC window, for
   * example, when changing the drag cursor.
   */
  if (event->implicit ||
      event->grab_window == win32_context->drag_window ||
      event->grab_window == win32_context->ipc_window)
    return FALSE;

  if (cdk_event_get_device ((CdkEvent *) event) !=
      cdk_drag_context_get_device (context))
    return FALSE;

  cdk_drag_context_cancel (context, CDK_DRAG_CANCEL_ERROR);
  return TRUE;
}

static gboolean
cdk_dnd_handle_button_event (CdkDragContext       *context,
                             const CdkEventButton *event)
{
  CDK_NOTE (DND, g_print ("cdk_dnd_handle_button_event: 0x%p\n",
                          context));

#if 0
  /* FIXME: Check the button matches */
  if (event->button != win32_context->button)
    return FALSE;
#endif

  if ((cdk_drag_context_get_selected_action (context) != 0) &&
      (cdk_drag_context_get_dest_window (context) != NULL))
    {
      g_signal_emit_by_name (context, "drop-performed",
                             cdk_event_get_time ((CdkEvent *) event));
    }
  else
    cdk_drag_context_cancel (context, CDK_DRAG_CANCEL_NO_TARGET);

  return TRUE;
}

gboolean
cdk_dnd_handle_drag_status (CdkDragContext    *context,
                            const CdkEventDND *event)
{
  CdkWin32DragContext *context_win32 = CDK_WIN32_DRAG_CONTEXT (context);
  CdkDragAction action;

  CDK_NOTE (DND, g_print ("cdk_dnd_handle_drag_status: 0x%p\n",
                          context));

  if (context != event->context)
    return FALSE;

  action = cdk_drag_context_get_selected_action (context);

  if (action != context_win32->current_action)
    {
      context_win32->current_action = action;
      g_signal_emit_by_name (context, "action-changed", action);
    }

  return TRUE;
}

static gboolean
cdk_dnd_handle_drop_finished (CdkDragContext *context,
                              const CdkEventDND *event)
{
  CdkWin32DragContext *win32_context = CDK_WIN32_DRAG_CONTEXT (context);

  CDK_NOTE (DND, g_print ("cdk_dnd_handle_drop_finihsed: 0x%p\n",
                          context));

  if (context != event->context)
    return FALSE;

  g_signal_emit_by_name (context, "dnd-finished");
  cdk_drag_drop_done (context, !win32_context->drop_failed);
  cdk_win32_selection_clear_targets (cdk_display_get_default (),
                                     cdk_win32_drag_context_get_selection (context));

  return TRUE;
}

gboolean
cdk_win32_drag_context_handle_event (CdkDragContext *context,
                                     const CdkEvent *event)
{
  CdkWin32DragContext *win32_context = CDK_WIN32_DRAG_CONTEXT (context);

  if (!context->is_source)
    return FALSE;
  if (!win32_context->grab_seat && event->type != CDK_DROP_FINISHED)
    return FALSE;

  switch (event->type)
    {
    case CDK_MOTION_NOTIFY:
      return cdk_dnd_handle_motion_event (context, &event->motion);
    case CDK_BUTTON_RELEASE:
      return cdk_dnd_handle_button_event (context, &event->button);
    case CDK_KEY_PRESS:
    case CDK_KEY_RELEASE:
      return cdk_dnd_handle_key_event (context, &event->key);
    case CDK_GRAB_BROKEN:
      return cdk_dnd_handle_grab_broken_event (context, &event->grab_broken);
    case CDK_DRAG_STATUS:
      return cdk_dnd_handle_drag_status (context, &event->dnd);
    case CDK_DROP_FINISHED:
      return cdk_dnd_handle_drop_finished (context, &event->dnd);
    default:
      break;
    }

  return FALSE;
}

void
cdk_win32_drag_context_action_changed (CdkDragContext *context,
                                       CdkDragAction   action)
{
  CdkCursor *cursor;

  cursor = cdk_drag_get_cursor (context, action);
  cdk_drag_context_set_cursor (context, cursor);
}

static CdkWindow *
cdk_win32_drag_context_get_drag_window (CdkDragContext *context)
{
  return CDK_WIN32_DRAG_CONTEXT (context)->drag_window;
}

static void
cdk_win32_drag_context_set_hotspot (CdkDragContext *context,
                                    gint            hot_x,
                                    gint            hot_y)
{
  CdkWin32DragContext *win32_context = CDK_WIN32_DRAG_CONTEXT (context);

  CDK_NOTE (DND, g_print ("cdk_drag_context_set_hotspot: 0x%p %d:%d\n",
                          context,
                          hot_x, hot_y));

  win32_context->hot_x = hot_x;
  win32_context->hot_y = hot_y;

  if (win32_context->grab_seat)
    {
      /* DnD is managed, update current position */
      move_drag_window (context, win32_context->last_x, win32_context->last_y);
    }
}

static void
cdk_win32_drag_context_class_init (CdkWin32DragContextClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CdkDragContextClass *context_class = CDK_DRAG_CONTEXT_CLASS (klass);

  object_class->finalize = cdk_win32_drag_context_finalize;

  context_class->find_window = cdk_win32_drag_context_find_window;
  context_class->drag_status = cdk_win32_drag_context_drag_status;
  context_class->drag_motion = cdk_win32_drag_context_drag_motion;
  context_class->drag_abort = cdk_win32_drag_context_drag_abort;
  context_class->drag_drop = cdk_win32_drag_context_drag_drop;
  context_class->drop_reply = cdk_win32_drag_context_drop_reply;
  context_class->drop_finish = cdk_win32_drag_context_drop_finish;
  context_class->drop_status = cdk_win32_drag_context_drop_status;
  context_class->get_selection = cdk_win32_drag_context_get_selection;

  context_class->get_drag_window = cdk_win32_drag_context_get_drag_window;
  context_class->set_hotspot = cdk_win32_drag_context_set_hotspot;
  context_class->drop_done = cdk_win32_drag_context_drop_done;
  context_class->manage_dnd = cdk_win32_drag_context_manage_dnd;
  context_class->set_cursor = cdk_win32_drag_context_set_cursor;
  context_class->cancel = cdk_win32_drag_context_cancel;
  context_class->drop_performed = cdk_win32_drag_context_drop_performed;
  context_class->handle_event = cdk_win32_drag_context_handle_event;
  context_class->action_changed = cdk_win32_drag_context_action_changed;

}
