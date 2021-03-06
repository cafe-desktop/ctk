/* CTK - The GIMP Toolkit
 * ctkprintsettings.c: Print Settings
 * Copyright (C) 2006, Red Hat, Inc.
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
#include <stdlib.h>

#include <glib/gprintf.h>

#include "ctkprintsettings.h"
#include "ctkprintutils.h"
#include "ctktypebuiltins.h"
#include "ctkwidget.h"


/**
 * SECTION:ctkprintsettings
 * @Short_description: Stores print settings
 * @Title: CtkPrintSettings
 *
 * A CtkPrintSettings object represents the settings of a print dialog in
 * a system-independent way. The main use for this object is that once
 * you’ve printed you can get a settings object that represents the settings
 * the user chose, and the next time you print you can pass that object in so
 * that the user doesn’t have to re-set all his settings.
 *
 * Its also possible to enumerate the settings so that you can easily save
 * the settings for the next time your app runs, or even store them in a
 * document. The predefined keys try to use shared values as much as possible
 * so that moving such a document between systems still works.
 *
 * Printing support was added in CTK+ 2.10.
 */

typedef struct _CtkPrintSettingsClass CtkPrintSettingsClass;

#define CTK_IS_PRINT_SETTINGS_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_PRINT_SETTINGS))
#define CTK_PRINT_SETTINGS_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_PRINT_SETTINGS, CtkPrintSettingsClass))
#define CTK_PRINT_SETTINGS_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_PRINT_SETTINGS, CtkPrintSettingsClass))

struct _CtkPrintSettings
{
  GObject parent_instance;

  GHashTable *hash;
};

struct _CtkPrintSettingsClass
{
  GObjectClass parent_class;
};

#define KEYFILE_GROUP_NAME "Print Settings"

G_DEFINE_TYPE (CtkPrintSettings, ctk_print_settings, G_TYPE_OBJECT)

static void
ctk_print_settings_finalize (GObject *object)
{
  CtkPrintSettings *settings = CTK_PRINT_SETTINGS (object);

  g_hash_table_destroy (settings->hash);

  G_OBJECT_CLASS (ctk_print_settings_parent_class)->finalize (object);
}

static void
ctk_print_settings_init (CtkPrintSettings *settings)
{
  settings->hash = g_hash_table_new_full (g_str_hash, g_str_equal,
					  g_free, g_free);
}

static void
ctk_print_settings_class_init (CtkPrintSettingsClass *class)
{
  GObjectClass *gobject_class = (GObjectClass *)class;

  gobject_class->finalize = ctk_print_settings_finalize;
}

/**
 * ctk_print_settings_new:
 * 
 * Creates a new #CtkPrintSettings object.
 *  
 * Returns: a new #CtkPrintSettings object
 *
 * Since: 2.10
 */
CtkPrintSettings *
ctk_print_settings_new (void)
{
  return g_object_new (CTK_TYPE_PRINT_SETTINGS, NULL);
}

static void
copy_hash_entry  (gpointer  key,
		  gpointer  value,
		  gpointer  user_data)
{
  CtkPrintSettings *settings = user_data;

  g_hash_table_insert (settings->hash, 
		       g_strdup (key), 
		       g_strdup (value));
}



/**
 * ctk_print_settings_copy:
 * @other: a #CtkPrintSettings
 *
 * Copies a #CtkPrintSettings object.
 *
 * Returns: (transfer full): a newly allocated copy of @other
 *
 * Since: 2.10
 */
CtkPrintSettings *
ctk_print_settings_copy (CtkPrintSettings *other)
{
  CtkPrintSettings *settings;

  if (other == NULL)
    return NULL;
  
  g_return_val_if_fail (CTK_IS_PRINT_SETTINGS (other), NULL);

  settings = ctk_print_settings_new ();

  g_hash_table_foreach (other->hash,
			copy_hash_entry,
			settings);

  return settings;
}

/**
 * ctk_print_settings_get:
 * @settings: a #CtkPrintSettings
 * @key: a key
 * 
 * Looks up the string value associated with @key.
 * 
 * Returns: the string value for @key
 * 
 * Since: 2.10
 */
const gchar *
ctk_print_settings_get (CtkPrintSettings *settings,
			const gchar      *key)
{
  return g_hash_table_lookup (settings->hash, key);
}

/**
 * ctk_print_settings_set:
 * @settings: a #CtkPrintSettings
 * @key: a key
 * @value: (allow-none): a string value, or %NULL
 *
 * Associates @value with @key.
 *
 * Since: 2.10
 */
void
ctk_print_settings_set (CtkPrintSettings *settings,
			const gchar      *key,
			const gchar      *value)
{
  if (value == NULL)
    ctk_print_settings_unset (settings, key);
  else
    g_hash_table_insert (settings->hash, 
			 g_strdup (key), 
			 g_strdup (value));
}

/**
 * ctk_print_settings_unset:
 * @settings: a #CtkPrintSettings
 * @key: a key
 * 
 * Removes any value associated with @key. 
 * This has the same effect as setting the value to %NULL.
 *
 * Since: 2.10 
 */
void
ctk_print_settings_unset (CtkPrintSettings *settings,
			  const gchar      *key)
{
  g_hash_table_remove (settings->hash, key);
}

/**
 * ctk_print_settings_has_key:
 * @settings: a #CtkPrintSettings
 * @key: a key
 * 
 * Returns %TRUE, if a value is associated with @key.
 * 
 * Returns: %TRUE, if @key has a value
 *
 * Since: 2.10
 */
gboolean        
ctk_print_settings_has_key (CtkPrintSettings *settings,
			    const gchar      *key)
{
  return ctk_print_settings_get (settings, key) != NULL;
}


/**
 * ctk_print_settings_get_bool:
 * @settings: a #CtkPrintSettings
 * @key: a key
 * 
 * Returns the boolean represented by the value
 * that is associated with @key. 
 *
 * The string “true” represents %TRUE, any other 
 * string %FALSE.
 *
 * Returns: %TRUE, if @key maps to a true value.
 * 
 * Since: 2.10
 **/
gboolean
ctk_print_settings_get_bool (CtkPrintSettings *settings,
			     const gchar      *key)
{
  const gchar *val;

  val = ctk_print_settings_get (settings, key);
  if (g_strcmp0 (val, "true") == 0)
    return TRUE;
  
  return FALSE;
}

/**
 * ctk_print_settings_get_bool_with_default:
 * @settings: a #CtkPrintSettings
 * @key: a key
 * @default_val: the default value
 * 
 * Returns the boolean represented by the value
 * that is associated with @key, or @default_val
 * if the value does not represent a boolean.
 *
 * The string “true” represents %TRUE, the string
 * “false” represents %FALSE.
 *
 * Returns: the boolean value associated with @key
 * 
 * Since: 2.10
 */
static gboolean
ctk_print_settings_get_bool_with_default (CtkPrintSettings *settings,
					  const gchar      *key,
					  gboolean          default_val)
{
  const gchar *val;

  val = ctk_print_settings_get (settings, key);
  if (g_strcmp0 (val, "true") == 0)
    return TRUE;

  if (g_strcmp0 (val, "false") == 0)
    return FALSE;
  
  return default_val;
}

/**
 * ctk_print_settings_set_bool:
 * @settings: a #CtkPrintSettings
 * @key: a key
 * @value: a boolean
 * 
 * Sets @key to a boolean value.
 *
 * Since: 2.10
 */
void
ctk_print_settings_set_bool (CtkPrintSettings *settings,
			     const gchar      *key,
			     gboolean          value)
{
  if (value)
    ctk_print_settings_set (settings, key, "true");
  else
    ctk_print_settings_set (settings, key, "false");
}

/**
 * ctk_print_settings_get_double_with_default:
 * @settings: a #CtkPrintSettings
 * @key: a key
 * @def: the default value
 * 
 * Returns the floating point number represented by 
 * the value that is associated with @key, or @default_val
 * if the value does not represent a floating point number.
 *
 * Floating point numbers are parsed with g_ascii_strtod().
 *
 * Returns: the floating point number associated with @key
 * 
 * Since: 2.10
 */
gdouble
ctk_print_settings_get_double_with_default (CtkPrintSettings *settings,
					    const gchar      *key,
					    gdouble           def)
{
  const gchar *val;

  val = ctk_print_settings_get (settings, key);
  if (val == NULL)
    return def;

  return g_ascii_strtod (val, NULL);
}

/**
 * ctk_print_settings_get_double:
 * @settings: a #CtkPrintSettings
 * @key: a key
 * 
 * Returns the double value associated with @key, or 0.
 * 
 * Returns: the double value of @key
 *
 * Since: 2.10
 */
gdouble
ctk_print_settings_get_double (CtkPrintSettings *settings,
			       const gchar      *key)
{
  return ctk_print_settings_get_double_with_default (settings, key, 0.0);
}

/**
 * ctk_print_settings_set_double:
 * @settings: a #CtkPrintSettings
 * @key: a key 
 * @value: a double value
 * 
 * Sets @key to a double value.
 * 
 * Since: 2.10
 */
void
ctk_print_settings_set_double (CtkPrintSettings *settings,
			       const gchar      *key,
			       gdouble           value)
{
  gchar buf[G_ASCII_DTOSTR_BUF_SIZE];
  
  g_ascii_dtostr (buf, G_ASCII_DTOSTR_BUF_SIZE, value);
  ctk_print_settings_set (settings, key, buf);
}

/**
 * ctk_print_settings_get_length:
 * @settings: a #CtkPrintSettings
 * @key: a key
 * @unit: the unit of the return value
 * 
 * Returns the value associated with @key, interpreted
 * as a length. The returned value is converted to @units.
 * 
 * Returns: the length value of @key, converted to @unit
 *
 * Since: 2.10
 */
gdouble
ctk_print_settings_get_length (CtkPrintSettings *settings,
			       const gchar      *key,
			       CtkUnit           unit)
{
  gdouble length = ctk_print_settings_get_double (settings, key);
  return _ctk_print_convert_from_mm (length, unit);
}

/**
 * ctk_print_settings_set_length:
 * @settings: a #CtkPrintSettings
 * @key: a key
 * @value: a length
 * @unit: the unit of @length
 * 
 * Associates a length in units of @unit with @key.
 *
 * Since: 2.10
 */
void
ctk_print_settings_set_length (CtkPrintSettings *settings,
			       const gchar      *key,
			       gdouble           value, 
			       CtkUnit           unit)
{
  ctk_print_settings_set_double (settings, key,
				 _ctk_print_convert_to_mm (value, unit));
}

/**
 * ctk_print_settings_get_int_with_default:
 * @settings: a #CtkPrintSettings
 * @key: a key
 * @def: the default value
 * 
 * Returns the value of @key, interpreted as
 * an integer, or the default value.
 * 
 * Returns: the integer value of @key
 *
 * Since: 2.10
 */
gint
ctk_print_settings_get_int_with_default (CtkPrintSettings *settings,
					 const gchar      *key,
					 gint              def)
{
  const gchar *val;

  val = ctk_print_settings_get (settings, key);
  if (val == NULL)
    return def;

  return atoi (val);
}

/**
 * ctk_print_settings_get_int:
 * @settings: a #CtkPrintSettings
 * @key: a key
 * 
 * Returns the integer value of @key, or 0.
 * 
 * Returns: the integer value of @key 
 *
 * Since: 2.10
 */
gint
ctk_print_settings_get_int (CtkPrintSettings *settings,
			    const gchar      *key)
{
  return ctk_print_settings_get_int_with_default (settings, key, 0);
}

/**
 * ctk_print_settings_set_int:
 * @settings: a #CtkPrintSettings
 * @key: a key
 * @value: an integer 
 * 
 * Sets @key to an integer value.
 *
 * Since: 2.10 
 */
void
ctk_print_settings_set_int (CtkPrintSettings *settings,
			    const gchar      *key,
			    gint              value)
{
  gchar buf[128];
  g_sprintf (buf, "%d", value);
  ctk_print_settings_set (settings, key, buf);
}

/**
 * ctk_print_settings_foreach:
 * @settings: a #CtkPrintSettings
 * @func: (scope call): the function to call
 * @user_data: user data for @func
 *
 * Calls @func for each key-value pair of @settings.
 *
 * Since: 2.10
 */
void
ctk_print_settings_foreach (CtkPrintSettings    *settings,
			    CtkPrintSettingsFunc func,
			    gpointer             user_data)
{
  g_hash_table_foreach (settings->hash, (GHFunc)func, user_data);
}

/**
 * ctk_print_settings_get_printer:
 * @settings: a #CtkPrintSettings
 * 
 * Convenience function to obtain the value of 
 * %CTK_PRINT_SETTINGS_PRINTER.
 *
 * Returns: the printer name
 *
 * Since: 2.10
 */
const gchar *
ctk_print_settings_get_printer (CtkPrintSettings *settings)
{
  return ctk_print_settings_get (settings, CTK_PRINT_SETTINGS_PRINTER);
}


/**
 * ctk_print_settings_set_printer:
 * @settings: a #CtkPrintSettings
 * @printer: the printer name
 * 
 * Convenience function to set %CTK_PRINT_SETTINGS_PRINTER
 * to @printer.
 *
 * Since: 2.10
 */
void
ctk_print_settings_set_printer (CtkPrintSettings *settings,
				const gchar      *printer)
{
  ctk_print_settings_set (settings, CTK_PRINT_SETTINGS_PRINTER, printer);
}

/**
 * ctk_print_settings_get_orientation:
 * @settings: a #CtkPrintSettings
 * 
 * Get the value of %CTK_PRINT_SETTINGS_ORIENTATION, 
 * converted to a #CtkPageOrientation.
 * 
 * Returns: the orientation
 *
 * Since: 2.10
 */
CtkPageOrientation
ctk_print_settings_get_orientation (CtkPrintSettings *settings)
{
  const gchar *val;

  val = ctk_print_settings_get (settings, CTK_PRINT_SETTINGS_ORIENTATION);

  if (val == NULL || strcmp (val, "portrait") == 0)
    return CTK_PAGE_ORIENTATION_PORTRAIT;

  if (strcmp (val, "landscape") == 0)
    return CTK_PAGE_ORIENTATION_LANDSCAPE;
  
  if (strcmp (val, "reverse_portrait") == 0)
    return CTK_PAGE_ORIENTATION_REVERSE_PORTRAIT;
  
  if (strcmp (val, "reverse_landscape") == 0)
    return CTK_PAGE_ORIENTATION_REVERSE_LANDSCAPE;
  
  return CTK_PAGE_ORIENTATION_PORTRAIT;
}

/**
 * ctk_print_settings_set_orientation:
 * @settings: a #CtkPrintSettings
 * @orientation: a page orientation
 * 
 * Sets the value of %CTK_PRINT_SETTINGS_ORIENTATION.
 * 
 * Since: 2.10
 */
void
ctk_print_settings_set_orientation (CtkPrintSettings   *settings,
				    CtkPageOrientation  orientation)
{
  const gchar *val;

  switch (orientation)
    {
    case CTK_PAGE_ORIENTATION_LANDSCAPE:
      val = "landscape";
      break;
    default:
    case CTK_PAGE_ORIENTATION_PORTRAIT:
      val = "portrait";
      break;
    case CTK_PAGE_ORIENTATION_REVERSE_LANDSCAPE:
      val = "reverse_landscape";
      break;
    case CTK_PAGE_ORIENTATION_REVERSE_PORTRAIT:
      val = "reverse_portrait";
      break;
    }
  ctk_print_settings_set (settings, CTK_PRINT_SETTINGS_ORIENTATION, val);
}

/**
 * ctk_print_settings_get_paper_size:
 * @settings: a #CtkPrintSettings
 * 
 * Gets the value of %CTK_PRINT_SETTINGS_PAPER_FORMAT, 
 * converted to a #CtkPaperSize.
 * 
 * Returns: the paper size
 *
 * Since: 2.10
 */
CtkPaperSize *     
ctk_print_settings_get_paper_size (CtkPrintSettings *settings)
{
  const gchar *val;
  const gchar *name;
  gdouble w, h;

  val = ctk_print_settings_get (settings, CTK_PRINT_SETTINGS_PAPER_FORMAT);
  if (val == NULL)
    return NULL;

  if (g_str_has_prefix (val, "custom-")) 
    {
      name = val + strlen ("custom-");
      w = ctk_print_settings_get_paper_width (settings, CTK_UNIT_MM);
      h = ctk_print_settings_get_paper_height (settings, CTK_UNIT_MM);
      return ctk_paper_size_new_custom (name, name, w, h, CTK_UNIT_MM);
    }

  return ctk_paper_size_new (val);
}

/**
 * ctk_print_settings_set_paper_size:
 * @settings: a #CtkPrintSettings
 * @paper_size: a paper size
 * 
 * Sets the value of %CTK_PRINT_SETTINGS_PAPER_FORMAT,
 * %CTK_PRINT_SETTINGS_PAPER_WIDTH and
 * %CTK_PRINT_SETTINGS_PAPER_HEIGHT.
 *
 * Since: 2.10
 */
void
ctk_print_settings_set_paper_size (CtkPrintSettings *settings,
				   CtkPaperSize     *paper_size)
{
  gchar *custom_name;

  if (paper_size == NULL) 
    {
      ctk_print_settings_set (settings, CTK_PRINT_SETTINGS_PAPER_FORMAT, NULL);
      ctk_print_settings_set (settings, CTK_PRINT_SETTINGS_PAPER_WIDTH, NULL);
      ctk_print_settings_set (settings, CTK_PRINT_SETTINGS_PAPER_HEIGHT, NULL);
    }
  else if (ctk_paper_size_is_custom (paper_size)) 
    {
      custom_name = g_strdup_printf ("custom-%s", 
				     ctk_paper_size_get_name (paper_size));
      ctk_print_settings_set (settings, CTK_PRINT_SETTINGS_PAPER_FORMAT, custom_name);
      g_free (custom_name);
      ctk_print_settings_set_paper_width (settings, 
					  ctk_paper_size_get_width (paper_size, 
								    CTK_UNIT_MM),
					  CTK_UNIT_MM);
      ctk_print_settings_set_paper_height (settings, 
					   ctk_paper_size_get_height (paper_size, 
								      CTK_UNIT_MM),
					   CTK_UNIT_MM);
    } 
  else
    ctk_print_settings_set (settings, CTK_PRINT_SETTINGS_PAPER_FORMAT, 
			    ctk_paper_size_get_name (paper_size));
}

/**
 * ctk_print_settings_get_paper_width:
 * @settings: a #CtkPrintSettings
 * @unit: the unit for the return value
 * 
 * Gets the value of %CTK_PRINT_SETTINGS_PAPER_WIDTH,
 * converted to @unit. 
 * 
 * Returns: the paper width, in units of @unit
 *
 * Since: 2.10
 */
gdouble
ctk_print_settings_get_paper_width (CtkPrintSettings *settings,
				    CtkUnit           unit)
{
  return ctk_print_settings_get_length (settings, CTK_PRINT_SETTINGS_PAPER_WIDTH, unit);
}

/**
 * ctk_print_settings_set_paper_width:
 * @settings: a #CtkPrintSettings
 * @width: the paper width
 * @unit: the units of @width
 * 
 * Sets the value of %CTK_PRINT_SETTINGS_PAPER_WIDTH.
 *
 * Since: 2.10
 */
void
ctk_print_settings_set_paper_width (CtkPrintSettings *settings,
				    gdouble           width, 
				    CtkUnit           unit)
{
  ctk_print_settings_set_length (settings, CTK_PRINT_SETTINGS_PAPER_WIDTH, width, unit);
}

/**
 * ctk_print_settings_get_paper_height:
 * @settings: a #CtkPrintSettings
 * @unit: the unit for the return value
 * 
 * Gets the value of %CTK_PRINT_SETTINGS_PAPER_HEIGHT,
 * converted to @unit. 
 * 
 * Returns: the paper height, in units of @unit
 *
 * Since: 2.10
 */
gdouble
ctk_print_settings_get_paper_height (CtkPrintSettings *settings,
				     CtkUnit           unit)
{
  return ctk_print_settings_get_length (settings, 
					CTK_PRINT_SETTINGS_PAPER_HEIGHT,
					unit);
}

/**
 * ctk_print_settings_set_paper_height:
 * @settings: a #CtkPrintSettings
 * @height: the paper height
 * @unit: the units of @height
 * 
 * Sets the value of %CTK_PRINT_SETTINGS_PAPER_HEIGHT.
 *
 * Since: 2.10
 */
void
ctk_print_settings_set_paper_height (CtkPrintSettings *settings,
				     gdouble           height, 
				     CtkUnit           unit)
{
  ctk_print_settings_set_length (settings, 
				 CTK_PRINT_SETTINGS_PAPER_HEIGHT, 
				 height, unit);
}

/**
 * ctk_print_settings_get_use_color:
 * @settings: a #CtkPrintSettings
 * 
 * Gets the value of %CTK_PRINT_SETTINGS_USE_COLOR.
 * 
 * Returns: whether to use color
 *
 * Since: 2.10
 */
gboolean
ctk_print_settings_get_use_color (CtkPrintSettings *settings)
{
  return ctk_print_settings_get_bool_with_default (settings, 
						   CTK_PRINT_SETTINGS_USE_COLOR,
						   TRUE);
}

/**
 * ctk_print_settings_set_use_color:
 * @settings: a #CtkPrintSettings
 * @use_color: whether to use color
 * 
 * Sets the value of %CTK_PRINT_SETTINGS_USE_COLOR.
 * 
 * Since: 2.10
 */
void
ctk_print_settings_set_use_color (CtkPrintSettings *settings,
				  gboolean          use_color)
{
  ctk_print_settings_set_bool (settings,
			       CTK_PRINT_SETTINGS_USE_COLOR, 
			       use_color);
}

/**
 * ctk_print_settings_get_collate:
 * @settings: a #CtkPrintSettings
 * 
 * Gets the value of %CTK_PRINT_SETTINGS_COLLATE.
 * 
 * Returns: whether to collate the printed pages
 *
 * Since: 2.10
 */
gboolean
ctk_print_settings_get_collate (CtkPrintSettings *settings)
{
  return ctk_print_settings_get_bool_with_default (settings,
                                                   CTK_PRINT_SETTINGS_COLLATE,
                                                   TRUE);
}

/**
 * ctk_print_settings_set_collate:
 * @settings: a #CtkPrintSettings
 * @collate: whether to collate the output
 * 
 * Sets the value of %CTK_PRINT_SETTINGS_COLLATE.
 * 
 * Since: 2.10
 */
void
ctk_print_settings_set_collate (CtkPrintSettings *settings,
				gboolean          collate)
{
  ctk_print_settings_set_bool (settings,
			       CTK_PRINT_SETTINGS_COLLATE, 
			       collate);
}

/**
 * ctk_print_settings_get_reverse:
 * @settings: a #CtkPrintSettings
 * 
 * Gets the value of %CTK_PRINT_SETTINGS_REVERSE.
 * 
 * Returns: whether to reverse the order of the printed pages
 *
 * Since: 2.10
 */
gboolean
ctk_print_settings_get_reverse (CtkPrintSettings *settings)
{
  return ctk_print_settings_get_bool (settings, 
				      CTK_PRINT_SETTINGS_REVERSE);
}

/**
 * ctk_print_settings_set_reverse:
 * @settings: a #CtkPrintSettings
 * @reverse: whether to reverse the output
 * 
 * Sets the value of %CTK_PRINT_SETTINGS_REVERSE.
 * 
 * Since: 2.10
 */
void
ctk_print_settings_set_reverse (CtkPrintSettings *settings,
				  gboolean        reverse)
{
  ctk_print_settings_set_bool (settings,
			       CTK_PRINT_SETTINGS_REVERSE, 
			       reverse);
}

/**
 * ctk_print_settings_get_duplex:
 * @settings: a #CtkPrintSettings
 * 
 * Gets the value of %CTK_PRINT_SETTINGS_DUPLEX.
 * 
 * Returns: whether to print the output in duplex.
 *
 * Since: 2.10
 */
CtkPrintDuplex
ctk_print_settings_get_duplex (CtkPrintSettings *settings)
{
  const gchar *val;

  val = ctk_print_settings_get (settings, CTK_PRINT_SETTINGS_DUPLEX);

  if (val == NULL || (strcmp (val, "simplex") == 0))
    return CTK_PRINT_DUPLEX_SIMPLEX;

  if (strcmp (val, "horizontal") == 0)
    return CTK_PRINT_DUPLEX_HORIZONTAL;
  
  if (strcmp (val, "vertical") == 0)
    return CTK_PRINT_DUPLEX_VERTICAL;
  
  return CTK_PRINT_DUPLEX_SIMPLEX;
}

/**
 * ctk_print_settings_set_duplex:
 * @settings: a #CtkPrintSettings
 * @duplex: a #CtkPrintDuplex value
 * 
 * Sets the value of %CTK_PRINT_SETTINGS_DUPLEX.
 * 
 * Since: 2.10
 */
void
ctk_print_settings_set_duplex (CtkPrintSettings *settings,
			       CtkPrintDuplex    duplex)
{
  const gchar *str;

  switch (duplex)
    {
    default:
    case CTK_PRINT_DUPLEX_SIMPLEX:
      str = "simplex";
      break;
    case CTK_PRINT_DUPLEX_HORIZONTAL:
      str = "horizontal";
      break;
    case CTK_PRINT_DUPLEX_VERTICAL:
      str = "vertical";
      break;
    }
  
  ctk_print_settings_set (settings, CTK_PRINT_SETTINGS_DUPLEX, str);
}

/**
 * ctk_print_settings_get_quality:
 * @settings: a #CtkPrintSettings
 * 
 * Gets the value of %CTK_PRINT_SETTINGS_QUALITY.
 * 
 * Returns: the print quality
 *
 * Since: 2.10
 */
CtkPrintQuality
ctk_print_settings_get_quality (CtkPrintSettings *settings)
{
  const gchar *val;

  val = ctk_print_settings_get (settings, CTK_PRINT_SETTINGS_QUALITY);

  if (val == NULL || (strcmp (val, "normal") == 0))
    return CTK_PRINT_QUALITY_NORMAL;

  if (strcmp (val, "high") == 0)
    return CTK_PRINT_QUALITY_HIGH;
  
  if (strcmp (val, "low") == 0)
    return CTK_PRINT_QUALITY_LOW;
  
  if (strcmp (val, "draft") == 0)
    return CTK_PRINT_QUALITY_DRAFT;
  
  return CTK_PRINT_QUALITY_NORMAL;
}

/**
 * ctk_print_settings_set_quality:
 * @settings: a #CtkPrintSettings
 * @quality: a #CtkPrintQuality value
 * 
 * Sets the value of %CTK_PRINT_SETTINGS_QUALITY.
 * 
 * Since: 2.10
 */
void
ctk_print_settings_set_quality (CtkPrintSettings *settings,
				CtkPrintQuality   quality)
{
  const gchar *str;

  switch (quality)
    {
    default:
    case CTK_PRINT_QUALITY_NORMAL:
      str = "normal";
      break;
    case CTK_PRINT_QUALITY_HIGH:
      str = "high";
      break;
    case CTK_PRINT_QUALITY_LOW:
      str = "low";
      break;
    case CTK_PRINT_QUALITY_DRAFT:
      str = "draft";
      break;
    }
  
  ctk_print_settings_set (settings, CTK_PRINT_SETTINGS_QUALITY, str);
}

/**
 * ctk_print_settings_get_page_set:
 * @settings: a #CtkPrintSettings
 * 
 * Gets the value of %CTK_PRINT_SETTINGS_PAGE_SET.
 * 
 * Returns: the set of pages to print
 *
 * Since: 2.10
 */
CtkPageSet
ctk_print_settings_get_page_set (CtkPrintSettings *settings)
{
  const gchar *val;

  val = ctk_print_settings_get (settings, CTK_PRINT_SETTINGS_PAGE_SET);

  if (val == NULL || (strcmp (val, "all") == 0))
    return CTK_PAGE_SET_ALL;

  if (strcmp (val, "even") == 0)
    return CTK_PAGE_SET_EVEN;
  
  if (strcmp (val, "odd") == 0)
    return CTK_PAGE_SET_ODD;
  
  return CTK_PAGE_SET_ALL;
}

/**
 * ctk_print_settings_set_page_set:
 * @settings: a #CtkPrintSettings
 * @page_set: a #CtkPageSet value
 * 
 * Sets the value of %CTK_PRINT_SETTINGS_PAGE_SET.
 * 
 * Since: 2.10
 */
void
ctk_print_settings_set_page_set (CtkPrintSettings *settings,
				 CtkPageSet        page_set)
{
  const gchar *str;

  switch (page_set)
    {
    default:
    case CTK_PAGE_SET_ALL:
      str = "all";
      break;
    case CTK_PAGE_SET_EVEN:
      str = "even";
      break;
    case CTK_PAGE_SET_ODD:
      str = "odd";
      break;
    }
  
  ctk_print_settings_set (settings, CTK_PRINT_SETTINGS_PAGE_SET, str);
}

/**
 * ctk_print_settings_get_number_up_layout:
 * @settings: a #CtkPrintSettings
 * 
 * Gets the value of %CTK_PRINT_SETTINGS_NUMBER_UP_LAYOUT.
 * 
 * Returns: layout of page in number-up mode
 *
 * Since: 2.14
 */
CtkNumberUpLayout
ctk_print_settings_get_number_up_layout (CtkPrintSettings *settings)
{
  CtkNumberUpLayout layout;
  CtkTextDirection  text_direction;
  GEnumClass       *enum_class;
  GEnumValue       *enum_value;
  const gchar      *val;

  g_return_val_if_fail (CTK_IS_PRINT_SETTINGS (settings), CTK_NUMBER_UP_LAYOUT_LEFT_TO_RIGHT_TOP_TO_BOTTOM);

  val = ctk_print_settings_get (settings, CTK_PRINT_SETTINGS_NUMBER_UP_LAYOUT);
  text_direction = ctk_widget_get_default_direction ();

  if (text_direction == CTK_TEXT_DIR_LTR)
    layout = CTK_NUMBER_UP_LAYOUT_LEFT_TO_RIGHT_TOP_TO_BOTTOM;
  else
    layout = CTK_NUMBER_UP_LAYOUT_RIGHT_TO_LEFT_TOP_TO_BOTTOM;

  if (val == NULL)
    return layout;

  enum_class = g_type_class_ref (CTK_TYPE_NUMBER_UP_LAYOUT);
  enum_value = g_enum_get_value_by_nick (enum_class, val);
  if (enum_value)
    layout = enum_value->value;
  g_type_class_unref (enum_class);

  return layout;
}

/**
 * ctk_print_settings_set_number_up_layout:
 * @settings: a #CtkPrintSettings
 * @number_up_layout: a #CtkNumberUpLayout value
 * 
 * Sets the value of %CTK_PRINT_SETTINGS_NUMBER_UP_LAYOUT.
 * 
 * Since: 2.14
 */
void
ctk_print_settings_set_number_up_layout (CtkPrintSettings  *settings,
					 CtkNumberUpLayout  number_up_layout)
{
  GEnumClass *enum_class;
  GEnumValue *enum_value;

  g_return_if_fail (CTK_IS_PRINT_SETTINGS (settings));

  enum_class = g_type_class_ref (CTK_TYPE_NUMBER_UP_LAYOUT);
  enum_value = g_enum_get_value (enum_class, number_up_layout);
  g_return_if_fail (enum_value != NULL);

  ctk_print_settings_set (settings, CTK_PRINT_SETTINGS_NUMBER_UP_LAYOUT, enum_value->value_nick);
  g_type_class_unref (enum_class);
}

/**
 * ctk_print_settings_get_n_copies:
 * @settings: a #CtkPrintSettings
 * 
 * Gets the value of %CTK_PRINT_SETTINGS_N_COPIES.
 * 
 * Returns: the number of copies to print
 *
 * Since: 2.10
 */
gint
ctk_print_settings_get_n_copies (CtkPrintSettings *settings)
{
  return ctk_print_settings_get_int_with_default (settings, CTK_PRINT_SETTINGS_N_COPIES, 1);
}

/**
 * ctk_print_settings_set_n_copies:
 * @settings: a #CtkPrintSettings
 * @num_copies: the number of copies 
 * 
 * Sets the value of %CTK_PRINT_SETTINGS_N_COPIES.
 * 
 * Since: 2.10
 */
void
ctk_print_settings_set_n_copies (CtkPrintSettings *settings,
				 gint              num_copies)
{
  ctk_print_settings_set_int (settings, CTK_PRINT_SETTINGS_N_COPIES,
			      num_copies);
}

/**
 * ctk_print_settings_get_number_up:
 * @settings: a #CtkPrintSettings
 * 
 * Gets the value of %CTK_PRINT_SETTINGS_NUMBER_UP.
 * 
 * Returns: the number of pages per sheet
 *
 * Since: 2.10
 */
gint
ctk_print_settings_get_number_up (CtkPrintSettings *settings)
{
  return ctk_print_settings_get_int_with_default (settings, CTK_PRINT_SETTINGS_NUMBER_UP, 1);
}

/**
 * ctk_print_settings_set_number_up:
 * @settings: a #CtkPrintSettings
 * @number_up: the number of pages per sheet 
 * 
 * Sets the value of %CTK_PRINT_SETTINGS_NUMBER_UP.
 * 
 * Since: 2.10
 */
void
ctk_print_settings_set_number_up (CtkPrintSettings *settings,
				  gint              number_up)
{
  ctk_print_settings_set_int (settings, CTK_PRINT_SETTINGS_NUMBER_UP,
				number_up);
}

/**
 * ctk_print_settings_get_resolution:
 * @settings: a #CtkPrintSettings
 * 
 * Gets the value of %CTK_PRINT_SETTINGS_RESOLUTION.
 * 
 * Returns: the resolution in dpi
 *
 * Since: 2.10
 */
gint
ctk_print_settings_get_resolution (CtkPrintSettings *settings)
{
  return ctk_print_settings_get_int_with_default (settings, CTK_PRINT_SETTINGS_RESOLUTION, 300);
}

/**
 * ctk_print_settings_set_resolution:
 * @settings: a #CtkPrintSettings
 * @resolution: the resolution in dpi
 * 
 * Sets the values of %CTK_PRINT_SETTINGS_RESOLUTION,
 * %CTK_PRINT_SETTINGS_RESOLUTION_X and 
 * %CTK_PRINT_SETTINGS_RESOLUTION_Y.
 * 
 * Since: 2.10
 */
void
ctk_print_settings_set_resolution (CtkPrintSettings *settings,
				   gint              resolution)
{
  ctk_print_settings_set_int (settings, CTK_PRINT_SETTINGS_RESOLUTION,
			      resolution);
  ctk_print_settings_set_int (settings, CTK_PRINT_SETTINGS_RESOLUTION_X,
			      resolution);
  ctk_print_settings_set_int (settings, CTK_PRINT_SETTINGS_RESOLUTION_Y,
			      resolution);
}

/**
 * ctk_print_settings_get_resolution_x:
 * @settings: a #CtkPrintSettings
 * 
 * Gets the value of %CTK_PRINT_SETTINGS_RESOLUTION_X.
 * 
 * Returns: the horizontal resolution in dpi
 *
 * Since: 2.16
 */
gint
ctk_print_settings_get_resolution_x (CtkPrintSettings *settings)
{
  return ctk_print_settings_get_int_with_default (settings, CTK_PRINT_SETTINGS_RESOLUTION_X, 300);
}

/**
 * ctk_print_settings_get_resolution_y:
 * @settings: a #CtkPrintSettings
 * 
 * Gets the value of %CTK_PRINT_SETTINGS_RESOLUTION_Y.
 * 
 * Returns: the vertical resolution in dpi
 *
 * Since: 2.16
 */
gint
ctk_print_settings_get_resolution_y (CtkPrintSettings *settings)
{
  return ctk_print_settings_get_int_with_default (settings, CTK_PRINT_SETTINGS_RESOLUTION_Y, 300);
}

/**
 * ctk_print_settings_set_resolution_xy:
 * @settings: a #CtkPrintSettings
 * @resolution_x: the horizontal resolution in dpi
 * @resolution_y: the vertical resolution in dpi
 * 
 * Sets the values of %CTK_PRINT_SETTINGS_RESOLUTION,
 * %CTK_PRINT_SETTINGS_RESOLUTION_X and
 * %CTK_PRINT_SETTINGS_RESOLUTION_Y.
 * 
 * Since: 2.16
 */
void
ctk_print_settings_set_resolution_xy (CtkPrintSettings *settings,
				      gint              resolution_x,
				      gint              resolution_y)
{
  ctk_print_settings_set_int (settings, CTK_PRINT_SETTINGS_RESOLUTION_X,
			      resolution_x);
  ctk_print_settings_set_int (settings, CTK_PRINT_SETTINGS_RESOLUTION_Y,
			      resolution_y);
  ctk_print_settings_set_int (settings, CTK_PRINT_SETTINGS_RESOLUTION,
			      resolution_x);
}

/**
 * ctk_print_settings_get_printer_lpi:
 * @settings: a #CtkPrintSettings
 * 
 * Gets the value of %CTK_PRINT_SETTINGS_PRINTER_LPI.
 * 
 * Returns: the resolution in lpi (lines per inch)
 *
 * Since: 2.16
 */
gdouble
ctk_print_settings_get_printer_lpi (CtkPrintSettings *settings)
{
  return ctk_print_settings_get_double_with_default (settings, CTK_PRINT_SETTINGS_PRINTER_LPI, 150.0);
}

/**
 * ctk_print_settings_set_printer_lpi:
 * @settings: a #CtkPrintSettings
 * @lpi: the resolution in lpi (lines per inch)
 * 
 * Sets the value of %CTK_PRINT_SETTINGS_PRINTER_LPI.
 * 
 * Since: 2.16
 */
void
ctk_print_settings_set_printer_lpi (CtkPrintSettings *settings,
				    gdouble           lpi)
{
  ctk_print_settings_set_double (settings, CTK_PRINT_SETTINGS_PRINTER_LPI,
			         lpi);
}

/**
 * ctk_print_settings_get_scale:
 * @settings: a #CtkPrintSettings
 * 
 * Gets the value of %CTK_PRINT_SETTINGS_SCALE.
 * 
 * Returns: the scale in percent
 *
 * Since: 2.10
 */
gdouble
ctk_print_settings_get_scale (CtkPrintSettings *settings)
{
  return ctk_print_settings_get_double_with_default (settings,
						     CTK_PRINT_SETTINGS_SCALE,
						     100.0);
}

/**
 * ctk_print_settings_set_scale:
 * @settings: a #CtkPrintSettings
 * @scale: the scale in percent
 * 
 * Sets the value of %CTK_PRINT_SETTINGS_SCALE.
 * 
 * Since: 2.10
 */
void
ctk_print_settings_set_scale (CtkPrintSettings *settings,
			      gdouble           scale)
{
  ctk_print_settings_set_double (settings, CTK_PRINT_SETTINGS_SCALE,
				 scale);
}

/**
 * ctk_print_settings_get_print_pages:
 * @settings: a #CtkPrintSettings
 * 
 * Gets the value of %CTK_PRINT_SETTINGS_PRINT_PAGES.
 * 
 * Returns: which pages to print
 *
 * Since: 2.10
 */
CtkPrintPages
ctk_print_settings_get_print_pages (CtkPrintSettings *settings)
{
  const gchar *val;

  val = ctk_print_settings_get (settings, CTK_PRINT_SETTINGS_PRINT_PAGES);

  if (val == NULL || (strcmp (val, "all") == 0))
    return CTK_PRINT_PAGES_ALL;

  if (strcmp (val, "selection") == 0)
    return CTK_PRINT_PAGES_SELECTION;

  if (strcmp (val, "current") == 0)
    return CTK_PRINT_PAGES_CURRENT;
  
  if (strcmp (val, "ranges") == 0)
    return CTK_PRINT_PAGES_RANGES;
  
  return CTK_PRINT_PAGES_ALL;
}

/**
 * ctk_print_settings_set_print_pages:
 * @settings: a #CtkPrintSettings
 * @pages: a #CtkPrintPages value
 * 
 * Sets the value of %CTK_PRINT_SETTINGS_PRINT_PAGES.
 * 
 * Since: 2.10
 */
void
ctk_print_settings_set_print_pages (CtkPrintSettings *settings,
				    CtkPrintPages     pages)
{
  const gchar *str;

  switch (pages)
    {
    default:
    case CTK_PRINT_PAGES_ALL:
      str = "all";
      break;
    case CTK_PRINT_PAGES_CURRENT:
      str = "current";
      break;
    case CTK_PRINT_PAGES_SELECTION:
      str = "selection";
      break;
    case CTK_PRINT_PAGES_RANGES:
      str = "ranges";
      break;
    }
  
  ctk_print_settings_set (settings, CTK_PRINT_SETTINGS_PRINT_PAGES, str);
}

/**
 * ctk_print_settings_get_page_ranges:
 * @settings: a #CtkPrintSettings
 * @num_ranges: (out): return location for the length of the returned array
 *
 * Gets the value of %CTK_PRINT_SETTINGS_PAGE_RANGES.
 *
 * Returns: (array length=num_ranges) (transfer full): an array
 *     of #CtkPageRanges.  Use g_free() to free the array when
 *     it is no longer needed.
 *
 * Since: 2.10
 */
CtkPageRange *
ctk_print_settings_get_page_ranges (CtkPrintSettings *settings,
				    gint             *num_ranges)
{
  const gchar *val;
  gchar **range_strs;
  CtkPageRange *ranges;
  gint i, n;
  
  val = ctk_print_settings_get (settings, CTK_PRINT_SETTINGS_PAGE_RANGES);

  if (val == NULL)
    {
      *num_ranges = 0;
      return NULL;
    }
  
  range_strs = g_strsplit (val, ",", 0);

  for (i = 0; range_strs[i] != NULL; i++)
    ;

  n = i;

  ranges = g_new0 (CtkPageRange, n);

  for (i = 0; i < n; i++)
    {
      gint start, end;
      gchar *str;

      start = (gint)strtol (range_strs[i], &str, 10);
      end = start;

      if (*str == '-')
	{
	  str++;
	  end = (gint)strtol (str, NULL, 10);
	}

      ranges[i].start = start;
      ranges[i].end = end;
    }

  g_strfreev (range_strs);

  *num_ranges = n;
  return ranges;
}

/**
 * ctk_print_settings_set_page_ranges:
 * @settings: a #CtkPrintSettings
 * @page_ranges: (array length=num_ranges): an array of #CtkPageRanges
 * @num_ranges: the length of @page_ranges
 * 
 * Sets the value of %CTK_PRINT_SETTINGS_PAGE_RANGES.
 * 
 * Since: 2.10
 */
void
ctk_print_settings_set_page_ranges  (CtkPrintSettings *settings,
				     CtkPageRange     *page_ranges,
				     gint              num_ranges)
{
  GString *s;
  gint i;
  
  s = g_string_new ("");

  for (i = 0; i < num_ranges; i++)
    {
      if (page_ranges[i].start == page_ranges[i].end)
	g_string_append_printf (s, "%d", page_ranges[i].start);
      else
	g_string_append_printf (s, "%d-%d",
				page_ranges[i].start,
				page_ranges[i].end);
      if (i < num_ranges - 1)
	g_string_append (s, ",");
    }

  
  ctk_print_settings_set (settings, CTK_PRINT_SETTINGS_PAGE_RANGES, 
			  s->str);

  g_string_free (s, TRUE);
}

/**
 * ctk_print_settings_get_default_source:
 * @settings: a #CtkPrintSettings
 * 
 * Gets the value of %CTK_PRINT_SETTINGS_DEFAULT_SOURCE.
 * 
 * Returns: the default source
 *
 * Since: 2.10
 */
const gchar *
ctk_print_settings_get_default_source (CtkPrintSettings *settings)
{
  return ctk_print_settings_get (settings, CTK_PRINT_SETTINGS_DEFAULT_SOURCE);
}

/**
 * ctk_print_settings_set_default_source:
 * @settings: a #CtkPrintSettings
 * @default_source: the default source
 * 
 * Sets the value of %CTK_PRINT_SETTINGS_DEFAULT_SOURCE.
 * 
 * Since: 2.10
 */
void
ctk_print_settings_set_default_source (CtkPrintSettings *settings,
				       const gchar      *default_source)
{
  ctk_print_settings_set (settings, CTK_PRINT_SETTINGS_DEFAULT_SOURCE, default_source);
}
     
/**
 * ctk_print_settings_get_media_type:
 * @settings: a #CtkPrintSettings
 * 
 * Gets the value of %CTK_PRINT_SETTINGS_MEDIA_TYPE.
 *
 * The set of media types is defined in PWG 5101.1-2002 PWG.
 * 
 * Returns: the media type
 *
 * Since: 2.10
 */
const gchar *
ctk_print_settings_get_media_type (CtkPrintSettings *settings)
{
  return ctk_print_settings_get (settings, CTK_PRINT_SETTINGS_MEDIA_TYPE);
}

/**
 * ctk_print_settings_set_media_type:
 * @settings: a #CtkPrintSettings
 * @media_type: the media type
 * 
 * Sets the value of %CTK_PRINT_SETTINGS_MEDIA_TYPE.
 * 
 * The set of media types is defined in PWG 5101.1-2002 PWG.
 *
 * Since: 2.10
 */
void
ctk_print_settings_set_media_type (CtkPrintSettings *settings,
				   const gchar      *media_type)
{
  ctk_print_settings_set (settings, CTK_PRINT_SETTINGS_MEDIA_TYPE, media_type);
}

/**
 * ctk_print_settings_get_dither:
 * @settings: a #CtkPrintSettings
 * 
 * Gets the value of %CTK_PRINT_SETTINGS_DITHER.
 * 
 * Returns: the dithering that is used
 *
 * Since: 2.10
 */
const gchar *
ctk_print_settings_get_dither (CtkPrintSettings *settings)
{
  return ctk_print_settings_get (settings, CTK_PRINT_SETTINGS_DITHER);
}

/**
 * ctk_print_settings_set_dither:
 * @settings: a #CtkPrintSettings
 * @dither: the dithering that is used
 * 
 * Sets the value of %CTK_PRINT_SETTINGS_DITHER.
 *
 * Since: 2.10
 */
void
ctk_print_settings_set_dither (CtkPrintSettings *settings,
			       const gchar      *dither)
{
  ctk_print_settings_set (settings, CTK_PRINT_SETTINGS_DITHER, dither);
}
     
/**
 * ctk_print_settings_get_finishings:
 * @settings: a #CtkPrintSettings
 * 
 * Gets the value of %CTK_PRINT_SETTINGS_FINISHINGS.
 * 
 * Returns: the finishings
 *
 * Since: 2.10
 */
const gchar *
ctk_print_settings_get_finishings (CtkPrintSettings *settings)
{
  return ctk_print_settings_get (settings, CTK_PRINT_SETTINGS_FINISHINGS);
}

/**
 * ctk_print_settings_set_finishings:
 * @settings: a #CtkPrintSettings
 * @finishings: the finishings
 * 
 * Sets the value of %CTK_PRINT_SETTINGS_FINISHINGS.
 *
 * Since: 2.10
 */
void
ctk_print_settings_set_finishings (CtkPrintSettings *settings,
				   const gchar      *finishings)
{
  ctk_print_settings_set (settings, CTK_PRINT_SETTINGS_FINISHINGS, finishings);
}
     
/**
 * ctk_print_settings_get_output_bin:
 * @settings: a #CtkPrintSettings
 * 
 * Gets the value of %CTK_PRINT_SETTINGS_OUTPUT_BIN.
 * 
 * Returns: the output bin
 *
 * Since: 2.10
 */
const gchar *
ctk_print_settings_get_output_bin (CtkPrintSettings *settings)
{
  return ctk_print_settings_get (settings, CTK_PRINT_SETTINGS_OUTPUT_BIN);
}

/**
 * ctk_print_settings_set_output_bin:
 * @settings: a #CtkPrintSettings
 * @output_bin: the output bin
 * 
 * Sets the value of %CTK_PRINT_SETTINGS_OUTPUT_BIN.
 *
 * Since: 2.10
 */
void
ctk_print_settings_set_output_bin (CtkPrintSettings *settings,
				   const gchar      *output_bin)
{
  ctk_print_settings_set (settings, CTK_PRINT_SETTINGS_OUTPUT_BIN, output_bin);
}

/**
 * ctk_print_settings_load_file:
 * @settings: a #CtkPrintSettings
 * @file_name: (type filename): the filename to read the settings from
 * @error: (allow-none): return location for errors, or %NULL
 *
 * Reads the print settings from @file_name. If the file could not be loaded
 * then error is set to either a #GFileError or #GKeyFileError.
 * See ctk_print_settings_to_file().
 *
 * Returns: %TRUE on success
 *
 * Since: 2.14
 */
gboolean
ctk_print_settings_load_file (CtkPrintSettings *settings,
                              const gchar      *file_name,
                              GError          **error)
{
  gboolean retval = FALSE;
  GKeyFile *key_file;

  g_return_val_if_fail (CTK_IS_PRINT_SETTINGS (settings), FALSE);
  g_return_val_if_fail (file_name != NULL, FALSE);

  key_file = g_key_file_new ();

  if (g_key_file_load_from_file (key_file, file_name, 0, error) &&
      ctk_print_settings_load_key_file (settings, key_file, NULL, error))
    retval = TRUE;

  g_key_file_free (key_file);

  return retval;
}

/**
 * ctk_print_settings_new_from_file:
 * @file_name: (type filename): the filename to read the settings from
 * @error: (allow-none): return location for errors, or %NULL
 * 
 * Reads the print settings from @file_name. Returns a new #CtkPrintSettings
 * object with the restored settings, or %NULL if an error occurred. If the
 * file could not be loaded then error is set to either a #GFileError or
 * #GKeyFileError.  See ctk_print_settings_to_file().
 *
 * Returns: the restored #CtkPrintSettings
 * 
 * Since: 2.12
 */
CtkPrintSettings *
ctk_print_settings_new_from_file (const gchar  *file_name,
			          GError      **error)
{
  CtkPrintSettings *settings = ctk_print_settings_new ();

  if (!ctk_print_settings_load_file (settings, file_name, error))
    {
      g_object_unref (settings);
      settings = NULL;
    }

  return settings;
}

/**
 * ctk_print_settings_load_key_file:
 * @settings: a #CtkPrintSettings
 * @key_file: the #GKeyFile to retrieve the settings from
 * @group_name: (allow-none): the name of the group to use, or %NULL to use the default
 *     “Print Settings”
 * @error: (allow-none): return location for errors, or %NULL
 * 
 * Reads the print settings from the group @group_name in @key_file. If the
 * file could not be loaded then error is set to either a #GFileError or
 * #GKeyFileError.
 *
 * Returns: %TRUE on success
 * 
 * Since: 2.14
 */
gboolean
ctk_print_settings_load_key_file (CtkPrintSettings *settings,
				  GKeyFile         *key_file,
				  const gchar      *group_name,
				  GError          **error)
{
  gchar **keys;
  gsize n_keys, i;
  GError *err = NULL;

  g_return_val_if_fail (CTK_IS_PRINT_SETTINGS (settings), FALSE);
  g_return_val_if_fail (key_file != NULL, FALSE);

  if (!group_name)
    group_name = KEYFILE_GROUP_NAME;

  keys = g_key_file_get_keys (key_file,
			      group_name,
			      &n_keys,
			      &err);
  if (err != NULL)
    {
      g_propagate_error (error, err);
      return FALSE;
    }
   
  for (i = 0 ; i < n_keys; ++i)
    {
      gchar *value;

      value = g_key_file_get_string (key_file,
				     group_name,
				     keys[i],
				     NULL);
      if (!value)
        continue;

      ctk_print_settings_set (settings, keys[i], value);
      g_free (value);
    }

  g_strfreev (keys);

  return TRUE;
}

/**
 * ctk_print_settings_new_from_key_file:
 * @key_file: the #GKeyFile to retrieve the settings from
 * @group_name: (allow-none): the name of the group to use, or %NULL to use
 *     the default “Print Settings”
 * @error: (allow-none): return location for errors, or %NULL
 *
 * Reads the print settings from the group @group_name in @key_file.  Returns a
 * new #CtkPrintSettings object with the restored settings, or %NULL if an
 * error occurred. If the file could not be loaded then error is set to either
 * a #GFileError or #GKeyFileError.
 *
 * Returns: the restored #CtkPrintSettings
 *
 * Since: 2.12
 */
CtkPrintSettings *
ctk_print_settings_new_from_key_file (GKeyFile     *key_file,
				      const gchar  *group_name,
				      GError      **error)
{
  CtkPrintSettings *settings = ctk_print_settings_new ();

  if (!ctk_print_settings_load_key_file (settings, key_file,
                                         group_name, error))
    {
      g_object_unref (settings);
      settings = NULL;
    }

  return settings;
}

/**
 * ctk_print_settings_to_file:
 * @settings: a #CtkPrintSettings
 * @file_name: (type filename): the file to save to
 * @error: (allow-none): return location for errors, or %NULL
 * 
 * This function saves the print settings from @settings to @file_name. If the
 * file could not be loaded then error is set to either a #GFileError or
 * #GKeyFileError.
 * 
 * Returns: %TRUE on success
 *
 * Since: 2.12
 */
gboolean
ctk_print_settings_to_file (CtkPrintSettings  *settings,
			    const gchar       *file_name,
			    GError           **error)
{
  GKeyFile *key_file;
  gboolean retval = FALSE;
  char *data = NULL;
  gsize len;
  GError *err = NULL;

  g_return_val_if_fail (CTK_IS_PRINT_SETTINGS (settings), FALSE);
  g_return_val_if_fail (file_name != NULL, FALSE);

  key_file = g_key_file_new ();
  ctk_print_settings_to_key_file (settings, key_file, NULL);

  data = g_key_file_to_data (key_file, &len, &err);
  if (!data)
    goto out;

  retval = g_file_set_contents (file_name, data, len, &err);

out:
  if (err != NULL)
    g_propagate_error (error, err);

  g_key_file_free (key_file);
  g_free (data);

  return retval;
}

typedef struct {
  GKeyFile *key_file;
  const gchar *group_name;
} SettingsData;

static void
add_value_to_key_file (const gchar  *key,
		       const gchar  *value,
		       SettingsData *data)
{
  g_key_file_set_string (data->key_file, data->group_name, key, value);
}

/**
 * ctk_print_settings_to_key_file:
 * @settings: a #CtkPrintSettings
 * @key_file: the #GKeyFile to save the print settings to
 * @group_name: (nullable): the group to add the settings to in @key_file, or
 *     %NULL to use the default “Print Settings”
 *
 * This function adds the print settings from @settings to @key_file.
 * 
 * Since: 2.12
 */
void
ctk_print_settings_to_key_file (CtkPrintSettings  *settings,
			        GKeyFile          *key_file,
				const gchar       *group_name)
{
  SettingsData data;

  g_return_if_fail (CTK_IS_PRINT_SETTINGS (settings));
  g_return_if_fail (key_file != NULL);

  if (!group_name)
    group_name = KEYFILE_GROUP_NAME;

  data.key_file = key_file;
  data.group_name = group_name;

  ctk_print_settings_foreach (settings,
			      (CtkPrintSettingsFunc) add_value_to_key_file,
			      &data);
}

static void
add_to_variant (const gchar *key,
                const gchar *value,
                gpointer     data)
{
  GVariantBuilder *builder = data;
  g_variant_builder_add (builder, "{sv}", key, g_variant_new_string (value));
}

/**
 * ctk_print_settings_to_gvariant:
 * @settings: a #CtkPrintSettings
 *
 * Serialize print settings to an a{sv} variant.
 *
 * Returns: (transfer none): a new, floating, #GVariant
 *
 * Since: 3.22
 */
GVariant *
ctk_print_settings_to_gvariant (CtkPrintSettings *settings)
{
  GVariantBuilder builder;

  g_variant_builder_init (&builder, G_VARIANT_TYPE_VARDICT);
  ctk_print_settings_foreach (settings, add_to_variant, &builder);

  return g_variant_builder_end (&builder);
}

/**
 * ctk_print_settings_new_from_gvariant:
 * @variant: an a{sv} #GVariant
 *
 * Deserialize print settings from an a{sv} variant in
 * the format produced by ctk_print_settings_to_gvariant().
 *
 * Returns: (transfer full): a new #CtkPrintSettings object
 *
 * Since: 3.22
 */
CtkPrintSettings *
ctk_print_settings_new_from_gvariant (GVariant *variant)
{
  CtkPrintSettings *settings;
  int i;

  g_return_val_if_fail (g_variant_is_of_type (variant, G_VARIANT_TYPE_VARDICT), NULL);

  settings = ctk_print_settings_new ();

  for (i = 0; i < g_variant_n_children (variant); i++)
    {
      const char *key;
      GVariant *v;

      g_variant_get_child (variant, i, "{&sv}", &key, &v);
      if (g_variant_is_of_type (v, G_VARIANT_TYPE_STRING))
        ctk_print_settings_set (settings, key, g_variant_get_string (v, NULL));
      g_variant_unref (v);
    }

  return settings;
}
