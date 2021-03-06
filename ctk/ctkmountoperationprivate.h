/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* CTK - The GIMP Toolkit
 * Copyright (C) David Zeuthen <davidz@redhat.com>
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

#ifndef __CTK_MOUNT_OPERATION_PRIVATE_H__
#define __CTK_MOUNT_OPERATION_PRIVATE_H__

#include <gio/gio.h>
#include <cdk/cdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

struct _CtkMountOperationLookupContext;
typedef struct _CtkMountOperationLookupContext CtkMountOperationLookupContext;

CtkMountOperationLookupContext *_ctk_mount_operation_lookup_context_get  (CdkDisplay *display);

gboolean _ctk_mount_operation_lookup_info         (CtkMountOperationLookupContext *context,
                                                   GPid                            pid,
                                                   gint                            size_pixels,
                                                   gchar                         **out_name,
                                                   gchar                         **out_command_line,
                                                   GdkPixbuf                     **out_pixbuf);

void     _ctk_mount_operation_lookup_context_free (CtkMountOperationLookupContext *context);

/* throw G_IO_ERROR_FAILED_HANDLED if a helper already reported the error to the user */
gboolean _ctk_mount_operation_kill_process (GPid      pid,
                                            GError  **error);

#endif /* __CTK_MOUNT_OPERATION_PRIVATE_H__ */
