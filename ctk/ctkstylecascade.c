/* GTK - The GIMP Toolkit
 * Copyright (C) 2012 Benjamin Otte <otte@gnome.org>
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

#include "gtkstylecascadeprivate.h"

#include "gtkstyleprovider.h"
#include "gtkstyleproviderprivate.h"
#include "gtkprivate.h"

typedef struct _GtkStyleCascadeIter GtkStyleCascadeIter;
typedef struct _GtkStyleProviderData GtkStyleProviderData;

struct _GtkStyleCascadeIter {
  int  n_cascades;
  int *cascade_index;  /* each one points at last index that was returned, */
                       /* not next one that should be returned */
};

struct _GtkStyleProviderData
{
  GtkStyleProvider *provider;
  guint priority;
  guint changed_signal_id;
};

static GtkStyleProvider *
ctk_style_cascade_iter_next (GtkStyleCascade     *cascade,
                             GtkStyleCascadeIter *iter)
{
  GtkStyleCascade *cas;
  int ix, highest_priority_index = 0;
  GtkStyleProviderData *highest_priority_data = NULL;

  for (cas = cascade, ix = 0; ix < iter->n_cascades; cas = cas->parent, ix++)
    {
      GtkStyleProviderData *data;

      if (iter->cascade_index[ix] <= 0)
        continue;

      data = &g_array_index (cas->providers,
                             GtkStyleProviderData,
                             iter->cascade_index[ix] - 1);
      if (highest_priority_data == NULL || data->priority > highest_priority_data->priority)
        {
          highest_priority_index = ix;
          highest_priority_data = data;
        }
    }

  if (highest_priority_data != NULL)
    {
      iter->cascade_index[highest_priority_index]--;
      return highest_priority_data->provider;
    }
  return NULL;
}

static GtkStyleProvider *
ctk_style_cascade_iter_init (GtkStyleCascade     *cascade,
                             GtkStyleCascadeIter *iter)
{
  GtkStyleCascade *cas = cascade;
  int ix;

  iter->n_cascades = 1;
  while ((cas = cas->parent) != NULL)
    iter->n_cascades++;

  iter->cascade_index = g_new (int, iter->n_cascades);
  for (cas = cascade, ix = 0; ix < iter->n_cascades; cas = cas->parent, ix++)
    iter->cascade_index[ix] = cas->providers->len;

  return ctk_style_cascade_iter_next (cascade, iter);
}

static void
ctk_style_cascade_iter_clear (GtkStyleCascadeIter *iter)
{
  g_free (iter->cascade_index);
}

static gboolean
ctk_style_cascade_get_style_property (GtkStyleProvider *provider,
                                      GtkWidgetPath    *path,
                                      GtkStateFlags     state,
                                      GParamSpec       *pspec,
                                      GValue           *value)
{
  GtkStyleCascade *cascade = CTK_STYLE_CASCADE (provider);
  GtkStyleCascadeIter iter;
  GtkStyleProvider *item;

  for (item = ctk_style_cascade_iter_init (cascade, &iter);
       item;
       item = ctk_style_cascade_iter_next (cascade, &iter))
    {
      if (ctk_style_provider_get_style_property (item,
                                                 path,
                                                 state,
                                                 pspec,
                                                 value))
        {
          ctk_style_cascade_iter_clear (&iter);
          return TRUE;
        }
    }

  ctk_style_cascade_iter_clear (&iter);
  return FALSE;
}

static void
ctk_style_cascade_provider_iface_init (GtkStyleProviderIface *iface)
{
  iface->get_style_property = ctk_style_cascade_get_style_property;
}

static GtkSettings *
ctk_style_cascade_get_settings (GtkStyleProviderPrivate *provider)
{
  GtkStyleCascade *cascade = CTK_STYLE_CASCADE (provider);
  GtkStyleCascadeIter iter;
  GtkSettings *settings;
  GtkStyleProvider *item;

  for (item = ctk_style_cascade_iter_init (cascade, &iter);
       item;
       item = ctk_style_cascade_iter_next (cascade, &iter))
    {
      if (!CTK_IS_STYLE_PROVIDER_PRIVATE (item))
        continue;
          
      settings = _ctk_style_provider_private_get_settings (CTK_STYLE_PROVIDER_PRIVATE (item));
      if (settings)
        {
          ctk_style_cascade_iter_clear (&iter);
          return settings;
        }
    }

  ctk_style_cascade_iter_clear (&iter);
  return NULL;
}

static GtkCssValue *
ctk_style_cascade_get_color (GtkStyleProviderPrivate *provider,
                             const char              *name)
{
  GtkStyleCascade *cascade = CTK_STYLE_CASCADE (provider);
  GtkStyleCascadeIter iter;
  GtkCssValue *color;
  GtkStyleProvider *item;

  for (item = ctk_style_cascade_iter_init (cascade, &iter);
       item;
       item = ctk_style_cascade_iter_next (cascade, &iter))
    {
      if (CTK_IS_STYLE_PROVIDER_PRIVATE (item))
        {
          color = _ctk_style_provider_private_get_color (CTK_STYLE_PROVIDER_PRIVATE (item), name);
          if (color)
            {
              ctk_style_cascade_iter_clear (&iter);
              return color;
            }
        }
      else
        {
          /* If somebody hits this code path, shout at them */
        }
    }

  ctk_style_cascade_iter_clear (&iter);
  return NULL;
}

static int
ctk_style_cascade_get_scale (GtkStyleProviderPrivate *provider)
{
  GtkStyleCascade *cascade = CTK_STYLE_CASCADE (provider);

  return cascade->scale;
}

static GtkCssKeyframes *
ctk_style_cascade_get_keyframes (GtkStyleProviderPrivate *provider,
                                 const char              *name)
{
  GtkStyleCascade *cascade = CTK_STYLE_CASCADE (provider);
  GtkStyleCascadeIter iter;
  GtkCssKeyframes *keyframes;
  GtkStyleProvider *item;

  for (item = ctk_style_cascade_iter_init (cascade, &iter);
       item;
       item = ctk_style_cascade_iter_next (cascade, &iter))
    {
      if (!CTK_IS_STYLE_PROVIDER_PRIVATE (item))
        continue;
          
      keyframes = _ctk_style_provider_private_get_keyframes (CTK_STYLE_PROVIDER_PRIVATE (item), name);
      if (keyframes)
        {
          ctk_style_cascade_iter_clear (&iter);
          return keyframes;
        }
    }

  ctk_style_cascade_iter_clear (&iter);
  return NULL;
}

static void
ctk_style_cascade_lookup (GtkStyleProviderPrivate *provider,
                          const GtkCssMatcher     *matcher,
                          GtkCssLookup            *lookup,
                          GtkCssChange            *change)
{
  GtkStyleCascade *cascade = CTK_STYLE_CASCADE (provider);
  GtkStyleCascadeIter iter;
  GtkStyleProvider *item;
  GtkCssChange iter_change;

  for (item = ctk_style_cascade_iter_init (cascade, &iter);
       item;
       item = ctk_style_cascade_iter_next (cascade, &iter))
    {
      GtkStyleProviderPrivate *sp = (GtkStyleProviderPrivate*)item;
      if (CTK_IS_STYLE_PROVIDER_PRIVATE (sp))
        {
          _ctk_style_provider_private_lookup (sp, matcher, lookup,
                                              change ? &iter_change : NULL);
          if (change)
            *change |= iter_change;
        }
      else
        {
          /* you lose */
          g_warn_if_reached ();
        }
    }
  ctk_style_cascade_iter_clear (&iter);
}

static void
ctk_style_cascade_provider_private_iface_init (GtkStyleProviderPrivateInterface *iface)
{
  iface->get_color = ctk_style_cascade_get_color;
  iface->get_settings = ctk_style_cascade_get_settings;
  iface->get_scale = ctk_style_cascade_get_scale;
  iface->get_keyframes = ctk_style_cascade_get_keyframes;
  iface->lookup = ctk_style_cascade_lookup;
}

G_DEFINE_TYPE_EXTENDED (GtkStyleCascade, _ctk_style_cascade, G_TYPE_OBJECT, 0,
                        G_IMPLEMENT_INTERFACE (CTK_TYPE_STYLE_PROVIDER,
                                               ctk_style_cascade_provider_iface_init)
                        G_IMPLEMENT_INTERFACE (CTK_TYPE_STYLE_PROVIDER_PRIVATE,
                                               ctk_style_cascade_provider_private_iface_init));

static void
ctk_style_cascade_dispose (GObject *object)
{
  GtkStyleCascade *cascade = CTK_STYLE_CASCADE (object);

  _ctk_style_cascade_set_parent (cascade, NULL);
  g_array_unref (cascade->providers);

  G_OBJECT_CLASS (_ctk_style_cascade_parent_class)->dispose (object);
}

static void
_ctk_style_cascade_class_init (GtkStyleCascadeClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = ctk_style_cascade_dispose;
}

static void
style_provider_data_clear (gpointer data_)
{
  GtkStyleProviderData *data = data_;

  g_signal_handler_disconnect (data->provider, data->changed_signal_id);
  g_object_unref (data->provider);
}

static void
_ctk_style_cascade_init (GtkStyleCascade *cascade)
{
  cascade->scale = 1;

  cascade->providers = g_array_new (FALSE, FALSE, sizeof (GtkStyleProviderData));
  g_array_set_clear_func (cascade->providers, style_provider_data_clear);
}

GtkStyleCascade *
_ctk_style_cascade_new (void)
{
  return g_object_new (CTK_TYPE_STYLE_CASCADE, NULL);
}

void
_ctk_style_cascade_set_parent (GtkStyleCascade *cascade,
                               GtkStyleCascade *parent)
{
  ctk_internal_return_if_fail (CTK_IS_STYLE_CASCADE (cascade));
  ctk_internal_return_if_fail (parent == NULL || CTK_IS_STYLE_CASCADE (parent));

  if (cascade->parent == parent)
    return;

  if (parent)
    {
      g_object_ref (parent);
      g_signal_connect_swapped (parent,
                                "-gtk-private-changed",
                                G_CALLBACK (_ctk_style_provider_private_changed),
                                cascade);
    }

  if (cascade->parent)
    {
      g_signal_handlers_disconnect_by_func (cascade->parent, 
                                            _ctk_style_provider_private_changed,
                                            cascade);
      g_object_unref (cascade->parent);
    }

  cascade->parent = parent;
}

void
_ctk_style_cascade_add_provider (GtkStyleCascade  *cascade,
                                 GtkStyleProvider *provider,
                                 guint             priority)
{
  GtkStyleProviderData data;
  guint i;

  ctk_internal_return_if_fail (CTK_IS_STYLE_CASCADE (cascade));
  ctk_internal_return_if_fail (CTK_IS_STYLE_PROVIDER (provider));
  ctk_internal_return_if_fail (CTK_STYLE_PROVIDER (cascade) != provider);

  data.provider = g_object_ref (provider);
  data.priority = priority;
  data.changed_signal_id = g_signal_connect_swapped (provider,
                                                     "-gtk-private-changed",
                                                     G_CALLBACK (_ctk_style_provider_private_changed),
                                                     cascade);

  /* ensure it gets removed first */
  _ctk_style_cascade_remove_provider (cascade, provider);

  for (i = 0; i < cascade->providers->len; i++)
    {
      if (g_array_index (cascade->providers, GtkStyleProviderData, i).priority > priority)
        break;
    }
  g_array_insert_val (cascade->providers, i, data);

  _ctk_style_provider_private_changed (CTK_STYLE_PROVIDER_PRIVATE (cascade));
}

void
_ctk_style_cascade_remove_provider (GtkStyleCascade  *cascade,
                                    GtkStyleProvider *provider)
{
  guint i;

  ctk_internal_return_if_fail (CTK_IS_STYLE_CASCADE (cascade));
  ctk_internal_return_if_fail (CTK_IS_STYLE_PROVIDER (provider));

  for (i = 0; i < cascade->providers->len; i++)
    {
      GtkStyleProviderData *data = &g_array_index (cascade->providers, GtkStyleProviderData, i);

      if (data->provider == provider)
        {
          g_array_remove_index (cascade->providers, i);
  
          _ctk_style_provider_private_changed (CTK_STYLE_PROVIDER_PRIVATE (cascade));
          break;
        }
    }
}

void
_ctk_style_cascade_set_scale (GtkStyleCascade *cascade,
                              int              scale)
{
  ctk_internal_return_if_fail (CTK_IS_STYLE_CASCADE (cascade));

  if (cascade->scale == scale)
    return;

  cascade->scale = scale;

  _ctk_style_provider_private_changed (CTK_STYLE_PROVIDER_PRIVATE (cascade));
}

int
_ctk_style_cascade_get_scale (GtkStyleCascade *cascade)
{
  ctk_internal_return_val_if_fail (CTK_IS_STYLE_CASCADE (cascade), 1);

  return cascade->scale;
}