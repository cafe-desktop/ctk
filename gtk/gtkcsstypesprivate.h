/* GTK - The GIMP Toolkit
 * Copyright (C) 2011 Benjamin Otte <otte@gnome.org>
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

#ifndef __CTK_CSS_TYPES_PRIVATE_H__
#define __CTK_CSS_TYPES_PRIVATE_H__

#include <glib-object.h>
#include <gtk/gtkstylecontext.h>

G_BEGIN_DECLS

typedef union _GtkCssMatcher GtkCssMatcher;
typedef struct _GtkCssNode GtkCssNode;
typedef struct _GtkCssNodeDeclaration GtkCssNodeDeclaration;
typedef struct _GtkCssStyle GtkCssStyle;
typedef struct _GtkStyleProviderPrivate GtkStyleProviderPrivate; /* dummy typedef */

#define CTK_CSS_CHANGE_CLASS                          (1ULL <<  0)
#define CTK_CSS_CHANGE_NAME                           (1ULL <<  1)
#define CTK_CSS_CHANGE_ID                             (1ULL <<  2)
#define CTK_CSS_CHANGE_FIRST_CHILD                    (1ULL <<  3)
#define CTK_CSS_CHANGE_LAST_CHILD                     (1ULL <<  4)
#define CTK_CSS_CHANGE_NTH_CHILD                      (1ULL <<  5)
#define CTK_CSS_CHANGE_NTH_LAST_CHILD                 (1ULL <<  6)
#define CTK_CSS_CHANGE_STATE                          (1ULL <<  7)
#define CTK_CSS_CHANGE_SIBLING_CLASS                  (1ULL <<  8)
#define CTK_CSS_CHANGE_SIBLING_NAME                   (1ULL <<  9)
#define CTK_CSS_CHANGE_SIBLING_ID                     (1ULL << 10)
#define CTK_CSS_CHANGE_SIBLING_FIRST_CHILD            (1ULL << 11)
#define CTK_CSS_CHANGE_SIBLING_LAST_CHILD             (1ULL << 12)
#define CTK_CSS_CHANGE_SIBLING_NTH_CHILD              (1ULL << 13)
#define CTK_CSS_CHANGE_SIBLING_NTH_LAST_CHILD         (1ULL << 14)
#define CTK_CSS_CHANGE_SIBLING_STATE                  (1ULL << 15)
#define CTK_CSS_CHANGE_PARENT_CLASS                   (1ULL << 16)
#define CTK_CSS_CHANGE_PARENT_NAME                    (1ULL << 17)
#define CTK_CSS_CHANGE_PARENT_ID                      (1ULL << 18)
#define CTK_CSS_CHANGE_PARENT_FIRST_CHILD             (1ULL << 19)
#define CTK_CSS_CHANGE_PARENT_LAST_CHILD              (1ULL << 20)
#define CTK_CSS_CHANGE_PARENT_NTH_CHILD               (1ULL << 21)
#define CTK_CSS_CHANGE_PARENT_NTH_LAST_CHILD          (1ULL << 22)
#define CTK_CSS_CHANGE_PARENT_STATE                   (1ULL << 23)
#define CTK_CSS_CHANGE_PARENT_SIBLING_CLASS           (1ULL << 24)
#define CTK_CSS_CHANGE_PARENT_SIBLING_ID              (1ULL << 25)
#define CTK_CSS_CHANGE_PARENT_SIBLING_NAME            (1ULL << 26)
#define CTK_CSS_CHANGE_PARENT_SIBLING_FIRST_CHILD     (1ULL << 27)
#define CTK_CSS_CHANGE_PARENT_SIBLING_LAST_CHILD      (1ULL << 28)
#define CTK_CSS_CHANGE_PARENT_SIBLING_NTH_CHILD       (1ULL << 29)
#define CTK_CSS_CHANGE_PARENT_SIBLING_NTH_LAST_CHILD  (1ULL << 30)
#define CTK_CSS_CHANGE_PARENT_SIBLING_STATE           (1ULL << 31)

/* add more */
#define CTK_CSS_CHANGE_SOURCE                         (1ULL << 32)
#define CTK_CSS_CHANGE_PARENT_STYLE                   (1ULL << 33)
#define CTK_CSS_CHANGE_TIMESTAMP                      (1ULL << 34)
#define CTK_CSS_CHANGE_ANIMATIONS                     (1ULL << 35)

#define CTK_CSS_CHANGE_RESERVED_BIT                   (1ULL << 62) /* Used internally in gtkcssselector.c */

typedef guint64 GtkCssChange;

#define CTK_CSS_CHANGE_POSITION (CTK_CSS_CHANGE_FIRST_CHILD | CTK_CSS_CHANGE_LAST_CHILD | \
                                 CTK_CSS_CHANGE_NTH_CHILD | CTK_CSS_CHANGE_NTH_LAST_CHILD)
#define CTK_CSS_CHANGE_SIBLING_POSITION (CTK_CSS_CHANGE_SIBLING_FIRST_CHILD | CTK_CSS_CHANGE_SIBLING_LAST_CHILD | \
                                         CTK_CSS_CHANGE_SIBLING_NTH_CHILD | CTK_CSS_CHANGE_SIBLING_NTH_LAST_CHILD)
#define CTK_CSS_CHANGE_PARENT_POSITION (CTK_CSS_CHANGE_PARENT_FIRST_CHILD | CTK_CSS_CHANGE_PARENT_LAST_CHILD | \
                                        CTK_CSS_CHANGE_PARENT_NTH_CHILD | CTK_CSS_CHANGE_PARENT_NTH_LAST_CHILD)
#define CTK_CSS_CHANGE_PARENT_SIBLING_POSITION (CTK_CSS_CHANGE_PARENT_SIBLING_FIRST_CHILD | CTK_CSS_CHANGE_PARENT_SIBLING_LAST_CHILD | \
                                                CTK_CSS_CHANGE_PARENT_SIBLING_NTH_CHILD | CTK_CSS_CHANGE_PARENT_SIBLING_NTH_LAST_CHILD)


#define CTK_CSS_CHANGE_ANY ((1 << 19) - 1)
#define CTK_CSS_CHANGE_ANY_SELF (CTK_CSS_CHANGE_CLASS | CTK_CSS_CHANGE_NAME | CTK_CSS_CHANGE_ID | CTK_CSS_CHANGE_POSITION | CTK_CSS_CHANGE_STATE)
#define CTK_CSS_CHANGE_ANY_SIBLING (CTK_CSS_CHANGE_SIBLING_CLASS | CTK_CSS_CHANGE_SIBLING_NAME | \
                                    CTK_CSS_CHANGE_SIBLING_ID | \
                                    CTK_CSS_CHANGE_SIBLING_POSITION | CTK_CSS_CHANGE_SIBLING_STATE)
#define CTK_CSS_CHANGE_ANY_PARENT (CTK_CSS_CHANGE_PARENT_CLASS | CTK_CSS_CHANGE_PARENT_SIBLING_CLASS | \
                                   CTK_CSS_CHANGE_PARENT_NAME | CTK_CSS_CHANGE_PARENT_SIBLING_NAME | \
                                   CTK_CSS_CHANGE_PARENT_ID | CTK_CSS_CHANGE_PARENT_SIBLING_ID | \
                                   CTK_CSS_CHANGE_PARENT_POSITION | CTK_CSS_CHANGE_PARENT_SIBLING_POSITION | \
                                   CTK_CSS_CHANGE_PARENT_STATE | CTK_CSS_CHANGE_PARENT_SIBLING_STATE)

/*
 * GtkCssAffects:
 * @CTK_CSS_AFFECTS_FOREGROUND: The foreground rendering is affected.
 *   This does not include things that affect the font. For those,
 *   see @CTK_CSS_AFFECTS_FONT.
 * @CTK_CSS_AFFECTS_BACKGROUND: The background rendering is affected.
 * @CTK_CSS_AFFECTS_BORDER: The border styling is affected.
 * @CTK_CSS_AFFECTS_PANGO_LAYOUT: Font rendering is affected.
 * @CTK_CSS_AFFECTS_FONT: The font is affected and should be reloaded
 *   if it was cached.
 * @CTK_CSS_AFFECTS_TEXT: Text rendering is affected.
 * @CTK_CSS_AFFECTS_TEXT_ATTRS: Text attributes are affected.
 * @CTK_CSS_AFFECTS_ICON: Fullcolor icons and their rendering is affected.
 * @CTK_CSS_AFFECTS_SYMBOLIC_ICON: Symbolic icons and their rendering is affected.
 * @CTK_CSS_AFFECTS_OUTLINE: The outline styling is affected. Outlines
 *   only affect elements that can be focused.
 * @CTK_CSS_AFFECTS_CLIP: Changes in this property may have an effect
 *   on the clipping area of the element. Changes in these properties
 *   should cause a reevaluation of the element's clip area.
 * @CTK_CSS_AFFECTS_SIZE: Changes in this property may have an effect
 *   on the allocated size of the element. Changes in these properties
 *   should cause a recomputation of the element's allocated size.
 *
 * The generic effects that a CSS property can have. If a value is
 * set, then the property will have an influence on that feature.
 *
 * Note that multiple values can be set.
 */
typedef enum {
  CTK_CSS_AFFECTS_FOREGROUND = (1 << 0),
  CTK_CSS_AFFECTS_BACKGROUND = (1 << 1),
  CTK_CSS_AFFECTS_BORDER = (1 << 2),
  CTK_CSS_AFFECTS_FONT = (1 << 3),
  CTK_CSS_AFFECTS_TEXT = (1 << 4),
  CTK_CSS_AFFECTS_TEXT_ATTRS = (1 << 5),
  CTK_CSS_AFFECTS_ICON = (1 << 6),
  CTK_CSS_AFFECTS_SYMBOLIC_ICON = (1 << 7),
  CTK_CSS_AFFECTS_OUTLINE = (1 << 8),
  CTK_CSS_AFFECTS_CLIP = (1 << 9),
  CTK_CSS_AFFECTS_SIZE = (1 << 10)
} GtkCssAffects;

#define CTK_CSS_AFFECTS_REDRAW (CTK_CSS_AFFECTS_FOREGROUND |    \
                                CTK_CSS_AFFECTS_BACKGROUND |    \
                                CTK_CSS_AFFECTS_BORDER |        \
                                CTK_CSS_AFFECTS_ICON |          \
                                CTK_CSS_AFFECTS_SYMBOLIC_ICON | \
                                CTK_CSS_AFFECTS_OUTLINE)

enum { /*< skip >*/
  CTK_CSS_PROPERTY_COLOR,
  CTK_CSS_PROPERTY_DPI,
  CTK_CSS_PROPERTY_FONT_SIZE,
  CTK_CSS_PROPERTY_ICON_THEME,
  CTK_CSS_PROPERTY_ICON_PALETTE,
  CTK_CSS_PROPERTY_BACKGROUND_COLOR,
  CTK_CSS_PROPERTY_FONT_FAMILY,
  CTK_CSS_PROPERTY_FONT_STYLE,
  CTK_CSS_PROPERTY_FONT_VARIANT,
  CTK_CSS_PROPERTY_FONT_WEIGHT,
  CTK_CSS_PROPERTY_FONT_STRETCH,
  CTK_CSS_PROPERTY_LETTER_SPACING,
  CTK_CSS_PROPERTY_TEXT_DECORATION_LINE,
  CTK_CSS_PROPERTY_TEXT_DECORATION_COLOR,
  CTK_CSS_PROPERTY_TEXT_DECORATION_STYLE,
  CTK_CSS_PROPERTY_TEXT_SHADOW,
  CTK_CSS_PROPERTY_BOX_SHADOW,
  CTK_CSS_PROPERTY_MARGIN_TOP,
  CTK_CSS_PROPERTY_MARGIN_LEFT,
  CTK_CSS_PROPERTY_MARGIN_BOTTOM,
  CTK_CSS_PROPERTY_MARGIN_RIGHT,
  CTK_CSS_PROPERTY_PADDING_TOP,
  CTK_CSS_PROPERTY_PADDING_LEFT,
  CTK_CSS_PROPERTY_PADDING_BOTTOM,
  CTK_CSS_PROPERTY_PADDING_RIGHT,
  CTK_CSS_PROPERTY_BORDER_TOP_STYLE,
  CTK_CSS_PROPERTY_BORDER_TOP_WIDTH,
  CTK_CSS_PROPERTY_BORDER_LEFT_STYLE,
  CTK_CSS_PROPERTY_BORDER_LEFT_WIDTH,
  CTK_CSS_PROPERTY_BORDER_BOTTOM_STYLE,
  CTK_CSS_PROPERTY_BORDER_BOTTOM_WIDTH,
  CTK_CSS_PROPERTY_BORDER_RIGHT_STYLE,
  CTK_CSS_PROPERTY_BORDER_RIGHT_WIDTH,
  CTK_CSS_PROPERTY_BORDER_TOP_LEFT_RADIUS,
  CTK_CSS_PROPERTY_BORDER_TOP_RIGHT_RADIUS,
  CTK_CSS_PROPERTY_BORDER_BOTTOM_RIGHT_RADIUS,
  CTK_CSS_PROPERTY_BORDER_BOTTOM_LEFT_RADIUS,
  CTK_CSS_PROPERTY_OUTLINE_STYLE,
  CTK_CSS_PROPERTY_OUTLINE_WIDTH,
  CTK_CSS_PROPERTY_OUTLINE_OFFSET,
  CTK_CSS_PROPERTY_OUTLINE_TOP_LEFT_RADIUS,
  CTK_CSS_PROPERTY_OUTLINE_TOP_RIGHT_RADIUS,
  CTK_CSS_PROPERTY_OUTLINE_BOTTOM_RIGHT_RADIUS,
  CTK_CSS_PROPERTY_OUTLINE_BOTTOM_LEFT_RADIUS,
  CTK_CSS_PROPERTY_BACKGROUND_CLIP,
  CTK_CSS_PROPERTY_BACKGROUND_ORIGIN,
  CTK_CSS_PROPERTY_BACKGROUND_SIZE,
  CTK_CSS_PROPERTY_BACKGROUND_POSITION,
  CTK_CSS_PROPERTY_BORDER_TOP_COLOR,
  CTK_CSS_PROPERTY_BORDER_RIGHT_COLOR,
  CTK_CSS_PROPERTY_BORDER_BOTTOM_COLOR,
  CTK_CSS_PROPERTY_BORDER_LEFT_COLOR,
  CTK_CSS_PROPERTY_OUTLINE_COLOR,
  CTK_CSS_PROPERTY_BACKGROUND_REPEAT,
  CTK_CSS_PROPERTY_BACKGROUND_IMAGE,
  CTK_CSS_PROPERTY_BACKGROUND_BLEND_MODE,
  CTK_CSS_PROPERTY_BORDER_IMAGE_SOURCE,
  CTK_CSS_PROPERTY_BORDER_IMAGE_REPEAT,
  CTK_CSS_PROPERTY_BORDER_IMAGE_SLICE,
  CTK_CSS_PROPERTY_BORDER_IMAGE_WIDTH,
  CTK_CSS_PROPERTY_ICON_SOURCE,
  CTK_CSS_PROPERTY_ICON_SHADOW,
  CTK_CSS_PROPERTY_ICON_STYLE,
  CTK_CSS_PROPERTY_ICON_TRANSFORM,
  CTK_CSS_PROPERTY_MIN_WIDTH,
  CTK_CSS_PROPERTY_MIN_HEIGHT,
  CTK_CSS_PROPERTY_TRANSITION_PROPERTY,
  CTK_CSS_PROPERTY_TRANSITION_DURATION,
  CTK_CSS_PROPERTY_TRANSITION_TIMING_FUNCTION,
  CTK_CSS_PROPERTY_TRANSITION_DELAY,
  CTK_CSS_PROPERTY_ANIMATION_NAME,
  CTK_CSS_PROPERTY_ANIMATION_DURATION,
  CTK_CSS_PROPERTY_ANIMATION_TIMING_FUNCTION,
  CTK_CSS_PROPERTY_ANIMATION_ITERATION_COUNT,
  CTK_CSS_PROPERTY_ANIMATION_DIRECTION,
  CTK_CSS_PROPERTY_ANIMATION_PLAY_STATE,
  CTK_CSS_PROPERTY_ANIMATION_DELAY,
  CTK_CSS_PROPERTY_ANIMATION_FILL_MODE,
  CTK_CSS_PROPERTY_OPACITY,
  CTK_CSS_PROPERTY_ICON_EFFECT,
  CTK_CSS_PROPERTY_ENGINE,
  CTK_CSS_PROPERTY_CTK_KEY_BINDINGS,
  CTK_CSS_PROPERTY_CARET_COLOR,
  CTK_CSS_PROPERTY_SECONDARY_CARET_COLOR,
  CTK_CSS_PROPERTY_FONT_FEATURE_SETTINGS,
  /* add more */
  CTK_CSS_PROPERTY_N_PROPERTIES
};

typedef enum /*< skip >*/ {
  CTK_CSS_BLEND_MODE_COLOR,
  CTK_CSS_BLEND_MODE_COLOR_BURN,
  CTK_CSS_BLEND_MODE_COLOR_DODGE,
  CTK_CSS_BLEND_MODE_DARKEN,
  CTK_CSS_BLEND_MODE_DIFFERENCE,
  CTK_CSS_BLEND_MODE_EXCLUSION,
  CTK_CSS_BLEND_MODE_HARD_LIGHT,
  CTK_CSS_BLEND_MODE_HUE,
  CTK_CSS_BLEND_MODE_LIGHTEN,
  CTK_CSS_BLEND_MODE_LUMINOSITY,
  CTK_CSS_BLEND_MODE_MULTIPLY,
  CTK_CSS_BLEND_MODE_NORMAL,
  CTK_CSS_BLEND_MODE_OVERLAY,
  CTK_CSS_BLEND_MODE_SATURATE,
  CTK_CSS_BLEND_MODE_SCREEN,
  CTK_CSS_BLEND_MODE_SOFT_LIGHT
} GtkCssBlendMode;

typedef enum /*< skip >*/ {
  CTK_CSS_IMAGE_BUILTIN_NONE,
  CTK_CSS_IMAGE_BUILTIN_CHECK,
  CTK_CSS_IMAGE_BUILTIN_CHECK_INCONSISTENT,
  CTK_CSS_IMAGE_BUILTIN_OPTION,
  CTK_CSS_IMAGE_BUILTIN_OPTION_INCONSISTENT,
  CTK_CSS_IMAGE_BUILTIN_ARROW_UP,
  CTK_CSS_IMAGE_BUILTIN_ARROW_DOWN,
  CTK_CSS_IMAGE_BUILTIN_ARROW_LEFT,
  CTK_CSS_IMAGE_BUILTIN_ARROW_RIGHT,
  CTK_CSS_IMAGE_BUILTIN_EXPANDER_HORIZONTAL_LEFT,
  CTK_CSS_IMAGE_BUILTIN_EXPANDER_VERTICAL_LEFT,
  CTK_CSS_IMAGE_BUILTIN_EXPANDER_HORIZONTAL_RIGHT,
  CTK_CSS_IMAGE_BUILTIN_EXPANDER_VERTICAL_RIGHT,
  CTK_CSS_IMAGE_BUILTIN_EXPANDER_HORIZONTAL_LEFT_EXPANDED,
  CTK_CSS_IMAGE_BUILTIN_EXPANDER_VERTICAL_LEFT_EXPANDED,
  CTK_CSS_IMAGE_BUILTIN_EXPANDER_HORIZONTAL_RIGHT_EXPANDED,
  CTK_CSS_IMAGE_BUILTIN_EXPANDER_VERTICAL_RIGHT_EXPANDED,
  CTK_CSS_IMAGE_BUILTIN_GRIP_TOPLEFT,
  CTK_CSS_IMAGE_BUILTIN_GRIP_TOP,
  CTK_CSS_IMAGE_BUILTIN_GRIP_TOPRIGHT,
  CTK_CSS_IMAGE_BUILTIN_GRIP_RIGHT,
  CTK_CSS_IMAGE_BUILTIN_GRIP_BOTTOMRIGHT,
  CTK_CSS_IMAGE_BUILTIN_GRIP_BOTTOM,
  CTK_CSS_IMAGE_BUILTIN_GRIP_BOTTOMLEFT,
  CTK_CSS_IMAGE_BUILTIN_GRIP_LEFT,
  CTK_CSS_IMAGE_BUILTIN_PANE_SEPARATOR,
  CTK_CSS_IMAGE_BUILTIN_HANDLE,
  CTK_CSS_IMAGE_BUILTIN_SPINNER
} GtkCssImageBuiltinType;

typedef enum /*< skip >*/ {
  CTK_CSS_AREA_BORDER_BOX,
  CTK_CSS_AREA_PADDING_BOX,
  CTK_CSS_AREA_CONTENT_BOX
} GtkCssArea;

typedef enum /*< skip >*/ {
  CTK_CSS_DIRECTION_NORMAL,
  CTK_CSS_DIRECTION_REVERSE,
  CTK_CSS_DIRECTION_ALTERNATE,
  CTK_CSS_DIRECTION_ALTERNATE_REVERSE
} GtkCssDirection;

typedef enum /*< skip >*/ {
  CTK_CSS_PLAY_STATE_RUNNING,
  CTK_CSS_PLAY_STATE_PAUSED
} GtkCssPlayState;

typedef enum /*< skip >*/ {
  CTK_CSS_FILL_NONE,
  CTK_CSS_FILL_FORWARDS,
  CTK_CSS_FILL_BACKWARDS,
  CTK_CSS_FILL_BOTH
} GtkCssFillMode;

typedef enum /*< skip >*/ {
  CTK_CSS_ICON_EFFECT_NONE,
  CTK_CSS_ICON_EFFECT_HIGHLIGHT,
  CTK_CSS_ICON_EFFECT_DIM
} GtkCssIconEffect;

typedef enum /*< skip >*/ {
  CTK_CSS_ICON_STYLE_REQUESTED,
  CTK_CSS_ICON_STYLE_REGULAR,
  CTK_CSS_ICON_STYLE_SYMBOLIC
} GtkCssIconStyle;

typedef enum /*< skip >*/ {
  /* relative font sizes */
  CTK_CSS_FONT_SIZE_SMALLER,
  CTK_CSS_FONT_SIZE_LARGER,
  /* absolute font sizes */
  CTK_CSS_FONT_SIZE_XX_SMALL,
  CTK_CSS_FONT_SIZE_X_SMALL,
  CTK_CSS_FONT_SIZE_SMALL,
  CTK_CSS_FONT_SIZE_MEDIUM,
  CTK_CSS_FONT_SIZE_LARGE,
  CTK_CSS_FONT_SIZE_X_LARGE,
  CTK_CSS_FONT_SIZE_XX_LARGE
} GtkCssFontSize;

typedef enum /*< skip >*/ {
  CTK_CSS_TEXT_DECORATION_LINE_NONE,
  CTK_CSS_TEXT_DECORATION_LINE_UNDERLINE,
  CTK_CSS_TEXT_DECORATION_LINE_LINE_THROUGH
} GtkTextDecorationLine;

typedef enum /*< skip >*/ {
  CTK_CSS_TEXT_DECORATION_STYLE_SOLID,
  CTK_CSS_TEXT_DECORATION_STYLE_DOUBLE,
  CTK_CSS_TEXT_DECORATION_STYLE_WAVY
} GtkTextDecorationStyle;

/* for the order in arrays */
typedef enum /*< skip >*/ {
  CTK_CSS_TOP,
  CTK_CSS_RIGHT,
  CTK_CSS_BOTTOM,
  CTK_CSS_LEFT
} GtkCssSide;

typedef enum /*< skip >*/ {
  CTK_CSS_TOP_LEFT,
  CTK_CSS_TOP_RIGHT,
  CTK_CSS_BOTTOM_RIGHT,
  CTK_CSS_BOTTOM_LEFT
} GtkCssCorner;

typedef enum /*< skip >*/ {
  CTK_CSS_DIMENSION_PERCENTAGE,
  CTK_CSS_DIMENSION_NUMBER,
  CTK_CSS_DIMENSION_LENGTH,
  CTK_CSS_DIMENSION_ANGLE,
  CTK_CSS_DIMENSION_TIME
} GtkCssDimension;

typedef enum /*< skip >*/ {
  /* CSS term: <number> */
  CTK_CSS_NUMBER,
  /* CSS term: <percentage> */
  CTK_CSS_PERCENT,
  /* CSS term: <length> */
  CTK_CSS_PX,
  CTK_CSS_PT,
  CTK_CSS_EM,
  CTK_CSS_EX,
  CTK_CSS_REM,
  CTK_CSS_PC,
  CTK_CSS_IN,
  CTK_CSS_CM,
  CTK_CSS_MM,
  /* CSS term: <angle> */
  CTK_CSS_RAD,
  CTK_CSS_DEG,
  CTK_CSS_GRAD,
  CTK_CSS_TURN,
  /* CSS term: <time> */
  CTK_CSS_S,
  CTK_CSS_MS,
} GtkCssUnit;

cairo_operator_t        _ctk_css_blend_mode_get_operator         (GtkCssBlendMode    mode);

GtkCssChange            _ctk_css_change_for_sibling              (GtkCssChange       match);
GtkCssChange            _ctk_css_change_for_child                (GtkCssChange       match);

GtkCssDimension         ctk_css_unit_get_dimension               (GtkCssUnit         unit);

char *                  ctk_css_change_to_string                 (GtkCssChange       change);
void                    ctk_css_change_print                     (GtkCssChange       change,
                                                                  GString           *string);

/* for lack of better place to put it */
/* mirror what cairo does */
#define ctk_rgba_is_clear(rgba) ((rgba)->alpha < ((double)0x00ff / (double)0xffff))

G_END_DECLS

#endif /* __CTK_CSS_TYPES_PRIVATE_H__ */
