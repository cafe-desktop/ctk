/* CDK - The GIMP Drawing Kit
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

/*
 * Modified by the CTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CDK_PRIVATE_WIN32_H__
#define __CDK_PRIVATE_WIN32_H__

#ifndef WINVER
/* Vista or newer */
#define WINVER 0x0600
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT WINVER
#endif

#include <cdk/cdkprivate.h>
#include <cdk/cdkcursorprivate.h>
#include <cdk/win32/cdkwindow-win32.h>
#include <cdk/win32/cdkwin32display.h>
#include <cdk/win32/cdkwin32screen.h>
#include <cdk/win32/cdkwin32keys.h>
#include <cdk/win32/cdkselection-win32.h>

#include "cdkinternals.h"

#include "config.h"

/* Make up for some minor w32api or MSVC6 header lossage */

#ifndef PS_JOIN_MASK
#define PS_JOIN_MASK (PS_JOIN_BEVEL|PS_JOIN_MITER|PS_JOIN_ROUND)
#endif

#ifndef FS_VIETNAMESE
#define FS_VIETNAMESE 0x100
#endif

#ifndef WM_GETOBJECT
#define WM_GETOBJECT 0x3D
#endif
#ifndef WM_NCXBUTTONDOWN
#define WM_NCXBUTTONDOWN 0xAB
#endif
#ifndef WM_NCXBUTTONUP
#define WM_NCXBUTTONUP 0xAC
#endif
#ifndef WM_NCXBUTTONDBLCLK
#define WM_NCXBUTTONDBLCLK 0xAD
#endif
#ifndef WM_CHANGEUISTATE
#define WM_CHANGEUISTATE 0x127
#endif
#ifndef WM_UPDATEUISTATE
#define WM_UPDATEUISTATE 0x128
#endif
#ifndef WM_QUERYUISTATE
#define WM_QUERYUISTATE 0x129
#endif
#ifndef WM_XBUTTONDOWN
#define WM_XBUTTONDOWN 0x20B
#endif
#ifndef WM_XBUTTONUP
#define WM_XBUTTONUP 0x20C
#endif
#ifndef WM_XBUTTONDBLCLK
#define WM_XBUTTONDBLCLK 0x20D
#endif
#ifndef WM_NCMOUSEHOVER
#define WM_NCMOUSEHOVER 0x2A0
#endif
#ifndef WM_NCMOUSELEAVE
#define WM_NCMOUSELEAVE 0x2A2
#endif
#ifndef WM_APPCOMMAND
#define WM_APPCOMMAND 0x319
#endif
#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x20E
#endif
#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif

/* According to
 * http://blog.airesoft.co.uk/2009/11/wm_messages/
 * this is the actual internal name MS uses for this undocumented message.
 * According to
 * https://bugs.winehq.org/show_bug.cgi?id=15055
 * wParam is 0
 * lParam is a pair of virtual desktop coordinates for the popup
 */
#ifndef WM_SYSMENU
#define WM_SYSMENU 0x313
#endif

#ifndef CF_DIBV5
#define CF_DIBV5 17
#endif


/* Define some combinations of CdkDebugFlags */
#define CDK_DEBUG_EVENTS_OR_INPUT (CDK_DEBUG_EVENTS|CDK_DEBUG_INPUT)
#define CDK_DEBUG_MISC_OR_EVENTS (CDK_DEBUG_MISC|CDK_DEBUG_EVENTS)

CdkScreen *CDK_WINDOW_SCREEN(GObject *win);

#define CDK_WINDOW_IS_WIN32(win)        (CDK_IS_WINDOW_IMPL_WIN32 (win->impl))

typedef struct _CdkColormapPrivateWin32 CdkColormapPrivateWin32;
typedef struct _CdkWin32SingleFont      CdkWin32SingleFont;

struct _CdkWin32Cursor
{
  CdkCursor cursor;

  gchar *name;
  HCURSOR hcursor;
};

struct _CdkWin32SingleFont
{
  HFONT hfont;
  UINT charset;
  UINT codepage;
  FONTSIGNATURE fs;
};

typedef enum {
  CDK_WIN32_PE_STATIC,
  CDK_WIN32_PE_AVAILABLE,
  CDK_WIN32_PE_INUSE
} CdkWin32PalEntryState;

struct _CdkColormapPrivateWin32
{
  HPALETTE hpal;
  gint current_size;		/* Current size of hpal */
  CdkWin32PalEntryState *use;
  gint private_val;

  GHashTable *hash;
  CdkColorInfo *info;
};

GType _cdk_gc_win32_get_type (void);

gulong _cdk_win32_get_next_tick (gulong suggested_tick);

void _cdk_window_init_position     (CdkWindow *window);
void _cdk_window_move_resize_child (CdkWindow *window,
				    gint       x,
				    gint       y,
				    gint       width,
				    gint       height);

gboolean _cdk_win32_window_enable_transparency (CdkWindow *window);


/* CdkWindowImpl methods */
void _cdk_win32_window_scroll (CdkWindow *window,
			       gint       dx,
			       gint       dy);
void _cdk_win32_window_move_region (CdkWindow       *window,
				    const cairo_region_t *region,
				    gint             dx,
				    gint             dy);


void _cdk_win32_selection_init (void);
void _cdk_win32_dnd_exit (void);

CdkDragProtocol _cdk_win32_window_get_drag_protocol (CdkWindow *window,
						     CdkWindow **target);

void	 cdk_win32_handle_table_insert  (HANDLE   *handle,
					 gpointer data);
void	 cdk_win32_handle_table_remove  (HANDLE handle);

HRGN	  _cdk_win32_cairo_region_to_hrgn (const cairo_region_t *region,
					   gint                  x_origin,
					   gint                  y_origin);

cairo_region_t *_cdk_win32_hrgn_to_region    (HRGN  hrgn,
                                              guint scale);

void	_cdk_win32_adjust_client_rect   (CdkWindow *window,
					 RECT      *RECT);

void    _cdk_selection_property_delete (CdkWindow *);

void    _cdk_dropfiles_store (gchar *data);

void       _cdk_push_modal_window   (CdkWindow *window);
void       _cdk_remove_modal_window (CdkWindow *window);
CdkWindow *_cdk_modal_current       (void);
gboolean   _cdk_modal_blocked       (CdkWindow *window);

#ifdef G_ENABLE_DEBUG
gchar *_cdk_win32_color_to_string      (const CdkColor *color);
void   _cdk_win32_print_paletteentries (const PALETTEENTRY *pep,
					const int           nentries);
void   _cdk_win32_print_system_palette (void);
void   _cdk_win32_print_hpalette       (HPALETTE     hpal);
void   _cdk_win32_print_dc             (HDC          hdc);

gchar *_cdk_win32_drag_protocol_to_string (CdkDragProtocol protocol);
gchar *_cdk_win32_window_state_to_string (CdkWindowState state);
gchar *_cdk_win32_window_style_to_string (LONG style);
gchar *_cdk_win32_window_exstyle_to_string (LONG style);
gchar *_cdk_win32_window_pos_bits_to_string (UINT flags);
gchar *_cdk_win32_drag_action_to_string (CdkDragAction actions);
gchar *_cdk_win32_window_description (CdkWindow *d);

gchar *_cdk_win32_rop2_to_string       (int          rop2);
gchar *_cdk_win32_lbstyle_to_string    (UINT         brush_style);
gchar *_cdk_win32_pstype_to_string     (DWORD        pen_style);
gchar *_cdk_win32_psstyle_to_string    (DWORD        pen_style);
gchar *_cdk_win32_psendcap_to_string   (DWORD        pen_style);
gchar *_cdk_win32_psjoin_to_string     (DWORD        pen_style);
gchar *_cdk_win32_message_to_string    (UINT         msg);
gchar *_cdk_win32_key_to_string        (LONG         lParam);
gchar *_cdk_win32_cf_to_string         (UINT         format);
gchar *_cdk_win32_data_to_string       (const guchar*data,
					int          nbytes);
gchar *_cdk_win32_rect_to_string       (const RECT  *rect);

gchar *_cdk_win32_cdkrectangle_to_string (const CdkRectangle *rect);
gchar *_cdk_win32_cairo_region_to_string (const cairo_region_t    *box);

void   _cdk_win32_print_event            (const CdkEvent     *event);

#endif

gchar  *_cdk_win32_last_error_string (void);
void    _cdk_win32_api_failed        (const gchar *where,
				     const gchar *api);
void    _cdk_other_api_failed        (const gchar *where,
				     const gchar *api);

#define WIN32_API_FAILED(api) _cdk_win32_api_failed (G_STRLOC , api)
#define WIN32_GDI_FAILED(api) WIN32_API_FAILED (api)
#define OTHER_API_FAILED(api) _cdk_other_api_failed (G_STRLOC, api)

/* These two macros call a GDI or other Win32 API and if the return
 * value is zero or NULL, print a warning message. The majority of GDI
 * calls return zero or NULL on failure. The value of the macros is nonzero
 * if the call succeeded, zero otherwise.
 */

#define GDI_CALL(api, arglist) (api arglist ? 1 : (WIN32_GDI_FAILED (#api), 0))
#define API_CALL(api, arglist) (api arglist ? 1 : (WIN32_API_FAILED (#api), 0))

extern LRESULT CALLBACK _cdk_win32_window_procedure (HWND, UINT, WPARAM, LPARAM);

extern CdkDisplay       *_cdk_display;

/* Offsets to add to Windows coordinates (which are relative to the
 * primary monitor's origin, and thus might be negative for monitors
 * to the left and/or above the primary monitor) to get CDK
 * coordinates, which should be non-negative on the whole screen.
 */
extern gint		 _cdk_offset_x, _cdk_offset_y;

extern HDC		 _cdk_display_hdc;
extern HINSTANCE	 _cdk_dll_hinstance;
extern HINSTANCE	 _cdk_app_hmodule;

extern gint		 _cdk_input_ignore_core;

/* These are thread specific, but CDK/win32 works OK only when invoked
 * from a single thread anyway.
 */
extern HKL		 _cdk_input_locale;
extern gboolean		 _cdk_input_locale_is_ime;
extern UINT		 _cdk_input_codepage;

extern guint		 _cdk_keymap_serial;

/* The singleton selection object pointer */
extern CdkWin32Selection *_win32_selection;

void _cdk_win32_dnd_do_dragdrop (void);
void _cdk_win32_ole2_dnd_property_change (CdkAtom       type,
					  gint          format,
					  const guchar *data,
					  gint          nelements);

typedef enum {
  CDK_WIN32_MODAL_OP_NONE = 0x0,
  CDK_WIN32_MODAL_OP_SIZE = 0x1 << 0,
  CDK_WIN32_MODAL_OP_MOVE = 0x1 << 1,
  CDK_WIN32_MODAL_OP_MENU = 0x1 << 2,
  CDK_WIN32_MODAL_OP_DND  = 0x1 << 3
} CdkWin32ModalOpKind;

#define CDK_WIN32_MODAL_OP_SIZEMOVE_MASK (CDK_WIN32_MODAL_OP_SIZE | CDK_WIN32_MODAL_OP_MOVE)

/* Non-zero while a modal sizing, moving, or dnd operation is in progress */
extern CdkWin32ModalOpKind _modal_operation_in_progress;

extern HWND		_modal_move_resize_window;

void  _cdk_win32_begin_modal_call (CdkWin32ModalOpKind kind);
void  _cdk_win32_end_modal_call (CdkWin32ModalOpKind kind);


/* Options */
extern gboolean		 _cdk_input_ignore_wintab;
extern gint		 _cdk_max_colors;

#define CDK_WIN32_COLORMAP_DATA(cmap) ((CdkColormapPrivateWin32 *) CDK_COLORMAP (cmap)->windowing_data)

extern CdkCursor *_cdk_win32_grab_cursor;

/* Convert a pixbuf to an HICON (or HCURSOR).  Supports alpha under
 * Windows XP, thresholds alpha otherwise.
 */
HICON _cdk_win32_pixbuf_to_hicon   (GdkPixbuf *pixbuf);
HICON _cdk_win32_pixbuf_to_hcursor (GdkPixbuf *pixbuf,
				    gint       x_hotspot,
				    gint       y_hotspot);

void _cdk_win32_display_init_cursors (CdkWin32Display     *display);
void _cdk_win32_display_finalize_cursors (CdkWin32Display *display);
void _cdk_win32_display_update_cursors (CdkWin32Display   *display);

typedef struct _Win32CursorTheme Win32CursorTheme;

struct _Win32CursorTheme {
  GHashTable *named_cursors;
};

typedef enum CdkWin32CursorLoadType {
  CDK_WIN32_CURSOR_LOAD_FROM_FILE = 0,
  CDK_WIN32_CURSOR_LOAD_FROM_RESOURCE_NULL = 1,
  CDK_WIN32_CURSOR_LOAD_FROM_RESOURCE_THIS = 2,
  CDK_WIN32_CURSOR_CREATE = 3,
} CdkWin32CursorLoadType;

typedef struct _Win32Cursor Win32Cursor;

struct _Win32Cursor {
  CdkWin32CursorLoadType load_type;
  gunichar2 *resource_name;
  gint width;
  gint height;
  guint load_flags;
  gint xcursor_number;
  CdkCursorType cursor_type;
};

Win32CursorTheme *win32_cursor_theme_load             (const gchar      *name,
                                                       gint              size);
Win32Cursor *     win32_cursor_theme_get_cursor       (Win32CursorTheme *theme,
                                                       const gchar      *name);
void              win32_cursor_theme_destroy          (Win32CursorTheme *theme);
Win32CursorTheme *_cdk_win32_display_get_cursor_theme (CdkWin32Display  *win32_display);

/* CdkDisplay member functions */
CdkCursor *_cdk_win32_display_get_cursor_for_type (CdkDisplay   *display,
						   CdkCursorType cursor_type);
CdkCursor *_cdk_win32_display_get_cursor_for_name (CdkDisplay  *display,
						   const gchar *name);
CdkCursor *_cdk_win32_display_get_cursor_for_surface (CdkDisplay *display,
						     cairo_surface_t  *surface,
						     gdouble          x,
						     gdouble          y);
void     _cdk_win32_display_get_default_cursor_size (CdkDisplay  *display,
						     guint       *width,
						     guint       *height);
void     _cdk_win32_display_get_maximal_cursor_size (CdkDisplay  *display,
						     guint       *width,
						     guint       *height);
gboolean _cdk_win32_display_supports_cursor_alpha (CdkDisplay    *display);
gboolean _cdk_win32_display_supports_cursor_color (CdkDisplay    *display);

GList *_cdk_win32_display_list_devices (CdkDisplay *dpy);

gboolean _cdk_win32_display_has_pending (CdkDisplay *display);
void _cdk_win32_display_queue_events (CdkDisplay *display);

gboolean _cdk_win32_selection_owner_set_for_display (CdkDisplay *display,
						     CdkWindow  *owner,
						     CdkAtom     selection,
						     guint32     time,
						     gboolean    send_event);
CdkWindow *_cdk_win32_display_get_selection_owner   (CdkDisplay *display,
						     CdkAtom     selection);
gboolean   _cdk_win32_display_set_selection_owner   (CdkDisplay *display,
						     CdkWindow  *owner,
						     CdkAtom     selection,
						     guint32     time,
						     gboolean    send_event);
void       _cdk_win32_display_send_selection_notify (CdkDisplay      *display,
						     CdkWindow       *requestor,
						     CdkAtom   	      selection,
						     CdkAtom          target,
						     CdkAtom          property,
						     guint32          time);
gint      _cdk_win32_display_get_selection_property (CdkDisplay *display,
						     CdkWindow  *requestor,
						     guchar    **data,
						     CdkAtom    *ret_type,
						     gint       *ret_format);
void      _cdk_win32_display_convert_selection (CdkDisplay *display,
						CdkWindow *requestor,
						CdkAtom    selection,
						CdkAtom    target,
						guint32    time);
gint      _cdk_win32_display_text_property_to_utf8_list (CdkDisplay    *display,
							 CdkAtom        encoding,
							 gint           format,
							 const guchar  *text,
							 gint           length,
							 gchar       ***list);
gchar     *_cdk_win32_display_utf8_to_string_target (CdkDisplay *display, const gchar *str);

gboolean   _cdk_win32_keymap_has_altgr           (CdkWin32Keymap *keymap);
guint8     _cdk_win32_keymap_get_active_group    (CdkWin32Keymap *keymap);
guint8     _cdk_win32_keymap_get_rshift_scancode (CdkWin32Keymap *keymap);
void       _cdk_win32_keymap_set_active_layout   (CdkWin32Keymap *keymap,
                                                  HKL             hkl);


CdkKeymap *_cdk_win32_display_get_keymap (CdkDisplay *display);

void       _cdk_win32_display_create_window_impl   (CdkDisplay    *display,
                                                    CdkWindow     *window,
                                                    CdkWindow     *real_parent,
                                                    CdkScreen     *screen,
                                                    CdkEventMask   event_mask,
                                                    CdkWindowAttr *attributes,
                                                    gint           attributes_mask);

/* stray CdkWindowImplWin32 members */
void _cdk_win32_window_register_dnd (CdkWindow *window);
CdkDragContext *_cdk_win32_window_drag_begin (CdkWindow *window, CdkDevice *device, GList *targets, gint x_root, gint y_root);
gboolean _cdk_win32_window_simulate_key (CdkWindow      *window,
				  gint            x,
				  gint            y,
				  guint           keyval,
				  CdkModifierType modifiers,
				  CdkEventType    key_pressrelease);
gboolean _cdk_win32_window_simulate_button (CdkWindow      *window,
				     gint            x,
				     gint            y,
				     guint           button, /*1..3*/
				     CdkModifierType modifiers,
				     CdkEventType    button_pressrelease);

gint _cdk_win32_window_get_property (CdkWindow   *window,
				     CdkAtom      property,
				     CdkAtom      type,
				     gulong       offset,
				     gulong       length,
				     gint         pdelete,
				     CdkAtom     *actual_property_type,
				     gint        *actual_format_type,
				     gint        *actual_length,
				     guchar     **data);
void _cdk_win32_window_change_property (CdkWindow    *window,
					CdkAtom       property,
					CdkAtom       type,
					gint          format,
					CdkPropMode   mode,
					const guchar *data,
					gint          nelements);
void _cdk_win32_window_delete_property (CdkWindow *window, CdkAtom    property);

/* Stray CdkWin32Screen members */
gboolean _cdk_win32_screen_get_setting (CdkScreen   *screen, const gchar *name, GValue *value);
void _cdk_win32_screen_on_displaychange_event (CdkWin32Screen *screen);

/* Distributed display manager implementation */
CdkDisplay *_cdk_win32_display_open (const gchar *display_name);
CdkAtom _cdk_win32_display_manager_atom_intern (CdkDisplayManager *manager,
						const gchar *atom_name,
						gint         only_if_exists);
gchar *_cdk_win32_display_manager_get_atom_name (CdkDisplayManager *manager,
					         CdkAtom            atom);
void _cdk_win32_append_event (CdkEvent *event);
void _cdk_win32_emit_configure_event (CdkWindow *window);


guint32 _cdk_win32_keymap_get_decimal_mark (CdkWin32Keymap *keymap);

void     _cdk_win32_window_handle_aerosnap      (CdkWindow            *window,
                                                 CdkWin32AeroSnapCombo combo);

gboolean _cdk_win32_get_window_rect             (CdkWindow  *window,
                                                 RECT       *rect);
void     _cdk_win32_do_emit_configure_event     (CdkWindow  *window,
                                                 RECT        rect);
void      cdk_win32_window_do_move_resize_drag  (CdkWindow  *window,
                                                 gint        x,
                                                 gint        y);
void      cdk_win32_window_end_move_resize_drag (CdkWindow  *window);
gboolean _cdk_win32_window_fill_min_max_info    (CdkWindow  *window,
                                                 MINMAXINFO *mmi);

gboolean _cdk_win32_window_lacks_wm_decorations (CdkWindow *window);

BOOL WINAPI CtkShowWindow (CdkWindow *window,
                           int        cmd_show);

void     _cdk_win32_screen_set_font_resolution (CdkWin32Screen *win32_screen);

/* Initialization */
void _cdk_win32_windowing_init (void);
void _cdk_dnd_init    (void);
void _cdk_events_init (CdkDisplay *display);

#endif /* __CDK_PRIVATE_WIN32_H__ */
