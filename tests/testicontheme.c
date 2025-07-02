/* testicontheme.c
 * Copyright (C) 2002, 2003  Red Hat, Inc.
 * Authors: Alexander Larsson, Owen Taylor
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

#include <ctk/ctk.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

static void
usage (void)
{
  g_print ("usage: test-icon-theme lookup <theme name> <icon name> [size] [scale]\n"
	   " or\n"
	   "usage: test-icon-theme list <theme name> [context]\n"
	   " or\n"
	   "usage: test-icon-theme display <theme name> <icon name> [size] [scale]\n"
	   " or\n"
	   "usage: test-icon-theme contexts <theme name>\n"
	   );
}

static void
icon_loaded_cb (GObject *source_object,
		GAsyncResult *res,
		gpointer user_data)
{
  GdkPixbuf *pixbuf;
  GError *error;

  error = NULL;
  pixbuf = ctk_icon_info_load_icon_finish (CTK_ICON_INFO (source_object),
					   res, &error);

  if (pixbuf == NULL)
    {
      g_print ("%s\n", error->message);
      exit (1);
    }

  ctk_image_set_from_pixbuf (CTK_IMAGE (user_data), pixbuf);
  g_object_unref (pixbuf);
}


int
main (int argc, char *argv[])
{
  CtkIconTheme *icon_theme;
  char *themename;
  GList *list;
  int size = 48;
  int scale = 1;
  CtkIconLookupFlags flags;
  
  ctk_init (&argc, &argv);

  if (argc < 3)
    {
      usage ();
      return 1;
    }

  flags = CTK_ICON_LOOKUP_USE_BUILTIN;

  if (g_getenv ("RTL"))
    flags |= CTK_ICON_LOOKUP_DIR_RTL;
  else
    flags |= CTK_ICON_LOOKUP_DIR_LTR;

  themename = argv[2];
  
  icon_theme = ctk_icon_theme_new ();
  
  ctk_icon_theme_set_custom_theme (icon_theme, themename);

  if (strcmp (argv[1], "display") == 0)
    {
      GError *error;
      GdkPixbuf *pixbuf;
      CtkWidget *window, *image;

      if (argc < 4)
	{
	  g_object_unref (icon_theme);
	  usage ();
	  return 1;
	}
      
      if (argc >= 5)
	size = atoi (argv[4]);

      if (argc >= 6)
	scale = atoi (argv[5]);

      error = NULL;
      pixbuf = ctk_icon_theme_load_icon_for_scale (icon_theme, argv[3], size, scale, flags, &error);
      if (!pixbuf)
        {
          g_print ("%s\n", error->message);
          return 1;
        }

      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      image = ctk_image_new ();
      ctk_image_set_from_pixbuf (CTK_IMAGE (image), pixbuf);
      g_object_unref (pixbuf);
      ctk_container_add (CTK_CONTAINER (window), image);
      g_signal_connect (window, "delete-event",
                        G_CALLBACK (ctk_main_quit), window);
      ctk_widget_show_all (window);
      
      ctk_main ();
    }
  else if (strcmp (argv[1], "display-async") == 0)
    {
      CtkWidget *window, *image;
      CtkIconInfo *info;

      if (argc < 4)
	{
	  g_object_unref (icon_theme);
	  usage ();
	  return 1;
	}

      if (argc >= 5)
	size = atoi (argv[4]);

      if (argc >= 6)
	scale = atoi (argv[5]);

      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      image = ctk_image_new ();
      ctk_container_add (CTK_CONTAINER (window), image);
      g_signal_connect (window, "delete-event",
                        G_CALLBACK (ctk_main_quit), window);
      ctk_widget_show_all (window);

      info = ctk_icon_theme_lookup_icon_for_scale (icon_theme, argv[3], size, scale, flags);

      if (info == NULL)
	{
          g_print ("Icon not found\n");
          return 1;
	}

      ctk_icon_info_load_icon_async (info,
				     NULL, icon_loaded_cb, image);

      ctk_main ();
    }
  else if (strcmp (argv[1], "list") == 0)
    {
      char *context;

      if (argc >= 4)
	context = argv[3];
      else
	context = NULL;

      list = ctk_icon_theme_list_icons (icon_theme,
					   context);
      
      while (list)
	{
	  g_print ("%s\n", (char *)list->data);
	  list = list->next;
	}
    }
  else if (strcmp (argv[1], "contexts") == 0)
    {
      list = ctk_icon_theme_list_contexts (icon_theme);
      
      while (list)
	{
	  g_print ("%s\n", (char *)list->data);
	  list = list->next;
	}
    }
  else if (strcmp (argv[1], "lookup") == 0)
    {
      CtkIconInfo *icon_info;

      if (argc < 4)
	{
	  g_object_unref (icon_theme);
	  usage ();
	  return 1;
	}

      if (argc >= 5)
	size = atoi (argv[4]);

      if (argc >= 6)
	scale = atoi (argv[5]);

      icon_info = ctk_icon_theme_lookup_icon_for_scale (icon_theme, argv[3], size, scale, flags);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      g_print ("icon for %s at %dx%d@%dx is %s\n", argv[3], size, size, scale,
	       icon_info ? (ctk_icon_info_get_builtin_pixbuf (icon_info) ? "<builtin>" : ctk_icon_info_get_filename (icon_info)) : "<none>");
G_GNUC_END_IGNORE_DEPRECATIONS

      if (icon_info)
	{
          GdkPixbuf *pixbuf;

          g_print ("Base size: %d, Scale: %d\n", ctk_icon_info_get_base_size (icon_info), ctk_icon_info_get_base_scale (icon_info));

          pixbuf = ctk_icon_info_load_icon (icon_info, NULL);
          if (pixbuf != NULL)
            {
              g_print ("Pixbuf size: %dx%d\n", gdk_pixbuf_get_width (pixbuf), gdk_pixbuf_get_height (pixbuf));
              g_object_unref (pixbuf);
            }

	  g_object_unref (icon_info);
	}
    }
  else
    {
      g_object_unref (icon_theme);
      usage ();
      return 1;
    }
 

  g_object_unref (icon_theme);
  
  return 0;
}
