/* GTK - The GIMP Toolkit
 * ctkprinteroption.c: Handling possible settings for a specific printer setting
 * Copyright (C) 2006, Red Hat, Inc.
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
#include <gmodule.h>

#include "ctkintl.h"
#include "ctkprivate.h"
#include "ctkprinteroption.h"

/*****************************************
 *            CtkPrinterOption           *
 *****************************************/

enum {
  CHANGED,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_VALUE
};

static guint signals[LAST_SIGNAL] = { 0 };

static void ctk_printer_option_set_property (GObject      *object,
                                             guint         prop_id,
                                             const GValue *value,
                                             GParamSpec   *pspec);
static void ctk_printer_option_get_property (GObject      *object,
                                             guint         prop_id,
                                             GValue       *value,
                                             GParamSpec   *pspec);

G_DEFINE_TYPE (CtkPrinterOption, ctk_printer_option, G_TYPE_OBJECT)

static void
ctk_printer_option_finalize (GObject *object)
{
  CtkPrinterOption *option = CTK_PRINTER_OPTION (object);
  int i;
  
  g_free (option->name);
  g_free (option->display_text);
  g_free (option->value);
  for (i = 0; i < option->num_choices; i++)
    {
      g_free (option->choices[i]);
      g_free (option->choices_display[i]);
    }
  g_free (option->choices);
  g_free (option->choices_display);
  g_free (option->group);
  
  G_OBJECT_CLASS (ctk_printer_option_parent_class)->finalize (object);
}

static void
ctk_printer_option_init (CtkPrinterOption *option)
{
  option->value = g_strdup ("");
  option->activates_default = FALSE;
}

static void
ctk_printer_option_class_init (CtkPrinterOptionClass *class)
{
  GObjectClass *gobject_class = (GObjectClass *)class;

  gobject_class->finalize = ctk_printer_option_finalize;
  gobject_class->set_property = ctk_printer_option_set_property;
  gobject_class->get_property = ctk_printer_option_get_property;

  signals[CHANGED] =
    g_signal_new (I_("changed"),
		  G_TYPE_FROM_CLASS (class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkPrinterOptionClass, changed),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);

  g_object_class_install_property (G_OBJECT_CLASS (class),
                                   PROP_VALUE,
                                   g_param_spec_string ("value",
                                                        P_("Option Value"),
                                                        P_("Value of the option"),
                                                        "",
                                                        CTK_PARAM_READWRITE));
}

CtkPrinterOption *
ctk_printer_option_new (const char *name, const char *display_text,
			CtkPrinterOptionType type)
{
  CtkPrinterOption *option;

  option = g_object_new (CTK_TYPE_PRINTER_OPTION, NULL);

  option->name = g_strdup (name);
  option->display_text = g_strdup (display_text);
  option->type = type;
  
  return option;
}

static void
ctk_printer_option_set_property (GObject         *object,
                                 guint            prop_id,
                                 const GValue    *value,
                                 GParamSpec      *pspec)
{
  CtkPrinterOption *option = CTK_PRINTER_OPTION (object);

  switch (prop_id)
    {
    case PROP_VALUE:
      ctk_printer_option_set (option, g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_printer_option_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  CtkPrinterOption *option = CTK_PRINTER_OPTION (object);

  switch (prop_id)
    {
    case PROP_VALUE:
      g_value_set_string (value, option->value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
emit_changed (CtkPrinterOption *option)
{
  g_signal_emit (option, signals[CHANGED], 0);
}

void
ctk_printer_option_set (CtkPrinterOption *option,
			const char *value)
{
  if (value == NULL)
    value = "";
  
  if (strcmp (option->value, value) == 0)
    return;

  if ((option->type == CTK_PRINTER_OPTION_TYPE_PICKONE ||
       option->type == CTK_PRINTER_OPTION_TYPE_ALTERNATIVE))
    {
      int i;
      
      for (i = 0; i < option->num_choices; i++)
	{
	  if (g_ascii_strcasecmp (value, option->choices[i]) == 0)
	    {
	      value = option->choices[i];
	      break;
	    }
	}

      if (i == option->num_choices)
	return; /* Not found in available choices */
    }
          
  g_free (option->value);
  option->value = g_strdup (value);
  
  emit_changed (option);
}

void
ctk_printer_option_set_boolean (CtkPrinterOption *option,
				gboolean value)
{
  ctk_printer_option_set (option, value ? "True" : "False");
}

void
ctk_printer_option_set_has_conflict  (CtkPrinterOption *option,
				      gboolean  has_conflict)
{
  has_conflict = has_conflict != 0;
  
  if (option->has_conflict == has_conflict)
    return;

  option->has_conflict = has_conflict;
  emit_changed (option);
}

void
ctk_printer_option_clear_has_conflict (CtkPrinterOption     *option)
{
  ctk_printer_option_set_has_conflict  (option, FALSE);
}

void
ctk_printer_option_allocate_choices (CtkPrinterOption     *option,
				     int num)
{
  g_free (option->choices);
  g_free (option->choices_display);

  option->num_choices = num;
  if (num == 0)
    {
      option->choices = NULL;
      option->choices_display = NULL;
    }
  else
    {
      option->choices = g_new0 (char *, num);
      option->choices_display = g_new0 (char *, num);
    }
}

void
ctk_printer_option_choices_from_array (CtkPrinterOption   *option,
				       int                 num_choices,
				       char               *choices[],
				       char              *choices_display[])
{
  int i;
  
  ctk_printer_option_allocate_choices (option, num_choices);
  for (i = 0; i < num_choices; i++)
    {
      option->choices[i] = g_strdup (choices[i]);
      option->choices_display[i] = g_strdup (choices_display[i]);
    }
}

gboolean
ctk_printer_option_has_choice (CtkPrinterOption     *option,
			       const char           *choice)
{
  int i;
  
  for (i = 0; i < option->num_choices; i++)
    {
      if (strcmp (option->choices[i], choice) == 0)
	return TRUE;
    }
  
  return FALSE;
}

void
ctk_printer_option_set_activates_default (CtkPrinterOption *option,
					  gboolean          activates)
{
  g_return_if_fail (CTK_IS_PRINTER_OPTION (option));

  option->activates_default = activates;
}

gboolean
ctk_printer_option_get_activates_default (CtkPrinterOption *option)
{
  g_return_val_if_fail (CTK_IS_PRINTER_OPTION (option), FALSE);

  return option->activates_default;
}
