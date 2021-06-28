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

#ifndef __CTK_TYPES_H__
#define __CTK_TYPES_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

G_BEGIN_DECLS

typedef struct _CtkAdjustment          CtkAdjustment;
typedef struct _CtkBuilder             CtkBuilder;
typedef struct _CtkClipboard	       CtkClipboard;
typedef struct _CtkIconSet             CtkIconSet;
typedef struct _CtkIconSource          CtkIconSource;
typedef struct _CtkRcStyle             CtkRcStyle;
typedef struct _CtkRequisition	       CtkRequisition;
typedef struct _CtkSelectionData       CtkSelectionData;
typedef struct _CtkSettings            CtkSettings;
typedef struct _CtkStyle               CtkStyle;
typedef struct _CtkStyleContext        CtkStyleContext;
typedef struct _CtkTooltip             CtkTooltip;
typedef struct _CtkWidget              CtkWidget;
typedef struct _CtkWidgetPath          CtkWidgetPath;
typedef struct _CtkWindow              CtkWindow;


typedef gboolean (*CtkRcPropertyParser) (const GParamSpec *pspec,
                                         const GString    *rc_string,
                                         GValue           *property_value);

typedef void (*CtkBuilderConnectFunc) (CtkBuilder    *builder,
				       GObject       *object,
				       const gchar   *signal_name,
				       const gchar   *handler_name,
				       GObject       *connect_object,
				       GConnectFlags  flags,
				       gpointer       user_data);

G_END_DECLS

#endif /* __CTK_TYPES_H__ */
