/* GDK - The GIMP Drawing Kit
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

#include "config.h"

#include "cdkeventtranslator.h"
#include "cdkwindow-x11.h"

typedef CdkEventTranslatorIface CdkEventTranslatorInterface;
G_DEFINE_INTERFACE (CdkEventTranslator, _cdk_x11_event_translator, G_TYPE_OBJECT);


static void
_cdk_x11_event_translator_default_init (CdkEventTranslatorInterface *iface)
{
}


CdkEvent *
_cdk_x11_event_translator_translate (CdkEventTranslator *translator,
                                     CdkDisplay         *display,
                                     XEvent             *xevent)
{
  CdkEventTranslatorIface *iface;
  CdkEvent *event;

  g_return_val_if_fail (GDK_IS_EVENT_TRANSLATOR (translator), NULL);
  g_return_val_if_fail (GDK_IS_DISPLAY (display), NULL);

  iface = GDK_EVENT_TRANSLATOR_GET_IFACE (translator);

  if (!iface->translate_event)
    return NULL;

  event = cdk_event_new (GDK_NOTHING);

  if ((iface->translate_event) (translator, display, event, xevent))
    return event;

  cdk_event_free (event);

  return NULL;
}

CdkEventMask
_cdk_x11_event_translator_get_handled_events (CdkEventTranslator *translator)
{
  CdkEventTranslatorIface *iface;

  g_return_val_if_fail (GDK_IS_EVENT_TRANSLATOR (translator), 0);

  iface = GDK_EVENT_TRANSLATOR_GET_IFACE (translator);

  if (iface->get_handled_events)
    return iface->get_handled_events (translator);

  return 0;
}

void
_cdk_x11_event_translator_select_window_events (CdkEventTranslator *translator,
                                                Window              window,
                                                CdkEventMask        event_mask)
{
  CdkEventTranslatorIface *iface;

  g_return_if_fail (GDK_IS_EVENT_TRANSLATOR (translator));

  iface = GDK_EVENT_TRANSLATOR_GET_IFACE (translator);

  if (iface->select_window_events)
    iface->select_window_events (translator, window, event_mask);
}

CdkWindow *
_cdk_x11_event_translator_get_window (CdkEventTranslator *translator,
                                      CdkDisplay         *display,
                                      XEvent             *xevent)
{
  CdkEventTranslatorIface *iface;

  g_return_val_if_fail (GDK_IS_EVENT_TRANSLATOR (translator), NULL);

  iface = GDK_EVENT_TRANSLATOR_GET_IFACE (translator);

  if (iface->get_window)
    return iface->get_window (translator, xevent);

  return NULL;
}
