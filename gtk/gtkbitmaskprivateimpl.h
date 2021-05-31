/*
 * Copyright © 2011 Red Hat Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Authors: Benjamin Otte <otte@gnome.org>
 */


static inline GtkBitmask *
_ctk_bitmask_new (void)
{
  return _ctk_bitmask_from_bits (0);
}

static inline GtkBitmask *
_ctk_bitmask_copy (const GtkBitmask *mask)
{
  if (_ctk_bitmask_is_allocated (mask))
    return _ctk_allocated_bitmask_copy (mask);
  else
    return (GtkBitmask *) mask;
}

static inline void
_ctk_bitmask_free (GtkBitmask *mask)
{
  if (_ctk_bitmask_is_allocated (mask))
    _ctk_allocated_bitmask_free (mask);
}

static inline char *
_ctk_bitmask_to_string (const GtkBitmask *mask)
{
  GString *string;
  
  string = g_string_new (NULL);
  _ctk_allocated_bitmask_print (mask, string);
  return g_string_free (string, FALSE);
}

static inline void
_ctk_bitmask_print (const GtkBitmask *mask,
                    GString          *string)
{
  _ctk_allocated_bitmask_print (mask, string);
}

static inline GtkBitmask *
_ctk_bitmask_intersect (GtkBitmask       *mask,
                        const GtkBitmask *other)
{
  return _ctk_allocated_bitmask_intersect (mask, other);
}

static inline GtkBitmask *
_ctk_bitmask_union (GtkBitmask       *mask,
                    const GtkBitmask *other)
{
  if (_ctk_bitmask_is_allocated (mask) ||
      _ctk_bitmask_is_allocated (other))
    return _ctk_allocated_bitmask_union (mask, other);
  else
    return _ctk_bitmask_from_bits (_ctk_bitmask_to_bits (mask)
                                   | _ctk_bitmask_to_bits (other));
}

static inline GtkBitmask *
_ctk_bitmask_subtract (GtkBitmask       *mask,
                       const GtkBitmask *other)
{
  return _ctk_allocated_bitmask_subtract (mask, other);
}

static inline gboolean
_ctk_bitmask_get (const GtkBitmask *mask,
                  guint             index_)
{
  if (_ctk_bitmask_is_allocated (mask))
    return _ctk_allocated_bitmask_get (mask, index_);
  else
    return index_ < CTK_BITMASK_N_DIRECT_BITS
           ? !!(_ctk_bitmask_to_bits (mask) & (((gsize) 1) << index_))
           : FALSE;
}

static inline GtkBitmask *
_ctk_bitmask_set (GtkBitmask *mask,
                  guint       index_,
                  gboolean    value)
{
  if (_ctk_bitmask_is_allocated (mask) ||
      (index_ >= CTK_BITMASK_N_DIRECT_BITS && value))
    return _ctk_allocated_bitmask_set (mask, index_, value);
  else if (index_ < CTK_BITMASK_N_DIRECT_BITS)
    {
      gsize bits = _ctk_bitmask_to_bits (mask);

      if (value)
        bits |= ((gsize) 1) << index_;
      else
        bits &= ~(((gsize) 1) << index_);

      return _ctk_bitmask_from_bits (bits);
    }
  else
    return mask;
}

static inline GtkBitmask *
_ctk_bitmask_invert_range (GtkBitmask *mask,
                           guint       start,
                           guint       end)
{
  if (_ctk_bitmask_is_allocated (mask) ||
      (end > CTK_BITMASK_N_DIRECT_BITS))
    return _ctk_allocated_bitmask_invert_range (mask, start, end);
  else
    {
      gsize invert = (((gsize) 1) << end) - (((gsize) 1) << start);
      
      return _ctk_bitmask_from_bits (_ctk_bitmask_to_bits (mask) ^ invert);
    }
}

static inline gboolean
_ctk_bitmask_is_empty (const GtkBitmask *mask)
{
  return mask == _ctk_bitmask_from_bits (0);
}

static inline gboolean
_ctk_bitmask_equals (const GtkBitmask *mask,
                     const GtkBitmask *other)
{
  if (mask == other)
    return TRUE;

  if (!_ctk_bitmask_is_allocated (mask) ||
      !_ctk_bitmask_is_allocated (other))
    return FALSE;

  return _ctk_allocated_bitmask_equals (mask, other);
}

static inline gboolean
_ctk_bitmask_intersects (const GtkBitmask *mask,
                         const GtkBitmask *other)
{
  if (_ctk_bitmask_is_allocated (mask) ||
      _ctk_bitmask_is_allocated (other))
    return _ctk_allocated_bitmask_intersects (mask, other);
  else
    return _ctk_bitmask_to_bits (mask) & _ctk_bitmask_to_bits (other) ? TRUE : FALSE;
}
