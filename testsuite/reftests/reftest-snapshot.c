/*
 * Copyright (C) 2011 Red Hat Inc.
 *
 * Author:
 *      Benjamin Otte <otte@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "reftest-snapshot.h"

#include "reftest-module.h"
#include "ctk-reftest.h"

#include <string.h>

typedef enum {
  SNAPSHOT_WINDOW,
  SNAPSHOT_DRAW
} SnapshotMode;

static CtkWidget *
builder_get_toplevel (CtkBuilder *builder)
{
  GSList *list, *walk;
  CtkWidget *window = NULL;

  list = ctk_builder_get_objects (builder);
  for (walk = list; walk; walk = walk->next)
    {
      if (CTK_IS_WINDOW (walk->data) &&
          ctk_widget_get_parent (walk->data) == NULL)
        {
          window = walk->data;
          break;
        }
    }
  
  g_slist_free (list);

  return window;
}

static gboolean
quit_when_idle (gpointer loop)
{
  g_main_loop_quit (loop);

  return G_SOURCE_REMOVE;
}

static gint inhibit_count;
static GMainLoop *loop;

void
reftest_inhibit_snapshot (void)
{
  inhibit_count++;
}

void
reftest_uninhibit_snapshot (void)
{
  g_assert (inhibit_count > 0);
  inhibit_count--;

  if (inhibit_count == 0)
    g_idle_add (quit_when_idle, loop);
}

static void
check_for_draw (CdkEvent *event,
		gpointer  data G_GNUC_UNUSED)
{
  if (event->type == CDK_EXPOSE)
    {
      reftest_uninhibit_snapshot ();
      cdk_event_handler_set ((CdkEventFunc) ctk_main_do_event, NULL, NULL);
    }

  ctk_main_do_event (event);
}

static cairo_surface_t *
snapshot_widget (CtkWidget *widget, SnapshotMode mode)
{
  cairo_surface_t *surface;
  cairo_pattern_t *bg;
  cairo_t *cr;

  g_assert (ctk_widget_get_realized (widget));

  loop = g_main_loop_new (NULL, FALSE);

  /* We wait until the widget is drawn for the first time.
   * We can not wait for a CtkWidget::draw event, because that might not
   * happen if the window is fully obscured by windowed child widgets.
   * Alternatively, we could wait for an expose event on widget's window.
   * Both of these are rather hairy, not sure what's best.
   *
   * We also use an inhibit mechanism, to give module functions a chance
   * to delay the snapshot.
   */
  reftest_inhibit_snapshot ();
  cdk_event_handler_set (check_for_draw, NULL, NULL);
  g_main_loop_run (loop);

  surface = cdk_window_create_similar_surface (ctk_widget_get_window (widget),
                                               CAIRO_CONTENT_COLOR,
                                               ctk_widget_get_allocated_width (widget),
                                               ctk_widget_get_allocated_height (widget));

  cr = cairo_create (surface);

  switch (mode)
    {
    case SNAPSHOT_WINDOW:
      {
        CdkWindow *window = ctk_widget_get_window (widget);
        if (cdk_window_get_window_type (window) == CDK_WINDOW_TOPLEVEL ||
            cdk_window_get_window_type (window) == CDK_WINDOW_FOREIGN)
          {
            /* give the WM/server some time to sync. They need it.
             * Also, do use popups instead of toplevels in your tests
             * whenever you can.
             */
            cdk_display_sync (cdk_window_get_display (window));
            g_timeout_add (500, quit_when_idle, loop);
            g_main_loop_run (loop);
          }
        cdk_cairo_set_source_window (cr, window, 0, 0);
        cairo_paint (cr);
      }
      break;
    case SNAPSHOT_DRAW:
      bg = cdk_window_get_background_pattern (ctk_widget_get_window (widget));
      if (bg)
        {
          cairo_set_source (cr, bg);
          cairo_paint (cr);
        }
      ctk_widget_draw (widget, cr);
      break;
    default:
      g_assert_not_reached();
      break;
    }

  cairo_destroy (cr);
  g_main_loop_unref (loop);
  ctk_widget_destroy (widget);

  return surface;
}

static void
connect_signals (CtkBuilder    *builder,
                 GObject       *object,
                 const gchar   *signal_name,
                 const gchar   *handler_name,
                 GObject       *connect_object,
                 GConnectFlags  flags,
                 gpointer       user_data)
{
  ReftestModule *module;
  const char *directory;
  GCallback func;
  GClosure *closure;
  char **split;

  directory = user_data;
  split = g_strsplit (handler_name, ":", -1);

  switch (g_strv_length (split))
    {
    case 1:
      func = ctk_builder_lookup_callback_symbol (builder, split[0]);

      if (func)
        {
          module = NULL;
        }
      else
        {
          module = reftest_module_new_self ();
          if (module == NULL)
            {
              g_error ("glib compiled without module support.");
              return;
            }
          func = reftest_module_lookup (module, split[0]);
          if (!func)
            {
              g_error ("failed to lookup handler for name '%s' when connecting signals", split[0]);
              return;
            }
        }
      break;
    case 2:
      if (g_getenv ("REFTEST_MODULE_DIR"))
        directory = g_getenv ("REFTEST_MODULE_DIR");
      module = reftest_module_new (directory, split[0]);
      if (module == NULL)
        {
          g_error ("Could not load module '%s' from '%s' when looking up '%s'", split[0], directory, handler_name);
          return;
        }
      func = reftest_module_lookup (module, split[1]);
      if (!func)
        {
          g_error ("failed to lookup handler for name '%s' in module '%s'", split[1], split[0]);
          return;
        }
      break;
    default:
      g_error ("Could not connect signal handler named '%s'", handler_name);
      return;
    }

  g_strfreev (split);

  if (connect_object)
    {
      if (flags & G_CONNECT_SWAPPED)
        closure = g_cclosure_new_object_swap (func, connect_object);
      else
        closure = g_cclosure_new_object (func, connect_object);
    }
  else
    {
      if (flags & G_CONNECT_SWAPPED)
        closure = g_cclosure_new_swap (func, NULL, NULL);
      else
        closure = g_cclosure_new (func, NULL, NULL);
    }
  
  if (module)
    g_closure_add_finalize_notifier (closure, module, (GClosureNotify) reftest_module_unref);

  g_signal_connect_closure (object, signal_name, closure, flags & G_CONNECT_AFTER ? TRUE : FALSE);
}

cairo_surface_t *
reftest_snapshot_ui_file (const char *ui_file)
{
  CtkWidget *window;
  CtkBuilder *builder;
  GError *error = NULL;
  char *directory;

  directory = g_path_get_dirname (ui_file);

  builder = ctk_builder_new ();
  ctk_builder_add_from_file (builder, ui_file, &error);
  g_assert_no_error (error);
  ctk_builder_connect_signals_full (builder, connect_signals, directory);
  window = builder_get_toplevel (builder);
  g_object_unref (builder);
  g_free (directory);
  g_assert (window);

  ctk_widget_show (window);

  return snapshot_widget (window, SNAPSHOT_WINDOW);
}
