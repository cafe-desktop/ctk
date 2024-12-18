/*
 * Copyright (C) 2014 Red Hat Inc.
 *
 * Author:
 *      Matthias Clasen <mclasen@redhat.com>
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

#include "ctk-reftest.h"

static gboolean 
tick_callback_for_1_frame (CtkWidget     *widget G_GNUC_UNUSED,
                           CdkFrameClock *frame_clock G_GNUC_UNUSED,
                           gpointer       unused G_GNUC_UNUSED)
{
  reftest_uninhibit_snapshot ();

  return G_SOURCE_REMOVE;
}

G_MODULE_EXPORT gboolean
inhibit_for_1_frame (CtkWidget *widget)
{
  reftest_inhibit_snapshot ();
  ctk_widget_add_tick_callback (widget,
                                tick_callback_for_1_frame,
                                NULL, NULL);

  return FALSE;
}

static gboolean 
tick_callback_for_2_frames (CtkWidget     *widget,
                            CdkFrameClock *frame_clock G_GNUC_UNUSED,
                            gpointer       unused G_GNUC_UNUSED)
{
  inhibit_for_1_frame (widget);
  reftest_uninhibit_snapshot ();

  return G_SOURCE_REMOVE;
}

G_MODULE_EXPORT gboolean
inhibit_for_2_frames (CtkWidget *widget)
{
  reftest_inhibit_snapshot ();
  ctk_widget_add_tick_callback (widget,
                                tick_callback_for_2_frames,
                                NULL, NULL);

  return FALSE;
}

static gboolean 
tick_callback_for_3_frames (CtkWidget     *widget,
                            CdkFrameClock *frame_clock G_GNUC_UNUSED,
                            gpointer       unused G_GNUC_UNUSED)
{
  inhibit_for_2_frames (widget);
  reftest_uninhibit_snapshot ();

  return G_SOURCE_REMOVE;
}

G_MODULE_EXPORT gboolean
inhibit_for_3_frames (CtkWidget *widget)
{
  reftest_inhibit_snapshot ();
  ctk_widget_add_tick_callback (widget,
                                tick_callback_for_3_frames,
                                NULL, NULL);

  return FALSE;
}

G_MODULE_EXPORT gboolean
add_reference_class_if_no_animation (CtkWidget *widget)
{
  gboolean enabled;
  CtkStyleContext *context;

  g_object_get (ctk_widget_get_settings (widget), "ctk-enable-animations", &enabled, NULL);
  if (enabled)
    return FALSE;

  g_message ("Adding reference class because animation is disabled");

  context = ctk_widget_get_style_context (widget);
  ctk_style_context_add_class (context, "reference");

  return FALSE;
}
