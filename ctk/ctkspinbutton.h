/* CTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * CtkSpinButton widget for CTK+
 * Copyright (C) 1998 Lars Hamann and Stefan Jeske
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
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

#ifndef __CTK_SPIN_BUTTON_H__
#define __CTK_SPIN_BUTTON_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkentry.h>


G_BEGIN_DECLS

#define CTK_TYPE_SPIN_BUTTON                  (ctk_spin_button_get_type ())
#define CTK_SPIN_BUTTON(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_SPIN_BUTTON, CtkSpinButton))
#define CTK_SPIN_BUTTON_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_SPIN_BUTTON, CtkSpinButtonClass))
#define CTK_IS_SPIN_BUTTON(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_SPIN_BUTTON))
#define CTK_IS_SPIN_BUTTON_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_SPIN_BUTTON))
#define CTK_SPIN_BUTTON_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_SPIN_BUTTON, CtkSpinButtonClass))

/**
 * CTK_INPUT_ERROR:
 *
 * Constant to return from a signal handler for the #CtkSpinButton::input
 * signal in case of conversion failure.
 */
#define CTK_INPUT_ERROR -1

/**
 * CtkSpinButtonUpdatePolicy:
 * @CTK_UPDATE_ALWAYS: When refreshing your #CtkSpinButton, the value is
 *     always displayed
 * @CTK_UPDATE_IF_VALID: When refreshing your #CtkSpinButton, the value is
 *     only displayed if it is valid within the bounds of the spin button's
 *     adjustment
 *
 * The spin button update policy determines whether the spin button displays
 * values even if they are outside the bounds of its adjustment.
 * See ctk_spin_button_set_update_policy().
 */
typedef enum
{
  CTK_UPDATE_ALWAYS,
  CTK_UPDATE_IF_VALID
} CtkSpinButtonUpdatePolicy;

/**
 * CtkSpinType:
 * @CTK_SPIN_STEP_FORWARD: Increment by the adjustments step increment.
 * @CTK_SPIN_STEP_BACKWARD: Decrement by the adjustments step increment.
 * @CTK_SPIN_PAGE_FORWARD: Increment by the adjustments page increment.
 * @CTK_SPIN_PAGE_BACKWARD: Decrement by the adjustments page increment.
 * @CTK_SPIN_HOME: Go to the adjustments lower bound.
 * @CTK_SPIN_END: Go to the adjustments upper bound.
 * @CTK_SPIN_USER_DEFINED: Change by a specified amount.
 *
 * The values of the CtkSpinType enumeration are used to specify the
 * change to make in ctk_spin_button_spin().
 */
typedef enum
{
  CTK_SPIN_STEP_FORWARD,
  CTK_SPIN_STEP_BACKWARD,
  CTK_SPIN_PAGE_FORWARD,
  CTK_SPIN_PAGE_BACKWARD,
  CTK_SPIN_HOME,
  CTK_SPIN_END,
  CTK_SPIN_USER_DEFINED
} CtkSpinType;


typedef struct _CtkSpinButton              CtkSpinButton;
typedef struct _CtkSpinButtonPrivate       CtkSpinButtonPrivate;
typedef struct _CtkSpinButtonClass         CtkSpinButtonClass;

/**
 * CtkSpinButton:
 *
 * The #CtkSpinButton-struct contains only private data and should
 * not be directly modified.
 */
struct _CtkSpinButton
{
  CtkEntry entry;

  /*< private >*/
  CtkSpinButtonPrivate *priv;
};

struct _CtkSpinButtonClass
{
  CtkEntryClass parent_class;

  gint (*input)  (CtkSpinButton *spin_button,
		  gdouble       *new_value);
  gint (*output) (CtkSpinButton *spin_button);
  void (*value_changed) (CtkSpinButton *spin_button);

  /* Action signals for keybindings, do not connect to these */
  void (*change_value) (CtkSpinButton *spin_button,
			CtkScrollType  scroll);

  void (*wrapped) (CtkSpinButton *spin_button);

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};


GDK_AVAILABLE_IN_ALL
GType		ctk_spin_button_get_type	   (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
void		ctk_spin_button_configure	   (CtkSpinButton  *spin_button,
						    CtkAdjustment  *adjustment,
						    gdouble	    climb_rate,
						    guint	    digits);

GDK_AVAILABLE_IN_ALL
CtkWidget*	ctk_spin_button_new		   (CtkAdjustment  *adjustment,
						    gdouble	    climb_rate,
						    guint	    digits);

GDK_AVAILABLE_IN_ALL
CtkWidget*	ctk_spin_button_new_with_range	   (gdouble  min,
						    gdouble  max,
						    gdouble  step);

GDK_AVAILABLE_IN_ALL
void		ctk_spin_button_set_adjustment	   (CtkSpinButton  *spin_button,
						    CtkAdjustment  *adjustment);

GDK_AVAILABLE_IN_ALL
CtkAdjustment*	ctk_spin_button_get_adjustment	   (CtkSpinButton  *spin_button);

GDK_AVAILABLE_IN_ALL
void		ctk_spin_button_set_digits	   (CtkSpinButton  *spin_button,
						    guint	    digits);
GDK_AVAILABLE_IN_ALL
guint           ctk_spin_button_get_digits         (CtkSpinButton  *spin_button);

GDK_AVAILABLE_IN_ALL
void		ctk_spin_button_set_increments	   (CtkSpinButton  *spin_button,
						    gdouble         step,
						    gdouble         page);
GDK_AVAILABLE_IN_ALL
void            ctk_spin_button_get_increments     (CtkSpinButton  *spin_button,
						    gdouble        *step,
						    gdouble        *page);

GDK_AVAILABLE_IN_ALL
void		ctk_spin_button_set_range	   (CtkSpinButton  *spin_button,
						    gdouble         min,
						    gdouble         max);
GDK_AVAILABLE_IN_ALL
void            ctk_spin_button_get_range          (CtkSpinButton  *spin_button,
						    gdouble        *min,
						    gdouble        *max);

GDK_AVAILABLE_IN_ALL
gdouble		ctk_spin_button_get_value          (CtkSpinButton  *spin_button);

GDK_AVAILABLE_IN_ALL
gint		ctk_spin_button_get_value_as_int   (CtkSpinButton  *spin_button);

GDK_AVAILABLE_IN_ALL
void		ctk_spin_button_set_value	   (CtkSpinButton  *spin_button,
						    gdouble	    value);

GDK_AVAILABLE_IN_ALL
void		ctk_spin_button_set_update_policy  (CtkSpinButton  *spin_button,
						    CtkSpinButtonUpdatePolicy  policy);
GDK_AVAILABLE_IN_ALL
CtkSpinButtonUpdatePolicy ctk_spin_button_get_update_policy (CtkSpinButton *spin_button);

GDK_AVAILABLE_IN_ALL
void		ctk_spin_button_set_numeric	   (CtkSpinButton  *spin_button,
						    gboolean	    numeric);
GDK_AVAILABLE_IN_ALL
gboolean        ctk_spin_button_get_numeric        (CtkSpinButton  *spin_button);

GDK_AVAILABLE_IN_ALL
void		ctk_spin_button_spin		   (CtkSpinButton  *spin_button,
						    CtkSpinType     direction,
						    gdouble	    increment);

GDK_AVAILABLE_IN_ALL
void		ctk_spin_button_set_wrap	   (CtkSpinButton  *spin_button,
						    gboolean	    wrap);
GDK_AVAILABLE_IN_ALL
gboolean        ctk_spin_button_get_wrap           (CtkSpinButton  *spin_button);

GDK_AVAILABLE_IN_ALL
void		ctk_spin_button_set_snap_to_ticks  (CtkSpinButton  *spin_button,
						    gboolean	    snap_to_ticks);
GDK_AVAILABLE_IN_ALL
gboolean        ctk_spin_button_get_snap_to_ticks  (CtkSpinButton  *spin_button);
GDK_AVAILABLE_IN_ALL
void            ctk_spin_button_update             (CtkSpinButton  *spin_button);

/* private */
void            _ctk_spin_button_get_panels        (CtkSpinButton  *spin_button,
                                                    GdkWindow     **down_panel,
                                                    GdkWindow     **up_panel);

G_END_DECLS

#endif /* __CTK_SPIN_BUTTON_H__ */
