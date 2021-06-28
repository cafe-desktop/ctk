/*
 * Copyright © 2013 Canonical Limited
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the licence, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#include "config.h"

#include "ctkapplicationprivate.h"

#ifdef GDK_WINDOWING_X11
#include <cdk/x11/cdkx.h>
#endif

#ifdef GDK_WINDOWING_WAYLAND
#include <cdk/wayland/cdkwayland.h>
#endif

#ifdef GDK_WINDOWING_QUARTZ
#include <cdk/quartz/cdkquartz.h>
#endif

G_DEFINE_TYPE (CtkApplicationImpl, ctk_application_impl, G_TYPE_OBJECT)

static void
ctk_application_impl_init (CtkApplicationImpl *impl)
{
}

static guint do_nothing (void) { return 0; }
static gboolean return_false (void) { return FALSE; }

static void
ctk_application_impl_class_init (CtkApplicationImplClass *class)
{
  /* NB: can only 'do_nothing' for functions with integer or void return */
  class->startup = (gpointer) do_nothing;
  class->shutdown = (gpointer) do_nothing;
  class->before_emit = (gpointer) do_nothing;
  class->window_added = (gpointer) do_nothing;
  class->window_removed = (gpointer) do_nothing;
  class->active_window_changed = (gpointer) do_nothing;
  class->handle_window_realize = (gpointer) do_nothing;
  class->handle_window_map = (gpointer) do_nothing;
  class->set_app_menu = (gpointer) do_nothing;
  class->set_menubar = (gpointer) do_nothing;
  class->inhibit = (gpointer) do_nothing;
  class->uninhibit = (gpointer) do_nothing;
  class->is_inhibited = (gpointer) do_nothing;
  class->prefers_app_menu = (gpointer) return_false;
}

void
ctk_application_impl_startup (CtkApplicationImpl *impl,
                              gboolean            register_session)
{
  CTK_APPLICATION_IMPL_GET_CLASS (impl)->startup (impl, register_session);
}

void
ctk_application_impl_shutdown (CtkApplicationImpl *impl)
{
  CTK_APPLICATION_IMPL_GET_CLASS (impl)->shutdown (impl);
}

void
ctk_application_impl_before_emit (CtkApplicationImpl *impl,
                                  GVariant           *platform_data)
{
  CTK_APPLICATION_IMPL_GET_CLASS (impl)->before_emit (impl, platform_data);
}

void
ctk_application_impl_window_added (CtkApplicationImpl *impl,
                                   CtkWindow          *window)
{
  CTK_APPLICATION_IMPL_GET_CLASS (impl)->window_added (impl, window);
}

void
ctk_application_impl_window_removed (CtkApplicationImpl *impl,
                                     CtkWindow          *window)
{
  CTK_APPLICATION_IMPL_GET_CLASS (impl)->window_removed (impl, window);
}

void
ctk_application_impl_active_window_changed (CtkApplicationImpl *impl,
                                            CtkWindow          *window)
{
  CTK_APPLICATION_IMPL_GET_CLASS (impl)->active_window_changed (impl, window);
}

void
ctk_application_impl_handle_window_realize (CtkApplicationImpl *impl,
                                            CtkWindow          *window)
{
  CTK_APPLICATION_IMPL_GET_CLASS (impl)->handle_window_realize (impl, window);
}

void
ctk_application_impl_handle_window_map (CtkApplicationImpl *impl,
                                        CtkWindow          *window)
{
  CTK_APPLICATION_IMPL_GET_CLASS (impl)->handle_window_map (impl, window);
}

void
ctk_application_impl_set_app_menu (CtkApplicationImpl *impl,
                                   GMenuModel         *app_menu)
{
  CTK_APPLICATION_IMPL_GET_CLASS (impl)->set_app_menu (impl, app_menu);
}

void
ctk_application_impl_set_menubar (CtkApplicationImpl *impl,
                                  GMenuModel         *menubar)
{
  CTK_APPLICATION_IMPL_GET_CLASS (impl)->set_menubar (impl, menubar);
}

guint
ctk_application_impl_inhibit (CtkApplicationImpl         *impl,
                              CtkWindow                  *window,
                              CtkApplicationInhibitFlags  flags,
                              const gchar                *reason)
{
  return CTK_APPLICATION_IMPL_GET_CLASS (impl)->inhibit (impl, window, flags, reason);
}

void
ctk_application_impl_uninhibit (CtkApplicationImpl *impl,
                                guint               cookie)
{
  CTK_APPLICATION_IMPL_GET_CLASS (impl)->uninhibit (impl, cookie);
}

gboolean
ctk_application_impl_is_inhibited (CtkApplicationImpl         *impl,
                                   CtkApplicationInhibitFlags  flags)
{
  return CTK_APPLICATION_IMPL_GET_CLASS (impl)->is_inhibited (impl, flags);
}

gboolean
ctk_application_impl_prefers_app_menu (CtkApplicationImpl *impl)
{
  return CTK_APPLICATION_IMPL_GET_CLASS (impl)->prefers_app_menu (impl);
}

CtkApplicationImpl *
ctk_application_impl_new (CtkApplication *application,
                          GdkDisplay     *display)
{
  CtkApplicationImpl *impl;
  GType impl_type;

  impl_type = ctk_application_impl_get_type ();

#ifdef GDK_WINDOWING_X11
  if (GDK_IS_X11_DISPLAY (display))
    impl_type = ctk_application_impl_x11_get_type ();
#endif

#ifdef GDK_WINDOWING_WAYLAND
  if (GDK_IS_WAYLAND_DISPLAY (display))
    impl_type = ctk_application_impl_wayland_get_type ();
#endif

#ifdef GDK_WINDOWING_QUARTZ
  if (GDK_IS_QUARTZ_DISPLAY (display))
    impl_type = ctk_application_impl_quartz_get_type ();
#endif

  impl = g_object_new (impl_type, NULL);
  impl->application = application;
  impl->display = display;

  return impl;
}
