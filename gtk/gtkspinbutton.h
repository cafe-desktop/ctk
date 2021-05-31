/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * GtkSpinButton widget for GTK+
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
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 */

#ifndef __CTK_SPIN_BUTTON_H__
#define __CTK_SPIN_BUTTON_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <gtk/gtk.h> can be included directly."
#endif

#include <gtk/gtkentry.h>


G_BEGIN_DECLS

#define CTK_TYPE_SPIN_BUTTON                  (ctk_spin_button_get_type ())
#define CTK_SPIN_BUTTON(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_SPIN_BUTTON, GtkSpinButton))
#define CTK_SPIN_BUTTON_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_SPIN_BUTTON, GtkSpinButtonClass))
#define CTK_IS_SPIN_BUTTON(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_SPIN_BUTTON))
#define CTK_IS_SPIN_BUTTON_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_SPIN_BUTTON))
#define CTK_SPIN_BUTTON_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_SPIN_BUTTON, GtkSpinButtonClass))

/**
 * CTK_INPUT_ERROR:
 *
 * Constant to return from a signal handler for the #GtkSpinButton::input
 * signal in case of conversion failure.
 */
#define CTK_INPUT_ERROR -1

/**
 * GtkSpinButtonUpdatePolicy:
 * @CTK_UPDATE_ALWAYS: When refreshing your #GtkSpinButton, the value is
 *     always displayed
 * @CTK_UPDATE_IF_VALID: When refreshing your #GtkSpinButton, the value is
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
} GtkSpinButtonUpdatePolicy;

/**
 * GtkSpinType:
 * @CTK_SPIN_STEP_FORWARD: Increment by the adjustments step increment.
 * @CTK_SPIN_STEP_BACKWARD: Decrement by the adjustments step increment.
 * @CTK_SPIN_PAGE_FORWARD: Increment by the adjustments page increment.
 * @CTK_SPIN_PAGE_BACKWARD: Decrement by the adjustments page increment.
 * @CTK_SPIN_HOME: Go to the adjustments lower bound.
 * @CTK_SPIN_END: Go to the adjustments upper bound.
 * @CTK_SPIN_USER_DEFINED: Change by a specified amount.
 *
 * The values of the GtkSpinType enumeration are used to specify the
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
} GtkSpinType;


typedef struct _GtkSpinButton              GtkSpinButton;
typedef struct _GtkSpinButtonPrivate       GtkSpinButtonPrivate;
typedef struct _GtkSpinButtonClass         GtkSpinButtonClass;

/**
 * GtkSpinButton:
 *
 * The #GtkSpinButton-struct contains only private data and should
 * not be directly modified.
 */
struct _GtkSpinButton
{
  GtkEntry entry;

  /*< private >*/
  GtkSpinButtonPrivate *priv;
};

struct _GtkSpinButtonClass
{
  GtkEntryClass parent_class;

  gint (*input)  (GtkSpinButton *spin_button,
		  gdouble       *new_value);
  gint (*output) (GtkSpinButton *spin_button);
  void (*value_changed) (GtkSpinButton *spin_button);

  /* Action signals for keybindings, do not connect to these */
  void (*change_value) (GtkSpinButton *spin_button,
			GtkScrollType  scroll);

  void (*wrapped) (GtkSpinButton *spin_button);

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};


GDK_AVAILABLE_IN_ALL
GType		ctk_spin_button_get_type	   (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
void		ctk_spin_button_configure	   (GtkSpinButton  *spin_button,
						    GtkAdjustment  *adjustment,
						    gdouble	    climb_rate,
						    guint	    digits);

GDK_AVAILABLE_IN_ALL
GtkWidget*	ctk_spin_button_new		   (GtkAdjustment  *adjustment,
						    gdouble	    climb_rate,
						    guint	    digits);

GDK_AVAILABLE_IN_ALL
GtkWidget*	ctk_spin_button_new_with_range	   (gdouble  min,
						    gdouble  max,
						    gdouble  step);

GDK_AVAILABLE_IN_ALL
void		ctk_spin_button_set_adjustment	   (GtkSpinButton  *spin_button,
						    GtkAdjustment  *adjustment);

GDK_AVAILABLE_IN_ALL
GtkAdjustment*	ctk_spin_button_get_adjustment	   (GtkSpinButton  *spin_button);

GDK_AVAILABLE_IN_ALL
void		ctk_spin_button_set_digits	   (GtkSpinButton  *spin_button,
						    guint	    digits);
GDK_AVAILABLE_IN_ALL
guint           ctk_spin_button_get_digits         (GtkSpinButton  *spin_button);

GDK_AVAILABLE_IN_ALL
void		ctk_spin_button_set_increments	   (GtkSpinButton  *spin_button,
						    gdouble         step,
						    gdouble         page);
GDK_AVAILABLE_IN_ALL
void            ctk_spin_button_get_increments     (GtkSpinButton  *spin_button,
						    gdouble        *step,
						    gdouble        *page);

GDK_AVAILABLE_IN_ALL
void		ctk_spin_button_set_range	   (GtkSpinButton  *spin_button,
						    gdouble         min,
						    gdouble         max);
GDK_AVAILABLE_IN_ALL
void            ctk_spin_button_get_range          (GtkSpinButton  *spin_button,
						    gdouble        *min,
						    gdouble        *max);

GDK_AVAILABLE_IN_ALL
gdouble		ctk_spin_button_get_value          (GtkSpinButton  *spin_button);

GDK_AVAILABLE_IN_ALL
gint		ctk_spin_button_get_value_as_int   (GtkSpinButton  *spin_button);

GDK_AVAILABLE_IN_ALL
void		ctk_spin_button_set_value	   (GtkSpinButton  *spin_button,
						    gdouble	    value);

GDK_AVAILABLE_IN_ALL
void		ctk_spin_button_set_update_policy  (GtkSpinButton  *spin_button,
						    GtkSpinButtonUpdatePolicy  policy);
GDK_AVAILABLE_IN_ALL
GtkSpinButtonUpdatePolicy ctk_spin_button_get_update_policy (GtkSpinButton *spin_button);

GDK_AVAILABLE_IN_ALL
void		ctk_spin_button_set_numeric	   (GtkSpinButton  *spin_button,
						    gboolean	    numeric);
GDK_AVAILABLE_IN_ALL
gboolean        ctk_spin_button_get_numeric        (GtkSpinButton  *spin_button);

GDK_AVAILABLE_IN_ALL
void		ctk_spin_button_spin		   (GtkSpinButton  *spin_button,
						    GtkSpinType     direction,
						    gdouble	    increment);

GDK_AVAILABLE_IN_ALL
void		ctk_spin_button_set_wrap	   (GtkSpinButton  *spin_button,
						    gboolean	    wrap);
GDK_AVAILABLE_IN_ALL
gboolean        ctk_spin_button_get_wrap           (GtkSpinButton  *spin_button);

GDK_AVAILABLE_IN_ALL
void		ctk_spin_button_set_snap_to_ticks  (GtkSpinButton  *spin_button,
						    gboolean	    snap_to_ticks);
GDK_AVAILABLE_IN_ALL
gboolean        ctk_spin_button_get_snap_to_ticks  (GtkSpinButton  *spin_button);
GDK_AVAILABLE_IN_ALL
void            ctk_spin_button_update             (GtkSpinButton  *spin_button);

/* private */
void            _ctk_spin_button_get_panels        (GtkSpinButton  *spin_button,
                                                    GdkWindow     **down_panel,
                                                    GdkWindow     **up_panel);

G_END_DECLS

#endif /* __CTK_SPIN_BUTTON_H__ */
