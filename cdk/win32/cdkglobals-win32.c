/* CDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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
#include "cdktypes.h"
#include "cdkprivate-win32.h"

CdkDisplay	 *_cdk_display = NULL;

gint		  _cdk_offset_x, _cdk_offset_y;

HDC		  _cdk_display_hdc;
HINSTANCE	  _cdk_dll_hinstance;
HINSTANCE	  _cdk_app_hmodule;

gint		  _cdk_input_ignore_core;

HKL		  _cdk_input_locale;
gboolean	  _cdk_input_locale_is_ime = FALSE;
UINT		  _cdk_input_codepage;

gint		  _cdk_input_ignore_wintab = FALSE;
gint		  _cdk_max_colors = 0;

CdkWin32ModalOpKind	  _modal_operation_in_progress = CDK_WIN32_MODAL_OP_NONE;
HWND              _modal_move_resize_window = NULL;

/* The singleton selection object pointer */
CdkWin32Selection *_win32_selection = NULL;
