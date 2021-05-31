/*
 * ctkimmodulebroadway
 * Copyright (C) 2013 Alexander Larsson
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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * $Id:$
 */

#include "config.h"
#include <string.h>

#include <ctk/ctk.h>
#include "ctk/ctkintl.h"
#include "ctk/ctkimmodule.h"

#include "gdk/broadway/gdkbroadway.h"

#define CTK_IM_CONTEXT_TYPE_BROADWAY (type_broadway)
#define CTK_IM_CONTEXT_BROADWAY(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_IM_CONTEXT_TYPE_BROADWAY, CtkIMContextBroadway))
#define CTK_IM_CONTEXT_BROADWAY_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), CTK_IM_CONTEXT_TYPE_BROADWAY, CtkIMContextBroadwayClass))

typedef struct _CtkIMContextBroadway
{
  CtkIMContextSimple parent;
  GdkWindow *client_window;
} CtkIMContextBroadway;

typedef struct _CtkIMContextBroadwayClass
{
  CtkIMContextSimpleClass parent_class;
} CtkIMContextBroadwayClass;

GType type_broadway = 0;
static GObjectClass *parent_class;

static const CtkIMContextInfo imbroadway_info =
{
  "broadway",      /* ID */
  NC_("input method menu", "Broadway"),      /* Human readable name */
  GETTEXT_PACKAGE, /* Translation domain */
  CTK_LOCALEDIR,   /* Dir for bindtextdomain (not strictly needed for "ctk+") */
  "",              /* Languages for which this module is the default */
};

static const CtkIMContextInfo *info_list[] =
{
  &imbroadway_info,
};

#ifndef INCLUDE_IM_broadway
#define MODULE_ENTRY(type,function) G_MODULE_EXPORT type im_module_ ## function
#else
#define MODULE_ENTRY(type, function) type _ctk_immodule_broadway_ ## function
#endif

static void
broadway_set_client_window (CtkIMContext *context, GdkWindow *window)
{
  CtkIMContextBroadway *bw = CTK_IM_CONTEXT_BROADWAY (context);

  bw->client_window = window;
}

static void
broadway_focus_in (CtkIMContext *context)
{
  CtkIMContextBroadway *bw = CTK_IM_CONTEXT_BROADWAY (context);
  GdkDisplay *display;

  if (bw->client_window)
    {
      display = gdk_window_get_display (bw->client_window);
      gdk_broadway_display_show_keyboard (GDK_BROADWAY_DISPLAY (display));
    }
}

static void
broadway_focus_out (CtkIMContext *context)
{
  CtkIMContextBroadway *bw = CTK_IM_CONTEXT_BROADWAY (context);
  GdkDisplay *display;

  if (bw->client_window)
    {
      display = gdk_window_get_display (bw->client_window);
      gdk_broadway_display_hide_keyboard (GDK_BROADWAY_DISPLAY (display));
    }
}

static void
ctk_im_context_broadway_class_init (CtkIMContextClass *klass)
{
  parent_class = g_type_class_peek_parent (klass);

  klass->focus_in = broadway_focus_in;
  klass->focus_out = broadway_focus_out;
  klass->set_client_window = broadway_set_client_window;
}

static void
ctk_im_context_broadway_init (CtkIMContext *im_context)
{
}

static void
ctk_im_context_broadway_register_type (GTypeModule *module)
{
  const GTypeInfo object_info =
  {
    sizeof (CtkIMContextBroadwayClass),
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) ctk_im_context_broadway_class_init,
    NULL,           /* class_finalize */
    NULL,           /* class_data */
    sizeof (CtkIMContextBroadway),
    0,
    (GInstanceInitFunc) ctk_im_context_broadway_init,
  };

  type_broadway =
    g_type_module_register_type (module,
                                 CTK_TYPE_IM_CONTEXT_SIMPLE,
                                 "CtkIMContextBroadway",
                                 &object_info, 0);
}

MODULE_ENTRY (void, init) (GTypeModule * module)
{
  ctk_im_context_broadway_register_type (module);
}

MODULE_ENTRY (void, exit) (void)
{
}

MODULE_ENTRY (void, list) (const CtkIMContextInfo *** contexts, int *n_contexts)
{
  *contexts = info_list;
  *n_contexts = G_N_ELEMENTS (info_list);
}

MODULE_ENTRY (CtkIMContext *, create) (const gchar * context_id)
{
  if (!strcmp (context_id, "broadway"))
    return g_object_new (type_broadway, NULL);
  else
    return NULL;
}
