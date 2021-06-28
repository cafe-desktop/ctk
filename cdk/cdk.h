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

#ifndef __CDK_H__
#define __CDK_H__

#define __CDK_H_INSIDE__

#include <cdk/cdkconfig.h>
#include <cdk/cdkversionmacros.h>
#include <cdk/cdkapplaunchcontext.h>
#include <cdk/cdkcairo.h>
#include <cdk/cdkcursor.h>
#include <cdk/cdkdevice.h>
#include <cdk/cdkdevicepad.h>
#include <cdk/cdkdevicetool.h>
#include <cdk/cdkdevicemanager.h>
#include <cdk/cdkdisplay.h>
#include <cdk/cdkdisplaymanager.h>
#include <cdk/cdkdnd.h>
#include <cdk/cdkdrawingcontext.h>
#include <cdk/cdkenumtypes.h>
#include <cdk/cdkevents.h>
#include <cdk/cdkframeclock.h>
#include <cdk/cdkframetimings.h>
#include <cdk/cdkglcontext.h>
#include <cdk/cdkkeys.h>
#include <cdk/cdkkeysyms.h>
#include <cdk/cdkmain.h>
#include <cdk/cdkmonitor.h>
#include <cdk/cdkpango.h>
#include <cdk/cdkpixbuf.h>
#include <cdk/cdkproperty.h>
#include <cdk/cdkrectangle.h>
#include <cdk/cdkrgba.h>
#include <cdk/cdkscreen.h>
#include <cdk/cdkseat.h>
#include <cdk/cdkselection.h>
#include <cdk/cdktestutils.h>
#include <cdk/cdkthreads.h>
#include <cdk/cdktypes.h>
#include <cdk/cdkvisual.h>
#include <cdk/cdkwindow.h>

#ifndef CDK_DISABLE_DEPRECATED
#include <cdk/deprecated/cdkcolor.h>
#endif

#include <cdk/cdk-autocleanup.h>

#undef __CDK_H_INSIDE__

#endif /* __CDK_H__ */
