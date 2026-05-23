/* CTK - The GIMP Toolkit
 * Copyright 2015  Emmanuele Bassi 
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

/*
 * Modified by the CTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#include "config.h"

#include "ctktooltipwindowprivate.h"

#include "ctkprivate.h"
#include "ctkintl.h"

#include "ctkaccessible.h"
#include "ctkbox.h"
#include "ctkimage.h"
#include "ctklabel.h"
#include "ctkmain.h"
#include "ctksettings.h"
#include "ctksizerequest.h"
#include "ctkwindowprivate.h"
#include "ctkwidgetprivate.h"

#define MAX_TOOLTIP_LINE_WIDTH  70

struct _CtkTooltipWindow
{
  CtkWindow parent_type;

  CtkWidget *box;
  CtkWidget *image;
  CtkWidget *label;
  CtkWidget *custom_widget;
};

struct _CtkTooltipWindowClass
{
  CtkWindowClass parent_class;
};

G_DEFINE_TYPE (CtkTooltipWindow, ctk_tooltip_window, CTK_TYPE_WINDOW)

static void
ctk_tooltip_window_class_init (CtkTooltipWindowClass *klass)
{
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);

  ctk_widget_class_set_css_name (widget_class, I_("tooltip"));
  ctk_widget_class_set_accessible_role (widget_class, ATK_ROLE_TOOL_TIP);
  ctk_widget_class_set_template_from_resource (widget_class, "/org/ctk/libctk/ui/ctktooltipwindow.ui");

  ctk_widget_class_bind_template_child (widget_class, CtkTooltipWindow, box);
  ctk_widget_class_bind_template_child (widget_class, CtkTooltipWindow, image);
  ctk_widget_class_bind_template_child (widget_class, CtkTooltipWindow, label);
}

static void
ctk_tooltip_window_init (CtkTooltipWindow *self)
{
  CtkWindow *window = CTK_WINDOW (self);

  ctk_widget_init_template (CTK_WIDGET (self));

  _ctk_window_request_csd (window);
}

CtkWidget *
ctk_tooltip_window_new (void)
{
  return g_object_new (CTK_TYPE_TOOLTIP_WINDOW,
                       "type", CTK_WINDOW_POPUP,
                       NULL);
}

void
ctk_tooltip_window_set_label_markup (CtkTooltipWindow *window,
                                     const char       *markup)
{
  if (markup != NULL)
    {
      ctk_label_set_markup (CTK_LABEL (window->label), markup);
      ctk_widget_show (window->label);
    }
  else
    {
      ctk_widget_hide (window->label);
    }
}

void
ctk_tooltip_window_set_label_text (CtkTooltipWindow *window,
                                   const char       *text)
{
  if (text != NULL)
    {
      ctk_label_set_text (CTK_LABEL (window->label), text);
      ctk_widget_show (window->label);
    }
  else
    {
      ctk_widget_hide (window->label);
    }
}

void
ctk_tooltip_window_set_image_icon (CtkTooltipWindow *window,
                                   CdkPixbuf        *pixbuf)
{

  if (pixbuf != NULL)
    {
      ctk_image_set_from_pixbuf (CTK_IMAGE (window->image), pixbuf);
      ctk_widget_show (window->image);
    }
  else
    {
      ctk_widget_hide (window->image);
    }
}

void
ctk_tooltip_window_set_image_icon_from_stock (CtkTooltipWindow *window,
                                              const char       *stock_id,
                                              CtkIconSize       icon_size)
{
  if (stock_id != NULL)
    {
 G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      ctk_image_set_from_stock (CTK_IMAGE (window->image), stock_id, icon_size);
 G_GNUC_END_IGNORE_DEPRECATIONS;

      ctk_widget_show (window->image);
    }
  else
    {
      ctk_widget_hide (window->image);
    }
}

void
ctk_tooltip_window_set_image_icon_from_name (CtkTooltipWindow *window,
                                             const char       *icon_name,
                                             CtkIconSize       icon_size)
{
  if (icon_name)
    {
      ctk_image_set_from_icon_name (CTK_IMAGE (window->image), icon_name, icon_size);
      ctk_widget_show (window->image);
    }
  else
    {
      ctk_widget_hide (window->image);
    }
}

void
ctk_tooltip_window_set_image_icon_from_gicon (CtkTooltipWindow *window,
                                              GIcon            *gicon,
                                              CtkIconSize       icon_size)
{
  if (gicon != NULL)
    {
      ctk_image_set_from_gicon (CTK_IMAGE (window->image), gicon, icon_size);
      ctk_widget_show (window->image);
    }
  else
    {
      ctk_widget_hide (window->image);
    }
}

void
ctk_tooltip_window_set_custom_widget (CtkTooltipWindow *window,
                                      CtkWidget        *custom_widget)
{
  /* No need to do anything if the custom widget stays the same */
  if (window->custom_widget == custom_widget)
    return;

  if (window->custom_widget != NULL)
    {
      CtkWidget *custom = window->custom_widget;

      /* Note: We must reset window->custom_widget first,
       * since ctk_container_remove() will recurse into
       * ctk_tooltip_set_custom()
       */
      window->custom_widget = NULL;
      ctk_container_remove (CTK_CONTAINER (window->box), custom);
      g_object_unref (custom);
    }

  if (custom_widget != NULL)
    {
      window->custom_widget = g_object_ref (custom_widget);

      ctk_container_add (CTK_CONTAINER (window->box), custom_widget);
      ctk_widget_show (custom_widget);
    }
}
