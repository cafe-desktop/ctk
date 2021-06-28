/* CTK - The GIMP Toolkit
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

#if !defined (__CDK_H_INSIDE__) && !defined (CDK_COMPILATION)
#error "Only <cdk/cdk.h> can be included directly."
#endif

#ifndef __GI_SCANNER__

G_DEFINE_AUTOPTR_CLEANUP_FUNC(CdkAppLaunchContext, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CdkCursor, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CdkDevice, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CdkDeviceManager, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CdkDisplay, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CdkDisplayManager, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CdkDragContext, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CdkFrameClock, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CdkGLContext, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CdkKeymap, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CdkScreen, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CdkVisual, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CdkWindow, g_object_unref)

G_DEFINE_AUTOPTR_CLEANUP_FUNC(CdkEvent, cdk_event_free)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CdkFrameTimings, cdk_frame_timings_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CdkRGBA, cdk_rgba_free)

#endif
