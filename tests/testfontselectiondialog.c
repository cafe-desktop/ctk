/* testfontselectiondialog.c
 * Copyright (C) 2011 Alberto Ruiz <aruiz@gnome.org>
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

#define GDK_DISABLE_DEPRECATION_WARNINGS

#include <ctk/ctk.h>

int
main (int argc, char *argv[])
{
  CtkWidget *dialog;
  CtkWidget *ok G_GNUC_UNUSED;

  ctk_init (&argc, &argv);

  dialog = ctk_font_selection_dialog_new (NULL);

  ok = ctk_font_selection_dialog_get_ok_button (CTK_FONT_SELECTION_DIALOG (dialog));

  ctk_dialog_run (CTK_DIALOG (dialog));

  ctk_widget_destroy (dialog);

  return 0;
}
