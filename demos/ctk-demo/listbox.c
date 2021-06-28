/* List Box
 *
 * CtkListBox allows lists with complicated layouts, using
 * regular widgets supporting sorting and filtering.
 *
 */

#include <ctk/ctk.h>
#include <stdlib.h>
#include <string.h>

static CdkPixbuf *avatar_pixbuf_other;
static CtkWidget *window = NULL;

#define CTK_TYPE_MESSAGE		  (ctk_message_get_type ())
#define CTK_MESSAGE(message)		  (G_TYPE_CHECK_INSTANCE_CAST ((message), CTK_TYPE_MESSAGE, CtkMessage))
#define CTK_MESSAGE_CLASS(klass)		  (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_MESSAGE, CtkMessageClass))
#define CTK_IS_MESSAGE(message)		  (G_TYPE_CHECK_INSTANCE_TYPE ((message), CTK_TYPE_MESSAGE))
#define CTK_IS_MESSAGE_CLASS(klass)	  (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_MESSAGE))
#define CTK_MESSAGE_GET_CLASS(obj)         (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_MESSAGE, CtkMessageClass))

#define CTK_TYPE_MESSAGE_ROW		  (ctk_message_row_get_type ())
#define CTK_MESSAGE_ROW(message_row)		  (G_TYPE_CHECK_INSTANCE_CAST ((message_row), CTK_TYPE_MESSAGE_ROW, CtkMessageRow))
#define CTK_MESSAGE_ROW_CLASS(klass)		  (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_MESSAGE_ROW, CtkMessageRowClass))
#define CTK_IS_MESSAGE_ROW(message_row)		  (G_TYPE_CHECK_INSTANCE_TYPE ((message_row), CTK_TYPE_MESSAGE_ROW))
#define CTK_IS_MESSAGE_ROW_CLASS(klass)	  (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_MESSAGE_ROW))
#define CTK_MESSAGE_ROW_GET_CLASS(obj)         (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_MESSAGE_ROW, CtkMessageRowClass))

typedef struct _CtkMessage   CtkMessage;
typedef struct _CtkMessageClass  CtkMessageClass;
typedef struct _CtkMessageRow   CtkMessageRow;
typedef struct _CtkMessageRowClass  CtkMessageRowClass;
typedef struct _CtkMessageRowPrivate  CtkMessageRowPrivate;


struct _CtkMessage
{
  GObject parent;

  guint id;
  char *sender_name;
  char *sender_nick;
  char *message;
  gint64 time;
  guint reply_to;
  char *resent_by;
  int n_favorites;
  int n_reshares;
};

struct _CtkMessageClass
{
  GObjectClass parent_class;
};

struct _CtkMessageRow
{
  CtkListBoxRow parent;

  CtkMessageRowPrivate *priv;
};

struct _CtkMessageRowClass
{
  CtkListBoxRowClass parent_class;
};

struct _CtkMessageRowPrivate
{
  CtkMessage *message;
  CtkRevealer *details_revealer;
  CtkImage *avatar_image;
  CtkWidget *extra_buttons_box;
  CtkLabel *content_label;
  CtkLabel *source_name;
  CtkLabel *source_nick;
  CtkLabel *short_time_label;
  CtkLabel *detailed_time_label;
  CtkBox *resent_box;
  CtkLinkButton *resent_by_button;
  CtkLabel *n_favorites_label;
  CtkLabel *n_reshares_label;
  CtkButton *expand_button;
};

GType      ctk_message_get_type  (void) G_GNUC_CONST;
GType      ctk_message_row_get_type  (void) G_GNUC_CONST;

G_DEFINE_TYPE (CtkMessage, ctk_message, G_TYPE_OBJECT);

static void
ctk_message_finalize (GObject *obj)
{
  CtkMessage *msg = CTK_MESSAGE (obj);

  g_free (msg->sender_name);
  g_free (msg->sender_nick);
  g_free (msg->message);
  g_free (msg->resent_by);

  G_OBJECT_CLASS (ctk_message_parent_class)->finalize (obj);
}
static void
ctk_message_class_init (CtkMessageClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = ctk_message_finalize;
}

static void
ctk_message_init (CtkMessage *msg)
{
}

static void
ctk_message_parse (CtkMessage *msg, const char *str)
{
  char **strv;
  int i;

  strv = g_strsplit (str, "|", 0);

  i = 0;
  msg->id = strtol (strv[i++], NULL, 10);
  msg->sender_name = g_strdup (strv[i++]);
  msg->sender_nick = g_strdup (strv[i++]);
  msg->message = g_strdup (strv[i++]);
  msg->time = strtol (strv[i++], NULL, 10);
  if (strv[i])
    {
      msg->reply_to = strtol (strv[i++], NULL, 10);
      if (strv[i])
        {
          if (*strv[i])
            msg->resent_by = g_strdup (strv[i]);
          i++;
          if (strv[i])
            {
              msg->n_favorites = strtol (strv[i++], NULL, 10);
              if (strv[i])
                {
                  msg->n_reshares = strtol (strv[i++], NULL, 10);
                }

            }
        }
    }

  g_strfreev (strv);
}

static CtkMessage *
ctk_message_new (const char *str)
{
  CtkMessage *msg;
  msg = g_object_new (ctk_message_get_type (), NULL);
  ctk_message_parse (msg, str);
  return msg;
}

G_DEFINE_TYPE_WITH_PRIVATE (CtkMessageRow, ctk_message_row, CTK_TYPE_LIST_BOX_ROW);


static void
ctk_message_row_update (CtkMessageRow *row)
{
  CtkMessageRowPrivate *priv = row->priv;
  GDateTime *t;
  char *s;

  ctk_label_set_text (priv->source_name, priv->message->sender_name);
  ctk_label_set_text (priv->source_nick, priv->message->sender_nick);
  ctk_label_set_text (priv->content_label, priv->message->message);
  t = g_date_time_new_from_unix_utc (priv->message->time);
  s = g_date_time_format (t, "%e %b %y");
  ctk_label_set_text (priv->short_time_label, s);
  g_free (s);
  s = g_date_time_format (t, "%X - %e %b %Y");
  ctk_label_set_text (priv->detailed_time_label, s);
  g_free (s);
  g_date_time_unref (t);

  ctk_widget_set_visible (CTK_WIDGET(priv->n_favorites_label),
                          priv->message->n_favorites != 0);
  s = g_strdup_printf ("<b>%d</b>\nFavorites", priv->message->n_favorites);
  ctk_label_set_markup (priv->n_favorites_label, s);
  g_free (s);

  ctk_widget_set_visible (CTK_WIDGET(priv->n_reshares_label),
                          priv->message->n_reshares != 0);
  s = g_strdup_printf ("<b>%d</b>\nReshares", priv->message->n_reshares);
  ctk_label_set_markup (priv->n_reshares_label, s);
  g_free (s);

  ctk_widget_set_visible (CTK_WIDGET (priv->resent_box), priv->message->resent_by != NULL);
  if (priv->message->resent_by)
    ctk_button_set_label (CTK_BUTTON (priv->resent_by_button), priv->message->resent_by);

  if (strcmp (priv->message->sender_nick, "@CTKtoolkit") == 0)
    ctk_image_set_from_icon_name (priv->avatar_image, "ctk3-demo", CTK_ICON_SIZE_DND);
  else
    ctk_image_set_from_pixbuf (priv->avatar_image, avatar_pixbuf_other);

}

static void
ctk_message_row_expand (CtkMessageRow *row)
{
  CtkMessageRowPrivate *priv = row->priv;
  gboolean expand;

  expand = !ctk_revealer_get_reveal_child (priv->details_revealer);

  ctk_revealer_set_reveal_child (priv->details_revealer, expand);
  if (expand)
    ctk_button_set_label (priv->expand_button, "Hide");
  else
    ctk_button_set_label (priv->expand_button, "Expand");
}

static void
expand_clicked (CtkMessageRow *row,
                CtkButton *button)
{
  ctk_message_row_expand (row);
}

static void
reshare_clicked (CtkMessageRow *row,
                 CtkButton *button)
{
  CtkMessageRowPrivate *priv = row->priv;

  priv->message->n_reshares++;
  ctk_message_row_update (row);

}

static void
favorite_clicked (CtkMessageRow *row,
                  CtkButton *button)
{
  CtkMessageRowPrivate *priv = row->priv;

  priv->message->n_favorites++;
  ctk_message_row_update (row);
}

static void
ctk_message_row_state_flags_changed (CtkWidget    *widget,
                                     CtkStateFlags previous_state_flags)
{
  CtkMessageRowPrivate *priv = CTK_MESSAGE_ROW (widget)->priv;
  CtkStateFlags flags;

  flags = ctk_widget_get_state_flags (widget);

  ctk_widget_set_visible (priv->extra_buttons_box,
                          flags & (CTK_STATE_FLAG_PRELIGHT | CTK_STATE_FLAG_SELECTED));

  CTK_WIDGET_CLASS (ctk_message_row_parent_class)->state_flags_changed (widget, previous_state_flags);
}

static void
ctk_message_row_finalize (GObject *obj)
{
  CtkMessageRowPrivate *priv = CTK_MESSAGE_ROW (obj)->priv;
  g_object_unref (priv->message);
  G_OBJECT_CLASS (ctk_message_row_parent_class)->finalize(obj);
}

static void
ctk_message_row_class_init (CtkMessageRowClass *klass)
{
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = ctk_message_row_finalize;

  ctk_widget_class_set_template_from_resource (widget_class, "/listbox/listbox.ui");
  ctk_widget_class_bind_template_child_private (widget_class, CtkMessageRow, content_label);
  ctk_widget_class_bind_template_child_private (widget_class, CtkMessageRow, source_name);
  ctk_widget_class_bind_template_child_private (widget_class, CtkMessageRow, source_nick);
  ctk_widget_class_bind_template_child_private (widget_class, CtkMessageRow, short_time_label);
  ctk_widget_class_bind_template_child_private (widget_class, CtkMessageRow, detailed_time_label);
  ctk_widget_class_bind_template_child_private (widget_class, CtkMessageRow, extra_buttons_box);
  ctk_widget_class_bind_template_child_private (widget_class, CtkMessageRow, details_revealer);
  ctk_widget_class_bind_template_child_private (widget_class, CtkMessageRow, avatar_image);
  ctk_widget_class_bind_template_child_private (widget_class, CtkMessageRow, resent_box);
  ctk_widget_class_bind_template_child_private (widget_class, CtkMessageRow, resent_by_button);
  ctk_widget_class_bind_template_child_private (widget_class, CtkMessageRow, n_reshares_label);
  ctk_widget_class_bind_template_child_private (widget_class, CtkMessageRow, n_favorites_label);
  ctk_widget_class_bind_template_child_private (widget_class, CtkMessageRow, expand_button);
  ctk_widget_class_bind_template_callback (widget_class, expand_clicked);
  ctk_widget_class_bind_template_callback (widget_class, reshare_clicked);
  ctk_widget_class_bind_template_callback (widget_class, favorite_clicked);

  widget_class->state_flags_changed = ctk_message_row_state_flags_changed;
}

static void
ctk_message_row_init (CtkMessageRow *row)
{
  row->priv = ctk_message_row_get_instance_private (row);

  ctk_widget_init_template (CTK_WIDGET (row));
}

static CtkMessageRow *
ctk_message_row_new (CtkMessage *message)
{
  CtkMessageRow *row;

  row = g_object_new (ctk_message_row_get_type (), NULL);
  row->priv->message = message;
  ctk_message_row_update (row);

  return row;
}

static int
ctk_message_row_sort (CtkMessageRow *a, CtkMessageRow *b, gpointer data)
{
  return b->priv->message->time - a->priv->message->time;
}

static void
row_activated (CtkListBox *listbox, CtkListBoxRow *row)
{
  ctk_message_row_expand (CTK_MESSAGE_ROW (row));
}

CtkWidget *
do_listbox (CtkWidget *do_widget)
{
  CtkWidget *scrolled, *listbox, *vbox, *label;
  CtkMessage *message;
  CtkMessageRow *row;
  GBytes *data;
  char **lines;
  int i;

  if (!window)
    {
      avatar_pixbuf_other = cdk_pixbuf_new_from_resource_at_scale ("/listbox/apple-red.png", 32, 32, FALSE, NULL);

      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_screen (CTK_WINDOW (window),
                             ctk_widget_get_screen (do_widget));
      ctk_window_set_title (CTK_WINDOW (window), "List Box");
      ctk_window_set_default_size (CTK_WINDOW (window),
                                   400, 600);

      /* NULL window variable when window is closed */
      g_signal_connect (window, "destroy",
                        G_CALLBACK (ctk_widget_destroyed),
                        &window);

      vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 12);
      ctk_container_add (CTK_CONTAINER (window), vbox);
      label = ctk_label_new ("Messages from Ctk+ and friends");
      ctk_box_pack_start (CTK_BOX (vbox), label, FALSE, FALSE, 0);
      scrolled = ctk_scrolled_window_new (NULL, NULL);
      ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (scrolled), CTK_POLICY_NEVER, CTK_POLICY_AUTOMATIC);
      ctk_box_pack_start (CTK_BOX (vbox), scrolled, TRUE, TRUE, 0);
      listbox = ctk_list_box_new ();
      ctk_container_add (CTK_CONTAINER (scrolled), listbox);

      ctk_list_box_set_sort_func (CTK_LIST_BOX (listbox), (CtkListBoxSortFunc)ctk_message_row_sort, listbox, NULL);
      ctk_list_box_set_activate_on_single_click (CTK_LIST_BOX (listbox), FALSE);
      g_signal_connect (listbox, "row-activated", G_CALLBACK (row_activated), NULL);

      ctk_widget_show_all (vbox);

      data = g_resources_lookup_data ("/listbox/messages.txt", 0, NULL);
      lines = g_strsplit (g_bytes_get_data (data, NULL), "\n", 0);

      for (i = 0; lines[i] != NULL && *lines[i]; i++)
        {
          message = ctk_message_new (lines[i]);
          row = ctk_message_row_new (message);
          ctk_widget_show (CTK_WIDGET (row));
          ctk_container_add (CTK_CONTAINER (listbox), CTK_WIDGET (row));
        }

      g_strfreev (lines);
      g_bytes_unref (data);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show (window);
  else
    ctk_widget_destroy (window);

  return window;
}
