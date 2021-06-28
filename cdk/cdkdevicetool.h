/* CDK - The GIMP Drawing Kit
 * Copyright (C) 2009 Carlos Garnacho <carlosg@gnome.org>
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

#ifndef __CDK_DEVICE_TOOL_H__
#define __CDK_DEVICE_TOOL_H__

#if !defined (__CDK_H_INSIDE__) && !defined (CDK_COMPILATION)
#error "Only <cdk/cdk.h> can be included directly."
#endif

#include <cdk/cdkversionmacros.h>
#include <cdk/cdktypes.h>


G_BEGIN_DECLS

#define CDK_TYPE_DEVICE_TOOL    (cdk_device_tool_get_type ())
#define CDK_DEVICE_TOOL(o)      (G_TYPE_CHECK_INSTANCE_CAST ((o), CDK_TYPE_DEVICE_TOOL, CdkDeviceTool))
#define CDK_IS_DEVICE_TOOL(o)   (G_TYPE_CHECK_INSTANCE_TYPE ((o), CDK_TYPE_DEVICE_TOOL))

typedef struct _CdkDeviceTool CdkDeviceTool;

/**
 * CdkDeviceToolType:
 * @CDK_DEVICE_TOOL_TYPE_UNKNOWN: Tool is of an unknown type.
 * @CDK_DEVICE_TOOL_TYPE_PEN: Tool is a standard tablet stylus.
 * @CDK_DEVICE_TOOL_TYPE_ERASER: Tool is standard tablet eraser.
 * @CDK_DEVICE_TOOL_TYPE_BRUSH: Tool is a brush stylus.
 * @CDK_DEVICE_TOOL_TYPE_PENCIL: Tool is a pencil stylus.
 * @CDK_DEVICE_TOOL_TYPE_AIRBRUSH: Tool is an airbrush stylus.
 * @CDK_DEVICE_TOOL_TYPE_MOUSE: Tool is a mouse.
 * @CDK_DEVICE_TOOL_TYPE_LENS: Tool is a lens cursor.
 *
 * Indicates the specific type of tool being used being a tablet. Such as an
 * airbrush, pencil, etc.
 *
 * Since: 3.22
 */
typedef enum {
  CDK_DEVICE_TOOL_TYPE_UNKNOWN,
  CDK_DEVICE_TOOL_TYPE_PEN,
  CDK_DEVICE_TOOL_TYPE_ERASER,
  CDK_DEVICE_TOOL_TYPE_BRUSH,
  CDK_DEVICE_TOOL_TYPE_PENCIL,
  CDK_DEVICE_TOOL_TYPE_AIRBRUSH,
  CDK_DEVICE_TOOL_TYPE_MOUSE,
  CDK_DEVICE_TOOL_TYPE_LENS,
} CdkDeviceToolType;

CDK_AVAILABLE_IN_3_22
GType cdk_device_tool_get_type (void) G_GNUC_CONST;

CDK_AVAILABLE_IN_3_22
guint64 cdk_device_tool_get_serial (CdkDeviceTool *tool);

CDK_AVAILABLE_IN_3_22
guint64 cdk_device_tool_get_hardware_id (CdkDeviceTool *tool);

CDK_AVAILABLE_IN_3_22
CdkDeviceToolType cdk_device_tool_get_tool_type (CdkDeviceTool *tool);

G_END_DECLS

#endif /* __CDK_DEVICE_TOOL_H__ */
