/* CTK - The GIMP Toolkit
 * Copyright (C) 1998, 2001 Tim Janik
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

#ifndef __CTK_ACCEL_GROUP_H__
#define __CTK_ACCEL_GROUP_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <cdk/cdk.h>
#include <ctk/ctkenums.h>

G_BEGIN_DECLS


/* --- type macros --- */
#define CTK_TYPE_ACCEL_GROUP              (ctk_accel_group_get_type ())
#define CTK_ACCEL_GROUP(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), CTK_TYPE_ACCEL_GROUP, CtkAccelGroup))
#define CTK_ACCEL_GROUP_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_ACCEL_GROUP, CtkAccelGroupClass))
#define CTK_IS_ACCEL_GROUP(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), CTK_TYPE_ACCEL_GROUP))
#define CTK_IS_ACCEL_GROUP_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_ACCEL_GROUP))
#define CTK_ACCEL_GROUP_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_ACCEL_GROUP, CtkAccelGroupClass))


/* --- accel flags --- */
/**
 * CtkAccelFlags:
 * @CTK_ACCEL_VISIBLE: Accelerator is visible
 * @CTK_ACCEL_LOCKED: Accelerator not removable
 * @CTK_ACCEL_MASK: Mask
 *
 * Accelerator flags used with ctk_accel_group_connect().
 */
typedef enum
{
  CTK_ACCEL_VISIBLE        = 1 << 0,
  CTK_ACCEL_LOCKED         = 1 << 1,
  CTK_ACCEL_MASK           = 0x07
} CtkAccelFlags;


/* --- typedefs & structures --- */
typedef struct _CtkAccelGroup	          CtkAccelGroup;
typedef struct _CtkAccelGroupClass        CtkAccelGroupClass;
typedef struct _CtkAccelGroupPrivate      CtkAccelGroupPrivate;
typedef struct _CtkAccelKey               CtkAccelKey;
typedef struct _CtkAccelGroupEntry        CtkAccelGroupEntry;
typedef gboolean (*CtkAccelGroupActivate) (CtkAccelGroup  *accel_group,
					   GObject        *acceleratable,
					   guint           keyval,
					   CdkModifierType modifier);

/**
 * CtkAccelGroupFindFunc:
 * @key: 
 * @closure: 
 * @data: (closure):
 * 
 * Since: 2.2
 */
typedef gboolean (*CtkAccelGroupFindFunc) (CtkAccelKey    *key,
					   GClosure       *closure,
					   gpointer        data);

/**
 * CtkAccelGroup:
 * 
 * An object representing and maintaining a group of accelerators.
 */
struct _CtkAccelGroup
{
  GObject               parent;
  CtkAccelGroupPrivate *priv;
};

/**
 * CtkAccelGroupClass:
 * @parent_class: The parent class.
 * @accel_changed: Signal emitted when an entry is added to or removed
 *    from the accel group.
 */
struct _CtkAccelGroupClass
{
  GObjectClass parent_class;

  /*< public >*/

  void	(*accel_changed)	(CtkAccelGroup	*accel_group,
				 guint           keyval,
				 CdkModifierType modifier,
				 GClosure       *accel_closure);

  /*< private >*/

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

/**
 * CtkAccelKey:
 * @accel_key: The accelerator keyval
 * @accel_mods:The accelerator modifiers
 * @accel_flags: The accelerator flags
 */
struct _CtkAccelKey
{
  guint           accel_key;
  CdkModifierType accel_mods;
  guint           accel_flags : 16;
};


/* -- Accelerator Groups --- */
GDK_AVAILABLE_IN_ALL
GType          ctk_accel_group_get_type           (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
CtkAccelGroup* ctk_accel_group_new	      	  (void);
GDK_AVAILABLE_IN_ALL
gboolean       ctk_accel_group_get_is_locked      (CtkAccelGroup  *accel_group);
GDK_AVAILABLE_IN_ALL
CdkModifierType 
               ctk_accel_group_get_modifier_mask  (CtkAccelGroup  *accel_group);
GDK_AVAILABLE_IN_ALL
void	       ctk_accel_group_lock		  (CtkAccelGroup  *accel_group);
GDK_AVAILABLE_IN_ALL
void	       ctk_accel_group_unlock		  (CtkAccelGroup  *accel_group);
GDK_AVAILABLE_IN_ALL
void	       ctk_accel_group_connect		  (CtkAccelGroup  *accel_group,
						   guint	   accel_key,
						   CdkModifierType accel_mods,
						   CtkAccelFlags   accel_flags,
						   GClosure	  *closure);
GDK_AVAILABLE_IN_ALL
void           ctk_accel_group_connect_by_path    (CtkAccelGroup  *accel_group,
						   const gchar	  *accel_path,
						   GClosure	  *closure);
GDK_AVAILABLE_IN_ALL
gboolean       ctk_accel_group_disconnect	  (CtkAccelGroup  *accel_group,
						   GClosure	  *closure);
GDK_AVAILABLE_IN_ALL
gboolean       ctk_accel_group_disconnect_key	  (CtkAccelGroup  *accel_group,
						   guint	   accel_key,
						   CdkModifierType accel_mods);
GDK_AVAILABLE_IN_ALL
gboolean       ctk_accel_group_activate           (CtkAccelGroup   *accel_group,
                                                   GQuark	   accel_quark,
                                                   GObject	  *acceleratable,
                                                   guint	   accel_key,
                                                   CdkModifierType accel_mods);


/* --- CtkActivatable glue --- */
void		_ctk_accel_group_attach		(CtkAccelGroup	*accel_group,
						 GObject	*object);
void		_ctk_accel_group_detach		(CtkAccelGroup	*accel_group,
						 GObject	*object);
GDK_AVAILABLE_IN_ALL
gboolean        ctk_accel_groups_activate      	(GObject	*object,
						 guint		 accel_key,
						 CdkModifierType accel_mods);
GDK_AVAILABLE_IN_ALL
GSList*	        ctk_accel_groups_from_object    (GObject	*object);
GDK_AVAILABLE_IN_ALL
CtkAccelKey*	ctk_accel_group_find		(CtkAccelGroup	      *accel_group,
						 CtkAccelGroupFindFunc find_func,
						 gpointer              data);
GDK_AVAILABLE_IN_ALL
CtkAccelGroup*	ctk_accel_group_from_accel_closure (GClosure    *closure);


/* --- Accelerators--- */
GDK_AVAILABLE_IN_ALL
gboolean ctk_accelerator_valid		      (guint	        keyval,
					       CdkModifierType  modifiers) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
void	 ctk_accelerator_parse		      (const gchar     *accelerator,
					       guint	       *accelerator_key,
					       CdkModifierType *accelerator_mods);
GDK_AVAILABLE_IN_3_4
void ctk_accelerator_parse_with_keycode       (const gchar     *accelerator,
                                               guint           *accelerator_key,
                                               guint          **accelerator_codes,
                                               CdkModifierType *accelerator_mods);
GDK_AVAILABLE_IN_ALL
gchar*	 ctk_accelerator_name		      (guint	        accelerator_key,
					       CdkModifierType  accelerator_mods);
GDK_AVAILABLE_IN_3_4
gchar*	 ctk_accelerator_name_with_keycode    (CdkDisplay      *display,
                                               guint            accelerator_key,
                                               guint            keycode,
                                               CdkModifierType  accelerator_mods);
GDK_AVAILABLE_IN_ALL
gchar*   ctk_accelerator_get_label            (guint           accelerator_key,
                                               CdkModifierType accelerator_mods);
GDK_AVAILABLE_IN_3_4
gchar*   ctk_accelerator_get_label_with_keycode (CdkDisplay      *display,
                                                 guint            accelerator_key,
                                                 guint            keycode,
                                                 CdkModifierType  accelerator_mods);
GDK_AVAILABLE_IN_ALL
void	 ctk_accelerator_set_default_mod_mask (CdkModifierType  default_mod_mask);
GDK_AVAILABLE_IN_ALL
CdkModifierType
	 ctk_accelerator_get_default_mod_mask (void);

GDK_AVAILABLE_IN_ALL
CtkAccelGroupEntry*	ctk_accel_group_query	(CtkAccelGroup	*accel_group,
						 guint		 accel_key,
						 CdkModifierType accel_mods,
						 guint          *n_entries);

struct _CtkAccelGroupEntry
{
  CtkAccelKey  key;
  GClosure    *closure;
  GQuark       accel_path_quark;
};

G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkAccelGroup, g_object_unref)

G_END_DECLS

#endif /* __CTK_ACCEL_GROUP_H__ */
