/*
 * Copyright (C) 2014 Red Hat Inc.
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

#include <ctk/ctk.h>


G_MODULE_EXPORT void
set_default_direction_ltr (void)
{
  g_test_message ("Attention: globally setting default text direction to LTR");
  ctk_widget_set_default_direction (CTK_TEXT_DIR_LTR);
}

G_MODULE_EXPORT void
set_default_direction_rtl (void)
{
  g_test_message ("Attention: globally setting default text direction to RTL");
  ctk_widget_set_default_direction (CTK_TEXT_DIR_RTL);
}

G_MODULE_EXPORT void
switch_default_direction (void)
{
  switch (ctk_widget_get_default_direction ())
    {
    case CTK_TEXT_DIR_LTR:
      g_test_message ("Attention: globally switching default text direction from LTR to RTL");
      ctk_widget_set_default_direction (CTK_TEXT_DIR_RTL);
      break;
    case CTK_TEXT_DIR_RTL:
      g_test_message ("Attention: globally switching default text direction from RTL to LTR");
      ctk_widget_set_default_direction (CTK_TEXT_DIR_LTR);
      break;
    case CTK_TEXT_DIR_NONE:
    default:
      g_assert_not_reached ();
      break;
    }
}

G_MODULE_EXPORT void
switch_direction (GtkWidget *widget)
{
  switch (ctk_widget_get_direction (widget))
    {
    case CTK_TEXT_DIR_LTR:
      ctk_widget_set_direction (widget, CTK_TEXT_DIR_RTL);
      break;
    case CTK_TEXT_DIR_RTL:
      ctk_widget_set_direction (widget, CTK_TEXT_DIR_LTR);
      break;
    case CTK_TEXT_DIR_NONE:
    default:
      g_assert_not_reached ();
      break;
    }
}

G_MODULE_EXPORT void
swap_child (GtkWidget *window)
{
  GtkWidget *image;

  ctk_container_remove (CTK_CONTAINER (window), ctk_bin_get_child (CTK_BIN (window)));

  image = ctk_image_new_from_icon_name ("go-next", CTK_ICON_SIZE_BUTTON);
  ctk_widget_show (image);
  ctk_container_add (CTK_CONTAINER (window), image);
}
