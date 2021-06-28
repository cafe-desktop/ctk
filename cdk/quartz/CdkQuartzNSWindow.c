/* CdkQuartzWindow.m
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
#import "CdkQuartzNSWindow.h"
#include "cdkquartzwindow.h"
#include "cdkdnd-quartz.h"
#include "cdkprivate-quartz.h"
#include "cdkinternal-quartz.h"
#include "cdkquartzdnd.h"

@implementation CdkQuartzNSWindow

- (void)windowWillClose:(NSNotification*)notification
{
  // Clears the delegate when window is going to be closed; since EL
  // Capitan it is possible that the methods of delegate would get
  // called after the window has been closed.
  [self setDelegate:nil];
}

-(BOOL)windowShouldClose:(id)sender
{
  CdkWindow *window = [[self contentView] cdkWindow];
  CdkEvent *event;

  event = cdk_event_new (GDK_DELETE);

  event->any.window = g_object_ref (window);
  event->any.send_event = FALSE;

  _cdk_event_queue_append (cdk_display_get_default (), event);

  return NO;
}

-(void)windowWillMiniaturize:(NSNotification *)aNotification
{
  CdkWindow *window = [[self contentView] cdkWindow];

  _cdk_quartz_window_detach_from_parent (window);
}

-(void)windowDidMiniaturize:(NSNotification *)aNotification
{
  CdkWindow *window = [[self contentView] cdkWindow];

  cdk_synthesize_window_state (window, 0, 
			       GDK_WINDOW_STATE_ICONIFIED);
}

-(void)windowDidDeminiaturize:(NSNotification *)aNotification
{
  CdkWindow *window = [[self contentView] cdkWindow];

  _cdk_quartz_window_attach_to_parent (window);

  cdk_synthesize_window_state (window, GDK_WINDOW_STATE_ICONIFIED, 0);
}

-(void)windowDidBecomeKey:(NSNotification *)aNotification
{
  CdkWindow *window = [[self contentView] cdkWindow];

  cdk_synthesize_window_state (window, 0, GDK_WINDOW_STATE_FOCUSED);
  _cdk_quartz_events_update_focus_window (window, TRUE);
}

-(void)windowDidResignKey:(NSNotification *)aNotification
{
  CdkWindow *window = [[self contentView] cdkWindow];

  _cdk_quartz_events_update_focus_window (window, FALSE);
  cdk_synthesize_window_state (window, GDK_WINDOW_STATE_FOCUSED, 0);
}

-(void)windowDidBecomeMain:(NSNotification *)aNotification
{
  CdkWindow *window = [[self contentView] cdkWindow];

  if (![self isVisible])
    {
      /* Note: This is a hack needed because for unknown reasons, hidden
       * windows get shown when clicking the dock icon when the application
       * is not already active.
       */
      [self orderOut:nil];
      return;
    }

  _cdk_quartz_window_did_become_main (window);
}

-(void)windowDidResignMain:(NSNotification *)aNotification
{
  CdkWindow *window;

  window = [[self contentView] cdkWindow];
  _cdk_quartz_window_did_resign_main (window);
}

/* Used in combination with NSLeftMouseUp in sendEvent to keep track
 * of when the window is being moved with the mouse.
 */
-(void)windowWillMove:(NSNotification *)aNotification
{
  inMove = YES;
}


#ifdef AVAILABLE_MAC_OS_X_VERSION_10_12_AND_LATER
#define NSLeftMouseUp NSEventTypeLeftMouseUp
#define NSLeftMouseDragged NSEventTypeLeftMouseDragged
#define NSMouseEntered NSEventTypeMouseEntered
#endif

-(void)sendEvent:(NSEvent *)event
{
  switch ([event type])
    {
    case NSLeftMouseUp:
    {
      double time = ((double)[event timestamp]) * 1000.0;

      _cdk_quartz_events_break_all_grabs (time);
      inManualMove = NO;
      inManualResize = NO;
      inMove = NO;
      break;
    }

    case NSLeftMouseDragged:
      if ([self trackManualMove] || [self trackManualResize])
        return;
      break;

    default:
      break;
    }

  [super sendEvent:event];
}

-(BOOL)isInMove
{
  return inMove;
}

-(void)checkSendEnterNotify
{
  CdkWindow *window = [[self contentView] cdkWindow];
  CdkWindowImplQuartz *impl = GDK_WINDOW_IMPL_QUARTZ (window->impl);

  /* When a new window has been created, and the mouse
   * is in the window area, we will not receive an NSMouseEntered
   * event.  Therefore, we synthesize an enter notify event manually.
   */
  if (!initialPositionKnown)
    {
      initialPositionKnown = YES;

      if (NSPointInRect ([NSEvent mouseLocation], [self frame]))
        {
          NSEvent *event;

          event = [NSEvent enterExitEventWithType: NSMouseEntered
                                         location: [self mouseLocationOutsideOfEventStream]
                                    modifierFlags: 0
                                        timestamp: [[NSApp currentEvent] timestamp]
                                     windowNumber: [impl->toplevel windowNumber]
                                          context: NULL
                                      eventNumber: 0
                                   trackingNumber: [impl->view trackingRect]
                                         userData: nil];

          [NSApp postEvent:event atStart:NO];
        }
    }
}

-(void)windowDidMove:(NSNotification *)aNotification
{
  CdkWindow *window = [[self contentView] cdkWindow];
  CdkEvent *event;

  gboolean maximized = cdk_window_get_state (window) & GDK_WINDOW_STATE_MAXIMIZED;

  /* In case the window is changed when maximized remove the maximized state */
  if (maximized && !inMaximizeTransition && !NSEqualRects (lastMaximizedFrame, [self frame]))
    {
      cdk_synthesize_window_state (window,
                                   GDK_WINDOW_STATE_MAXIMIZED,
                                   0);
    }

  _cdk_quartz_window_update_position (window);

  /* Synthesize a configure event */
  event = cdk_event_new (GDK_CONFIGURE);
  event->configure.window = g_object_ref (window);
  event->configure.x = window->x;
  event->configure.y = window->y;
  event->configure.width = window->width;
  event->configure.height = window->height;

  _cdk_event_queue_append (cdk_display_get_default (), event);

  [self checkSendEnterNotify];
}

-(void)windowDidResize:(NSNotification *)aNotification
{
  NSRect content_rect = [self contentRectForFrameRect:[self frame]];
  CdkWindow *window = [[self contentView] cdkWindow];
  CdkEvent *event;
  gboolean maximized = cdk_window_get_state (window) & GDK_WINDOW_STATE_MAXIMIZED;

  /* see same in windowDidMove */
  if (maximized && !inMaximizeTransition && !NSEqualRects (lastMaximizedFrame, [self frame]))
    {
      cdk_synthesize_window_state (window,
                                   GDK_WINDOW_STATE_MAXIMIZED,
                                   0);
    }

  window->width = content_rect.size.width;
  window->height = content_rect.size.height;

  /* Certain resize operations (e.g. going fullscreen), also move the
   * origin of the window.
   */
  _cdk_quartz_window_update_position (window);

  [[self contentView] setFrame:NSMakeRect (0, 0, window->width, window->height)];

  _cdk_window_update_size (window);

  /* Synthesize a configure event */
  event = cdk_event_new (GDK_CONFIGURE);
  event->configure.window = g_object_ref (window);
  event->configure.x = window->x;
  event->configure.y = window->y;
  event->configure.width = window->width;
  event->configure.height = window->height;

  _cdk_event_queue_append (cdk_display_get_default (), event);

  [self checkSendEnterNotify];
}

-(id)initWithContentRect:(NSRect)contentRect
#if MAC_OS_X_VERSION_MIN_REQUIRED < 101200
               styleMask:(NSUInteger)styleMask
#else
               styleMask:(NSWindowStyleMask)styleMask
#endif
                 backing:(NSBackingStoreType)backingType
                   defer:(BOOL)flag
                  screen:(NSScreen *)screen
{
  self = [super initWithContentRect:contentRect
	                  styleMask:styleMask
	                    backing:backingType
	                      defer:flag
                             screen:screen];

  [self setAcceptsMouseMovedEvents:YES];
  [self setDelegate:(id<NSWindowDelegate>)self];
  [self setReleasedWhenClosed:YES];

  return self;
}

-(BOOL)canBecomeMainWindow
{
  CdkWindow *window = [[self contentView] cdkWindow];
  CdkWindowImplQuartz *impl = GDK_WINDOW_IMPL_QUARTZ (window->impl);

  switch (impl->type_hint)
    {
    case GDK_WINDOW_TYPE_HINT_NORMAL:
    case GDK_WINDOW_TYPE_HINT_DIALOG:
      return YES;
      
    case GDK_WINDOW_TYPE_HINT_MENU:
    case GDK_WINDOW_TYPE_HINT_TOOLBAR:
    case GDK_WINDOW_TYPE_HINT_SPLASHSCREEN:
    case GDK_WINDOW_TYPE_HINT_UTILITY:
    case GDK_WINDOW_TYPE_HINT_DOCK:
    case GDK_WINDOW_TYPE_HINT_DESKTOP:
    case GDK_WINDOW_TYPE_HINT_DROPDOWN_MENU:
    case GDK_WINDOW_TYPE_HINT_POPUP_MENU:
    case GDK_WINDOW_TYPE_HINT_TOOLTIP:
    case GDK_WINDOW_TYPE_HINT_NOTIFICATION:
    case GDK_WINDOW_TYPE_HINT_COMBO:
    case GDK_WINDOW_TYPE_HINT_DND:
      return NO;
    }
  
  return YES;
}

-(BOOL)canBecomeKeyWindow
{
  CdkWindow *window = [[self contentView] cdkWindow];
  CdkWindowImplQuartz *impl = GDK_WINDOW_IMPL_QUARTZ (window->impl);

  if (!window->accept_focus)
    return NO;

  /* Popup windows should not be able to get focused in the window
   * manager sense, it's only handled through grabs.
   */
  if (window->window_type == GDK_WINDOW_TEMP)
    return NO;

  switch (impl->type_hint)
    {
    case GDK_WINDOW_TYPE_HINT_NORMAL:
    case GDK_WINDOW_TYPE_HINT_DIALOG:
    case GDK_WINDOW_TYPE_HINT_MENU:
    case GDK_WINDOW_TYPE_HINT_TOOLBAR:
    case GDK_WINDOW_TYPE_HINT_UTILITY:
    case GDK_WINDOW_TYPE_HINT_DOCK:
    case GDK_WINDOW_TYPE_HINT_DESKTOP:
    case GDK_WINDOW_TYPE_HINT_DROPDOWN_MENU:
    case GDK_WINDOW_TYPE_HINT_POPUP_MENU:
    case GDK_WINDOW_TYPE_HINT_COMBO:
      return YES;
      
    case GDK_WINDOW_TYPE_HINT_SPLASHSCREEN:
    case GDK_WINDOW_TYPE_HINT_TOOLTIP:
    case GDK_WINDOW_TYPE_HINT_NOTIFICATION:
    case GDK_WINDOW_TYPE_HINT_DND:
      return NO;
    }
  
  return YES;
}

- (void)showAndMakeKey:(BOOL)makeKey
{
  CdkWindow *window = [[self contentView] cdkWindow];
  CdkWindowImplQuartz *impl = GDK_WINDOW_IMPL_QUARTZ (window->impl);

  inShowOrHide = YES;

  if (makeKey)
    [impl->toplevel makeKeyAndOrderFront:impl->toplevel];
  else
    [impl->toplevel orderFront:nil];

  inShowOrHide = NO;

  [self checkSendEnterNotify];
}

- (void)hide
{
  CdkWindow *window = [[self contentView] cdkWindow];
  CdkWindowImplQuartz *impl = GDK_WINDOW_IMPL_QUARTZ (window->impl);

  inShowOrHide = YES;
  [impl->toplevel orderOut:nil];
  inShowOrHide = NO;

  initialPositionKnown = NO;
}

- (NSPoint)convertPointToScreen:(NSPoint)point
{
#if MAC_OS_X_VERSION_MAX_ALLOWED >= 101400
  if (cdk_quartz_osx_version () >= GDK_OSX_MOJAVE)
    return [super convertPointToScreen: point];
#endif
#if MAC_OS_X_VERSION_MIN_REQUIRED < 1070
  if (cdk_quartz_osx_version () < GDK_OSX_LION)
    return [self convertBaseToScreen:point];
#endif
  {
    NSRect inrect = NSMakeRect (point.x, point.y, 0.0, 0.0);
    NSRect outrect = [self convertRectToScreen: inrect];
    return outrect.origin;
  }
}

- (NSPoint)convertPointFromScreen:(NSPoint)point
{
#if MAC_OS_X_VERSION_MAX_ALLOWED >= 101400
  if (cdk_quartz_osx_version () >= GDK_OSX_MOJAVE)
    return [super convertPointFromScreen: point];
#endif
#if MAC_OS_X_VERSION_MIN_REQUIRED < 1070
  if (cdk_quartz_osx_version () < GDK_OSX_LION)
    return [self convertScreenToBase:point];
#endif
  {
    NSRect inrect = NSMakeRect (point.x, point.y, 0.0, 0.0);
    NSRect outrect = [self convertRectFromScreen: inrect];
    return outrect.origin;
  }
}

- (BOOL)trackManualMove
{
  CdkWindow *window = [[self contentView] cdkWindow];
  CdkWindowImplQuartz *impl = GDK_WINDOW_IMPL_QUARTZ (window->impl);
  NSPoint currentLocation;
  NSPoint newOrigin;
  NSRect screenFrame = [[NSScreen mainScreen] visibleFrame];
  NSRect windowFrame = [self frame];

  if (!inManualMove)
    return NO;

  currentLocation = [self convertPointToScreen:[self mouseLocationOutsideOfEventStream]];
  newOrigin.x = currentLocation.x - initialMoveLocation.x;
  newOrigin.y = currentLocation.y - initialMoveLocation.y;

  /* Clamp vertical position to below the menu bar. */
  if (newOrigin.y + windowFrame.size.height - impl->shadow_top > screenFrame.origin.y + screenFrame.size.height)
    newOrigin.y = screenFrame.origin.y + screenFrame.size.height - windowFrame.size.height + impl->shadow_top;

  [self setFrameOrigin:newOrigin];

  return YES;
}

/* Used by cdkevents-quartz.c to decide if our sendEvent() handler above
 * will see the event or if it will be subjected to standard processing
 * by GDK.
*/
-(BOOL)isInManualResizeOrMove
{
  return inManualResize || inManualMove;
}

-(void)beginManualMove
{
  NSRect frame = [self frame];

  if (inMove || inManualMove || inManualResize)
    return;

  inManualMove = YES;

  initialMoveLocation = [self convertPointToScreen:[self mouseLocationOutsideOfEventStream]];
  initialMoveLocation.x -= frame.origin.x;
  initialMoveLocation.y -= frame.origin.y;
}

- (BOOL)trackManualResize
{
  NSPoint mouse_location;
  NSRect new_frame;
  float mdx, mdy, dw, dh, dx, dy;
  NSSize min_size;

  if (!inManualResize || inTrackManualResize)
    return NO;

  inTrackManualResize = YES;
  mouse_location = [self convertPointToScreen:[self mouseLocationOutsideOfEventStream]];
  mdx = initialResizeLocation.x - mouse_location.x;
  mdy = initialResizeLocation.y - mouse_location.y;

  /* Set how a mouse location delta translates to changes in width,
   * height and position.
   */
  dw = dh = dx = dy = 0.0;
  if (resizeEdge == GDK_WINDOW_EDGE_EAST ||
      resizeEdge == GDK_WINDOW_EDGE_NORTH_EAST ||
      resizeEdge == GDK_WINDOW_EDGE_SOUTH_EAST)
    {
      dw = -1.0;
    }
  if (resizeEdge == GDK_WINDOW_EDGE_NORTH ||
      resizeEdge == GDK_WINDOW_EDGE_NORTH_WEST ||
      resizeEdge == GDK_WINDOW_EDGE_NORTH_EAST)
    {
      dh = -1.0;
    }
  if (resizeEdge == GDK_WINDOW_EDGE_SOUTH ||
      resizeEdge == GDK_WINDOW_EDGE_SOUTH_WEST ||
      resizeEdge == GDK_WINDOW_EDGE_SOUTH_EAST)
    {
      dh = 1.0;
      dy = -1.0;
    }
  if (resizeEdge == GDK_WINDOW_EDGE_WEST ||
      resizeEdge == GDK_WINDOW_EDGE_NORTH_WEST ||
      resizeEdge == GDK_WINDOW_EDGE_SOUTH_WEST)
    {
      dw = 1.0;
      dx = -1.0;
    }

  /* Apply changes to the frame captured when we started resizing */
  new_frame = initialResizeFrame;
  new_frame.origin.x += mdx * dx;
  new_frame.origin.y += mdy * dy;
  new_frame.size.width += mdx * dw;
  new_frame.size.height += mdy * dh;

  /* In case the resulting window would be too small reduce the
   * change to both size and position.
   */
  min_size = [self contentMinSize];

  if (new_frame.size.width < min_size.width)
    {
      if (dx)
        new_frame.origin.x -= min_size.width - new_frame.size.width;
      new_frame.size.width = min_size.width;
    }

  if (new_frame.size.height < min_size.height)
    {
      if (dy)
        new_frame.origin.y -= min_size.height - new_frame.size.height;
      new_frame.size.height = min_size.height;
    }

  /* We could also apply aspect ratio:
     new_frame.size.height = new_frame.size.width / [self aspectRatio].width * [self aspectRatio].height;
  */

  [self setFrame:new_frame display:YES];

  /* Let the resizing be handled by CTK+. */
  if (g_main_context_pending (NULL))
    g_main_context_iteration (NULL, FALSE);

  inTrackManualResize = NO;

  return YES;
}

-(void)beginManualResize:(CdkWindowEdge)edge
{
  if (inMove || inManualMove || inManualResize)
    return;

  inManualResize = YES;
  resizeEdge = edge;

  initialResizeFrame = [self frame];
  initialResizeLocation = [self convertPointToScreen:[self mouseLocationOutsideOfEventStream]];
}



static CdkDragContext *current_context = NULL;

static CdkDragAction
drag_operation_to_drag_action (NSDragOperation operation)
{
  CdkDragAction result = 0;

  /* GDK and Quartz drag operations do not map 1:1.
   * This mapping represents about the best that we
   * can come up.
   *
   * Note that NSDragOperationPrivate and GDK_ACTION_PRIVATE
   * have almost opposite meanings: the GDK one means that the
   * destination is solely responsible for the action; the Quartz
   * one means that the source and destination will agree
   * privately on the action. NSOperationGeneric is close in meaning
   * to GDK_ACTION_PRIVATE but there is a problem: it will be
   * sent for any ordinary drag, and likely not understood
   * by any intra-widget drag (since the source & dest are the
   * same).
   */

  if (operation & NSDragOperationGeneric)
    result |= GDK_ACTION_MOVE;
  if (operation & NSDragOperationCopy)
    result |= GDK_ACTION_COPY;
  if (operation & NSDragOperationMove)
    result |= GDK_ACTION_MOVE;
  if (operation & NSDragOperationLink)
    result |= GDK_ACTION_LINK;

  return result;
}

static NSDragOperation
drag_action_to_drag_operation (CdkDragAction action)
{
  NSDragOperation result = 0;

  if (action & GDK_ACTION_COPY)
    result |= NSDragOperationCopy;
  if (action & GDK_ACTION_LINK)
    result |= NSDragOperationLink;
  if (action & GDK_ACTION_MOVE)
    result |= NSDragOperationMove;

  return result;
}

static void
update_context_from_dragging_info (id <NSDraggingInfo> sender)
{
  g_assert (current_context != NULL);

  GDK_QUARTZ_DRAG_CONTEXT (current_context)->dragging_info = sender;
  current_context->suggested_action = drag_operation_to_drag_action ([sender draggingSourceOperationMask]);
  current_context->actions = current_context->suggested_action;
}

- (NSDragOperation)draggingEntered:(id <NSDraggingInfo>)sender
{
  CdkSeat *seat = NULL;
  CdkEvent *event;
  CdkWindow *window;

  if (current_context)
    g_object_unref (current_context);
  
  current_context = g_object_new (GDK_TYPE_QUARTZ_DRAG_CONTEXT, NULL);
  update_context_from_dragging_info (sender);

  window = [[self contentView] cdkWindow];

  current_context->display = cdk_window_get_display (window);

  seat = cdk_display_get_default_seat (cdk_display_get_default ());
  cdk_drag_context_set_device (current_context, cdk_seat_get_pointer (seat));
  event = cdk_event_new (GDK_DRAG_ENTER);
  event->dnd.window = g_object_ref (window);
  event->dnd.send_event = FALSE;
  event->dnd.context = g_object_ref (current_context);
  event->dnd.time = GDK_CURRENT_TIME;

  cdk_event_set_device (event, cdk_drag_context_get_device (current_context));
  cdk_event_set_seat (event, cdk_device_get_seat (cdk_drag_context_get_device (current_context)));

  _cdk_event_emit (event);

  cdk_event_free (event);

  return NSDragOperationNone;
}

- (void)draggingEnded:(id <NSDraggingInfo>)sender
{
  /* leave a note for the source about what action was taken */
  if (_cdk_quartz_drag_source_context && current_context)
   _cdk_quartz_drag_source_context->action = current_context->action;

  if (current_context)
    g_object_unref (current_context);
  current_context = NULL;
}

- (void)draggingExited:(id <NSDraggingInfo>)sender
{
  CdkEvent *event;
  
  event = cdk_event_new (GDK_DRAG_LEAVE);
  event->dnd.window = g_object_ref ([[self contentView] cdkWindow]);
  event->dnd.send_event = FALSE;
  event->dnd.context = g_object_ref (current_context);
  event->dnd.time = GDK_CURRENT_TIME;

  cdk_event_set_device (event, cdk_drag_context_get_device (current_context));
  cdk_event_set_seat (event, cdk_device_get_seat (cdk_drag_context_get_device (current_context)));

  _cdk_event_emit (event);

  cdk_event_free (event);
  
  g_object_unref (current_context);
  current_context = NULL;
}

- (NSDragOperation)draggingUpdated:(id <NSDraggingInfo>)sender
{
  NSPoint point = [sender draggingLocation];
  NSPoint screen_point = [self convertPointToScreen:point];
  CdkEvent *event;
  int gx, gy;

  update_context_from_dragging_info (sender);
  _cdk_quartz_window_nspoint_to_cdk_xy (screen_point, &gx, &gy);

  event = cdk_event_new (GDK_DRAG_MOTION);
  event->dnd.window = g_object_ref ([[self contentView] cdkWindow]);
  event->dnd.send_event = FALSE;
  event->dnd.context = g_object_ref (current_context);
  event->dnd.time = GDK_CURRENT_TIME;
  event->dnd.x_root = gx;
  event->dnd.y_root = gy;

  cdk_event_set_device (event, cdk_drag_context_get_device (current_context));
  cdk_event_set_seat (event, cdk_device_get_seat (cdk_drag_context_get_device (current_context)));

  _cdk_event_emit (event);

  cdk_event_free (event);

  return drag_action_to_drag_operation (current_context->action);
}

- (BOOL)performDragOperation:(id <NSDraggingInfo>)sender
{
  NSPoint point = [sender draggingLocation];
  NSPoint screen_point = [self convertPointToScreen:point];
  CdkEvent *event;
  int gy, gx;

  update_context_from_dragging_info (sender);
  _cdk_quartz_window_nspoint_to_cdk_xy (screen_point, &gx, &gy);

  event = cdk_event_new (GDK_DROP_START);
  event->dnd.window = g_object_ref ([[self contentView] cdkWindow]);
  event->dnd.send_event = FALSE;
  event->dnd.context = g_object_ref (current_context);
  event->dnd.time = GDK_CURRENT_TIME;
  event->dnd.x_root = gx;
  event->dnd.y_root = gy;

  cdk_event_set_device (event, cdk_drag_context_get_device (current_context));
  cdk_event_set_seat (event, cdk_device_get_seat (cdk_drag_context_get_device (current_context)));

  _cdk_event_emit (event);

  cdk_event_free (event);

  g_object_unref (current_context);
  current_context = NULL;

  return YES;
}

- (BOOL)wantsPeriodicDraggingUpdates
{
  return NO;
}

- (void)draggedImage:(NSImage *)anImage endedAt:(NSPoint)aPoint operation:(NSDragOperation)operation
{
  CdkEvent *event;
  CdkScreen *screen;
  CdkDevice *device;

  g_assert (_cdk_quartz_drag_source_context != NULL);

  event = cdk_event_new (GDK_DROP_FINISHED);
  event->dnd.window = g_object_ref ([[self contentView] cdkWindow]);
  event->dnd.send_event = FALSE;
  event->dnd.context = g_object_ref (_cdk_quartz_drag_source_context);

  screen = cdk_window_get_screen (event->dnd.window);

  if (screen)
    {
      GList* windows, *list;
      gint gx, gy;

      event->dnd.context->dest_window = NULL;

      windows = cdk_screen_get_toplevel_windows (screen);
      _cdk_quartz_window_nspoint_to_cdk_xy (aPoint, &gx, &gy);

      for (list = windows; list; list = list->next)
        {
          CdkWindow* win = (CdkWindow*) list->data;
          gint wx, wy;
          gint ww, wh;

          cdk_window_get_root_origin (win, &wx, &wy);
          ww = cdk_window_get_width (win);
          wh = cdk_window_get_height (win);

          if (gx > wx && gy > wy && gx <= wx + ww && gy <= wy + wh)
            event->dnd.context->dest_window = g_object_ref (win);
            break;
        }
    }

  device = cdk_drag_context_get_device (_cdk_quartz_drag_source_context);
  cdk_event_set_device (event, device);
  cdk_event_set_seat (event, cdk_device_get_seat (device));

  _cdk_event_emit (event);

  cdk_event_free (event);

  g_object_unref (_cdk_quartz_drag_source_context);
  _cdk_quartz_drag_source_context = NULL;
}

#ifdef AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER
#if MAC_OS_X_VERSION_MIN_REQUIRED < 101200
typedef enum
{
 GDK_QUARTZ_FULL_SCREEN_MASK = NSFullScreenWindowMask,
} CdkQuartzFullScreen;
#else
typedef enum
{
 GDK_QUARTZ_FULL_SCREEN_MASK = NSWindowStyleMaskFullScreen,
} CdkQuartzFullScreen;
#endif
- (void)setStyleMask:(NSUInteger)styleMask
{
  gboolean was_fullscreen;
  gboolean is_fullscreen;

  was_fullscreen = (([self styleMask] & GDK_QUARTZ_FULL_SCREEN_MASK) != 0);

  [super setStyleMask:styleMask];

  is_fullscreen = (([self styleMask] & GDK_QUARTZ_FULL_SCREEN_MASK) != 0);

  if (was_fullscreen != is_fullscreen)
    _cdk_quartz_window_update_fullscreen_state ([[self contentView] cdkWindow]);
}

#endif

- (NSRect)constrainFrameRect:(NSRect)frameRect toScreen:(NSScreen *)screen
{
  NSRect rect;
  CdkWindow *window = [[self contentView] cdkWindow];
  CdkWindowImplQuartz *impl = GDK_WINDOW_IMPL_QUARTZ (window->impl);

  /* Allow the window to move up "shadow_top" more than normally allowed
   * by the default impl. This makes it possible to move windows with
   * client side shadow right up to the screen's menu bar. */
  rect = [super constrainFrameRect:frameRect toScreen:screen];
  if (frameRect.origin.y > rect.origin.y)
    rect.origin.y = MIN (frameRect.origin.y, rect.origin.y + impl->shadow_top);

  return rect;
}

- (NSRect)windowWillUseStandardFrame:(NSWindow *)nsWindow
                        defaultFrame:(NSRect)newFrame
{
  NSRect screenFrame = [[self screen] visibleFrame];
  CdkWindow *window = [[self contentView] cdkWindow];
  gboolean maximized = cdk_window_get_state (window) & GDK_WINDOW_STATE_MAXIMIZED;

  if (!maximized)
    return screenFrame;
  else
    return lastUnmaximizedFrame;
}

- (BOOL)windowShouldZoom:(NSWindow *)nsWindow
                 toFrame:(NSRect)newFrame
{

  CdkWindow *window = [[self contentView] cdkWindow];
  gboolean maximized = cdk_window_get_state (window) & GDK_WINDOW_STATE_MAXIMIZED;

  if (maximized)
    {
      lastMaximizedFrame = newFrame;
      cdk_synthesize_window_state (window,
                                   GDK_WINDOW_STATE_MAXIMIZED,
                                   0);
    }
  else
    {
      lastUnmaximizedFrame = [nsWindow frame];
      cdk_synthesize_window_state (window,
                                   0,
                                   GDK_WINDOW_STATE_MAXIMIZED);
    }

  inMaximizeTransition = YES;
  return YES;
}

-(void)windowDidEndLiveResize:(NSNotification *)aNotification
{
  inMaximizeTransition = NO;
}

-(NSSize)window:(NSWindow *)window willUseFullScreenContentSize:(NSSize)proposedSize
{
  return [[window screen] frame].size;
}

-(void)windowWillEnterFullScreen:(NSNotification *)aNotification
{
  lastUnfullscreenFrame = [self frame];
}

-(void)windowWillExitFullScreen:(NSNotification *)aNotification
{
  [self setFrame:lastUnfullscreenFrame display:YES];
}

@end
