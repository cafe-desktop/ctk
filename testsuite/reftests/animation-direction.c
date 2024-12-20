/*
 * Copyright (C) 2016 Red Hat Inc.
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
unblock (gpointer data G_GNUC_UNUSED)
{
  reftest_uninhibit_snapshot ();
  return G_SOURCE_REMOVE;
}

G_MODULE_EXPORT void
strip_attributes_if_no_animation (CtkWidget *widget)
{
  gboolean enabled;

  g_object_get (ctk_widget_get_settings (widget), "ctk-enable-animations", &enabled, NULL);
  if (enabled)
    return;

  g_message ("Unsetting text attributes because animation is disabled.");

  reftest_inhibit_snapshot ();
  ctk_label_set_attributes (CTK_LABEL (widget), NULL);
  g_timeout_add (500, unblock, NULL);
}
