/*
 * ctkimmodulequartz
 * Copyright (C) 2011 Hiroyuki Yamamoto
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

#include <AvailabilityMacros.h>

#define CTK_COMPILATION 1 // For cdkquartz-ctk-only.h

#include "cdk/quartz/cdkinternal-quartz.h"
#include "cdk/quartz/cdkquartz-ctk-only.h"
#include "cdk/quartz/CdkQuartzView.h"

#define CTK_IM_CONTEXT_TYPE_QUARTZ (type_quartz)
#define CTK_IM_CONTEXT_QUARTZ(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_IM_CONTEXT_TYPE_QUARTZ, CtkIMContextQuartz))
#define CTK_IM_CONTEXT_QUARTZ_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), CTK_IM_CONTEXT_TYPE_QUARTZ, CtkIMContextQuartzClass))

#if MAC_OS_X_VERSION_MIN_REQUIRED < 10120
#define NS_EVENT_KEY_DOWN NSKeyDown
#else
#define NS_EVENT_KEY_DOWN NSEventTypeKeyDown
#endif

typedef struct _CtkIMContextQuartz
{
  CtkIMContext parent;
  CtkIMContext *slave;
  CdkWindow *client_window;
  gchar *preedit_str;
  unsigned int cursor_index;
  unsigned int selected_len;
  CdkRectangle *cursor_rect;
  gboolean focused;
} CtkIMContextQuartz;

typedef struct _CtkIMContextQuartzClass
{
  CtkIMContextClass parent_class;
} CtkIMContextQuartzClass;

GType type_quartz = 0;
static GObjectClass *parent_class;

static const CtkIMContextInfo imquartz_info =
{
  "quartz",
  NC_("input method menu", "Mac OS X Quartz"),
  GETTEXT_PACKAGE,
  CTK_LOCALEDIR,
  "ja:ko:zh:*",
};

static const CtkIMContextInfo *info_list[] =
{
  &imquartz_info,
};

#ifndef INCLUDE_IM_quartz
#define MODULE_ENTRY(type,function) G_MODULE_EXPORT type im_module_ ## function
#else
#define MODULE_ENTRY(type, function) type _ctk_immodule_quartz_ ## function
#endif

static void
quartz_get_preedit_string (CtkIMContext *context,
                           gchar **str,
                           PangoAttrList **attrs,
                           gint *cursor_pos)
{
  CtkIMContextQuartz *qc = CTK_IM_CONTEXT_QUARTZ (context);

  CTK_NOTE (MISC, g_print ("quartz_get_preedit_string\n"));

  if (str)
    *str = qc->preedit_str ? g_strdup (qc->preedit_str) : g_strdup ("");

  if (attrs)
    {
      *attrs = pango_attr_list_new ();
      int len = g_utf8_strlen (*str, -1);
      gchar *ch = *str;
      if (len > 0)
        {
          PangoAttribute *attr;
          int i = 0;
          for (;;)
            {
              gchar *s = ch;
              ch = g_utf8_next_char (ch);

              if (i >= qc->cursor_index &&
		  i < qc->cursor_index + qc->selected_len)
                attr = pango_attr_underline_new (PANGO_UNDERLINE_DOUBLE);
              else
                attr = pango_attr_underline_new (PANGO_UNDERLINE_SINGLE);

              attr->start_index = s - *str;
              if (!*ch)
                attr->end_index = attr->start_index + strlen (s);
              else
                attr->end_index = ch - *str;

              pango_attr_list_change (*attrs, attr);

              if (!*ch)
                break;
              i++;
            }
        }
    }
  if (cursor_pos)
    *cursor_pos = qc->cursor_index;
}

static gboolean
output_result (CtkIMContext *context,
               CdkWindow *win)
{
  CtkIMContextQuartz *qc = CTK_IM_CONTEXT_QUARTZ (context);
  gboolean retval = FALSE;
  int fixed_str_replace_len;
  gchar *fixed_str, *marked_str;

  fixed_str_replace_len = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (win),
      TIC_INSERT_TEXT_REPLACE_LEN));
  fixed_str = g_strdup (g_object_get_data (G_OBJECT (win), TIC_INSERT_TEXT));
  marked_str = g_strdup (g_object_get_data (G_OBJECT (win), TIC_MARKED_TEXT));
  if (fixed_str)
    {
      CTK_NOTE (MISC, g_print ("tic-insert-text: %s\n", fixed_str));
      g_free (qc->preedit_str);
      qc->preedit_str = NULL;
      g_object_set_data (G_OBJECT (win), TIC_INSERT_TEXT, NULL);
      if (fixed_str_replace_len)
        {
          gboolean retval;
          g_object_set_data (G_OBJECT (win), TIC_INSERT_TEXT_REPLACE_LEN, 0);
          g_signal_emit_by_name (context, "delete-surrounding",
              -fixed_str_replace_len, fixed_str_replace_len, &retval);
        }
      g_signal_emit_by_name (context, "commit", fixed_str);
      g_signal_emit_by_name (context, "preedit_changed");

      unsigned int filtered =
	   GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (win),
						GIC_FILTER_KEY));
      CTK_NOTE (MISC, g_print ("filtered, %d\n", filtered));
      if (filtered)
        retval = TRUE;
      else
        retval = FALSE;
    }
  if (marked_str)
    {
      CTK_NOTE (MISC, g_print ("tic-marked-text: %s\n", marked_str));
      qc->cursor_index =
	   GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (win),
						TIC_SELECTED_POS));
      qc->selected_len =
	   GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (win),
						TIC_SELECTED_LEN));
      g_free (qc->preedit_str);
      qc->preedit_str = g_strdup (marked_str);
      g_object_set_data (G_OBJECT (win), TIC_MARKED_TEXT, NULL);
      g_signal_emit_by_name (context, "preedit_changed");
      retval = TRUE;
    }
  if (!fixed_str && !marked_str)
    {
      unsigned int filtered =
	  GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (win),
					       GIC_FILTER_KEY));
      if (filtered)
        retval = TRUE;
      if (qc->preedit_str && strlen (qc->preedit_str) > 0)
        retval = TRUE;
    }
  g_free (fixed_str);
  g_free (marked_str);
  return retval;
}

static gboolean
quartz_filter_keypress (CtkIMContext *context,
                        CdkEventKey *event)
{
  CtkIMContextQuartz *qc = CTK_IM_CONTEXT_QUARTZ (context);
  gboolean retval;
  NSView *nsview;
  CdkWindow *win;

  CTK_NOTE (MISC, g_print ("quartz_filter_keypress\n"));

  if (!CDK_IS_QUARTZ_WINDOW (qc->client_window))
    return FALSE;

  NSEvent *nsevent = cdk_quartz_event_get_nsevent ((CdkEvent *)event);

  if (!nsevent)
    {
      if (event->hardware_keycode == 0 && event->keyval == 0xffffff)
        /* update text input changes by mouse events */
        return output_result (context, event->window);
      else
        return ctk_im_context_filter_keypress (qc->slave, event);
    }

  nsview = cdk_quartz_window_get_nsview (qc->client_window);

  win = (CdkWindow *)[(CdkQuartzView *)[[nsevent window] contentView] cdkWindow];
  CTK_NOTE (MISC, g_print ("client_window: %p, win: %p, nsview: %p\n",
                           qc->client_window, win, nsview));

  if (event->type == CDK_KEY_RELEASE)
    return FALSE;

  if (event->hardware_keycode == 55)	/* Command */
    return FALSE;

  if (event->hardware_keycode == 53) /* Escape */
    return FALSE;

  NSEventType etype = [nsevent type];
  if (etype == NS_EVENT_KEY_DOWN)
    {
       g_object_set_data (G_OBJECT (win), TIC_IN_KEY_DOWN,
                                          GUINT_TO_POINTER (TRUE));
       [nsview keyDown: nsevent];
    }
  /* JIS_Eisu || JIS_Kana */
  if (event->hardware_keycode == 102 || event->hardware_keycode == 104)
    return FALSE;

  retval = output_result(context, win);
  g_object_set_data (G_OBJECT (win), TIC_IN_KEY_DOWN,
                                     GUINT_TO_POINTER (FALSE));
  CTK_NOTE (MISC, g_print ("quartz_filter_keypress done\n"));

  return retval;
}

static void
discard_preedit (CtkIMContext *context)
{
  CtkIMContextQuartz *qc = CTK_IM_CONTEXT_QUARTZ (context);

  if (!qc->client_window)
    return;

  if (!CDK_IS_QUARTZ_WINDOW (qc->client_window))
    return;

  NSView *nsview = cdk_quartz_window_get_nsview (qc->client_window);
  if (!nsview)
    return;

  /* reset any partial input for this NSView */
  [(CdkQuartzView *)nsview unmarkText];
#if MAC_OS_X_VERSION_MIN_REQUIRED < 1060
  NSInputManager *currentInputManager = [NSInputManager currentInputManager];
  [currentInputManager markedTextAbandoned:nsview];
#else
  [[NSTextInputContext currentInputContext] discardMarkedText];
#endif
  if (qc->preedit_str && strlen (qc->preedit_str) > 0)
    {
      g_signal_emit_by_name (context, "commit", qc->preedit_str);

      g_free (qc->preedit_str);
      qc->preedit_str = NULL;
      g_signal_emit_by_name (context, "preedit_changed");
    }
}

static void
quartz_reset (CtkIMContext *context)
{
  CTK_NOTE (MISC, g_print ("quartz_reset\n"));
  discard_preedit (context);
}

static void
quartz_set_client_window (CtkIMContext *context, CdkWindow *window)
{
  CtkIMContextQuartz *qc = CTK_IM_CONTEXT_QUARTZ (context);

  CTK_NOTE (MISC, g_print ("quartz_set_client_window: %p\n", window));

  qc->client_window = window;
}

static void
quartz_focus_in (CtkIMContext *context)
{
  CTK_NOTE (MISC, g_print ("quartz_focus_in\n"));

  CtkIMContextQuartz *qc = CTK_IM_CONTEXT_QUARTZ (context);
  qc->focused = TRUE;
}

static void
quartz_focus_out (CtkIMContext *context)
{
  CTK_NOTE (MISC, g_print ("quartz_focus_out\n"));

  CtkIMContextQuartz *qc = CTK_IM_CONTEXT_QUARTZ (context);
  qc->focused = FALSE;

  /* Commit any partially built strings or it'll mess up other CTK+ widgets in the window */
  discard_preedit (context);
}

static void
quartz_set_cursor_location (CtkIMContext *context, CdkRectangle *area)
{
  CtkIMContextQuartz *qc = CTK_IM_CONTEXT_QUARTZ (context);
  gint x, y;
  NSView *nsview;
  CdkWindow *win;

  CTK_NOTE (MISC, g_print ("quartz_set_cursor_location\n"));

  if (!qc->client_window)
    return;

  if (!qc->focused)
    return;

  qc->cursor_rect->x = area->x;
  qc->cursor_rect->y = area->y;
  qc->cursor_rect->width = area->width;
  qc->cursor_rect->height = area->height;

  cdk_window_get_origin (qc->client_window, &x, &y);

  qc->cursor_rect->x = area->x + x;
  qc->cursor_rect->y = area->y + y;

  if (!CDK_IS_QUARTZ_WINDOW (qc->client_window))
    return;

  nsview = cdk_quartz_window_get_nsview (qc->client_window);
  win = (CdkWindow *)[ (CdkQuartzView*)nsview cdkWindow];
  g_object_set_data (G_OBJECT (win), GIC_CURSOR_RECT, qc->cursor_rect);
}

static void
quartz_set_use_preedit (CtkIMContext *context, gboolean use_preedit)
{
  CTK_NOTE (MISC, g_print ("quartz_set_use_preedit: %d\n", use_preedit));
}

static void
commit_cb (CtkIMContext *context, const gchar *str, CtkIMContextQuartz *qc)
{
  g_signal_emit_by_name (qc, "commit", str);
}

static void
imquartz_finalize (GObject *obj)
{
  CTK_NOTE (MISC, g_print ("imquartz_finalize\n"));

  CtkIMContextQuartz *qc = CTK_IM_CONTEXT_QUARTZ (obj);
  g_free (qc->preedit_str);
  qc->preedit_str = NULL;
  g_free (qc->cursor_rect);
  qc->cursor_rect = NULL;

  g_signal_handlers_disconnect_by_func (qc->slave, (gpointer)commit_cb, qc);
  g_object_unref (qc->slave);

  parent_class->finalize (obj);
}

static void
ctk_im_context_quartz_class_init (CtkIMContextClass *klass)
{
  CTK_NOTE (MISC, g_print ("ctk_im_context_quartz_class_init\n"));

  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  parent_class = g_type_class_peek_parent (klass);

  klass->get_preedit_string = quartz_get_preedit_string;
  klass->filter_keypress = quartz_filter_keypress;
  klass->reset = quartz_reset;
  klass->set_client_window = quartz_set_client_window;
  klass->focus_in = quartz_focus_in;
  klass->focus_out = quartz_focus_out;
  klass->set_cursor_location = quartz_set_cursor_location;
  klass->set_use_preedit = quartz_set_use_preedit;

  object_class->finalize = imquartz_finalize;
}

static void
ctk_im_context_quartz_init (CtkIMContext *im_context)
{
  CTK_NOTE (MISC, g_print ("ctk_im_context_quartz_init\n"));

  CtkIMContextQuartz *qc = CTK_IM_CONTEXT_QUARTZ (im_context);
  qc->preedit_str = g_strdup ("");
  qc->cursor_index = 0;
  qc->selected_len = 0;
  qc->cursor_rect = g_malloc (sizeof (CdkRectangle));
  qc->focused = FALSE;

  qc->slave = g_object_new (CTK_TYPE_IM_CONTEXT_SIMPLE, NULL);
  g_signal_connect (G_OBJECT (qc->slave), "commit", G_CALLBACK (commit_cb), qc);
}

static void
ctk_im_context_quartz_register_type (GTypeModule *module)
{
  const GTypeInfo object_info =
  {
    sizeof (CtkIMContextQuartzClass),
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) ctk_im_context_quartz_class_init,
    NULL,           /* class_finalize */
    NULL,           /* class_data */
    sizeof (CtkIMContextQuartz),
    0,
    (GInstanceInitFunc) ctk_im_context_quartz_init,
  };

  type_quartz =
    g_type_module_register_type (module,
                                 CTK_TYPE_IM_CONTEXT,
                                 "CtkIMContextQuartz",
                                 &object_info, 0);
}

MODULE_ENTRY (void, init) (GTypeModule * module)
{
  ctk_im_context_quartz_register_type (module);
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
  g_return_val_if_fail (context_id, NULL);

  if (!strcmp (context_id, "quartz"))
    {
      CTK_NOTE (MISC, g_print ("immodule_quartz create\n"));
      return g_object_new (type_quartz, NULL);
    }
  else
    return NULL;
}
