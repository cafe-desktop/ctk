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

#ifndef __CDK_SELECTION_H__
#define __CDK_SELECTION_H__

#if !defined (__CDK_H_INSIDE__) && !defined (CDK_COMPILATION)
#error "Only <cdk/cdk.h> can be included directly."
#endif

#include <cdk/cdktypes.h>
#include <cdk/cdkversionmacros.h>

G_BEGIN_DECLS

/* Predefined atoms relating to selections. In general, one will need to use
 * cdk_intern_atom
 */
/**
 * CDK_SELECTION_PRIMARY:
 *
 * A #CdkAtom representing the `PRIMARY` selection.
 */
#define CDK_SELECTION_PRIMARY 		_CDK_MAKE_ATOM (1)

/**
 * CDK_SELECTION_SECONDARY:
 *
 * A #CdkAtom representing the `SECONDARY` selection.
 */
#define CDK_SELECTION_SECONDARY 	_CDK_MAKE_ATOM (2)

/**
 * CDK_SELECTION_CLIPBOARD:
 *
 * A #CdkAtom representing the `CLIPBOARD` selection.
 */
#define CDK_SELECTION_CLIPBOARD 	_CDK_MAKE_ATOM (69)

/**
 * CDK_TARGET_BITMAP:
 *
 * A #CdkAtom representing the `BITMAP` selection target.
 */
#define CDK_TARGET_BITMAP 		_CDK_MAKE_ATOM (5)

/**
 * CDK_TARGET_COLORMAP:
 *
 * A #CdkAtom representing the `COLORMAP` selection target.
 */
#define CDK_TARGET_COLORMAP 		_CDK_MAKE_ATOM (7)

/**
 * CDK_TARGET_DRAWABLE:
 *
 * A #CdkAtom representing the `DRAWABLE` selection target.
 */
#define CDK_TARGET_DRAWABLE 		_CDK_MAKE_ATOM (17)

/**
 * CDK_TARGET_PIXMAP:
 *
 * A #CdkAtom representing the `PIXMAP` selection target.
 */
#define CDK_TARGET_PIXMAP 		_CDK_MAKE_ATOM (20)

/**
 * CDK_TARGET_STRING:
 *
 * A #CdkAtom representing the `STRING` selection target.
 */
#define CDK_TARGET_STRING 		_CDK_MAKE_ATOM (31)

/**
 * CDK_SELECTION_TYPE_ATOM:
 *
 * A #CdkAtom representing the `ATOM` selection type.
 */
#define CDK_SELECTION_TYPE_ATOM 	_CDK_MAKE_ATOM (4)

/**
 * CDK_SELECTION_TYPE_BITMAP:
 *
 * A #CdkAtom representing the `BITMAP` selection type.
 */
#define CDK_SELECTION_TYPE_BITMAP 	_CDK_MAKE_ATOM (5)

/**
 * CDK_SELECTION_TYPE_COLORMAP:
 *
 * A #CdkAtom representing the `COLORMAP` selection type.
 */
#define CDK_SELECTION_TYPE_COLORMAP 	_CDK_MAKE_ATOM (7)

/**
 * CDK_SELECTION_TYPE_DRAWABLE:
 *
 * A #CdkAtom representing the `DRAWABLE` selection type.
 */
#define CDK_SELECTION_TYPE_DRAWABLE 	_CDK_MAKE_ATOM (17)

/**
 * CDK_SELECTION_TYPE_INTEGER:
 *
 * A #CdkAtom representing the `INTEGER` selection type.
 */
#define CDK_SELECTION_TYPE_INTEGER 	_CDK_MAKE_ATOM (19)

/**
 * CDK_SELECTION_TYPE_PIXMAP:
 *
 * A #CdkAtom representing the `PIXMAP` selection type.
 */
#define CDK_SELECTION_TYPE_PIXMAP 	_CDK_MAKE_ATOM (20)

/**
 * CDK_SELECTION_TYPE_WINDOW:
 *
 * A #CdkAtom representing the `WINDOW` selection type.
 */
#define CDK_SELECTION_TYPE_WINDOW 	_CDK_MAKE_ATOM (33)

/**
 * CDK_SELECTION_TYPE_STRING:
 *
 * A #CdkAtom representing the `STRING` selection type.
 */
#define CDK_SELECTION_TYPE_STRING 	_CDK_MAKE_ATOM (31)

/* Selections
 */

CDK_AVAILABLE_IN_ALL
gboolean   cdk_selection_owner_set (CdkWindow	 *owner,
				    CdkAtom	  selection,
				    guint32	  time_,
				    gboolean      send_event);
CDK_AVAILABLE_IN_ALL
CdkWindow* cdk_selection_owner_get (CdkAtom	  selection);
CDK_AVAILABLE_IN_ALL
gboolean   cdk_selection_owner_set_for_display (CdkDisplay *display,
						CdkWindow  *owner,
						CdkAtom     selection,
						guint32     time_,
						gboolean    send_event);
CDK_AVAILABLE_IN_ALL
CdkWindow *cdk_selection_owner_get_for_display (CdkDisplay *display,
						CdkAtom     selection);

/**
 * cdk_selection_convert:
 * @requestor: a #CdkWindow.
 * @selection: an atom identifying the selection to get the
 *   contents of.
 * @target: the form in which to retrieve the selection.
 * @time_: the timestamp to use when retrieving the
 *   selection. The selection owner may refuse the
 *   request if it did not own the selection at
 *   the time indicated by the timestamp.
 *
 * Retrieves the contents of a selection in a given
 * form.
 */
CDK_AVAILABLE_IN_ALL
void	   cdk_selection_convert   (CdkWindow	 *requestor,
				    CdkAtom	  selection,
				    CdkAtom	  target,
				    guint32	  time_);
CDK_AVAILABLE_IN_ALL
gint       cdk_selection_property_get (CdkWindow  *requestor,
				       guchar	 **data,
				       CdkAtom	  *prop_type,
				       gint	  *prop_format);

CDK_AVAILABLE_IN_ALL
void	   cdk_selection_send_notify (CdkWindow      *requestor,
				      CdkAtom	      selection,
				      CdkAtom	      target,
				      CdkAtom	      property,
				      guint32	      time_);

CDK_AVAILABLE_IN_ALL
void       cdk_selection_send_notify_for_display (CdkDisplay      *display,
						  CdkWindow       *requestor,
						  CdkAtom     	   selection,
						  CdkAtom     	   target,
						  CdkAtom     	   property,
						  guint32     	   time_);

G_END_DECLS

#endif /* __CDK_SELECTION_H__ */
