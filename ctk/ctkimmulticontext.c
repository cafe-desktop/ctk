/* CTK - The GIMP Toolkit
 * Copyright (C) 2000 Red Hat, Inc.
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
#include <locale.h>

#include "ctkimmulticontext.h"
#include "ctkimmoduleprivate.h"
#include "ctkintl.h"
#include "ctklabel.h"
#include "ctkmain.h"
#include "ctkprivate.h"
#include "ctkradiomenuitem.h"
#include "ctkseparatormenuitem.h"
#include "ctksettings.h"


/**
 * SECTION:ctkimmulticontext
 * @Short_description: An input method context supporting multiple, loadable input methods
 * @Title: CtkIMMulticontext
 */


struct _CtkIMMulticontextPrivate
{
  CtkIMContext          *slave;

  GdkWindow             *client_window;
  GdkRectangle           cursor_location;

  gchar                 *context_id;
  gchar                 *context_id_aux;

  guint                  use_preedit          : 1;
  guint                  have_cursor_location : 1;
  guint                  focus_in             : 1;
};

static void     ctk_im_multicontext_notify             (GObject                 *object,
                                                        GParamSpec              *pspec);
static void     ctk_im_multicontext_finalize           (GObject                 *object);

static void     ctk_im_multicontext_set_slave          (CtkIMMulticontext       *multicontext,
							CtkIMContext            *slave,
							gboolean                 finalizing);

static void     ctk_im_multicontext_set_client_window  (CtkIMContext            *context,
							GdkWindow               *window);
static void     ctk_im_multicontext_get_preedit_string (CtkIMContext            *context,
							gchar                  **str,
							PangoAttrList          **attrs,
							gint                   *cursor_pos);
static gboolean ctk_im_multicontext_filter_keypress    (CtkIMContext            *context,
							GdkEventKey             *event);
static void     ctk_im_multicontext_focus_in           (CtkIMContext            *context);
static void     ctk_im_multicontext_focus_out          (CtkIMContext            *context);
static void     ctk_im_multicontext_reset              (CtkIMContext            *context);
static void     ctk_im_multicontext_set_cursor_location (CtkIMContext            *context,
							GdkRectangle		*area);
static void     ctk_im_multicontext_set_use_preedit    (CtkIMContext            *context,
							gboolean                 use_preedit);
static gboolean ctk_im_multicontext_get_surrounding    (CtkIMContext            *context,
							gchar                  **text,
							gint                    *cursor_index);
static void     ctk_im_multicontext_set_surrounding    (CtkIMContext            *context,
							const char              *text,
							gint                     len,
							gint                     cursor_index);

static void     ctk_im_multicontext_preedit_start_cb        (CtkIMContext      *slave,
							     CtkIMMulticontext *multicontext);
static void     ctk_im_multicontext_preedit_end_cb          (CtkIMContext      *slave,
							     CtkIMMulticontext *multicontext);
static void     ctk_im_multicontext_preedit_changed_cb      (CtkIMContext      *slave,
							     CtkIMMulticontext *multicontext);
static void     ctk_im_multicontext_commit_cb               (CtkIMContext      *slave,
							     const gchar       *str,
							     CtkIMMulticontext *multicontext);
static gboolean ctk_im_multicontext_retrieve_surrounding_cb (CtkIMContext      *slave,
							     CtkIMMulticontext *multicontext);
static gboolean ctk_im_multicontext_delete_surrounding_cb   (CtkIMContext      *slave,
							     gint               offset,
							     gint               n_chars,
							     CtkIMMulticontext *multicontext);

static void propagate_purpose (CtkIMMulticontext *context);

static const gchar *global_context_id = NULL;

G_DEFINE_TYPE_WITH_PRIVATE (CtkIMMulticontext, ctk_im_multicontext, CTK_TYPE_IM_CONTEXT)

static void
ctk_im_multicontext_class_init (CtkIMMulticontextClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  CtkIMContextClass *im_context_class = CTK_IM_CONTEXT_CLASS (class);

  gobject_class->notify = ctk_im_multicontext_notify;

  im_context_class->set_client_window = ctk_im_multicontext_set_client_window;
  im_context_class->get_preedit_string = ctk_im_multicontext_get_preedit_string;
  im_context_class->filter_keypress = ctk_im_multicontext_filter_keypress;
  im_context_class->focus_in = ctk_im_multicontext_focus_in;
  im_context_class->focus_out = ctk_im_multicontext_focus_out;
  im_context_class->reset = ctk_im_multicontext_reset;
  im_context_class->set_cursor_location = ctk_im_multicontext_set_cursor_location;
  im_context_class->set_use_preedit = ctk_im_multicontext_set_use_preedit;
  im_context_class->set_surrounding = ctk_im_multicontext_set_surrounding;
  im_context_class->get_surrounding = ctk_im_multicontext_get_surrounding;

  gobject_class->finalize = ctk_im_multicontext_finalize;
}

static void
ctk_im_multicontext_init (CtkIMMulticontext *multicontext)
{
  CtkIMMulticontextPrivate *priv;
  
  multicontext->priv = ctk_im_multicontext_get_instance_private (multicontext);
  priv = multicontext->priv;

  priv->slave = NULL;
  priv->use_preedit = TRUE;
  priv->have_cursor_location = FALSE;
  priv->focus_in = FALSE;
}

/**
 * ctk_im_multicontext_new:
 *
 * Creates a new #CtkIMMulticontext.
 *
 * Returns: a new #CtkIMMulticontext.
 **/
CtkIMContext *
ctk_im_multicontext_new (void)
{
  return g_object_new (CTK_TYPE_IM_MULTICONTEXT, NULL);
}

static void
ctk_im_multicontext_finalize (GObject *object)
{
  CtkIMMulticontext *multicontext = CTK_IM_MULTICONTEXT (object);
  CtkIMMulticontextPrivate *priv = multicontext->priv;

  ctk_im_multicontext_set_slave (multicontext, NULL, TRUE);
  g_free (priv->context_id);
  g_free (priv->context_id_aux);

  G_OBJECT_CLASS (ctk_im_multicontext_parent_class)->finalize (object);
}

static void
ctk_im_multicontext_set_slave (CtkIMMulticontext *multicontext,
			       CtkIMContext      *slave,
			       gboolean           finalizing)
{
  CtkIMMulticontextPrivate *priv = multicontext->priv;
  gboolean need_preedit_changed = FALSE;
  
  if (priv->slave)
    {
      if (!finalizing)
	ctk_im_context_reset (priv->slave);
      
      g_signal_handlers_disconnect_by_func (priv->slave,
					    ctk_im_multicontext_preedit_start_cb,
					    multicontext);
      g_signal_handlers_disconnect_by_func (priv->slave,
					    ctk_im_multicontext_preedit_end_cb,
					    multicontext);
      g_signal_handlers_disconnect_by_func (priv->slave,
					    ctk_im_multicontext_preedit_changed_cb,
					    multicontext);
      g_signal_handlers_disconnect_by_func (priv->slave,
					    ctk_im_multicontext_commit_cb,
					    multicontext);
      g_signal_handlers_disconnect_by_func (priv->slave,
					    ctk_im_multicontext_retrieve_surrounding_cb,
					    multicontext);
      g_signal_handlers_disconnect_by_func (priv->slave,
					    ctk_im_multicontext_delete_surrounding_cb,
					    multicontext);

      g_object_unref (priv->slave);
      priv->slave = NULL;

      if (!finalizing)
	need_preedit_changed = TRUE;
    }

  priv->slave = slave;

  if (priv->slave)
    {
      g_object_ref (priv->slave);

      propagate_purpose (multicontext);

      g_signal_connect (priv->slave, "preedit-start",
			G_CALLBACK (ctk_im_multicontext_preedit_start_cb),
			multicontext);
      g_signal_connect (priv->slave, "preedit-end",
			G_CALLBACK (ctk_im_multicontext_preedit_end_cb),
			multicontext);
      g_signal_connect (priv->slave, "preedit-changed",
			G_CALLBACK (ctk_im_multicontext_preedit_changed_cb),
			multicontext);
      g_signal_connect (priv->slave, "commit",
			G_CALLBACK (ctk_im_multicontext_commit_cb),
			multicontext);
      g_signal_connect (priv->slave, "retrieve-surrounding",
			G_CALLBACK (ctk_im_multicontext_retrieve_surrounding_cb),
			multicontext);
      g_signal_connect (priv->slave, "delete-surrounding",
			G_CALLBACK (ctk_im_multicontext_delete_surrounding_cb),
			multicontext);

      if (!priv->use_preedit)	/* Default is TRUE */
	ctk_im_context_set_use_preedit (slave, FALSE);
      if (priv->client_window)
	ctk_im_context_set_client_window (slave, priv->client_window);
      if (priv->have_cursor_location)
	ctk_im_context_set_cursor_location (slave, &priv->cursor_location);
      if (priv->focus_in)
	ctk_im_context_focus_in (slave);
    }

  if (need_preedit_changed)
    g_signal_emit_by_name (multicontext, "preedit-changed");
}

static const gchar *
get_effective_context_id (CtkIMMulticontext *multicontext)
{
  CtkIMMulticontextPrivate *priv = multicontext->priv;

  if (priv->context_id_aux)
    return priv->context_id_aux;

  if (!global_context_id)
    global_context_id = _ctk_im_module_get_default_context_id ();

  return global_context_id;
}

static CtkIMContext *
ctk_im_multicontext_get_slave (CtkIMMulticontext *multicontext)
{
  CtkIMMulticontextPrivate *priv = multicontext->priv;

  if (g_strcmp0 (priv->context_id, get_effective_context_id (multicontext)) != 0)
    ctk_im_multicontext_set_slave (multicontext, NULL, FALSE);

  if (!priv->slave)
    {
      CtkIMContext *slave;

      g_free (priv->context_id);

      priv->context_id = g_strdup (get_effective_context_id (multicontext));

      slave = _ctk_im_module_create (priv->context_id);
      if (slave)
        {
          ctk_im_multicontext_set_slave (multicontext, slave, FALSE);
          g_object_unref (slave);
        }
    }

  return priv->slave;
}

static void
im_module_setting_changed (CtkSettings *settings, 
                           gpointer     data)
{
  global_context_id = NULL;
}


static void
ctk_im_multicontext_set_client_window (CtkIMContext *context,
				       GdkWindow    *window)
{
  CtkIMMulticontext *multicontext = CTK_IM_MULTICONTEXT (context);
  CtkIMMulticontextPrivate *priv = multicontext->priv;
  CtkIMContext *slave;
  GdkScreen *screen;
  CtkSettings *settings;
  gboolean connected;

  priv->client_window = window;

  if (window)
    {
      screen = gdk_window_get_screen (window);
      settings = ctk_settings_get_for_screen (screen);

      connected = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (settings),
                                                      "ctk-im-module-connected"));
      if (!connected)
        {
          g_signal_connect (settings, "notify::ctk-im-module",
                            G_CALLBACK (im_module_setting_changed), NULL);
          g_object_set_data (G_OBJECT (settings), "ctk-im-module-connected",
                             GINT_TO_POINTER (TRUE));

          global_context_id = NULL;
        }
    }

  slave = ctk_im_multicontext_get_slave (multicontext);
  if (slave)
    ctk_im_context_set_client_window (slave, window);
}

static void
ctk_im_multicontext_get_preedit_string (CtkIMContext   *context,
					gchar         **str,
					PangoAttrList **attrs,
					gint           *cursor_pos)
{
  CtkIMMulticontext *multicontext = CTK_IM_MULTICONTEXT (context);
  CtkIMContext *slave = ctk_im_multicontext_get_slave (multicontext);

  if (slave)
    ctk_im_context_get_preedit_string (slave, str, attrs, cursor_pos);
  else
    {
      if (str)
	*str = g_strdup ("");
      if (attrs)
	*attrs = pango_attr_list_new ();
    }
}

static gboolean
ctk_im_multicontext_filter_keypress (CtkIMContext *context,
				     GdkEventKey  *event)
{
  CtkIMMulticontext *multicontext = CTK_IM_MULTICONTEXT (context);
  CtkIMContext *slave = ctk_im_multicontext_get_slave (multicontext);

  if (slave)
    {
      return ctk_im_context_filter_keypress (slave, event);
    }
  else
    {
      GdkDisplay *display;
      GdkModifierType no_text_input_mask;

      display = gdk_window_get_display (event->window);

      no_text_input_mask =
        gdk_keymap_get_modifier_mask (gdk_keymap_get_for_display (display),
                                      GDK_MODIFIER_INTENT_NO_TEXT_INPUT);

      if (event->type == GDK_KEY_PRESS &&
          (event->state & no_text_input_mask) == 0)
        {
          gunichar ch;

          ch = gdk_keyval_to_unicode (event->keyval);
          if (ch != 0 && !g_unichar_iscntrl (ch))
            {
              gint len;
              gchar buf[10];

              len = g_unichar_to_utf8 (ch, buf);
              buf[len] = '\0';

              g_signal_emit_by_name (multicontext, "commit", buf);

              return TRUE;
            }
        }
    }

  return FALSE;
}

static void
ctk_im_multicontext_focus_in (CtkIMContext   *context)
{
  CtkIMMulticontext *multicontext = CTK_IM_MULTICONTEXT (context);
  CtkIMMulticontextPrivate *priv = multicontext->priv;
  CtkIMContext *slave = ctk_im_multicontext_get_slave (multicontext);

  priv->focus_in = TRUE;

  if (slave)
    ctk_im_context_focus_in (slave);
}

static void
ctk_im_multicontext_focus_out (CtkIMContext   *context)
{
  CtkIMMulticontext *multicontext = CTK_IM_MULTICONTEXT (context);
  CtkIMMulticontextPrivate *priv = multicontext->priv;
  CtkIMContext *slave = ctk_im_multicontext_get_slave (multicontext);

  priv->focus_in = FALSE;

  if (slave)
    ctk_im_context_focus_out (slave);
}

static void
ctk_im_multicontext_reset (CtkIMContext   *context)
{
  CtkIMMulticontext *multicontext = CTK_IM_MULTICONTEXT (context);
  CtkIMContext *slave = ctk_im_multicontext_get_slave (multicontext);

  if (slave)
    ctk_im_context_reset (slave);
}

static void
ctk_im_multicontext_set_cursor_location (CtkIMContext   *context,
					 GdkRectangle   *area)
{
  CtkIMMulticontext *multicontext = CTK_IM_MULTICONTEXT (context);
  CtkIMMulticontextPrivate *priv = multicontext->priv;
  CtkIMContext *slave = ctk_im_multicontext_get_slave (multicontext);

  priv->have_cursor_location = TRUE;
  priv->cursor_location = *area;

  if (slave)
    ctk_im_context_set_cursor_location (slave, area);
}

static void
ctk_im_multicontext_set_use_preedit (CtkIMContext   *context,
				     gboolean	    use_preedit)
{
  CtkIMMulticontext *multicontext = CTK_IM_MULTICONTEXT (context);
  CtkIMMulticontextPrivate *priv = multicontext->priv;
  CtkIMContext *slave = ctk_im_multicontext_get_slave (multicontext);

  use_preedit = use_preedit != FALSE;

  priv->use_preedit = use_preedit;

  if (slave)
    ctk_im_context_set_use_preedit (slave, use_preedit);
}

static gboolean
ctk_im_multicontext_get_surrounding (CtkIMContext  *context,
				     gchar        **text,
				     gint          *cursor_index)
{
  CtkIMMulticontext *multicontext = CTK_IM_MULTICONTEXT (context);
  CtkIMContext *slave = ctk_im_multicontext_get_slave (multicontext);

  if (slave)
    return ctk_im_context_get_surrounding (slave, text, cursor_index);
  else
    {
      if (text)
	*text = NULL;
      if (cursor_index)
	*cursor_index = 0;

      return FALSE;
    }
}

static void
ctk_im_multicontext_set_surrounding (CtkIMContext *context,
				     const char   *text,
				     gint          len,
				     gint          cursor_index)
{
  CtkIMMulticontext *multicontext = CTK_IM_MULTICONTEXT (context);
  CtkIMContext *slave = ctk_im_multicontext_get_slave (multicontext);

  if (slave)
    ctk_im_context_set_surrounding (slave, text, len, cursor_index);
}

static void
ctk_im_multicontext_preedit_start_cb   (CtkIMContext      *slave,
					CtkIMMulticontext *multicontext)
{
  g_signal_emit_by_name (multicontext, "preedit-start");
}

static void
ctk_im_multicontext_preedit_end_cb (CtkIMContext      *slave,
				    CtkIMMulticontext *multicontext)
{
  g_signal_emit_by_name (multicontext, "preedit-end");
}

static void
ctk_im_multicontext_preedit_changed_cb (CtkIMContext      *slave,
					CtkIMMulticontext *multicontext)
{
  g_signal_emit_by_name (multicontext, "preedit-changed");
}

static void
ctk_im_multicontext_commit_cb (CtkIMContext      *slave,
			       const gchar       *str,
			       CtkIMMulticontext *multicontext)
{
  g_signal_emit_by_name (multicontext, "commit", str);
}

static gboolean
ctk_im_multicontext_retrieve_surrounding_cb (CtkIMContext      *slave,
					     CtkIMMulticontext *multicontext)
{
  gboolean result;
  
  g_signal_emit_by_name (multicontext, "retrieve-surrounding", &result);

  return result;
}

static gboolean
ctk_im_multicontext_delete_surrounding_cb (CtkIMContext      *slave,
					   gint               offset,
					   gint               n_chars,
					   CtkIMMulticontext *multicontext)
{
  gboolean result;
  
  g_signal_emit_by_name (multicontext, "delete-surrounding",
			 offset, n_chars, &result);

  return result;
}

static void
activate_cb (CtkWidget         *menuitem,
	     CtkIMMulticontext *context)
{
  if (ctk_check_menu_item_get_active (CTK_CHECK_MENU_ITEM (menuitem)))
    {
      const gchar *id = g_object_get_data (G_OBJECT (menuitem), "ctk-context-id");

      ctk_im_multicontext_set_context_id (context, id);
    }
}

static int
pathnamecmp (const char *a,
	     const char *b)
{
#ifndef G_OS_WIN32
  return strcmp (a, b);
#else
  /* Ignore case insensitivity, probably not that relevant here. Just
   * make sure slash and backslash compare equal.
   */
  while (*a && *b)
    if ((G_IS_DIR_SEPARATOR (*a) && G_IS_DIR_SEPARATOR (*b)) ||
	*a == *b)
      a++, b++;
    else
      return (*a - *b);
  return (*a - *b);
#endif
}

/**
 * ctk_im_multicontext_append_menuitems:
 * @context: a #CtkIMMulticontext
 * @menushell: a #CtkMenuShell
 * 
 * Add menuitems for various available input methods to a menu;
 * the menuitems, when selected, will switch the input method
 * for the context and the global default input method.
 *
 * Deprecated: 3.10: It is better to use the system-wide input
 *     method framework for changing input methods. Modern
 *     desktop shells offer on-screen displays for this that
 *     can triggered with a keyboard shortcut, e.g. Super-Space.
 **/
void
ctk_im_multicontext_append_menuitems (CtkIMMulticontext *context,
				      CtkMenuShell      *menushell)
{
  CtkIMMulticontextPrivate *priv = context->priv;
  const CtkIMContextInfo **contexts;
  guint n_contexts, i;
  GSList *group = NULL;
  CtkWidget *menuitem, *system_menuitem;
  const char *system_context_id; 

  system_context_id = _ctk_im_module_get_default_context_id ();
  system_menuitem = menuitem = ctk_radio_menu_item_new_with_label (group, C_("input method menu", "System"));
  if (!priv->context_id_aux)
    ctk_check_menu_item_set_active (CTK_CHECK_MENU_ITEM (menuitem), TRUE);
  group = ctk_radio_menu_item_get_group (CTK_RADIO_MENU_ITEM (menuitem));
  g_object_set_data (G_OBJECT (menuitem), I_("ctk-context-id"), NULL);
  g_signal_connect (menuitem, "activate", G_CALLBACK (activate_cb), context);

  ctk_widget_show (menuitem);
  ctk_menu_shell_append (menushell, menuitem);

  _ctk_im_module_list (&contexts, &n_contexts);

  for (i = 0; i < n_contexts; i++)
    {
      const gchar *translated_name;
#ifdef ENABLE_NLS
      if (contexts[i]->domain && contexts[i]->domain[0])
	{
	  if (strcmp (contexts[i]->domain, GETTEXT_PACKAGE) == 0)
	    {
	      /* Same translation domain as CTK+ */
	      if (!(contexts[i]->domain_dirname && contexts[i]->domain_dirname[0]) ||
		  pathnamecmp (contexts[i]->domain_dirname, _ctk_get_localedir ()) == 0)
		{
		  /* Empty or NULL, domain directory, or same as
		   * CTK+. Input method may have a name in the CTK+
		   * message catalog.
		   */
		  translated_name = g_dpgettext2 (GETTEXT_PACKAGE, "input method menu", contexts[i]->context_name);
		}
	      else
		{
		  /* Separate domain directory but the same
		   * translation domain as CTK+. We can't call
		   * bindtextdomain() as that would make CTK+ forget
		   * its own messages.
		   */
		  g_warning ("Input method %s should not use CTK's translation domain %s",
			     contexts[i]->context_id, GETTEXT_PACKAGE);
		  /* Try translating the name in CTK+'s domain */
		  translated_name = g_dpgettext2 (GETTEXT_PACKAGE, "input method menu", contexts[i]->context_name);
		}
	    }
	  else if (contexts[i]->domain_dirname && contexts[i]->domain_dirname[0])
	    /* Input method has own translation domain and message catalog */
	    {
	      bindtextdomain (contexts[i]->domain,
			      contexts[i]->domain_dirname);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
	      bind_textdomain_codeset (contexts[i]->domain, "UTF-8");
#endif
	      translated_name = g_dgettext (contexts[i]->domain, contexts[i]->context_name);
	    }
	  else
	    {
	      /* Different translation domain, but no domain directory */
	      translated_name = contexts[i]->context_name;
	    }
	}
      else
	/* Empty or NULL domain. We assume that input method does not
	 * want a translated name in this case.
	 */
	translated_name = contexts[i]->context_name;
#else
      translated_name = contexts[i]->context_name;
#endif
      menuitem = ctk_radio_menu_item_new_with_label (group, translated_name);

      if ((priv->context_id_aux &&
           strcmp (contexts[i]->context_id, priv->context_id_aux) == 0))
        ctk_check_menu_item_set_active (CTK_CHECK_MENU_ITEM (menuitem), TRUE);

      if (strcmp (contexts[i]->context_id, system_context_id) == 0)
        {
          CtkWidget *label;
          char *text;

          label = ctk_bin_get_child (CTK_BIN (system_menuitem));
          text = g_strdup_printf (C_("input method menu", "System (%s)"), translated_name);
          ctk_label_set_text (CTK_LABEL (label), text);
          g_free (text);
        }

      group = ctk_radio_menu_item_get_group (CTK_RADIO_MENU_ITEM (menuitem));

      g_object_set_data (G_OBJECT (menuitem), I_("ctk-context-id"),
			 (char *)contexts[i]->context_id);
      g_signal_connect (menuitem, "activate",
			G_CALLBACK (activate_cb), context);

      ctk_widget_show (menuitem);
      ctk_menu_shell_append (menushell, menuitem);
    }

  g_free (contexts);
}

/**
 * ctk_im_multicontext_get_context_id:
 * @context: a #CtkIMMulticontext
 *
 * Gets the id of the currently active slave of the @context.
 *
 * Returns: the id of the currently active slave
 *
 * Since: 2.16
 */
const char *
ctk_im_multicontext_get_context_id (CtkIMMulticontext *context)
{
  g_return_val_if_fail (CTK_IS_IM_MULTICONTEXT (context), NULL);

  return context->priv->context_id;
}

/**
 * ctk_im_multicontext_set_context_id:
 * @context: a #CtkIMMulticontext
 * @context_id: the id to use 
 *
 * Sets the context id for @context.
 *
 * This causes the currently active slave of @context to be
 * replaced by the slave corresponding to the new context id.
 *
 * Since: 2.16
 */
void
ctk_im_multicontext_set_context_id (CtkIMMulticontext *context,
                                    const char        *context_id)
{
  CtkIMMulticontextPrivate *priv;

  g_return_if_fail (CTK_IS_IM_MULTICONTEXT (context));

  priv = context->priv;

  ctk_im_context_reset (CTK_IM_CONTEXT (context));
  g_free (priv->context_id_aux);
  priv->context_id_aux = g_strdup (context_id);
  ctk_im_multicontext_set_slave (context, NULL, FALSE);
}

static void
propagate_purpose (CtkIMMulticontext *context)
{
  CtkInputPurpose purpose;
  CtkInputHints hints;

  if (context->priv->slave == NULL)
    return;

  g_object_get (context, "input-purpose", &purpose, NULL);
  g_object_set (context->priv->slave, "input-purpose", purpose, NULL);

  g_object_get (context, "input-hints", &hints, NULL);
  g_object_set (context->priv->slave, "input-hints", hints, NULL);
}

static void
ctk_im_multicontext_notify (GObject      *object,
                            GParamSpec   *pspec)
{
  propagate_purpose (CTK_IM_MULTICONTEXT (object));
}
