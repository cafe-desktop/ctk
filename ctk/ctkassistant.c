/*
 * CTK - The GIMP Toolkit
 * Copyright (C) 1999  Red Hat, Inc.
 * Copyright (C) 2002  Anders Carlsson <andersca@gnu.org>
 * Copyright (C) 2003  Matthias Clasen <mclasen@redhat.com>
 * Copyright (C) 2005  Carlos Garnacho Parro <carlosg@gnome.org>
 *
 * All rights reserved.
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

/**
 * SECTION:ctkassistant
 * @Short_description: A widget used to guide users through multi-step operations
 * @Title: CtkAssistant
 *
 * A #CtkAssistant is a widget used to represent a generally complex
 * operation splitted in several steps, guiding the user through its
 * pages and controlling the page flow to collect the necessary data.
 *
 * The design of CtkAssistant is that it controls what buttons to show
 * and to make sensitive, based on what it knows about the page sequence
 * and the [type][CtkAssistantPageType] of each page,
 * in addition to state information like the page
 * [completion][ctk-assistant-set-page-complete]
 * and [committed][ctk-assistant-commit] status.
 *
 * If you have a case that doesn’t quite fit in #CtkAssistants way of
 * handling buttons, you can use the #CTK_ASSISTANT_PAGE_CUSTOM page
 * type and handle buttons yourself.
 *
 * # CtkAssistant as CtkBuildable
 *
 * The CtkAssistant implementation of the #CtkBuildable interface
 * exposes the @action_area as internal children with the name
 * “action_area”.
 *
 * To add pages to an assistant in #CtkBuilder, simply add it as a
 * child to the CtkAssistant object, and set its child properties
 * as necessary.
 *
 * # CSS nodes
 *
 * CtkAssistant has a single CSS node with the name assistant.
 */

#include "config.h"

#include <atk/atk.h>

#include "ctkassistant.h"

#include "ctkbutton.h"
#include "ctkbox.h"
#include "ctkframe.h"
#include "ctknotebook.h"
#include "ctkimage.h"
#include "ctklabel.h"
#include "ctksettings.h"
#include "ctksizegroup.h"
#include "ctksizerequest.h"
#include "ctktypebuiltins.h"
#include "ctkintl.h"
#include "ctkprivate.h"
#include "ctkbuildable.h"
#include "a11y/ctkwindowaccessible.h"


#define HEADER_SPACING 12
#define ACTION_AREA_SPACING 12

typedef struct _CtkAssistantPage CtkAssistantPage;

struct _CtkAssistantPage
{
  CtkAssistantPageType type;
  guint      complete     : 1;
  guint      complete_set : 1;
  guint      has_padding  : 1;

  gchar *title;

  CtkWidget *box;
  CtkWidget *page;
  CtkWidget *regular_title;
  CtkWidget *current_title;
  GdkPixbuf *header_image;
  GdkPixbuf *sidebar_image;
};

struct _CtkAssistantPrivate
{
  CtkWidget *cancel;
  CtkWidget *forward;
  CtkWidget *back;
  CtkWidget *apply;
  CtkWidget *close;
  CtkWidget *last;

  CtkWidget *sidebar;
  CtkWidget *content;
  CtkWidget *action_area;
  CtkWidget *headerbar;
  gint use_header_bar;
  gboolean constructed;

  GList     *pages;
  GSList    *visited_pages;
  CtkAssistantPage *current_page;

  CtkSizeGroup *button_size_group;
  CtkSizeGroup *title_size_group;

  CtkAssistantPageFunc forward_function;
  gpointer forward_function_data;
  GDestroyNotify forward_data_destroy;

  gint extra_buttons;

  guint committed : 1;
};

static void     ctk_assistant_class_init         (CtkAssistantClass *class);
static void     ctk_assistant_init               (CtkAssistant      *assistant);
static void     ctk_assistant_destroy            (CtkWidget         *widget);
static void     ctk_assistant_map                (CtkWidget         *widget);
static void     ctk_assistant_unmap              (CtkWidget         *widget);
static gboolean ctk_assistant_delete_event       (CtkWidget         *widget,
                                                  CdkEventAny       *event);
static void     ctk_assistant_add                (CtkContainer      *container,
                                                  CtkWidget         *page);
static void     ctk_assistant_remove             (CtkContainer      *container,
                                                  CtkWidget         *page);
static void     ctk_assistant_set_child_property (CtkContainer      *container,
                                                  CtkWidget         *child,
                                                  guint              property_id,
                                                  const GValue      *value,
                                                  GParamSpec        *pspec);
static void     ctk_assistant_get_child_property (CtkContainer      *container,
                                                  CtkWidget         *child,
                                                  guint              property_id,
                                                  GValue            *value,
                                                  GParamSpec        *pspec);

static void       ctk_assistant_buildable_interface_init     (CtkBuildableIface *iface);
static gboolean   ctk_assistant_buildable_custom_tag_start   (CtkBuildable  *buildable,
                                                              CtkBuilder    *builder,
                                                              GObject       *child,
                                                              const gchar   *tagname,
                                                              GMarkupParser *parser,
                                                              gpointer      *data);
static void       ctk_assistant_buildable_custom_finished    (CtkBuildable  *buildable,
                                                              CtkBuilder    *builder,
                                                              GObject       *child,
                                                              const gchar   *tagname,
                                                              gpointer       user_data);

static GList*     find_page                                  (CtkAssistant  *assistant,
                                                              CtkWidget     *page);
static void       ctk_assistant_do_set_page_header_image     (CtkAssistant  *assistant,
                                                              CtkWidget     *page,
                                                              GdkPixbuf     *pixbuf);
static void       ctk_assistant_do_set_page_side_image       (CtkAssistant  *assistant,
                                                              CtkWidget     *page,
                                                              GdkPixbuf     *pixbuf);

static void       on_assistant_close                         (CtkWidget     *widget,
							      CtkAssistant  *assistant);
static void       on_assistant_apply                         (CtkWidget     *widget,
							      CtkAssistant  *assistant);
static void       on_assistant_forward                       (CtkWidget     *widget,
							      CtkAssistant  *assistant);
static void       on_assistant_back                          (CtkWidget     *widget,
							      CtkAssistant  *assistant);
static void       on_assistant_cancel                        (CtkWidget     *widget,
							      CtkAssistant  *assistant);
static void       on_assistant_last                          (CtkWidget     *widget,
							      CtkAssistant  *assistant);
static void       assistant_remove_page_cb                   (CtkContainer  *container,
							      CtkWidget     *page,
							      CtkAssistant  *assistant);

GType             _ctk_assistant_accessible_get_type         (void);

enum
{
  CHILD_PROP_0,
  CHILD_PROP_PAGE_TYPE,
  CHILD_PROP_PAGE_TITLE,
  CHILD_PROP_PAGE_HEADER_IMAGE,
  CHILD_PROP_PAGE_SIDEBAR_IMAGE,
  CHILD_PROP_PAGE_COMPLETE,
  CHILD_PROP_HAS_PADDING
};

enum
{
  CANCEL,
  PREPARE,
  APPLY,
  CLOSE,
  ESCAPE,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_USE_HEADER_BAR
};

static guint signals [LAST_SIGNAL] = { 0 };


G_DEFINE_TYPE_WITH_CODE (CtkAssistant, ctk_assistant, CTK_TYPE_WINDOW,
                         G_ADD_PRIVATE (CtkAssistant)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_BUILDABLE,
                                                ctk_assistant_buildable_interface_init))

static void
set_use_header_bar (CtkAssistant *assistant,
                    gint          use_header_bar)
{
  CtkAssistantPrivate *priv = assistant->priv;

  if (use_header_bar == -1)
    return;

  priv->use_header_bar = use_header_bar;
}

static void
ctk_assistant_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  CtkAssistant *assistant = CTK_ASSISTANT (object);

  switch (prop_id)
    {
    case PROP_USE_HEADER_BAR:
      set_use_header_bar (assistant, g_value_get_int (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_assistant_get_property (GObject      *object,
                            guint         prop_id,
                            GValue       *value,
                            GParamSpec   *pspec)
{
  CtkAssistant *assistant = CTK_ASSISTANT (object);
  CtkAssistantPrivate *priv = assistant->priv;

  switch (prop_id)
    {
    case PROP_USE_HEADER_BAR:
      g_value_set_int (value, priv->use_header_bar);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
add_cb (CtkContainer *container,
        CtkWidget    *widget G_GNUC_UNUSED,
        CtkAssistant *assistant)
{
  CtkAssistantPrivate *priv = assistant->priv;

  if (priv->use_header_bar)
    g_warning ("Content added to the action area of a assistant using header bars");

  ctk_widget_show (CTK_WIDGET (container));
}

static void
apply_use_header_bar (CtkAssistant *assistant)
{
  CtkAssistantPrivate *priv = assistant->priv;

  ctk_widget_set_visible (priv->action_area, !priv->use_header_bar);
  ctk_widget_set_visible (priv->headerbar, priv->use_header_bar);
  if (!priv->use_header_bar)
    ctk_window_set_titlebar (CTK_WINDOW (assistant), NULL);
  else
    g_signal_connect (priv->action_area, "add", G_CALLBACK (add_cb), assistant);
}

static void
add_to_header_bar (CtkAssistant *assistant,
                   CtkWidget    *child)
{
  CtkAssistantPrivate *priv = assistant->priv;

  ctk_widget_set_valign (child, CTK_ALIGN_CENTER);

  if (child == priv->back || child == priv->cancel)
    ctk_header_bar_pack_start (CTK_HEADER_BAR (priv->headerbar), child);
  else
    ctk_header_bar_pack_end (CTK_HEADER_BAR (priv->headerbar), child);
}

static void
add_action_widgets (CtkAssistant *assistant)
{
  CtkAssistantPrivate *priv = assistant->priv;
  GList *children;
  GList *l;

  if (priv->use_header_bar)
    {
      children = ctk_container_get_children (CTK_CONTAINER (priv->action_area));
      for (l = children; l != NULL; l = l->next)
        {
          CtkWidget *child = l->data;
          gboolean has_default;

          has_default = ctk_widget_has_default (child);

          g_object_ref (child);
          ctk_container_remove (CTK_CONTAINER (priv->action_area), child);
          add_to_header_bar (assistant, child);
          g_object_unref (child);

          if (has_default)
            {
              ctk_widget_grab_default (child);
              ctk_style_context_add_class (ctk_widget_get_style_context (child), CTK_STYLE_CLASS_SUGGESTED_ACTION);
            }
        }
      g_list_free (children);
    }
}

static void
ctk_assistant_constructed (GObject *object)
{
  CtkAssistant *assistant = CTK_ASSISTANT (object);
  CtkAssistantPrivate *priv = assistant->priv;

  G_OBJECT_CLASS (ctk_assistant_parent_class)->constructed (object);

  priv->constructed = TRUE;
  if (priv->use_header_bar == -1)
    priv->use_header_bar = FALSE;

  add_action_widgets (assistant);
  apply_use_header_bar (assistant);
}

static void
escape_cb (CtkAssistant *assistant)
{
  CtkAssistantPrivate *priv = assistant->priv;

  /* Do not allow cancelling in the middle of a progress page */
  if (priv->current_page &&
      (priv->current_page->type != CTK_ASSISTANT_PAGE_PROGRESS ||
       priv->current_page->complete))
    g_signal_emit (assistant, signals [CANCEL], 0, NULL);

  /* don't run any user handlers - this is not a public signal */
  g_signal_stop_emission (assistant, signals[ESCAPE], 0);
}

static void
ctk_assistant_class_init (CtkAssistantClass *class)
{
  GObjectClass *gobject_class;
  CtkWidgetClass *widget_class;
  CtkContainerClass *container_class;
  CtkBindingSet *binding_set;

  gobject_class   = (GObjectClass *) class;
  widget_class    = (CtkWidgetClass *) class;
  container_class = (CtkContainerClass *) class;

  gobject_class->constructed  = ctk_assistant_constructed;
  gobject_class->set_property = ctk_assistant_set_property;
  gobject_class->get_property = ctk_assistant_get_property;

  widget_class->destroy = ctk_assistant_destroy;
  widget_class->map = ctk_assistant_map;
  widget_class->unmap = ctk_assistant_unmap;
  widget_class->delete_event = ctk_assistant_delete_event;

  ctk_widget_class_set_accessible_type (widget_class, _ctk_assistant_accessible_get_type ());

  container_class->add = ctk_assistant_add;
  container_class->remove = ctk_assistant_remove;
  container_class->set_child_property = ctk_assistant_set_child_property;
  container_class->get_child_property = ctk_assistant_get_child_property;

  /**
   * CtkAssistant::cancel:
   * @assistant: the #CtkAssistant
   *
   * The ::cancel signal is emitted when then the cancel button is clicked.
   *
   * Since: 2.10
   */
  signals[CANCEL] =
    g_signal_new (I_("cancel"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkAssistantClass, cancel),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  /**
   * CtkAssistant::prepare:
   * @assistant: the #CtkAssistant
   * @page: the current page
   *
   * The ::prepare signal is emitted when a new page is set as the
   * assistant's current page, before making the new page visible.
   *
   * A handler for this signal can do any preparations which are
   * necessary before showing @page.
   *
   * Since: 2.10
   */
  signals[PREPARE] =
    g_signal_new (I_("prepare"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkAssistantClass, prepare),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1, CTK_TYPE_WIDGET);

  /**
   * CtkAssistant::apply:
   * @assistant: the #CtkAssistant
   *
   * The ::apply signal is emitted when the apply button is clicked.
   *
   * The default behavior of the #CtkAssistant is to switch to the page
   * after the current page, unless the current page is the last one.
   *
   * A handler for the ::apply signal should carry out the actions for
   * which the wizard has collected data. If the action takes a long time
   * to complete, you might consider putting a page of type
   * %CTK_ASSISTANT_PAGE_PROGRESS after the confirmation page and handle
   * this operation within the #CtkAssistant::prepare signal of the progress
   * page.
   *
   * Since: 2.10
   */
  signals[APPLY] =
    g_signal_new (I_("apply"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkAssistantClass, apply),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  /**
   * CtkAssistant::close:
   * @assistant: the #CtkAssistant
   *
   * The ::close signal is emitted either when the close button of
   * a summary page is clicked, or when the apply button in the last
   * page in the flow (of type %CTK_ASSISTANT_PAGE_CONFIRM) is clicked.
   *
   * Since: 2.10
   */
  signals[CLOSE] =
    g_signal_new (I_("close"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkAssistantClass, close),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  signals[ESCAPE] =
    g_signal_new_class_handler (I_("escape"),
                                G_TYPE_FROM_CLASS (gobject_class),
                                G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                                G_CALLBACK (escape_cb),
                                NULL, NULL,
                                NULL,
                                G_TYPE_NONE, 0);

  binding_set = ctk_binding_set_by_class (class);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_Escape, 0, "escape", 0);

  /**
   * CtkAssistant:use-header-bar:
   *
   * %TRUE if the assistant uses a #CtkHeaderBar for action buttons
   * instead of the action-area.
   *
   * For technical reasons, this property is declared as an integer
   * property, but you should only set it to %TRUE or %FALSE.
   *
   * Since: 3.12
   */
  g_object_class_install_property (gobject_class,
                                   PROP_USE_HEADER_BAR,
                                   g_param_spec_int ("use-header-bar",
                                                     P_("Use Header Bar"),
                                                     P_("Use Header Bar for actions."),
                                                     -1, 1, -1,
                                                     CTK_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY));

  /**
   * CtkAssistant:header-padding:
   *
   * Number of pixels around the header.
   *
   * Deprecated:3.20: This style property is ignored.
   */
  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("header-padding",
                                                             P_("Header Padding"),
                                                             P_("Number of pixels around the header."),
                                                             0,
                                                             G_MAXINT,
                                                             6,
                                                             CTK_PARAM_READABLE | G_PARAM_DEPRECATED));

  /**
   * CtkAssistant:content-padding:
   *
   * Number of pixels around the content.
   *
   * Deprecated:3.20: This style property is ignored.
   */
  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("content-padding",
                                                             P_("Content Padding"),
                                                             P_("Number of pixels around the content pages."),
                                                             0,
                                                             G_MAXINT,
                                                             1,
                                                             CTK_PARAM_READABLE | G_PARAM_DEPRECATED));

  /**
   * CtkAssistant:page-type:
   *
   * The type of the assistant page.
   *
   * Since: 2.10
   */
  ctk_container_class_install_child_property (container_class,
                                              CHILD_PROP_PAGE_TYPE,
                                              g_param_spec_enum ("page-type",
                                                                 P_("Page type"),
                                                                 P_("The type of the assistant page"),
                                                                 CTK_TYPE_ASSISTANT_PAGE_TYPE,
                                                                 CTK_ASSISTANT_PAGE_CONTENT,
                                                                 CTK_PARAM_READWRITE));

  /**
   * CtkAssistant:title:
   *
   * The title of the page.
   *
   * Since: 2.10
   */
  ctk_container_class_install_child_property (container_class,
                                              CHILD_PROP_PAGE_TITLE,
                                              g_param_spec_string ("title",
                                                                   P_("Page title"),
                                                                   P_("The title of the assistant page"),
                                                                   NULL,
                                                                   CTK_PARAM_READWRITE));

  /**
   * CtkAssistant:header-image:
   *
   * This image used to be displayed in the page header.
   *
   * Since: 2.10
   *
   * Deprecated: 3.2: Since CTK+ 3.2, a header is no longer shown;
   *     add your header decoration to the page content instead.
   */
  ctk_container_class_install_child_property (container_class,
                                              CHILD_PROP_PAGE_HEADER_IMAGE,
                                              g_param_spec_object ("header-image",
                                                                   P_("Header image"),
                                                                   P_("Header image for the assistant page"),
                                                                   GDK_TYPE_PIXBUF,
                                                                   CTK_PARAM_READWRITE));

  /**
   * CtkAssistant:sidebar-image:
   *
   * This image used to be displayed in the 'sidebar'.
   *
   * Since: 2.10
   *
   * Deprecated: 3.2: Since CTK+ 3.2, the sidebar image is no longer shown.
   */
  ctk_container_class_install_child_property (container_class,
                                              CHILD_PROP_PAGE_SIDEBAR_IMAGE,
                                              g_param_spec_object ("sidebar-image",
                                                                   P_("Sidebar image"),
                                                                   P_("Sidebar image for the assistant page"),
                                                                   GDK_TYPE_PIXBUF,
                                                                   CTK_PARAM_READWRITE));

  /**
   * CtkAssistant:complete:
   *
   * Setting the "complete" child property to %TRUE marks a page as
   * complete (i.e.: all the required fields are filled out). CTK+ uses
   * this information to control the sensitivity of the navigation buttons.
   *
   * Since: 2.10
   */
  ctk_container_class_install_child_property (container_class,
                                              CHILD_PROP_PAGE_COMPLETE,
                                              g_param_spec_boolean ("complete",
                                                                    P_("Page complete"),
                                                                    P_("Whether all required fields on the page have been filled out"),
                                                                    FALSE,
                                                                    G_PARAM_READWRITE));

  ctk_container_class_install_child_property (container_class, CHILD_PROP_HAS_PADDING,
      g_param_spec_boolean ("has-padding", P_("Has padding"), P_("Whether the assistant adds padding around the page"),
                            TRUE, G_PARAM_READWRITE));

  /* Bind class to template
   */
  ctk_widget_class_set_template_from_resource (widget_class,
					       "/org/ctk/libctk/ui/ctkassistant.ui");

  ctk_widget_class_bind_template_child_internal_private (widget_class, CtkAssistant, action_area);
  ctk_widget_class_bind_template_child_internal_private (widget_class, CtkAssistant, headerbar);
  ctk_widget_class_bind_template_child_private (widget_class, CtkAssistant, content);
  ctk_widget_class_bind_template_child_private (widget_class, CtkAssistant, cancel);
  ctk_widget_class_bind_template_child_private (widget_class, CtkAssistant, forward);
  ctk_widget_class_bind_template_child_private (widget_class, CtkAssistant, back);
  ctk_widget_class_bind_template_child_private (widget_class, CtkAssistant, apply);
  ctk_widget_class_bind_template_child_private (widget_class, CtkAssistant, close);
  ctk_widget_class_bind_template_child_private (widget_class, CtkAssistant, last);
  ctk_widget_class_bind_template_child_private (widget_class, CtkAssistant, sidebar);
  ctk_widget_class_bind_template_child_private (widget_class, CtkAssistant, button_size_group);
  ctk_widget_class_bind_template_child_private (widget_class, CtkAssistant, title_size_group);

  ctk_widget_class_bind_template_callback (widget_class, assistant_remove_page_cb);
  ctk_widget_class_bind_template_callback (widget_class, on_assistant_close);
  ctk_widget_class_bind_template_callback (widget_class, on_assistant_apply);
  ctk_widget_class_bind_template_callback (widget_class, on_assistant_forward);
  ctk_widget_class_bind_template_callback (widget_class, on_assistant_back);
  ctk_widget_class_bind_template_callback (widget_class, on_assistant_cancel);
  ctk_widget_class_bind_template_callback (widget_class, on_assistant_last);

  ctk_widget_class_set_css_name (widget_class, "assistant");
}

static gint
default_forward_function (gint current_page, gpointer data)
{
  CtkAssistant *assistant;
  CtkAssistantPrivate *priv;
  CtkAssistantPage *page_info;
  GList *page_node;

  assistant = CTK_ASSISTANT (data);
  priv = assistant->priv;

  page_node = g_list_nth (priv->pages, ++current_page);

  if (!page_node)
    return -1;

  page_info = (CtkAssistantPage *) page_node->data;

  while (page_node && !ctk_widget_get_visible (page_info->page))
    {
      page_node = page_node->next;
      current_page++;

      if (page_node)
        page_info = (CtkAssistantPage *) page_node->data;
    }

  return current_page;
}

static gboolean
last_button_visible (CtkAssistant *assistant, CtkAssistantPage *page)
{
  CtkAssistantPrivate *priv = assistant->priv;
  CtkAssistantPage *page_info;
  gint count, page_num, n_pages;

  if (page == NULL)
    return FALSE;

  if (page->type != CTK_ASSISTANT_PAGE_CONTENT)
    return FALSE;

  count = 0;
  page_num = g_list_index (priv->pages, page);
  n_pages  = g_list_length (priv->pages);
  page_info = page;

  while (page_num >= 0 && page_num < n_pages &&
         page_info->type == CTK_ASSISTANT_PAGE_CONTENT &&
         (count == 0 || page_info->complete) &&
         count < n_pages)
    {
      page_num = (priv->forward_function) (page_num, priv->forward_function_data);
      page_info = g_list_nth_data (priv->pages, page_num);

      count++;
    }

  /* Make the last button visible if we can skip multiple
   * pages and end on a confirmation or summary page
   */
  if (count > 1 && page_info &&
      (page_info->type == CTK_ASSISTANT_PAGE_CONFIRM ||
       page_info->type == CTK_ASSISTANT_PAGE_SUMMARY))
    return TRUE;
  else
    return FALSE;
}

static void
update_actions_size (CtkAssistant *assistant)
{
  CtkAssistantPrivate *priv = assistant->priv;
  GList *l;
  CtkAssistantPage *page;
  gint buttons, page_buttons;

  if (!priv->current_page)
    return;

  /* Some heuristics to find out how many buttons we should
   * reserve space for. It is possible to trick this code
   * with page forward functions and invisible pages, etc.
   */
  buttons = 0;
  for (l = priv->pages; l; l = l->next)
    {
      page = l->data;

      if (!ctk_widget_get_visible (page->page))
        continue;

      page_buttons = 2; /* cancel, forward/apply/close */
      if (l != priv->pages)
        page_buttons += 1; /* back */
      if (last_button_visible (assistant, page))
        page_buttons += 1; /* last */

      buttons = MAX (buttons, page_buttons);
    }

  buttons += priv->extra_buttons;

  ctk_widget_set_size_request (priv->action_area,
                               buttons * ctk_widget_get_allocated_width (priv->cancel) + (buttons - 1) * 6,
                               -1);
}

static void
compute_last_button_state (CtkAssistant *assistant)
{
  CtkAssistantPrivate *priv = assistant->priv;

  ctk_widget_set_sensitive (priv->last, priv->current_page->complete);
  if (last_button_visible (assistant, priv->current_page))
    ctk_widget_show (priv->last);
  else
    ctk_widget_hide (priv->last);
}

static void
compute_progress_state (CtkAssistant *assistant)
{
  CtkAssistantPrivate *priv = assistant->priv;
  gint page_num, n_pages;

  n_pages = ctk_assistant_get_n_pages (assistant);
  page_num = ctk_assistant_get_current_page (assistant);

  page_num = (priv->forward_function) (page_num, priv->forward_function_data);

  if (page_num >= 0 && page_num < n_pages)
    ctk_widget_show (priv->forward);
  else
    ctk_widget_hide (priv->forward);
}

static void
update_buttons_state (CtkAssistant *assistant)
{
  CtkAssistantPrivate *priv = assistant->priv;

  if (!priv->current_page)
    return;

  switch (priv->current_page->type)
    {
    case CTK_ASSISTANT_PAGE_INTRO:
      ctk_widget_set_sensitive (priv->cancel, TRUE);
      ctk_widget_set_sensitive (priv->forward, priv->current_page->complete);
      ctk_widget_grab_default (priv->forward);
      ctk_widget_show (priv->forward);
      ctk_widget_hide (priv->back);
      ctk_widget_hide (priv->apply);
      ctk_widget_hide (priv->close);
      compute_last_button_state (assistant);
      break;
    case CTK_ASSISTANT_PAGE_CONFIRM:
      ctk_widget_set_sensitive (priv->cancel, TRUE);
      ctk_widget_set_sensitive (priv->back, TRUE);
      ctk_widget_set_sensitive (priv->apply, priv->current_page->complete);
      ctk_widget_grab_default (priv->apply);
      ctk_widget_show (priv->back);
      ctk_widget_show (priv->apply);
      ctk_widget_hide (priv->forward);
      ctk_widget_hide (priv->close);
      ctk_widget_hide (priv->last);
      break;
    case CTK_ASSISTANT_PAGE_CONTENT:
      ctk_widget_set_sensitive (priv->cancel, TRUE);
      ctk_widget_set_sensitive (priv->back, TRUE);
      ctk_widget_set_sensitive (priv->forward, priv->current_page->complete);
      ctk_widget_grab_default (priv->forward);
      ctk_widget_show (priv->back);
      ctk_widget_show (priv->forward);
      ctk_widget_hide (priv->apply);
      ctk_widget_hide (priv->close);
      compute_last_button_state (assistant);
      break;
    case CTK_ASSISTANT_PAGE_SUMMARY:
      ctk_widget_set_sensitive (priv->close, priv->current_page->complete);
      ctk_widget_grab_default (priv->close);
      ctk_widget_show (priv->close);
      ctk_widget_hide (priv->back);
      ctk_widget_hide (priv->forward);
      ctk_widget_hide (priv->apply);
      ctk_widget_hide (priv->last);
      break;
    case CTK_ASSISTANT_PAGE_PROGRESS:
      ctk_widget_set_sensitive (priv->cancel, priv->current_page->complete);
      ctk_widget_set_sensitive (priv->back, priv->current_page->complete);
      ctk_widget_set_sensitive (priv->forward, priv->current_page->complete);
      ctk_widget_grab_default (priv->forward);
      ctk_widget_show (priv->back);
      ctk_widget_hide (priv->apply);
      ctk_widget_hide (priv->close);
      ctk_widget_hide (priv->last);
      compute_progress_state (assistant);
      break;
    case CTK_ASSISTANT_PAGE_CUSTOM:
      ctk_widget_hide (priv->cancel);
      ctk_widget_hide (priv->back);
      ctk_widget_hide (priv->forward);
      ctk_widget_hide (priv->apply);
      ctk_widget_hide (priv->last);
      ctk_widget_hide (priv->close);
      break;
    default:
      g_assert_not_reached ();
    }

  if (priv->committed)
    ctk_widget_hide (priv->cancel);
  else if (priv->current_page->type == CTK_ASSISTANT_PAGE_SUMMARY ||
           priv->current_page->type == CTK_ASSISTANT_PAGE_CUSTOM)
    ctk_widget_hide (priv->cancel);
  else
    ctk_widget_show (priv->cancel);

  /* this is quite general, we don't want to
   * go back if it's the first page
   */
  if (!priv->visited_pages)
    ctk_widget_hide (priv->back);
}

static gboolean
update_page_title_state (CtkAssistant *assistant, GList *list)
{
  CtkAssistantPage *page, *other;
  CtkAssistantPrivate *priv = assistant->priv;
  gboolean visible;
  GList *l;

  page = list->data;

  if (page->title == NULL || page->title[0] == 0)
    visible = FALSE;
  else
    visible = ctk_widget_get_visible (page->page);

  if (page == priv->current_page)
    {
      ctk_widget_set_visible (page->regular_title, FALSE);
      ctk_widget_set_visible (page->current_title, visible);
    }
  else
    {
      /* If multiple consecutive pages have the same title,
       * we only show it once, since it would otherwise look
       * silly. We have to be a little careful, since we
       * _always_ show the title of the current page.
       */
      if (list->prev)
        {
          other = list->prev->data;
          if (g_strcmp0 (page->title, other->title) == 0)
            visible = FALSE;
        }
      for (l = list->next; l; l = l->next)
        {
          other = l->data;
          if (g_strcmp0 (page->title, other->title) != 0)
            break;

          if (other == priv->current_page)
            {
              visible = FALSE;
              break;
            }
        }

      ctk_widget_set_visible (page->regular_title, visible);
      ctk_widget_set_visible (page->current_title, FALSE);
    }

  return visible;
}

static void
update_title_state (CtkAssistant *assistant)
{
  CtkAssistantPrivate *priv = assistant->priv;
  GList *l;
  gboolean show_titles;

  show_titles = FALSE;
  for (l = priv->pages; l != NULL; l = l->next)
    {
      if (update_page_title_state (assistant, l))
        show_titles = TRUE;
    }

  ctk_widget_set_visible (priv->sidebar, show_titles);
}

static void
set_current_page (CtkAssistant *assistant,
                  gint          page_num)
{
  CtkAssistantPrivate *priv = assistant->priv;

  priv->current_page = (CtkAssistantPage *)g_list_nth_data (priv->pages, page_num);

  g_signal_emit (assistant, signals [PREPARE], 0, priv->current_page->page);
  /* do not continue if the prepare signal handler has already changed the
   * current page */
  if (priv->current_page != (CtkAssistantPage *)g_list_nth_data (priv->pages, page_num))
    return;

  update_title_state (assistant);

  ctk_window_set_title (CTK_WINDOW (assistant), priv->current_page->title);

  ctk_notebook_set_current_page (CTK_NOTEBOOK (priv->content), page_num);

  /* update buttons state, flow may have changed */
  if (ctk_widget_get_mapped (CTK_WIDGET (assistant)))
    update_buttons_state (assistant);

  if (!ctk_widget_child_focus (priv->current_page->page, CTK_DIR_TAB_FORWARD))
    {
      CtkWidget *button[6];
      gint i;

      /* find the best button to focus */
      button[0] = priv->apply;
      button[1] = priv->close;
      button[2] = priv->forward;
      button[3] = priv->back;
      button[4] = priv->cancel;
      button[5] = priv->last;
      for (i = 0; i < 6; i++)
        {
          if (ctk_widget_get_visible (button[i]) &&
              ctk_widget_get_sensitive (button[i]))
            {
              ctk_widget_grab_focus (button[i]);
              break;
            }
        }
    }
}

static gint
compute_next_step (CtkAssistant *assistant)
{
  CtkAssistantPrivate *priv = assistant->priv;
  CtkAssistantPage *page_info;
  gint current_page, n_pages, next_page;

  current_page = ctk_assistant_get_current_page (assistant);
  page_info = priv->current_page;
  n_pages = ctk_assistant_get_n_pages (assistant);

  next_page = (priv->forward_function) (current_page,
                                        priv->forward_function_data);

  if (next_page >= 0 && next_page < n_pages)
    {
      priv->visited_pages = g_slist_prepend (priv->visited_pages, page_info);
      set_current_page (assistant, next_page);

      return TRUE;
    }

  return FALSE;
}

static void
on_assistant_close (CtkWidget    *widget G_GNUC_UNUSED,
                    CtkAssistant *assistant)
{
  g_signal_emit (assistant, signals [CLOSE], 0, NULL);
}

static void
on_assistant_apply (CtkWidget    *widget G_GNUC_UNUSED,
                    CtkAssistant *assistant)
{
  gboolean success;

  g_signal_emit (assistant, signals [APPLY], 0);

  success = compute_next_step (assistant);

  /* if the assistant hasn't switched to another page, just emit
   * the CLOSE signal, it't the last page in the assistant flow
   */
  if (!success)
    g_signal_emit (assistant, signals [CLOSE], 0);
}

static void
on_assistant_forward (CtkWidget    *widget G_GNUC_UNUSED,
                      CtkAssistant *assistant)
{
  ctk_assistant_next_page (assistant);
}

static void
on_assistant_back (CtkWidget    *widget G_GNUC_UNUSED,
                   CtkAssistant *assistant)
{
  ctk_assistant_previous_page (assistant);
}

static void
on_assistant_cancel (CtkWidget    *widget G_GNUC_UNUSED,
                     CtkAssistant *assistant)
{
  g_signal_emit (assistant, signals [CANCEL], 0, NULL);
}

static void
on_assistant_last (CtkWidget    *widget G_GNUC_UNUSED,
                   CtkAssistant *assistant)
{
  CtkAssistantPrivate *priv = assistant->priv;

  while (priv->current_page->type == CTK_ASSISTANT_PAGE_CONTENT &&
         priv->current_page->complete)
    compute_next_step (assistant);
}

static gboolean
alternative_button_order (CtkAssistant *assistant)
{
  CtkSettings *settings;
  CdkScreen *screen;
  gboolean result;

  screen   = ctk_widget_get_screen (CTK_WIDGET (assistant));
  settings = ctk_settings_get_for_screen (screen);

  g_object_get (settings,
                "ctk-alternative-button-order", &result,
                NULL);
  return result;
}

static void
on_page_notify (CtkWidget  *widget G_GNUC_UNUSED,
                GParamSpec *arg G_GNUC_UNUSED,
                gpointer    data)
{
  CtkAssistant *assistant = CTK_ASSISTANT (data);

  if (ctk_widget_get_mapped (CTK_WIDGET (assistant)))
    {
      update_buttons_state (assistant);
      update_title_state (assistant);
    }
}

static void
assistant_remove_page_cb (CtkContainer *container G_GNUC_UNUSED,
                          CtkWidget    *page,
                          CtkAssistant *assistant)
{
  CtkAssistantPrivate *priv = assistant->priv;
  CtkAssistantPage *page_info;
  GList *page_node;
  GList *element;

  element = find_page (assistant, page);
  if (!element)
    return;

  page_info = element->data;

  /* If this is the current page, we need to switch away. */
  if (page_info == priv->current_page)
    {
      if (!compute_next_step (assistant))
        {
          /* The best we can do at this point is probably to pick
           * the first visible page.
           */
          page_node = priv->pages;

          while (page_node &&
                 !ctk_widget_get_visible (((CtkAssistantPage *) page_node->data)->page))
            page_node = page_node->next;

          if (page_node == element)
            page_node = page_node->next;

          if (page_node)
            priv->current_page = page_node->data;
          else
            priv->current_page = NULL;
        }
    }

  g_signal_handlers_disconnect_by_func (page_info->page, on_page_notify, assistant);

  ctk_size_group_remove_widget (priv->title_size_group, page_info->regular_title);
  ctk_size_group_remove_widget (priv->title_size_group, page_info->current_title);

  ctk_container_remove (CTK_CONTAINER (priv->sidebar), page_info->regular_title);
  ctk_container_remove (CTK_CONTAINER (priv->sidebar), page_info->current_title);

  priv->pages = g_list_remove_link (priv->pages, element);
  priv->visited_pages = g_slist_remove_all (priv->visited_pages, page_info);

  g_free (page_info->title);

  g_slice_free (CtkAssistantPage, page_info);
  g_list_free_1 (element);

  if (ctk_widget_get_mapped (CTK_WIDGET (assistant)))
    {
      update_buttons_state (assistant);
      update_actions_size (assistant);
    }
}

static void
ctk_assistant_init (CtkAssistant *assistant)
{
  CtkAssistantPrivate *priv;

  assistant->priv = ctk_assistant_get_instance_private (assistant);

  priv = assistant->priv;
  priv->pages = NULL;
  priv->current_page = NULL;
  priv->visited_pages = NULL;

  priv->forward_function = default_forward_function;
  priv->forward_function_data = assistant;
  priv->forward_data_destroy = NULL;

  g_object_get (ctk_widget_get_settings (CTK_WIDGET (assistant)),
                "ctk-dialogs-use-header", &priv->use_header_bar,
                NULL);

  ctk_widget_init_template (CTK_WIDGET (assistant));

  if (alternative_button_order (assistant))
    {
      GList *buttons, *l;

      /* Reverse the action area children for the alternative button order setting */
      buttons = ctk_container_get_children (CTK_CONTAINER (priv->action_area));

      for (l = buttons; l; l = l->next)
	ctk_box_reorder_child (CTK_BOX (priv->action_area), CTK_WIDGET (l->data), -1);

      g_list_free (buttons);
    }
}

static void
ctk_assistant_set_child_property (CtkContainer *container,
                                  CtkWidget    *child,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  switch (property_id)
    {
    case CHILD_PROP_PAGE_TYPE:
      ctk_assistant_set_page_type (CTK_ASSISTANT (container), child,
                                   g_value_get_enum (value));
      break;
    case CHILD_PROP_PAGE_TITLE:
      ctk_assistant_set_page_title (CTK_ASSISTANT (container), child,
                                    g_value_get_string (value));
      break;
    case CHILD_PROP_PAGE_HEADER_IMAGE:
      ctk_assistant_do_set_page_header_image (CTK_ASSISTANT (container), child,
                                              g_value_get_object (value));
      break;
    case CHILD_PROP_PAGE_SIDEBAR_IMAGE:
      ctk_assistant_do_set_page_side_image (CTK_ASSISTANT (container), child,
                                            g_value_get_object (value));
      break;
    case CHILD_PROP_PAGE_COMPLETE:
      ctk_assistant_set_page_complete (CTK_ASSISTANT (container), child,
                                       g_value_get_boolean (value));
      break;
    case CHILD_PROP_HAS_PADDING:
      ctk_assistant_set_page_has_padding (CTK_ASSISTANT (container), child,
                                          g_value_get_boolean (value));
      break;
    default:
      CTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      break;
    }
}

static void
ctk_assistant_get_child_property (CtkContainer *container,
                                  CtkWidget    *child,
                                  guint         property_id,
                                  GValue       *value,
                                  GParamSpec   *pspec)
{
  CtkAssistant *assistant = CTK_ASSISTANT (container);

  switch (property_id)
    {
    case CHILD_PROP_PAGE_TYPE:
      g_value_set_enum (value,
                        ctk_assistant_get_page_type (assistant, child));
      break;
    case CHILD_PROP_PAGE_TITLE:
      g_value_set_string (value,
                          ctk_assistant_get_page_title (assistant, child));
      break;
    case CHILD_PROP_PAGE_HEADER_IMAGE:
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      g_value_set_object (value,
                          ctk_assistant_get_page_header_image (assistant, child));
G_GNUC_END_IGNORE_DEPRECATIONS
      break;
    case CHILD_PROP_PAGE_SIDEBAR_IMAGE:
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      g_value_set_object (value,
                          ctk_assistant_get_page_side_image (assistant, child));
G_GNUC_END_IGNORE_DEPRECATIONS
      break;
    case CHILD_PROP_PAGE_COMPLETE:
      g_value_set_boolean (value,
                           ctk_assistant_get_page_complete (assistant, child));
      break;
    case CHILD_PROP_HAS_PADDING:
      g_value_set_boolean (value,
                           ctk_assistant_get_page_has_padding (assistant, child));
      break;
    default:
      CTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      break;
    }
}

static void
ctk_assistant_destroy (CtkWidget *widget)
{
  CtkAssistant *assistant = CTK_ASSISTANT (widget);
  CtkAssistantPrivate *priv = assistant->priv;

  /* We set current to NULL so that the remove code doesn't try
   * to do anything funny
   */
  priv->current_page = NULL;

  if (priv->content)
    {
      CtkNotebook *notebook;
      CtkWidget *page;

      /* Remove all pages from the content notebook. */
      notebook = (CtkNotebook *) priv->content;
      while ((page = ctk_notebook_get_nth_page (notebook, 0)) != NULL)
        ctk_container_remove ((CtkContainer *) notebook, page);

      /* Our CtkAssistantPage list should be empty now. */
      g_warn_if_fail (priv->pages == NULL);

      priv->content = NULL;
    }

  if (priv->sidebar)
    priv->sidebar = NULL;

  if (priv->action_area)
    priv->action_area = NULL;

  if (priv->forward_function)
    {
      if (priv->forward_function_data &&
          priv->forward_data_destroy)
        priv->forward_data_destroy (priv->forward_function_data);

      priv->forward_function = NULL;
      priv->forward_function_data = NULL;
      priv->forward_data_destroy = NULL;
    }

  if (priv->visited_pages)
    {
      g_slist_free (priv->visited_pages);
      priv->visited_pages = NULL;
    }

  ctk_window_set_titlebar (CTK_WINDOW (widget), NULL);
  CTK_WIDGET_CLASS (ctk_assistant_parent_class)->destroy (widget);
}

static GList*
find_page (CtkAssistant  *assistant,
           CtkWidget     *page)
{
  CtkAssistantPrivate *priv = assistant->priv;
  GList *child = priv->pages;

  while (child)
    {
      CtkAssistantPage *page_info = child->data;
      if (page_info->page == page || page_info->box == page)
        return child;

      child = child->next;
    }

  return NULL;
}

static void
ctk_assistant_map (CtkWidget *widget)
{
  CtkAssistant *assistant = CTK_ASSISTANT (widget);
  CtkAssistantPrivate *priv = assistant->priv;
  GList *page_node;
  CtkAssistantPage *page;
  gint page_num;

  /* if there's no default page, pick the first one */
  page = NULL;
  page_num = 0;
  if (!priv->current_page)
    {
      page_node = priv->pages;

      while (page_node && !ctk_widget_get_visible (((CtkAssistantPage *) page_node->data)->page))
        {
          page_node = page_node->next;
          page_num++;
        }

      if (page_node)
        page = page_node->data;
    }

  if (page && ctk_widget_get_visible (page->page))
    set_current_page (assistant, page_num);

  update_buttons_state (assistant);
  update_actions_size (assistant);
  update_title_state (assistant);

  CTK_WIDGET_CLASS (ctk_assistant_parent_class)->map (widget);
}

static void
ctk_assistant_unmap (CtkWidget *widget)
{
  CtkAssistant *assistant = CTK_ASSISTANT (widget);
  CtkAssistantPrivate *priv = assistant->priv;

  g_slist_free (priv->visited_pages);
  priv->visited_pages = NULL;
  priv->current_page  = NULL;

  CTK_WIDGET_CLASS (ctk_assistant_parent_class)->unmap (widget);
}

static gboolean
ctk_assistant_delete_event (CtkWidget   *widget,
                            CdkEventAny *event G_GNUC_UNUSED)
{
  CtkAssistant *assistant = CTK_ASSISTANT (widget);
  CtkAssistantPrivate *priv = assistant->priv;

  /* Do not allow cancelling in the middle of a progress page */
  if (priv->current_page &&
      (priv->current_page->type != CTK_ASSISTANT_PAGE_PROGRESS ||
       priv->current_page->complete))
    g_signal_emit (widget, signals [CANCEL], 0, NULL);

  return TRUE;
}

static void
ctk_assistant_add (CtkContainer *container,
                   CtkWidget    *page)
{
  /* A bit tricky here, CtkAssistant doesnt exactly play by 
   * the rules by allowing ctk_container_add() to insert pages.
   *
   * For the first invocation (from the builder template invocation),
   * let's make sure we add the actual direct container content properly.
   */
  if (!ctk_bin_get_child (CTK_BIN (container)))
    {
      ctk_widget_set_parent (page, CTK_WIDGET (container));
      _ctk_bin_set_child (CTK_BIN (container), page);
      return;
    }

  ctk_assistant_append_page (CTK_ASSISTANT (container), page);
}

static void
ctk_assistant_remove (CtkContainer *container,
                      CtkWidget    *page)
{
  CtkAssistant *assistant = (CtkAssistant*) container;
  CtkWidget *box;

  /* Forward this removal to the content notebook */
  box = ctk_widget_get_parent (page);
  if (CTK_IS_BOX (box) &&
      assistant->priv->content != NULL &&
      ctk_widget_get_parent (box) == assistant->priv->content)
    {
      ctk_container_remove (CTK_CONTAINER (box), page);
      ctk_container_remove (CTK_CONTAINER (assistant->priv->content), box);
    }
}

/**
 * ctk_assistant_new:
 *
 * Creates a new #CtkAssistant.
 *
 * Returns: a newly created #CtkAssistant
 *
 * Since: 2.10
 */
CtkWidget*
ctk_assistant_new (void)
{
  CtkWidget *assistant;

  assistant = g_object_new (CTK_TYPE_ASSISTANT, NULL);

  return assistant;
}

/**
 * ctk_assistant_get_current_page:
 * @assistant: a #CtkAssistant
 *
 * Returns the page number of the current page.
 *
 * Returns: The index (starting from 0) of the current
 *     page in the @assistant, or -1 if the @assistant has no pages,
 *     or no current page.
 *
 * Since: 2.10
 */
gint
ctk_assistant_get_current_page (CtkAssistant *assistant)
{
  CtkAssistantPrivate *priv;

  g_return_val_if_fail (CTK_IS_ASSISTANT (assistant), -1);

  priv = assistant->priv;

  if (!priv->pages || !priv->current_page)
    return -1;

  return g_list_index (priv->pages, priv->current_page);
}

/**
 * ctk_assistant_set_current_page:
 * @assistant: a #CtkAssistant
 * @page_num: index of the page to switch to, starting from 0.
 *     If negative, the last page will be used. If greater
 *     than the number of pages in the @assistant, nothing
 *     will be done.
 *
 * Switches the page to @page_num.
 *
 * Note that this will only be necessary in custom buttons,
 * as the @assistant flow can be set with
 * ctk_assistant_set_forward_page_func().
 *
 * Since: 2.10
 */
void
ctk_assistant_set_current_page (CtkAssistant *assistant,
                                gint          page_num)
{
  CtkAssistantPrivate *priv;
  CtkAssistantPage *page;

  g_return_if_fail (CTK_IS_ASSISTANT (assistant));

  priv = assistant->priv;

  if (page_num >= 0)
    page = (CtkAssistantPage *) g_list_nth_data (priv->pages, page_num);
  else
    {
      page = (CtkAssistantPage *) g_list_last (priv->pages)->data;
      page_num = g_list_length (priv->pages);
    }

  g_return_if_fail (page != NULL);

  if (priv->current_page == page)
    return;

  /* only add the page to the visited list if the assistant is mapped,
   * if not, just use it as an initial page setting, for the cases where
   * the initial page is != to 0
   */
  if (ctk_widget_get_mapped (CTK_WIDGET (assistant)))
    priv->visited_pages = g_slist_prepend (priv->visited_pages,
                                           priv->current_page);

  set_current_page (assistant, page_num);
}

/**
 * ctk_assistant_next_page:
 * @assistant: a #CtkAssistant
 *
 * Navigate to the next page.
 *
 * It is a programming error to call this function when
 * there is no next page.
 *
 * This function is for use when creating pages of the
 * #CTK_ASSISTANT_PAGE_CUSTOM type.
 *
 * Since: 3.0
 */
void
ctk_assistant_next_page (CtkAssistant *assistant)
{
  g_return_if_fail (CTK_IS_ASSISTANT (assistant));

  if (!compute_next_step (assistant))
    g_critical ("Page flow is broken.\n"
                "You may want to end it with a page of type\n"
                "CTK_ASSISTANT_PAGE_CONFIRM or CTK_ASSISTANT_PAGE_SUMMARY");
}

/**
 * ctk_assistant_previous_page:
 * @assistant: a #CtkAssistant
 *
 * Navigate to the previous visited page.
 *
 * It is a programming error to call this function when
 * no previous page is available.
 *
 * This function is for use when creating pages of the
 * #CTK_ASSISTANT_PAGE_CUSTOM type.
 *
 * Since: 3.0
 */
void
ctk_assistant_previous_page (CtkAssistant *assistant)
{
  CtkAssistantPrivate *priv;
  CtkAssistantPage *page_info;
  GSList *page_node;

  g_return_if_fail (CTK_IS_ASSISTANT (assistant));

  priv = assistant->priv;

  /* skip the progress pages when going back */
  do
    {
      page_node = priv->visited_pages;

      g_return_if_fail (page_node != NULL);

      priv->visited_pages = priv->visited_pages->next;
      page_info = (CtkAssistantPage *) page_node->data;
      g_slist_free_1 (page_node);
    }
  while (page_info->type == CTK_ASSISTANT_PAGE_PROGRESS ||
         !ctk_widget_get_visible (page_info->page));

  set_current_page (assistant, g_list_index (priv->pages, page_info));
}

/**
 * ctk_assistant_get_n_pages:
 * @assistant: a #CtkAssistant
 *
 * Returns the number of pages in the @assistant
 *
 * Returns: the number of pages in the @assistant
 *
 * Since: 2.10
 */
gint
ctk_assistant_get_n_pages (CtkAssistant *assistant)
{
  CtkAssistantPrivate *priv;

  g_return_val_if_fail (CTK_IS_ASSISTANT (assistant), 0);

  priv = assistant->priv;

  return g_list_length (priv->pages);
}

/**
 * ctk_assistant_get_nth_page:
 * @assistant: a #CtkAssistant
 * @page_num: the index of a page in the @assistant,
 *     or -1 to get the last page
 *
 * Returns the child widget contained in page number @page_num.
 *
 * Returns: (nullable) (transfer none): the child widget, or %NULL
 *     if @page_num is out of bounds
 *
 * Since: 2.10
 */
CtkWidget*
ctk_assistant_get_nth_page (CtkAssistant *assistant,
                            gint          page_num)
{
  CtkAssistantPrivate *priv;
  CtkAssistantPage *page;
  GList *elem;

  g_return_val_if_fail (CTK_IS_ASSISTANT (assistant), NULL);
  g_return_val_if_fail (page_num >= -1, NULL);

  priv = assistant->priv;

  if (page_num == -1)
    elem = g_list_last (priv->pages);
  else
    elem = g_list_nth (priv->pages, page_num);

  if (!elem)
    return NULL;

  page = (CtkAssistantPage *) elem->data;

  return page->page;
}

/**
 * ctk_assistant_prepend_page:
 * @assistant: a #CtkAssistant
 * @page: a #CtkWidget
 *
 * Prepends a page to the @assistant.
 *
 * Returns: the index (starting at 0) of the inserted page
 *
 * Since: 2.10
 */
gint
ctk_assistant_prepend_page (CtkAssistant *assistant,
                            CtkWidget    *page)
{
  g_return_val_if_fail (CTK_IS_ASSISTANT (assistant), 0);
  g_return_val_if_fail (CTK_IS_WIDGET (page), 0);

  return ctk_assistant_insert_page (assistant, page, 0);
}

/**
 * ctk_assistant_append_page:
 * @assistant: a #CtkAssistant
 * @page: a #CtkWidget
 *
 * Appends a page to the @assistant.
 *
 * Returns: the index (starting at 0) of the inserted page
 *
 * Since: 2.10
 */
gint
ctk_assistant_append_page (CtkAssistant *assistant,
                           CtkWidget    *page)
{
  g_return_val_if_fail (CTK_IS_ASSISTANT (assistant), 0);
  g_return_val_if_fail (CTK_IS_WIDGET (page), 0);

  return ctk_assistant_insert_page (assistant, page, -1);
}

/**
 * ctk_assistant_insert_page:
 * @assistant: a #CtkAssistant
 * @page: a #CtkWidget
 * @position: the index (starting at 0) at which to insert the page,
 *     or -1 to append the page to the @assistant
 *
 * Inserts a page in the @assistant at a given position.
 *
 * Returns: the index (starting from 0) of the inserted page
 *
 * Since: 2.10
 */
gint
ctk_assistant_insert_page (CtkAssistant *assistant,
                           CtkWidget    *page,
                           gint          position)
{
  CtkAssistantPrivate *priv;
  CtkAssistantPage *page_info;
  gint n_pages;
  CtkStyleContext *context;
  CtkWidget *box;

  g_return_val_if_fail (CTK_IS_ASSISTANT (assistant), 0);
  g_return_val_if_fail (CTK_IS_WIDGET (page), 0);
  g_return_val_if_fail (ctk_widget_get_parent (page) == NULL, 0);
  g_return_val_if_fail (!ctk_widget_is_toplevel (page), 0);

  priv = assistant->priv;

  page_info = g_slice_new0 (CtkAssistantPage);
  page_info->page  = page;
  page_info->regular_title = ctk_label_new (NULL);
  page_info->has_padding = TRUE;
  ctk_widget_set_no_show_all (page_info->regular_title, TRUE);
  page_info->current_title = ctk_label_new (NULL);
  ctk_widget_set_no_show_all (page_info->current_title, TRUE);

  ctk_label_set_xalign (CTK_LABEL (page_info->regular_title), 0.0);
  ctk_label_set_xalign (CTK_LABEL (page_info->current_title), 0.0);

  ctk_widget_show (page_info->regular_title);
  ctk_widget_hide (page_info->current_title);

  context = ctk_widget_get_style_context (page_info->current_title);
  ctk_style_context_add_class (context, CTK_STYLE_CLASS_HIGHLIGHT);

  ctk_size_group_add_widget (priv->title_size_group, page_info->regular_title);
  ctk_size_group_add_widget (priv->title_size_group, page_info->current_title);

  g_signal_connect (G_OBJECT (page), "notify::visible",
                    G_CALLBACK (on_page_notify), assistant);

  g_signal_connect (G_OBJECT (page), "child-notify::page-title",
                    G_CALLBACK (on_page_notify), assistant);

  g_signal_connect (G_OBJECT (page), "child-notify::page-type",
                    G_CALLBACK (on_page_notify), assistant);

  n_pages = g_list_length (priv->pages);

  if (position < 0 || position > n_pages)
    position = n_pages;

  priv->pages = g_list_insert (priv->pages, page_info, position);

  ctk_box_pack_start (CTK_BOX (priv->sidebar), page_info->regular_title, FALSE, FALSE, 0);
  ctk_box_pack_start (CTK_BOX (priv->sidebar), page_info->current_title, FALSE, FALSE, 0);
  ctk_box_reorder_child (CTK_BOX (priv->sidebar), page_info->regular_title, 2 * position);
  ctk_box_reorder_child (CTK_BOX (priv->sidebar), page_info->current_title, 2 * position + 1);

  box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
  ctk_widget_show (box);
  ctk_box_pack_start (CTK_BOX (box), page, TRUE, TRUE, 0);
  g_object_set (box, "margin", 12, NULL);
  g_signal_connect (box, "remove", G_CALLBACK (assistant_remove_page_cb), assistant);

  ctk_notebook_insert_page (CTK_NOTEBOOK (priv->content), box, NULL, position);

  page_info->box = box;

  if (ctk_widget_get_mapped (CTK_WIDGET (assistant)))
    {
      update_buttons_state (assistant);
      update_actions_size (assistant);
    }

  return position;
}

/**
 * ctk_assistant_remove_page:
 * @assistant: a #CtkAssistant
 * @page_num: the index of a page in the @assistant,
 *     or -1 to remove the last page
 *
 * Removes the @page_num’s page from @assistant.
 *
 * Since: 3.2
 */
void
ctk_assistant_remove_page (CtkAssistant *assistant,
                           gint          page_num)
{
  CtkWidget *page;

  g_return_if_fail (CTK_IS_ASSISTANT (assistant));

  page = ctk_assistant_get_nth_page (assistant, page_num);

  if (page)
    ctk_container_remove (CTK_CONTAINER (assistant), page);
}

/**
 * ctk_assistant_set_forward_page_func:
 * @assistant: a #CtkAssistant
 * @page_func: (allow-none): the #CtkAssistantPageFunc, or %NULL
 *     to use the default one
 * @data: user data for @page_func
 * @destroy: destroy notifier for @data
 *
 * Sets the page forwarding function to be @page_func.
 *
 * This function will be used to determine what will be
 * the next page when the user presses the forward button.
 * Setting @page_func to %NULL will make the assistant to
 * use the default forward function, which just goes to the
 * next visible page.
 *
 * Since: 2.10
 */
void
ctk_assistant_set_forward_page_func (CtkAssistant         *assistant,
                                     CtkAssistantPageFunc  page_func,
                                     gpointer              data,
                                     GDestroyNotify        destroy)
{
  CtkAssistantPrivate *priv;

  g_return_if_fail (CTK_IS_ASSISTANT (assistant));

  priv = assistant->priv;

  if (priv->forward_data_destroy &&
      priv->forward_function_data)
    (*priv->forward_data_destroy) (priv->forward_function_data);

  if (page_func)
    {
      priv->forward_function = page_func;
      priv->forward_function_data = data;
      priv->forward_data_destroy = destroy;
    }
  else
    {
      priv->forward_function = default_forward_function;
      priv->forward_function_data = assistant;
      priv->forward_data_destroy = NULL;
    }

  /* Page flow has possibly changed, so the
   * buttons state might need to change too
   */
  if (ctk_widget_get_mapped (CTK_WIDGET (assistant)))
    update_buttons_state (assistant);
}

static void
add_to_action_area (CtkAssistant *assistant,
                    CtkWidget    *child)
{
  CtkAssistantPrivate *priv = assistant->priv;

  ctk_widget_set_valign (child, CTK_ALIGN_BASELINE);

  ctk_box_pack_end (CTK_BOX (priv->action_area), child, FALSE, FALSE, 0);
}

/**
 * ctk_assistant_add_action_widget:
 * @assistant: a #CtkAssistant
 * @child: a #CtkWidget
 *
 * Adds a widget to the action area of a #CtkAssistant.
 *
 * Since: 2.10
 */
void
ctk_assistant_add_action_widget (CtkAssistant *assistant,
                                 CtkWidget    *child)
{
  CtkAssistantPrivate *priv;

  g_return_if_fail (CTK_IS_ASSISTANT (assistant));
  g_return_if_fail (CTK_IS_WIDGET (child));

  priv = assistant->priv;

  if (CTK_IS_BUTTON (child))
    {
      ctk_size_group_add_widget (priv->button_size_group, child);
      priv->extra_buttons += 1;
      if (ctk_widget_get_mapped (CTK_WIDGET (assistant)))
        update_actions_size (assistant);
    }

  if (priv->constructed && priv->use_header_bar)
    add_to_header_bar (assistant, child);
  else
    add_to_action_area (assistant, child);
}

/**
 * ctk_assistant_remove_action_widget:
 * @assistant: a #CtkAssistant
 * @child: a #CtkWidget
 *
 * Removes a widget from the action area of a #CtkAssistant.
 *
 * Since: 2.10
 */
void
ctk_assistant_remove_action_widget (CtkAssistant *assistant,
                                    CtkWidget    *child)
{
  CtkAssistantPrivate *priv;

  g_return_if_fail (CTK_IS_ASSISTANT (assistant));
  g_return_if_fail (CTK_IS_WIDGET (child));

  priv = assistant->priv;

  if (CTK_IS_BUTTON (child))
    {
      ctk_size_group_remove_widget (priv->button_size_group, child);
      priv->extra_buttons -= 1;
      if (ctk_widget_get_mapped (CTK_WIDGET (assistant)))
        update_actions_size (assistant);
    }

  ctk_container_remove (CTK_CONTAINER (priv->action_area), child);
}

/**
 * ctk_assistant_set_page_title:
 * @assistant: a #CtkAssistant
 * @page: a page of @assistant
 * @title: the new title for @page
 *
 * Sets a title for @page.
 *
 * The title is displayed in the header area of the assistant
 * when @page is the current page.
 *
 * Since: 2.10
 */
void
ctk_assistant_set_page_title (CtkAssistant *assistant,
                              CtkWidget    *page,
                              const gchar  *title)
{
  CtkAssistantPage *page_info;
  GList *child;

  g_return_if_fail (CTK_IS_ASSISTANT (assistant));
  g_return_if_fail (CTK_IS_WIDGET (page));

  child = find_page (assistant, page);

  g_return_if_fail (child != NULL);

  page_info = (CtkAssistantPage*) child->data;

  g_free (page_info->title);
  page_info->title = g_strdup (title);

  ctk_label_set_text ((CtkLabel*) page_info->regular_title, title);
  ctk_label_set_text ((CtkLabel*) page_info->current_title, title);

  update_title_state (assistant);

  ctk_container_child_notify (CTK_CONTAINER (assistant), page, "title");
}

/**
 * ctk_assistant_get_page_title:
 * @assistant: a #CtkAssistant
 * @page: a page of @assistant
 *
 * Gets the title for @page.
 *
 * Returns: the title for @page
 *
 * Since: 2.10
 */
const gchar*
ctk_assistant_get_page_title (CtkAssistant *assistant,
                              CtkWidget    *page)
{
  CtkAssistantPage *page_info;
  GList *child;

  g_return_val_if_fail (CTK_IS_ASSISTANT (assistant), NULL);
  g_return_val_if_fail (CTK_IS_WIDGET (page), NULL);

  child = find_page (assistant, page);

  g_return_val_if_fail (child != NULL, NULL);

  page_info = (CtkAssistantPage*) child->data;

  return page_info->title;
}

/**
 * ctk_assistant_set_page_type:
 * @assistant: a #CtkAssistant
 * @page: a page of @assistant
 * @type: the new type for @page
 *
 * Sets the page type for @page.
 *
 * The page type determines the page behavior in the @assistant.
 *
 * Since: 2.10
 */
void
ctk_assistant_set_page_type (CtkAssistant         *assistant,
                             CtkWidget            *page,
                             CtkAssistantPageType  type)
{
  CtkAssistantPage *page_info;
  GList *child;

  g_return_if_fail (CTK_IS_ASSISTANT (assistant));
  g_return_if_fail (CTK_IS_WIDGET (page));

  child = find_page (assistant, page);

  g_return_if_fail (child != NULL);

  page_info = (CtkAssistantPage*) child->data;

  if (type != page_info->type)
    {
      page_info->type = type;

      /* backwards compatibility to the era before fixing bug 604289 */
      if (type == CTK_ASSISTANT_PAGE_SUMMARY && !page_info->complete_set)
        {
          ctk_assistant_set_page_complete (assistant, page, TRUE);
          page_info->complete_set = FALSE;
        }

      /* Always set buttons state, a change in a future page
       * might change current page buttons
       */
      update_buttons_state (assistant);

      ctk_container_child_notify (CTK_CONTAINER (assistant), page, "page-type");
    }
}

/**
 * ctk_assistant_get_page_type:
 * @assistant: a #CtkAssistant
 * @page: a page of @assistant
 *
 * Gets the page type of @page.
 *
 * Returns: the page type of @page
 *
 * Since: 2.10
 */
CtkAssistantPageType
ctk_assistant_get_page_type (CtkAssistant *assistant,
                             CtkWidget    *page)
{
  CtkAssistantPage *page_info;
  GList *child;

  g_return_val_if_fail (CTK_IS_ASSISTANT (assistant), CTK_ASSISTANT_PAGE_CONTENT);
  g_return_val_if_fail (CTK_IS_WIDGET (page), CTK_ASSISTANT_PAGE_CONTENT);

  child = find_page (assistant, page);

  g_return_val_if_fail (child != NULL, CTK_ASSISTANT_PAGE_CONTENT);

  page_info = (CtkAssistantPage*) child->data;

  return page_info->type;
}

/**
 * ctk_assistant_set_page_header_image:
 * @assistant: a #CtkAssistant
 * @page: a page of @assistant
 * @pixbuf: (allow-none): the new header image @page
 *
 * Sets a header image for @page.
 *
 * Since: 2.10
 *
 * Deprecated: 3.2: Since CTK+ 3.2, a header is no longer shown;
 *     add your header decoration to the page content instead.
 */
void
ctk_assistant_set_page_header_image (CtkAssistant *assistant,
                                     CtkWidget    *page,
                                     GdkPixbuf    *pixbuf)
{
  g_return_if_fail (CTK_IS_ASSISTANT (assistant));
  g_return_if_fail (CTK_IS_WIDGET (page));
  g_return_if_fail (pixbuf == NULL || GDK_IS_PIXBUF (pixbuf));

  ctk_assistant_do_set_page_header_image (assistant, page, pixbuf);
}

static void
ctk_assistant_do_set_page_header_image (CtkAssistant *assistant,
                                        CtkWidget    *page,
                                        GdkPixbuf    *pixbuf)
{
  CtkAssistantPage *page_info;
  GList *child;

  child = find_page (assistant, page);

  g_return_if_fail (child != NULL);

  page_info = (CtkAssistantPage*) child->data;

  if (pixbuf != page_info->header_image)
    {
      if (page_info->header_image)
        {
          g_object_unref (page_info->header_image);
          page_info->header_image = NULL;
        }

      if (pixbuf)
        page_info->header_image = g_object_ref (pixbuf);

      ctk_container_child_notify (CTK_CONTAINER (assistant), page, "header-image");
    }
}

/**
 * ctk_assistant_get_page_header_image:
 * @assistant: a #CtkAssistant
 * @page: a page of @assistant
 *
 * Gets the header image for @page.
 *
 * Returns: (transfer none): the header image for @page,
 *     or %NULL if there’s no header image for the page
 *
 * Since: 2.10
 *
 * Deprecated: 3.2: Since CTK+ 3.2, a header is no longer shown;
 *     add your header decoration to the page content instead.
 */
GdkPixbuf*
ctk_assistant_get_page_header_image (CtkAssistant *assistant,
                                     CtkWidget    *page)
{
  CtkAssistantPage *page_info;
  GList *child;

  g_return_val_if_fail (CTK_IS_ASSISTANT (assistant), NULL);
  g_return_val_if_fail (CTK_IS_WIDGET (page), NULL);

  child = find_page (assistant, page);

  g_return_val_if_fail (child != NULL, NULL);

  page_info = (CtkAssistantPage*) child->data;

  return page_info->header_image;
}

/**
 * ctk_assistant_set_page_side_image:
 * @assistant: a #CtkAssistant
 * @page: a page of @assistant
 * @pixbuf: (allow-none): the new side image @page
 *
 * Sets a side image for @page.
 *
 * This image used to be displayed in the side area of the assistant
 * when @page is the current page.
 *
 * Since: 2.10
 *
 * Deprecated: 3.2: Since CTK+ 3.2, sidebar images are not
 *     shown anymore.
 */
void
ctk_assistant_set_page_side_image (CtkAssistant *assistant,
                                   CtkWidget    *page,
                                   GdkPixbuf    *pixbuf)
{
  g_return_if_fail (CTK_IS_ASSISTANT (assistant));
  g_return_if_fail (CTK_IS_WIDGET (page));
  g_return_if_fail (pixbuf == NULL || GDK_IS_PIXBUF (pixbuf));

  ctk_assistant_do_set_page_side_image (assistant, page, pixbuf);
}

static void
ctk_assistant_do_set_page_side_image (CtkAssistant *assistant,
                                      CtkWidget    *page,
                                      GdkPixbuf    *pixbuf)
{
  CtkAssistantPage *page_info;
  GList *child;

  child = find_page (assistant, page);

  g_return_if_fail (child != NULL);

  page_info = (CtkAssistantPage*) child->data;

  if (pixbuf != page_info->sidebar_image)
    {
      if (page_info->sidebar_image)
        {
          g_object_unref (page_info->sidebar_image);
          page_info->sidebar_image = NULL;
        }

      if (pixbuf)
        page_info->sidebar_image = g_object_ref (pixbuf);

      ctk_container_child_notify (CTK_CONTAINER (assistant), page, "sidebar-image");
    }
}

/**
 * ctk_assistant_get_page_side_image:
 * @assistant: a #CtkAssistant
 * @page: a page of @assistant
 *
 * Gets the side image for @page.
 *
 * Returns: (transfer none): the side image for @page,
 *     or %NULL if there’s no side image for the page
 *
 * Since: 2.10
 *
 * Deprecated: 3.2: Since CTK+ 3.2, sidebar images are not
 *     shown anymore.
 */
GdkPixbuf*
ctk_assistant_get_page_side_image (CtkAssistant *assistant,
                                   CtkWidget    *page)
{
  CtkAssistantPage *page_info;
  GList *child;

  g_return_val_if_fail (CTK_IS_ASSISTANT (assistant), NULL);
  g_return_val_if_fail (CTK_IS_WIDGET (page), NULL);

  child = find_page (assistant, page);

  g_return_val_if_fail (child != NULL, NULL);

  page_info = (CtkAssistantPage*) child->data;

  return page_info->sidebar_image;
}

/**
 * ctk_assistant_set_page_complete:
 * @assistant: a #CtkAssistant
 * @page: a page of @assistant
 * @complete: the completeness status of the page
 *
 * Sets whether @page contents are complete.
 *
 * This will make @assistant update the buttons state
 * to be able to continue the task.
 *
 * Since: 2.10
 */
void
ctk_assistant_set_page_complete (CtkAssistant *assistant,
                                 CtkWidget    *page,
                                 gboolean      complete)
{
  CtkAssistantPage *page_info;
  GList *child;

  g_return_if_fail (CTK_IS_ASSISTANT (assistant));
  g_return_if_fail (CTK_IS_WIDGET (page));

  child = find_page (assistant, page);

  g_return_if_fail (child != NULL);

  page_info = (CtkAssistantPage*) child->data;

  if (complete != page_info->complete)
    {
      page_info->complete = complete;
      page_info->complete_set = TRUE;

      /* Always set buttons state, a change in a future page
       * might change current page buttons
       */
      update_buttons_state (assistant);

      ctk_container_child_notify (CTK_CONTAINER (assistant), page, "complete");
    }
}

/**
 * ctk_assistant_get_page_complete:
 * @assistant: a #CtkAssistant
 * @page: a page of @assistant
 *
 * Gets whether @page is complete.
 *
 * Returns: %TRUE if @page is complete.
 *
 * Since: 2.10
 */
gboolean
ctk_assistant_get_page_complete (CtkAssistant *assistant,
                                 CtkWidget    *page)
{
  CtkAssistantPage *page_info;
  GList *child;

  g_return_val_if_fail (CTK_IS_ASSISTANT (assistant), FALSE);
  g_return_val_if_fail (CTK_IS_WIDGET (page), FALSE);

  child = find_page (assistant, page);

  g_return_val_if_fail (child != NULL, FALSE);

  page_info = (CtkAssistantPage*) child->data;

  return page_info->complete;
}

/**
 * ctk_assistant_set_page_has_padding:
 * @assistant: a #CtkAssistant
 * @page: a page of @assistant
 * @has_padding: whether this page has padding
 *
 * Sets whether the assistant is adding padding around
 * the page.
 *
 * Since: 3.18
 */
void
ctk_assistant_set_page_has_padding (CtkAssistant *assistant,
                                    CtkWidget    *page,
                                    gboolean      has_padding)
{
  CtkAssistantPage *page_info;
  GList *child;

  g_return_if_fail (CTK_IS_ASSISTANT (assistant));
  g_return_if_fail (CTK_IS_WIDGET (page));

  child = find_page (assistant, page);

  g_return_if_fail (child != NULL);

  page_info = (CtkAssistantPage*) child->data;

  if (page_info->has_padding != has_padding)
    {
      page_info->has_padding = has_padding;

      g_object_set (page_info->box,
                    "margin", has_padding ? 12 : 0,
                    NULL);

      ctk_container_child_notify (CTK_CONTAINER (assistant), page, "has-padding");
    }
}

/**
 * ctk_assistant_get_page_has_padding:
 * @assistant: a #CtkAssistant
 * @page: a page of @assistant
 *
 * Gets whether page has padding.
 *
 * Returns: %TRUE if @page has padding
 * Since: 3.18
 */
gboolean
ctk_assistant_get_page_has_padding (CtkAssistant *assistant,
                                    CtkWidget    *page)
{
  CtkAssistantPage *page_info;
  GList *child;

  g_return_val_if_fail (CTK_IS_ASSISTANT (assistant), FALSE);
  g_return_val_if_fail (CTK_IS_WIDGET (page), FALSE);

  child = find_page (assistant, page);

  g_return_val_if_fail (child != NULL, TRUE);

  page_info = (CtkAssistantPage*) child->data;

  return page_info->has_padding;
}

/**
 * ctk_assistant_update_buttons_state:
 * @assistant: a #CtkAssistant
 *
 * Forces @assistant to recompute the buttons state.
 *
 * CTK+ automatically takes care of this in most situations,
 * e.g. when the user goes to a different page, or when the
 * visibility or completeness of a page changes.
 *
 * One situation where it can be necessary to call this
 * function is when changing a value on the current page
 * affects the future page flow of the assistant.
 *
 * Since: 2.10
 */
void
ctk_assistant_update_buttons_state (CtkAssistant *assistant)
{
  g_return_if_fail (CTK_IS_ASSISTANT (assistant));

  update_buttons_state (assistant);
}

/**
 * ctk_assistant_commit:
 * @assistant: a #CtkAssistant
 *
 * Erases the visited page history so the back button is not
 * shown on the current page, and removes the cancel button
 * from subsequent pages.
 *
 * Use this when the information provided up to the current
 * page is hereafter deemed permanent and cannot be modified
 * or undone. For example, showing a progress page to track
 * a long-running, unreversible operation after the user has
 * clicked apply on a confirmation page.
 *
 * Since: 2.22
 */
void
ctk_assistant_commit (CtkAssistant *assistant)
{
  g_return_if_fail (CTK_IS_ASSISTANT (assistant));

  g_slist_free (assistant->priv->visited_pages);
  assistant->priv->visited_pages = NULL;

  assistant->priv->committed = TRUE;

  update_buttons_state (assistant);
}

/* accessible implementation */

/* dummy typedefs */
typedef CtkWindowAccessible      CtkAssistantAccessible;
typedef CtkWindowAccessibleClass CtkAssistantAccessibleClass;

G_DEFINE_TYPE (CtkAssistantAccessible, _ctk_assistant_accessible, CTK_TYPE_WINDOW_ACCESSIBLE);

static gint
ctk_assistant_accessible_get_n_children (AtkObject *accessible)
{
  CtkWidget *widget;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (accessible));
  if (widget == NULL)
    return 0;

  return g_list_length (CTK_ASSISTANT (widget)->priv->pages) + 2;
}

static AtkObject *
ctk_assistant_accessible_ref_child (AtkObject *accessible,
                                    gint       index)
{
  CtkAssistant *assistant;
  CtkAssistantPrivate *priv;
  CtkWidget *widget, *child;
  gint n_pages;
  AtkObject *obj;
  const gchar *title;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (accessible));
  if (widget == NULL)
    return NULL;

  assistant = CTK_ASSISTANT (widget);
  priv = assistant->priv;
  n_pages = g_list_length (priv->pages);

  if (index < 0)
    return NULL;
  else if (index < n_pages)
    {
      CtkAssistantPage *page = g_list_nth_data (priv->pages, index);

      child = page->page;
      title = ctk_assistant_get_page_title (assistant, child);
    }
  else if (index == n_pages)
    {
      child = priv->action_area;
      title = NULL;
    }
  else if (index == n_pages + 1)
    {
      child = priv->headerbar;
      title = NULL;
    }
  else
    return NULL;

  obj = ctk_widget_get_accessible (child);

  if (title)
    atk_object_set_name (obj, title);

  return g_object_ref (obj);
}

static void
_ctk_assistant_accessible_class_init (CtkAssistantAccessibleClass *klass)
{
  AtkObjectClass *atk_class = ATK_OBJECT_CLASS (klass);

  atk_class->get_n_children = ctk_assistant_accessible_get_n_children;
  atk_class->ref_child = ctk_assistant_accessible_ref_child;
}

static void
_ctk_assistant_accessible_init (CtkAssistantAccessible *self G_GNUC_UNUSED)
{
}

/* buildable implementation */

static CtkBuildableIface *parent_buildable_iface;

static void
ctk_assistant_buildable_interface_init (CtkBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);
  iface->custom_tag_start = ctk_assistant_buildable_custom_tag_start;
  iface->custom_finished = ctk_assistant_buildable_custom_finished;
}

gboolean
ctk_assistant_buildable_custom_tag_start (CtkBuildable  *buildable,
                                          CtkBuilder    *builder,
                                          GObject       *child,
                                          const gchar   *tagname,
                                          GMarkupParser *parser,
                                          gpointer      *data)
{
  return parent_buildable_iface->custom_tag_start (buildable, builder, child,
                                                   tagname, parser, data);
}

static void
ctk_assistant_buildable_custom_finished (CtkBuildable *buildable,
                                         CtkBuilder   *builder,
                                         GObject      *child,
                                         const gchar  *tagname,
                                         gpointer      user_data)
{
  parent_buildable_iface->custom_finished (buildable, builder, child,
                                           tagname, user_data);
}
