/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * CtkBindingSet: Keybinding manager for GObjects.
 * Copyright (C) 1998 Tim Janik
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

#ifndef __CTK_BINDINGS_H__
#define __CTK_BINDINGS_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <gdk/gdk.h>
#include <ctk/ctkenums.h>

G_BEGIN_DECLS

typedef struct _CtkBindingSet    CtkBindingSet;
typedef struct _CtkBindingEntry  CtkBindingEntry;
typedef struct _CtkBindingSignal CtkBindingSignal;
typedef struct _CtkBindingArg    CtkBindingArg;

/**
 * CtkBindingSet:
 * @set_name: unique name of this binding set
 * @priority: unused
 * @widget_path_pspecs: unused
 * @widget_class_pspecs: unused
 * @class_branch_pspecs: unused
 * @entries: the key binding entries in this binding set
 * @current: implementation detail
 * @parsed: whether this binding set stems from a CSS file and is reset upon theme changes
 *
 * A binding set maintains a list of activatable key bindings.
 * A single binding set can match multiple types of widgets.
 * Similar to style contexts, can be matched by any information contained
 * in a widgets #CtkWidgetPath. When a binding within a set is matched upon
 * activation, an action signal is emitted on the target widget to carry out
 * the actual activation.
 */
struct _CtkBindingSet
{
  gchar           *set_name;
  gint             priority;
  GSList          *widget_path_pspecs;
  GSList          *widget_class_pspecs;
  GSList          *class_branch_pspecs;
  CtkBindingEntry *entries;
  CtkBindingEntry *current;
  guint            parsed : 1;
};

/**
 * CtkBindingEntry:
 * @keyval: key value to match
 * @modifiers: key modifiers to match
 * @binding_set: binding set this entry belongs to
 * @destroyed: implementation detail
 * @in_emission: implementation detail
 * @marks_unbound: implementation detail
 * @set_next: linked list of entries maintained by binding set
 * @hash_next: implementation detail
 * @signals: action signals of this entry
 *
 * Each key binding element of a binding sets binding list is
 * represented by a CtkBindingEntry.
 */
struct _CtkBindingEntry
{
  /* key portion */
  guint             keyval;
  GdkModifierType   modifiers;

  CtkBindingSet    *binding_set;
  guint             destroyed     : 1;
  guint             in_emission   : 1;
  guint             marks_unbound : 1;
  CtkBindingEntry  *set_next;
  CtkBindingEntry  *hash_next;
  CtkBindingSignal *signals;
};

/**
 * CtkBindingArg:
 * @arg_type: implementation detail
 *
 * A #CtkBindingArg holds the data associated with
 * an argument for a key binding signal emission as
 * stored in #CtkBindingSignal.
 */
struct _CtkBindingArg
{
  GType      arg_type;
  union {
    glong    long_data;
    gdouble  double_data;
    gchar   *string_data;
  } d;
};

/**
 * CtkBindingSignal:
 * @next: implementation detail
 * @signal_name: the action signal to be emitted
 * @n_args: number of arguments specified for the signal
 * @args: (array length=n_args): the arguments specified for the signal
 *
 * A CtkBindingSignal stores the necessary information to
 * activate a widget in response to a key press via a signal
 * emission.
 */
struct _CtkBindingSignal
{
  CtkBindingSignal *next;
  gchar            *signal_name;
  guint             n_args;
  CtkBindingArg    *args;
};

GDK_AVAILABLE_IN_ALL
CtkBindingSet *ctk_binding_set_new           (const gchar         *set_name);
GDK_AVAILABLE_IN_ALL
CtkBindingSet *ctk_binding_set_by_class      (gpointer             object_class);
GDK_AVAILABLE_IN_ALL
CtkBindingSet *ctk_binding_set_find          (const gchar         *set_name);

GDK_AVAILABLE_IN_ALL
gboolean       ctk_bindings_activate         (GObject             *object,
                                              guint                keyval,
                                              GdkModifierType      modifiers);
GDK_AVAILABLE_IN_ALL
gboolean       ctk_bindings_activate_event   (GObject             *object,
                                              GdkEventKey         *event);
GDK_AVAILABLE_IN_ALL
gboolean       ctk_binding_set_activate      (CtkBindingSet       *binding_set,
                                              guint                keyval,
                                              GdkModifierType      modifiers,
                                              GObject             *object);

GDK_AVAILABLE_IN_ALL
void           ctk_binding_entry_skip        (CtkBindingSet       *binding_set,
                                              guint                keyval,
                                              GdkModifierType      modifiers);
GDK_AVAILABLE_IN_ALL
void           ctk_binding_entry_add_signal  (CtkBindingSet       *binding_set,
                                              guint                keyval,
                                              GdkModifierType      modifiers,
                                              const gchar         *signal_name,
                                              guint                n_args,
                                              ...);
GDK_AVAILABLE_IN_ALL
void           ctk_binding_entry_add_signall (CtkBindingSet       *binding_set,
                                              guint                keyval,
                                              GdkModifierType      modifiers,
                                              const gchar         *signal_name,
                                              GSList              *binding_args);

GDK_AVAILABLE_IN_ALL
GTokenType     ctk_binding_entry_add_signal_from_string
                                             (CtkBindingSet       *binding_set,
                                              const gchar         *signal_desc);

GDK_AVAILABLE_IN_ALL
void           ctk_binding_entry_remove      (CtkBindingSet       *binding_set,
                                              guint                keyval,
                                              GdkModifierType      modifiers);

G_END_DECLS

#endif /* __CTK_BINDINGS_H__ */
