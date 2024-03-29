/* CTK+ - accessibility implementations
 * Copyright 2001 Sun Microsystems Inc.
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
#include <ctk/ctk.h>
#include "ctkimageaccessible.h"
#include "ctktoolbarprivate.h"
#include "ctkintl.h"

struct _CtkImageAccessiblePrivate
{
  gchar *image_description;
  gchar *stock_name;
};

static void atk_image_interface_init (AtkImageIface  *iface);

G_DEFINE_TYPE_WITH_CODE (CtkImageAccessible, ctk_image_accessible, CTK_TYPE_WIDGET_ACCESSIBLE,
                         G_ADD_PRIVATE (CtkImageAccessible)
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_IMAGE, atk_image_interface_init))

static void
ctk_image_accessible_initialize (AtkObject *accessible,
                                 gpointer   data)
{
  ATK_OBJECT_CLASS (ctk_image_accessible_parent_class)->initialize (accessible, data);

  accessible->role = ATK_ROLE_ICON;
}

typedef struct {
  const gchar *name;
  const gchar *label;
} NameMapEntry;

static const NameMapEntry name_map[] = {
  { "help-about", NC_("Stock label", "_About") },
  { "list-add", NC_("Stock label", "_Add") },
  { "format-text-bold", NC_("Stock label", "_Bold") },
  { "media-optical", NC_("Stock label", "_CD-ROM") },
  { "edit-clear", NC_("Stock label", "_Clear") },
  { "window-close", NC_("Stock label", "_Close") },
  { "window-minimize", N_("Minimize") },
  { "window-maximize", N_("Maximize") },
  { "window-restore", N_("Restore") },
  { "edit-copy", NC_("Stock label", "_Copy") },
  { "edit-cut", NC_("Stock label", "Cu_t") },
  { "edit-delete", NC_("Stock label", "_Delete") },
  { "dialog-error", NC_("Stock label", "Error") },
  { "dialog-information", NC_("Stock label", "Information") },
  { "dialog-question", NC_("Stock label", "Question") },
  { "dialog-warning", NC_("Stock label", "Warning") },
  { "system-run", NC_("Stock label", "_Execute") },
  { "text-x-generic", NC_("Stock label", "_File") },
  { "edit-find", NC_("Stock label", "_Find") },
  { "edit-find-replace", NC_("Stock label", "Find and _Replace") },
  { "media-floppy", NC_("Stock label", "_Floppy") },
  { "view-fullscreen", NC_("Stock label", "_Fullscreen") },
  { "go-bottom", NC_("Stock label, navigation", "_Bottom") },
  { "go-first", NC_("Stock label, navigation", "_First") },
  { "go-last", NC_("Stock label, navigation", "_Last") },
  { "go-top", NC_("Stock label, navigation", "_Top") },
  { "go-previous", NC_("Stock label, navigation", "_Back") },
  { "go-down", NC_("Stock label, navigation", "_Down") },
  { "go-next", NC_("Stock label, navigation", "_Forward") },
  { "go-up", NC_("Stock label, navigation", "_Up") },
  { "drive-harddisk", NC_("Stock label", "_Hard Disk") },
  { "help-contents", NC_("Stock label", "_Help") },
  { "go-home", NC_("Stock label", "_Home") },
  { "format-indent-more", NC_("Stock label", "Increase Indent") },
  { "format-text-italic", NC_("Stock label", "_Italic") },
  { "go-jump", NC_("Stock label", "_Jump to") },
  { "format-justify-center", NC_("Stock label", "_Center") },
  { "format-justify-fill", NC_("Stock label", "_Fill") },
  { "format-justify-left", NC_("Stock label", "_Left") },
  { "format-justify-right", NC_("Stock label", "_Right") },
  { "view-restore", NC_("Stock label", "_Leave Fullscreen") },
  { "media-seek-forward", NC_("Stock label, media", "_Forward") },
  { "media-skip-forward", NC_("Stock label, media", "_Next") },
  { "media-playback-pause", NC_("Stock label, media", "P_ause") },
  { "media-playback-start", NC_("Stock label, media", "_Play") },
  { "media-skip-backward", NC_("Stock label, media", "Pre_vious") },
  { "media-record", NC_("Stock label, media", "_Record") },
  { "media-seek-backward", NC_("Stock label, media", "R_ewind") },
  { "media-playback-stop", NC_("Stock label, media", "_Stop") },
  { "network-idle", NC_("Stock label", "_Network") },
  { "document-new", NC_("Stock label", "_New") },
  { "document-open", NC_("Stock label", "_Open") },
  { "edit-paste", NC_("Stock label", "_Paste") },
  { "document-print", NC_("Stock label", "_Print") },
  { "document-print-preview", NC_("Stock label", "Print Pre_view") },
  { "document-properties", NC_("Stock label", "_Properties") },
  { "application-exit", NC_("Stock label", "_Quit") },
  { "edit-redo", NC_("Stock label", "_Redo") },
  { "view-refresh", NC_("Stock label", "_Refresh") },
  { "list-remove", NC_("Stock label", "_Remove") },
  { "document-revert", NC_("Stock label", "_Revert") },
  { "document-save", NC_("Stock label", "_Save") },
  { "document-save-as", NC_("Stock label", "Save _As") },
  { "edit-select-all", NC_("Stock label", "Select _All") },
  { "view-sort-ascending", NC_("Stock label", "_Ascending") },
  { "view-sort-descending", NC_("Stock label", "_Descending") },
  { "tools-check-spelling", NC_("Stock label", "_Spell Check") },
  { "process-stop", NC_("Stock label", "_Stop") },
  { "format-text-strikethrough", NC_("Stock label", "_Strikethrough") },
  { "format-text-underline", NC_("Stock label", "_Underline") },
  { "edit-undo", NC_("Stock label", "_Undo") },
  { "format-indent-less", NC_("Stock label", "Decrease Indent") },
  { "zoom-original", NC_("Stock label", "_Normal Size") },
  { "zoom-fit-best", NC_("Stock label", "Best _Fit") },
  { "zoom-in", NC_("Stock label", "Zoom _In") },
  { "zoom-out", NC_("Stock label", "Zoom _Out") }
};

static gchar *
name_from_icon_name (const gchar *icon_name)
{
  gchar *name;
  const gchar *label;
  gint i;

  name = g_strdup (icon_name);
  if (g_str_has_suffix (name, "-symbolic"))
    name[strlen (name) - strlen ("-symbolic")] = '\0';

  for (i = 0; i < G_N_ELEMENTS (name_map); i++)
    {
      if (g_str_equal (name, name_map[i].name))
        {
          label = g_dpgettext2 (GETTEXT_PACKAGE, "Stock label", name_map[i].label);
          g_free (name);

          return _ctk_toolbar_elide_underscores (label);
        }
    }

  g_free (name);
  return NULL;
}

static void
ctk_image_accessible_finalize (GObject *object)
{
  CtkImageAccessible *aimage = CTK_IMAGE_ACCESSIBLE (object);

  g_free (aimage->priv->image_description);
  g_free (aimage->priv->stock_name);

  G_OBJECT_CLASS (ctk_image_accessible_parent_class)->finalize (object);
}

static const gchar *
ctk_image_accessible_get_name (AtkObject *accessible)
{
  CtkWidget* widget;
  CtkImage *image;
  CtkImageAccessible *image_accessible;
  CtkStockItem stock_item;
  gchar *stock_id;
  const gchar *name;
  CtkImageType storage_type;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (accessible));
  if (widget == NULL)
    return NULL;

  name = ATK_OBJECT_CLASS (ctk_image_accessible_parent_class)->get_name (accessible);
  if (name)
    return name;

  image = CTK_IMAGE (widget);
  image_accessible = CTK_IMAGE_ACCESSIBLE (accessible);

  g_free (image_accessible->priv->stock_name);
  image_accessible->priv->stock_name = NULL;

  storage_type = ctk_image_get_storage_type (image);
  if (storage_type == CTK_IMAGE_STOCK)
    {
      ctk_image_get_stock (image, &stock_id, NULL);
      if (stock_id == NULL)
        return NULL;

      if (!ctk_stock_lookup (stock_id, &stock_item))
        return NULL;

      image_accessible->priv->stock_name = _ctk_toolbar_elide_underscores (stock_item.label);
    }
  else if (storage_type == CTK_IMAGE_ICON_NAME)
    {
      const gchar *icon_name;

      ctk_image_get_icon_name (image, &icon_name, NULL);
      image_accessible->priv->stock_name = name_from_icon_name (icon_name);
    }
  else if (storage_type == CTK_IMAGE_GICON)
    {
      GIcon *icon;

      ctk_image_get_gicon (image, &icon, NULL);
      if (G_IS_THEMED_ICON (icon))
        {
          const gchar * const *icon_names;

          icon_names = g_themed_icon_get_names (G_THEMED_ICON (icon));
          image_accessible->priv->stock_name = name_from_icon_name (icon_names[0]);
        }
    }

  return image_accessible->priv->stock_name;
}

static void
ctk_image_accessible_class_init (CtkImageAccessibleClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  AtkObjectClass  *class = ATK_OBJECT_CLASS (klass);

  gobject_class->finalize = ctk_image_accessible_finalize;
  class->initialize = ctk_image_accessible_initialize;
  class->get_name = ctk_image_accessible_get_name;
}

static void
ctk_image_accessible_init (CtkImageAccessible *image)
{
  image->priv = ctk_image_accessible_get_instance_private (image);
}

static const gchar *
ctk_image_accessible_get_image_description (AtkImage *image)
{
  CtkImageAccessible *accessible = CTK_IMAGE_ACCESSIBLE (image);

  return accessible->priv->image_description;
}

static void
ctk_image_accessible_get_image_position (AtkImage     *image,
                                         gint         *x,
                                         gint         *y,
                                         AtkCoordType  coord_type)
{
  atk_component_get_extents (ATK_COMPONENT (image), x, y, NULL, NULL,
                             coord_type);
}

static void
ctk_image_accessible_get_image_size (AtkImage *image,
                                     gint     *width,
                                     gint     *height)
{
  CtkWidget* widget;
  CtkImage *ctk_image;
  CtkImageType image_type;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (image));
  if (widget == NULL)
    {
      *height = -1;
      *width = -1;
      return;
    }

  ctk_image = CTK_IMAGE (widget);

  image_type = ctk_image_get_storage_type (ctk_image);
  switch (image_type)
    {
    case CTK_IMAGE_PIXBUF:
      {
        GdkPixbuf *pixbuf;

        pixbuf = ctk_image_get_pixbuf (ctk_image);
        *height = gdk_pixbuf_get_height (pixbuf);
        *width = gdk_pixbuf_get_width (pixbuf);
        break;
      }
    case CTK_IMAGE_STOCK:
    case CTK_IMAGE_ICON_SET:
    case CTK_IMAGE_ICON_NAME:
    case CTK_IMAGE_GICON:
      {
        CtkIconSize size;

        g_object_get (ctk_image, "icon-size", &size, NULL);
        ctk_icon_size_lookup (size, width, height);
        break;
      }
    case CTK_IMAGE_ANIMATION:
      {
        GdkPixbufAnimation *animation;

        animation = ctk_image_get_animation (ctk_image);
        *height = gdk_pixbuf_animation_get_height (animation);
        *width = gdk_pixbuf_animation_get_width (animation);
        break;
      }
    default:
      {
        *height = -1;
        *width = -1;
        break;
      }
    }
}

static gboolean
ctk_image_accessible_set_image_description (AtkImage    *image,
                                            const gchar *description)
{
  CtkImageAccessible* accessible = CTK_IMAGE_ACCESSIBLE (image);

  g_free (accessible->priv->image_description);
  accessible->priv->image_description = g_strdup (description);

  return TRUE;
}

static void
atk_image_interface_init (AtkImageIface *iface)
{
  iface->get_image_description = ctk_image_accessible_get_image_description;
  iface->get_image_position = ctk_image_accessible_get_image_position;
  iface->get_image_size = ctk_image_accessible_get_image_size;
  iface->set_image_description = ctk_image_accessible_set_image_description;
}
