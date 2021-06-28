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

#ifndef __CDK_CURSOR_H__
#define __CDK_CURSOR_H__

#if !defined (__CDK_H_INSIDE__) && !defined (CDK_COMPILATION)
#error "Only <cdk/cdk.h> can be included directly."
#endif

#include <cdk/cdkversionmacros.h>
#include <cdk/cdktypes.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

G_BEGIN_DECLS

#define CDK_TYPE_CURSOR              (cdk_cursor_get_type ())
#define CDK_CURSOR(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), CDK_TYPE_CURSOR, CdkCursor))
#define CDK_IS_CURSOR(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), CDK_TYPE_CURSOR))


/**
 * CdkCursorType:
 * @CDK_X_CURSOR: ![](X_cursor.png)
 * @CDK_ARROW: ![](arrow.png)
 * @CDK_BASED_ARROW_DOWN: ![](based_arrow_down.png)
 * @CDK_BASED_ARROW_UP: ![](based_arrow_up.png)
 * @CDK_BOAT: ![](boat.png)
 * @CDK_BOGOSITY: ![](bogosity.png)
 * @CDK_BOTTOM_LEFT_CORNER: ![](bottom_left_corner.png)
 * @CDK_BOTTOM_RIGHT_CORNER: ![](bottom_right_corner.png)
 * @CDK_BOTTOM_SIDE: ![](bottom_side.png)
 * @CDK_BOTTOM_TEE: ![](bottom_tee.png)
 * @CDK_BOX_SPIRAL: ![](box_spiral.png)
 * @CDK_CENTER_PTR: ![](center_ptr.png)
 * @CDK_CIRCLE: ![](circle.png)
 * @CDK_CLOCK: ![](clock.png)
 * @CDK_COFFEE_MUG: ![](coffee_mug.png)
 * @CDK_CROSS: ![](cross.png)
 * @CDK_CROSS_REVERSE: ![](cross_reverse.png)
 * @CDK_CROSSHAIR: ![](crosshair.png)
 * @CDK_DIAMOND_CROSS: ![](diamond_cross.png)
 * @CDK_DOT: ![](dot.png)
 * @CDK_DOTBOX: ![](dotbox.png)
 * @CDK_DOUBLE_ARROW: ![](double_arrow.png)
 * @CDK_DRAFT_LARGE: ![](draft_large.png)
 * @CDK_DRAFT_SMALL: ![](draft_small.png)
 * @CDK_DRAPED_BOX: ![](draped_box.png)
 * @CDK_EXCHANGE: ![](exchange.png)
 * @CDK_FLEUR: ![](fleur.png)
 * @CDK_GOBBLER: ![](gobbler.png)
 * @CDK_GUMBY: ![](gumby.png)
 * @CDK_HAND1: ![](hand1.png)
 * @CDK_HAND2: ![](hand2.png)
 * @CDK_HEART: ![](heart.png)
 * @CDK_ICON: ![](icon.png)
 * @CDK_IRON_CROSS: ![](iron_cross.png)
 * @CDK_LEFT_PTR: ![](left_ptr.png)
 * @CDK_LEFT_SIDE: ![](left_side.png)
 * @CDK_LEFT_TEE: ![](left_tee.png)
 * @CDK_LEFTBUTTON: ![](leftbutton.png)
 * @CDK_LL_ANGLE: ![](ll_angle.png)
 * @CDK_LR_ANGLE: ![](lr_angle.png)
 * @CDK_MAN: ![](man.png)
 * @CDK_MIDDLEBUTTON: ![](middlebutton.png)
 * @CDK_MOUSE: ![](mouse.png)
 * @CDK_PENCIL: ![](pencil.png)
 * @CDK_PIRATE: ![](pirate.png)
 * @CDK_PLUS: ![](plus.png)
 * @CDK_QUESTION_ARROW: ![](question_arrow.png)
 * @CDK_RIGHT_PTR: ![](right_ptr.png)
 * @CDK_RIGHT_SIDE: ![](right_side.png)
 * @CDK_RIGHT_TEE: ![](right_tee.png)
 * @CDK_RIGHTBUTTON: ![](rightbutton.png)
 * @CDK_RTL_LOGO: ![](rtl_logo.png)
 * @CDK_SAILBOAT: ![](sailboat.png)
 * @CDK_SB_DOWN_ARROW: ![](sb_down_arrow.png)
 * @CDK_SB_H_DOUBLE_ARROW: ![](sb_h_double_arrow.png)
 * @CDK_SB_LEFT_ARROW: ![](sb_left_arrow.png)
 * @CDK_SB_RIGHT_ARROW: ![](sb_right_arrow.png)
 * @CDK_SB_UP_ARROW: ![](sb_up_arrow.png)
 * @CDK_SB_V_DOUBLE_ARROW: ![](sb_v_double_arrow.png)
 * @CDK_SHUTTLE: ![](shuttle.png)
 * @CDK_SIZING: ![](sizing.png)
 * @CDK_SPIDER: ![](spider.png)
 * @CDK_SPRAYCAN: ![](spraycan.png)
 * @CDK_STAR: ![](star.png)
 * @CDK_TARGET: ![](target.png)
 * @CDK_TCROSS: ![](tcross.png)
 * @CDK_TOP_LEFT_ARROW: ![](top_left_arrow.png)
 * @CDK_TOP_LEFT_CORNER: ![](top_left_corner.png)
 * @CDK_TOP_RIGHT_CORNER: ![](top_right_corner.png)
 * @CDK_TOP_SIDE: ![](top_side.png)
 * @CDK_TOP_TEE: ![](top_tee.png)
 * @CDK_TREK: ![](trek.png)
 * @CDK_UL_ANGLE: ![](ul_angle.png)
 * @CDK_UMBRELLA: ![](umbrella.png)
 * @CDK_UR_ANGLE: ![](ur_angle.png)
 * @CDK_WATCH: ![](watch.png)
 * @CDK_XTERM: ![](xterm.png)
 * @CDK_LAST_CURSOR: last cursor type
 * @CDK_BLANK_CURSOR: Blank cursor. Since 2.16
 * @CDK_CURSOR_IS_PIXMAP: type of cursors constructed with
 *   cdk_cursor_new_from_pixbuf()
 *
 * Predefined cursors.
 *
 * Note that these IDs are directly taken from the X cursor font, and many
 * of these cursors are either not useful, or are not available on other platforms.
 *
 * The recommended way to create cursors is to use cdk_cursor_new_from_name().
 */
typedef enum
{
  CDK_X_CURSOR 		  = 0,
  CDK_ARROW 		  = 2,
  CDK_BASED_ARROW_DOWN    = 4,
  CDK_BASED_ARROW_UP 	  = 6,
  CDK_BOAT 		  = 8,
  CDK_BOGOSITY 		  = 10,
  CDK_BOTTOM_LEFT_CORNER  = 12,
  CDK_BOTTOM_RIGHT_CORNER = 14,
  CDK_BOTTOM_SIDE 	  = 16,
  CDK_BOTTOM_TEE 	  = 18,
  CDK_BOX_SPIRAL 	  = 20,
  CDK_CENTER_PTR 	  = 22,
  CDK_CIRCLE 		  = 24,
  CDK_CLOCK	 	  = 26,
  CDK_COFFEE_MUG 	  = 28,
  CDK_CROSS 		  = 30,
  CDK_CROSS_REVERSE 	  = 32,
  CDK_CROSSHAIR 	  = 34,
  CDK_DIAMOND_CROSS 	  = 36,
  CDK_DOT 		  = 38,
  CDK_DOTBOX 		  = 40,
  CDK_DOUBLE_ARROW 	  = 42,
  CDK_DRAFT_LARGE 	  = 44,
  CDK_DRAFT_SMALL 	  = 46,
  CDK_DRAPED_BOX 	  = 48,
  CDK_EXCHANGE 		  = 50,
  CDK_FLEUR 		  = 52,
  CDK_GOBBLER 		  = 54,
  CDK_GUMBY 		  = 56,
  CDK_HAND1 		  = 58,
  CDK_HAND2 		  = 60,
  CDK_HEART 		  = 62,
  CDK_ICON 		  = 64,
  CDK_IRON_CROSS 	  = 66,
  CDK_LEFT_PTR 		  = 68,
  CDK_LEFT_SIDE 	  = 70,
  CDK_LEFT_TEE 		  = 72,
  CDK_LEFTBUTTON 	  = 74,
  CDK_LL_ANGLE 		  = 76,
  CDK_LR_ANGLE 	 	  = 78,
  CDK_MAN 		  = 80,
  CDK_MIDDLEBUTTON 	  = 82,
  CDK_MOUSE 		  = 84,
  CDK_PENCIL 		  = 86,
  CDK_PIRATE 		  = 88,
  CDK_PLUS 		  = 90,
  CDK_QUESTION_ARROW 	  = 92,
  CDK_RIGHT_PTR 	  = 94,
  CDK_RIGHT_SIDE 	  = 96,
  CDK_RIGHT_TEE 	  = 98,
  CDK_RIGHTBUTTON 	  = 100,
  CDK_RTL_LOGO 		  = 102,
  CDK_SAILBOAT 		  = 104,
  CDK_SB_DOWN_ARROW 	  = 106,
  CDK_SB_H_DOUBLE_ARROW   = 108,
  CDK_SB_LEFT_ARROW 	  = 110,
  CDK_SB_RIGHT_ARROW 	  = 112,
  CDK_SB_UP_ARROW 	  = 114,
  CDK_SB_V_DOUBLE_ARROW   = 116,
  CDK_SHUTTLE 		  = 118,
  CDK_SIZING 		  = 120,
  CDK_SPIDER		  = 122,
  CDK_SPRAYCAN 		  = 124,
  CDK_STAR 		  = 126,
  CDK_TARGET 		  = 128,
  CDK_TCROSS 		  = 130,
  CDK_TOP_LEFT_ARROW 	  = 132,
  CDK_TOP_LEFT_CORNER 	  = 134,
  CDK_TOP_RIGHT_CORNER 	  = 136,
  CDK_TOP_SIDE 		  = 138,
  CDK_TOP_TEE 		  = 140,
  CDK_TREK 		  = 142,
  CDK_UL_ANGLE 		  = 144,
  CDK_UMBRELLA 		  = 146,
  CDK_UR_ANGLE 		  = 148,
  CDK_WATCH 		  = 150,
  CDK_XTERM 		  = 152,
  CDK_LAST_CURSOR,
  CDK_BLANK_CURSOR        = -2,
  CDK_CURSOR_IS_PIXMAP 	  = -1
} CdkCursorType;

/* Cursors
 */

CDK_AVAILABLE_IN_ALL
GType      cdk_cursor_get_type           (void) G_GNUC_CONST;

CDK_AVAILABLE_IN_ALL
CdkCursor* cdk_cursor_new_for_display	 (CdkDisplay      *display,
					  CdkCursorType    cursor_type);
CDK_DEPRECATED_IN_3_16_FOR(cdk_cursor_new_for_display)
CdkCursor* cdk_cursor_new		 (CdkCursorType	   cursor_type);
CDK_AVAILABLE_IN_ALL
CdkCursor* cdk_cursor_new_from_pixbuf	 (CdkDisplay      *display,
					  GdkPixbuf       *pixbuf,
					  gint             x,
					  gint             y);
CDK_AVAILABLE_IN_3_10
CdkCursor* cdk_cursor_new_from_surface	 (CdkDisplay      *display,
					  cairo_surface_t *surface,
					  gdouble          x,
					  gdouble          y);
CDK_AVAILABLE_IN_ALL
CdkCursor*  cdk_cursor_new_from_name	 (CdkDisplay      *display,
					  const gchar     *name);
CDK_AVAILABLE_IN_ALL
CdkDisplay* cdk_cursor_get_display	 (CdkCursor	  *cursor);
CDK_DEPRECATED_IN_3_0_FOR(g_object_ref)
CdkCursor * cdk_cursor_ref               (CdkCursor       *cursor);
CDK_DEPRECATED_IN_3_0_FOR(g_object_unref)
void        cdk_cursor_unref             (CdkCursor       *cursor);
CDK_AVAILABLE_IN_ALL
GdkPixbuf*  cdk_cursor_get_image         (CdkCursor       *cursor);
CDK_AVAILABLE_IN_3_10
cairo_surface_t *cdk_cursor_get_surface  (CdkCursor       *cursor,
					  gdouble         *x_hot,
					  gdouble         *y_hot);
CDK_AVAILABLE_IN_ALL
CdkCursorType cdk_cursor_get_cursor_type (CdkCursor       *cursor);


G_END_DECLS

#endif /* __CDK_CURSOR_H__ */
