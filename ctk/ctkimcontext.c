/* CTK - The GIMP Toolkit
 * Copyright (C) 2000 Red Hat, Inc.
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

#include "config.h"
#include <string.h>
#include "ctkimcontext.h"
#include "ctkprivate.h"
#include "ctktypebuiltins.h"
#include "ctkmarshalers.h"
#include "ctkintl.h"

/**
 * SECTION:ctkimcontext
 * @title: CtkIMContext
 * @short_description: Base class for input method contexts
 * @include: ctk/ctk.h,ctk/ctkimmodule.h
 *
 * #CtkIMContext defines the interface for CTK+ input methods. An input method
 * is used by CTK+ text input widgets like #CtkEntry to map from key events to
 * Unicode character strings.
 *
 * The default input method can be set programmatically via the 
 * #CtkSettings:ctk-im-module CtkSettings property. Alternatively, you may set 
 * the CTK_IM_MODULE environment variable as documented in
 * [Running CTK+ Applications][ctk-running].
 *
 * The #CtkEntry #CtkEntry:im-module and #CtkTextView #CtkTextView:im-module 
 * properties may also be used to set input methods for specific widget 
 * instances. For instance, a certain entry widget might be expected to contain 
 * certain characters which would be easier to input with a certain input 
 * method.
 *
 * An input method may consume multiple key events in sequence and finally
 * output the composed result. This is called preediting, and an input method
 * may provide feedback about this process by displaying the intermediate
 * composition states as preedit text. For instance, the default CTK+ input
 * method implements the input of arbitrary Unicode code points by holding down
 * the Control and Shift keys and then typing “U” followed by the hexadecimal
 * digits of the code point.  When releasing the Control and Shift keys,
 * preediting ends and the character is inserted as text. Ctrl+Shift+u20AC for
 * example results in the € sign.
 *
 * Additional input methods can be made available for use by CTK+ widgets as
 * loadable modules. An input method module is a small shared library which
 * implements a subclass of #CtkIMContext or #CtkIMContextSimple and exports
 * these four functions:
 *
 * |[<!-- language="C" -->
 * void im_module_init(GTypeModule *module);
 * ]|
 * This function should register the #GType of the #CtkIMContext subclass which
 * implements the input method by means of g_type_module_register_type(). Note
 * that g_type_register_static() cannot be used as the type needs to be
 * registered dynamically.
 *
 * |[<!-- language="C" -->
 * void im_module_exit(void);
 * ]|
 * Here goes any cleanup code your input method might require on module unload.
 *
 * |[<!-- language="C" -->
 * void im_module_list(const CtkIMContextInfo ***contexts, int *n_contexts)
 * {
 *   *contexts = info_list;
 *   *n_contexts = G_N_ELEMENTS (info_list);
 * }
 * ]|
 * This function returns the list of input methods provided by the module. The
 * example implementation above shows a common solution and simply returns a
 * pointer to statically defined array of #CtkIMContextInfo items for each
 * provided input method.
 *
 * |[<!-- language="C" -->
 * CtkIMContext * im_module_create(const gchar *context_id);
 * ]|
 * This function should return a pointer to a newly created instance of the
 * #CtkIMContext subclass identified by @context_id. The context ID is the same
 * as specified in the #CtkIMContextInfo array returned by im_module_list().
 *
 * After a new loadable input method module has been installed on the system,
 * the configuration file `ctk.immodules` needs to be
 * regenerated by [ctk-query-immodules-3.0][ctk-query-immodules-3.0],
 * in order for the new input method to become available to CTK+ applications.
 */

enum {
  PREEDIT_START,
  PREEDIT_END,
  PREEDIT_CHANGED,
  COMMIT,
  RETRIEVE_SURROUNDING,
  DELETE_SURROUNDING,
  LAST_SIGNAL
};

enum {
  PROP_INPUT_PURPOSE = 1,
  PROP_INPUT_HINTS,
  LAST_PROPERTY
};

static guint im_context_signals[LAST_SIGNAL] = { 0, };
static GParamSpec *properties[LAST_PROPERTY] = { NULL, };

typedef struct _CtkIMContextPrivate CtkIMContextPrivate;
struct _CtkIMContextPrivate {
  CtkInputPurpose purpose;
  CtkInputHints hints;
};

static void     ctk_im_context_real_get_preedit_string (CtkIMContext   *context,
							gchar         **str,
							PangoAttrList **attrs,
							gint           *cursor_pos);
static gboolean ctk_im_context_real_filter_keypress    (CtkIMContext   *context,
							CdkEventKey    *event);
static gboolean ctk_im_context_real_get_surrounding    (CtkIMContext   *context,
							gchar         **text,
							gint           *cursor_index);
static void     ctk_im_context_real_set_surrounding    (CtkIMContext   *context,
							const char     *text,
							gint            len,
							gint            cursor_index);

static void     ctk_im_context_get_property            (GObject        *obj,
                                                        guint           property_id,
                                                        GValue         *value,
                                                        GParamSpec     *pspec);
static void     ctk_im_context_set_property            (GObject        *obj,
                                                        guint           property_id,
                                                        const GValue   *value,
                                                        GParamSpec     *pspec);


G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (CtkIMContext, ctk_im_context, G_TYPE_OBJECT)

/**
 * CtkIMContextClass:
 * @preedit_start: Default handler of the #CtkIMContext::preedit-start signal.
 * @preedit_end: Default handler of the #CtkIMContext::preedit-end signal.
 * @preedit_changed: Default handler of the #CtkIMContext::preedit-changed
 *   signal.
 * @commit: Default handler of the #CtkIMContext::commit signal.
 * @retrieve_surrounding: Default handler of the
 *   #CtkIMContext::retrieve-surrounding signal.
 * @delete_surrounding: Default handler of the
 *   #CtkIMContext::delete-surrounding signal.
 * @set_client_window: Called via ctk_im_context_set_client_window() when the
 *   input window where the entered text will appear changes. Override this to
 *   keep track of the current input window, for instance for the purpose of
 *   positioning a status display of your input method.
 * @get_preedit_string: Called via ctk_im_context_get_preedit_string() to
 *   retrieve the text currently being preedited for display at the cursor
 *   position. Any input method which composes complex characters or any
 *   other compositions from multiple sequential key presses should override
 *   this method to provide feedback.
 * @filter_keypress: Called via ctk_im_context_filter_keypress() on every
 *   key press or release event. Every non-trivial input method needs to
 *   override this in order to implement the mapping from key events to text.
 *   A return value of %TRUE indicates to the caller that the event was
 *   consumed by the input method. In that case, the #CtkIMContext::commit
 *   signal should be emitted upon completion of a key sequence to pass the
 *   resulting text back to the input widget. Alternatively, %FALSE may be
 *   returned to indicate that the event wasn’t handled by the input method.
 *   If a builtin mapping exists for the key, it is used to produce a
 *   character.
 * @focus_in: Called via ctk_im_context_focus_in() when the input widget
 *   has gained focus. May be overridden to keep track of the current focus.
 * @focus_out: Called via ctk_im_context_focus_out() when the input widget
 *   has lost focus. May be overridden to keep track of the current focus.
 * @reset: Called via ctk_im_context_reset() to signal a change such as a
 *   change in cursor position. An input method that implements preediting
 *   should override this method to clear the preedit state on reset.
 * @set_cursor_location: Called via ctk_im_context_set_cursor_location()
 *   to inform the input method of the current cursor location relative to
 *   the client window. May be overridden to implement the display of popup
 *   windows at the cursor position.
 * @set_use_preedit: Called via ctk_im_context_set_use_preedit() to control
 *   the use of the preedit string. Override this to display feedback by some
 *   other means if turned off.
 * @set_surrounding: Called via ctk_im_context_set_surrounding() in response
 *   to signal #CtkIMContext::retrieve-surrounding to update the input
 *   method’s idea of the context around the cursor. It is not necessary to
 *   override this method even with input methods which implement
 *   context-dependent behavior. The base implementation is sufficient for
 *   ctk_im_context_get_surrounding() to work.
 * @get_surrounding: Called via ctk_im_context_get_surrounding() to update
 *   the context around the cursor location. It is not necessary to override
 *   this method even with input methods which implement context-dependent
 *   behavior. The base implementation emits
 *   #CtkIMContext::retrieve-surrounding and records the context received
 *   by the subsequent invocation of @get_surrounding.
 */
static void
ctk_im_context_class_init (CtkIMContextClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = ctk_im_context_get_property;
  object_class->set_property = ctk_im_context_set_property;

  klass->get_preedit_string = ctk_im_context_real_get_preedit_string;
  klass->filter_keypress = ctk_im_context_real_filter_keypress;
  klass->get_surrounding = ctk_im_context_real_get_surrounding;
  klass->set_surrounding = ctk_im_context_real_set_surrounding;

  /**
   * CtkIMContext::preedit-start:
   * @context: the object on which the signal is emitted
   *
   * The ::preedit-start signal is emitted when a new preediting sequence
   * starts.
   */
  im_context_signals[PREEDIT_START] =
    g_signal_new (I_("preedit-start"),
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkIMContextClass, preedit_start),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);
  /**
   * CtkIMContext::preedit-end:
   * @context: the object on which the signal is emitted
   *
   * The ::preedit-end signal is emitted when a preediting sequence
   * has been completed or canceled.
   */
  im_context_signals[PREEDIT_END] =
    g_signal_new (I_("preedit-end"),
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkIMContextClass, preedit_end),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);
  /**
   * CtkIMContext::preedit-changed:
   * @context: the object on which the signal is emitted
   *
   * The ::preedit-changed signal is emitted whenever the preedit sequence
   * currently being entered has changed.  It is also emitted at the end of
   * a preedit sequence, in which case
   * ctk_im_context_get_preedit_string() returns the empty string.
   */
  im_context_signals[PREEDIT_CHANGED] =
    g_signal_new (I_("preedit-changed"),
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkIMContextClass, preedit_changed),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);
  /**
   * CtkIMContext::commit:
   * @context: the object on which the signal is emitted
   * @str: the completed character(s) entered by the user
   *
   * The ::commit signal is emitted when a complete input sequence
   * has been entered by the user. This can be a single character
   * immediately after a key press or the final result of preediting.
   */
  im_context_signals[COMMIT] =
    g_signal_new (I_("commit"),
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkIMContextClass, commit),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 1,
		  G_TYPE_STRING);
  /**
   * CtkIMContext::retrieve-surrounding:
   * @context: the object on which the signal is emitted
   *
   * The ::retrieve-surrounding signal is emitted when the input method
   * requires the context surrounding the cursor.  The callback should set
   * the input method surrounding context by calling the
   * ctk_im_context_set_surrounding() method.
   *
   * Returns: %TRUE if the signal was handled.
   */
  im_context_signals[RETRIEVE_SURROUNDING] =
    g_signal_new (I_("retrieve-surrounding"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkIMContextClass, retrieve_surrounding),
                  _ctk_boolean_handled_accumulator, NULL,
                  _ctk_marshal_BOOLEAN__VOID,
                  G_TYPE_BOOLEAN, 0);
  g_signal_set_va_marshaller (im_context_signals[RETRIEVE_SURROUNDING],
                              G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_BOOLEAN__VOIDv);
  /**
   * CtkIMContext::delete-surrounding:
   * @context: the object on which the signal is emitted
   * @offset:  the character offset from the cursor position of the text
   *           to be deleted. A negative value indicates a position before
   *           the cursor.
   * @n_chars: the number of characters to be deleted
   *
   * The ::delete-surrounding signal is emitted when the input method
   * needs to delete all or part of the context surrounding the cursor.
   *
   * Returns: %TRUE if the signal was handled.
   */
  im_context_signals[DELETE_SURROUNDING] =
    g_signal_new (I_("delete-surrounding"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkIMContextClass, delete_surrounding),
                  _ctk_boolean_handled_accumulator, NULL,
                  _ctk_marshal_BOOLEAN__INT_INT,
                  G_TYPE_BOOLEAN, 2,
                  G_TYPE_INT,
                  G_TYPE_INT);
  g_signal_set_va_marshaller (im_context_signals[DELETE_SURROUNDING],
                              G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_BOOLEAN__INT_INTv);

  properties[PROP_INPUT_PURPOSE] =
    g_param_spec_enum ("input-purpose",
                         P_("Purpose"),
                         P_("Purpose of the text field"),
                         CTK_TYPE_INPUT_PURPOSE,
                         CTK_INPUT_PURPOSE_FREE_FORM,
                         G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  properties[PROP_INPUT_HINTS] =
    g_param_spec_flags ("input-hints",
                         P_("hints"),
                         P_("Hints for the text field behaviour"),
                         CTK_TYPE_INPUT_HINTS,
                         CTK_INPUT_HINT_NONE,
                         G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROPERTY, properties);
}

static void
ctk_im_context_init (CtkIMContext *im_context G_GNUC_UNUSED)
{
}

static void
ctk_im_context_real_get_preedit_string (CtkIMContext   *context G_GNUC_UNUSED,
					gchar         **str,
					PangoAttrList **attrs,
					gint           *cursor_pos)
{
  if (str)
    *str = g_strdup ("");
  if (attrs)
    *attrs = pango_attr_list_new ();
  if (cursor_pos)
    *cursor_pos = 0;
}

static gboolean
ctk_im_context_real_filter_keypress (CtkIMContext *context G_GNUC_UNUSED,
				     CdkEventKey  *event G_GNUC_UNUSED)
{
  return FALSE;
}

typedef struct
{
  gchar *text;
  gint cursor_index;
} SurroundingInfo;

static void
ctk_im_context_real_set_surrounding (CtkIMContext  *context,
				     const gchar   *text,
				     gint           len,
				     gint           cursor_index)
{
  SurroundingInfo *info = g_object_get_data (G_OBJECT (context),
                                             "ctk-im-surrounding-info");

  if (info)
    {
      g_free (info->text);
      info->text = g_strndup (text, len);
      info->cursor_index = cursor_index;
    }
}

static gboolean
ctk_im_context_real_get_surrounding (CtkIMContext *context,
				     gchar       **text,
				     gint         *cursor_index)
{
  gboolean result;
  gboolean info_is_local = FALSE;
  SurroundingInfo local_info = { NULL, 0 };
  SurroundingInfo *info;
  
  info = g_object_get_data (G_OBJECT (context), "ctk-im-surrounding-info");
  if (!info)
    {
      info = &local_info;
      g_object_set_data (G_OBJECT (context), I_("ctk-im-surrounding-info"), info);
      info_is_local = TRUE;
    }
  
  g_signal_emit (context,
		 im_context_signals[RETRIEVE_SURROUNDING], 0,
		 &result);

  if (result)
    {
      *text = g_strdup (info->text ? info->text : "");
      *cursor_index = info->cursor_index;
    }
  else
    {
      *text = NULL;
      *cursor_index = 0;
    }

  if (info_is_local)
    {
      g_free (info->text);
      g_object_set_data (G_OBJECT (context), I_("ctk-im-surrounding-info"), NULL);
    }
  
  return result;
}

/**
 * ctk_im_context_set_client_window:
 * @context: a #CtkIMContext
 * @window: (allow-none):  the client window. This may be %NULL to indicate
 *           that the previous client window no longer exists.
 * 
 * Set the client window for the input context; this is the
 * #CdkWindow in which the input appears. This window is
 * used in order to correctly position status windows, and may
 * also be used for purposes internal to the input method.
 **/
void
ctk_im_context_set_client_window (CtkIMContext *context,
				  CdkWindow    *window)
{
  CtkIMContextClass *klass;
  
  g_return_if_fail (CTK_IS_IM_CONTEXT (context));

  klass = CTK_IM_CONTEXT_GET_CLASS (context);
  if (klass->set_client_window)
    klass->set_client_window (context, window);
}

/**
 * ctk_im_context_get_preedit_string:
 * @context:    a #CtkIMContext
 * @str:        (out) (transfer full): location to store the retrieved
 *              string. The string retrieved must be freed with g_free().
 * @attrs:      (out) (transfer full): location to store the retrieved
 *              attribute list.  When you are done with this list, you
 *              must unreference it with pango_attr_list_unref().
 * @cursor_pos: (out): location to store position of cursor (in characters)
 *              within the preedit string.  
 * 
 * Retrieve the current preedit string for the input context,
 * and a list of attributes to apply to the string.
 * This string should be displayed inserted at the insertion
 * point.
 **/
void
ctk_im_context_get_preedit_string (CtkIMContext   *context,
				   gchar         **str,
				   PangoAttrList **attrs,
				   gint           *cursor_pos)
{
  CtkIMContextClass *klass;
  
  g_return_if_fail (CTK_IS_IM_CONTEXT (context));
  
  klass = CTK_IM_CONTEXT_GET_CLASS (context);
  klass->get_preedit_string (context, str, attrs, cursor_pos);
  g_return_if_fail (str == NULL || g_utf8_validate (*str, -1, NULL));
}

/**
 * ctk_im_context_filter_keypress:
 * @context: a #CtkIMContext
 * @event: the key event
 * 
 * Allow an input method to internally handle key press and release 
 * events. If this function returns %TRUE, then no further processing
 * should be done for this key event.
 * 
 * Returns: %TRUE if the input method handled the key event.
 *
 **/
gboolean
ctk_im_context_filter_keypress (CtkIMContext *context,
				CdkEventKey  *key)
{
  CtkIMContextClass *klass;
  
  g_return_val_if_fail (CTK_IS_IM_CONTEXT (context), FALSE);
  g_return_val_if_fail (key != NULL, FALSE);

  klass = CTK_IM_CONTEXT_GET_CLASS (context);
  return klass->filter_keypress (context, key);
}

/**
 * ctk_im_context_focus_in:
 * @context: a #CtkIMContext
 *
 * Notify the input method that the widget to which this
 * input context corresponds has gained focus. The input method
 * may, for example, change the displayed feedback to reflect
 * this change.
 **/
void
ctk_im_context_focus_in (CtkIMContext   *context)
{
  CtkIMContextClass *klass;
  
  g_return_if_fail (CTK_IS_IM_CONTEXT (context));
  
  klass = CTK_IM_CONTEXT_GET_CLASS (context);
  if (klass->focus_in)
    klass->focus_in (context);
}

/**
 * ctk_im_context_focus_out:
 * @context: a #CtkIMContext
 *
 * Notify the input method that the widget to which this
 * input context corresponds has lost focus. The input method
 * may, for example, change the displayed feedback or reset the contexts
 * state to reflect this change.
 **/
void
ctk_im_context_focus_out (CtkIMContext   *context)
{
  CtkIMContextClass *klass;
  
  g_return_if_fail (CTK_IS_IM_CONTEXT (context));

  klass = CTK_IM_CONTEXT_GET_CLASS (context);
  if (klass->focus_out)
    klass->focus_out (context);
}

/**
 * ctk_im_context_reset:
 * @context: a #CtkIMContext
 *
 * Notify the input method that a change such as a change in cursor
 * position has been made. This will typically cause the input
 * method to clear the preedit state.
 **/
void
ctk_im_context_reset (CtkIMContext   *context)
{
  CtkIMContextClass *klass;
  
  g_return_if_fail (CTK_IS_IM_CONTEXT (context));

  klass = CTK_IM_CONTEXT_GET_CLASS (context);
  if (klass->reset)
    klass->reset (context);
}


/**
 * ctk_im_context_set_cursor_location:
 * @context: a #CtkIMContext
 * @area: new location
 *
 * Notify the input method that a change in cursor 
 * position has been made. The location is relative to the client
 * window.
 **/
void
ctk_im_context_set_cursor_location (CtkIMContext       *context,
				    const CdkRectangle *area)
{
  CtkIMContextClass *klass;
  
  g_return_if_fail (CTK_IS_IM_CONTEXT (context));

  klass = CTK_IM_CONTEXT_GET_CLASS (context);
  if (klass->set_cursor_location)
    klass->set_cursor_location (context, (CdkRectangle *) area);
}

/**
 * ctk_im_context_set_use_preedit:
 * @context: a #CtkIMContext
 * @use_preedit: whether the IM context should use the preedit string.
 * 
 * Sets whether the IM context should use the preedit string
 * to display feedback. If @use_preedit is FALSE (default
 * is TRUE), then the IM context may use some other method to display
 * feedback, such as displaying it in a child of the root window.
 **/
void
ctk_im_context_set_use_preedit (CtkIMContext *context,
				gboolean      use_preedit)
{
  CtkIMContextClass *klass;
  
  g_return_if_fail (CTK_IS_IM_CONTEXT (context));

  klass = CTK_IM_CONTEXT_GET_CLASS (context);
  if (klass->set_use_preedit)
    klass->set_use_preedit (context, use_preedit);
}

/**
 * ctk_im_context_set_surrounding:
 * @context: a #CtkIMContext 
 * @text: text surrounding the insertion point, as UTF-8.
 *        the preedit string should not be included within
 *        @text.
 * @len: the length of @text, or -1 if @text is nul-terminated
 * @cursor_index: the byte index of the insertion cursor within @text.
 * 
 * Sets surrounding context around the insertion point and preedit
 * string. This function is expected to be called in response to the
 * CtkIMContext::retrieve_surrounding signal, and will likely have no
 * effect if called at other times.
 **/
void
ctk_im_context_set_surrounding (CtkIMContext  *context,
				const gchar   *text,
				gint           len,
				gint           cursor_index)
{
  CtkIMContextClass *klass;
  
  g_return_if_fail (CTK_IS_IM_CONTEXT (context));
  g_return_if_fail (text != NULL || len == 0);

  if (text == NULL && len == 0)
    text = "";
  if (len < 0)
    len = strlen (text);

  g_return_if_fail (cursor_index >= 0 && cursor_index <= len);

  klass = CTK_IM_CONTEXT_GET_CLASS (context);
  if (klass->set_surrounding)
    klass->set_surrounding (context, text, len, cursor_index);
}

/**
 * ctk_im_context_get_surrounding:
 * @context: a #CtkIMContext
 * @text: (out) (transfer full): location to store a UTF-8 encoded
 *        string of text holding context around the insertion point.
 *        If the function returns %TRUE, then you must free the result
 *        stored in this location with g_free().
 * @cursor_index: (out): location to store byte index of the insertion
 *        cursor within @text.
 * 
 * Retrieves context around the insertion point. Input methods
 * typically want context in order to constrain input text based on
 * existing text; this is important for languages such as Thai where
 * only some sequences of characters are allowed.
 *
 * This function is implemented by emitting the
 * CtkIMContext::retrieve_surrounding signal on the input method; in
 * response to this signal, a widget should provide as much context as
 * is available, up to an entire paragraph, by calling
 * ctk_im_context_set_surrounding(). Note that there is no obligation
 * for a widget to respond to the ::retrieve_surrounding signal, so input
 * methods must be prepared to function without context.
 *
 * Returns: %TRUE if surrounding text was provided; in this case
 *    you must free the result stored in *text.
 **/
gboolean
ctk_im_context_get_surrounding (CtkIMContext *context,
				gchar       **text,
				gint         *cursor_index)
{
  CtkIMContextClass *klass;
  gchar *local_text = NULL;
  gint local_index;
  gboolean result = FALSE;
  
  g_return_val_if_fail (CTK_IS_IM_CONTEXT (context), FALSE);

  klass = CTK_IM_CONTEXT_GET_CLASS (context);
  if (klass->get_surrounding)
    result = klass->get_surrounding (context,
				     text ? text : &local_text,
				     cursor_index ? cursor_index : &local_index);

  if (result)
    g_free (local_text);

  return result;
}

/**
 * ctk_im_context_delete_surrounding:
 * @context: a #CtkIMContext
 * @offset: offset from cursor position in chars;
 *    a negative value means start before the cursor.
 * @n_chars: number of characters to delete.
 * 
 * Asks the widget that the input context is attached to to delete
 * characters around the cursor position by emitting the
 * CtkIMContext::delete_surrounding signal. Note that @offset and @n_chars
 * are in characters not in bytes which differs from the usage other
 * places in #CtkIMContext.
 *
 * In order to use this function, you should first call
 * ctk_im_context_get_surrounding() to get the current context, and
 * call this function immediately afterwards to make sure that you
 * know what you are deleting. You should also account for the fact
 * that even if the signal was handled, the input context might not
 * have deleted all the characters that were requested to be deleted.
 *
 * This function is used by an input method that wants to make
 * subsitutions in the existing text in response to new input. It is
 * not useful for applications.
 * 
 * Returns: %TRUE if the signal was handled.
 **/
gboolean
ctk_im_context_delete_surrounding (CtkIMContext *context,
				   gint          offset,
				   gint          n_chars)
{
  gboolean result;
  
  g_return_val_if_fail (CTK_IS_IM_CONTEXT (context), FALSE);

  g_signal_emit (context,
		 im_context_signals[DELETE_SURROUNDING], 0,
		 offset, n_chars, &result);

  return result;
}

static void
ctk_im_context_get_property (GObject    *obj,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  CtkIMContextPrivate *priv = ctk_im_context_get_instance_private (CTK_IM_CONTEXT (obj));

  switch (property_id)
    {
    case PROP_INPUT_PURPOSE:
      g_value_set_enum (value, priv->purpose);
      break;
    case PROP_INPUT_HINTS:
      g_value_set_flags (value, priv->hints);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
      break;
    }
}

static void
ctk_im_context_set_property (GObject      *obj,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  CtkIMContextPrivate *priv = ctk_im_context_get_instance_private (CTK_IM_CONTEXT (obj));

  switch (property_id)
    {
    case PROP_INPUT_PURPOSE:
      if (priv->purpose != g_value_get_enum (value))
        {
          priv->purpose = g_value_get_enum (value);
          g_object_notify_by_pspec (obj, pspec);
        }
      break;
    case PROP_INPUT_HINTS:
      if (priv->hints != g_value_get_flags (value))
        {
          priv->hints = g_value_get_flags (value);
          g_object_notify_by_pspec (obj, pspec);
        }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
      break;
    }
}
