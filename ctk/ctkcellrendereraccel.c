/* ctkcellrendereraccel.h
 * Copyright (C) 2000  Red Hat, Inc.,  Jonathan Blandford <jrb@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "ctkcellrendereraccel.h"

#include "ctkintl.h"
#include "ctkaccelgroup.h"
#include "ctkmarshalers.h"
#include "ctklabel.h"
#include "ctkeventbox.h"
#include "ctkmain.h"
#include "ctksizerequest.h"
#include "ctktypebuiltins.h"
#include "ctkprivate.h"


/**
 * SECTION:ctkcellrendereraccel
 * @Short_description: Renders a keyboard accelerator in a cell
 * @Title: CtkCellRendererAccel
 *
 * #CtkCellRendererAccel displays a keyboard accelerator (i.e. a key
 * combination like `Control + a`). If the cell renderer is editable,
 * the accelerator can be changed by simply typing the new combination.
 *
 * The #CtkCellRendererAccel cell renderer was added in CTK+ 2.10.
 */


static void ctk_cell_renderer_accel_get_property (GObject         *object,
                                                  guint            param_id,
                                                  GValue          *value,
                                                  GParamSpec      *pspec);
static void ctk_cell_renderer_accel_set_property (GObject         *object,
                                                  guint            param_id,
                                                  const GValue    *value,
                                                  GParamSpec      *pspec);
static void ctk_cell_renderer_accel_get_preferred_width 
                                                 (CtkCellRenderer *cell,
                                                  CtkWidget       *widget,
                                                  gint            *minimum_size,
                                                  gint            *natural_size);
static CtkCellEditable *
           ctk_cell_renderer_accel_start_editing (CtkCellRenderer      *cell,
                                                  CdkEvent             *event,
                                                  CtkWidget            *widget,
                                                  const gchar          *path,
                                                  const CdkRectangle   *background_area,
                                                  const CdkRectangle   *cell_area,
                                                  CtkCellRendererState  flags);
static gchar *convert_keysym_state_to_string     (CtkCellRendererAccel *accel,
                                                  guint                 keysym,
                                                  CdkModifierType       mask,
                                                  guint                 keycode);
static CtkWidget *ctk_cell_editable_event_box_new (CtkCellRenderer          *cell,
                                                   CtkCellRendererAccelMode  mode,
                                                   const gchar              *path);

enum {
  ACCEL_EDITED,
  ACCEL_CLEARED,
  LAST_SIGNAL
};

enum {
  PROP_ACCEL_KEY = 1,
  PROP_ACCEL_MODS,
  PROP_KEYCODE,
  PROP_ACCEL_MODE
};

static guint signals[LAST_SIGNAL] = { 0 };

struct _CtkCellRendererAccelPrivate
{
  CtkWidget *sizing_label;

  CtkCellRendererAccelMode accel_mode;
  CdkModifierType accel_mods;
  guint accel_key;
  guint keycode;

  CdkDevice *grab_pointer;
};

G_DEFINE_TYPE_WITH_PRIVATE (CtkCellRendererAccel, ctk_cell_renderer_accel, CTK_TYPE_CELL_RENDERER_TEXT)

static void
ctk_cell_renderer_accel_init (CtkCellRendererAccel *cell_accel)
{
  gchar *text;

  cell_accel->priv = ctk_cell_renderer_accel_get_instance_private (cell_accel);

  text = convert_keysym_state_to_string (cell_accel, 0, 0, 0);
  g_object_set (cell_accel, "text", text, NULL);
  g_free (text);
}

static void
ctk_cell_renderer_accel_class_init (CtkCellRendererAccelClass *cell_accel_class)
{
  GObjectClass *object_class;
  CtkCellRendererClass *cell_renderer_class;

  object_class = G_OBJECT_CLASS (cell_accel_class);
  cell_renderer_class = CTK_CELL_RENDERER_CLASS (cell_accel_class);

  object_class->set_property = ctk_cell_renderer_accel_set_property;
  object_class->get_property = ctk_cell_renderer_accel_get_property;

  cell_renderer_class->get_preferred_width = ctk_cell_renderer_accel_get_preferred_width;
  cell_renderer_class->start_editing = ctk_cell_renderer_accel_start_editing;

  /**
   * CtkCellRendererAccel:accel-key:
   *
   * The keyval of the accelerator.
   *
   * Since: 2.10
   */
  g_object_class_install_property (object_class,
                                   PROP_ACCEL_KEY,
                                   g_param_spec_uint ("accel-key",
                                                     P_("Accelerator key"),
                                                     P_("The keyval of the accelerator"),
                                                      0,
                                                      G_MAXINT,
                                                      0,
                                                      CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));
  
  /**
   * CtkCellRendererAccel:accel-mods:
   *
   * The modifier mask of the accelerator.
   *
   * Since: 2.10
   */
  g_object_class_install_property (object_class,
                                   PROP_ACCEL_MODS,
                                   g_param_spec_flags ("accel-mods",
                                                       P_("Accelerator modifiers"),
                                                       P_("The modifier mask of the accelerator"),
                                                       CDK_TYPE_MODIFIER_TYPE,
                                                       0,
                                                       CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * CtkCellRendererAccel:keycode:
   *
   * The hardware keycode of the accelerator. Note that the hardware keycode is
   * only relevant if the key does not have a keyval. Normally, the keyboard
   * configuration should assign keyvals to all keys.
   *
   * Since: 2.10
   */ 
  g_object_class_install_property (object_class,
                                   PROP_KEYCODE,
                                   g_param_spec_uint ("keycode",
                                                      P_("Accelerator keycode"),
                                                      P_("The hardware keycode of the accelerator"),
                                                      0,
                                                      G_MAXINT,
                                                      0,
                                                      CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * CtkCellRendererAccel:accel-mode:
   *
   * Determines if the edited accelerators are CTK+ accelerators. If
   * they are, consumed modifiers are suppressed, only accelerators
   * accepted by CTK+ are allowed, and the accelerators are rendered
   * in the same way as they are in menus.
   *
   * Since: 2.10
   */
  g_object_class_install_property (object_class,
                                   PROP_ACCEL_MODE,
                                   g_param_spec_enum ("accel-mode",
                                                      P_("Accelerator Mode"),
                                                      P_("The type of accelerators"),
                                                      CTK_TYPE_CELL_RENDERER_ACCEL_MODE,
                                                      CTK_CELL_RENDERER_ACCEL_MODE_CTK,
                                                      CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));
  
  /**
   * CtkCellRendererAccel::accel-edited:
   * @accel: the object reveiving the signal
   * @path_string: the path identifying the row of the edited cell
   * @accel_key: the new accelerator keyval
   * @accel_mods: the new acclerator modifier mask
   * @hardware_keycode: the keycode of the new accelerator
   *
   * Gets emitted when the user has selected a new accelerator.
   *
   * Since: 2.10
   */
  signals[ACCEL_EDITED] = g_signal_new (I_("accel-edited"),
                                        CTK_TYPE_CELL_RENDERER_ACCEL,
                                        G_SIGNAL_RUN_LAST,
                                        G_STRUCT_OFFSET (CtkCellRendererAccelClass, accel_edited),
                                        NULL, NULL,
                                        _ctk_marshal_VOID__STRING_UINT_FLAGS_UINT,
                                        G_TYPE_NONE, 4,
                                        G_TYPE_STRING,
                                        G_TYPE_UINT,
                                        CDK_TYPE_MODIFIER_TYPE,
                                        G_TYPE_UINT);

  /**
   * CtkCellRendererAccel::accel-cleared:
   * @accel: the object reveiving the signal
   * @path_string: the path identifying the row of the edited cell
   *
   * Gets emitted when the user has removed the accelerator.
   *
   * Since: 2.10
   */
  signals[ACCEL_CLEARED] = g_signal_new (I_("accel-cleared"),
                                         CTK_TYPE_CELL_RENDERER_ACCEL,
                                         G_SIGNAL_RUN_LAST,
                                         G_STRUCT_OFFSET (CtkCellRendererAccelClass, accel_cleared),
                                         NULL, NULL,
                                         NULL,
                                         G_TYPE_NONE, 1,
                                         G_TYPE_STRING);
}


/**
 * ctk_cell_renderer_accel_new:
 *
 * Creates a new #CtkCellRendererAccel.
 * 
 * Returns: the new cell renderer
 *
 * Since: 2.10
 */
CtkCellRenderer *
ctk_cell_renderer_accel_new (void)
{
  return g_object_new (CTK_TYPE_CELL_RENDERER_ACCEL, NULL);
}

static gchar *
convert_keysym_state_to_string (CtkCellRendererAccel *accel,
                                guint                 keysym,
                                CdkModifierType       mask,
                                guint                 keycode)
{
  CtkCellRendererAccelPrivate *priv = accel->priv;

  if (keysym == 0 && keycode == 0)
    /* This label is displayed in a treeview cell displaying
     * a disabled accelerator key combination.
     */
    return g_strdup (C_("Accelerator", "Disabled"));
  else 
    {
      if (priv->accel_mode == CTK_CELL_RENDERER_ACCEL_MODE_CTK)
        {
          if (!ctk_accelerator_valid (keysym, mask))
            /* This label is displayed in a treeview cell displaying
             * an accelerator key combination that is not valid according
             * to ctk_accelerator_valid().
             */
            return g_strdup (C_("Accelerator", "Invalid"));

          return ctk_accelerator_get_label (keysym, mask);
        }
      else 
        {
          gchar *name;

          name = ctk_accelerator_get_label_with_keycode (NULL, keysym, keycode, mask);
          if (name == NULL)
            name = ctk_accelerator_name_with_keycode (NULL, keysym, keycode, mask);

          return name;
        }
    }
}

static void
ctk_cell_renderer_accel_get_property (GObject    *object,
                                      guint       param_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  CtkCellRendererAccelPrivate *priv = CTK_CELL_RENDERER_ACCEL (object)->priv;

  switch (param_id)
    {
    case PROP_ACCEL_KEY:
      g_value_set_uint (value, priv->accel_key);
      break;

    case PROP_ACCEL_MODS:
      g_value_set_flags (value, priv->accel_mods);
      break;

    case PROP_KEYCODE:
      g_value_set_uint (value, priv->keycode);
      break;

    case PROP_ACCEL_MODE:
      g_value_set_enum (value, priv->accel_mode);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
    }
}

static void
ctk_cell_renderer_accel_set_property (GObject      *object,
                                      guint         param_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  CtkCellRendererAccel *accel = CTK_CELL_RENDERER_ACCEL (object);
  CtkCellRendererAccelPrivate *priv = accel->priv;
  gboolean changed = FALSE;

  switch (param_id)
    {
    case PROP_ACCEL_KEY:
      {
        guint accel_key = g_value_get_uint (value);

        if (priv->accel_key != accel_key)
          {
            priv->accel_key = accel_key;
            changed = TRUE;
            g_object_notify (object, "accel-key");
          }
      }
      break;

    case PROP_ACCEL_MODS:
      {
        guint accel_mods = g_value_get_flags (value);

        if (priv->accel_mods != accel_mods)
          {
            priv->accel_mods = accel_mods;
            changed = TRUE;
            g_object_notify (object, "accel-mods");
          }
      }
      break;
    case PROP_KEYCODE:
      {
        guint keycode = g_value_get_uint (value);

        if (priv->keycode != keycode)
          {
            priv->keycode = keycode;
            changed = TRUE;
            g_object_notify (object, "keycode");
          }
      }
      break;

    case PROP_ACCEL_MODE:
      if (priv->accel_mode != g_value_get_enum (value))
        {
          priv->accel_mode = g_value_get_enum (value);
          g_object_notify (object, "accel-mode");
        }
      break;
      
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
    }

  if (changed)
    {
      gchar *text;

      text = convert_keysym_state_to_string (accel, priv->accel_key, priv->accel_mods, priv->keycode);
      g_object_set (accel, "text", text, NULL);
      g_free (text);
    }
}

static void
ctk_cell_renderer_accel_get_preferred_width (CtkCellRenderer *cell,
                                             CtkWidget       *widget,
                                             gint            *minimum_size,
                                             gint            *natural_size)

{
  CtkCellRendererAccelPrivate *priv = CTK_CELL_RENDERER_ACCEL (cell)->priv;
  CtkRequisition min_req, nat_req;

  if (priv->sizing_label == NULL)
    priv->sizing_label = ctk_label_new (_("New accelerator…"));

  ctk_widget_get_preferred_size (priv->sizing_label, &min_req, &nat_req);

  CTK_CELL_RENDERER_CLASS (ctk_cell_renderer_accel_parent_class)->get_preferred_width (cell, widget,
                                                                                       minimum_size, natural_size);

  /* FIXME: need to take the cell_area et al. into account */
  if (minimum_size)
    *minimum_size = MAX (*minimum_size, min_req.width);
  if (natural_size)
    *natural_size = MAX (*natural_size, nat_req.width);
}

static CtkCellEditable *
ctk_cell_renderer_accel_start_editing (CtkCellRenderer      *cell,
                                       CdkEvent             *event,
                                       CtkWidget            *widget,
                                       const gchar          *path,
                                       const CdkRectangle   *background_area G_GNUC_UNUSED,
                                       const CdkRectangle   *cell_area G_GNUC_UNUSED,
                                       CtkCellRendererState  flags G_GNUC_UNUSED)
{
  CtkCellRendererAccelPrivate *priv;
  CtkCellRendererText *celltext;
  CtkCellRendererAccel *accel;
  CtkWidget *label;
  CtkWidget *eventbox;
  gboolean editable;
  CdkDevice *device, *pointer;
  CdkWindow *window;

  celltext = CTK_CELL_RENDERER_TEXT (cell);
  accel = CTK_CELL_RENDERER_ACCEL (cell);
  priv = accel->priv;

  /* If the cell isn't editable we return NULL. */
  g_object_get (celltext, "editable", &editable, NULL);
  if (!editable)
    return NULL;

  window = ctk_widget_get_window (ctk_widget_get_toplevel (widget));

  if (event)
    device = cdk_event_get_device (event);
  else
    device = ctk_get_current_event_device ();

  if (!device || !window)
    return NULL;

  if (cdk_device_get_source (device) == CDK_SOURCE_KEYBOARD)
    pointer = cdk_device_get_associated_device (device);
  else
    pointer = device;

  if (cdk_seat_grab (cdk_device_get_seat (pointer), window,
                     CDK_SEAT_CAPABILITY_ALL, FALSE,
                     NULL, event, NULL, NULL) != CDK_GRAB_SUCCESS)
    return NULL;

  priv->grab_pointer = pointer;

  eventbox = ctk_cell_editable_event_box_new (cell, priv->accel_mode, path);

  label = ctk_label_new (NULL);
  ctk_widget_set_halign (label, CTK_ALIGN_START);
  ctk_widget_set_valign (label, CTK_ALIGN_CENTER);

  ctk_widget_set_state_flags (label, CTK_STATE_FLAG_SELECTED, TRUE);

  /* This label is displayed in a treeview cell displaying an accelerator
   * when the cell is clicked to change the acelerator.
   */
  ctk_label_set_text (CTK_LABEL (label), _("New accelerator…"));

  ctk_container_add (CTK_CONTAINER (eventbox), label);

  ctk_widget_show_all (eventbox);
  ctk_grab_add (eventbox);

  return CTK_CELL_EDITABLE (eventbox);
}

static void
ctk_cell_renderer_accel_ungrab (CtkCellRendererAccel *accel)
{
  CtkCellRendererAccelPrivate *priv = accel->priv;

  if (priv->grab_pointer)
    {
      cdk_seat_ungrab (cdk_device_get_seat (priv->grab_pointer));
      priv->grab_pointer = NULL;
    }
}

/* --------------------------------- */

typedef struct _CtkCellEditableEventBox CtkCellEditableEventBox;
typedef         CtkEventBoxClass        CtkCellEditableEventBoxClass;

struct _CtkCellEditableEventBox
{
  CtkEventBox box;
  gboolean editing_canceled;
  CtkCellRendererAccelMode accel_mode;
  gchar *path;
  CtkCellRenderer *cell;
};

enum {
  PROP_EDITING_CANCELED = 1,
  PROP_MODE,
  PROP_PATH
};

GType       ctk_cell_editable_event_box_get_type (void);
static void ctk_cell_editable_event_box_cell_editable_init (CtkCellEditableIface *iface);

G_DEFINE_TYPE_WITH_CODE (CtkCellEditableEventBox, ctk_cell_editable_event_box, CTK_TYPE_EVENT_BOX,
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_CELL_EDITABLE, ctk_cell_editable_event_box_cell_editable_init))

static void
ctk_cell_editable_event_box_start_editing (CtkCellEditable *cell_editable G_GNUC_UNUSED,
                                           CdkEvent        *event G_GNUC_UNUSED)
{
  /* do nothing, because we are pointless */
}

static void
ctk_cell_editable_event_box_cell_editable_init (CtkCellEditableIface *iface)
{
  iface->start_editing = ctk_cell_editable_event_box_start_editing;
}

static gboolean
ctk_cell_editable_event_box_key_press_event (CtkWidget   *widget,
                                             CdkEventKey *event)
{
  CtkCellEditableEventBox *box = (CtkCellEditableEventBox*)widget;
  CdkModifierType accel_mods = 0;
  guint accel_key;
  guint keyval;
  gboolean edited;
  gboolean cleared;
  CdkModifierType consumed_modifiers;
  CdkDisplay *display;

  display = ctk_widget_get_display (widget);

  if (event->is_modifier)
    return TRUE;

  edited = FALSE;
  cleared = FALSE;

  accel_mods = event->state;

  if (event->keyval == CDK_KEY_Sys_Req && 
      (accel_mods & CDK_MOD1_MASK) != 0)
    {
      /* HACK: we don't want to use SysRq as a keybinding (but we do
       * want Alt+Print), so we avoid translation from Alt+Print to SysRq
       */
      keyval = CDK_KEY_Print;
      consumed_modifiers = 0;
    }
  else
    {
      _ctk_translate_keyboard_accel_state (cdk_keymap_get_for_display (display),
                                           event->hardware_keycode,
                                           event->state,
                                           ctk_accelerator_get_default_mod_mask (),
                                           event->group,
                                           &keyval, NULL, NULL, &consumed_modifiers);
    }

  accel_key = cdk_keyval_to_lower (keyval);
  if (accel_key == CDK_KEY_ISO_Left_Tab) 
    accel_key = CDK_KEY_Tab;

  accel_mods &= ctk_accelerator_get_default_mod_mask ();

  /* Filter consumed modifiers 
   */
  if (box->accel_mode == CTK_CELL_RENDERER_ACCEL_MODE_CTK)
    accel_mods &= ~consumed_modifiers;
  
  /* Put shift back if it changed the case of the key, not otherwise.
   */
  if (accel_key != keyval)
    accel_mods |= CDK_SHIFT_MASK;
    
  if (accel_mods == 0)
    {
      switch (keyval)
	{
	case CDK_KEY_BackSpace:
	  cleared = TRUE;
          /* fall thru */
	case CDK_KEY_Escape:
	  goto out;
	default:
	  break;
	}
    }

  if (box->accel_mode == CTK_CELL_RENDERER_ACCEL_MODE_CTK &&
      !ctk_accelerator_valid (accel_key, accel_mods))
    {
      ctk_widget_error_bell (widget);
      return TRUE;
    }

  edited = TRUE;

 out:
  ctk_grab_remove (widget);
  ctk_cell_renderer_accel_ungrab (CTK_CELL_RENDERER_ACCEL (box->cell));
  ctk_cell_editable_editing_done (CTK_CELL_EDITABLE (widget));
  ctk_cell_editable_remove_widget (CTK_CELL_EDITABLE (widget));

  if (edited)
    g_signal_emit (box->cell, signals[ACCEL_EDITED], 0, box->path,
                   accel_key, accel_mods, event->hardware_keycode);
  else if (cleared)
    g_signal_emit (box->cell, signals[ACCEL_CLEARED], 0, box->path);

  return TRUE;
}

static void
ctk_cell_editable_event_box_unrealize (CtkWidget *widget)
{
  CtkCellEditableEventBox *box = (CtkCellEditableEventBox*)widget;

  ctk_grab_remove (widget);
  ctk_cell_renderer_accel_ungrab (CTK_CELL_RENDERER_ACCEL (box->cell));
  
  CTK_WIDGET_CLASS (ctk_cell_editable_event_box_parent_class)->unrealize (widget); 
}

static void
ctk_cell_editable_event_box_set_property (GObject      *object,
                                          guint         prop_id,
                                          const GValue *value,
                                          GParamSpec   *pspec)
{
  CtkCellEditableEventBox *box = (CtkCellEditableEventBox*)object;

  switch (prop_id)
    {
    case PROP_EDITING_CANCELED:
      box->editing_canceled = g_value_get_boolean (value);
      break;
    case PROP_MODE:
      box->accel_mode = g_value_get_enum (value);
      break;
    case PROP_PATH:
      box->path = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_cell_editable_event_box_get_property (GObject    *object,
                                          guint       prop_id,
                                          GValue     *value,
                                          GParamSpec *pspec)
{
  CtkCellEditableEventBox *box = (CtkCellEditableEventBox*)object;

  switch (prop_id)
    {
    case PROP_EDITING_CANCELED:
      g_value_set_boolean (value, box->editing_canceled);
      break;
    case PROP_MODE:
      g_value_set_enum (value, box->accel_mode);
      break;
    case PROP_PATH:
      g_value_set_string (value, box->path);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_cell_editable_event_box_finalize (GObject *object)
{
  CtkCellEditableEventBox *box = (CtkCellEditableEventBox*)object;

  g_free (box->path);

  G_OBJECT_CLASS (ctk_cell_editable_event_box_parent_class)->finalize (object);
}

static void
ctk_cell_editable_event_box_class_init (CtkCellEditableEventBoxClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (class);

  object_class->finalize = ctk_cell_editable_event_box_finalize;
  object_class->set_property = ctk_cell_editable_event_box_set_property;
  object_class->get_property = ctk_cell_editable_event_box_get_property;

  widget_class->key_press_event = ctk_cell_editable_event_box_key_press_event;
  widget_class->unrealize = ctk_cell_editable_event_box_unrealize;

  g_object_class_override_property (object_class,
                                    PROP_EDITING_CANCELED,
                                    "editing-canceled");

  g_object_class_install_property (object_class, PROP_MODE,
      g_param_spec_enum ("accel-mode", NULL, NULL,
                         CTK_TYPE_CELL_RENDERER_ACCEL_MODE,
                         CTK_CELL_RENDERER_ACCEL_MODE_CTK,
                         CTK_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_PATH,
      g_param_spec_string ("path", NULL, NULL,
                           NULL, CTK_PARAM_READWRITE));

  ctk_widget_class_set_css_name (widget_class, "acceleditor");
}

static void
ctk_cell_editable_event_box_init (CtkCellEditableEventBox *box)
{
  ctk_widget_set_can_focus (CTK_WIDGET (box), TRUE);
}

static CtkWidget *
ctk_cell_editable_event_box_new (CtkCellRenderer          *cell,
                                 CtkCellRendererAccelMode  mode,
                                 const gchar              *path)
{
  CtkCellEditableEventBox *box;

  box = g_object_new (ctk_cell_editable_event_box_get_type (),
                      "accel-mode", mode,
                      "path", path,
                      NULL);
  box->cell = cell;

  return CTK_WIDGET (box);
}
