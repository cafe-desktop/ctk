/* stresstest-toolbar.c
 *
 * Copyright (C) 2003 Soeren Sandmann <sandmann@daimi.au.dk>
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
#include <gtk/gtk.h>

typedef struct _Info Info;
struct _Info
{
  GtkWindow  *window;
  GtkToolbar *toolbar;
  gint	      counter;
};

static void
add_random (GtkToolbar *toolbar, gint n)
{
  gint n_items;
  gint position;
  gchar *label = g_strdup_printf ("Button %d", n);

  GtkToolItem *toolitem = ctk_tool_button_new (NULL, label);
  ctk_tool_item_set_tooltip_text (toolitem, "Bar");

  g_free (label);
  ctk_widget_show_all (CTK_WIDGET (toolitem));

  n_items = ctk_toolbar_get_n_items (toolbar);
  if (n_items == 0)
    position = 0;
  else
    position = g_random_int_range (0, n_items);

  ctk_toolbar_insert (toolbar, toolitem, position);
}

static void
remove_random (GtkToolbar *toolbar)
{
  GtkToolItem *tool_item;
  gint n_items;
  gint position;

  n_items = ctk_toolbar_get_n_items (toolbar);

  if (n_items == 0)
    return;

  position = g_random_int_range (0, n_items);

  tool_item = ctk_toolbar_get_nth_item (toolbar, position);

  ctk_container_remove (CTK_CONTAINER (toolbar),
                        CTK_WIDGET (tool_item));
}

static gboolean
stress_test_old_api (gpointer data)
{
  typedef enum {
    ADD_RANDOM,
    REMOVE_RANDOM,
    LAST_ACTION
  } Action;
      
  Info *info = data;
  Action action;
  gint n_items;

  if (info->counter++ == 200)
    {
      ctk_main_quit ();
      return FALSE;
    }

  if (!info->toolbar)
    {
      info->toolbar = CTK_TOOLBAR (ctk_toolbar_new ());
      ctk_container_add (CTK_CONTAINER (info->window),
			 CTK_WIDGET (info->toolbar));
      ctk_widget_show (CTK_WIDGET (info->toolbar));
    }

  n_items = ctk_toolbar_get_n_items (info->toolbar);
  if (n_items == 0)
    {
      add_random (info->toolbar, info->counter);
      return TRUE;
    }
  else if (n_items > 50)
    {
      int i;
      for (i = 0; i < 25; i++)
	remove_random (info->toolbar);
      return TRUE;
    }
  
  action = g_random_int_range (0, LAST_ACTION);

  switch (action)
    {
    case ADD_RANDOM:
      add_random (info->toolbar, info->counter);
      break;

    case REMOVE_RANDOM:
      remove_random (info->toolbar);
      break;
      
    default:
      g_assert_not_reached();
      break;
    }
  
  return TRUE;
}


gint
main (gint argc, gchar **argv)
{
  Info info;
  
  ctk_init (&argc, &argv);

  info.toolbar = NULL;
  info.counter = 0;
  info.window = CTK_WINDOW (ctk_window_new (CTK_WINDOW_TOPLEVEL));

  ctk_widget_show (CTK_WIDGET (info.window));
  
  gdk_threads_add_idle (stress_test_old_api, &info);

  ctk_widget_show_all (CTK_WIDGET (info.window));
  
  ctk_main ();

  ctk_widget_destroy (CTK_WIDGET (info.window));

  info.toolbar = NULL;
  info.window = NULL;
  
  return 0;
}
