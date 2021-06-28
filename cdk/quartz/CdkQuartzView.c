/* CdkQuartzView.m
 *
 * Copyright (C) 2005-2007 Imendio AB
 * Copyright (C) 2011 Hiroyuki Yamamoto
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

#include <AvailabilityMacros.h>
#include "config.h"
#import "CdkQuartzView.h"
#include "cdkquartzwindow.h"
#include "cdkprivate-quartz.h"
#include "cdkquartz.h"
#include "cdkinternal-quartz.h"

@implementation CdkQuartzView

-(id)initWithFrame: (NSRect)frameRect
{
  if ((self = [super initWithFrame: frameRect]))
    {
      markedRange = NSMakeRange (NSNotFound, 0);
      selectedRange = NSMakeRange (0, 0);
    }
  [self setValue: @(YES) forKey: @"postsFrameChangedNotifications"];

  return self;
}

-(BOOL)acceptsFirstResponder
{
  CDK_NOTE (EVENTS, g_message ("acceptsFirstResponder"));
  return YES;
}

-(BOOL)becomeFirstResponder
{
  CDK_NOTE (EVENTS, g_message ("becomeFirstResponder"));
  return YES;
}

-(BOOL)resignFirstResponder
{
  CDK_NOTE (EVENTS, g_message ("resignFirstResponder"));
  return YES;
}

-(void) keyDown: (NSEvent *) theEvent
{
  /* NOTE: When user press Cmd+A, interpretKeyEvents: will call noop:
     method. When user press and hold A to show the accented char window,
     it consumed repeating key down events for key 'A' do NOT call
     any other method. We use this behavior to determine if this key
     down event is filtered by interpretKeyEvents.
  */

  g_object_set_data (G_OBJECT (cdk_window), GIC_FILTER_KEY,
                     GUINT_TO_POINTER (GIC_FILTER_FILTERED));

  CDK_NOTE (EVENTS, g_message ("keyDown"));
  [self interpretKeyEvents: [NSArray arrayWithObject: theEvent]];
}

-(void)flagsChanged: (NSEvent *) theEvent
{
}

-(NSUInteger)characterIndexForPoint: (NSPoint)aPoint
{
  CDK_NOTE (EVENTS, g_message ("characterIndexForPoint"));
  return 0;
}

-(NSRect)firstRectForCharacterRange: (NSRange)aRange actualRange: (NSRangePointer)actualRange
{
  CDK_NOTE (EVENTS, g_message ("firstRectForCharacterRange"));
  gint ns_x, ns_y;
  CdkRectangle *rect;

  rect = g_object_get_data (G_OBJECT (cdk_window), GIC_CURSOR_RECT);
  if (rect)
    {
      _cdk_quartz_window_cdk_xy_to_xy (rect->x, rect->y + rect->height,
				       &ns_x, &ns_y);

      return NSMakeRect (ns_x, ns_y, rect->width, rect->height);
    }
  else
    {
      return NSMakeRect (0, 0, 0, 0);
    }
}

-(NSArray *)validAttributesForMarkedText
{
  CDK_NOTE (EVENTS, g_message ("validAttributesForMarkedText"));
  return [NSArray arrayWithObjects: NSUnderlineStyleAttributeName, nil];
}

-(NSAttributedString *)attributedSubstringForProposedRange: (NSRange)aRange actualRange: (NSRangePointer)actualRange
{
  CDK_NOTE (EVENTS, g_message ("attributedSubstringForProposedRange"));
  return nil;
}

-(BOOL)hasMarkedText
{
  CDK_NOTE (EVENTS, g_message ("hasMarkedText"));
  return markedRange.location != NSNotFound && markedRange.length != 0;
}

-(NSRange)markedRange
{
  CDK_NOTE (EVENTS, g_message ("markedRange"));
  return markedRange;
}

-(NSRange)selectedRange
{
  CDK_NOTE (EVENTS, g_message ("selectedRange"));
  return selectedRange;
}

-(void)unmarkText
{
  CDK_NOTE (EVENTS, g_message ("unmarkText"));
  selectedRange = NSMakeRange (0, 0);
  markedRange = NSMakeRange (NSNotFound, 0);

  g_object_set_data_full (G_OBJECT (cdk_window), TIC_MARKED_TEXT, NULL, g_free);
}

-(void)setMarkedText: (id)aString selectedRange: (NSRange)newSelection replacementRange: (NSRange)replacementRange
{
  CDK_NOTE (EVENTS, g_message ("setMarkedText"));
  const char *str;

  if (replacementRange.location == NSNotFound)
    {
      markedRange = NSMakeRange (newSelection.location, [aString length]);
      selectedRange = NSMakeRange (newSelection.location, newSelection.length);
    }
  else {
      markedRange = NSMakeRange (replacementRange.location, [aString length]);
      selectedRange = NSMakeRange (replacementRange.location + newSelection.location, newSelection.length);
    }

  if ([aString isKindOfClass: [NSAttributedString class]])
    {
      str = [[aString string] UTF8String];
    }
  else {
      str = [aString UTF8String];
    }

  g_object_set_data_full (G_OBJECT (cdk_window), TIC_MARKED_TEXT, g_strdup (str), g_free);
  g_object_set_data (G_OBJECT (cdk_window), TIC_SELECTED_POS,
		     GUINT_TO_POINTER (selectedRange.location));
  g_object_set_data (G_OBJECT (cdk_window), TIC_SELECTED_LEN,
		     GUINT_TO_POINTER (selectedRange.length));

  CDK_NOTE (EVENTS, g_message ("setMarkedText: set %s (%p, nsview %p): %s",
			       TIC_MARKED_TEXT, cdk_window, self,
			       str ? str : "(empty)"));

  /* handle text input changes by mouse events */
  if (!GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (cdk_window),
                                            TIC_IN_KEY_DOWN)))
    {
      _cdk_quartz_synthesize_null_key_event(cdk_window);
    }
}

-(void)doCommandBySelector: (SEL)aSelector
{
  CDK_NOTE (EVENTS, g_message ("doCommandBySelector %s", aSelector));
  g_object_set_data (G_OBJECT (cdk_window), GIC_FILTER_KEY,
                     GUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)insertText: (id)aString replacementRange: (NSRange)replacementRange
{
  CDK_NOTE (EVENTS, g_message ("insertText"));
  const char *str;
  NSString *string;

  if ([self hasMarkedText])
    [self unmarkText];

  if ([aString isKindOfClass: [NSAttributedString class]])
      string = [aString string];
  else
      string = aString;

  NSCharacterSet *ctrlChars = [NSCharacterSet controlCharacterSet];
  NSCharacterSet *wsnlChars = [NSCharacterSet whitespaceAndNewlineCharacterSet];
  if ([string rangeOfCharacterFromSet:ctrlChars].length &&
      [string rangeOfCharacterFromSet:wsnlChars].length == 0)
    {
      /* discard invalid text input with Chinese input methods */
      str = "";
      [self unmarkText];
#if MAC_OS_X_VERSION_MIN_REQUIRED < 1060
      NSInputManager *currentInputManager = [NSInputManager currentInputManager];
      [currentInputManager markedTextAbandoned:self];
#else
      [[NSTextInputContext currentInputContext] discardMarkedText];
#endif
    }
  else
   {
      str = [string UTF8String];
      selectedRange = NSMakeRange ([string length], 0);
   }

  if (replacementRange.length > 0)
    {
      g_object_set_data (G_OBJECT (cdk_window), TIC_INSERT_TEXT_REPLACE_LEN,
                         GINT_TO_POINTER (replacementRange.length));
    }

  g_object_set_data_full (G_OBJECT (cdk_window), TIC_INSERT_TEXT, g_strdup (str), g_free);
  CDK_NOTE (EVENTS, g_message ("insertText: set %s (%p, nsview %p): %s",
			     TIC_INSERT_TEXT, cdk_window, self,
			     str ? str : "(empty)"));

  g_object_set_data (G_OBJECT (cdk_window), GIC_FILTER_KEY,
		     GUINT_TO_POINTER (GIC_FILTER_FILTERED));

  /* handle text input changes by mouse events */
  if (!GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (cdk_window),
                                            TIC_IN_KEY_DOWN)))
    {
      _cdk_quartz_synthesize_null_key_event(cdk_window);
    }
}
/* --------------------------------------------------------------- */

-(void)dealloc
{
  if (trackingRect)
    {
      [self removeTrackingRect: trackingRect];
      trackingRect = 0;
    }

  [super dealloc];
}

-(void)setCdkWindow: (CdkWindow *)window
{
  cdk_window = window;
}

-(CdkWindow *)cdkWindow
{
  return cdk_window;
}

-(NSTrackingRectTag)trackingRect
{
  return trackingRect;
}

-(BOOL)isFlipped
{
  return YES;
}

-(BOOL)isOpaque
{
  if (CDK_WINDOW_DESTROYED (cdk_window))
    return YES;

  /* A view is opaque if its CdkWindow doesn't have the RGBA visual */
  return cdk_window_get_visual (cdk_window) !=
    cdk_screen_get_rgba_visual (_cdk_screen);
}

-(void)drawRect: (NSRect)rect
{
  CdkRectangle cdk_rect;
  CdkWindowImplQuartz *impl = CDK_WINDOW_IMPL_QUARTZ (cdk_window->impl);
  const NSRect *drawn_rects;
  NSInteger count;
  int i;
  cairo_region_t *region;

  if (CDK_WINDOW_DESTROYED (cdk_window))
    return;

  if (! (cdk_window->event_mask & CDK_EXPOSURE_MASK))
    return;

  if (NSEqualRects (rect, NSZeroRect))
    return;

  if (!CDK_WINDOW_IS_MAPPED (cdk_window))
    {
      /* If the window is not yet mapped, clip_region_with_children
       * will be empty causing the usual code below to draw nothing.
       * To not see garbage on the screen, we draw an aesthetic color
       * here. The garbage would be visible if any widget enabled
       * the NSView's CALayer in order to add sublayers for custom
       * native rendering.
       */
      [NSGraphicsContext saveGraphicsState];

      [[NSColor windowBackgroundColor] setFill];
      [NSBezierPath fillRect: rect];

      [NSGraphicsContext restoreGraphicsState];

      return;
    }

  /* Clear our own bookkeeping of regions that need display */
  if (impl->needs_display_region)
    {
      cairo_region_destroy (impl->needs_display_region);
      impl->needs_display_region = NULL;
    }

  [self getRectsBeingDrawn: &drawn_rects count: &count];
  region = cairo_region_create ();

  for (i = 0; i < count; i++)
    {
      cdk_rect.x = drawn_rects[i].origin.x;
      cdk_rect.y = drawn_rects[i].origin.y;
      cdk_rect.width = drawn_rects[i].size.width;
      cdk_rect.height = drawn_rects[i].size.height;

      cairo_region_union_rectangle (region, &cdk_rect);
    }

  impl->in_paint_rect_count++;
  _cdk_window_process_updates_recurse (cdk_window, region);
  impl->in_paint_rect_count--;

  cairo_region_destroy (region);

  if (needsInvalidateShadow)
    {
      [[self window] invalidateShadow];
      needsInvalidateShadow = NO;
    }
}

-(void)setNeedsInvalidateShadow: (BOOL)invalidate
{
  needsInvalidateShadow = invalidate;
}

/* For information on setting up tracking rects properly, see here:
 * http://developer.apple.com/documentation/Cocoa/Conceptual/EventOverview/EventOverview.pdf
 */
-(void)updateTrackingRect
{
  CdkWindowImplQuartz *impl = CDK_WINDOW_IMPL_QUARTZ (cdk_window->impl);
  NSRect rect;

  if (!impl || !impl->toplevel)
    return;

  if (trackingRect)
    {
      [self removeTrackingRect: trackingRect];
      trackingRect = 0;
    }

  if (!impl->toplevel)
    return;

  /* Note, if we want to set assumeInside we can use:
   * NSPointInRect ([[self window] convertScreenToBase:[NSEvent mouseLocation]], rect)
   */

  rect = [self bounds];
  trackingRect = [self addTrackingRect: rect
		  owner: self
		  userData: nil
		  assumeInside: NO];
}

-(void)viewDidMoveToWindow
{
  if (![self window]) /* We are destroyed already */
    return;

  [self updateTrackingRect];
}

-(void)viewWillMoveToWindow: (NSWindow *)newWindow
{
  if (newWindow == nil && trackingRect)
    {
      [self removeTrackingRect: trackingRect];
      trackingRect = 0;
    }
}

-(void)setFrame: (NSRect)frame
{
  if (CDK_WINDOW_DESTROYED (cdk_window))
    return;
  
  [super setFrame: frame];

  if ([self window])
    [self updateTrackingRect];
}

@end
