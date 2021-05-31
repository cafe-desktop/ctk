/* List Box
 *
 * GtkListBox allows lists with complicated layouts, using
 * regular widgets supporting sorting and filtering.
 *
 */

#include <ctk/ctk.h>
#include <stdlib.h>
#include <string.h>

static GdkPixbuf *avatar_pixbuf_other;
static GtkWidget *window = NULL;

#define CTK_TYPE_MESSAGE		  (ctk_message_get_type ())
#define CTK_MESSAGE(message)		  (G_TYPE_CHECK_INSTANCE_CAST ((message), CTK_TYPE_MESSAGE, GtkMessage))
#define CTK_MESSAGE_CLASS(klass)		  (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_MESSAGE, GtkMessageClass))
#define CTK_IS_MESSAGE(message)		  (G_TYPE_CHECK_INSTANCE_TYPE ((message), CTK_TYPE_MESSAGE))
#define CTK_IS_MESSAGE_CLASS(klass)	  (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_MESSAGE))
#define CTK_MESSAGE_GET_CLASS(obj)         (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_MESSAGE, GtkMessageClass))

#define CTK_TYPE_MESSAGE_ROW		  (ctk_message_row_get_type ())
#define CTK_MESSAGE_ROW(message_row)		  (G_TYPE_CHECK_INSTANCE_CAST ((message_row), CTK_TYPE_MESSAGE_ROW, GtkMessageRow))
#define CTK_MESSAGE_ROW_CLASS(klass)		  (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_MESSAGE_ROW, GtkMessageRowClass))
#define CTK_IS_MESSAGE_ROW(message_row)		  (G_TYPE_CHECK_INSTANCE_TYPE ((message_row), CTK_TYPE_MESSAGE_ROW))
#define CTK_IS_MESSAGE_ROW_CLASS(klass)	  (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_MESSAGE_ROW))
#define CTK_MESSAGE_ROW_GET_CLASS(obj)         (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_MESSAGE_ROW, GtkMessageRowClass))

typedef struct _GtkMessage   GtkMessage;
typedef struct _GtkMessageClass  GtkMessageClass;
typedef struct _GtkMessageRow   GtkMessageRow;
typedef struct _GtkMessageRowClass  GtkMessageRowClass;
typedef struct _GtkMessageRowPrivate  GtkMessageRowPrivate;


struct _GtkMessage
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

struct _GtkMessageClass
{
  GObjectClass parent_class;
};

struct _GtkMessageRow
{
  GtkListBoxRow parent;

  GtkMessageRowPrivate *priv;
};

struct _GtkMessageRowClass
{
  GtkListBoxRowClass parent_class;
};

struct _GtkMessageRowPrivate
{
  GtkMessage *message;
  GtkRevealer *details_revealer;
  GtkImage *avatar_image;
  GtkWidget *extra_buttons_box;
  GtkLabel *content_label;
  GtkLabel *source_name;
  GtkLabel *source_nick;
  GtkLabel *short_time_label;
  GtkLabel *detailed_time_label;
  GtkBox *resent_box;
  GtkLinkButton *resent_by_button;
  GtkLabel *n_favorites_label;
  GtkLabel *n_reshares_label;
  GtkButton *expand_button;
};

GType      ctk_message_get_type  (void) G_GNUC_CONST;
GType      ctk_message_row_get_type  (void) G_GNUC_CONST;

G_DEFINE_TYPE (GtkMessage, ctk_message, G_TYPE_OBJECT);

static void
ctk_message_finalize (GObject *obj)
{
  GtkMessage *msg = CTK_MESSAGE (obj);

  g_free (msg->sender_name);
  g_free (msg->sender_nick);
  g_free (msg->message);
  g_free (msg->resent_by);

  G_OBJECT_CLASS (ctk_message_parent_class)->finalize (obj);
}
static void
ctk_message_class_init (GtkMessageClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = ctk_message_finalize;
}

static void
ctk_message_init (GtkMessage *msg)
{
}

static void
ctk_message_parse (GtkMessage *msg, const char *str)
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

static GtkMessage *
ctk_message_new (const char *str)
{
  GtkMessage *msg;
  msg = g_object_new (ctk_message_get_type (), NULL);
  ctk_message_parse (msg, str);
  return msg;
}

G_DEFINE_TYPE_WITH_PRIVATE (GtkMessageRow, ctk_message_row, CTK_TYPE_LIST_BOX_ROW);


static void
ctk_message_row_update (GtkMessageRow *row)
{
  GtkMessageRowPrivate *priv = row->priv;
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

  if (strcmp (priv->message->sender_nick, "@GTKtoolkit") == 0)
    ctk_image_set_from_icon_name (priv->avatar_image, "ctk3-demo", CTK_ICON_SIZE_DND);
  else
    ctk_image_set_from_pixbuf (priv->avatar_image, avatar_pixbuf_other);

}

static void
ctk_message_row_expand (GtkMessageRow *row)
{
  GtkMessageRowPrivate *priv = row->priv;
  gboolean expand;

  expand = !ctk_revealer_get_reveal_child (priv->details_revealer);

  ctk_revealer_set_reveal_child (priv->details_revealer, expand);
  if (expand)
    ctk_button_set_label (priv->expand_button, "Hide");
  else
    ctk_button_set_label (priv->expand_button, "Expand");
}

static void
expand_clicked (GtkMessageRow *row,
                GtkButton *button)
{
  ctk_message_row_expand (row);
}

static void
reshare_clicked (GtkMessageRow *row,
                 GtkButton *button)
{
  GtkMessageRowPrivate *priv = row->priv;

  priv->message->n_reshares++;
  ctk_message_row_update (row);

}

static void
favorite_clicked (GtkMessageRow *row,
                  GtkButton *button)
{
  GtkMessageRowPrivate *priv = row->priv;

  priv->message->n_favorites++;
  ctk_message_row_update (row);
}

static void
ctk_message_row_state_flags_changed (GtkWidget    *widget,
                                     GtkStateFlags previous_state_flags)
{
  GtkMessageRowPrivate *priv = CTK_MESSAGE_ROW (widget)->priv;
  GtkStateFlags flags;

  flags = ctk_widget_get_state_flags (widget);

  ctk_widget_set_visible (priv->extra_buttons_box,
                          flags & (CTK_STATE_FLAG_PRELIGHT | CTK_STATE_FLAG_SELECTED));

  CTK_WIDGET_CLASS (ctk_message_row_parent_class)->state_flags_changed (widget, previous_state_flags);
}

static void
ctk_message_row_finalize (GObject *obj)
{
  GtkMessageRowPrivate *priv = CTK_MESSAGE_ROW (obj)->priv;
  g_object_unref (priv->message);
  G_OBJECT_CLASS (ctk_message_row_parent_class)->finalize(obj);
}

static void
ctk_message_row_class_init (GtkMessageRowClass *klass)
{
  GtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = ctk_message_row_finalize;

  ctk_widget_class_set_template_from_resource (widget_class, "/listbox/listbox.ui");
  ctk_widget_class_bind_template_child_private (widget_class, GtkMessageRow, content_label);
  ctk_widget_class_bind_template_child_private (widget_class, GtkMessageRow, source_name);
  ctk_widget_class_bind_template_child_private (widget_class, GtkMessageRow, source_nick);
  ctk_widget_class_bind_template_child_private (widget_class, GtkMessageRow, short_time_label);
  ctk_widget_class_bind_template_child_private (widget_class, GtkMessageRow, detailed_time_label);
  ctk_widget_class_bind_template_child_private (widget_class, GtkMessageRow, extra_buttons_box);
  ctk_widget_class_bind_template_child_private (widget_class, GtkMessageRow, details_revealer);
  ctk_widget_class_bind_template_child_private (widget_class, GtkMessageRow, avatar_image);
  ctk_widget_class_bind_template_child_private (widget_class, GtkMessageRow, resent_box);
  ctk_widget_class_bind_template_child_private (widget_class, GtkMessageRow, resent_by_button);
  ctk_widget_class_bind_template_child_private (widget_class, GtkMessageRow, n_reshares_label);
  ctk_widget_class_bind_template_child_private (widget_class, GtkMessageRow, n_favorites_label);
  ctk_widget_class_bind_template_child_private (widget_class, GtkMessageRow, expand_button);
  ctk_widget_class_bind_template_callback (widget_class, expand_clicked);
  ctk_widget_class_bind_template_callback (widget_class, reshare_clicked);
  ctk_widget_class_bind_template_callback (widget_class, favorite_clicked);

  widget_class->state_flags_changed = ctk_message_row_state_flags_changed;
}

static void
ctk_message_row_init (GtkMessageRow *row)
{
  row->priv = ctk_message_row_get_instance_private (row);

  ctk_widget_init_template (CTK_WIDGET (row));
}

static GtkMessageRow *
ctk_message_row_new (GtkMessage *message)
{
  GtkMessageRow *row;

  row = g_object_new (ctk_message_row_get_type (), NULL);
  row->priv->message = message;
  ctk_message_row_update (row);

  return row;
}

static int
ctk_message_row_sort (GtkMessageRow *a, GtkMessageRow *b, gpointer data)
{
  return b->priv->message->time - a->priv->message->time;
}

static void
row_activated (GtkListBox *listbox, GtkListBoxRow *row)
{
  ctk_message_row_expand (CTK_MESSAGE_ROW (row));
}

GtkWidget *
do_listbox (GtkWidget *do_widget)
{
  GtkWidget *scrolled, *listbox, *vbox, *label;
  GtkMessage *message;
  GtkMessageRow *row;
  GBytes *data;
  char **lines;
  int i;

  if (!window)
    {
      avatar_pixbuf_other = gdk_pixbuf_new_from_resource_at_scale ("/listbox/apple-red.png", 32, 32, FALSE, NULL);

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
      label = ctk_label_new ("Messages from Gtk+ and friends");
      ctk_box_pack_start (CTK_BOX (vbox), label, FALSE, FALSE, 0);
      scrolled = ctk_scrolled_window_new (NULL, NULL);
      ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (scrolled), CTK_POLICY_NEVER, CTK_POLICY_AUTOMATIC);
      ctk_box_pack_start (CTK_BOX (vbox), scrolled, TRUE, TRUE, 0);
      listbox = ctk_list_box_new ();
      ctk_container_add (CTK_CONTAINER (scrolled), listbox);

      ctk_list_box_set_sort_func (CTK_LIST_BOX (listbox), (GtkListBoxSortFunc)ctk_message_row_sort, listbox, NULL);
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
