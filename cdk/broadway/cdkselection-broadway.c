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

#include "cdkselection.h"

#include "cdkproperty.h"
#include "cdkprivate.h"
#include "cdkprivate-broadway.h"
#include "cdkdisplay-broadway.h"

#include <string.h>


typedef struct _OwnerInfo OwnerInfo;

struct _OwnerInfo
{
  CdkAtom    selection;
  CdkWindow *owner;
  gulong     serial;
};

static GSList *owner_list;

/* When a window is destroyed we check if it is the owner
 * of any selections. This is somewhat inefficient, but
 * owner_list is typically short, and it is a low memory,
 * low code solution
 */
void
_cdk_broadway_selection_window_destroyed (CdkWindow *window)
{
  GSList *tmp_list = owner_list;
  while (tmp_list)
    {
      OwnerInfo *info = tmp_list->data;
      tmp_list = tmp_list->next;

      if (info->owner == window)
	{
	  owner_list = g_slist_remove (owner_list, info);
	  g_free (info);
	}
    }
}

gboolean
_cdk_broadway_display_set_selection_owner (CdkDisplay *display,
					   CdkWindow  *owner,
					   CdkAtom     selection,
					   guint32     time G_GNUC_UNUSED,
					   gboolean    send_event G_GNUC_UNUSED)
{
  GSList *tmp_list;
  OwnerInfo *info;

  if (cdk_display_is_closed (display))
    return FALSE;

  tmp_list = owner_list;
  while (tmp_list)
    {
      info = tmp_list->data;
      if (info->selection == selection)
        {
          owner_list = g_slist_remove (owner_list, info);
          g_free (info);
          break;
        }
      tmp_list = tmp_list->next;
    }

  if (owner)
    {
      info = g_new (OwnerInfo, 1);
      info->owner = owner;
      info->serial = _cdk_display_get_next_serial (display);

      info->selection = selection;

      owner_list = g_slist_prepend (owner_list, info);
    }

  return TRUE;
}

CdkWindow *
_cdk_broadway_display_get_selection_owner (CdkDisplay *display,
					   CdkAtom     selection)
{
  GSList *tmp_list;

  if (cdk_display_is_closed (display))
    return NULL;

  tmp_list = owner_list;
  while (tmp_list)
    {
      OwnerInfo *info;

      info = tmp_list->data;
      if (info->selection == selection)
	return info->owner;
      tmp_list = tmp_list->next;
    }

  return NULL;
}

void
_cdk_broadway_display_convert_selection (CdkDisplay *display G_GNUC_UNUSED,
					 CdkWindow *requestor G_GNUC_UNUSED,
					 CdkAtom    selection G_GNUC_UNUSED,
					 CdkAtom    target G_GNUC_UNUSED,
					 guint32    time G_GNUC_UNUSED)
{
  g_warning ("convert_selection not implemented");
}

gint
_cdk_broadway_display_get_selection_property (CdkDisplay *display G_GNUC_UNUSED,
					      CdkWindow  *requestor G_GNUC_UNUSED,
					      guchar    **data,
					      CdkAtom    *ret_type,
					      gint       *ret_format)
{
  if (ret_type)
    *ret_type = CDK_NONE;
  if (ret_format)
    *ret_format = 0;
  if (data)
    *data = NULL;

  g_warning ("get_selection_property not implemented");

  return 0;
}

void
_cdk_broadway_display_send_selection_notify (CdkDisplay *display,
					     CdkWindow  *requestor G_GNUC_UNUSED,
					     CdkAtom     selection G_GNUC_UNUSED,
					     CdkAtom     target G_GNUC_UNUSED,
					     CdkAtom     property G_GNUC_UNUSED,
					     guint32     time G_GNUC_UNUSED)
{
  g_return_if_fail (CDK_IS_DISPLAY (display));

  g_warning ("send_selection_notify not implemented");
}


static gint
make_list (const gchar  *text,
           gint          length,
           gboolean      latin1,
           gchar      ***list)
{
  GSList *strings = NULL;
  gint n_strings = 0;
  gint i;
  const gchar *p = text;
  const gchar *q;
  GSList *tmp_list;
  GError *error = NULL;

  while (p < text + length)
    {
      gchar *str;

      q = p;
      while (*q && q < text + length)
        q++;

      if (latin1)
        {
          str = g_convert (p, q - p,
                           "UTF-8", "ISO-8859-1",
                           NULL, NULL, &error);

          if (!str)
            {
              g_warning ("Error converting selection from STRING: %s",
                         error->message);
              g_error_free (error);
            }
        }
      else
        {
          str = g_strndup (p, q - p);
          if (!g_utf8_validate (str, -1, NULL))
            {
              g_warning ("Error converting selection from UTF8_STRING");
              g_free (str);
              str = NULL;
            }
        }

      if (str)
        {
          strings = g_slist_prepend (strings, str);
          n_strings++;
        }

      p = q + 1;
    }

  if (list)
    {
      *list = g_new (gchar *, n_strings + 1);
      (*list)[n_strings] = NULL;
    }

  i = n_strings;
  tmp_list = strings;
  while (tmp_list)
    {
      if (list)
        (*list)[--i] = tmp_list->data;
      else
        g_free (tmp_list->data);

      tmp_list = tmp_list->next;
    }

  g_slist_free (strings);

  return n_strings;
}

gint 
_cdk_broadway_display_text_property_to_utf8_list (CdkDisplay    *display,
						  CdkAtom        encoding,
						  gint           format G_GNUC_UNUSED,
						  const guchar  *text,
						  gint           length,
						  gchar       ***list)
{
  g_return_val_if_fail (text != NULL, 0);
  g_return_val_if_fail (length >= 0, 0);
  g_return_val_if_fail (CDK_IS_DISPLAY (display), 0);

  if (encoding == CDK_TARGET_STRING)
    {
      return make_list ((gchar *)text, length, TRUE, list);
    }
  else if (encoding == cdk_atom_intern_static_string ("UTF8_STRING"))
    {
      return make_list ((gchar *)text, length, FALSE, list);
    }
  
  if (list)
    *list = NULL;
  return 0;
}

gchar *
_cdk_broadway_display_utf8_to_string_target (CdkDisplay  *display G_GNUC_UNUSED,
					     const gchar *str)
{
  return g_strdup (str);
}
