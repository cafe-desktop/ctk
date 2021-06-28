/* cdkcursor-quartz.c
 *
 * Copyright (C) 2005-2007 Imendio AB
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

#include "cdkdisplay.h"
#include "cdkcursor.h"
#include "cdkcursorprivate.h"
#include "cdkquartzcursor.h"
#include "cdkprivate-quartz.h"
#include "cdkinternal-quartz.h"
#include "cdkquartz-ctk-only.h"

#include "xcursors.h"

struct _CdkQuartzCursor
{
  CdkCursor cursor;

  NSCursor *nscursor;
};

struct _CdkQuartzCursorClass
{
  CdkCursorClass cursor_class;
};


static CdkCursor *cached_xcursors[G_N_ELEMENTS (xcursors)];

static CdkCursor *
cdk_quartz_cursor_new_from_nscursor (NSCursor      *nscursor,
                                     CdkCursorType  cursor_type)
{
  CdkQuartzCursor *private;

  private = g_object_new (CDK_TYPE_QUARTZ_CURSOR,
                          "cursor-type", cursor_type,
                          "display", _cdk_display,
                          NULL);
  private->nscursor = nscursor;

  return CDK_CURSOR (private);
}

static CdkCursor *
create_blank_cursor (void)
{
  NSCursor *nscursor;
  NSImage *nsimage;
  NSSize size = { 1.0, 1.0 };

  nsimage = [[NSImage alloc] initWithSize:size];
  nscursor = [[NSCursor alloc] initWithImage:nsimage
                               hotSpot:NSMakePoint(0.0, 0.0)];
  [nsimage release];

  return cdk_quartz_cursor_new_from_nscursor (nscursor, CDK_BLANK_CURSOR);
}

static gboolean
get_bit (const guchar *data,
         gint          width,
         gint          height,
         gint          x,
         gint          y)
{
  gint bytes_per_line;
  const guchar *src;

  if (x < 0 || y < 0 || x >= width || y >= height)
    return FALSE;

  bytes_per_line = (width + 7) / 8;

  src = &data[y * bytes_per_line];
  return ((src[x / 8] >> x % 8) & 1);
}

static CdkCursor *
create_builtin_cursor (CdkCursorType cursor_type)
{
  CdkCursor *cursor;
  NSBitmapImageRep *bitmap_rep;
  NSInteger mask_width, mask_height;
  gint src_width, src_height;
  gint dst_stride;
  const guchar *mask_start, *src_start;
  gint dx, dy;
  gint x, y;
  NSPoint hotspot;
  NSImage *image;
  NSCursor *nscursor;

  if (cursor_type >= G_N_ELEMENTS (xcursors) || cursor_type < 0)
    return NULL;

  cursor = cached_xcursors[cursor_type];
  if (cursor)
    return cursor;

  CDK_QUARTZ_ALLOC_POOL;

  src_width = xcursors[cursor_type].width;
  src_height = xcursors[cursor_type].height;
  mask_width = xcursors[cursor_type+1].width;
  mask_height = xcursors[cursor_type+1].height;

  bitmap_rep = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL
		pixelsWide:mask_width pixelsHigh:mask_height
		bitsPerSample:8 samplesPerPixel:4
		hasAlpha:YES isPlanar:NO colorSpaceName:NSDeviceRGBColorSpace
		bytesPerRow:0 bitsPerPixel:0];

  dst_stride = [bitmap_rep bytesPerRow];

  src_start = xcursors[cursor_type].bits;
  mask_start = xcursors[cursor_type+1].bits;

  dx = xcursors[cursor_type+1].hotx - xcursors[cursor_type].hotx;
  dy = xcursors[cursor_type+1].hoty - xcursors[cursor_type].hoty;

  for (y = 0; y < mask_height; y++)
    {
      guchar *dst = [bitmap_rep bitmapData] + y * dst_stride;

      for (x = 0; x < mask_width; x++)
	{
	  if (get_bit (mask_start, mask_width, mask_height, x, y))
            {
              if (get_bit (src_start, src_width, src_height, x - dx, y - dy))
                {
                  *dst++ = 0;
                  *dst++ = 0;
                  *dst++ = 0;
                }
              else
                {
                  *dst++ = 0xff;
                  *dst++ = 0xff;
                  *dst++ = 0xff;
                }

              *dst++ = 0xff;
            }
	  else
            {
              *dst++ = 0;
              *dst++ = 0;
              *dst++ = 0;
              *dst++ = 0;
            }
        }
    }

  image = [[NSImage alloc] init];
  [image addRepresentation:bitmap_rep];
  [bitmap_rep release];

  hotspot = NSMakePoint (xcursors[cursor_type+1].hotx,
                         xcursors[cursor_type+1].hoty);

  nscursor = [[NSCursor alloc] initWithImage:image hotSpot:hotspot];
  [image release];

  cursor = cdk_quartz_cursor_new_from_nscursor (nscursor, CDK_CURSOR_IS_PIXMAP);

  cached_xcursors[cursor_type] = g_object_ref (cursor);

  CDK_QUARTZ_RELEASE_POOL;

  return cursor;
}

CdkCursor*
_cdk_quartz_display_get_cursor_for_type (CdkDisplay    *display,
                                         CdkCursorType  cursor_type)
{
  NSCursor *nscursor;

  g_return_val_if_fail (display == cdk_display_get_default (), NULL);

  switch (cursor_type)
    {
    case CDK_XTERM:
      nscursor = [NSCursor IBeamCursor];
      break;
    case CDK_SB_H_DOUBLE_ARROW:
      nscursor = [NSCursor resizeLeftRightCursor];
      break;
    case CDK_SB_V_DOUBLE_ARROW:
      nscursor = [NSCursor resizeUpDownCursor];
      break;
    case CDK_SB_UP_ARROW:
    case CDK_BASED_ARROW_UP:
    case CDK_BOTTOM_TEE:
    case CDK_TOP_SIDE:
      nscursor = [NSCursor resizeUpCursor];
      break;
    case CDK_SB_DOWN_ARROW:
    case CDK_BASED_ARROW_DOWN:
    case CDK_TOP_TEE:
    case CDK_BOTTOM_SIDE:
      nscursor = [NSCursor resizeDownCursor];
      break;
    case CDK_SB_LEFT_ARROW:
    case CDK_RIGHT_TEE:
    case CDK_LEFT_SIDE:
      nscursor = [NSCursor resizeLeftCursor];
      break;
    case CDK_SB_RIGHT_ARROW:
    case CDK_LEFT_TEE:
    case CDK_RIGHT_SIDE:
      nscursor = [NSCursor resizeRightCursor];
      break;
    case CDK_TCROSS:
    case CDK_CROSS:
    case CDK_CROSSHAIR:
    case CDK_DIAMOND_CROSS:
      nscursor = [NSCursor crosshairCursor];
      break;
    case CDK_HAND1:
    case CDK_HAND2:
      nscursor = [NSCursor pointingHandCursor];
      break;
    case CDK_CURSOR_IS_PIXMAP:
      return NULL;
    case CDK_BLANK_CURSOR:
      return create_blank_cursor ();
    default:
      return g_object_ref (create_builtin_cursor (cursor_type));
    }

  [nscursor retain];
  return cdk_quartz_cursor_new_from_nscursor (nscursor, cursor_type);
}


CdkCursor *
_cdk_quartz_display_get_cursor_for_surface (CdkDisplay      *display,
					    cairo_surface_t *surface,
					    gdouble          x,
					    gdouble          y)
{
  NSImage *image;
  NSCursor *nscursor;
  CdkCursor *cursor;
  CdkPixbuf *pixbuf;

  CDK_QUARTZ_ALLOC_POOL;

  pixbuf = cdk_pixbuf_get_from_surface (surface, 0, 0,
					cairo_image_surface_get_width (surface),
					cairo_image_surface_get_height (surface));
  image = cdk_quartz_pixbuf_to_ns_image_libctk_only (pixbuf);
  nscursor = [[NSCursor alloc] initWithImage:image hotSpot:NSMakePoint(x, y)];

  cursor = cdk_quartz_cursor_new_from_nscursor (nscursor, CDK_CURSOR_IS_PIXMAP);

  g_object_unref (pixbuf);

  CDK_QUARTZ_RELEASE_POOL;

  return cursor;
}

/* OS X only exports a number of cursor types in its public NSCursor interface.
 * By overriding the private _coreCursorType method, we can tell OS X to load
 * one of its internal cursors instead (since cursor images are loaded on demand
 * instead of in advance). WebKit does this too.
 */

@interface cdkCoreCursor : NSCursor {
@private
	int type;
	BOOL override;
}
@end

@implementation cdkCoreCursor

- (int)_coreCursorType
{
	if (self->override)
		return self->type;
	return [super _coreCursorType];
}

#define CUSTOM_CURSOR_CTOR(name, id) \
	+ (cdkCoreCursor *)name \
	{ \
		cdkCoreCursor *obj; \
		obj = [self new]; \
		if (obj) { \
			obj->override = YES; \
			obj->type = id; \
		} \
		return obj; \
	}
CUSTOM_CURSOR_CTOR(cdkHelpCursor, 40)
CUSTOM_CURSOR_CTOR(cdkProgressCursor, 4)
/* TODO OS X doesn't seem to have a way to get this. There is an undocumented
 * method +[NSCursor _waitCursor], but it doesn't actually return this cursor,
 * but rather some odd low-quality non-animating version of this cursor. Use
 * the progress cursor instead for now.
 */
CUSTOM_CURSOR_CTOR(cdkWaitCursor, 4)
CUSTOM_CURSOR_CTOR(cdkAliasCursor, 2)
CUSTOM_CURSOR_CTOR(cdkMoveCursor, 39)
/* TODO OS X doesn't seem to provide one; copy the move cursor for now
 *  since it looks similar to what we want. */
CUSTOM_CURSOR_CTOR(cdkAllScrollCursor, 39)
CUSTOM_CURSOR_CTOR(cdkNEResizeCursor, 29)
CUSTOM_CURSOR_CTOR(cdkNWResizeCursor, 33)
CUSTOM_CURSOR_CTOR(cdkSEResizeCursor, 35)
CUSTOM_CURSOR_CTOR(cdkSWResizeCursor, 37)
CUSTOM_CURSOR_CTOR(cdkEWResizeCursor, 28)
CUSTOM_CURSOR_CTOR(cdkNSResizeCursor, 32)
CUSTOM_CURSOR_CTOR(cdkNESWResizeCursor, 30)
CUSTOM_CURSOR_CTOR(cdkNWSEResizeCursor, 34)
CUSTOM_CURSOR_CTOR(cdkZoomInCursor, 42)
CUSTOM_CURSOR_CTOR(cdkZoomOutCursor, 43)

@end

struct CursorsByName {
  const gchar *name;
  NSString *selector;
};

static const struct CursorsByName cursors_by_name[] = {
  /* Link & Status */
  { "context-menu", @"contextualMenuCursor" },
  { "help", @"cdkHelpCursor" },
  { "pointer", @"pointingHandCursor" },
  { "progress", @"cdkProgressCursor" },
  { "wait", @"cdkWaitCursor" },
  /* Selection */
  { "cell", @"crosshairCursor" },
  { "crosshair", @"crosshairCursor" },
  { "text", @"IBeamCursor" },
  { "vertical-text", @"IBeamCursorForVerticalLayout" },
  /* Drag & Drop */
  { "alias", @"cdkAliasCursor" },
  { "copy", @"dragCopyCursor" },
  { "move", @"cdkMoveCursor" },
  { "no-drop", @"operationNotAllowedCursor" },
  { "not-allowed", @"operationNotAllowedCursor" },
  { "grab", @"openHandCursor" },
  { "grabbing", @"closedHandCursor" },
  /* Resize & Scrolling */
  { "all-scroll", @"cdkAllScrollCursor" },
  { "col-resize", @"resizeLeftRightCursor" },
  { "row-resize", @"resizeUpDownCursor" },
  { "n-resize", @"resizeUpCursor" },
  { "e-resize", @"resizeRightCursor" },
  { "s-resize", @"resizeDownCursor" },
  { "w-resize", @"resizeLeftCursor" },
  { "ne-resize", @"cdkNEResizeCursor" },
  { "nw-resize", @"cdkNWResizeCursor" },
  { "se-resize", @"cdkSEResizeCursor" },
  { "sw-resize", @"cdkSWResizeCursor" },
  { "ew-resize", @"cdkEWResizeCursor" },
  { "ns-resize", @"cdkNSResizeCursor" },
  { "nesw-resize", @"cdkNESWResizeCursor" },
  { "nwse-resize", @"cdkNWSEResizeCursor" },
  /* Zoom */
  { "zoom-in", @"cdkZoomInCursor" },
  { "zoom-out", @"cdkZoomOutCursor" },
  { NULL, NULL },
};

CdkCursor*
_cdk_quartz_display_get_cursor_for_name (CdkDisplay  *display,
                                         const gchar *name)
{
  NSCursor *nscursor;
  const struct CursorsByName *test;
  SEL selector;

  if (name == NULL || g_str_equal (name, "none"))
    return create_blank_cursor ();

  // use this selector if nothing found
  selector = @selector(arrowCursor);
  for (test = cursors_by_name; test->name != NULL; test++)
    if (g_str_equal (name, test->name))
      {
        selector = NSSelectorFromString(test->selector);
        break;
      }
  nscursor = [[cdkCoreCursor class] performSelector:selector];

  [nscursor retain];
  return cdk_quartz_cursor_new_from_nscursor (nscursor, CDK_CURSOR_IS_PIXMAP);
}

G_DEFINE_TYPE (CdkQuartzCursor, cdk_quartz_cursor, CDK_TYPE_CURSOR)

static cairo_surface_t *cdk_quartz_cursor_get_surface (CdkCursor *cursor,
						       gdouble *x_hot,
						       gdouble *y_hot);

static void
cdk_quartz_cursor_finalize (GObject *object)
{
  CdkQuartzCursor *private = CDK_QUARTZ_CURSOR (object);

  if (private->nscursor)
    [private->nscursor release];
  private->nscursor = NULL;
}

static void
cdk_quartz_cursor_class_init (CdkQuartzCursorClass *quartz_cursor_class)
{
  CdkCursorClass *cursor_class = CDK_CURSOR_CLASS (quartz_cursor_class);
  GObjectClass *object_class = G_OBJECT_CLASS (quartz_cursor_class);

  object_class->finalize = cdk_quartz_cursor_finalize;

  cursor_class->get_surface = cdk_quartz_cursor_get_surface;
}

static void
cdk_quartz_cursor_init (CdkQuartzCursor *cursor)
{
}


gboolean
_cdk_quartz_display_supports_cursor_alpha (CdkDisplay *display)
{
  return TRUE;
}

gboolean
_cdk_quartz_display_supports_cursor_color (CdkDisplay *display)
{
  return TRUE;
}

void
_cdk_quartz_display_get_default_cursor_size (CdkDisplay *display,
                                             guint      *width,
                                             guint      *height)
{
  /* Mac OS X doesn't have the notion of a default size */
  *width = 32;
  *height = 32;
}

void
_cdk_quartz_display_get_maximal_cursor_size (CdkDisplay *display,
                                             guint       *width,
                                             guint       *height)
{
  /* Cursor sizes in Mac OS X can be arbitrarily large */
  *width = 65536;
  *height = 65536;
}

NSCursor *
_cdk_quartz_cursor_get_ns_cursor (CdkCursor *cursor)
{
  CdkQuartzCursor *cursor_private;

  if (!cursor)
    return [NSCursor arrowCursor];

  g_return_val_if_fail (CDK_IS_QUARTZ_CURSOR (cursor), NULL);

  cursor_private = CDK_QUARTZ_CURSOR (cursor);

  return cursor_private->nscursor;
}

static cairo_surface_t *
cdk_quartz_cursor_get_surface (CdkCursor *cursor,
			       gdouble *x_hot,
			       gdouble *y_hot)
{
  /* FIXME: Implement */
  return NULL;
}
