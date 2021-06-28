/* GDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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

#include "cdkselection.h"

#include "cdkproperty.h"
#include "cdkdisplayprivate.h"


/**
 * SECTION:selections
 * @Short_description: Functions for transferring data between programs
 * @Title: Selections
 *
 * GDK’s selection functions, based on the [X selection mechanism](
 * https://www.x.org/releases/X11R7.6/doc/xorg-docs/specs/ICCCM/icccm.html),
 * provide a way to transfer arbitrary chunks of
 * data between programs. A “selection” is a essentially
 * a named clipboard, identified by a string interned as a #CdkAtom. By
 * claiming ownership of a selection, an application indicates that it will
 * be responsible for supplying its contents. The most common selections are
 * `PRIMARY` and `CLIPBOARD`.
 *
 * The contents of a selection can be represented in a number of formats,
 * called “targets”. Each target is identified by an atom.
 * A list of all possible targets supported by the selection owner can be
 * retrieved by requesting the special target `TARGETS`. When
 * a selection is retrieved, the data is accompanied by a type (an atom), and
 * a format (an integer, representing the number of bits per item).
 * See [Properties and Atoms][cdk3-Properties-and-Atoms]
 * for more information.
 *
 * The functions in this section only contain the lowlevel parts of the
 * selection protocol. A considerably more complicated implementation is needed
 * on top of this. CTK+ contains such an implementation in the functions in
 * `ctkselection.h` and programmers should use those functions
 * instead of the ones presented here. If you plan to implement selection
 * handling directly on top of the functions here, you should refer to the
 * [X Inter-Client Communication Conventions Manual (ICCCM)](
 * https://www.x.org/releases/X11R7.6/doc/xorg-docs/specs/ICCCM/icccm.html).
 *
 * Note that although much of the selection API design is based on that of X,
 * it will work on other GDK backends too.
 */

/**
 * cdk_selection_owner_set:
 * @owner: (allow-none): a #CdkWindow or %NULL to indicate that the
 *   the owner for the given should be unset.
 * @selection: an atom identifying a selection.
 * @time_: timestamp to use when setting the selection.
 *   If this is older than the timestamp given last
 *   time the owner was set for the given selection, the
 *   request will be ignored.
 * @send_event: if %TRUE, and the new owner is different
 *   from the current owner, the current owner
 *   will be sent a SelectionClear event.
 *
 * Sets the owner of the given selection.
 *
 * Returns: %TRUE if the selection owner was successfully
 *   changed to @owner, otherwise %FALSE.
 */
gboolean
cdk_selection_owner_set (CdkWindow *owner,
			 CdkAtom    selection,
			 guint32    time,
			 gboolean   send_event)
{
  return cdk_selection_owner_set_for_display (cdk_display_get_default (),
					      owner, selection, 
					      time, send_event);
}

/**
 * cdk_selection_owner_get:
 * @selection: an atom indentifying a selection.
 *
 * Determines the owner of the given selection.
 *
 * Returns: (nullable) (transfer none): if there is a selection owner
 *   for this window, and it is a window known to the current process,
 *   the #CdkWindow that owns the selection, otherwise %NULL. Note
 *   that the return value may be owned by a different process if a
 *   foreign window was previously created for that window, but a new
 *   foreign window will never be created by this call.
 */
CdkWindow*
cdk_selection_owner_get (CdkAtom selection)
{
  return cdk_selection_owner_get_for_display (cdk_display_get_default (), 
					      selection);
}

/**
 * cdk_selection_send_notify:
 * @requestor: window to which to deliver response.
 * @selection: selection that was requested.
 * @target: target that was selected.
 * @property: property in which the selection owner stored the
 *   data, or %GDK_NONE to indicate that the request
 *   was rejected.
 * @time_: timestamp.
 *
 * Sends a response to SelectionRequest event.
 */
void
cdk_selection_send_notify (CdkWindow      *requestor,
			   CdkAtom         selection,
			   CdkAtom         target,
			   CdkAtom         property,
			   guint32         time)
{
  cdk_selection_send_notify_for_display (cdk_window_get_display (requestor),
					 requestor, selection, 
					 target, property, time);
}

/**
 * cdk_selection_owner_set_for_display:
 * @display: the #CdkDisplay
 * @owner: (nullable): a #CdkWindow or %NULL to indicate that the owner for
 *         the given should be unset
 * @selection: an atom identifying a selection
 * @time_: timestamp to use when setting the selection
 *         If this is older than the timestamp given last time the owner was
 *         set for the given selection, the request will be ignored
 * @send_event: if %TRUE, and the new owner is different from the current
 *              owner, the current owner will be sent a SelectionClear event
 *
 * Sets the #CdkWindow @owner as the current owner of the selection @selection.
 *
 * Returns: %TRUE if the selection owner was successfully changed to owner,
 *    otherwise %FALSE.
 *
 * Since: 2.2
 */
gboolean
cdk_selection_owner_set_for_display (CdkDisplay *display,
                                     CdkWindow  *owner,
                                     CdkAtom     selection,
                                     guint32     time,
                                     gboolean    send_event)
{
  g_return_val_if_fail (GDK_IS_DISPLAY (display), FALSE);
  g_return_val_if_fail (selection != GDK_NONE, FALSE);

  return GDK_DISPLAY_GET_CLASS (display)
           ->set_selection_owner (display, owner, selection, time, send_event);
}

/**
 * cdk_selection_owner_get_for_display:
 * @display: a #CdkDisplay
 * @selection: an atom indentifying a selection
 *
 * Determine the owner of the given selection.
 *
 * Note that the return value may be owned by a different
 * process if a foreign window was previously created for that
 * window, but a new foreign window will never be created by this call.
 *
 * Returns: (nullable) (transfer none): if there is a selection owner
 *    for this window, and it is a window known to the current
 *    process, the #CdkWindow that owns the selection, otherwise
 *    %NULL.
 *
 * Since: 2.2
 */
CdkWindow *
cdk_selection_owner_get_for_display (CdkDisplay *display,
                                     CdkAtom     selection)
{
  g_return_val_if_fail (GDK_IS_DISPLAY (display), NULL);
  g_return_val_if_fail (selection != GDK_NONE, NULL);

  return GDK_DISPLAY_GET_CLASS (display)->get_selection_owner (display, selection);
}

/**
 * cdk_selection_send_notify_for_display:
 * @display: the #CdkDisplay where @requestor is realized
 * @requestor: window to which to deliver response
 * @selection: selection that was requested
 * @target: target that was selected
 * @property: property in which the selection owner stored the data,
 *            or %GDK_NONE to indicate that the request was rejected
 * @time_: timestamp
 *
 * Send a response to SelectionRequest event.
 *
 * Since: 2.2
 */
void
cdk_selection_send_notify_for_display (CdkDisplay       *display,
                                       CdkWindow        *requestor,
                                       CdkAtom          selection,
                                       CdkAtom          target,
                                       CdkAtom          property,
                                       guint32          time_)
{
  g_return_if_fail (GDK_IS_DISPLAY (display));

  GDK_DISPLAY_GET_CLASS (display)
    ->send_selection_notify (display, requestor, selection,target, property, time_);
}

/**
 * cdk_selection_property_get: (skip)
 * @requestor: the window on which the data is stored
 * @data: location to store a pointer to the retrieved data.
       If the retrieval failed, %NULL we be stored here, otherwise, it
       will be non-%NULL and the returned data should be freed with g_free()
       when you are finished using it. The length of the
       allocated memory is one more than the length
       of the returned data, and the final byte will always
       be zero, to ensure nul-termination of strings
 * @prop_type: location to store the type of the property
 * @prop_format: location to store the format of the property
 *
 * Retrieves selection data that was stored by the selection
 * data in response to a call to cdk_selection_convert(). This function
 * will not be used by applications, who should use the #CtkClipboard
 * API instead.
 *
 * Returns: the length of the retrieved data.
 */
gint
cdk_selection_property_get (CdkWindow  *requestor,
                            guchar    **data,
                            CdkAtom    *ret_type,
                            gint       *ret_format)
{
  CdkDisplay *display;

  g_return_val_if_fail (GDK_IS_WINDOW (requestor), 0);

  display = cdk_window_get_display (requestor);

  return GDK_DISPLAY_GET_CLASS (display)
           ->get_selection_property (display, requestor, data, ret_type, ret_format);
}

void
cdk_selection_convert (CdkWindow *requestor,
                       CdkAtom    selection,
                       CdkAtom    target,
                       guint32    time)
{
  CdkDisplay *display;

  g_return_if_fail (selection != GDK_NONE);

  display = cdk_window_get_display (requestor);

  GDK_DISPLAY_GET_CLASS (display)
    ->convert_selection (display, requestor, selection, target, time);
}

/**
 * cdk_text_property_to_utf8_list_for_display:
 * @display:  a #CdkDisplay
 * @encoding: an atom representing the encoding of the text
 * @format:   the format of the property
 * @text:     (array length=length): the text to convert
 * @length:   the length of @text, in bytes
 * @list:     (out) (array zero-terminated=1): location to store the list
 *            of strings or %NULL. The list should be freed with
 *            g_strfreev().
 *
 * Converts a text property in the given encoding to
 * a list of UTF-8 strings.
 *
 * Returns: the number of strings in the resulting list
 *
 * Since: 2.2
 */
gint
cdk_text_property_to_utf8_list_for_display (CdkDisplay     *display,
                                            CdkAtom         encoding,
                                            gint            format,
                                            const guchar   *text,
                                            gint            length,
                                            gchar        ***list)
{
  g_return_val_if_fail (text != NULL, 0);
  g_return_val_if_fail (length >= 0, 0);
  g_return_val_if_fail (GDK_IS_DISPLAY (display), 0);

  return GDK_DISPLAY_GET_CLASS (display)
           ->text_property_to_utf8_list (display, encoding, format, text, length, list);
}

/**
 * cdk_utf8_to_string_target:
 * @str: a UTF-8 string
 *
 * Converts an UTF-8 string into the best possible representation
 * as a STRING. The representation of characters not in STRING
 * is not specified; it may be as pseudo-escape sequences
 * \x{ABCD}, or it may be in some other form of approximation.
 *
 * Returns: (nullable): the newly-allocated string, or %NULL if the
 *          conversion failed. (It should not fail for any properly
 *          formed UTF-8 string unless system limits like memory or
 *          file descriptors are exceeded.)
 **/
gchar *
cdk_utf8_to_string_target (const gchar *str)
{
  CdkDisplay *display = cdk_display_get_default ();

  return GDK_DISPLAY_GET_CLASS (display)->utf8_to_string_target (display, str);
}
