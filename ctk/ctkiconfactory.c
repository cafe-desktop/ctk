/* CTK - The GIMP Toolkit
 * Copyright (C) 2000 Red Hat, Inc.
 *               2008 Johan Dahlin
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

#include "config.h"

#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "ctkcssenumvalueprivate.h"
#include "ctkcssiconthemevalueprivate.h"
#include "ctkiconfactory.h"
#include "ctkiconcache.h"
#include "ctkdebug.h"
#include "ctkicontheme.h"
#include "ctksettingsprivate.h"
#include "ctkstock.h"
#include "ctkwidget.h"
#include "ctkintl.h"
#include "ctkbuildable.h"
#include "ctkbuilderprivate.h"
#include "ctktypebuiltins.h"
#include "ctkstyle.h"
#include "ctkstylecontextprivate.h"
#include "ctkrender.h"
#include "ctkrenderprivate.h"

/**
 * SECTION:ctkiconfactory
 * @Short_description: Manipulating stock icons
 * @Title: Themeable Stock Images
 *
 * An icon factory manages a collection of #CtkIconSet; a #CtkIconSet manages a
 * set of variants of a particular icon (i.e. a #CtkIconSet contains variants for
 * different sizes and widget states). Icons in an icon factory are named by a
 * stock ID, which is a simple string identifying the icon. Each #CtkStyle has a
 * list of #CtkIconFactory derived from the current theme; those icon factories
 * are consulted first when searching for an icon. If the theme doesn’t set a
 * particular icon, CTK+ looks for the icon in a list of default icon factories,
 * maintained by ctk_icon_factory_add_default() and
 * ctk_icon_factory_remove_default(). Applications with icons should add a default
 * icon factory with their icons, which will allow themes to override the icons
 * for the application.
 *
 * To display an icon, always use ctk_style_lookup_icon_set() on the widget that
 * will display the icon, or the convenience function
 * ctk_widget_render_icon(). These functions take the theme into account when
 * looking up the icon to use for a given stock ID.
 *
 * # CtkIconFactory as CtkBuildable # {#CtkIconFactory-BUILDER-UI}
 *
 * CtkIconFactory supports a custom <sources> element, which can contain
 * multiple <source> elements. The following attributes are allowed:
 *
 * - stock-id
 *
 *     The stock id of the source, a string. This attribute is
 *     mandatory
 *
 * - filename
 *
 *     The filename of the source, a string.  This attribute is
 *     optional
 *
 * - icon-name
 *
 *     The icon name for the source, a string.  This attribute is
 *     optional.
 *
 * - size
 *
 *     Size of the icon, a #CtkIconSize enum value.  This attribute is
 *     optional.
 *
 * - direction
 *
 *     Direction of the source, a #CtkTextDirection enum value.  This
 *     attribute is optional.
 *
 * - state
 *
 *     State of the source, a #CtkStateType enum value.  This
 *     attribute is optional.
 *
 *
 * ## A #CtkIconFactory UI definition fragment. ##
 *
 * |[
 * <object class="CtkIconFactory" id="iconfactory1">
 *   <sources>
 *     <source stock-id="apple-red" filename="apple-red.png"/>
 *   </sources>
 * </object>
 * <object class="CtkWindow" id="window1">
 *   <child>
 *     <object class="CtkButton" id="apple_button">
 *       <property name="label">apple-red</property>
 *       <property name="use-stock">True</property>
 *     </object>
 *   </child>
 * </object>
 * ]|
 */


static GSList *all_icon_factories = NULL;

struct _CtkIconFactoryPrivate
{
  GHashTable *icons;
};

typedef enum {
  CTK_ICON_SOURCE_EMPTY,
  CTK_ICON_SOURCE_ICON_NAME,
  CTK_ICON_SOURCE_STATIC_ICON_NAME,
  CTK_ICON_SOURCE_FILENAME,
  CTK_ICON_SOURCE_PIXBUF
} CtkIconSourceType;

struct _CtkIconSource
{
  CtkIconSourceType type;

  union {
    gchar *icon_name;
    gchar *filename;
    GdkPixbuf *pixbuf;
  } source;

  GdkPixbuf *filename_pixbuf;

  CtkTextDirection direction;
  CtkStateType state;
  CtkIconSize size;

  /* If TRUE, then the parameter is wildcarded, and the above
   * fields should be ignored. If FALSE, the parameter is
   * specified, and the above fields should be valid.
   */
  guint any_direction : 1;
  guint any_state : 1;
  guint any_size : 1;
};


static void
ctk_icon_factory_buildable_init  (CtkBuildableIface      *iface);

static gboolean ctk_icon_factory_buildable_custom_tag_start (CtkBuildable     *buildable,
							     CtkBuilder       *builder,
							     GObject          *child,
							     const gchar      *tagname,
							     GMarkupParser    *parser,
							     gpointer         *data);
static void ctk_icon_factory_buildable_custom_tag_end (CtkBuildable *buildable,
						       CtkBuilder   *builder,
						       GObject      *child,
						       const gchar  *tagname,
						       gpointer     *user_data);
static void ctk_icon_factory_finalize   (GObject             *object);
static void get_default_icons           (CtkIconFactory      *icon_factory);
static void icon_source_clear           (CtkIconSource       *source);

static CtkIconSize icon_size_register_intern (const gchar *name,
					      gint         width,
					      gint         height);

#define CTK_ICON_SOURCE_INIT(any_direction, any_state, any_size)	\
  { CTK_ICON_SOURCE_EMPTY, { NULL }, NULL,				\
   0, 0, 0,								\
   any_direction, any_state, any_size }

G_DEFINE_TYPE_WITH_CODE (CtkIconFactory, ctk_icon_factory, G_TYPE_OBJECT,
                         G_ADD_PRIVATE (CtkIconFactory)
			 G_IMPLEMENT_INTERFACE (CTK_TYPE_BUILDABLE,
						ctk_icon_factory_buildable_init))

static void
ctk_icon_factory_init (CtkIconFactory *factory)
{
  CtkIconFactoryPrivate *priv;

  factory->priv = ctk_icon_factory_get_instance_private (factory);
  priv = factory->priv;

  priv->icons = g_hash_table_new (g_str_hash, g_str_equal);
  all_icon_factories = g_slist_prepend (all_icon_factories, factory);
}

static void
ctk_icon_factory_class_init (CtkIconFactoryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = ctk_icon_factory_finalize;
}

static void
ctk_icon_factory_buildable_init (CtkBuildableIface *iface)
{
  iface->custom_tag_start = ctk_icon_factory_buildable_custom_tag_start;
  iface->custom_tag_end = ctk_icon_factory_buildable_custom_tag_end;
}

static void
free_icon_set (gpointer key,
	       gpointer value,
	       gpointer data G_GNUC_UNUSED)
{
  g_free (key);
  ctk_icon_set_unref (value);
}

static void
ctk_icon_factory_finalize (GObject *object)
{
  CtkIconFactory *factory = CTK_ICON_FACTORY (object);
  CtkIconFactoryPrivate *priv = factory->priv;

  all_icon_factories = g_slist_remove (all_icon_factories, factory);

  g_hash_table_foreach (priv->icons, free_icon_set, NULL);

  g_hash_table_destroy (priv->icons);

  G_OBJECT_CLASS (ctk_icon_factory_parent_class)->finalize (object);
}

/**
 * ctk_icon_factory_new:
 *
 * Creates a new #CtkIconFactory. An icon factory manages a collection
 * of #CtkIconSets; a #CtkIconSet manages a set of variants of a
 * particular icon (i.e. a #CtkIconSet contains variants for different
 * sizes and widget states). Icons in an icon factory are named by a
 * stock ID, which is a simple string identifying the icon. Each
 * #CtkStyle has a list of #CtkIconFactorys derived from the current
 * theme; those icon factories are consulted first when searching for
 * an icon. If the theme doesn’t set a particular icon, CTK+ looks for
 * the icon in a list of default icon factories, maintained by
 * ctk_icon_factory_add_default() and
 * ctk_icon_factory_remove_default(). Applications with icons should
 * add a default icon factory with their icons, which will allow
 * themes to override the icons for the application.
 *
 * Returns: a new #CtkIconFactory
 */
CtkIconFactory*
ctk_icon_factory_new (void)
{
  return g_object_new (CTK_TYPE_ICON_FACTORY, NULL);
}

/**
 * ctk_icon_factory_add:
 * @factory: a #CtkIconFactory
 * @stock_id: icon name
 * @icon_set: icon set
 *
 * Adds the given @icon_set to the icon factory, under the name
 * @stock_id.  @stock_id should be namespaced for your application,
 * e.g. “myapp-whatever-icon”.  Normally applications create a
 * #CtkIconFactory, then add it to the list of default factories with
 * ctk_icon_factory_add_default(). Then they pass the @stock_id to
 * widgets such as #CtkImage to display the icon. Themes can provide
 * an icon with the same name (such as "myapp-whatever-icon") to
 * override your application’s default icons. If an icon already
 * existed in @factory for @stock_id, it is unreferenced and replaced
 * with the new @icon_set.
 */
void
ctk_icon_factory_add (CtkIconFactory *factory,
                      const gchar    *stock_id,
                      CtkIconSet     *icon_set)
{
  CtkIconFactoryPrivate *priv = factory->priv;
  gpointer old_key = NULL;
  gpointer old_value = NULL;

  g_return_if_fail (CTK_IS_ICON_FACTORY (factory));
  g_return_if_fail (stock_id != NULL);
  g_return_if_fail (icon_set != NULL);

  g_hash_table_lookup_extended (priv->icons, stock_id,
                                &old_key, &old_value);

  if (old_value == icon_set)
    return;

  ctk_icon_set_ref (icon_set);

  /* GHashTable key memory management is so fantastically broken. */
  if (old_key)
    g_hash_table_insert (priv->icons, old_key, icon_set);
  else
    g_hash_table_insert (priv->icons, g_strdup (stock_id), icon_set);

  if (old_value)
    ctk_icon_set_unref (old_value);
}

/**
 * ctk_icon_factory_lookup:
 * @factory: a #CtkIconFactory
 * @stock_id: an icon name
 *
 * Looks up @stock_id in the icon factory, returning an icon set
 * if found, otherwise %NULL. For display to the user, you should
 * use ctk_style_lookup_icon_set() on the #CtkStyle for the
 * widget that will display the icon, instead of using this
 * function directly, so that themes are taken into account.
 *
 * Returns: (transfer none): icon set of @stock_id.
 */
CtkIconSet *
ctk_icon_factory_lookup (CtkIconFactory *factory,
                         const gchar    *stock_id)
{
  CtkIconFactoryPrivate *priv;

  g_return_val_if_fail (CTK_IS_ICON_FACTORY (factory), NULL);
  g_return_val_if_fail (stock_id != NULL, NULL);

  priv = factory->priv;

  return g_hash_table_lookup (priv->icons, stock_id);
}

static GSList *default_factories = NULL;

/**
 * ctk_icon_factory_add_default:
 * @factory: a #CtkIconFactory
 *
 * Adds an icon factory to the list of icon factories searched by
 * ctk_style_lookup_icon_set(). This means that, for example,
 * ctk_image_new_from_stock() will be able to find icons in @factory.
 * There will normally be an icon factory added for each library or
 * application that comes with icons. The default icon factories
 * can be overridden by themes.
 */
void
ctk_icon_factory_add_default (CtkIconFactory *factory)
{
  g_return_if_fail (CTK_IS_ICON_FACTORY (factory));

  g_object_ref (factory);

  default_factories = g_slist_prepend (default_factories, factory);
}

/**
 * ctk_icon_factory_remove_default:
 * @factory: a #CtkIconFactory previously added with ctk_icon_factory_add_default()
 *
 * Removes an icon factory from the list of default icon
 * factories. Not normally used; you might use it for a library that
 * can be unloaded or shut down.
 */
void
ctk_icon_factory_remove_default (CtkIconFactory  *factory)
{
  g_return_if_fail (CTK_IS_ICON_FACTORY (factory));

  default_factories = g_slist_remove (default_factories, factory);

  g_object_unref (factory);
}

static CtkIconFactory *
_ctk_icon_factory_get_default_icons (void)
{
  static CtkIconFactory *default_icons = NULL;
  CtkIconFactory *icons = NULL;
  CdkScreen *screen = cdk_screen_get_default ();

  if (screen)
    icons = g_object_get_data (G_OBJECT (screen), "ctk-default-icons");

  if (icons == NULL)
    {
      if (default_icons == NULL)
        {
          default_icons = ctk_icon_factory_new ();
          get_default_icons (default_icons);
        }
      if (screen)
        g_object_set_data_full (G_OBJECT (screen),
                                I_("ctk-default-icons"),
                                default_icons,
                                g_object_unref);
      icons = default_icons;
    }

  return icons;
}

/**
 * ctk_icon_factory_lookup_default:
 * @stock_id: an icon name
 *
 * Looks for an icon in the list of default icon factories.  For
 * display to the user, you should use ctk_style_lookup_icon_set() on
 * the #CtkStyle for the widget that will display the icon, instead of
 * using this function directly, so that themes are taken into
 * account.
 *
 * Returns: (transfer none): a #CtkIconSet, or %NULL
 */
CtkIconSet *
ctk_icon_factory_lookup_default (const gchar *stock_id)
{
  GSList *tmp_list;
  CtkIconFactory *default_icons;

  g_return_val_if_fail (stock_id != NULL, NULL);

  tmp_list = default_factories;
  while (tmp_list != NULL)
    {
      CtkIconSet *icon_set =
        ctk_icon_factory_lookup (CTK_ICON_FACTORY (tmp_list->data),
                                 stock_id);

      if (icon_set)
        return icon_set;

      tmp_list = tmp_list->next;
    }

  default_icons = _ctk_icon_factory_get_default_icons ();
  if (default_icons)
    return ctk_icon_factory_lookup (default_icons, stock_id);
  else
    return NULL;
}

static void
register_stock_icon (CtkIconFactory *factory,
		     const gchar    *stock_id,
                     const gchar    *icon_name)
{
  CtkIconSet *set = ctk_icon_set_new ();
  CtkIconSource source = CTK_ICON_SOURCE_INIT (TRUE, TRUE, TRUE);

  source.type = CTK_ICON_SOURCE_STATIC_ICON_NAME;
  source.source.icon_name = (gchar *)icon_name;
  source.direction = CTK_TEXT_DIR_NONE;
  ctk_icon_set_add_source (set, &source);

  ctk_icon_factory_add (factory, stock_id, set);
  ctk_icon_set_unref (set);
}

static void
register_bidi_stock_icon (CtkIconFactory *factory,
			  const gchar    *stock_id,
                          const gchar    *icon_name)
{
  CtkIconSet *set = ctk_icon_set_new ();
  CtkIconSource source = CTK_ICON_SOURCE_INIT (FALSE, TRUE, TRUE);

  source.type = CTK_ICON_SOURCE_STATIC_ICON_NAME;
  source.source.icon_name = (gchar *)icon_name;
  source.direction = CTK_TEXT_DIR_LTR;
  ctk_icon_set_add_source (set, &source);

  source.type = CTK_ICON_SOURCE_STATIC_ICON_NAME;
  source.source.icon_name = (gchar *)icon_name;
  source.direction = CTK_TEXT_DIR_RTL;
  ctk_icon_set_add_source (set, &source);

  ctk_icon_factory_add (factory, stock_id, set);
  ctk_icon_set_unref (set);
}

static void
get_default_icons (CtkIconFactory *factory)
{
  /* KEEP IN SYNC with ctkstock.c */

  register_stock_icon (factory, CTK_STOCK_DIALOG_AUTHENTICATION, "dialog-password");
  register_stock_icon (factory, CTK_STOCK_DIALOG_ERROR, "dialog-error");
  register_stock_icon (factory, CTK_STOCK_DIALOG_INFO, "dialog-information");
  register_stock_icon (factory, CTK_STOCK_DIALOG_QUESTION, "dialog-question");
  register_stock_icon (factory, CTK_STOCK_DIALOG_WARNING, "dialog-warning");
  register_stock_icon (factory, CTK_STOCK_DND, CTK_STOCK_DND);
  register_stock_icon (factory, CTK_STOCK_DND_MULTIPLE, CTK_STOCK_DND_MULTIPLE);
  register_stock_icon (factory, CTK_STOCK_APPLY, CTK_STOCK_APPLY);
  register_stock_icon (factory, CTK_STOCK_CANCEL, CTK_STOCK_CANCEL);
  register_stock_icon (factory, CTK_STOCK_NO, CTK_STOCK_NO);
  register_stock_icon (factory, CTK_STOCK_OK, CTK_STOCK_OK);
  register_stock_icon (factory, CTK_STOCK_YES, CTK_STOCK_YES);
  register_stock_icon (factory, CTK_STOCK_CLOSE, "window-close");
  register_stock_icon (factory, CTK_STOCK_ADD, "list-add");
  register_stock_icon (factory, CTK_STOCK_JUSTIFY_CENTER, "format-justify-center");
  register_stock_icon (factory, CTK_STOCK_JUSTIFY_FILL, "format-justify-fill");
  register_stock_icon (factory, CTK_STOCK_JUSTIFY_LEFT, "format-justify-left");
  register_stock_icon (factory, CTK_STOCK_JUSTIFY_RIGHT, "format-justify-right");
  register_stock_icon (factory, CTK_STOCK_GOTO_BOTTOM, "go-bottom");
  register_stock_icon (factory, CTK_STOCK_CDROM, "media-optical");
  register_stock_icon (factory, CTK_STOCK_CONVERT, CTK_STOCK_CONVERT);
  register_stock_icon (factory, CTK_STOCK_COPY, "edit-copy");
  register_stock_icon (factory, CTK_STOCK_CUT, "edit-cut");
  register_stock_icon (factory, CTK_STOCK_GO_DOWN, "go-down");
  register_stock_icon (factory, CTK_STOCK_EXECUTE, "system-run");
  register_stock_icon (factory, CTK_STOCK_QUIT, "application-exit");
  register_bidi_stock_icon (factory, CTK_STOCK_GOTO_FIRST, "go-first");
  register_stock_icon (factory, CTK_STOCK_SELECT_FONT, CTK_STOCK_SELECT_FONT);
  register_stock_icon (factory, CTK_STOCK_FULLSCREEN, "view-fullscreen");
  register_stock_icon (factory, CTK_STOCK_LEAVE_FULLSCREEN, "view-restore");
  register_stock_icon (factory, CTK_STOCK_HARDDISK, "drive-harddisk");
  register_stock_icon (factory, CTK_STOCK_HELP, "help-contents");
  register_stock_icon (factory, CTK_STOCK_HOME, "go-home");
  register_stock_icon (factory, CTK_STOCK_INFO, "dialog-information");
  register_bidi_stock_icon (factory, CTK_STOCK_JUMP_TO, "go-jump");
  register_bidi_stock_icon (factory, CTK_STOCK_GOTO_LAST, "go-last");
  register_bidi_stock_icon (factory, CTK_STOCK_GO_BACK, "go-previous");
  register_stock_icon (factory, CTK_STOCK_MISSING_IMAGE, "image-missing");
  register_stock_icon (factory, CTK_STOCK_NETWORK, "network-idle");
  register_stock_icon (factory, CTK_STOCK_NEW, "document-new");
  register_stock_icon (factory, CTK_STOCK_OPEN, "document-open");
  register_stock_icon (factory, CTK_STOCK_ORIENTATION_PORTRAIT, CTK_STOCK_ORIENTATION_PORTRAIT);
  register_stock_icon (factory, CTK_STOCK_ORIENTATION_LANDSCAPE, CTK_STOCK_ORIENTATION_LANDSCAPE);
  register_stock_icon (factory, CTK_STOCK_ORIENTATION_REVERSE_PORTRAIT, CTK_STOCK_ORIENTATION_REVERSE_PORTRAIT);
  register_stock_icon (factory, CTK_STOCK_ORIENTATION_REVERSE_LANDSCAPE, CTK_STOCK_ORIENTATION_REVERSE_LANDSCAPE);
  register_stock_icon (factory, CTK_STOCK_PAGE_SETUP, CTK_STOCK_PAGE_SETUP);
  register_stock_icon (factory, CTK_STOCK_PASTE, "edit-paste");
  register_stock_icon (factory, CTK_STOCK_PREFERENCES, CTK_STOCK_PREFERENCES);
  register_stock_icon (factory, CTK_STOCK_PRINT, "document-print");
  register_stock_icon (factory, CTK_STOCK_PRINT_ERROR, "printer-error");
  register_stock_icon (factory, CTK_STOCK_PRINT_PAUSED, "printer-paused");
  register_stock_icon (factory, CTK_STOCK_PRINT_PREVIEW, "document-print-preview");
  register_stock_icon (factory, CTK_STOCK_PRINT_REPORT, "printer-info");
  register_stock_icon (factory, CTK_STOCK_PRINT_WARNING, "printer-warning");
  register_stock_icon (factory, CTK_STOCK_PROPERTIES, "document-properties");
  register_bidi_stock_icon (factory, CTK_STOCK_REDO, "edit-redo");
  register_stock_icon (factory, CTK_STOCK_REMOVE, "list-remove");
  register_stock_icon (factory, CTK_STOCK_REFRESH, "view-refresh");
  register_bidi_stock_icon (factory, CTK_STOCK_REVERT_TO_SAVED, "document-revert");
  register_bidi_stock_icon (factory, CTK_STOCK_GO_FORWARD, "go-next");
  register_stock_icon (factory, CTK_STOCK_SAVE, "document-save");
  register_stock_icon (factory, CTK_STOCK_FLOPPY, "media-floppy");
  register_stock_icon (factory, CTK_STOCK_SAVE_AS, "document-save-as");
  register_stock_icon (factory, CTK_STOCK_FIND, "edit-find");
  register_stock_icon (factory, CTK_STOCK_FIND_AND_REPLACE, "edit-find-replace");
  register_stock_icon (factory, CTK_STOCK_SORT_DESCENDING, "view-sort-descending");
  register_stock_icon (factory, CTK_STOCK_SORT_ASCENDING, "view-sort-ascending");
  register_stock_icon (factory, CTK_STOCK_SPELL_CHECK, "tools-check-spelling");
  register_stock_icon (factory, CTK_STOCK_STOP, "process-stop");
  register_stock_icon (factory, CTK_STOCK_BOLD, "format-text-bold");
  register_stock_icon (factory, CTK_STOCK_ITALIC, "format-text-italic");
  register_stock_icon (factory, CTK_STOCK_STRIKETHROUGH, "format-text-strikethrough");
  register_stock_icon (factory, CTK_STOCK_UNDERLINE, "format-text-underline");
  register_bidi_stock_icon (factory, CTK_STOCK_INDENT, "format-indent-more");
  register_bidi_stock_icon (factory, CTK_STOCK_UNINDENT, "format-indent-less");
  register_stock_icon (factory, CTK_STOCK_GOTO_TOP, "go-top");
  register_stock_icon (factory, CTK_STOCK_DELETE, "edit-delete");
  register_bidi_stock_icon (factory, CTK_STOCK_UNDELETE, CTK_STOCK_UNDELETE);
  register_bidi_stock_icon (factory, CTK_STOCK_UNDO, "edit-undo");
  register_stock_icon (factory, CTK_STOCK_GO_UP, "go-up");
  register_stock_icon (factory, CTK_STOCK_FILE, "text-x-generic");
  register_stock_icon (factory, CTK_STOCK_DIRECTORY, "folder");
  register_stock_icon (factory, CTK_STOCK_ABOUT, "help-about");
  register_stock_icon (factory, CTK_STOCK_CONNECT, CTK_STOCK_CONNECT);
  register_stock_icon (factory, CTK_STOCK_DISCONNECT, CTK_STOCK_DISCONNECT);
  register_stock_icon (factory, CTK_STOCK_EDIT, CTK_STOCK_EDIT);
  register_stock_icon (factory, CTK_STOCK_CAPS_LOCK_WARNING, CTK_STOCK_CAPS_LOCK_WARNING);
  register_bidi_stock_icon (factory, CTK_STOCK_MEDIA_FORWARD, "media-seek-forward");
  register_bidi_stock_icon (factory, CTK_STOCK_MEDIA_NEXT, "media-skip-forward");
  register_stock_icon (factory, CTK_STOCK_MEDIA_PAUSE, "media-playback-pause");
  register_bidi_stock_icon (factory, CTK_STOCK_MEDIA_PLAY, "media-playback-start");
  register_bidi_stock_icon (factory, CTK_STOCK_MEDIA_PREVIOUS, "media-skip-backward");
  register_stock_icon (factory, CTK_STOCK_MEDIA_RECORD, "media-record");
  register_bidi_stock_icon (factory, CTK_STOCK_MEDIA_REWIND, "media-seek-backward");
  register_stock_icon (factory, CTK_STOCK_MEDIA_STOP, "media-playback-stop");
  register_stock_icon (factory, CTK_STOCK_INDEX, CTK_STOCK_INDEX);
  register_stock_icon (factory, CTK_STOCK_ZOOM_100, "zoom-original");
  register_stock_icon (factory, CTK_STOCK_ZOOM_IN, "zoom-in");
  register_stock_icon (factory, CTK_STOCK_ZOOM_OUT, "zoom-out");
  register_stock_icon (factory, CTK_STOCK_ZOOM_FIT, "zoom-fit-best");
  register_stock_icon (factory, CTK_STOCK_SELECT_ALL, "edit-select-all");
  register_bidi_stock_icon (factory, CTK_STOCK_CLEAR, "edit-clear");
  register_stock_icon (factory, CTK_STOCK_SELECT_COLOR, CTK_STOCK_SELECT_COLOR);
  register_stock_icon (factory, CTK_STOCK_COLOR_PICKER, CTK_STOCK_COLOR_PICKER);
}

/************************************************************
 *                    Icon size handling                    *
 ************************************************************/

typedef struct _IconSize IconSize;

struct _IconSize
{
  gint size;
  gchar *name;

  gint width;
  gint height;
};

typedef struct _IconAlias IconAlias;

struct _IconAlias
{
  gchar *name;
  gint   target;
};

static GHashTable *icon_aliases = NULL;
static IconSize *icon_sizes = NULL;
static gint      icon_sizes_allocated = 0;
static gint      icon_sizes_used = 0;

static void
init_icon_sizes (void)
{
  if (icon_sizes == NULL)
    {
#define NUM_BUILTIN_SIZES 7
      gint i;

      icon_aliases = g_hash_table_new (g_str_hash, g_str_equal);

      icon_sizes = g_new (IconSize, NUM_BUILTIN_SIZES);
      icon_sizes_allocated = NUM_BUILTIN_SIZES;
      icon_sizes_used = NUM_BUILTIN_SIZES;

      icon_sizes[CTK_ICON_SIZE_INVALID].size = 0;
      icon_sizes[CTK_ICON_SIZE_INVALID].name = NULL;
      icon_sizes[CTK_ICON_SIZE_INVALID].width = 0;
      icon_sizes[CTK_ICON_SIZE_INVALID].height = 0;

      /* the name strings aren't copied since we don't ever remove
       * icon sizes, so we don't need to know whether they're static.
       * Even if we did I suppose removing the builtin sizes would be
       * disallowed.
       */

      icon_sizes[CTK_ICON_SIZE_MENU].size = CTK_ICON_SIZE_MENU;
      icon_sizes[CTK_ICON_SIZE_MENU].name = "ctk-menu";
      icon_sizes[CTK_ICON_SIZE_MENU].width = 16;
      icon_sizes[CTK_ICON_SIZE_MENU].height = 16;

      icon_sizes[CTK_ICON_SIZE_BUTTON].size = CTK_ICON_SIZE_BUTTON;
      icon_sizes[CTK_ICON_SIZE_BUTTON].name = "ctk-button";
      icon_sizes[CTK_ICON_SIZE_BUTTON].width = 16;
      icon_sizes[CTK_ICON_SIZE_BUTTON].height = 16;

      icon_sizes[CTK_ICON_SIZE_SMALL_TOOLBAR].size = CTK_ICON_SIZE_SMALL_TOOLBAR;
      icon_sizes[CTK_ICON_SIZE_SMALL_TOOLBAR].name = "ctk-small-toolbar";
      icon_sizes[CTK_ICON_SIZE_SMALL_TOOLBAR].width = 16;
      icon_sizes[CTK_ICON_SIZE_SMALL_TOOLBAR].height = 16;

      icon_sizes[CTK_ICON_SIZE_LARGE_TOOLBAR].size = CTK_ICON_SIZE_LARGE_TOOLBAR;
      icon_sizes[CTK_ICON_SIZE_LARGE_TOOLBAR].name = "ctk-large-toolbar";
      icon_sizes[CTK_ICON_SIZE_LARGE_TOOLBAR].width = 24;
      icon_sizes[CTK_ICON_SIZE_LARGE_TOOLBAR].height = 24;

      icon_sizes[CTK_ICON_SIZE_DND].size = CTK_ICON_SIZE_DND;
      icon_sizes[CTK_ICON_SIZE_DND].name = "ctk-dnd";
      icon_sizes[CTK_ICON_SIZE_DND].width = 32;
      icon_sizes[CTK_ICON_SIZE_DND].height = 32;

      icon_sizes[CTK_ICON_SIZE_DIALOG].size = CTK_ICON_SIZE_DIALOG;
      icon_sizes[CTK_ICON_SIZE_DIALOG].name = "ctk-dialog";
      icon_sizes[CTK_ICON_SIZE_DIALOG].width = 48;
      icon_sizes[CTK_ICON_SIZE_DIALOG].height = 48;

      g_assert ((CTK_ICON_SIZE_DIALOG + 1) == NUM_BUILTIN_SIZES);

      /* Alias everything to itself. */
      i = 1; /* skip invalid size */
      while (i < NUM_BUILTIN_SIZES)
        {
          ctk_icon_size_register_alias (icon_sizes[i].name, icon_sizes[i].size);

          ++i;
        }

#undef NUM_BUILTIN_SIZES
    }
}

static gboolean
icon_size_lookup_intern (CtkIconSize  size,
			 gint        *widthp,
			 gint        *heightp)
{
  init_icon_sizes ();

  if (size == (CtkIconSize)-1)
    return FALSE;

  if (size >= icon_sizes_used)
    return FALSE;

  if (size == CTK_ICON_SIZE_INVALID)
    return FALSE;

  if (widthp)
    *widthp = icon_sizes[size].width;

  if (heightp)
    *heightp = icon_sizes[size].height;

  return TRUE;
}

/**
 * ctk_icon_size_lookup_for_settings:
 * @settings: a #CtkSettings object, used to determine
 *   which set of user preferences to used.
 * @size: (type int): an icon size (#CtkIconSize)
 * @width: (out) (allow-none): location to store icon width
 * @height: (out) (allow-none): location to store icon height
 *
 * Obtains the pixel size of a semantic icon size, possibly
 * modified by user preferences for a particular
 * #CtkSettings. Normally @size would be
 * #CTK_ICON_SIZE_MENU, #CTK_ICON_SIZE_BUTTON, etc.  This function
 * isn’t normally needed, ctk_widget_render_icon_pixbuf() is the usual
 * way to get an icon for rendering, then just look at the size of
 * the rendered pixbuf. The rendered pixbuf may not even correspond to
 * the width/height returned by ctk_icon_size_lookup(), because themes
 * are free to render the pixbuf however they like, including changing
 * the usual size.
 *
 * Returns: %TRUE if @size was a valid size
 *
 * Since: 2.2
 */
gboolean
ctk_icon_size_lookup_for_settings (CtkSettings *settings,
				   CtkIconSize  size,
				   gint        *width,
				   gint        *height)
{
  g_return_val_if_fail (CTK_IS_SETTINGS (settings), FALSE);

  return icon_size_lookup_intern (size, width, height);
}

/**
 * ctk_icon_size_lookup:
 * @size: (type int): an icon size (#CtkIconSize)
 * @width: (out) (allow-none): location to store icon width
 * @height: (out) (allow-none): location to store icon height
 *
 * Obtains the pixel size of a semantic icon size @size:
 * #CTK_ICON_SIZE_MENU, #CTK_ICON_SIZE_BUTTON, etc.  This function
 * isn’t normally needed, ctk_icon_theme_load_icon() is the usual
 * way to get an icon for rendering, then just look at the size of
 * the rendered pixbuf. The rendered pixbuf may not even correspond to
 * the width/height returned by ctk_icon_size_lookup(), because themes
 * are free to render the pixbuf however they like, including changing
 * the usual size.
 *
 * Returns: %TRUE if @size was a valid size
 */
gboolean
ctk_icon_size_lookup (CtkIconSize  size,
                      gint        *widthp,
                      gint        *heightp)
{
  CTK_NOTE (MULTIHEAD,
	    g_warning ("ctk_icon_size_lookup ()) is not multihead safe"));

  return icon_size_lookup_intern (size, widthp, heightp);
}

static CtkIconSize
icon_size_register_intern (const gchar *name,
			   gint         width,
			   gint         height)
{
  IconAlias *old_alias;
  CtkIconSize size;

  init_icon_sizes ();

  old_alias = g_hash_table_lookup (icon_aliases, name);
  if (old_alias && icon_sizes[old_alias->target].width > 0)
    {
      g_warning ("Icon size name '%s' already exists", name);
      return CTK_ICON_SIZE_INVALID;
    }

  if (old_alias)
    {
      size = old_alias->target;
    }
  else
    {
      if (icon_sizes_used == icon_sizes_allocated)
	{
	  icon_sizes_allocated *= 2;
	  icon_sizes = g_renew (IconSize, icon_sizes, icon_sizes_allocated);
	}

      size = icon_sizes_used++;

      /* alias to self. */
      ctk_icon_size_register_alias (name, size);

      icon_sizes[size].size = size;
      icon_sizes[size].name = g_strdup (name);
    }

  icon_sizes[size].width = width;
  icon_sizes[size].height = height;

  return size;
}

/**
 * ctk_icon_size_register:
 * @name: name of the icon size
 * @width: the icon width
 * @height: the icon height
 *
 * Registers a new icon size, along the same lines as #CTK_ICON_SIZE_MENU,
 * etc. Returns the integer value for the size.
 *
 * Returns: (type int): integer value representing the size (#CtkIconSize)
 */
CtkIconSize
ctk_icon_size_register (const gchar *name,
                        gint         width,
                        gint         height)
{
  g_return_val_if_fail (name != NULL, 0);
  g_return_val_if_fail (width > 0, 0);
  g_return_val_if_fail (height > 0, 0);

  return icon_size_register_intern (name, width, height);
}

/**
 * ctk_icon_size_register_alias:
 * @alias: an alias for @target
 * @target: (type int): an existing icon size (#CtkIconSize)
 *
 * Registers @alias as another name for @target.
 * So calling ctk_icon_size_from_name() with @alias as argument
 * will return @target.
 */
void
ctk_icon_size_register_alias (const gchar *alias,
                              CtkIconSize  target)
{
  IconAlias *ia;

  g_return_if_fail (alias != NULL);

  init_icon_sizes ();

  if (!icon_size_lookup_intern (target, NULL, NULL))
    g_warning ("ctk_icon_size_register_alias: Icon size %u does not exist", target);

  ia = g_hash_table_lookup (icon_aliases, alias);
  if (ia)
    {
      if (icon_sizes[ia->target].width > 0)
	{
	  g_warning ("ctk_icon_size_register_alias: Icon size name '%s' already exists", alias);
	  return;
	}

      ia->target = target;
    }

  if (!ia)
    {
      ia = g_new (IconAlias, 1);
      ia->name = g_strdup (alias);
      ia->target = target;

      g_hash_table_insert (icon_aliases, ia->name, ia);
    }
}

/**
 * ctk_icon_size_from_name:
 * @name: the name to look up.
 *
 * Looks up the icon size associated with @name.
 *
 * Returns: (type int): the icon size (#CtkIconSize)
 */
CtkIconSize
ctk_icon_size_from_name (const gchar *name)
{
  IconAlias *ia;

  init_icon_sizes ();

  ia = g_hash_table_lookup (icon_aliases, name);

  if (ia && icon_sizes[ia->target].width > 0)
    return ia->target;
  else
    return CTK_ICON_SIZE_INVALID;
}

/**
 * ctk_icon_size_get_name:
 * @size: (type int): a #CtkIconSize.
 *
 * Gets the canonical name of the given icon size. The returned string
 * is statically allocated and should not be freed.
 *
 * Returns: the name of the given icon size.
 */
const gchar*
ctk_icon_size_get_name (CtkIconSize  size)
{
  if (size >= icon_sizes_used)
    return NULL;
  else
    return icon_sizes[size].name;
}

/************************************************************/

/* Icon Set */

struct _CtkIconSet
{
  guint ref_count;

  GSList *sources;
};

/**
 * ctk_icon_set_new:
 *
 * Creates a new #CtkIconSet. A #CtkIconSet represents a single icon
 * in various sizes and widget states. It can provide a #GdkPixbuf
 * for a given size and state on request, and automatically caches
 * some of the rendered #GdkPixbuf objects.
 *
 * Normally you would use ctk_widget_render_icon_pixbuf() instead of
 * using #CtkIconSet directly. The one case where you’d use
 * #CtkIconSet is to create application-specific icon sets to place in
 * a #CtkIconFactory.
 *
 * Returns: a new #CtkIconSet
 */
CtkIconSet*
ctk_icon_set_new (void)
{
  CtkIconSet *icon_set;

  icon_set = g_new (CtkIconSet, 1);

  icon_set->ref_count = 1;
  icon_set->sources = NULL;

  return icon_set;
}

/**
 * ctk_icon_set_new_from_pixbuf:
 * @pixbuf: a #GdkPixbuf
 *
 * Creates a new #CtkIconSet with @pixbuf as the default/fallback
 * source image. If you don’t add any additional #CtkIconSource to the
 * icon set, all variants of the icon will be created from @pixbuf,
 * using scaling, pixelation, etc. as required to adjust the icon size
 * or make the icon look insensitive/prelighted.
 *
 * Returns: a new #CtkIconSet
 */
CtkIconSet *
ctk_icon_set_new_from_pixbuf (GdkPixbuf *pixbuf)
{
  CtkIconSet *set;

  CtkIconSource source = CTK_ICON_SOURCE_INIT (TRUE, TRUE, TRUE);

  g_return_val_if_fail (pixbuf != NULL, NULL);

  set = ctk_icon_set_new ();

  ctk_icon_source_set_pixbuf (&source, pixbuf);
  ctk_icon_set_add_source (set, &source);
  ctk_icon_source_set_pixbuf (&source, NULL);

  return set;
}


/**
 * ctk_icon_set_ref:
 * @icon_set: a #CtkIconSet.
 *
 * Increments the reference count on @icon_set.
 *
 * Returns: @icon_set.
 */
CtkIconSet*
ctk_icon_set_ref (CtkIconSet *icon_set)
{
  g_return_val_if_fail (icon_set != NULL, NULL);
  g_return_val_if_fail (icon_set->ref_count > 0, NULL);

  icon_set->ref_count += 1;

  return icon_set;
}

/**
 * ctk_icon_set_unref:
 * @icon_set: a #CtkIconSet
 *
 * Decrements the reference count on @icon_set, and frees memory
 * if the reference count reaches 0.
 */
void
ctk_icon_set_unref (CtkIconSet *icon_set)
{
  g_return_if_fail (icon_set != NULL);
  g_return_if_fail (icon_set->ref_count > 0);

  icon_set->ref_count -= 1;

  if (icon_set->ref_count == 0)
    {
      GSList *tmp_list = icon_set->sources;
      while (tmp_list != NULL)
        {
          ctk_icon_source_free (tmp_list->data);

          tmp_list = tmp_list->next;
        }
      g_slist_free (icon_set->sources);

      g_free (icon_set);
    }
}

G_DEFINE_BOXED_TYPE (CtkIconSet, ctk_icon_set,
                     ctk_icon_set_ref,
                     ctk_icon_set_unref)

/**
 * ctk_icon_set_copy:
 * @icon_set: a #CtkIconSet
 *
 * Copies @icon_set by value.
 *
 * Returns: a new #CtkIconSet identical to the first.
 **/
CtkIconSet*
ctk_icon_set_copy (CtkIconSet *icon_set)
{
  CtkIconSet *copy;
  GSList *tmp_list;

  copy = ctk_icon_set_new ();

  tmp_list = icon_set->sources;
  while (tmp_list != NULL)
    {
      copy->sources = g_slist_prepend (copy->sources,
                                       ctk_icon_source_copy (tmp_list->data));

      tmp_list = tmp_list->next;
    }

  copy->sources = g_slist_reverse (copy->sources);

  return copy;
}

static gboolean
sizes_equivalent (CtkIconSize lhs,
                  CtkIconSize rhs)
{
  /* We used to consider sizes equivalent if they were
   * the same pixel size, but we don't have the CtkSettings
   * here, so we can't do that. Plus, it's not clear that
   * it is right... it was just a workaround for the fact
   * that we register icons by logical size, not pixel size.
   */
#if 1
  return lhs == rhs;
#else

  gint r_w, r_h, l_w, l_h;

  icon_size_lookup_intern (rhs, &r_w, &r_h);
  icon_size_lookup_intern (lhs, &l_w, &l_h);

  return r_w == l_w && r_h == l_h;
#endif
}

static CtkIconSource *
find_best_matching_source (CtkIconSet       *icon_set,
			   CtkTextDirection  direction,
			   CtkStateType      state,
			   CtkIconSize       size,
			   GSList           *failed)
{
  CtkIconSource *source;
  GSList *tmp_list;

  /* We need to find the best icon source.  Direction matters more
   * than state, state matters more than size. icon_set->sources
   * is sorted according to wildness, so if we take the first
   * match we find it will be the least-wild match (if there are
   * multiple matches for a given "wildness" then the RC file contained
   * dumb stuff, and we end up with an arbitrary matching source)
   */

  source = NULL;
  tmp_list = icon_set->sources;
  while (tmp_list != NULL)
    {
      CtkIconSource *s = tmp_list->data;

      if ((s->any_direction || (s->direction == direction)) &&
          (s->any_state || (s->state == state)) &&
          (s->any_size || size == (CtkIconSize)-1 || (sizes_equivalent (size, s->size))))
        {
	  if (!g_slist_find (failed, s))
	    {
	      source = s;
	      break;
	    }
	}

      tmp_list = tmp_list->next;
    }

  return source;
}

static gboolean
ensure_filename_pixbuf (CtkIconSet    *icon_set,
			CtkIconSource *source)
{
  if (source->filename_pixbuf == NULL)
    {
      GError *error = NULL;

      source->filename_pixbuf = gdk_pixbuf_new_from_file (source->source.filename, &error);

      if (source->filename_pixbuf == NULL)
	{
	  /* Remove this icon source so we don't keep trying to
	   * load it.
	   */
	  g_warning ("Error loading icon: %s", error->message);
	  g_error_free (error);

	  icon_set->sources = g_slist_remove (icon_set->sources, source);

	  ctk_icon_source_free (source);

	  return FALSE;
	}
    }

  return TRUE;
}

static GdkPixbuf *
render_icon_name_pixbuf (CtkIconSource *icon_source,
			 CtkCssStyle   *style,
			 CtkIconSize    size,
                         gint           scale)
{
  GdkPixbuf *pixbuf;
  GdkPixbuf *tmp_pixbuf;
  CtkIconTheme *icon_theme;
  gint width, height, pixel_size;
  gint *sizes, *s, dist;
  GError *error = NULL;

  icon_theme = ctk_css_icon_theme_value_get_icon_theme
    (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_ICON_THEME));

  if (!ctk_icon_size_lookup (size, &width, &height))
    {
      if (size == (CtkIconSize)-1)
	{
	  /* Find an available size close to 48 */
	  sizes = ctk_icon_theme_get_icon_sizes (icon_theme, icon_source->source.icon_name);
	  dist = 1000;
	  width = height = 48;
	  for (s = sizes; *s; s++)
	    {
	      if (*s == -1)
		{
		  width = height = 48;
		  break;
		}
	      if (*s < 48)
		{
		  if (48 - *s < dist)
		    {
		      width = height = *s;
		      dist = 48 - *s;
		    }
		}
	      else
		{
		  if (*s - 48 < dist)
		    {
		      width = height = *s;
		      dist = *s - 48;
		    }
		}
	    }

	  g_free (sizes);
	}
      else
	{
	  g_warning ("Invalid icon size %u\n", size);
	  width = height = 24;
	}
    }

  pixel_size = MIN (width, height);

  if (icon_source->direction != CTK_TEXT_DIR_NONE)
    {
      gchar *suffix[3] = { NULL, "-ltr", "-rtl" };
      gchar *names[3];
      CtkIconInfo *info;

      names[0] = g_strconcat (icon_source->source.icon_name, suffix[icon_source->direction], NULL);
      names[1] = icon_source->source.icon_name;
      names[2] = NULL;

      info = ctk_icon_theme_choose_icon_for_scale (icon_theme,
                                                   (const char **) names,
                                                   pixel_size, scale,
                                                   CTK_ICON_LOOKUP_USE_BUILTIN);
      g_free (names[0]);
      if (info)
        {
          tmp_pixbuf = ctk_icon_info_load_icon (info, &error);
          g_object_unref (info);
        }
      else
        tmp_pixbuf = NULL;
    }
  else
    {
      tmp_pixbuf = ctk_icon_theme_load_icon_for_scale (icon_theme,
                                                       icon_source->source.icon_name,
                                                       pixel_size, scale, 0,
                                                       &error);
    }

  if (!tmp_pixbuf)
    {
      g_warning ("Error loading theme icon '%s' for stock: %s",
                 icon_source->source.icon_name, error ? error->message : "");
      if (error)
        g_error_free (error);
      return NULL;
    }

  pixbuf = ctk_render_icon_pixbuf_unpacked (tmp_pixbuf,
                                            -1,
                                            ctk_icon_source_get_state_wildcarded (icon_source)
                                            ? _ctk_css_icon_effect_value_get (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_ICON_EFFECT))
                                            : CTK_CSS_ICON_EFFECT_NONE);

  if (!pixbuf)
    g_warning ("Failed to render icon");

  g_object_unref (tmp_pixbuf);

  return pixbuf;
}

static GdkPixbuf *
find_and_render_icon_source (CtkIconSet       *icon_set,
			     CtkCssStyle      *style,
			     CtkTextDirection  direction,
			     CtkStateType      state,
			     CtkIconSize       size,
			     gint              scale)
{
  GSList *failed = NULL;
  GdkPixbuf *pixbuf = NULL;

  /* We treat failure in two different ways:
   *
   *  A) If loading a source that specifies a filename fails,
   *     we treat that as permanent, and remove the source
   *     from the CtkIconSet. (in ensure_filename_pixbuf ()
   *  B) If loading a themed icon fails, or scaling an icon
   *     fails, we treat that as transient and will try
   *     again next time the icon falls out of the cache
   *     and we need to recreate it.
   */
  while (pixbuf == NULL)
    {
      CtkIconSource *source = find_best_matching_source (icon_set, direction, state, size, failed);

      if (source == NULL)
	break;

      switch (source->type)
	{
	case CTK_ICON_SOURCE_FILENAME:
	  if (!ensure_filename_pixbuf (icon_set, source))
	    break;
	  /* Fall through */
	case CTK_ICON_SOURCE_PIXBUF:
          pixbuf = ctk_render_icon_pixbuf_unpacked (ctk_icon_source_get_pixbuf (source),
                                                    ctk_icon_source_get_size_wildcarded (source) ? size : -1,
                                                    ctk_icon_source_get_state_wildcarded (source)
                                                    ? _ctk_css_icon_effect_value_get (
                                                       ctk_css_style_get_value (style, CTK_CSS_PROPERTY_ICON_EFFECT))
                                                    : CTK_CSS_ICON_EFFECT_NONE);
	  if (!pixbuf)
	    {
	      g_warning ("Failed to render icon");
	      failed = g_slist_prepend (failed, source);
	    }

	  if (scale != 1)
	    {
	      GdkPixbuf *tmp = pixbuf;
	      pixbuf = gdk_pixbuf_scale_simple (pixbuf,
						gdk_pixbuf_get_width (pixbuf) * scale,
						gdk_pixbuf_get_height (pixbuf) * scale,
						GDK_INTERP_BILINEAR);
	      g_object_unref (tmp);
	    }
	  break;
	case CTK_ICON_SOURCE_ICON_NAME:
	case CTK_ICON_SOURCE_STATIC_ICON_NAME:
          pixbuf = render_icon_name_pixbuf (source, style,
                                            size, scale);
	  if (!pixbuf)
	    failed = g_slist_prepend (failed, source);
	  break;
	case CTK_ICON_SOURCE_EMPTY:
	  g_assert_not_reached ();
	}
    }

  g_slist_free (failed);

  return pixbuf;
}

static GdkPixbuf*
render_fallback_image (CtkCssStyle       *style,
                       CtkTextDirection   direction G_GNUC_UNUSED,
                       CtkStateType       state G_GNUC_UNUSED,
                       CtkIconSize        size)
{
  /* This icon can be used for any direction/state/size */
  static CtkIconSource fallback_source = CTK_ICON_SOURCE_INIT (TRUE, TRUE, TRUE);

  if (fallback_source.type == CTK_ICON_SOURCE_EMPTY)
    {
      fallback_source.type = CTK_ICON_SOURCE_STATIC_ICON_NAME;
      fallback_source.source.icon_name = (gchar *)"image-missing";
      fallback_source.direction = CTK_TEXT_DIR_NONE;
    }

  return render_icon_name_pixbuf (&fallback_source, style, size, 1);
}

GdkPixbuf*
ctk_icon_set_render_icon_pixbuf_for_scale (CtkIconSet       *icon_set,
					   CtkCssStyle      *style,
                                           CtkTextDirection  direction,
					   CtkIconSize       size,
					   gint              scale)
{
  GdkPixbuf *icon = NULL;
  CtkStateType state;
  CtkCssIconEffect effect;

  g_return_val_if_fail (icon_set != NULL, NULL);
  g_return_val_if_fail (CTK_IS_CSS_STYLE (style), NULL);

  effect = _ctk_css_icon_effect_value_get
    (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_ICON_EFFECT));

  switch (effect)
    {
    default:
      g_assert_not_reached ();
    case CTK_CSS_ICON_EFFECT_NONE:
      state = CTK_STATE_NORMAL;
      break;
    case CTK_CSS_ICON_EFFECT_HIGHLIGHT:
      state = CTK_STATE_PRELIGHT;
      break;
    case CTK_CSS_ICON_EFFECT_DIM:
      state = CTK_STATE_PRELIGHT;
      break;
    }

  if (icon_set->sources)
    icon = find_and_render_icon_source (icon_set, style, direction, state,
                                        size, scale);

  if (icon == NULL)
    icon = render_fallback_image (style, direction, state, size);

  return icon;
}

/**
 * ctk_icon_set_render_icon_pixbuf:
 * @icon_set: a #CtkIconSet
 * @context: a #CtkStyleContext
 * @size: (type int): icon size (#CtkIconSize). A size of `(CtkIconSize)-1`
 *        means render at the size of the source and don’t scale.
 *
 * Renders an icon using ctk_render_icon_pixbuf(). In most cases,
 * ctk_widget_render_icon_pixbuf() is better, since it automatically provides
 * most of the arguments from the current widget settings.  This
 * function never returns %NULL; if the icon can’t be rendered
 * (perhaps because an image file fails to load), a default "missing
 * image" icon will be returned instead.
 *
 * Returns: (transfer full): a #GdkPixbuf to be displayed
 *
 * Since: 3.0
 */
GdkPixbuf *
ctk_icon_set_render_icon_pixbuf (CtkIconSet        *icon_set,
                                 CtkStyleContext   *context,
                                 CtkIconSize        size)
{
  g_return_val_if_fail (icon_set != NULL, NULL);
  g_return_val_if_fail (CTK_IS_STYLE_CONTEXT (context), NULL);

  return ctk_icon_set_render_icon_pixbuf_for_scale (icon_set,
                                                    ctk_style_context_lookup_style (context),
                                                    ctk_style_context_get_direction (context),
                                                    size,
                                                    1);
}

/**
 * ctk_icon_set_render_icon_surface:
 * @icon_set: a #CtkIconSet
 * @context: a #CtkStyleContext
 * @size: (type int): icon size (#CtkIconSize). A size of `(CtkIconSize)-1`
 *        means render at the size of the source and don’t scale.
 * @scale: the window scale to render for
 * @for_window: (allow-none): #CdkWindow to optimize drawing for, or %NULL
 *
 * Renders an icon using ctk_render_icon_pixbuf() and converts it to a
 * cairo surface. 
 *
 * This function never returns %NULL; if the icon can’t be rendered
 * (perhaps because an image file fails to load), a default "missing
 * image" icon will be returned instead.
 *
 * Returns: (transfer full): a #cairo_surface_t to be displayed
 *
 * Since: 3.10
 */
cairo_surface_t *
ctk_icon_set_render_icon_surface  (CtkIconSet      *icon_set,
				   CtkStyleContext *context,
				   CtkIconSize      size,
				   gint             scale,
				   CdkWindow       *for_window)
{
  GdkPixbuf *pixbuf;
  cairo_surface_t *surface;

  pixbuf = ctk_icon_set_render_icon_pixbuf_for_scale (icon_set,
                                                      ctk_style_context_lookup_style (context),
                                                      ctk_style_context_get_direction (context),
                                                      size,
                                                      scale);

  surface = cdk_cairo_surface_create_from_pixbuf (pixbuf, scale, for_window);
  g_object_unref (pixbuf);

  return surface;
}

/**
 * ctk_icon_set_render_icon:
 * @icon_set: a #CtkIconSet
 * @style: (allow-none): a #CtkStyle associated with @widget, or %NULL
 * @direction: text direction
 * @state: widget state
 * @size: (type int): icon size (#CtkIconSize). A size of `(CtkIconSize)-1`
 *        means render at the size of the source and don’t scale.
 * @widget: (allow-none): widget that will display the icon, or %NULL.
 *          The only use that is typically made of this
 *          is to determine the appropriate #CdkScreen.
 * @detail: (allow-none): detail to pass to the theme engine, or %NULL.
 *          Note that passing a detail of anything but %NULL
 *          will disable caching.
 *
 * Renders an icon using ctk_style_render_icon(). In most cases,
 * ctk_widget_render_icon() is better, since it automatically provides
 * most of the arguments from the current widget settings.  This
 * function never returns %NULL; if the icon can’t be rendered
 * (perhaps because an image file fails to load), a default "missing
 * image" icon will be returned instead.
 *
 * Returns: (transfer full): a #GdkPixbuf to be displayed
 */
GdkPixbuf*
ctk_icon_set_render_icon (CtkIconSet        *icon_set,
                          CtkStyle          *style,
                          CtkTextDirection   direction,
                          CtkStateType       state,
                          CtkIconSize        size,
                          CtkWidget         *widget,
                          const char        *detail G_GNUC_UNUSED)
{
  GdkPixbuf *icon;
  CtkStyleContext *context = NULL;
  CtkStateFlags flags = 0;

  g_return_val_if_fail (icon_set != NULL, NULL);


  g_return_val_if_fail (style == NULL || CTK_IS_STYLE (style), NULL);

  if (style && ctk_style_has_context (style))
    {
      g_object_get (style, "context", &context, NULL);
      /* g_object_get returns a refed object */
      if (context)
        g_object_unref (context);
    }
  else if (widget)
    {
      context = ctk_widget_get_style_context (widget);
    }

  if (!context)
    return render_fallback_image (ctk_style_context_lookup_style (context), direction, state, size);

  ctk_style_context_save (context);

  switch (state)
    {
    case CTK_STATE_PRELIGHT:
      flags |= CTK_STATE_FLAG_PRELIGHT;
      break;
    case CTK_STATE_INSENSITIVE:
      flags |= CTK_STATE_FLAG_INSENSITIVE;
      break;
    default:
      break;
    }

  ctk_style_context_set_state (context, flags);
  ctk_style_context_set_direction (context, direction);

  icon = ctk_icon_set_render_icon_pixbuf (icon_set, context, size);

  ctk_style_context_restore (context);

  return icon;
}

/* Order sources by their "wildness", so that "wilder" sources are
 * greater than “specific” sources; for determining ordering,
 * direction beats state beats size.
 */

static int
icon_source_compare (gconstpointer ap, gconstpointer bp)
{
  const CtkIconSource *a = ap;
  const CtkIconSource *b = bp;

  if (!a->any_direction && b->any_direction)
    return -1;
  else if (a->any_direction && !b->any_direction)
    return 1;
  else if (!a->any_state && b->any_state)
    return -1;
  else if (a->any_state && !b->any_state)
    return 1;
  else if (!a->any_size && b->any_size)
    return -1;
  else if (a->any_size && !b->any_size)
    return 1;
  else
    return 0;
}

/**
 * ctk_icon_set_add_source:
 * @icon_set: a #CtkIconSet
 * @source: a #CtkIconSource
 *
 * Icon sets have a list of #CtkIconSource, which they use as base
 * icons for rendering icons in different states and sizes. Icons are
 * scaled, made to look insensitive, etc. in
 * ctk_icon_set_render_icon(), but #CtkIconSet needs base images to
 * work with. The base images and when to use them are described by
 * a #CtkIconSource.
 *
 * This function copies @source, so you can reuse the same source immediately
 * without affecting the icon set.
 *
 * An example of when you’d use this function: a web browser’s "Back
 * to Previous Page" icon might point in a different direction in
 * Hebrew and in English; it might look different when insensitive;
 * and it might change size depending on toolbar mode (small/large
 * icons). So a single icon set would contain all those variants of
 * the icon, and you might add a separate source for each one.
 *
 * You should nearly always add a “default” icon source with all
 * fields wildcarded, which will be used as a fallback if no more
 * specific source matches. #CtkIconSet always prefers more specific
 * icon sources to more generic icon sources. The order in which you
 * add the sources to the icon set does not matter.
 *
 * ctk_icon_set_new_from_pixbuf() creates a new icon set with a
 * default icon source based on the given pixbuf.
 */
void
ctk_icon_set_add_source (CtkIconSet          *icon_set,
                         const CtkIconSource *source)
{
  g_return_if_fail (icon_set != NULL);
  g_return_if_fail (source != NULL);

  if (source->type == CTK_ICON_SOURCE_EMPTY)
    {
      g_warning ("Useless empty CtkIconSource");
      return;
    }

  icon_set->sources = g_slist_insert_sorted (icon_set->sources,
                                             ctk_icon_source_copy (source),
                                             icon_source_compare);
}

/**
 * ctk_icon_set_get_sizes:
 * @icon_set: a #CtkIconSet
 * @sizes: (array length=n_sizes) (out) (type int): return location
 *     for array of sizes (#CtkIconSize)
 * @n_sizes: location to store number of elements in returned array
 *
 * Obtains a list of icon sizes this icon set can render. The returned
 * array must be freed with g_free().
 */
void
ctk_icon_set_get_sizes (CtkIconSet   *icon_set,
                        CtkIconSize **sizes,
                        gint         *n_sizes)
{
  GSList *tmp_list;
  gboolean all_sizes = FALSE;
  GSList *specifics = NULL;

  g_return_if_fail (icon_set != NULL);
  g_return_if_fail (sizes != NULL);
  g_return_if_fail (n_sizes != NULL);

  tmp_list = icon_set->sources;
  while (tmp_list != NULL)
    {
      CtkIconSource *source;

      source = tmp_list->data;

      if (source->any_size)
        {
          all_sizes = TRUE;
          break;
        }
      else
        specifics = g_slist_prepend (specifics, GINT_TO_POINTER (source->size));

      tmp_list = tmp_list->next;
    }

  if (all_sizes)
    {
      /* Need to find out what sizes exist */
      gint i;

      init_icon_sizes ();

      *sizes = g_new (CtkIconSize, icon_sizes_used);
      *n_sizes = icon_sizes_used - 1;

      i = 1;
      while (i < icon_sizes_used)
        {
          (*sizes)[i - 1] = icon_sizes[i].size;
          ++i;
        }
    }
  else
    {
      gint i;

      *n_sizes = g_slist_length (specifics);
      *sizes = g_new (CtkIconSize, *n_sizes);

      i = 0;
      tmp_list = specifics;
      while (tmp_list != NULL)
        {
          (*sizes)[i] = GPOINTER_TO_INT (tmp_list->data);

          ++i;
          tmp_list = tmp_list->next;
        }
    }

  g_slist_free (specifics);
}


/**
 * ctk_icon_source_new:
 *
 * Creates a new #CtkIconSource. A #CtkIconSource contains a #GdkPixbuf (or
 * image filename) that serves as the base image for one or more of the
 * icons in a #CtkIconSet, along with a specification for which icons in the
 * icon set will be based on that pixbuf or image file. An icon set contains
 * a set of icons that represent “the same” logical concept in different states,
 * different global text directions, and different sizes.
 *
 * So for example a web browser’s “Back to Previous Page” icon might
 * point in a different direction in Hebrew and in English; it might
 * look different when insensitive; and it might change size depending
 * on toolbar mode (small/large icons). So a single icon set would
 * contain all those variants of the icon. #CtkIconSet contains a list
 * of #CtkIconSource from which it can derive specific icon variants in
 * the set.
 *
 * In the simplest case, #CtkIconSet contains one source pixbuf from
 * which it derives all variants. The convenience function
 * ctk_icon_set_new_from_pixbuf() handles this case; if you only have
 * one source pixbuf, just use that function.
 *
 * If you want to use a different base pixbuf for different icon
 * variants, you create multiple icon sources, mark which variants
 * they’ll be used to create, and add them to the icon set with
 * ctk_icon_set_add_source().
 *
 * By default, the icon source has all parameters wildcarded. That is,
 * the icon source will be used as the base icon for any desired text
 * direction, widget state, or icon size.
 *
 * Returns: a new #CtkIconSource
 */
CtkIconSource*
ctk_icon_source_new (void)
{
  CtkIconSource *src;

  src = g_new0 (CtkIconSource, 1);

  src->direction = CTK_TEXT_DIR_NONE;
  src->size = CTK_ICON_SIZE_INVALID;
  src->state = CTK_STATE_NORMAL;

  src->any_direction = TRUE;
  src->any_state = TRUE;
  src->any_size = TRUE;

  return src;
}

/**
 * ctk_icon_source_copy:
 * @source: a #CtkIconSource
 *
 * Creates a copy of @source; mostly useful for language bindings.
 *
 * Returns: a new #CtkIconSource
 */
CtkIconSource*
ctk_icon_source_copy (const CtkIconSource *source)
{
  CtkIconSource *copy;

  g_return_val_if_fail (source != NULL, NULL);

  copy = g_new (CtkIconSource, 1);

  *copy = *source;

  switch (copy->type)
    {
    case CTK_ICON_SOURCE_EMPTY:
    case CTK_ICON_SOURCE_STATIC_ICON_NAME:
      break;
    case CTK_ICON_SOURCE_ICON_NAME:
      copy->source.icon_name = g_strdup (copy->source.icon_name);
      break;
    case CTK_ICON_SOURCE_FILENAME:
      copy->source.filename = g_strdup (copy->source.filename);
      if (copy->filename_pixbuf)
	g_object_ref (copy->filename_pixbuf);
      break;
    case CTK_ICON_SOURCE_PIXBUF:
      g_object_ref (copy->source.pixbuf);
      break;
    default:
      g_assert_not_reached();
    }

  return copy;
}

/**
 * ctk_icon_source_free:
 * @source: a #CtkIconSource
 *
 * Frees a dynamically-allocated icon source, along with its
 * filename, size, and pixbuf fields if those are not %NULL.
 */
void
ctk_icon_source_free (CtkIconSource *source)
{
  g_return_if_fail (source != NULL);

  icon_source_clear (source);
  g_free (source);
}

G_DEFINE_BOXED_TYPE (CtkIconSource, ctk_icon_source,
                     ctk_icon_source_copy,
                     ctk_icon_source_free)

static void
icon_source_clear (CtkIconSource *source)
{
  switch (source->type)
    {
    case CTK_ICON_SOURCE_EMPTY:
      break;
    case CTK_ICON_SOURCE_ICON_NAME:
      g_free (source->source.icon_name);
      /* fall thru */
    case CTK_ICON_SOURCE_STATIC_ICON_NAME:
      source->source.icon_name = NULL;
      break;
    case CTK_ICON_SOURCE_FILENAME:
      g_free (source->source.filename);
      source->source.filename = NULL;
      if (source->filename_pixbuf) 
	g_object_unref (source->filename_pixbuf);
      source->filename_pixbuf = NULL;
      break;
    case CTK_ICON_SOURCE_PIXBUF:
      g_object_unref (source->source.pixbuf);
      source->source.pixbuf = NULL;
      break;
    default:
      g_assert_not_reached();
    }

  source->type = CTK_ICON_SOURCE_EMPTY;
}

/**
 * ctk_icon_source_set_filename:
 * @source: a #CtkIconSource
 * @filename: (type filename): image file to use
 *
 * Sets the name of an image file to use as a base image when creating
 * icon variants for #CtkIconSet. The filename must be absolute.
 */
void
ctk_icon_source_set_filename (CtkIconSource *source,
			      const gchar   *filename)
{
  g_return_if_fail (source != NULL);
  g_return_if_fail (filename == NULL || g_path_is_absolute (filename));

  if (source->type == CTK_ICON_SOURCE_FILENAME &&
      source->source.filename == filename)
    return;

  icon_source_clear (source);

  if (filename != NULL)
    {
      source->type = CTK_ICON_SOURCE_FILENAME;
      source->source.filename = g_strdup (filename);
    }
}

/**
 * ctk_icon_source_set_icon_name:
 * @source: a #CtkIconSource
 * @icon_name: (allow-none): name of icon to use
 *
 * Sets the name of an icon to look up in the current icon theme
 * to use as a base image when creating icon variants for #CtkIconSet.
 */
void
ctk_icon_source_set_icon_name (CtkIconSource *source,
			       const gchar   *icon_name)
{
  g_return_if_fail (source != NULL);

  if (source->type == CTK_ICON_SOURCE_ICON_NAME &&
      source->source.icon_name == icon_name)
    return;

  icon_source_clear (source);

  if (icon_name != NULL)
    {
      source->type = CTK_ICON_SOURCE_ICON_NAME;
      source->source.icon_name = g_strdup (icon_name);
    }
}

/**
 * ctk_icon_source_set_pixbuf:
 * @source: a #CtkIconSource
 * @pixbuf: pixbuf to use as a source
 *
 * Sets a pixbuf to use as a base image when creating icon variants
 * for #CtkIconSet.
 */
void
ctk_icon_source_set_pixbuf (CtkIconSource *source,
                            GdkPixbuf     *pixbuf)
{
  g_return_if_fail (source != NULL);
  g_return_if_fail (pixbuf == NULL || GDK_IS_PIXBUF (pixbuf));

  if (source->type == CTK_ICON_SOURCE_PIXBUF &&
      source->source.pixbuf == pixbuf)
    return;

  icon_source_clear (source);

  if (pixbuf != NULL)
    {
      source->type = CTK_ICON_SOURCE_PIXBUF;
      source->source.pixbuf = g_object_ref (pixbuf);
    }
}

/**
 * ctk_icon_source_get_filename:
 * @source: a #CtkIconSource
 *
 * Retrieves the source filename, or %NULL if none is set. The
 * filename is not a copy, and should not be modified or expected to
 * persist beyond the lifetime of the icon source.
 *
 * Returns: (type filename): image filename. This string must not
 * be modified or freed.
 */
const gchar*
ctk_icon_source_get_filename (const CtkIconSource *source)
{
  g_return_val_if_fail (source != NULL, NULL);

  if (source->type == CTK_ICON_SOURCE_FILENAME)
    return source->source.filename;
  else
    return NULL;
}

/**
 * ctk_icon_source_get_icon_name:
 * @source: a #CtkIconSource
 *
 * Retrieves the source icon name, or %NULL if none is set. The
 * icon_name is not a copy, and should not be modified or expected to
 * persist beyond the lifetime of the icon source.
 *
 * Returns: icon name. This string must not be modified or freed.
 */
const gchar*
ctk_icon_source_get_icon_name (const CtkIconSource *source)
{
  g_return_val_if_fail (source != NULL, NULL);

  if (source->type == CTK_ICON_SOURCE_ICON_NAME ||
     source->type == CTK_ICON_SOURCE_STATIC_ICON_NAME)
    return source->source.icon_name;
  else
    return NULL;
}

/**
 * ctk_icon_source_get_pixbuf:
 * @source: a #CtkIconSource
 *
 * Retrieves the source pixbuf, or %NULL if none is set.
 * In addition, if a filename source is in use, this
 * function in some cases will return the pixbuf from
 * loaded from the filename. This is, for example, true
 * for the CtkIconSource passed to the #CtkStyle render_icon()
 * virtual function. The reference count on the pixbuf is
 * not incremented.
 *
 * Returns: (transfer none): source pixbuf
 */
GdkPixbuf*
ctk_icon_source_get_pixbuf (const CtkIconSource *source)
{
  g_return_val_if_fail (source != NULL, NULL);

  if (source->type == CTK_ICON_SOURCE_PIXBUF)
    return source->source.pixbuf;
  else if (source->type == CTK_ICON_SOURCE_FILENAME)
    return source->filename_pixbuf;
  else
    return NULL;
}

/**
 * ctk_icon_source_set_direction_wildcarded:
 * @source: a #CtkIconSource
 * @setting: %TRUE to wildcard the text direction
 *
 * If the text direction is wildcarded, this source can be used
 * as the base image for an icon in any #CtkTextDirection.
 * If the text direction is not wildcarded, then the
 * text direction the icon source applies to should be set
 * with ctk_icon_source_set_direction(), and the icon source
 * will only be used with that text direction.
 *
 * #CtkIconSet prefers non-wildcarded sources (exact matches) over
 * wildcarded sources, and will use an exact match when possible.
 */
void
ctk_icon_source_set_direction_wildcarded (CtkIconSource *source,
                                          gboolean       setting)
{
  g_return_if_fail (source != NULL);

  source->any_direction = setting != FALSE;
}

/**
 * ctk_icon_source_set_state_wildcarded:
 * @source: a #CtkIconSource
 * @setting: %TRUE to wildcard the widget state
 *
 * If the widget state is wildcarded, this source can be used as the
 * base image for an icon in any #CtkStateType.  If the widget state
 * is not wildcarded, then the state the source applies to should be
 * set with ctk_icon_source_set_state() and the icon source will
 * only be used with that specific state.
 *
 * #CtkIconSet prefers non-wildcarded sources (exact matches) over
 * wildcarded sources, and will use an exact match when possible.
 *
 * #CtkIconSet will normally transform wildcarded source images to
 * produce an appropriate icon for a given state, for example
 * lightening an image on prelight, but will not modify source images
 * that match exactly.
 */
void
ctk_icon_source_set_state_wildcarded (CtkIconSource *source,
                                      gboolean       setting)
{
  g_return_if_fail (source != NULL);

  source->any_state = setting != FALSE;
}


/**
 * ctk_icon_source_set_size_wildcarded:
 * @source: a #CtkIconSource
 * @setting: %TRUE to wildcard the widget state
 *
 * If the icon size is wildcarded, this source can be used as the base
 * image for an icon of any size.  If the size is not wildcarded, then
 * the size the source applies to should be set with
 * ctk_icon_source_set_size() and the icon source will only be used
 * with that specific size.
 *
 * #CtkIconSet prefers non-wildcarded sources (exact matches) over
 * wildcarded sources, and will use an exact match when possible.
 *
 * #CtkIconSet will normally scale wildcarded source images to produce
 * an appropriate icon at a given size, but will not change the size
 * of source images that match exactly.
 */
void
ctk_icon_source_set_size_wildcarded (CtkIconSource *source,
                                     gboolean       setting)
{
  g_return_if_fail (source != NULL);

  source->any_size = setting != FALSE;
}

/**
 * ctk_icon_source_get_size_wildcarded:
 * @source: a #CtkIconSource
 *
 * Gets the value set by ctk_icon_source_set_size_wildcarded().
 *
 * Returns: %TRUE if this icon source is a base for any icon size variant
 */
gboolean
ctk_icon_source_get_size_wildcarded (const CtkIconSource *source)
{
  g_return_val_if_fail (source != NULL, TRUE);

  return source->any_size;
}

/**
 * ctk_icon_source_get_state_wildcarded:
 * @source: a #CtkIconSource
 *
 * Gets the value set by ctk_icon_source_set_state_wildcarded().
 *
 * Returns: %TRUE if this icon source is a base for any widget state variant
 */
gboolean
ctk_icon_source_get_state_wildcarded (const CtkIconSource *source)
{
  g_return_val_if_fail (source != NULL, TRUE);

  return source->any_state;
}

/**
 * ctk_icon_source_get_direction_wildcarded:
 * @source: a #CtkIconSource
 *
 * Gets the value set by ctk_icon_source_set_direction_wildcarded().
 *
 * Returns: %TRUE if this icon source is a base for any text direction variant
 */
gboolean
ctk_icon_source_get_direction_wildcarded (const CtkIconSource *source)
{
  g_return_val_if_fail (source != NULL, TRUE);

  return source->any_direction;
}

/**
 * ctk_icon_source_set_direction:
 * @source: a #CtkIconSource
 * @direction: text direction this source applies to
 *
 * Sets the text direction this icon source is intended to be used
 * with.
 *
 * Setting the text direction on an icon source makes no difference
 * if the text direction is wildcarded. Therefore, you should usually
 * call ctk_icon_source_set_direction_wildcarded() to un-wildcard it
 * in addition to calling this function.
 */
void
ctk_icon_source_set_direction (CtkIconSource   *source,
                               CtkTextDirection direction)
{
  g_return_if_fail (source != NULL);

  source->direction = direction;
}

/**
 * ctk_icon_source_set_state:
 * @source: a #CtkIconSource
 * @state: widget state this source applies to
 *
 * Sets the widget state this icon source is intended to be used
 * with.
 *
 * Setting the widget state on an icon source makes no difference
 * if the state is wildcarded. Therefore, you should usually
 * call ctk_icon_source_set_state_wildcarded() to un-wildcard it
 * in addition to calling this function.
 */
void
ctk_icon_source_set_state (CtkIconSource *source,
                           CtkStateType   state)
{
  g_return_if_fail (source != NULL);

  source->state = state;
}

/**
 * ctk_icon_source_set_size:
 * @source: a #CtkIconSource
 * @size: (type int): icon size (#CtkIconSize) this source applies to
 *
 * Sets the icon size this icon source is intended to be used
 * with.
 *
 * Setting the icon size on an icon source makes no difference
 * if the size is wildcarded. Therefore, you should usually
 * call ctk_icon_source_set_size_wildcarded() to un-wildcard it
 * in addition to calling this function.
 */
void
ctk_icon_source_set_size (CtkIconSource *source,
                          CtkIconSize    size)
{
  g_return_if_fail (source != NULL);

  source->size = size;
}

/**
 * ctk_icon_source_get_direction:
 * @source: a #CtkIconSource
 *
 * Obtains the text direction this icon source applies to. The return
 * value is only useful/meaningful if the text direction is not
 * wildcarded.
 *
 * Returns: text direction this source matches
 */
CtkTextDirection
ctk_icon_source_get_direction (const CtkIconSource *source)
{
  g_return_val_if_fail (source != NULL, 0);

  return source->direction;
}

/**
 * ctk_icon_source_get_state:
 * @source: a #CtkIconSource
 *
 * Obtains the widget state this icon source applies to. The return
 * value is only useful/meaningful if the widget state is not
 * wildcarded.
 *
 * Returns: widget state this source matches
 */
CtkStateType
ctk_icon_source_get_state (const CtkIconSource *source)
{
  g_return_val_if_fail (source != NULL, 0);

  return source->state;
}

/**
 * ctk_icon_source_get_size:
 * @source: a #CtkIconSource
 *
 * Obtains the icon size this source applies to. The return value
 * is only useful/meaningful if the icon size is not wildcarded.
 *
 * Returns: (type int): icon size (#CtkIconSize) this source matches.
 */
CtkIconSize
ctk_icon_source_get_size (const CtkIconSource *source)
{
  g_return_val_if_fail (source != NULL, 0);

  return source->size;
}

/**
 * _ctk_icon_factory_list_ids:
 *
 * Gets all known IDs stored in an existing icon factory.
 * The strings in the returned list aren’t copied.
 * The list itself should be freed.
 *
 * Returns: List of ids in icon factories
 */
GList*
_ctk_icon_factory_list_ids (void)
{
  GSList *tmp_list;
  GList *ids;

  ids = NULL;

  _ctk_icon_factory_get_default_icons ();

  tmp_list = all_icon_factories;
  while (tmp_list != NULL)
    {
      GList *these_ids;
      CtkIconFactory *factory = CTK_ICON_FACTORY (tmp_list->data);
      CtkIconFactoryPrivate *priv = factory->priv;

      these_ids = g_hash_table_get_keys (priv->icons);

      ids = g_list_concat (ids, these_ids);

      tmp_list = tmp_list->next;
    }

  return ids;
}

typedef struct {
  GSList *sources;
  gboolean in_source;

} IconFactoryParserData;

typedef struct {
  gchar            *stock_id;
  gchar            *filename;
  gchar            *icon_name;
  CtkTextDirection  direction;
  CtkIconSize       size;
  CtkStateType      state;
} IconSourceParserData;

static void
icon_source_start_element (GMarkupParseContext  *context,
                           const gchar          *element_name,
                           const gchar         **names,
                           const gchar         **values,
                           gpointer              user_data,
                           GError              **error)
{
  gint i;
  gchar *stock_id = NULL;
  gchar *filename = NULL;
  gchar *icon_name = NULL;
  gint size = -1;
  gint direction = -1;
  gint state = -1;
  IconFactoryParserData *parser_data;
  IconSourceParserData *source_data;
  gchar *error_msg;
  GQuark error_domain;

  parser_data = (IconFactoryParserData*)user_data;

  if (!parser_data->in_source)
    {
      if (strcmp (element_name, "sources") != 0)
        {
          error_msg = g_strdup_printf ("Unexpected element %s, expected <sources>", element_name);
          error_domain = CTK_BUILDER_ERROR_INVALID_TAG;
          goto error;
        }
      parser_data->in_source = TRUE;
      return;
    }
  else
    {
      if (strcmp (element_name, "source") != 0)
        {
          error_msg = g_strdup_printf ("Unexpected element %s, expected <source>", element_name);
          error_domain = CTK_BUILDER_ERROR_INVALID_TAG;
          goto error;
        }
    }

  for (i = 0; names[i]; i++)
    {
      if (strcmp (names[i], "stock-id") == 0)
        stock_id = g_strdup (values[i]);
      else if (strcmp (names[i], "filename") == 0)
        filename = g_strdup (values[i]);
      else if (strcmp (names[i], "icon-name") == 0)
        icon_name = g_strdup (values[i]);
      else if (strcmp (names[i], "size") == 0)
        {
          if (!_ctk_builder_enum_from_string (CTK_TYPE_ICON_SIZE,
                                              values[i],
                                              &size,
                                              error))
            return;
        }
      else if (strcmp (names[i], "direction") == 0)
        {
          if (!_ctk_builder_enum_from_string (CTK_TYPE_TEXT_DIRECTION,
                                              values[i],
                                              &direction,
                                              error))
            return;
        }
      else if (strcmp (names[i], "state") == 0)
        {
          if (!_ctk_builder_enum_from_string (CTK_TYPE_STATE_TYPE,
                                              values[i],
                                              &state,
                                              error))
            return;
        }
      else
        {
          error_msg = g_strdup_printf ("'%s' is not a valid attribute of <%s>",
                                       names[i], "source");
          error_domain = CTK_BUILDER_ERROR_INVALID_ATTRIBUTE;
          goto error;
        }
    }

  if (!stock_id)
    {
      error_msg = g_strdup_printf ("<source> requires a stock_id");
      error_domain = CTK_BUILDER_ERROR_MISSING_ATTRIBUTE;
      goto error;
    }

  source_data = g_slice_new (IconSourceParserData);
  source_data->stock_id = stock_id;
  source_data->filename = filename;
  source_data->icon_name = icon_name;
  source_data->size = size;
  source_data->direction = direction;
  source_data->state = state;

  parser_data->sources = g_slist_prepend (parser_data->sources, source_data);
  return;

 error:
  {
    gchar *tmp;
    gint line_number, char_number;

    g_markup_parse_context_get_position (context, &line_number, &char_number);

    tmp = g_strdup_printf ("%s:%d:%d %s", "input",
                           line_number, char_number, error_msg);
    g_set_error_literal (error, CTK_BUILDER_ERROR, error_domain, tmp);
    g_free (tmp);
    g_free (stock_id);
    g_free (filename);
    g_free (icon_name);
    return;
  }
}

static const GMarkupParser icon_source_parser =
  {
    .start_element = icon_source_start_element,
  };

static gboolean
ctk_icon_factory_buildable_custom_tag_start (CtkBuildable     *buildable,
					     CtkBuilder       *builder G_GNUC_UNUSED,
					     GObject          *child G_GNUC_UNUSED,
					     const gchar      *tagname,
					     GMarkupParser    *parser,
					     gpointer         *data)
{
  g_assert (buildable);

  if (strcmp (tagname, "sources") == 0)
    {
      IconFactoryParserData *parser_data;

      parser_data = g_slice_new0 (IconFactoryParserData);
      *parser = icon_source_parser;
      *data = parser_data;
      return TRUE;
    }
  return FALSE;
}

static void
ctk_icon_factory_buildable_custom_tag_end (CtkBuildable *buildable,
					   CtkBuilder   *builder,
					   GObject      *child G_GNUC_UNUSED,
					   const gchar  *tagname,
					   gpointer     *user_data)
{
  CtkIconFactory *icon_factory;

  icon_factory = CTK_ICON_FACTORY (buildable);

  if (strcmp (tagname, "sources") == 0)
    {
      IconFactoryParserData *parser_data;
      CtkIconSource *icon_source;
      CtkIconSet *icon_set;
      GSList *l;

      parser_data = (IconFactoryParserData*)user_data;

      for (l = parser_data->sources; l; l = l->next)
	{
	  IconSourceParserData *source_data = l->data;

	  icon_set = ctk_icon_factory_lookup (icon_factory, source_data->stock_id);
	  if (!icon_set)
	    {
	      icon_set = ctk_icon_set_new ();
	      ctk_icon_factory_add (icon_factory, source_data->stock_id, icon_set);
              ctk_icon_set_unref (icon_set);
	    }

	  icon_source = ctk_icon_source_new ();

	  if (source_data->filename)
	    {
	      gchar *filename;
	      filename = _ctk_builder_get_absolute_filename (builder, source_data->filename);
	      ctk_icon_source_set_filename (icon_source, filename);
	      g_free (filename);
	    }
	  if (source_data->icon_name)
	    ctk_icon_source_set_icon_name (icon_source, source_data->icon_name);
	  if ((gint)source_data->size != -1)
            {
              ctk_icon_source_set_size (icon_source, source_data->size);
              ctk_icon_source_set_size_wildcarded (icon_source, FALSE);
            }
	  if ((gint)source_data->direction != -1)
            {
              ctk_icon_source_set_direction (icon_source, source_data->direction);
              ctk_icon_source_set_direction_wildcarded (icon_source, FALSE);
            }
	  if ((gint)source_data->state != -1)
            {
              ctk_icon_source_set_state (icon_source, source_data->state);
              ctk_icon_source_set_state_wildcarded (icon_source, FALSE);
            }

	  /* Inline source_add() to avoid creating a copy */
	  g_assert (icon_source->type != CTK_ICON_SOURCE_EMPTY);
	  icon_set->sources = g_slist_insert_sorted (icon_set->sources,
						     icon_source,
						     icon_source_compare);

	  g_free (source_data->stock_id);
	  g_free (source_data->filename);
	  g_free (source_data->icon_name);
	  g_slice_free (IconSourceParserData, source_data);
	}
      g_slist_free (parser_data->sources);
      g_slice_free (IconFactoryParserData, parser_data);

      /* TODO: Add an attribute/tag to prevent this.
       * Usually it's the right thing to do though.
       */
      ctk_icon_factory_add_default (icon_factory);
    }
}
