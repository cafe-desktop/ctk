/* cdkutils-quartz.c
 *
 * Copyright (C) 2005 Imendio AB
 * Copyright (C) 2010  Kristian Rietveld  <kris@ctk.org>
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

#include "config.h"

#include <cdk/cdk.h>
#include <cdkinternals.h>

#include "cdkquartz-ctk-only.h"
#include <cdkquartzutils.h>

NSImage *
cdk_quartz_pixbuf_to_ns_image_libctk_only (GdkPixbuf *pixbuf)
{
  NSBitmapImageRep  *bitmap_rep;
  NSImage           *image;
  gboolean           has_alpha;
  
  has_alpha = cdk_pixbuf_get_has_alpha (pixbuf);
  
  /* Create a bitmap image rep */
  bitmap_rep = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL 
                                         pixelsWide:cdk_pixbuf_get_width (pixbuf)
					 pixelsHigh:cdk_pixbuf_get_height (pixbuf)
					 bitsPerSample:8 samplesPerPixel:has_alpha ? 4 : 3
					 hasAlpha:has_alpha isPlanar:NO colorSpaceName:NSDeviceRGBColorSpace
					 bytesPerRow:0 bitsPerPixel:0];
	
  {
    /* Add pixel data to bitmap rep */
    guchar *src, *dst;
    int src_stride, dst_stride;
    int x, y;
		
    src_stride = cdk_pixbuf_get_rowstride (pixbuf);
    dst_stride = [bitmap_rep bytesPerRow];
		
    for (y = 0; y < cdk_pixbuf_get_height (pixbuf); y++) 
      {
	src = cdk_pixbuf_get_pixels (pixbuf) + y * src_stride;
	dst = [bitmap_rep bitmapData] + y * dst_stride;
	
	for (x = 0; x < cdk_pixbuf_get_width (pixbuf); x++)
	  {
	    if (has_alpha)
	      {
		guchar red, green, blue, alpha;
		
		red = *src++;
		green = *src++;
		blue = *src++;
		alpha = *src++;
		
		*dst++ = (red * alpha) / 255;
		*dst++ = (green * alpha) / 255;
		*dst++ = (blue * alpha) / 255;
		*dst++ = alpha;
	      }
	    else
	     {
	       *dst++ = *src++;
	       *dst++ = *src++;
	       *dst++ = *src++;
	     }
	  }
      }	
  }
	
  image = [[NSImage alloc] init];
  [image addRepresentation:bitmap_rep];
  [bitmap_rep release];
  [image autorelease];
	
  return image;
}

NSEvent *
cdk_quartz_event_get_nsevent (CdkEvent *event)
{
  /* FIXME: If the event here is unallocated, we crash. */
  return ((CdkEventPrivate *) event)->windowing_data;
}

/*
 * Code for key code conversion
 *
 * Copyright (C) 2009 Paul Davis
 */
gunichar
cdk_quartz_get_key_equivalent (guint key)
{
  if (key >= CDK_KEY_A && key <= CDK_KEY_Z)
    return key + (CDK_KEY_a - CDK_KEY_A);

  if (key >= CDK_KEY_space && key <= CDK_KEY_asciitilde)
    return key;

  switch (key)
    {
      case CDK_KEY_BackSpace:
        return NSBackspaceCharacter;
      case CDK_KEY_Delete:
        return NSDeleteFunctionKey;
      case CDK_KEY_Pause:
        return NSPauseFunctionKey;
      case CDK_KEY_Scroll_Lock:
        return NSScrollLockFunctionKey;
      case CDK_KEY_Sys_Req:
        return NSSysReqFunctionKey;
      case CDK_KEY_Home:
        return NSHomeFunctionKey;
      case CDK_KEY_Left:
      case CDK_KEY_leftarrow:
        return NSLeftArrowFunctionKey;
      case CDK_KEY_Up:
      case CDK_KEY_uparrow:
        return NSUpArrowFunctionKey;
      case CDK_KEY_Right:
      case CDK_KEY_rightarrow:
        return NSRightArrowFunctionKey;
      case CDK_KEY_Down:
      case CDK_KEY_downarrow:
        return NSDownArrowFunctionKey;
      case CDK_KEY_Page_Up:
        return NSPageUpFunctionKey;
      case CDK_KEY_Page_Down:
        return NSPageDownFunctionKey;
      case CDK_KEY_End:
        return NSEndFunctionKey;
      case CDK_KEY_Begin:
        return NSBeginFunctionKey;
      case CDK_KEY_Select:
        return NSSelectFunctionKey;
      case CDK_KEY_Print:
        return NSPrintFunctionKey;
      case CDK_KEY_Execute:
        return NSExecuteFunctionKey;
      case CDK_KEY_Insert:
        return NSInsertFunctionKey;
      case CDK_KEY_Undo:
        return NSUndoFunctionKey;
      case CDK_KEY_Redo:
        return NSRedoFunctionKey;
      case CDK_KEY_Menu:
        return NSMenuFunctionKey;
      case CDK_KEY_Find:
        return NSFindFunctionKey;
      case CDK_KEY_Help:
        return NSHelpFunctionKey;
      case CDK_KEY_Break:
        return NSBreakFunctionKey;
      case CDK_KEY_Mode_switch:
        return NSModeSwitchFunctionKey;
      case CDK_KEY_F1:
        return NSF1FunctionKey;
      case CDK_KEY_F2:
        return NSF2FunctionKey;
      case CDK_KEY_F3:
        return NSF3FunctionKey;
      case CDK_KEY_F4:
        return NSF4FunctionKey;
      case CDK_KEY_F5:
        return NSF5FunctionKey;
      case CDK_KEY_F6:
        return NSF6FunctionKey;
      case CDK_KEY_F7:
        return NSF7FunctionKey;
      case CDK_KEY_F8:
        return NSF8FunctionKey;
      case CDK_KEY_F9:
        return NSF9FunctionKey;
      case CDK_KEY_F10:
        return NSF10FunctionKey;
      case CDK_KEY_F11:
        return NSF11FunctionKey;
      case CDK_KEY_F12:
        return NSF12FunctionKey;
      case CDK_KEY_F13:
        return NSF13FunctionKey;
      case CDK_KEY_F14:
        return NSF14FunctionKey;
      case CDK_KEY_F15:
        return NSF15FunctionKey;
      case CDK_KEY_F16:
        return NSF16FunctionKey;
      case CDK_KEY_F17:
        return NSF17FunctionKey;
      case CDK_KEY_F18:
        return NSF18FunctionKey;
      case CDK_KEY_F19:
        return NSF19FunctionKey;
      case CDK_KEY_F20:
        return NSF20FunctionKey;
      case CDK_KEY_F21:
        return NSF21FunctionKey;
      case CDK_KEY_F22:
        return NSF22FunctionKey;
      case CDK_KEY_F23:
        return NSF23FunctionKey;
      case CDK_KEY_F24:
        return NSF24FunctionKey;
      case CDK_KEY_F25:
        return NSF25FunctionKey;
      case CDK_KEY_F26:
        return NSF26FunctionKey;
      case CDK_KEY_F27:
        return NSF27FunctionKey;
      case CDK_KEY_F28:
        return NSF28FunctionKey;
      case CDK_KEY_F29:
        return NSF29FunctionKey;
      case CDK_KEY_F30:
        return NSF30FunctionKey;
      case CDK_KEY_F31:
        return NSF31FunctionKey;
      case CDK_KEY_F32:
        return NSF32FunctionKey;
      case CDK_KEY_F33:
        return NSF33FunctionKey;
      case CDK_KEY_F34:
        return NSF34FunctionKey;
      case CDK_KEY_F35:
        return NSF35FunctionKey;
      default:
        break;
    }

  return '\0';
}
