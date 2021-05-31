/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CTK_ENUMS_H__
#define __CTK_ENUMS_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <glib-object.h>


/**
 * SECTION:ctkenums
 * @Short_description: Public enumerated types used throughout GTK+
 * @Title: Standard Enumerations
 */


G_BEGIN_DECLS

/**
 * CtkAlign:
 * @CTK_ALIGN_FILL: stretch to fill all space if possible, center if
 *     no meaningful way to stretch
 * @CTK_ALIGN_START: snap to left or top side, leaving space on right
 *     or bottom
 * @CTK_ALIGN_END: snap to right or bottom side, leaving space on left
 *     or top
 * @CTK_ALIGN_CENTER: center natural width of widget inside the
 *     allocation
 * @CTK_ALIGN_BASELINE: align the widget according to the baseline. Since 3.10.
 *
 * Controls how a widget deals with extra space in a single (x or y)
 * dimension.
 *
 * Alignment only matters if the widget receives a “too large” allocation,
 * for example if you packed the widget with the #CtkWidget:expand
 * flag inside a #CtkBox, then the widget might get extra space.  If
 * you have for example a 16x16 icon inside a 32x32 space, the icon
 * could be scaled and stretched, it could be centered, or it could be
 * positioned to one side of the space.
 *
 * Note that in horizontal context @CTK_ALIGN_START and @CTK_ALIGN_END
 * are interpreted relative to text direction.
 *
 * CTK_ALIGN_BASELINE support for it is optional for containers and widgets, and
 * it is only supported for vertical alignment.  When its not supported by
 * a child or a container it is treated as @CTK_ALIGN_FILL.
 */
typedef enum
{
  CTK_ALIGN_FILL,
  CTK_ALIGN_START,
  CTK_ALIGN_END,
  CTK_ALIGN_CENTER,
  CTK_ALIGN_BASELINE
} CtkAlign;

/**
 * CtkArrowType:
 * @CTK_ARROW_UP: Represents an upward pointing arrow.
 * @CTK_ARROW_DOWN: Represents a downward pointing arrow.
 * @CTK_ARROW_LEFT: Represents a left pointing arrow.
 * @CTK_ARROW_RIGHT: Represents a right pointing arrow.
 * @CTK_ARROW_NONE: No arrow. Since 2.10.
 *
 * Used to indicate the direction in which an arrow should point.
 */
typedef enum
{
  CTK_ARROW_UP,
  CTK_ARROW_DOWN,
  CTK_ARROW_LEFT,
  CTK_ARROW_RIGHT,
  CTK_ARROW_NONE
} CtkArrowType;

/**
 * CtkBaselinePosition:
 * @CTK_BASELINE_POSITION_TOP: Align the baseline at the top
 * @CTK_BASELINE_POSITION_CENTER: Center the baseline
 * @CTK_BASELINE_POSITION_BOTTOM: Align the baseline at the bottom
 *
 * Whenever a container has some form of natural row it may align
 * children in that row along a common typographical baseline. If
 * the amount of verical space in the row is taller than the total
 * requested height of the baseline-aligned children then it can use a
 * #CtkBaselinePosition to select where to put the baseline inside the
 * extra availible space.
 *
 * Since: 3.10
 */
typedef enum
{
  CTK_BASELINE_POSITION_TOP,
  CTK_BASELINE_POSITION_CENTER,
  CTK_BASELINE_POSITION_BOTTOM
} CtkBaselinePosition;

/**
 * CtkDeleteType:
 * @CTK_DELETE_CHARS: Delete characters.
 * @CTK_DELETE_WORD_ENDS: Delete only the portion of the word to the
 *   left/right of cursor if we’re in the middle of a word.
 * @CTK_DELETE_WORDS: Delete words.
 * @CTK_DELETE_DISPLAY_LINES: Delete display-lines. Display-lines
 *   refers to the visible lines, with respect to to the current line
 *   breaks. As opposed to paragraphs, which are defined by line
 *   breaks in the input.
 * @CTK_DELETE_DISPLAY_LINE_ENDS: Delete only the portion of the
 *   display-line to the left/right of cursor.
 * @CTK_DELETE_PARAGRAPH_ENDS: Delete to the end of the
 *   paragraph. Like C-k in Emacs (or its reverse).
 * @CTK_DELETE_PARAGRAPHS: Delete entire line. Like C-k in pico.
 * @CTK_DELETE_WHITESPACE: Delete only whitespace. Like M-\ in Emacs.
 *
 * See also: #CtkEntry::delete-from-cursor.
 */
typedef enum
{
  CTK_DELETE_CHARS,
  CTK_DELETE_WORD_ENDS,
  CTK_DELETE_WORDS,
  CTK_DELETE_DISPLAY_LINES,
  CTK_DELETE_DISPLAY_LINE_ENDS,
  CTK_DELETE_PARAGRAPH_ENDS,
  CTK_DELETE_PARAGRAPHS,
  CTK_DELETE_WHITESPACE
} CtkDeleteType;

/* Focus movement types */
/**
 * CtkDirectionType:
 * @CTK_DIR_TAB_FORWARD: Move forward.
 * @CTK_DIR_TAB_BACKWARD: Move backward.
 * @CTK_DIR_UP: Move up.
 * @CTK_DIR_DOWN: Move down.
 * @CTK_DIR_LEFT: Move left.
 * @CTK_DIR_RIGHT: Move right.
 *
 * Focus movement types.
 */
typedef enum
{
  CTK_DIR_TAB_FORWARD,
  CTK_DIR_TAB_BACKWARD,
  CTK_DIR_UP,
  CTK_DIR_DOWN,
  CTK_DIR_LEFT,
  CTK_DIR_RIGHT
} CtkDirectionType;

/**
 * CtkIconSize:
 * @CTK_ICON_SIZE_INVALID: Invalid size.
 * @CTK_ICON_SIZE_MENU: Size appropriate for menus (16px).
 * @CTK_ICON_SIZE_SMALL_TOOLBAR: Size appropriate for small toolbars (16px).
 * @CTK_ICON_SIZE_LARGE_TOOLBAR: Size appropriate for large toolbars (24px)
 * @CTK_ICON_SIZE_BUTTON: Size appropriate for buttons (16px)
 * @CTK_ICON_SIZE_DND: Size appropriate for drag and drop (32px)
 * @CTK_ICON_SIZE_DIALOG: Size appropriate for dialogs (48px)
 *
 * Built-in stock icon sizes.
 */
typedef enum
{
  CTK_ICON_SIZE_INVALID,
  CTK_ICON_SIZE_MENU,
  CTK_ICON_SIZE_SMALL_TOOLBAR,
  CTK_ICON_SIZE_LARGE_TOOLBAR,
  CTK_ICON_SIZE_BUTTON,
  CTK_ICON_SIZE_DND,
  CTK_ICON_SIZE_DIALOG
} CtkIconSize;

/**
 * CtkSensitivityType:
 * @CTK_SENSITIVITY_AUTO: The arrow is made insensitive if the
 *   thumb is at the end
 * @CTK_SENSITIVITY_ON: The arrow is always sensitive
 * @CTK_SENSITIVITY_OFF: The arrow is always insensitive
 *
 * Determines how GTK+ handles the sensitivity of stepper arrows
 * at the end of range widgets.
 */
typedef enum
{
  CTK_SENSITIVITY_AUTO,
  CTK_SENSITIVITY_ON,
  CTK_SENSITIVITY_OFF
} CtkSensitivityType;

/* Reading directions for text */
/**
 * CtkTextDirection:
 * @CTK_TEXT_DIR_NONE: No direction.
 * @CTK_TEXT_DIR_LTR: Left to right text direction.
 * @CTK_TEXT_DIR_RTL: Right to left text direction.
 *
 * Reading directions for text.
 */
typedef enum
{
  CTK_TEXT_DIR_NONE,
  CTK_TEXT_DIR_LTR,
  CTK_TEXT_DIR_RTL
} CtkTextDirection;

/**
 * CtkJustification:
 * @CTK_JUSTIFY_LEFT: The text is placed at the left edge of the label.
 * @CTK_JUSTIFY_RIGHT: The text is placed at the right edge of the label.
 * @CTK_JUSTIFY_CENTER: The text is placed in the center of the label.
 * @CTK_JUSTIFY_FILL: The text is placed is distributed across the label.
 *
 * Used for justifying the text inside a #CtkLabel widget. (See also
 * #CtkAlignment).
 */
typedef enum
{
  CTK_JUSTIFY_LEFT,
  CTK_JUSTIFY_RIGHT,
  CTK_JUSTIFY_CENTER,
  CTK_JUSTIFY_FILL
} CtkJustification;

/**
 * CtkMenuDirectionType:
 * @CTK_MENU_DIR_PARENT: To the parent menu shell
 * @CTK_MENU_DIR_CHILD: To the submenu, if any, associated with the item
 * @CTK_MENU_DIR_NEXT: To the next menu item
 * @CTK_MENU_DIR_PREV: To the previous menu item
 *
 * An enumeration representing directional movements within a menu.
 */
typedef enum
{
  CTK_MENU_DIR_PARENT,
  CTK_MENU_DIR_CHILD,
  CTK_MENU_DIR_NEXT,
  CTK_MENU_DIR_PREV
} CtkMenuDirectionType;

/**
 * CtkMessageType:
 * @CTK_MESSAGE_INFO: Informational message
 * @CTK_MESSAGE_WARNING: Non-fatal warning message
 * @CTK_MESSAGE_QUESTION: Question requiring a choice
 * @CTK_MESSAGE_ERROR: Fatal error message
 * @CTK_MESSAGE_OTHER: None of the above
 *
 * The type of message being displayed in the dialog.
 */
typedef enum
{
  CTK_MESSAGE_INFO,
  CTK_MESSAGE_WARNING,
  CTK_MESSAGE_QUESTION,
  CTK_MESSAGE_ERROR,
  CTK_MESSAGE_OTHER
} CtkMessageType;

/**
 * CtkMovementStep:
 * @CTK_MOVEMENT_LOGICAL_POSITIONS: Move forward or back by graphemes
 * @CTK_MOVEMENT_VISUAL_POSITIONS:  Move left or right by graphemes
 * @CTK_MOVEMENT_WORDS:             Move forward or back by words
 * @CTK_MOVEMENT_DISPLAY_LINES:     Move up or down lines (wrapped lines)
 * @CTK_MOVEMENT_DISPLAY_LINE_ENDS: Move to either end of a line
 * @CTK_MOVEMENT_PARAGRAPHS:        Move up or down paragraphs (newline-ended lines)
 * @CTK_MOVEMENT_PARAGRAPH_ENDS:    Move to either end of a paragraph
 * @CTK_MOVEMENT_PAGES:             Move by pages
 * @CTK_MOVEMENT_BUFFER_ENDS:       Move to ends of the buffer
 * @CTK_MOVEMENT_HORIZONTAL_PAGES:  Move horizontally by pages
 */
typedef enum
{
  CTK_MOVEMENT_LOGICAL_POSITIONS,
  CTK_MOVEMENT_VISUAL_POSITIONS,
  CTK_MOVEMENT_WORDS,
  CTK_MOVEMENT_DISPLAY_LINES,
  CTK_MOVEMENT_DISPLAY_LINE_ENDS,
  CTK_MOVEMENT_PARAGRAPHS,
  CTK_MOVEMENT_PARAGRAPH_ENDS,
  CTK_MOVEMENT_PAGES,
  CTK_MOVEMENT_BUFFER_ENDS,
  CTK_MOVEMENT_HORIZONTAL_PAGES
} CtkMovementStep;

/**
 * CtkScrollStep:
 * @CTK_SCROLL_STEPS: Scroll in steps.
 * @CTK_SCROLL_PAGES: Scroll by pages.
 * @CTK_SCROLL_ENDS: Scroll to ends.
 * @CTK_SCROLL_HORIZONTAL_STEPS: Scroll in horizontal steps.
 * @CTK_SCROLL_HORIZONTAL_PAGES: Scroll by horizontal pages.
 * @CTK_SCROLL_HORIZONTAL_ENDS: Scroll to the horizontal ends.
 */
typedef enum
{
  CTK_SCROLL_STEPS,
  CTK_SCROLL_PAGES,
  CTK_SCROLL_ENDS,
  CTK_SCROLL_HORIZONTAL_STEPS,
  CTK_SCROLL_HORIZONTAL_PAGES,
  CTK_SCROLL_HORIZONTAL_ENDS
} CtkScrollStep;

/**
 * CtkOrientation:
 * @CTK_ORIENTATION_HORIZONTAL: The element is in horizontal orientation.
 * @CTK_ORIENTATION_VERTICAL: The element is in vertical orientation.
 *
 * Represents the orientation of widgets and other objects which can be switched
 * between horizontal and vertical orientation on the fly, like #CtkToolbar or
 * #CtkGesturePan.
 */
typedef enum
{
  CTK_ORIENTATION_HORIZONTAL,
  CTK_ORIENTATION_VERTICAL
} CtkOrientation;

/**
 * CtkPackType:
 * @CTK_PACK_START: The child is packed into the start of the box
 * @CTK_PACK_END: The child is packed into the end of the box
 *
 * Represents the packing location #CtkBox children. (See: #CtkVBox,
 * #CtkHBox, and #CtkButtonBox).
 */
typedef enum
{
  CTK_PACK_START,
  CTK_PACK_END
} CtkPackType;

/**
 * CtkPositionType:
 * @CTK_POS_LEFT: The feature is at the left edge.
 * @CTK_POS_RIGHT: The feature is at the right edge.
 * @CTK_POS_TOP: The feature is at the top edge.
 * @CTK_POS_BOTTOM: The feature is at the bottom edge.
 *
 * Describes which edge of a widget a certain feature is positioned at, e.g. the
 * tabs of a #CtkNotebook, the handle of a #CtkHandleBox or the label of a
 * #CtkScale.
 */
typedef enum
{
  CTK_POS_LEFT,
  CTK_POS_RIGHT,
  CTK_POS_TOP,
  CTK_POS_BOTTOM
} CtkPositionType;

/**
 * CtkReliefStyle:
 * @CTK_RELIEF_NORMAL: Draw a normal relief.
 * @CTK_RELIEF_HALF: A half relief. Deprecated in 3.14, does the same as @CTK_RELIEF_NORMAL
 * @CTK_RELIEF_NONE: No relief.
 *
 * Indicated the relief to be drawn around a #CtkButton.
 */
typedef enum
{
  CTK_RELIEF_NORMAL,
  CTK_RELIEF_HALF,
  CTK_RELIEF_NONE
} CtkReliefStyle;

/**
 * CtkScrollType:
 * @CTK_SCROLL_NONE: No scrolling.
 * @CTK_SCROLL_JUMP: Jump to new location.
 * @CTK_SCROLL_STEP_BACKWARD: Step backward.
 * @CTK_SCROLL_STEP_FORWARD: Step forward.
 * @CTK_SCROLL_PAGE_BACKWARD: Page backward.
 * @CTK_SCROLL_PAGE_FORWARD: Page forward.
 * @CTK_SCROLL_STEP_UP: Step up.
 * @CTK_SCROLL_STEP_DOWN: Step down.
 * @CTK_SCROLL_PAGE_UP: Page up.
 * @CTK_SCROLL_PAGE_DOWN: Page down.
 * @CTK_SCROLL_STEP_LEFT: Step to the left.
 * @CTK_SCROLL_STEP_RIGHT: Step to the right.
 * @CTK_SCROLL_PAGE_LEFT: Page to the left.
 * @CTK_SCROLL_PAGE_RIGHT: Page to the right.
 * @CTK_SCROLL_START: Scroll to start.
 * @CTK_SCROLL_END: Scroll to end.
 *
 * Scrolling types.
 */
typedef enum
{
  CTK_SCROLL_NONE,
  CTK_SCROLL_JUMP,
  CTK_SCROLL_STEP_BACKWARD,
  CTK_SCROLL_STEP_FORWARD,
  CTK_SCROLL_PAGE_BACKWARD,
  CTK_SCROLL_PAGE_FORWARD,
  CTK_SCROLL_STEP_UP,
  CTK_SCROLL_STEP_DOWN,
  CTK_SCROLL_PAGE_UP,
  CTK_SCROLL_PAGE_DOWN,
  CTK_SCROLL_STEP_LEFT,
  CTK_SCROLL_STEP_RIGHT,
  CTK_SCROLL_PAGE_LEFT,
  CTK_SCROLL_PAGE_RIGHT,
  CTK_SCROLL_START,
  CTK_SCROLL_END
} CtkScrollType;

/**
 * CtkSelectionMode:
 * @CTK_SELECTION_NONE: No selection is possible.
 * @CTK_SELECTION_SINGLE: Zero or one element may be selected.
 * @CTK_SELECTION_BROWSE: Exactly one element is selected.
 *     In some circumstances, such as initially or during a search
 *     operation, it’s possible for no element to be selected with
 *     %CTK_SELECTION_BROWSE. What is really enforced is that the user
 *     can’t deselect a currently selected element except by selecting
 *     another element.
 * @CTK_SELECTION_MULTIPLE: Any number of elements may be selected.
 *      The Ctrl key may be used to enlarge the selection, and Shift
 *      key to select between the focus and the child pointed to.
 *      Some widgets may also allow Click-drag to select a range of elements.
 *
 * Used to control what selections users are allowed to make.
 */
typedef enum
{
  CTK_SELECTION_NONE,
  CTK_SELECTION_SINGLE,
  CTK_SELECTION_BROWSE,
  CTK_SELECTION_MULTIPLE
} CtkSelectionMode;

/**
 * CtkShadowType:
 * @CTK_SHADOW_NONE: No outline.
 * @CTK_SHADOW_IN: The outline is bevelled inwards.
 * @CTK_SHADOW_OUT: The outline is bevelled outwards like a button.
 * @CTK_SHADOW_ETCHED_IN: The outline has a sunken 3d appearance.
 * @CTK_SHADOW_ETCHED_OUT: The outline has a raised 3d appearance.
 *
 * Used to change the appearance of an outline typically provided by a #CtkFrame.
 *
 * Note that many themes do not differentiate the appearance of the
 * various shadow types: Either their is no visible shadow (@CTK_SHADOW_NONE),
 * or there is (any other value).
 */
typedef enum
{
  CTK_SHADOW_NONE,
  CTK_SHADOW_IN,
  CTK_SHADOW_OUT,
  CTK_SHADOW_ETCHED_IN,
  CTK_SHADOW_ETCHED_OUT
} CtkShadowType;

/* Widget states */

/**
 * CtkStateType:
 * @CTK_STATE_NORMAL: State during normal operation.
 * @CTK_STATE_ACTIVE: State of a currently active widget, such as a depressed button.
 * @CTK_STATE_PRELIGHT: State indicating that the mouse pointer is over
 *                      the widget and the widget will respond to mouse clicks.
 * @CTK_STATE_SELECTED: State of a selected item, such the selected row in a list.
 * @CTK_STATE_INSENSITIVE: State indicating that the widget is
 *                         unresponsive to user actions.
 * @CTK_STATE_INCONSISTENT: The widget is inconsistent, such as checkbuttons
 *                          or radiobuttons that aren’t either set to %TRUE nor %FALSE,
 *                          or buttons requiring the user attention.
 * @CTK_STATE_FOCUSED: The widget has the keyboard focus.
 *
 * This type indicates the current state of a widget; the state determines how
 * the widget is drawn. The #CtkStateType enumeration is also used to
 * identify different colors in a #CtkStyle for drawing, so states can be
 * used for subparts of a widget as well as entire widgets.
 *
 * Deprecated: 3.14: All APIs that are using this enumeration have been deprecated
 *     in favor of alternatives using #CtkStateFlags.
 */
typedef enum
{
  CTK_STATE_NORMAL,
  CTK_STATE_ACTIVE,
  CTK_STATE_PRELIGHT,
  CTK_STATE_SELECTED,
  CTK_STATE_INSENSITIVE,
  CTK_STATE_INCONSISTENT,
  CTK_STATE_FOCUSED
} CtkStateType;

/**
 * CtkToolbarStyle:
 * @CTK_TOOLBAR_ICONS: Buttons display only icons in the toolbar.
 * @CTK_TOOLBAR_TEXT: Buttons display only text labels in the toolbar.
 * @CTK_TOOLBAR_BOTH: Buttons display text and icons in the toolbar.
 * @CTK_TOOLBAR_BOTH_HORIZ: Buttons display icons and text alongside each
 *  other, rather than vertically stacked
 *
 * Used to customize the appearance of a #CtkToolbar. Note that
 * setting the toolbar style overrides the user’s preferences
 * for the default toolbar style.  Note that if the button has only
 * a label set and CTK_TOOLBAR_ICONS is used, the label will be
 * visible, and vice versa.
 */
typedef enum
{
  CTK_TOOLBAR_ICONS,
  CTK_TOOLBAR_TEXT,
  CTK_TOOLBAR_BOTH,
  CTK_TOOLBAR_BOTH_HORIZ
} CtkToolbarStyle;

/**
 * CtkWrapMode:
 * @CTK_WRAP_NONE: do not wrap lines; just make the text area wider
 * @CTK_WRAP_CHAR: wrap text, breaking lines anywhere the cursor can
 *     appear (between characters, usually - if you want to be technical,
 *     between graphemes, see pango_get_log_attrs())
 * @CTK_WRAP_WORD: wrap text, breaking lines in between words
 * @CTK_WRAP_WORD_CHAR: wrap text, breaking lines in between words, or if
 *     that is not enough, also between graphemes
 *
 * Describes a type of line wrapping.
 */
typedef enum
{
  CTK_WRAP_NONE,
  CTK_WRAP_CHAR,
  CTK_WRAP_WORD,
  CTK_WRAP_WORD_CHAR
} CtkWrapMode;

/**
 * CtkSortType:
 * @CTK_SORT_ASCENDING: Sorting is in ascending order.
 * @CTK_SORT_DESCENDING: Sorting is in descending order.
 *
 * Determines the direction of a sort.
 */
typedef enum
{
  CTK_SORT_ASCENDING,
  CTK_SORT_DESCENDING
} CtkSortType;

/* Style for ctk input method preedit/status */
/**
 * CtkIMPreeditStyle:
 * @CTK_IM_PREEDIT_NOTHING: Deprecated
 * @CTK_IM_PREEDIT_CALLBACK: Deprecated
 * @CTK_IM_PREEDIT_NONE: Deprecated
 *
 * Style for input method preedit. See also
 * #CtkSettings:ctk-im-preedit-style
 *
 * Deprecated: 3.10
 */
typedef enum
{
  CTK_IM_PREEDIT_NOTHING,
  CTK_IM_PREEDIT_CALLBACK,
  CTK_IM_PREEDIT_NONE
} CtkIMPreeditStyle;

/**
 * CtkIMStatusStyle:
 * @CTK_IM_STATUS_NOTHING: Deprecated
 * @CTK_IM_STATUS_CALLBACK: Deprecated
 * @CTK_IM_STATUS_NONE: Deprecated
 *
 * Style for input method status. See also
 * #CtkSettings:ctk-im-status-style
 *
 * Deprecated: 3.10
 */
typedef enum
{
  CTK_IM_STATUS_NOTHING,
  CTK_IM_STATUS_CALLBACK,
  CTK_IM_STATUS_NONE
} CtkIMStatusStyle;

/**
 * CtkPackDirection:
 * @CTK_PACK_DIRECTION_LTR: Widgets are packed left-to-right
 * @CTK_PACK_DIRECTION_RTL: Widgets are packed right-to-left
 * @CTK_PACK_DIRECTION_TTB: Widgets are packed top-to-bottom
 * @CTK_PACK_DIRECTION_BTT: Widgets are packed bottom-to-top
 *
 * Determines how widgets should be packed inside menubars
 * and menuitems contained in menubars.
 */
typedef enum
{
  CTK_PACK_DIRECTION_LTR,
  CTK_PACK_DIRECTION_RTL,
  CTK_PACK_DIRECTION_TTB,
  CTK_PACK_DIRECTION_BTT
} CtkPackDirection;

/**
 * CtkPrintPages:
 * @CTK_PRINT_PAGES_ALL: All pages.
 * @CTK_PRINT_PAGES_CURRENT: Current page.
 * @CTK_PRINT_PAGES_RANGES: Range of pages.
 * @CTK_PRINT_PAGES_SELECTION: Selected pages.
 *
 * See also ctk_print_job_set_pages()
 */
typedef enum
{
  CTK_PRINT_PAGES_ALL,
  CTK_PRINT_PAGES_CURRENT,
  CTK_PRINT_PAGES_RANGES,
  CTK_PRINT_PAGES_SELECTION
} CtkPrintPages;

/**
 * CtkPageSet:
 * @CTK_PAGE_SET_ALL: All pages.
 * @CTK_PAGE_SET_EVEN: Even pages.
 * @CTK_PAGE_SET_ODD: Odd pages.
 *
 * See also ctk_print_job_set_page_set().
 */
typedef enum
{
  CTK_PAGE_SET_ALL,
  CTK_PAGE_SET_EVEN,
  CTK_PAGE_SET_ODD
} CtkPageSet;

/**
 * CtkNumberUpLayout:
 * @CTK_NUMBER_UP_LAYOUT_LEFT_TO_RIGHT_TOP_TO_BOTTOM: ![](layout-lrtb.png)
 * @CTK_NUMBER_UP_LAYOUT_LEFT_TO_RIGHT_BOTTOM_TO_TOP: ![](layout-lrbt.png)
 * @CTK_NUMBER_UP_LAYOUT_RIGHT_TO_LEFT_TOP_TO_BOTTOM: ![](layout-rltb.png)
 * @CTK_NUMBER_UP_LAYOUT_RIGHT_TO_LEFT_BOTTOM_TO_TOP: ![](layout-rlbt.png)
 * @CTK_NUMBER_UP_LAYOUT_TOP_TO_BOTTOM_LEFT_TO_RIGHT: ![](layout-tblr.png)
 * @CTK_NUMBER_UP_LAYOUT_TOP_TO_BOTTOM_RIGHT_TO_LEFT: ![](layout-tbrl.png)
 * @CTK_NUMBER_UP_LAYOUT_BOTTOM_TO_TOP_LEFT_TO_RIGHT: ![](layout-btlr.png)
 * @CTK_NUMBER_UP_LAYOUT_BOTTOM_TO_TOP_RIGHT_TO_LEFT: ![](layout-btrl.png)
 *
 * Used to determine the layout of pages on a sheet when printing
 * multiple pages per sheet.
 */
typedef enum
{
  CTK_NUMBER_UP_LAYOUT_LEFT_TO_RIGHT_TOP_TO_BOTTOM, /*< nick=lrtb >*/
  CTK_NUMBER_UP_LAYOUT_LEFT_TO_RIGHT_BOTTOM_TO_TOP, /*< nick=lrbt >*/
  CTK_NUMBER_UP_LAYOUT_RIGHT_TO_LEFT_TOP_TO_BOTTOM, /*< nick=rltb >*/
  CTK_NUMBER_UP_LAYOUT_RIGHT_TO_LEFT_BOTTOM_TO_TOP, /*< nick=rlbt >*/
  CTK_NUMBER_UP_LAYOUT_TOP_TO_BOTTOM_LEFT_TO_RIGHT, /*< nick=tblr >*/
  CTK_NUMBER_UP_LAYOUT_TOP_TO_BOTTOM_RIGHT_TO_LEFT, /*< nick=tbrl >*/
  CTK_NUMBER_UP_LAYOUT_BOTTOM_TO_TOP_LEFT_TO_RIGHT, /*< nick=btlr >*/
  CTK_NUMBER_UP_LAYOUT_BOTTOM_TO_TOP_RIGHT_TO_LEFT  /*< nick=btrl >*/
} CtkNumberUpLayout;

/**
 * CtkPageOrientation:
 * @CTK_PAGE_ORIENTATION_PORTRAIT: Portrait mode.
 * @CTK_PAGE_ORIENTATION_LANDSCAPE: Landscape mode.
 * @CTK_PAGE_ORIENTATION_REVERSE_PORTRAIT: Reverse portrait mode.
 * @CTK_PAGE_ORIENTATION_REVERSE_LANDSCAPE: Reverse landscape mode.
 *
 * See also ctk_print_settings_set_orientation().
 */
typedef enum
{
  CTK_PAGE_ORIENTATION_PORTRAIT,
  CTK_PAGE_ORIENTATION_LANDSCAPE,
  CTK_PAGE_ORIENTATION_REVERSE_PORTRAIT,
  CTK_PAGE_ORIENTATION_REVERSE_LANDSCAPE
} CtkPageOrientation;

/**
 * CtkPrintQuality:
 * @CTK_PRINT_QUALITY_LOW: Low quality.
 * @CTK_PRINT_QUALITY_NORMAL: Normal quality.
 * @CTK_PRINT_QUALITY_HIGH: High quality.
 * @CTK_PRINT_QUALITY_DRAFT: Draft quality.
 *
 * See also ctk_print_settings_set_quality().
 */
typedef enum
{
  CTK_PRINT_QUALITY_LOW,
  CTK_PRINT_QUALITY_NORMAL,
  CTK_PRINT_QUALITY_HIGH,
  CTK_PRINT_QUALITY_DRAFT
} CtkPrintQuality;

/**
 * CtkPrintDuplex:
 * @CTK_PRINT_DUPLEX_SIMPLEX: No duplex.
 * @CTK_PRINT_DUPLEX_HORIZONTAL: Horizontal duplex.
 * @CTK_PRINT_DUPLEX_VERTICAL: Vertical duplex.
 *
 * See also ctk_print_settings_set_duplex().
 */
typedef enum
{
  CTK_PRINT_DUPLEX_SIMPLEX,
  CTK_PRINT_DUPLEX_HORIZONTAL,
  CTK_PRINT_DUPLEX_VERTICAL
} CtkPrintDuplex;


/**
 * CtkUnit:
 * @CTK_UNIT_NONE: No units.
 * @CTK_UNIT_POINTS: Dimensions in points.
 * @CTK_UNIT_INCH: Dimensions in inches.
 * @CTK_UNIT_MM: Dimensions in millimeters
 *
 * See also ctk_print_settings_set_paper_width().
 */
typedef enum
{
  CTK_UNIT_NONE,
  CTK_UNIT_POINTS,
  CTK_UNIT_INCH,
  CTK_UNIT_MM
} CtkUnit;

#define CTK_UNIT_PIXEL CTK_UNIT_NONE

/**
 * CtkTreeViewGridLines:
 * @CTK_TREE_VIEW_GRID_LINES_NONE: No grid lines.
 * @CTK_TREE_VIEW_GRID_LINES_HORIZONTAL: Horizontal grid lines.
 * @CTK_TREE_VIEW_GRID_LINES_VERTICAL: Vertical grid lines.
 * @CTK_TREE_VIEW_GRID_LINES_BOTH: Horizontal and vertical grid lines.
 *
 * Used to indicate which grid lines to draw in a tree view.
 */
typedef enum
{
  CTK_TREE_VIEW_GRID_LINES_NONE,
  CTK_TREE_VIEW_GRID_LINES_HORIZONTAL,
  CTK_TREE_VIEW_GRID_LINES_VERTICAL,
  CTK_TREE_VIEW_GRID_LINES_BOTH
} CtkTreeViewGridLines;

/**
 * CtkDragResult:
 * @CTK_DRAG_RESULT_SUCCESS: The drag operation was successful.
 * @CTK_DRAG_RESULT_NO_TARGET: No suitable drag target.
 * @CTK_DRAG_RESULT_USER_CANCELLED: The user cancelled the drag operation.
 * @CTK_DRAG_RESULT_TIMEOUT_EXPIRED: The drag operation timed out.
 * @CTK_DRAG_RESULT_GRAB_BROKEN: The pointer or keyboard grab used
 *  for the drag operation was broken.
 * @CTK_DRAG_RESULT_ERROR: The drag operation failed due to some
 *  unspecified error.
 *
 * Gives an indication why a drag operation failed.
 * The value can by obtained by connecting to the
 * #CtkWidget::drag-failed signal.
 */
typedef enum
{
  CTK_DRAG_RESULT_SUCCESS,
  CTK_DRAG_RESULT_NO_TARGET,
  CTK_DRAG_RESULT_USER_CANCELLED,
  CTK_DRAG_RESULT_TIMEOUT_EXPIRED,
  CTK_DRAG_RESULT_GRAB_BROKEN,
  CTK_DRAG_RESULT_ERROR
} CtkDragResult;

/**
 * CtkSizeGroupMode:
 * @CTK_SIZE_GROUP_NONE: group has no effect
 * @CTK_SIZE_GROUP_HORIZONTAL: group affects horizontal requisition
 * @CTK_SIZE_GROUP_VERTICAL: group affects vertical requisition
 * @CTK_SIZE_GROUP_BOTH: group affects both horizontal and vertical requisition
 *
 * The mode of the size group determines the directions in which the size
 * group affects the requested sizes of its component widgets.
 **/
typedef enum {
  CTK_SIZE_GROUP_NONE,
  CTK_SIZE_GROUP_HORIZONTAL,
  CTK_SIZE_GROUP_VERTICAL,
  CTK_SIZE_GROUP_BOTH
} CtkSizeGroupMode;

/**
 * CtkSizeRequestMode:
 * @CTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH: Prefer height-for-width geometry management
 * @CTK_SIZE_REQUEST_WIDTH_FOR_HEIGHT: Prefer width-for-height geometry management
 * @CTK_SIZE_REQUEST_CONSTANT_SIZE: Don’t trade height-for-width or width-for-height
 * 
 * Specifies a preference for height-for-width or
 * width-for-height geometry management.
 */
typedef enum
{
  CTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH = 0,
  CTK_SIZE_REQUEST_WIDTH_FOR_HEIGHT,
  CTK_SIZE_REQUEST_CONSTANT_SIZE
} CtkSizeRequestMode;

/**
 * CtkScrollablePolicy:
 * @CTK_SCROLL_MINIMUM: Scrollable adjustments are based on the minimum size
 * @CTK_SCROLL_NATURAL: Scrollable adjustments are based on the natural size
 *
 * Defines the policy to be used in a scrollable widget when updating
 * the scrolled window adjustments in a given orientation.
 */
typedef enum
{
  CTK_SCROLL_MINIMUM = 0,
  CTK_SCROLL_NATURAL
} CtkScrollablePolicy;

/**
 * CtkStateFlags:
 * @CTK_STATE_FLAG_NORMAL: State during normal operation.
 * @CTK_STATE_FLAG_ACTIVE: Widget is active.
 * @CTK_STATE_FLAG_PRELIGHT: Widget has a mouse pointer over it.
 * @CTK_STATE_FLAG_SELECTED: Widget is selected.
 * @CTK_STATE_FLAG_INSENSITIVE: Widget is insensitive.
 * @CTK_STATE_FLAG_INCONSISTENT: Widget is inconsistent.
 * @CTK_STATE_FLAG_FOCUSED: Widget has the keyboard focus.
 * @CTK_STATE_FLAG_BACKDROP: Widget is in a background toplevel window.
 * @CTK_STATE_FLAG_DIR_LTR: Widget is in left-to-right text direction. Since 3.8
 * @CTK_STATE_FLAG_DIR_RTL: Widget is in right-to-left text direction. Since 3.8
 * @CTK_STATE_FLAG_LINK: Widget is a link. Since 3.12
 * @CTK_STATE_FLAG_VISITED: The location the widget points to has already been visited. Since 3.12
 * @CTK_STATE_FLAG_CHECKED: Widget is checked. Since 3.14
 * @CTK_STATE_FLAG_DROP_ACTIVE: Widget is highlighted as a drop target for DND. Since 3.20
 *
 * Describes a widget state. Widget states are used to match the widget
 * against CSS pseudo-classes. Note that GTK extends the regular CSS
 * classes and sometimes uses different names.
 */
typedef enum
{
  CTK_STATE_FLAG_NORMAL       = 0,
  CTK_STATE_FLAG_ACTIVE       = 1 << 0,
  CTK_STATE_FLAG_PRELIGHT     = 1 << 1,
  CTK_STATE_FLAG_SELECTED     = 1 << 2,
  CTK_STATE_FLAG_INSENSITIVE  = 1 << 3,
  CTK_STATE_FLAG_INCONSISTENT = 1 << 4,
  CTK_STATE_FLAG_FOCUSED      = 1 << 5,
  CTK_STATE_FLAG_BACKDROP     = 1 << 6,
  CTK_STATE_FLAG_DIR_LTR      = 1 << 7,
  CTK_STATE_FLAG_DIR_RTL      = 1 << 8,
  CTK_STATE_FLAG_LINK         = 1 << 9,
  CTK_STATE_FLAG_VISITED      = 1 << 10,
  CTK_STATE_FLAG_CHECKED      = 1 << 11,
  CTK_STATE_FLAG_DROP_ACTIVE  = 1 << 12
} CtkStateFlags;

/**
 * CtkRegionFlags:
 * @CTK_REGION_EVEN: Region has an even number within a set.
 * @CTK_REGION_ODD: Region has an odd number within a set.
 * @CTK_REGION_FIRST: Region is the first one within a set.
 * @CTK_REGION_LAST: Region is the last one within a set.
 * @CTK_REGION_ONLY: Region is the only one within a set.
 * @CTK_REGION_SORTED: Region is part of a sorted area.
 *
 * Describes a region within a widget.
 */
typedef enum {
  CTK_REGION_EVEN    = 1 << 0,
  CTK_REGION_ODD     = 1 << 1,
  CTK_REGION_FIRST   = 1 << 2,
  CTK_REGION_LAST    = 1 << 3,
  CTK_REGION_ONLY    = 1 << 4,
  CTK_REGION_SORTED  = 1 << 5
} CtkRegionFlags;

/**
 * CtkJunctionSides:
 * @CTK_JUNCTION_NONE: No junctions.
 * @CTK_JUNCTION_CORNER_TOPLEFT: Element connects on the top-left corner.
 * @CTK_JUNCTION_CORNER_TOPRIGHT: Element connects on the top-right corner.
 * @CTK_JUNCTION_CORNER_BOTTOMLEFT: Element connects on the bottom-left corner.
 * @CTK_JUNCTION_CORNER_BOTTOMRIGHT: Element connects on the bottom-right corner.
 * @CTK_JUNCTION_TOP: Element connects on the top side.
 * @CTK_JUNCTION_BOTTOM: Element connects on the bottom side.
 * @CTK_JUNCTION_LEFT: Element connects on the left side.
 * @CTK_JUNCTION_RIGHT: Element connects on the right side.
 *
 * Describes how a rendered element connects to adjacent elements.
 */
typedef enum {
  CTK_JUNCTION_NONE   = 0,
  CTK_JUNCTION_CORNER_TOPLEFT = 1 << 0,
  CTK_JUNCTION_CORNER_TOPRIGHT = 1 << 1,
  CTK_JUNCTION_CORNER_BOTTOMLEFT = 1 << 2,
  CTK_JUNCTION_CORNER_BOTTOMRIGHT = 1 << 3,
  CTK_JUNCTION_TOP    = (CTK_JUNCTION_CORNER_TOPLEFT | CTK_JUNCTION_CORNER_TOPRIGHT),
  CTK_JUNCTION_BOTTOM = (CTK_JUNCTION_CORNER_BOTTOMLEFT | CTK_JUNCTION_CORNER_BOTTOMRIGHT),
  CTK_JUNCTION_LEFT   = (CTK_JUNCTION_CORNER_TOPLEFT | CTK_JUNCTION_CORNER_BOTTOMLEFT),
  CTK_JUNCTION_RIGHT  = (CTK_JUNCTION_CORNER_TOPRIGHT | CTK_JUNCTION_CORNER_BOTTOMRIGHT)
} CtkJunctionSides;

/**
 * CtkBorderStyle:
 * @CTK_BORDER_STYLE_NONE: No visible border
 * @CTK_BORDER_STYLE_SOLID: A single line segment
 * @CTK_BORDER_STYLE_INSET: Looks as if the content is sunken into the canvas
 * @CTK_BORDER_STYLE_OUTSET: Looks as if the content is coming out of the canvas
 * @CTK_BORDER_STYLE_HIDDEN: Same as @CTK_BORDER_STYLE_NONE
 * @CTK_BORDER_STYLE_DOTTED: A series of round dots
 * @CTK_BORDER_STYLE_DASHED: A series of square-ended dashes
 * @CTK_BORDER_STYLE_DOUBLE: Two parallel lines with some space between them
 * @CTK_BORDER_STYLE_GROOVE: Looks as if it were carved in the canvas
 * @CTK_BORDER_STYLE_RIDGE: Looks as if it were coming out of the canvas
 *
 * Describes how the border of a UI element should be rendered.
 */
typedef enum {
  CTK_BORDER_STYLE_NONE,
  CTK_BORDER_STYLE_SOLID,
  CTK_BORDER_STYLE_INSET,
  CTK_BORDER_STYLE_OUTSET,
  CTK_BORDER_STYLE_HIDDEN,
  CTK_BORDER_STYLE_DOTTED,
  CTK_BORDER_STYLE_DASHED,
  CTK_BORDER_STYLE_DOUBLE,
  CTK_BORDER_STYLE_GROOVE,
  CTK_BORDER_STYLE_RIDGE
} CtkBorderStyle;

/**
 * CtkLevelBarMode:
 * @CTK_LEVEL_BAR_MODE_CONTINUOUS: the bar has a continuous mode
 * @CTK_LEVEL_BAR_MODE_DISCRETE: the bar has a discrete mode
 *
 * Describes how #CtkLevelBar contents should be rendered.
 * Note that this enumeration could be extended with additional modes
 * in the future.
 *
 * Since: 3.6
 */
typedef enum {
  CTK_LEVEL_BAR_MODE_CONTINUOUS,
  CTK_LEVEL_BAR_MODE_DISCRETE
} CtkLevelBarMode;

G_END_DECLS

/**
 * CtkInputPurpose:
 * @CTK_INPUT_PURPOSE_FREE_FORM: Allow any character
 * @CTK_INPUT_PURPOSE_ALPHA: Allow only alphabetic characters
 * @CTK_INPUT_PURPOSE_DIGITS: Allow only digits
 * @CTK_INPUT_PURPOSE_NUMBER: Edited field expects numbers
 * @CTK_INPUT_PURPOSE_PHONE: Edited field expects phone number
 * @CTK_INPUT_PURPOSE_URL: Edited field expects URL
 * @CTK_INPUT_PURPOSE_EMAIL: Edited field expects email address
 * @CTK_INPUT_PURPOSE_NAME: Edited field expects the name of a person
 * @CTK_INPUT_PURPOSE_PASSWORD: Like @CTK_INPUT_PURPOSE_FREE_FORM, but characters are hidden
 * @CTK_INPUT_PURPOSE_PIN: Like @CTK_INPUT_PURPOSE_DIGITS, but characters are hidden
 * @CTK_INPUT_PURPOSE_TERMINAL: Allow any character, in addition to control codes
 *
 * Describes primary purpose of the input widget. This information is
 * useful for on-screen keyboards and similar input methods to decide
 * which keys should be presented to the user.
 *
 * Note that the purpose is not meant to impose a totally strict rule
 * about allowed characters, and does not replace input validation.
 * It is fine for an on-screen keyboard to let the user override the
 * character set restriction that is expressed by the purpose. The
 * application is expected to validate the entry contents, even if
 * it specified a purpose.
 *
 * The difference between @CTK_INPUT_PURPOSE_DIGITS and
 * @CTK_INPUT_PURPOSE_NUMBER is that the former accepts only digits
 * while the latter also some punctuation (like commas or points, plus,
 * minus) and “e” or “E” as in 3.14E+000.
 *
 * This enumeration may be extended in the future; input methods should
 * interpret unknown values as “free form”.
 *
 * Since: 3.6
 */
typedef enum
{
  CTK_INPUT_PURPOSE_FREE_FORM,
  CTK_INPUT_PURPOSE_ALPHA,
  CTK_INPUT_PURPOSE_DIGITS,
  CTK_INPUT_PURPOSE_NUMBER,
  CTK_INPUT_PURPOSE_PHONE,
  CTK_INPUT_PURPOSE_URL,
  CTK_INPUT_PURPOSE_EMAIL,
  CTK_INPUT_PURPOSE_NAME,
  CTK_INPUT_PURPOSE_PASSWORD,
  CTK_INPUT_PURPOSE_PIN,
  CTK_INPUT_PURPOSE_TERMINAL,
} CtkInputPurpose;

/**
 * CtkInputHints:
 * @CTK_INPUT_HINT_NONE: No special behaviour suggested
 * @CTK_INPUT_HINT_SPELLCHECK: Suggest checking for typos
 * @CTK_INPUT_HINT_NO_SPELLCHECK: Suggest not checking for typos
 * @CTK_INPUT_HINT_WORD_COMPLETION: Suggest word completion
 * @CTK_INPUT_HINT_LOWERCASE: Suggest to convert all text to lowercase
 * @CTK_INPUT_HINT_UPPERCASE_CHARS: Suggest to capitalize all text
 * @CTK_INPUT_HINT_UPPERCASE_WORDS: Suggest to capitalize the first
 *     character of each word
 * @CTK_INPUT_HINT_UPPERCASE_SENTENCES: Suggest to capitalize the
 *     first word of each sentence
 * @CTK_INPUT_HINT_INHIBIT_OSK: Suggest to not show an onscreen keyboard
 *     (e.g for a calculator that already has all the keys).
 * @CTK_INPUT_HINT_VERTICAL_WRITING: The text is vertical. Since 3.18
 * @CTK_INPUT_HINT_EMOJI: Suggest offering Emoji support. Since 3.22.20
 * @CTK_INPUT_HINT_NO_EMOJI: Suggest not offering Emoji support. Since 3.22.20
 *
 * Describes hints that might be taken into account by input methods
 * or applications. Note that input methods may already tailor their
 * behaviour according to the #CtkInputPurpose of the entry.
 *
 * Some common sense is expected when using these flags - mixing
 * @CTK_INPUT_HINT_LOWERCASE with any of the uppercase hints makes no sense.
 *
 * This enumeration may be extended in the future; input methods should
 * ignore unknown values.
 *
 * Since: 3.6
 */
typedef enum
{
  CTK_INPUT_HINT_NONE                = 0,
  CTK_INPUT_HINT_SPELLCHECK          = 1 << 0,
  CTK_INPUT_HINT_NO_SPELLCHECK       = 1 << 1,
  CTK_INPUT_HINT_WORD_COMPLETION     = 1 << 2,
  CTK_INPUT_HINT_LOWERCASE           = 1 << 3,
  CTK_INPUT_HINT_UPPERCASE_CHARS     = 1 << 4,
  CTK_INPUT_HINT_UPPERCASE_WORDS     = 1 << 5,
  CTK_INPUT_HINT_UPPERCASE_SENTENCES = 1 << 6,
  CTK_INPUT_HINT_INHIBIT_OSK         = 1 << 7,
  CTK_INPUT_HINT_VERTICAL_WRITING    = 1 << 8,
  CTK_INPUT_HINT_EMOJI               = 1 << 9,
  CTK_INPUT_HINT_NO_EMOJI            = 1 << 10
} CtkInputHints;

/**
 * CtkPropagationPhase:
 * @CTK_PHASE_NONE: Events are not delivered automatically. Those can be
 *   manually fed through ctk_event_controller_handle_event(). This should
 *   only be used when full control about when, or whether the controller
 *   handles the event is needed.
 * @CTK_PHASE_CAPTURE: Events are delivered in the capture phase. The
 *   capture phase happens before the bubble phase, runs from the toplevel down
 *   to the event widget. This option should only be used on containers that
 *   might possibly handle events before their children do.
 * @CTK_PHASE_BUBBLE: Events are delivered in the bubble phase. The bubble
 *   phase happens after the capture phase, and before the default handlers
 *   are run. This phase runs from the event widget, up to the toplevel.
 * @CTK_PHASE_TARGET: Events are delivered in the default widget event handlers,
 *   note that widget implementations must chain up on button, motion, touch and
 *   grab broken handlers for controllers in this phase to be run.
 *
 * Describes the stage at which events are fed into a #CtkEventController.
 *
 * Since: 3.14
 */
typedef enum
{
  CTK_PHASE_NONE,
  CTK_PHASE_CAPTURE,
  CTK_PHASE_BUBBLE,
  CTK_PHASE_TARGET
} CtkPropagationPhase;

/**
 * CtkEventSequenceState:
 * @CTK_EVENT_SEQUENCE_NONE: The sequence is handled, but not grabbed.
 * @CTK_EVENT_SEQUENCE_CLAIMED: The sequence is handled and grabbed.
 * @CTK_EVENT_SEQUENCE_DENIED: The sequence is denied.
 *
 * Describes the state of a #GdkEventSequence in a #CtkGesture.
 *
 * Since: 3.14
 */
typedef enum
{
  CTK_EVENT_SEQUENCE_NONE,
  CTK_EVENT_SEQUENCE_CLAIMED,
  CTK_EVENT_SEQUENCE_DENIED
} CtkEventSequenceState;

/**
 * CtkPanDirection:
 * @CTK_PAN_DIRECTION_LEFT: panned towards the left
 * @CTK_PAN_DIRECTION_RIGHT: panned towards the right
 * @CTK_PAN_DIRECTION_UP: panned upwards
 * @CTK_PAN_DIRECTION_DOWN: panned downwards
 *
 * Describes the panning direction of a #CtkGesturePan
 *
 * Since: 3.14
 */
typedef enum
{
  CTK_PAN_DIRECTION_LEFT,
  CTK_PAN_DIRECTION_RIGHT,
  CTK_PAN_DIRECTION_UP,
  CTK_PAN_DIRECTION_DOWN
} CtkPanDirection;

/**
 * CtkPopoverConstraint:
 * @CTK_POPOVER_CONSTRAINT_NONE: Don't constrain the popover position
 *   beyond what is imposed by the implementation
 * @CTK_POPOVER_CONSTRAINT_WINDOW: Constrain the popover to the boundaries
 *   of the window that it is attached to
 *
 * Describes constraints to positioning of popovers. More values
 * may be added to this enumeration in the future.
 *
 * Since: 3.20
 */
typedef enum
{
  CTK_POPOVER_CONSTRAINT_NONE,
  CTK_POPOVER_CONSTRAINT_WINDOW
} CtkPopoverConstraint;


#endif /* __CTK_ENUMS_H__ */
