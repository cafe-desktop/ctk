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

#ifndef __CTK_ACCEL_MAP_H__
#define __CTK_ACCEL_MAP_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkaccelgroup.h>

G_BEGIN_DECLS

/* --- global CtkAccelMap object --- */
#define CTK_TYPE_ACCEL_MAP                (ctk_accel_map_get_type ())
#define CTK_ACCEL_MAP(accel_map)	  (G_TYPE_CHECK_INSTANCE_CAST ((accel_map), CTK_TYPE_ACCEL_MAP, CtkAccelMap))
#define CTK_ACCEL_MAP_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_ACCEL_MAP, CtkAccelMapClass))
#define CTK_IS_ACCEL_MAP(accel_map)	  (G_TYPE_CHECK_INSTANCE_TYPE ((accel_map), CTK_TYPE_ACCEL_MAP))
#define CTK_IS_ACCEL_MAP_CLASS(klass)	  (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_ACCEL_MAP))
#define CTK_ACCEL_MAP_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_ACCEL_MAP, CtkAccelMapClass))

typedef struct _CtkAccelMap      CtkAccelMap;
typedef struct _CtkAccelMapClass CtkAccelMapClass;

/* --- notifier --- */
/**
 * CtkAccelMapForeach:
 * @data: User data passed to ctk_accel_map_foreach() or
 *  ctk_accel_map_foreach_unfiltered()
 * @accel_path: Accel path of the current accelerator
 * @accel_key: Key of the current accelerator
 * @accel_mods: Modifiers of the current accelerator
 * @changed: Changed flag of the accelerator (if %TRUE, accelerator has changed
 *  during runtime and would need to be saved during an accelerator dump)
 */
typedef void (*CtkAccelMapForeach)		(gpointer	 data,
						 const gchar	*accel_path,
						 guint           accel_key,
						 CdkModifierType accel_mods,
						 gboolean	 changed);


/* --- public API --- */

CDK_AVAILABLE_IN_ALL
void	   ctk_accel_map_add_entry	(const gchar		*accel_path,
					 guint			 accel_key,
					 CdkModifierType         accel_mods);
CDK_AVAILABLE_IN_ALL
gboolean   ctk_accel_map_lookup_entry	(const gchar		*accel_path,
					 CtkAccelKey		*key);
CDK_AVAILABLE_IN_ALL
gboolean   ctk_accel_map_change_entry	(const gchar		*accel_path,
					 guint			 accel_key,
					 CdkModifierType	 accel_mods,
					 gboolean		 replace);
CDK_AVAILABLE_IN_ALL
void	   ctk_accel_map_load		(const gchar		*file_name);
CDK_AVAILABLE_IN_ALL
void	   ctk_accel_map_save		(const gchar		*file_name);
CDK_AVAILABLE_IN_ALL
void	   ctk_accel_map_foreach	(gpointer		 data,
					 CtkAccelMapForeach	 foreach_func);
CDK_AVAILABLE_IN_ALL
void	   ctk_accel_map_load_fd	(gint			 fd);
CDK_AVAILABLE_IN_ALL
void	   ctk_accel_map_load_scanner	(GScanner		*scanner);
CDK_AVAILABLE_IN_ALL
void	   ctk_accel_map_save_fd	(gint			 fd);

CDK_AVAILABLE_IN_ALL
void       ctk_accel_map_lock_path      (const gchar            *accel_path);
CDK_AVAILABLE_IN_ALL
void       ctk_accel_map_unlock_path    (const gchar            *accel_path);

/* --- filter functions --- */
CDK_AVAILABLE_IN_ALL
void	ctk_accel_map_add_filter	 (const gchar		*filter_pattern);
CDK_AVAILABLE_IN_ALL
void	ctk_accel_map_foreach_unfiltered (gpointer		 data,
					  CtkAccelMapForeach	 foreach_func);

/* --- notification --- */
CDK_AVAILABLE_IN_ALL
GType        ctk_accel_map_get_type (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
CtkAccelMap *ctk_accel_map_get      (void);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkAccelMap, g_object_unref)

G_END_DECLS

#endif /* __CTK_ACCEL_MAP_H__ */
