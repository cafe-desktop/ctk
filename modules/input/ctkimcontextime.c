/*
 * ctkimcontextime.c
 * Copyright (C) 2003 Takuro Ashie
 * Copyright (C) 2003-2004 Kazuki IWAMOTO
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
 *
 */

/*
 *  Please see the following site for the detail of Windows IME API.
 *  http://msdn.microsoft.com/library/default.asp?url=/library/en-us/appendix/hh/appendix/imeimes2_35ph.asp
 */

#ifdef CTK_DISABLE_DEPRECATED
#undef CTK_DISABLE_DEPRECATED
#endif

#include "config.h"
#include "ctkimcontextime.h"

#include "imm-extra.h"

#include "cdk/cdkkeysyms-compat.h"
#include "cdk/win32/cdkwin32.h"
#include "cdk/win32/cdkprivate-win32.h"
#include "cdk/cdkkeysyms.h"
#include "cdk/cdkinternals.h"

#include <pango/pango.h>

/* avoid warning */
#ifdef STRICT
# undef STRICT
# include <pango/pangowin32.h>
# ifndef STRICT
#   define STRICT 1
# endif
#else /* STRICT */
#   include <pango/pangowin32.h>
#endif /* STRICT */

/* Determines what happens when focus is lost while preedit is in process. */
typedef enum {
  /* Preedit is committed. */
  CTK_WIN32_IME_FOCUS_BEHAVIOR_COMMIT,
  /* Preedit is discarded. */
  CTK_WIN32_IME_FOCUS_BEHAVIOR_DISCARD,
  /* Preedit follows the cursor (that means it will appear in the widget
   * that receives the focus) */
  CTK_WIN32_IME_FOCUS_BEHAVIOR_FOLLOW,
} CtkWin32IMEFocusBehavior;

struct _CtkIMContextIMEPrivate
{
  /* When pretend_empty_preedit is set to TRUE,
   * ctk_im_context_ime_get_preedit_string() will return an empty string
   * instead of the actual content of ImmGetCompositionStringW().
   *
   * This is necessary because CtkEntry expects the preedit buffer to be
   * cleared before commit() is called, otherwise it leads to an assertion
   * failure in Pango. However, since we emit the commit() signal while
   * handling the WM_IME_COMPOSITION message, the IME buffer will be non-empty,
   * so we temporarily set this flag while emmiting the appropriate signals.
   *
   * See also:
   *   https://bugzilla.gnome.org/show_bug.cgi?id=787142
   *   https://gitlab.gnome.org/GNOME/ctk/commit/c255ba68fc2c918dd84da48a472e7973d3c00b03
   */
  gboolean pretend_empty_preedit;
  CtkWin32IMEFocusBehavior focus_behavior;
};


/* GObject class methods */
static void ctk_im_context_ime_class_init (CtkIMContextIMEClass *class);
static void ctk_im_context_ime_init       (CtkIMContextIME      *context_ime);
static void ctk_im_context_ime_dispose    (GObject              *obj);
static void ctk_im_context_ime_finalize   (GObject              *obj);

static void ctk_im_context_ime_set_property (GObject      *object,
                                             guint         prop_id,
                                             const GValue *value,
                                             GParamSpec   *pspec);
static void ctk_im_context_ime_get_property (GObject      *object,
                                             guint         prop_id,
                                             GValue       *value,
                                             GParamSpec   *pspec);

/* CtkIMContext's virtual functions */
static void ctk_im_context_ime_set_client_window   (CtkIMContext *context,
                                                    CdkWindow    *client_window);
static gboolean ctk_im_context_ime_filter_keypress (CtkIMContext   *context,
                                                    CdkEventKey    *event);
static void ctk_im_context_ime_reset               (CtkIMContext   *context);
static void ctk_im_context_ime_get_preedit_string  (CtkIMContext   *context,
                                                    gchar         **str,
                                                    PangoAttrList **attrs,
                                                    gint           *cursor_pos);
static void ctk_im_context_ime_focus_in            (CtkIMContext   *context);
static void ctk_im_context_ime_focus_out           (CtkIMContext   *context);
static void ctk_im_context_ime_set_cursor_location (CtkIMContext   *context,
                                                    CdkRectangle   *area);
static void ctk_im_context_ime_set_use_preedit     (CtkIMContext   *context,
                                                    gboolean        use_preedit);

/* CtkIMContextIME's private functions */
static void ctk_im_context_ime_set_preedit_font (CtkIMContext    *context);

static CdkFilterReturn
ctk_im_context_ime_message_filter               (CdkXEvent       *xevent,
                                                 CdkEvent        *event,
                                                 gpointer         data);
static void get_window_position                 (CdkWindow       *win,
                                                 gint            *x,
                                                 gint            *y);
static void cb_client_widget_hierarchy_changed  (CtkWidget       *widget,
                                                 CtkWidget       *widget2,
                                                 CtkIMContextIME *context_ime);

GType ctk_type_im_context_ime = 0;
static GObjectClass *parent_class;

void
ctk_im_context_ime_register_type (GTypeModule *type_module)
{
  const GTypeInfo im_context_ime_info = {
    sizeof (CtkIMContextIMEClass),
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) ctk_im_context_ime_class_init,
    NULL,                       /* class_finalize */
    NULL,                       /* class_data */
    sizeof (CtkIMContextIME),
    0,
    (GInstanceInitFunc) ctk_im_context_ime_init,
  };

  ctk_type_im_context_ime =
    g_type_module_register_type (type_module,
                                 CTK_TYPE_IM_CONTEXT,
                                 "CtkIMContextIME", &im_context_ime_info, 0);
}

static void
ctk_im_context_ime_class_init (CtkIMContextIMEClass *class)
{
  CtkIMContextClass *im_context_class = CTK_IM_CONTEXT_CLASS (class);
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

  parent_class = g_type_class_peek_parent (class);

  gobject_class->finalize     = ctk_im_context_ime_finalize;
  gobject_class->dispose      = ctk_im_context_ime_dispose;
  gobject_class->set_property = ctk_im_context_ime_set_property;
  gobject_class->get_property = ctk_im_context_ime_get_property;

  im_context_class->set_client_window   = ctk_im_context_ime_set_client_window;
  im_context_class->filter_keypress     = ctk_im_context_ime_filter_keypress;
  im_context_class->reset               = ctk_im_context_ime_reset;
  im_context_class->get_preedit_string  = ctk_im_context_ime_get_preedit_string;
  im_context_class->focus_in            = ctk_im_context_ime_focus_in;
  im_context_class->focus_out           = ctk_im_context_ime_focus_out;
  im_context_class->set_cursor_location = ctk_im_context_ime_set_cursor_location;
  im_context_class->set_use_preedit     = ctk_im_context_ime_set_use_preedit;
}

static void
ctk_im_context_ime_init (CtkIMContextIME *context_ime)
{
  context_ime->client_window          = NULL;
  context_ime->toplevel               = NULL;
  context_ime->use_preedit            = TRUE;
  context_ime->preediting             = FALSE;
  context_ime->opened                 = FALSE;
  context_ime->focus                  = FALSE;
  context_ime->cursor_location.x      = 0;
  context_ime->cursor_location.y      = 0;
  context_ime->cursor_location.width  = 0;
  context_ime->cursor_location.height = 0;
  context_ime->commit_string          = NULL;

  context_ime->priv = g_malloc0 (sizeof (CtkIMContextIMEPrivate));
  context_ime->priv->focus_behavior = CTK_WIN32_IME_FOCUS_BEHAVIOR_COMMIT;
}


static void
ctk_im_context_ime_dispose (GObject *obj)
{
  CtkIMContext *context = CTK_IM_CONTEXT (obj);
  CtkIMContextIME *context_ime = CTK_IM_CONTEXT_IME (obj);

  if (context_ime->client_window != NULL)
    ctk_im_context_ime_set_client_window (context, NULL);

  if (G_OBJECT_CLASS (parent_class)->dispose)
    G_OBJECT_CLASS (parent_class)->dispose (obj);
}


static void
ctk_im_context_ime_finalize (GObject *obj)
{
  CtkIMContextIME *context_ime = CTK_IM_CONTEXT_IME (obj);

  g_free (context_ime->priv);
  context_ime->priv = NULL;
  if (G_OBJECT_CLASS (parent_class)->finalize)
    G_OBJECT_CLASS (parent_class)->finalize (obj);
}


static void
ctk_im_context_ime_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  CtkIMContextIME *context_ime = CTK_IM_CONTEXT_IME (object);

  g_return_if_fail (CTK_IS_IM_CONTEXT_IME (context_ime));

  switch (prop_id)
    {
    default:
      break;
    }
}


static void
ctk_im_context_ime_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  CtkIMContextIME *context_ime = CTK_IM_CONTEXT_IME (object);

  g_return_if_fail (CTK_IS_IM_CONTEXT_IME (context_ime));

  switch (prop_id)
    {
    default:
      break;
    }
}


CtkIMContext *
ctk_im_context_ime_new (void)
{
  return g_object_new (CTK_TYPE_IM_CONTEXT_IME, NULL);
}


static void
ctk_im_context_ime_set_client_window (CtkIMContext *context,
                                      CdkWindow    *client_window)
{
  CtkIMContextIME *context_ime;
  CdkWindow *toplevel = NULL;

  g_return_if_fail (CTK_IS_IM_CONTEXT_IME (context));
  context_ime = CTK_IM_CONTEXT_IME (context);

  if (client_window != NULL && !CDK_IS_WINDOW (client_window))
    {
      g_warning ("client_window is not a CdkWindow!");
      client_window = NULL;
    }

  if (client_window != NULL)
    {
      toplevel = cdk_window_get_toplevel (client_window);

      if (CDK_IS_WINDOW (toplevel))
        {
          HWND hwnd = cdk_win32_window_get_impl_hwnd (toplevel);
          HIMC himc = ImmGetContext (hwnd);
          if (himc)
	    {
	      context_ime->opened = ImmGetOpenStatus (himc);
	      ImmReleaseContext (hwnd, himc);
	    }
          else
            {
              context_ime->opened = FALSE;
            }
        }
      else
        {
          g_warning ("Could not find toplevel window.");
        }
    }
  else if (context_ime->focus)
    {
      ctk_im_context_ime_focus_out (context);
    }

  context_ime->client_window = client_window;
  context_ime->toplevel = toplevel;

  if (client_window)
    g_return_if_fail (CDK_IS_WINDOW (context_ime->toplevel));
}

static gboolean
ctk_im_context_ime_filter_keypress (CtkIMContext *context,
                                    CdkEventKey  *event)
{
  CtkIMContextIME *context_ime;
  CdkEventPrivate *event_priv;
  gchar *utf8;

  g_return_val_if_fail (CTK_IS_IM_CONTEXT_IME (context), FALSE);
  g_return_val_if_fail (event, FALSE);

  context_ime = CTK_IM_CONTEXT_IME (context);

  g_return_val_if_fail (cdk_event_is_allocated ((CdkEvent*)event), FALSE);

  event_priv = (CdkEventPrivate*) event;
  if (event_priv->translation_len == 0)
    return FALSE;

  utf8 = g_utf16_to_utf8 (event_priv->translation, event_priv->translation_len, NULL, NULL, NULL);
  g_signal_emit_by_name (context_ime, "commit", utf8);
  g_free (utf8);

  return TRUE;
}


static void
ctk_im_context_ime_reset (CtkIMContext *context)
{
  CtkIMContextIME *context_ime = CTK_IM_CONTEXT_IME (context);
  HWND hwnd;
  HIMC himc;

  if (!CDK_IS_WINDOW (context_ime->client_window))
    return;

  g_return_if_fail (CDK_IS_WINDOW (context_ime->toplevel));

  hwnd = cdk_win32_window_get_impl_hwnd (context_ime->toplevel);
  himc = ImmGetContext (hwnd);
  if (!himc)
    return;

  ImmNotifyIME (himc, NI_COMPOSITIONSTR, CPS_CANCEL, 0);

  if (context_ime->preediting)
    {
      context_ime->preediting = FALSE;
      g_signal_emit_by_name (context, "preedit-changed");
    }

  ImmReleaseContext (hwnd, himc);
}


static gchar *
get_utf8_preedit_string (CtkIMContextIME *context_ime,
                         gint             kind,
                         gint            *pos_ret)
{
  gunichar2 *utf16str = NULL;
  glong size;
  gchar *utf8str = NULL;
  HWND hwnd;
  HIMC himc;
  gint pos = 0;
  GError *error = NULL;

  if (pos_ret)
    *pos_ret = 0;

  if (!CDK_IS_WINDOW (context_ime->toplevel))
    return g_strdup ("");
  hwnd = cdk_win32_window_get_impl_hwnd (context_ime->toplevel);
  himc = ImmGetContext (hwnd);
  if (!himc)
    return g_strdup ("");

  size = ImmGetCompositionStringW (himc, kind, NULL, 0);
  if (size > 0)
    {
      utf16str = g_malloc (size);

      ImmGetCompositionStringW (himc, kind, utf16str, size);
      utf8str = g_utf16_to_utf8 (utf16str, size / sizeof (gunichar2),
                                 NULL, NULL, &error);
      if (error)
        {
          g_warning ("%s", error->message);
          g_error_free (error);
        }

      if (pos_ret)
        {
          pos = ImmGetCompositionStringW (himc, GCS_CURSORPOS, NULL, 0);
          if (pos < 0 || size < pos)
            {
              g_warning ("ImmGetCompositionString: "
                         "Invalid cursor position!");
              pos = 0;
            }
        }
    }

  if (!utf8str)
    {
      utf8str = g_strdup ("");
      pos = 0;
    }

  if (pos_ret)
    *pos_ret = pos;

  ImmReleaseContext (hwnd, himc);

  g_free (utf16str);

  return utf8str;
}


static PangoAttrList *
get_pango_attr_list (CtkIMContextIME *context_ime, const gchar *utf8str)
{
  PangoAttrList *attrs = pango_attr_list_new ();
  HWND hwnd;
  HIMC himc;
  guint8 *buf = NULL;

  if (!CDK_IS_WINDOW (context_ime->client_window))
    return attrs;

  g_return_val_if_fail (CDK_IS_WINDOW (context_ime->toplevel), attrs);

  hwnd = cdk_win32_window_get_impl_hwnd (context_ime->toplevel);
  himc = ImmGetContext (hwnd);
  if (!himc)
    return attrs;

  if (context_ime->preediting)
    {
      const gchar *schr = utf8str, *echr;
      guint16 f_red, f_green, f_blue, b_red, b_green, b_blue;
      glong len, spos = 0, epos, sidx = 0, eidx;
      PangoAttribute *attr;

      /*
       *  get attributes list of IME.
       */
      len = ImmGetCompositionStringW (himc, GCS_COMPATTR, NULL, 0);
      buf = g_malloc (len);
      ImmGetCompositionStringW (himc, GCS_COMPATTR, buf, len);

      /*
       *  schr and echr are pointer in utf8str.
       */
      for (echr = g_utf8_next_char (utf8str); *schr != '\0';
           echr = g_utf8_next_char (echr))
        {
          /*
           *  spos and epos are buf(attributes list of IME) position by
           *  bytes.
           *  Using the wide-char API, this value is same with UTF-8 offset.
           */
	  epos = g_utf8_pointer_to_offset (utf8str, echr);

          /*
           *  sidx and eidx are positions in utf8str by bytes.
           */
          eidx = echr - utf8str;

          /*
           *  convert attributes list to PangoAttriute.
           */
          if (*echr == '\0' || buf[spos] != buf[epos])
            {
              switch (buf[spos])
                {
                case ATTR_TARGET_CONVERTED:
                  attr = pango_attr_underline_new (PANGO_UNDERLINE_DOUBLE);
                  attr->start_index = sidx;
                  attr->end_index = eidx;
                  pango_attr_list_change (attrs, attr);
                  f_red = f_green = f_blue = 0;
                  b_red = b_green = b_blue = 0xffff;
                  break;
                case ATTR_TARGET_NOTCONVERTED:
                  f_red = f_green = f_blue = 0xffff;
                  b_red = b_green = b_blue = 0;
                  break;
                case ATTR_INPUT_ERROR:
                  f_red = f_green = f_blue = 0;
                  b_red = b_green = b_blue = 0x7fff;
                  break;
                default:        /* ATTR_INPUT,ATTR_CONVERTED,ATTR_FIXEDCONVERTED */
                  attr = pango_attr_underline_new (PANGO_UNDERLINE_SINGLE);
                  attr->start_index = sidx;
                  attr->end_index = eidx;
                  pango_attr_list_change (attrs, attr);
                  f_red = f_green = f_blue = 0;
                  b_red = b_green = b_blue = 0xffff;
                }
              attr = pango_attr_foreground_new (f_red, f_green, f_blue);
              attr->start_index = sidx;
              attr->end_index = eidx;
              pango_attr_list_change (attrs, attr);
              attr = pango_attr_background_new (b_red, b_green, b_blue);
              attr->start_index = sidx;
              attr->end_index = eidx;
              pango_attr_list_change (attrs, attr);

              schr = echr;
              spos = epos;
              sidx = eidx;
            }
        }
    }

  ImmReleaseContext (hwnd, himc);
  g_free(buf);

  return attrs;
}


static void
ctk_im_context_ime_get_preedit_string (CtkIMContext   *context,
                                       gchar         **str,
                                       PangoAttrList **attrs,
                                       gint           *cursor_pos)
{
  gchar *utf8str = NULL;
  gint pos = 0;
  CtkIMContextIME *context_ime;

  context_ime = CTK_IM_CONTEXT_IME (context);

  if (!context_ime->focus || context_ime->priv->pretend_empty_preedit)
    utf8str = g_strdup ("");
  else
    utf8str = get_utf8_preedit_string (context_ime, GCS_COMPSTR, &pos);

  if (attrs)
    *attrs = get_pango_attr_list (context_ime, utf8str);

  if (str)
    *str = utf8str;
  else
    g_free (utf8str);

  if (cursor_pos)
    *cursor_pos = pos;
}


static void
ctk_im_context_ime_focus_in (CtkIMContext *context)
{
  CtkIMContextIME *context_ime = CTK_IM_CONTEXT_IME (context);
  CdkWindow *toplevel = NULL;
  CtkWidget *widget = NULL;
  HWND hwnd;
  HIMC himc;

  if (!CDK_IS_WINDOW (context_ime->client_window))
    return;

  /* swtich current context */
  context_ime->focus = TRUE;

  toplevel = cdk_window_get_toplevel (context_ime->client_window);
  if (!CDK_IS_WINDOW (toplevel))
    {
      g_warning ("Could not find toplevel window.");
      context_ime->toplevel = NULL;
      context_ime->opened = FALSE;
      return;
    }

  hwnd = cdk_win32_window_get_impl_hwnd (toplevel);
  himc = ImmGetContext (hwnd);
  if (!himc)
    return;

  cdk_window_add_filter (toplevel,
                         ctk_im_context_ime_message_filter, context_ime);

  /* trace reparenting (probably no need) */
  cdk_window_get_user_data (context_ime->client_window, (gpointer) & widget);
  if (CTK_IS_WIDGET (widget))
    {
      g_signal_connect (widget, "hierarchy-changed",
                        G_CALLBACK (cb_client_widget_hierarchy_changed),
                        context_ime);
    }
  else
    {
      /* warning? */
    }

  context_ime->opened = ImmGetOpenStatus (himc);

  switch (context_ime->priv->focus_behavior)
    {
    case CTK_WIN32_IME_FOCUS_BEHAVIOR_COMMIT:
    case CTK_WIN32_IME_FOCUS_BEHAVIOR_DISCARD:
      ctk_im_context_ime_reset (context);
      break;

    case CTK_WIN32_IME_FOCUS_BEHAVIOR_FOLLOW:
      {
        gchar *utf8str = get_utf8_preedit_string (context_ime, GCS_COMPSTR, NULL);
        if (utf8str != NULL && strlen(utf8str) > 0)
          {
            context_ime->preediting = TRUE;
            ctk_im_context_ime_set_cursor_location (context, NULL);
            g_signal_emit_by_name (context, "preedit-start");
            g_signal_emit_by_name (context, "preedit-changed");
          }
        g_free (utf8str);
      }
      break;
    }

  /* clean */
  ImmReleaseContext (hwnd, himc);
}


static void
ctk_im_context_ime_focus_out (CtkIMContext *context)
{
  CtkIMContextIME *context_ime = CTK_IM_CONTEXT_IME (context);
  CtkWidget *widget = NULL;
  gboolean was_preediting;

  if (!CDK_IS_WINDOW (context_ime->client_window))
    return;

  was_preediting = context_ime->preediting;

  context_ime->opened = FALSE;
  context_ime->preediting = FALSE;
  context_ime->focus = FALSE;

  switch (context_ime->priv->focus_behavior)
    {
      case CTK_WIN32_IME_FOCUS_BEHAVIOR_COMMIT:
        if (was_preediting)
          {
            gchar *utf8str = get_utf8_preedit_string (context_ime, GCS_COMPSTR, NULL);

            context_ime->priv->pretend_empty_preedit = TRUE;
            g_signal_emit_by_name (context, "preedit-changed");
            g_signal_emit_by_name (context, "preedit-end");

            g_signal_emit_by_name (context, "commit", utf8str);

            g_signal_emit_by_name (context, "preedit-start");
            g_signal_emit_by_name (context, "preedit-changed");
            context_ime->priv->pretend_empty_preedit = FALSE;

            g_free (utf8str);
          }
        /* fallthrough */

      case CTK_WIN32_IME_FOCUS_BEHAVIOR_DISCARD:
        ctk_im_context_ime_reset (context);

        /* Callbacks triggered by im_context_ime_reset() could set the focus back to our
           context. In that case, we want to exit here. */

        if (context_ime->focus)
          return;

        break;

      case CTK_WIN32_IME_FOCUS_BEHAVIOR_FOLLOW:
        break;
    }

  /* remove signal handler */
  cdk_window_get_user_data (context_ime->client_window, (gpointer) & widget);
  if (CTK_IS_WIDGET (widget))
    {
      g_signal_handlers_disconnect_by_func
        (G_OBJECT (widget),
         G_CALLBACK (cb_client_widget_hierarchy_changed), context_ime);
    }

  /* remove filter */
  if (CDK_IS_WINDOW (context_ime->toplevel))
    {
      cdk_window_remove_filter (context_ime->toplevel,
                                ctk_im_context_ime_message_filter,
                                context_ime);
    }

  if (was_preediting)
    {
      g_signal_emit_by_name (context, "preedit-changed");
      g_signal_emit_by_name (context, "preedit-end");
    }
}


static void
ctk_im_context_ime_set_cursor_location (CtkIMContext *context,
                                        CdkRectangle *area)
{
  gint wx = 0, wy = 0;
  CtkIMContextIME *context_ime;
  COMPOSITIONFORM cf;
  HWND hwnd;
  HIMC himc;
  guint scale;

  g_return_if_fail (CTK_IS_IM_CONTEXT_IME (context));

  context_ime = CTK_IM_CONTEXT_IME (context);
  if (area)
    context_ime->cursor_location = *area;

  if (!context_ime->client_window)
    return;

  hwnd = cdk_win32_window_get_impl_hwnd (context_ime->toplevel);
  himc = ImmGetContext (hwnd);
  if (!himc)
    return;

  scale = cdk_window_get_scale_factor (context_ime->client_window);
  get_window_position (context_ime->client_window, &wx, &wy);
  cf.dwStyle = CFS_POINT;
  cf.ptCurrentPos.x = (wx + context_ime->cursor_location.x) * scale;
  cf.ptCurrentPos.y = (wy + context_ime->cursor_location.y) * scale;
  ImmSetCompositionWindow (himc, &cf);

  ImmReleaseContext (hwnd, himc);
}


static void
ctk_im_context_ime_set_use_preedit (CtkIMContext *context,
                                    gboolean      use_preedit)
{
  CtkIMContextIME *context_ime;

  g_return_if_fail (CTK_IS_IM_CONTEXT_IME (context));
  context_ime = CTK_IM_CONTEXT_IME (context);

  context_ime->use_preedit = use_preedit;
  if (context_ime->preediting)
    {
      HWND hwnd;
      HIMC himc;

      hwnd = cdk_win32_window_get_impl_hwnd (context_ime->toplevel);
      himc = ImmGetContext (hwnd);
      if (!himc)
        return;

      /* FIXME: What to do? */

      ImmReleaseContext (hwnd, himc);
    }
}


static void
ctk_im_context_ime_set_preedit_font (CtkIMContext *context)
{
  CtkIMContextIME *context_ime;
  CtkWidget *widget = NULL;
  HWND hwnd;
  HIMC himc;
  HKL ime = GetKeyboardLayout (0);
  const gchar *lang;
  gunichar wc;
  PangoContext *pango_context;
  PangoFont *font;
  LOGFONT *logfont;
  CtkStyleContext *style;
  PangoFontDescription *font_desc;

  g_return_if_fail (CTK_IS_IM_CONTEXT_IME (context));

  context_ime = CTK_IM_CONTEXT_IME (context);
  if (!context_ime->client_window)
    return;

  cdk_window_get_user_data (context_ime->client_window, (gpointer) &widget);
  if (!CTK_IS_WIDGET (widget))
    return;

  hwnd = cdk_win32_window_get_impl_hwnd (context_ime->toplevel);
  himc = ImmGetContext (hwnd);
  if (!himc)
    return;

  /* set font */
  pango_context = ctk_widget_get_pango_context (widget);
  if (!pango_context)
    goto ERROR_OUT;

  /* Try to make sure we use a font that actually can show the
   * language in question.
   */ 

  switch (PRIMARYLANGID (LOWORD (ime)))
    {
    case LANG_JAPANESE:
      lang = "ja"; break;
    case LANG_KOREAN:
      lang = "ko"; break;
    case LANG_CHINESE:
      switch (SUBLANGID (LOWORD (ime)))
	{
	case SUBLANG_CHINESE_TRADITIONAL:
	  lang = "zh_TW"; break;
	case SUBLANG_CHINESE_SIMPLIFIED:
	  lang = "zh_CN"; break;
	case SUBLANG_CHINESE_HONGKONG:
	  lang = "zh_HK"; break;
	case SUBLANG_CHINESE_SINGAPORE:
	  lang = "zh_SG"; break;
	case SUBLANG_CHINESE_MACAU:
	  lang = "zh_MO"; break;
	default:
	  lang = "zh"; break;
	}
      break;
    default:
      lang = ""; break;
    }

  style = ctk_widget_get_style_context (widget);
  ctk_style_context_save (style);
  ctk_style_context_set_state (style, CTK_STATE_FLAG_NORMAL);
  ctk_style_context_get (style,
                         ctk_style_context_get_state (style),
                         "font",
                         &font_desc,
                         NULL);
  ctk_style_context_restore (style);
  
  if (lang[0])
    {
      /* We know what language it is. Look for a character, any
       * character, that language needs.
       */
      PangoLanguage *pango_lang = pango_language_from_string (lang);
      PangoFontset *fontset =
        pango_context_load_fontset (pango_context,
				                            font_desc,
				                            pango_lang);
      gunichar *sample =
	g_utf8_to_ucs4 (pango_language_get_sample_string (pango_lang),
			-1, NULL, NULL, NULL);
      wc = 0x4E00;		/* In all CJK languages? */
      if (sample != NULL)
	{
	  int i;

	  for (i = 0; sample[i]; i++)
	    if (g_unichar_iswide (sample[i]))
	      {
		wc = sample[i];
		break;
	      }
	  g_free (sample);
	}
      font = pango_fontset_get_font (fontset, wc);
      g_object_unref (fontset);
    }
  else
    font = pango_context_load_font (pango_context, font_desc);

  if (!font)
    goto ERROR_OUT;

  logfont = pango_win32_font_logfont (font);
  if (logfont)
    ImmSetCompositionFont (himc, logfont);

  g_object_unref (font);

ERROR_OUT:
  /* clean */
  ImmReleaseContext (hwnd, himc);
}

static CdkFilterReturn
ctk_im_context_ime_message_filter (CdkXEvent *xevent,
                                   CdkEvent  *event,
                                   gpointer   data)
{
  CtkIMContext *context;
  CtkIMContextIME *context_ime;
  HWND hwnd;
  HIMC himc;
  MSG *msg = (MSG *) xevent;
  CdkFilterReturn retval = CDK_FILTER_CONTINUE;

  g_return_val_if_fail (CTK_IS_IM_CONTEXT_IME (data), retval);

  context = CTK_IM_CONTEXT (data);
  context_ime = CTK_IM_CONTEXT_IME (data);

  if (!context_ime->focus)
    return retval;

  g_return_val_if_fail (CDK_IS_WINDOW (context_ime->toplevel), retval);

  hwnd = cdk_win32_window_get_impl_hwnd (context_ime->toplevel);
  himc = ImmGetContext (hwnd);
  if (!himc)
    return retval;

  switch (msg->message)
    {
    case WM_IME_COMPOSITION:
      {
        gint wx = 0, wy = 0;
        CANDIDATEFORM cf;
        guint scale = cdk_window_get_scale_factor (context_ime->client_window);

        get_window_position (context_ime->client_window, &wx, &wy);
        /* FIXME! */
        {
          POINT pt;
          RECT rc;
          GetWindowRect (hwnd, &rc);
          pt.x = wx * scale;
          pt.y = wy * scale;
          ClientToScreen (hwnd, &pt);
          wx = (pt.x - rc.left) / scale;
          wy = (pt.y - rc.top) / scale;
        }
        cf.dwIndex = 0;
        cf.dwStyle = CFS_CANDIDATEPOS;
        cf.ptCurrentPos.x = (wx + context_ime->cursor_location.x) * scale;
        cf.ptCurrentPos.y = (wy + context_ime->cursor_location.y
          + context_ime->cursor_location.height) * scale;
        ImmSetCandidateWindow (himc, &cf);

        if ((msg->lParam & GCS_COMPSTR))
          g_signal_emit_by_name (context, "preedit-changed");

        if (msg->lParam & GCS_RESULTSTR)
          {
            gchar *utf8str = get_utf8_preedit_string (context_ime, GCS_RESULTSTR, NULL);

            if (utf8str)
              {
                context_ime->priv->pretend_empty_preedit = TRUE;
                g_signal_emit_by_name (context, "preedit-changed");
                g_signal_emit_by_name (context, "preedit-end");

                g_signal_emit_by_name (context, "commit", utf8str);

                g_signal_emit_by_name (context, "preedit-start");
                g_signal_emit_by_name (context, "preedit-changed");
                context_ime->priv->pretend_empty_preedit = FALSE;

                retval = TRUE;
              }

            g_free (utf8str);
          }

        if (context_ime->use_preedit)
          retval = TRUE;
      }
      break;

    case WM_IME_STARTCOMPOSITION:
      context_ime->preediting = TRUE;
      ctk_im_context_ime_set_cursor_location (context, NULL);
      g_signal_emit_by_name (context, "preedit-start");
      if (context_ime->use_preedit)
        retval = TRUE;
      break;

    case WM_IME_ENDCOMPOSITION:
      context_ime->preediting = FALSE;
      g_signal_emit_by_name (context, "preedit-changed");
      g_signal_emit_by_name (context, "preedit-end");

      if (context_ime->use_preedit)
        retval = TRUE;
      break;

    case WM_IME_NOTIFY:
      switch (msg->wParam)
        {
        case IMN_SETOPENSTATUS:
          context_ime->opened = ImmGetOpenStatus (himc);
          ctk_im_context_ime_set_preedit_font (context);
          break;

        default:
          break;
        }

    default:
      break;
    }

  ImmReleaseContext (hwnd, himc);
  return retval;
}


/*
 * x and y must be initialized to 0.
 */
static void
get_window_position (CdkWindow *win, gint *x, gint *y)
{
  CdkWindow *parent, *toplevel;
  gint wx, wy;

  g_return_if_fail (CDK_IS_WINDOW (win));
  g_return_if_fail (x && y);

  cdk_window_get_position (win, &wx, &wy);
  *x += wx;
  *y += wy;
  parent = cdk_window_get_parent (win);
  toplevel = cdk_window_get_toplevel (win);

  if (parent && parent != toplevel)
    get_window_position (parent, x, y);
}


/*
 *  probably, this handler isn't needed.
 */
static void
cb_client_widget_hierarchy_changed (CtkWidget       *widget,
                                    CtkWidget       *widget2,
                                    CtkIMContextIME *context_ime)
{
  CdkWindow *new_toplevel;

  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (CTK_IS_IM_CONTEXT_IME (context_ime));

  if (!context_ime->client_window)
    return;
  if (!context_ime->focus)
    return;

  new_toplevel = cdk_window_get_toplevel (context_ime->client_window);
  if (context_ime->client_window)
    g_return_if_fail (new_toplevel != NULL);
  if (context_ime->toplevel == new_toplevel)
    return;

  /* remove filter from old toplevel */
  if (CDK_IS_WINDOW (context_ime->toplevel))
    {
      cdk_window_remove_filter (context_ime->toplevel,
                                ctk_im_context_ime_message_filter,
                                context_ime);
    }

  /* add filter to new toplevel */
  if (CDK_IS_WINDOW (new_toplevel))
    {
      cdk_window_add_filter (new_toplevel,
                             ctk_im_context_ime_message_filter, context_ime);
    }

  context_ime->toplevel = new_toplevel;
}
