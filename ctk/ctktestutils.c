/* Ctk+ testing utilities
 * Copyright (C) 2007 Imendio AB
 * Authors: Tim Janik
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

#include "ctkspinbutton.h"
#include "ctkmain.h"
#include "ctkbox.h"
#include "ctklabel.h"
#include "ctkbutton.h"
#include "ctktextview.h"
#include "ctkrange.h"

#include <locale.h>
#include <string.h>
#include <math.h>

/* This is a hack.
 * We want to include the same headers as ctktypefuncs.c but we are not
 * allowed to include ctkx.h directly during CTK compilation.
 * So....
 */
#undef CTK_COMPILATION
#include <ctk/ctkx.h>
#define CTK_COMPILATION

/**
 * SECTION:ctktesting
 * @Short_description: Utilities for testing CTK+ applications
 * @Title: Testing
 */

/**
 * ctk_test_init:
 * @argcp: Address of the `argc` parameter of the
 *        main() function. Changed if any arguments were handled.
 * @argvp: (inout) (array length=argcp): Address of the 
 *        `argv` parameter of main().
 *        Any parameters understood by g_test_init() or ctk_init() are
 *        stripped before return.
 * @...: currently unused
 *
 * This function is used to initialize a CTK+ test program.
 *
 * It will in turn call g_test_init() and ctk_init() to properly
 * initialize the testing framework and graphical toolkit. It’ll 
 * also set the program’s locale to “C” and prevent loading of rc 
 * files and Ctk+ modules. This is done to make tets program
 * environments as deterministic as possible.
 *
 * Like ctk_init() and g_test_init(), any known arguments will be
 * processed and stripped from @argc and @argv.
 *
 * Since: 2.14
 **/
void
ctk_test_init (int    *argcp,
               char ***argvp,
               ...)
{
  g_test_init (argcp, argvp, NULL);
  /* - enter C locale
   * - call g_test_init();
   * - call ctk_init();
   * - prevent RC files from loading;
   * - prevent Ctk modules from loading;
   * - supply mock object for CtkSettings
   * FUTURE TODO:
   * - this function could install a mock object around CtkSettings
   */
  g_setenv ("CTK_MODULES", "", TRUE);
  ctk_disable_setlocale();
  setlocale (LC_ALL, "C");
  g_test_bug_base ("http://bugzilla.gnome.org/show_bug.cgi?id=%s");

  /* XSendEvent() doesn't work yet on XI2 events.
   * So at the moment cdk_test_simulate_* can only
   * send events that CTK+ understands if XI2 is
   * disabled, bummer.
   */
  cdk_disable_multidevice ();

  ctk_init (argcp, argvp);
}

static GSList*
test_find_widget_input_windows (CtkWidget *widget,
                                gboolean   input_only)
{
  CdkWindow *window;
  GList *node, *children;
  GSList *matches = NULL;
  gpointer udata;

  window = ctk_widget_get_window (widget);

  cdk_window_get_user_data (window, &udata);
  if (udata == widget && (!input_only || (CDK_IS_WINDOW (window) && cdk_window_is_input_only (CDK_WINDOW (window)))))
    matches = g_slist_prepend (matches, window);
  children = cdk_window_get_children (ctk_widget_get_parent_window (widget));
  for (node = children; node; node = node->next)
    {
      cdk_window_get_user_data (node->data, &udata);
      if (udata == widget && (!input_only || (CDK_IS_WINDOW (node->data) && cdk_window_is_input_only (CDK_WINDOW (node->data)))))
        matches = g_slist_prepend (matches, node->data);
    }
  return g_slist_reverse (matches);
}

static gboolean
quit_main_loop_callback (CtkWidget     *widget G_GNUC_UNUSED,
                         CdkFrameClock *frame_clock G_GNUC_UNUSED,
                         gpointer       user_data G_GNUC_UNUSED)
{
  ctk_main_quit ();

  return G_SOURCE_REMOVE;
}

/**
 * ctk_test_widget_wait_for_draw:
 * @widget: the widget to wait for
 *
 * Enters the main loop and waits for @widget to be “drawn”. In this
 * context that means it waits for the frame clock of @widget to have
 * run a full styling, layout and drawing cycle.
 *
 * This function is intended to be used for syncing with actions that
 * depend on @widget relayouting or on interaction with the display
 * server.
 *
 * Since: 3.10
 **/
void
ctk_test_widget_wait_for_draw (CtkWidget *widget)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));

  /* We can do this here because the whole tick procedure does not
   * reenter the main loop. Otherwise we'd need to manually get the
   * frame clock and connect to the after-paint signal.
   */
  ctk_widget_add_tick_callback (widget,
                                quit_main_loop_callback,
                                NULL,
                                NULL);

  ctk_main ();
}

/**
 * ctk_test_widget_send_key:
 * @widget: Widget to generate a key press and release on.
 * @keyval: A Cdk keyboard value.
 * @modifiers: Keyboard modifiers the event is setup with.
 *
 * This function will generate keyboard press and release events in
 * the middle of the first CdkWindow found that belongs to @widget.
 * For windowless widgets like #CtkButton (which returns %FALSE from
 * ctk_widget_get_has_window()), this will often be an
 * input-only event window. For other widgets, this is usually widget->window.
 * Certain caveats should be considered when using this function, in
 * particular because the mouse pointer is warped to the key press
 * location, see cdk_test_simulate_key() for details.
 *
 * Returns: whether all actions neccessary for the key event simulation were carried out successfully.
 *
 * Since: 2.14
 **/
gboolean
ctk_test_widget_send_key (CtkWidget      *widget,
                          guint           keyval,
                          CdkModifierType modifiers)
{
  gboolean k1res, k2res;
  GSList *iwindows = test_find_widget_input_windows (widget, FALSE);
  if (!iwindows)
    iwindows = test_find_widget_input_windows (widget, TRUE);
  if (!iwindows)
    return FALSE;
  k1res = cdk_test_simulate_key (iwindows->data, -1, -1, keyval, modifiers, CDK_KEY_PRESS);
  k2res = cdk_test_simulate_key (iwindows->data, -1, -1, keyval, modifiers, CDK_KEY_RELEASE);
  g_slist_free (iwindows);
  return k1res && k2res;
}

/**
 * ctk_test_widget_click:
 * @widget: Widget to generate a button click on.
 * @button: Number of the pointer button for the event, usually 1, 2 or 3.
 * @modifiers: Keyboard modifiers the event is setup with.
 *
 * This function will generate a @button click (button press and button
 * release event) in the middle of the first CdkWindow found that belongs
 * to @widget.
 * For windowless widgets like #CtkButton (which returns %FALSE from
 * ctk_widget_get_has_window()), this will often be an
 * input-only event window. For other widgets, this is usually widget->window.
 * Certain caveats should be considered when using this function, in
 * particular because the mouse pointer is warped to the button click
 * location, see cdk_test_simulate_button() for details.
 *
 * Returns: whether all actions neccessary for the button click simulation were carried out successfully.
 *
 * Since: 2.14
 *
 * Deprecated: 3.20: This testing infrastructure is phased out in favor of reftests.
 **/
gboolean
ctk_test_widget_click (CtkWidget      *widget,
                       guint           button,
                       CdkModifierType modifiers)
{
  gboolean b1res, b2res;
  GSList *iwindows = test_find_widget_input_windows (widget, FALSE);
  if (!iwindows)
    iwindows = test_find_widget_input_windows (widget, TRUE);
  if (!iwindows)
    return FALSE;
  b1res = cdk_test_simulate_button (iwindows->data, -1, -1, button, modifiers, CDK_BUTTON_PRESS);
  b2res = cdk_test_simulate_button (iwindows->data, -1, -1, button, modifiers, CDK_BUTTON_RELEASE);
  g_slist_free (iwindows);
  return b1res && b2res;
}

/**
 * ctk_test_spin_button_click:
 * @spinner: valid CtkSpinButton widget.
 * @button:  Number of the pointer button for the event, usually 1, 2 or 3.
 * @upwards: %TRUE for upwards arrow click, %FALSE for downwards arrow click.
 *
 * This function will generate a @button click in the upwards or downwards
 * spin button arrow areas, usually leading to an increase or decrease of
 * spin button’s value.
 *
 * Returns: whether all actions neccessary for the button click simulation were carried out successfully.
 *
 * Since: 2.14
 *
 * Deprecated: 3.20: This testing infrastructure is phased out in favor of reftests.
 **/
gboolean
ctk_test_spin_button_click (CtkSpinButton  *spinner,
                            guint           button,
                            gboolean        upwards)
{
  CdkWindow *down_panel = NULL, *up_panel = NULL, *panel;
  gboolean b1res = FALSE, b2res = FALSE;

  _ctk_spin_button_get_panels (spinner, &down_panel, &up_panel);

  panel = (upwards) ? up_panel : down_panel;

  if (panel)
    {
      gint width = cdk_window_get_width (panel);
      b1res = cdk_test_simulate_button (panel, width - 1, 1, button, 0, CDK_BUTTON_PRESS);
      b2res = cdk_test_simulate_button (panel, width - 1, 1, button, 0, CDK_BUTTON_RELEASE);
    }
  return b1res && b2res;
}

/**
 * ctk_test_find_label:
 * @widget:        Valid label or container widget.
 * @label_pattern: Shell-glob pattern to match a label string.
 *
 * This function will search @widget and all its descendants for a CtkLabel
 * widget with a text string matching @label_pattern.
 * The @label_pattern may contain asterisks “*” and question marks “?” as
 * placeholders, g_pattern_match() is used for the matching.
 * Note that locales other than "C“ tend to alter (translate” label strings,
 * so this function is genrally only useful in test programs with
 * predetermined locales, see ctk_test_init() for more details.
 *
 * Returns: (transfer none): a CtkLabel widget if any is found.
 *
 * Since: 2.14
 **/
CtkWidget*
ctk_test_find_label (CtkWidget    *widget,
                     const gchar  *label_pattern)
{
  CtkWidget *label = NULL;

  if (CTK_IS_LABEL (widget))
    {
      const gchar *text = ctk_label_get_text (CTK_LABEL (widget));
      if (g_pattern_match_simple (label_pattern, text))
        return widget;
    }

  if (CTK_IS_CONTAINER (widget))
    {
      GList *node, *list;

      list = ctk_container_get_children (CTK_CONTAINER (widget));
      for (node = list; node; node = node->next)
        {
          label = ctk_test_find_label (node->data, label_pattern);
          if (label)
            break;
        }
      g_list_free (list);
    }
  return label;
}

static GList*
test_list_descendants (CtkWidget *widget,
                       GType      widget_type)
{
  GList *results = NULL;
  if (CTK_IS_CONTAINER (widget))
    {
      GList *node, *list = ctk_container_get_children (CTK_CONTAINER (widget));
      for (node = list; node; node = node->next)
        {
          if (!widget_type || g_type_is_a (G_OBJECT_TYPE (node->data), widget_type))
            results = g_list_prepend (results, node->data);
          else
            results = g_list_concat (results, test_list_descendants (node->data, widget_type));
        }
      g_list_free (list);
    }
  return results;
}

static int
widget_geo_dist (CtkWidget *a,
                 CtkWidget *b,
                 CtkWidget *base)
{
  CtkAllocation allocation;
  int ax0, ay0, ax1, ay1, bx0, by0, bx1, by1, xdist = 0, ydist = 0;

  ctk_widget_get_allocation (a, &allocation);
  if (!ctk_widget_translate_coordinates (a, base, 0, 0, &ax0, &ay0) ||
      !ctk_widget_translate_coordinates (a, base, allocation.width, allocation.height, &ax1, &ay1))
    return -G_MAXINT;

  ctk_widget_get_allocation (b, &allocation);
  if (!ctk_widget_translate_coordinates (b, base, 0, 0, &bx0, &by0) ||
      !ctk_widget_translate_coordinates (b, base, allocation.width, allocation.height, &bx1, &by1))
    return +G_MAXINT;

  if (bx0 >= ax1)
    xdist = bx0 - ax1;
  else if (ax0 >= bx1)
    xdist = ax0 - bx1;
  if (by0 >= ay1)
    ydist = by0 - ay1;
  else if (ay0 >= by1)
    ydist = ay0 - by1;

  return xdist + ydist;
}

static int
widget_geo_cmp (gconstpointer a,
                gconstpointer b,
                gpointer      user_data)
{
  gpointer *data = user_data;
  CtkWidget *wa = (void*) a, *wb = (void*) b, *toplevel = data[0], *base_widget = data[1];
  int adist = widget_geo_dist (wa, base_widget, toplevel);
  int bdist = widget_geo_dist (wb, base_widget, toplevel);
  return adist > bdist ? +1 : adist == bdist ? 0 : -1;
}

/**
 * ctk_test_find_sibling:
 * @base_widget:        Valid widget, part of a widget hierarchy
 * @widget_type:        Type of a aearched for sibling widget
 *
 * This function will search siblings of @base_widget and siblings of its
 * ancestors for all widgets matching @widget_type.
 * Of the matching widgets, the one that is geometrically closest to
 * @base_widget will be returned.
 * The general purpose of this function is to find the most likely “action”
 * widget, relative to another labeling widget. Such as finding a
 * button or text entry widget, given its corresponding label widget.
 *
 * Returns: (transfer none): a widget of type @widget_type if any is found.
 *
 * Since: 2.14
 **/
CtkWidget*
ctk_test_find_sibling (CtkWidget *base_widget,
                       GType      widget_type)
{
  GList *siblings = NULL;
  CtkWidget *tmpwidget = base_widget;
  gpointer data[2];
  /* find all sibling candidates */
  while (tmpwidget)
    {
      tmpwidget = ctk_widget_get_parent (tmpwidget);
      siblings = g_list_concat (siblings, test_list_descendants (tmpwidget, widget_type));
    }
  /* sort them by distance to base_widget */
  data[0] = ctk_widget_get_toplevel (base_widget);
  data[1] = base_widget;
  siblings = g_list_sort_with_data (siblings, widget_geo_cmp, data);
  /* pick nearest != base_widget */
  siblings = g_list_remove (siblings, base_widget);
  tmpwidget = siblings ? siblings->data : NULL;
  g_list_free (siblings);
  return tmpwidget;
}

/**
 * ctk_test_find_widget:
 * @widget:        Container widget, usually a CtkWindow.
 * @label_pattern: Shell-glob pattern to match a label string.
 * @widget_type:   Type of a aearched for label sibling widget.
 *
 * This function will search the descendants of @widget for a widget
 * of type @widget_type that has a label matching @label_pattern next
 * to it. This is most useful for automated GUI testing, e.g. to find
 * the “OK” button in a dialog and synthesize clicks on it.
 * However see ctk_test_find_label(), ctk_test_find_sibling() and
 * ctk_test_widget_click() for possible caveats involving the search of
 * such widgets and synthesizing widget events.
 *
 * Returns: (nullable) (transfer none): a valid widget if any is found or %NULL.
 *
 * Since: 2.14
 **/
CtkWidget*
ctk_test_find_widget (CtkWidget    *widget,
                      const gchar  *label_pattern,
                      GType         widget_type)
{
  CtkWidget *label = ctk_test_find_label (widget, label_pattern);
  if (!label)
    label = ctk_test_find_label (ctk_widget_get_toplevel (widget), label_pattern);
  if (label)
    return ctk_test_find_sibling (label, widget_type);
  return NULL;
}

/**
 * ctk_test_slider_set_perc:
 * @widget:     valid widget pointer.
 * @percentage: value between 0 and 100.
 *
 * This function will adjust the slider position of all CtkRange
 * based widgets, such as scrollbars or scales, it’ll also adjust
 * spin buttons. The adjustment value of these widgets is set to
 * a value between the lower and upper limits, according to the
 * @percentage argument.
 *
 * Since: 2.14
 *
 * Deprecated: 3.20: This testing infrastructure is phased out in favor of reftests.
 **/
void
ctk_test_slider_set_perc (CtkWidget      *widget,
                          double          percentage)
{
  CtkAdjustment *adjustment = NULL;
  if (CTK_IS_RANGE (widget))
    adjustment = ctk_range_get_adjustment (CTK_RANGE (widget));
  else if (CTK_IS_SPIN_BUTTON (widget))
    adjustment = ctk_spin_button_get_adjustment (CTK_SPIN_BUTTON (widget));
  if (adjustment)
    ctk_adjustment_set_value (adjustment, 
                              ctk_adjustment_get_lower (adjustment) 
                              + (ctk_adjustment_get_upper (adjustment) 
                                 - ctk_adjustment_get_lower (adjustment) 
                                 - ctk_adjustment_get_page_size (adjustment))
                                * percentage * 0.01);
}

/**
 * ctk_test_slider_get_value:
 * @widget:     valid widget pointer.
 *
 * Retrive the literal adjustment value for CtkRange based
 * widgets and spin buttons. Note that the value returned by
 * this function is anything between the lower and upper bounds
 * of the adjustment belonging to @widget, and is not a percentage
 * as passed in to ctk_test_slider_set_perc().
 *
 * Returns: ctk_adjustment_get_value (adjustment) for an adjustment belonging to @widget.
 *
 * Since: 2.14
 *
 * Deprecated: 3.20: This testing infrastructure is phased out in favor of reftests.
 **/
double
ctk_test_slider_get_value (CtkWidget *widget)
{
  CtkAdjustment *adjustment = NULL;
  if (CTK_IS_RANGE (widget))
    adjustment = ctk_range_get_adjustment (CTK_RANGE (widget));
  else if (CTK_IS_SPIN_BUTTON (widget))
    adjustment = ctk_spin_button_get_adjustment (CTK_SPIN_BUTTON (widget));
  return adjustment ? ctk_adjustment_get_value (adjustment) : 0;
}

/**
 * ctk_test_text_set:
 * @widget:     valid widget pointer.
 * @string:     a 0-terminated C string
 *
 * Set the text string of @widget to @string if it is a CtkLabel,
 * CtkEditable (entry and text widgets) or CtkTextView.
 *
 * Since: 2.14
 *
 * Deprecated: 3.20: This testing infrastructure is phased out in favor of reftests.
 **/
void
ctk_test_text_set (CtkWidget   *widget,
                   const gchar *string)
{
  if (CTK_IS_LABEL (widget))
    ctk_label_set_text (CTK_LABEL (widget), string);
  else if (CTK_IS_EDITABLE (widget))
    {
      int pos = 0;
      ctk_editable_delete_text (CTK_EDITABLE (widget), 0, -1);
      ctk_editable_insert_text (CTK_EDITABLE (widget), string, -1, &pos);
    }
  else if (CTK_IS_TEXT_VIEW (widget))
    {
      CtkTextBuffer *tbuffer = ctk_text_view_get_buffer (CTK_TEXT_VIEW (widget));
      ctk_text_buffer_set_text (tbuffer, string, -1);
    }
}

/**
 * ctk_test_text_get:
 * @widget:     valid widget pointer.
 *
 * Retrive the text string of @widget if it is a CtkLabel,
 * CtkEditable (entry and text widgets) or CtkTextView.
 *
 * Returns: new 0-terminated C string, needs to be released with g_free().
 *
 * Since: 2.14
 *
 * Deprecated: 3.20: This testing infrastructure is phased out in favor of reftests.
 **/
gchar*
ctk_test_text_get (CtkWidget *widget)
{
  if (CTK_IS_LABEL (widget))
    return g_strdup (ctk_label_get_text (CTK_LABEL (widget)));
  else if (CTK_IS_EDITABLE (widget))
    {
      return g_strdup (ctk_editable_get_chars (CTK_EDITABLE (widget), 0, -1));
    }
  else if (CTK_IS_TEXT_VIEW (widget))
    {
      CtkTextBuffer *tbuffer = ctk_text_view_get_buffer (CTK_TEXT_VIEW (widget));
      CtkTextIter start, end;
      ctk_text_buffer_get_start_iter (tbuffer, &start);
      ctk_text_buffer_get_end_iter (tbuffer, &end);
      return ctk_text_buffer_get_text (tbuffer, &start, &end, FALSE);
    }
  return NULL;
}

/**
 * ctk_test_create_widget:
 * @widget_type: a valid widget type.
 * @first_property_name: (allow-none): Name of first property to set or %NULL
 * @...: value to set the first property to, followed by more
 *    name-value pairs, terminated by %NULL
 *
 * This function wraps g_object_new() for widget types.
 * It’ll automatically show all created non window widgets, also
 * g_object_ref_sink() them (to keep them alive across a running test)
 * and set them up for destruction during the next test teardown phase.
 *
 * Returns: (transfer none): a newly created widget.
 *
 * Since: 2.14
 *
 * Deprecated: 3.20: This testing infrastructure is phased out in favor of reftests.
 */
CtkWidget*
ctk_test_create_widget (GType        widget_type,
                        const gchar *first_property_name,
                        ...)
{
  CtkWidget *widget;
  va_list var_args;
  g_return_val_if_fail (g_type_is_a (widget_type, CTK_TYPE_WIDGET), NULL);
  va_start (var_args, first_property_name);
  widget = (CtkWidget*) g_object_new_valist (widget_type, first_property_name, var_args);
  va_end (var_args);
  if (widget)
    {
      if (!CTK_IS_WINDOW (widget))
        ctk_widget_show (widget);
      g_object_ref_sink (widget);
      g_test_queue_unref (widget);
      g_test_queue_destroy ((GDestroyNotify) ctk_widget_destroy, widget);
    }
  return widget;
}

static void
try_main_quit (void)
{
  if (ctk_main_level())
    ctk_main_quit();
}

static int
test_increment_intp (int *intp)
{
  if (intp != NULL)
    *intp += 1;
  return 1; /* TRUE in case we're connected to event signals */
}

/**
 * ctk_test_display_button_window:
 * @window_title:       Title of the window to be displayed.
 * @dialog_text:        Text inside the window to be displayed.
 * @...:                %NULL terminated list of (const char *label, int *nump) pairs.
 *
 * Create a window with window title @window_title, text contents @dialog_text,
 * and a number of buttons, according to the paired argument list given
 * as @... parameters.
 * Each button is created with a @label and a ::clicked signal handler that
 * incremrents the integer stored in @nump.
 * The window will be automatically shown with ctk_widget_show_now() after
 * creation, so when this function returns it has already been mapped,
 * resized and positioned on screen.
 * The window will quit any running ctk_main()-loop when destroyed, and it
 * will automatically be destroyed upon test function teardown.
 *
 * Returns: (transfer full): a widget pointer to the newly created CtkWindow.
 *
 * Since: 2.14
 *
 * Deprecated: 3.20: This testing infrastructure is phased out in favor of reftests.
 **/
CtkWidget*
ctk_test_display_button_window (const gchar *window_title,
                                const gchar *dialog_text,
                                ...) /* NULL terminated list of (label, &int) pairs */
{
  va_list var_args;
  CtkWidget *window, *vbox;
  const char *arg1;
G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  window = ctk_test_create_widget (CTK_TYPE_WINDOW, "title", window_title, NULL);
  vbox = ctk_test_create_widget (CTK_TYPE_BOX, "parent", window, "orientation", CTK_ORIENTATION_VERTICAL, NULL);
  ctk_test_create_widget (CTK_TYPE_LABEL, "label", dialog_text, "parent", vbox, NULL);
G_GNUC_END_IGNORE_DEPRECATIONS;
  g_signal_connect (window, "destroy", G_CALLBACK (try_main_quit), NULL);
  va_start (var_args, dialog_text);
  arg1 = va_arg (var_args, const char*);
  while (arg1)
    {
      int *arg2 = va_arg (var_args, int*);
      CtkWidget *button;
G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      button = ctk_test_create_widget (CTK_TYPE_BUTTON, "label", arg1, "parent", vbox, NULL);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      g_signal_connect_swapped (button, "clicked", G_CALLBACK (test_increment_intp), arg2);
      arg1 = va_arg (var_args, const char*);
    }
  va_end (var_args);
  ctk_widget_show_all (vbox);
  ctk_widget_show_now (window);
  while (ctk_events_pending ())
    ctk_main_iteration ();
  return window;
}

/**
 * ctk_test_create_simple_window:
 * @window_title:       Title of the window to be displayed.
 * @dialog_text:        Text inside the window to be displayed.
 *
 * Create a simple window with window title @window_title and
 * text contents @dialog_text.
 * The window will quit any running ctk_main()-loop when destroyed, and it
 * will automatically be destroyed upon test function teardown.
 *
 * Returns: (transfer none): a widget pointer to the newly created CtkWindow.
 *
 * Since: 2.14
 *
 * Deprecated: 3.20: This testing infrastructure is phased out in favor of reftests.
 **/
CtkWidget*
ctk_test_create_simple_window (const gchar *window_title,
                               const gchar *dialog_text)
{
  CtkWidget *window, *vbox;
G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  window = ctk_test_create_widget (CTK_TYPE_WINDOW, "title", window_title, NULL);
  vbox = ctk_test_create_widget (CTK_TYPE_BOX, "parent", window, "orientation", CTK_ORIENTATION_VERTICAL, NULL);
  ctk_test_create_widget (CTK_TYPE_LABEL, "label", dialog_text, "parent", vbox, NULL);
G_GNUC_END_IGNORE_DEPRECATIONS;
  g_signal_connect (window, "destroy", G_CALLBACK (try_main_quit), NULL);
  ctk_widget_show_all (vbox);
  return window;
}

static GType *all_registered_types = NULL;
static guint  n_all_registered_types = 0;

/**
 * ctk_test_list_all_types:
 * @n_types: location to store number of types
 *
 * Return the type ids that have been registered after
 * calling ctk_test_register_all_types().
 *
 * Returns: (array length=n_types zero-terminated=1) (transfer none):
 *    0-terminated array of type ids
 *
 * Since: 2.14
 */
const GType*
ctk_test_list_all_types (guint *n_types)
{
  if (n_types)
    *n_types = n_all_registered_types;
  return all_registered_types;
}

/**
 * ctk_test_register_all_types:
 *
 * Force registration of all core Ctk+ and Cdk object types.
 * This allowes to refer to any of those object types via
 * g_type_from_name() after calling this function.
 *
 * Since: 2.14
 **/
void
ctk_test_register_all_types (void)
{
  if (!all_registered_types)
    {
      const guint max_ctk_types = 999;
      GType *tp;
      all_registered_types = g_new0 (GType, max_ctk_types);
      tp = all_registered_types;
#include "ctktypefuncs.inc"
      n_all_registered_types = tp - all_registered_types;
      g_assert (n_all_registered_types + 1 < max_ctk_types);
      *tp = 0;
    }
}
