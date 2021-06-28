/* CDK - The GIMP Drawing Kit
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
 * Modified by the CTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/. 
 */

#include "config.h"

#include "cdkproperty.h"
#include "cdkmain.h"
#include "cdkprivate.h"
#include "cdkinternals.h"
#include "cdkselection.h"
#include "cdkprivate-x11.h"
#include "cdkdisplay-x11.h"
#include "cdkscreen-x11.h"

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <string.h>

#define N_PREDEFINED_ATOMS 69

#define ATOM_TO_INDEX(atom) (GPOINTER_TO_UINT(atom))
#define INDEX_TO_ATOM(atom) ((CdkAtom)GUINT_TO_POINTER(atom))

static void
insert_atom_pair (CdkDisplay *display,
		  CdkAtom     virtual_atom,
		  Atom        xatom)
{
  CdkX11Display *display_x11 = CDK_X11_DISPLAY (display);  
  
  if (!display_x11->atom_from_virtual)
    {
      display_x11->atom_from_virtual = g_hash_table_new (g_direct_hash, NULL);
      display_x11->atom_to_virtual = g_hash_table_new (g_direct_hash, NULL);
    }
  
  g_hash_table_insert (display_x11->atom_from_virtual, 
		       CDK_ATOM_TO_POINTER (virtual_atom), 
		       GUINT_TO_POINTER (xatom));
  g_hash_table_insert (display_x11->atom_to_virtual,
		       GUINT_TO_POINTER (xatom), 
		       CDK_ATOM_TO_POINTER (virtual_atom));
}

static Atom
lookup_cached_xatom (CdkDisplay *display,
		     CdkAtom     atom)
{
  CdkX11Display *display_x11 = CDK_X11_DISPLAY (display);

  if (ATOM_TO_INDEX (atom) < N_PREDEFINED_ATOMS)
    return ATOM_TO_INDEX (atom);
  
  if (display_x11->atom_from_virtual)
    return GPOINTER_TO_UINT (g_hash_table_lookup (display_x11->atom_from_virtual,
						  CDK_ATOM_TO_POINTER (atom)));

  return None;
}

/**
 * cdk_x11_atom_to_xatom_for_display:
 * @display: (type CdkX11Display): A #CdkDisplay
 * @atom: A #CdkAtom, or %CDK_NONE
 *
 * Converts from a #CdkAtom to the X atom for a #CdkDisplay
 * with the same string value. The special value %CDK_NONE
 * is converted to %None.
 *
 * Returns: the X atom corresponding to @atom, or %None
 *
 * Since: 2.2
 **/
Atom
cdk_x11_atom_to_xatom_for_display (CdkDisplay *display,
				   CdkAtom     atom)
{
  Atom xatom = None;

  g_return_val_if_fail (CDK_IS_DISPLAY (display), None);

  if (atom == CDK_NONE)
    return None;

  if (cdk_display_is_closed (display))
    return None;

  xatom = lookup_cached_xatom (display, atom);

  if (!xatom)
    {
      char *name = cdk_atom_name (atom);

      xatom = XInternAtom (CDK_DISPLAY_XDISPLAY (display), name, FALSE);
      insert_atom_pair (display, atom, xatom);

      g_free (name);
    }

  return xatom;
}

void
_cdk_x11_precache_atoms (CdkDisplay          *display,
			 const gchar * const *atom_names,
			 gint                 n_atoms)
{
  Atom *xatoms;
  CdkAtom *atoms;
  const gchar **xatom_names;
  gint n_xatoms;
  gint i;

  xatoms = g_new (Atom, n_atoms);
  xatom_names = g_new (const gchar *, n_atoms);
  atoms = g_new (CdkAtom, n_atoms);

  n_xatoms = 0;
  for (i = 0; i < n_atoms; i++)
    {
      CdkAtom atom = cdk_atom_intern_static_string (atom_names[i]);
      if (lookup_cached_xatom (display, atom) == None)
	{
	  atoms[n_xatoms] = atom;
	  xatom_names[n_xatoms] = atom_names[i];
	  n_xatoms++;
	}
    }

  if (n_xatoms)
    XInternAtoms (CDK_DISPLAY_XDISPLAY (display),
                  (char **)xatom_names, n_xatoms, False, xatoms);

  for (i = 0; i < n_xatoms; i++)
    insert_atom_pair (display, atoms[i], xatoms[i]);

  g_free (xatoms);
  g_free (xatom_names);
  g_free (atoms);
}

/**
 * cdk_x11_atom_to_xatom:
 * @atom: A #CdkAtom 
 * 
 * Converts from a #CdkAtom to the X atom for the default CDK display
 * with the same string value.
 * 
 * Returns: the X atom corresponding to @atom.
 **/
Atom
cdk_x11_atom_to_xatom (CdkAtom atom)
{
  return cdk_x11_atom_to_xatom_for_display (cdk_display_get_default (), atom);
}

/**
 * cdk_x11_xatom_to_atom_for_display:
 * @display: (type CdkX11Display): A #CdkDisplay
 * @xatom: an X atom 
 * 
 * Convert from an X atom for a #CdkDisplay to the corresponding
 * #CdkAtom.
 * 
 * Returns: (transfer none): the corresponding #CdkAtom.
 *
 * Since: 2.2
 **/
CdkAtom
cdk_x11_xatom_to_atom_for_display (CdkDisplay *display,
				   Atom	       xatom)
{
  CdkX11Display *display_x11;
  CdkAtom virtual_atom = CDK_NONE;
  
  g_return_val_if_fail (CDK_IS_DISPLAY (display), CDK_NONE);

  if (xatom == None)
    return CDK_NONE;

  if (cdk_display_is_closed (display))
    return CDK_NONE;

  display_x11 = CDK_X11_DISPLAY (display);
  
  if (xatom < N_PREDEFINED_ATOMS)
    return INDEX_TO_ATOM (xatom);
  
  if (display_x11->atom_to_virtual)
    virtual_atom = CDK_POINTER_TO_ATOM (g_hash_table_lookup (display_x11->atom_to_virtual,
							     GUINT_TO_POINTER (xatom)));
  
  if (!virtual_atom)
    {
      /* If this atom doesn't exist, we'll die with an X error unless
       * we take precautions
       */
      char *name;
      cdk_x11_display_error_trap_push (display);
      name = XGetAtomName (CDK_DISPLAY_XDISPLAY (display), xatom);
      if (cdk_x11_display_error_trap_pop (display))
	{
	  g_warning (G_STRLOC " invalid X atom: %ld", xatom);
	}
      else
	{
	  virtual_atom = cdk_atom_intern (name, FALSE);
	  XFree (name);
	  
	  insert_atom_pair (display, virtual_atom, xatom);
	}
    }

  return virtual_atom;
}

/**
 * cdk_x11_xatom_to_atom:
 * @xatom: an X atom for the default CDK display
 * 
 * Convert from an X atom for the default display to the corresponding
 * #CdkAtom.
 * 
 * Returns: (transfer none): the corresponding G#dkAtom.
 **/
CdkAtom
cdk_x11_xatom_to_atom (Atom xatom)
{
  return cdk_x11_xatom_to_atom_for_display (cdk_display_get_default (), xatom);
}

/**
 * cdk_x11_get_xatom_by_name_for_display:
 * @display: (type CdkX11Display): a #CdkDisplay
 * @atom_name: a string
 * 
 * Returns the X atom for a #CdkDisplay corresponding to @atom_name.
 * This function caches the result, so if called repeatedly it is much
 * faster than XInternAtom(), which is a round trip to the server each time.
 * 
 * Returns: a X atom for a #CdkDisplay
 *
 * Since: 2.2
 **/
Atom
cdk_x11_get_xatom_by_name_for_display (CdkDisplay  *display,
				       const gchar *atom_name)
{
  g_return_val_if_fail (CDK_IS_DISPLAY (display), None);
  return cdk_x11_atom_to_xatom_for_display (display,
					    cdk_atom_intern (atom_name, FALSE));
}

Atom
_cdk_x11_get_xatom_for_display_printf (CdkDisplay    *display,
                                       const gchar   *format,
                                       ...)
{
  va_list args;
  char *atom_name;
  Atom atom;

  va_start (args, format);
  atom_name = g_strdup_vprintf (format, args);
  va_end (args);

  atom = cdk_x11_get_xatom_by_name_for_display (display, atom_name);

  g_free (atom_name);

  return atom;
}

/**
 * cdk_x11_get_xatom_by_name:
 * @atom_name: a string
 * 
 * Returns the X atom for CDK’s default display corresponding to @atom_name.
 * This function caches the result, so if called repeatedly it is much
 * faster than XInternAtom(), which is a round trip to the server each time.
 * 
 * Returns: a X atom for CDK’s default display.
 **/
Atom
cdk_x11_get_xatom_by_name (const gchar *atom_name)
{
  return cdk_x11_get_xatom_by_name_for_display (cdk_display_get_default (),
						atom_name);
}

/**
 * cdk_x11_get_xatom_name_for_display:
 * @display: (type CdkX11Display): the #CdkDisplay where @xatom is defined
 * @xatom: an X atom 
 * 
 * Returns the name of an X atom for its display. This
 * function is meant mainly for debugging, so for convenience, unlike
 * XAtomName() and cdk_atom_name(), the result doesn’t need to
 * be freed. 
 *
 * Returns: name of the X atom; this string is owned by CDK,
 *   so it shouldn’t be modifed or freed. 
 *
 * Since: 2.2
 **/
const gchar *
cdk_x11_get_xatom_name_for_display (CdkDisplay *display,
				    Atom        xatom)
{
  g_return_val_if_fail (CDK_IS_DISPLAY (display), NULL);

  return _cdk_atom_name_const (cdk_x11_xatom_to_atom_for_display (display, xatom));
}

/**
 * cdk_x11_get_xatom_name:
 * @xatom: an X atom for CDK’s default display
 * 
 * Returns the name of an X atom for CDK’s default display. This
 * function is meant mainly for debugging, so for convenience, unlike
 * XAtomName() and cdk_atom_name(), the result 
 * doesn’t need to be freed. Also, this function will never return %NULL, 
 * even if @xatom is invalid.
 * 
 * Returns: name of the X atom; this string is owned by CTK+,
 *   so it shouldn’t be modifed or freed. 
 **/
const gchar *
cdk_x11_get_xatom_name (Atom xatom)
{
  return _cdk_atom_name_const (cdk_x11_xatom_to_atom (xatom));
}

gboolean
_cdk_x11_window_get_property (CdkWindow   *window,
                              CdkAtom      property,
                              CdkAtom      type,
                              gulong       offset,
                              gulong       length,
                              gint         pdelete,
                              CdkAtom     *actual_property_type,
                              gint        *actual_format_type,
                              gint        *actual_length,
                              guchar     **data)
{
  CdkDisplay *display;
  Atom ret_prop_type;
  gint ret_format;
  gulong ret_nitems;
  gulong ret_bytes_after;
  gulong get_length;
  gulong ret_length;
  guchar *ret_data;
  Atom xproperty;
  Atom xtype;
  int res;

  g_return_val_if_fail (!window || CDK_WINDOW_IS_X11 (window), FALSE);

  if (!window)
    {
      CdkScreen *screen = cdk_screen_get_default ();
      window = cdk_screen_get_root_window (screen);
    }
  else if (!CDK_WINDOW_IS_X11 (window))
    return FALSE;

  if (CDK_WINDOW_DESTROYED (window))
    return FALSE;

  display = cdk_window_get_display (window);
  xproperty = cdk_x11_atom_to_xatom_for_display (display, property);
  if (type == CDK_NONE)
    xtype = AnyPropertyType;
  else
    xtype = cdk_x11_atom_to_xatom_for_display (display, type);

  ret_data = NULL;
  
  /* 
   * Round up length to next 4 byte value.  Some code is in the (bad?)
   * habit of passing G_MAXLONG as the length argument, causing an
   * overflow to negative on the add.  In this case, we clamp the
   * value to G_MAXLONG.
   */
  get_length = length + 3;
  if (get_length > G_MAXLONG)
    get_length = G_MAXLONG;

  /* To fail, either the user passed 0 or G_MAXULONG */
  get_length = get_length / 4;
  if (get_length == 0)
    {
      g_warning ("cdk_propery-get(): invalid length 0");
      return FALSE;
    }

  res = XGetWindowProperty (CDK_DISPLAY_XDISPLAY (display),
			    CDK_WINDOW_XID (window), xproperty,
			    offset, get_length, pdelete,
			    xtype, &ret_prop_type, &ret_format,
			    &ret_nitems, &ret_bytes_after,
			    &ret_data);

  if (res != Success || (ret_prop_type == None && ret_format == 0))
    {
      return FALSE;
    }

  if (actual_property_type)
    *actual_property_type = cdk_x11_xatom_to_atom_for_display (display, ret_prop_type);
  if (actual_format_type)
    *actual_format_type = ret_format;

  if ((xtype != AnyPropertyType) && (ret_prop_type != xtype))
    {
      XFree (ret_data);
      g_warning ("Couldn't match property type %s to %s\n", 
		 cdk_x11_get_xatom_name_for_display (display, ret_prop_type), 
		 cdk_x11_get_xatom_name_for_display (display, xtype));
      return FALSE;
    }

  /* FIXME: ignoring bytes_after could have very bad effects */

  if (data)
    {
      if (ret_prop_type == XA_ATOM ||
	  ret_prop_type == cdk_x11_get_xatom_by_name_for_display (display, "ATOM_PAIR"))
	{
	  /*
	   * data is an array of X atom, we need to convert it
	   * to an array of CDK Atoms
	   */
	  gint i;
	  CdkAtom *ret_atoms = g_new (CdkAtom, ret_nitems);
	  Atom *xatoms = (Atom *)ret_data;

	  *data = (guchar *)ret_atoms;

	  for (i = 0; i < ret_nitems; i++)
	    ret_atoms[i] = cdk_x11_xatom_to_atom_for_display (display, xatoms[i]);
	  
	  if (actual_length)
	    *actual_length = ret_nitems * sizeof (CdkAtom);
	}
      else
	{
	  switch (ret_format)
	    {
	    case 8:
	      ret_length = ret_nitems;
	      break;
	    case 16:
	      ret_length = sizeof(short) * ret_nitems;
	      break;
	    case 32:
	      ret_length = sizeof(long) * ret_nitems;
	      break;
	    default:
	      g_warning ("unknown property return format: %d", ret_format);
	      XFree (ret_data);
	      return FALSE;
	    }
	  
	  *data = g_new (guchar, ret_length);
	  memcpy (*data, ret_data, ret_length);
	  if (actual_length)
	    *actual_length = ret_length;
	}
    }

  XFree (ret_data);

  return TRUE;
}

void
_cdk_x11_window_change_property (CdkWindow    *window,
                                 CdkAtom       property,
                                 CdkAtom       type,
                                 gint          format,
                                 CdkPropMode   mode,
                                 const guchar *data,
                                 gint          nelements)
{
  CdkDisplay *display;
  Window xwindow;
  Atom xproperty;
  Atom xtype;

  g_return_if_fail (!window || CDK_WINDOW_IS_X11 (window));

  if (!window)
    {
      CdkScreen *screen;
      
      screen = cdk_screen_get_default ();
      window = cdk_screen_get_root_window (screen);
    }
  else if (!CDK_WINDOW_IS_X11 (window))
    return;

  if (CDK_WINDOW_DESTROYED (window))
    return;

  cdk_window_ensure_native (window);

  display = cdk_window_get_display (window);
  xproperty = cdk_x11_atom_to_xatom_for_display (display, property);
  xtype = cdk_x11_atom_to_xatom_for_display (display, type);
  xwindow = CDK_WINDOW_XID (window);

  if (xtype == XA_ATOM ||
      xtype == cdk_x11_get_xatom_by_name_for_display (display, "ATOM_PAIR"))
    {
      /*
       * data is an array of CdkAtom, we need to convert it
       * to an array of X Atoms
       */
      gint i;
      CdkAtom *atoms = (CdkAtom*) data;
      Atom *xatoms;

      xatoms = g_new (Atom, nelements);
      for (i = 0; i < nelements; i++)
	xatoms[i] = cdk_x11_atom_to_xatom_for_display (display, atoms[i]);

      XChangeProperty (CDK_DISPLAY_XDISPLAY (display), xwindow,
		       xproperty, xtype,
		       format, mode, (guchar *)xatoms, nelements);
      g_free (xatoms);
    }
  else
    XChangeProperty (CDK_DISPLAY_XDISPLAY (display), xwindow, xproperty, 
		     xtype, format, mode, (guchar *)data, nelements);
}

void
_cdk_x11_window_delete_property (CdkWindow *window,
                                 CdkAtom    property)
{
  g_return_if_fail (!window || CDK_WINDOW_IS_X11 (window));

  if (!window)
    {
      CdkScreen *screen = cdk_screen_get_default ();
      window = cdk_screen_get_root_window (screen);
    }
  else if (!CDK_WINDOW_IS_X11 (window))
    return;

  if (CDK_WINDOW_DESTROYED (window))
    return;

  XDeleteProperty (CDK_WINDOW_XDISPLAY (window), CDK_WINDOW_XID (window),
		   cdk_x11_atom_to_xatom_for_display (CDK_WINDOW_DISPLAY (window),
						      property));
}
