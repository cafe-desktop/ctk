/* CTK - The GIMP Toolkit
 * ctkprintbackend.h: Abstract printer backend interfaces
 * Copyright (C) 2003, Red Hat, Inc.
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
#include "ctkmodules.h"
#include "ctkmodulesprivate.h"
#include "ctkmarshalers.h"
#include "ctkprivate.h"
#include "ctkprintbackend.h"


static void ctk_print_backend_dispose      (GObject      *object);
static void ctk_print_backend_set_property (GObject      *object,
                                            guint         prop_id,
                                            const GValue *value,
                                            GParamSpec   *pspec);
static void ctk_print_backend_get_property (GObject      *object,
                                            guint         prop_id,
                                            GValue       *value,
                                            GParamSpec   *pspec);

struct _CtkPrintBackendPrivate
{
  GHashTable *printers;
  guint printer_list_requested : 1;
  guint printer_list_done : 1;
  CtkPrintBackendStatus status;
  char **auth_info_required;
  char **auth_info;
  gboolean store_auth_info;
};

enum {
  PRINTER_LIST_CHANGED,
  PRINTER_LIST_DONE,
  PRINTER_ADDED,
  PRINTER_REMOVED,
  PRINTER_STATUS_CHANGED,
  REQUEST_PASSWORD,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

enum 
{ 
  PROP_ZERO,
  PROP_STATUS
};

static GObjectClass *backend_parent_class;

GQuark
ctk_print_backend_error_quark (void)
{
  static GQuark quark = 0;
  if (quark == 0)
    quark = g_quark_from_static_string ("ctk-print-backend-error-quark");
  return quark;
}

/*****************************************
 *     CtkPrintBackendModule modules     *
 *****************************************/

typedef struct _CtkPrintBackendModule CtkPrintBackendModule;
typedef struct _CtkPrintBackendModuleClass CtkPrintBackendModuleClass;

struct _CtkPrintBackendModule
{
  GTypeModule parent_instance;
  
  GModule *library;

  void             (*init)     (GTypeModule    *module);
  void             (*exit)     (void);
  CtkPrintBackend* (*create)   (void);

  gchar *path;
};

struct _CtkPrintBackendModuleClass
{
  GTypeModuleClass parent_class;
};

GType _ctk_print_backend_module_get_type (void);

G_DEFINE_TYPE (CtkPrintBackendModule, _ctk_print_backend_module, G_TYPE_TYPE_MODULE)
#define CTK_TYPE_PRINT_BACKEND_MODULE      (_ctk_print_backend_module_get_type ())
#define CTK_PRINT_BACKEND_MODULE(module)   (G_TYPE_CHECK_INSTANCE_CAST ((module), CTK_TYPE_PRINT_BACKEND_MODULE, CtkPrintBackendModule))

static GSList *loaded_backends;

static gboolean
ctk_print_backend_module_load (GTypeModule *module)
{
  CtkPrintBackendModule *pb_module = CTK_PRINT_BACKEND_MODULE (module); 
  gpointer initp, exitp, createp;
 
  pb_module->library = g_module_open (pb_module->path, G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL);
  if (!pb_module->library)
    {
      g_warning ("%s", g_module_error ());
      return FALSE;
    }
  
  /* extract symbols from the lib */
  if (!g_module_symbol (pb_module->library, "pb_module_init",
			&initp) ||
      !g_module_symbol (pb_module->library, "pb_module_exit", 
			&exitp) ||
      !g_module_symbol (pb_module->library, "pb_module_create", 
			&createp))
    {
      g_warning ("%s", g_module_error ());
      g_module_close (pb_module->library);
      
      return FALSE;
    }

  pb_module->init = initp;
  pb_module->exit = exitp;
  pb_module->create = createp;

  /* call the printbackend's init function to let it */
  /* setup anything it needs to set up. */
  pb_module->init (module);

  return TRUE;
}

static void
ctk_print_backend_module_unload (GTypeModule *module)
{
  CtkPrintBackendModule *pb_module = CTK_PRINT_BACKEND_MODULE (module);
  
  pb_module->exit();

  g_module_close (pb_module->library);
  pb_module->library = NULL;

  pb_module->init = NULL;
  pb_module->exit = NULL;
  pb_module->create = NULL;
}

/* This only will ever be called if an error occurs during
 * initialization
 */
static void
ctk_print_backend_module_finalize (GObject *object)
{
  CtkPrintBackendModule *module = CTK_PRINT_BACKEND_MODULE (object);

  g_free (module->path);

  G_OBJECT_CLASS (_ctk_print_backend_module_parent_class)->finalize (object);
}

static void
_ctk_print_backend_module_class_init (CtkPrintBackendModuleClass *class)
{
  GTypeModuleClass *module_class = G_TYPE_MODULE_CLASS (class);
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

  module_class->load = ctk_print_backend_module_load;
  module_class->unload = ctk_print_backend_module_unload;

  gobject_class->finalize = ctk_print_backend_module_finalize;
}

static void 
ctk_print_backend_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  CtkPrintBackend *backend = CTK_PRINT_BACKEND (object);
  CtkPrintBackendPrivate *priv = backend->priv;

  switch (prop_id)
    {
    case PROP_STATUS:
      priv->status = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void 
ctk_print_backend_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  CtkPrintBackend *backend = CTK_PRINT_BACKEND (object);
  CtkPrintBackendPrivate *priv = backend->priv;

  switch (prop_id)
    {
    case PROP_STATUS:
      g_value_set_int (value, priv->status);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
_ctk_print_backend_module_init (CtkPrintBackendModule *pb_module)
{
}

static CtkPrintBackend *
_ctk_print_backend_module_create (CtkPrintBackendModule *pb_module)
{
  if (g_type_module_use (G_TYPE_MODULE (pb_module)))
    {
      CtkPrintBackend *pb;

      pb = pb_module->create ();
      g_type_module_unuse (G_TYPE_MODULE (pb_module));
      return pb;
    }
  return NULL;
}

static CtkPrintBackend *
_ctk_print_backend_create (const gchar *backend_name)
{
  GSList *l;
  gchar *module_path;
  CtkPrintBackendModule *pb_module;
  CtkPrintBackend *pb;

  for (l = loaded_backends; l != NULL; l = l->next)
    {
      pb_module = l->data;
      
      if (strcmp (G_TYPE_MODULE (pb_module)->name, backend_name) == 0)
	return _ctk_print_backend_module_create (pb_module);
    }

  pb = NULL;
  if (g_module_supported ())
    {
      gchar *full_name;

      full_name = g_strconcat ("printbackend-", backend_name, NULL);
      module_path = _ctk_find_module (full_name, "printbackends");
      g_free (full_name);

      if (module_path)
	{
	  pb_module = g_object_new (CTK_TYPE_PRINT_BACKEND_MODULE, NULL);

	  g_type_module_set_name (G_TYPE_MODULE (pb_module), backend_name);
	  pb_module->path = module_path;

	  loaded_backends = g_slist_prepend (loaded_backends,
		   		             pb_module);

	  pb = _ctk_print_backend_module_create (pb_module);

	  /* Increase use-count so that we don't unload print backends.
	   * There is a problem with module unloading in the cups module,
	   * see cups_dispatch_watch_finalize for details. 
	   */
	  g_type_module_use (G_TYPE_MODULE (pb_module));
	}
    }

  return pb;
}

/**
 * ctk_print_backend_load_modules:
 *
 * Returns: (element-type CtkPrintBackend) (transfer container):
 */
GList *
ctk_print_backend_load_modules (void)
{
  GList *result;
  gchar *setting;
  gchar **backends;
  gint i;
  CtkSettings *settings;

  result = NULL;

  settings = ctk_settings_get_default ();
  if (settings)
    g_object_get (settings, "ctk-print-backends", &setting, NULL);
  else
    setting = g_strdup (CTK_PRINT_BACKENDS);

  backends = g_strsplit (setting, ",", -1);

  for (i = 0; backends[i]; i++)
    {
      CtkPrintBackend *backend;

      backend = _ctk_print_backend_create (g_strstrip (backends[i]));
      if (backend)
        result = g_list_append (result, backend);
    }

  g_strfreev (backends);
  g_free (setting);

  return result;
}

/*****************************************
 *             CtkPrintBackend           *
 *****************************************/

G_DEFINE_TYPE_WITH_PRIVATE (CtkPrintBackend, ctk_print_backend, G_TYPE_OBJECT)

static void                 fallback_printer_request_details       (CtkPrinter          *printer);
static gboolean             fallback_printer_mark_conflicts        (CtkPrinter          *printer,
								    CtkPrinterOptionSet *options);
static gboolean             fallback_printer_get_hard_margins      (CtkPrinter          *printer,
                                                                    gdouble             *top,
                                                                    gdouble             *bottom,
                                                                    gdouble             *left,
                                                                    gdouble             *right);
static gboolean             fallback_printer_get_hard_margins_for_paper_size (CtkPrinter          *printer,
									      CtkPaperSize        *paper_size,
									      gdouble             *top,
									      gdouble             *bottom,
									      gdouble             *left,
									      gdouble             *right);
static GList *              fallback_printer_list_papers           (CtkPrinter          *printer);
static CtkPageSetup *       fallback_printer_get_default_page_size (CtkPrinter          *printer);
static CtkPrintCapabilities fallback_printer_get_capabilities      (CtkPrinter          *printer);
static void                 request_password                       (CtkPrintBackend     *backend,
                                                                    gpointer             auth_info_required,
                                                                    gpointer             auth_info_default,
                                                                    gpointer             auth_info_display,
                                                                    gpointer             auth_info_visible,
                                                                    const gchar         *prompt,
                                                                    gboolean             can_store_auth_info);
  
static void
ctk_print_backend_class_init (CtkPrintBackendClass *class)
{
  GObjectClass *object_class;
  object_class = (GObjectClass *) class;

  backend_parent_class = g_type_class_peek_parent (class);
  
  object_class->dispose = ctk_print_backend_dispose;
  object_class->set_property = ctk_print_backend_set_property;
  object_class->get_property = ctk_print_backend_get_property;

  class->printer_request_details = fallback_printer_request_details;
  class->printer_mark_conflicts = fallback_printer_mark_conflicts;
  class->printer_get_hard_margins = fallback_printer_get_hard_margins;
  class->printer_get_hard_margins_for_paper_size = fallback_printer_get_hard_margins_for_paper_size;
  class->printer_list_papers = fallback_printer_list_papers;
  class->printer_get_default_page_size = fallback_printer_get_default_page_size;
  class->printer_get_capabilities = fallback_printer_get_capabilities;
  class->request_password = request_password;
  
  g_object_class_install_property (object_class, 
                                   PROP_STATUS,
                                   g_param_spec_int ("status",
                                                     "Status",
                                                     "The status of the print backend",
                                                     CTK_PRINT_BACKEND_STATUS_UNKNOWN,
                                                     CTK_PRINT_BACKEND_STATUS_UNAVAILABLE,
                                                     CTK_PRINT_BACKEND_STATUS_UNKNOWN,
                                                     CTK_PARAM_READWRITE)); 
  
  signals[PRINTER_LIST_CHANGED] =
    g_signal_new (I_("printer-list-changed"),
		  G_TYPE_FROM_CLASS (class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkPrintBackendClass, printer_list_changed),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);
  signals[PRINTER_LIST_DONE] =
    g_signal_new (I_("printer-list-done"),
		    G_TYPE_FROM_CLASS (class),
		    G_SIGNAL_RUN_LAST,
		    G_STRUCT_OFFSET (CtkPrintBackendClass, printer_list_done),
		    NULL, NULL,
		    NULL,
		    G_TYPE_NONE, 0);
  signals[PRINTER_ADDED] =
    g_signal_new (I_("printer-added"),
		  G_TYPE_FROM_CLASS (class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkPrintBackendClass, printer_added),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 1, CTK_TYPE_PRINTER);
  signals[PRINTER_REMOVED] =
    g_signal_new (I_("printer-removed"),
		  G_TYPE_FROM_CLASS (class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkPrintBackendClass, printer_removed),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 1, CTK_TYPE_PRINTER);
  signals[PRINTER_STATUS_CHANGED] =
    g_signal_new (I_("printer-status-changed"),
		  G_TYPE_FROM_CLASS (class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkPrintBackendClass, printer_status_changed),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 1, CTK_TYPE_PRINTER);
  signals[REQUEST_PASSWORD] =
    g_signal_new (I_("request-password"),
		  G_TYPE_FROM_CLASS (class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkPrintBackendClass, request_password),
		  NULL, NULL, NULL,
		  G_TYPE_NONE, 6, G_TYPE_POINTER, G_TYPE_POINTER, G_TYPE_POINTER,
		  G_TYPE_POINTER, G_TYPE_STRING, G_TYPE_BOOLEAN);
}

static void
ctk_print_backend_init (CtkPrintBackend *backend)
{
  CtkPrintBackendPrivate *priv;

  priv = backend->priv = ctk_print_backend_get_instance_private (backend);

  priv->printers = g_hash_table_new_full (g_str_hash, g_str_equal, 
					  (GDestroyNotify) g_free,
					  (GDestroyNotify) g_object_unref);
  priv->auth_info_required = NULL;
  priv->auth_info = NULL;
}

static void
ctk_print_backend_dispose (GObject *object)
{
  CtkPrintBackend *backend;
  CtkPrintBackendPrivate *priv;

  backend = CTK_PRINT_BACKEND (object);
  priv = backend->priv;

  /* We unref the printers in dispose, not in finalize so that
   * we can break refcount cycles with ctk_print_backend_destroy 
   */
  if (priv->printers)
    {
      g_hash_table_destroy (priv->printers);
      priv->printers = NULL;
    }

  backend_parent_class->dispose (object);
}


static void
fallback_printer_request_details (CtkPrinter *printer)
{
}

static gboolean
fallback_printer_mark_conflicts (CtkPrinter          *printer,
				 CtkPrinterOptionSet *options)
{
  return FALSE;
}

static gboolean
fallback_printer_get_hard_margins (CtkPrinter *printer,
				   gdouble    *top,
				   gdouble    *bottom,
				   gdouble    *left,
				   gdouble    *right)
{
  return FALSE;
}

static gboolean
fallback_printer_get_hard_margins_for_paper_size (CtkPrinter   *printer,
						  CtkPaperSize *paper_size,
						  gdouble      *top,
						  gdouble      *bottom,
						  gdouble      *left,
						  gdouble      *right)
{
  return FALSE;
}

static GList *
fallback_printer_list_papers (CtkPrinter *printer)
{
  return NULL;
}

static CtkPageSetup *
fallback_printer_get_default_page_size (CtkPrinter *printer)
{
  return NULL;
}

static CtkPrintCapabilities
fallback_printer_get_capabilities (CtkPrinter *printer)
{
  return 0;
}


static void
printer_hash_to_sorted_active_list (const gchar  *key,
                                    gpointer      value,
                                    GList       **out_list)
{
  CtkPrinter *printer;

  printer = CTK_PRINTER (value);

  if (ctk_printer_get_name (printer) == NULL)
    return;

  if (!ctk_printer_is_active (printer))
    return;

  *out_list = g_list_insert_sorted (*out_list, value, (GCompareFunc) ctk_printer_compare);
}


void
ctk_print_backend_add_printer (CtkPrintBackend *backend,
			       CtkPrinter      *printer)
{
  CtkPrintBackendPrivate *priv;
  
  g_return_if_fail (CTK_IS_PRINT_BACKEND (backend));

  priv = backend->priv;

  if (!priv->printers)
    return;
  
  g_hash_table_insert (priv->printers,
		       g_strdup (ctk_printer_get_name (printer)), 
		       g_object_ref (printer));
}

void
ctk_print_backend_remove_printer (CtkPrintBackend *backend,
				  CtkPrinter      *printer)
{
  CtkPrintBackendPrivate *priv;
  
  g_return_if_fail (CTK_IS_PRINT_BACKEND (backend));
  priv = backend->priv;

  if (!priv->printers)
    return;
  
  g_hash_table_remove (priv->printers,
		       ctk_printer_get_name (printer));
}

void
ctk_print_backend_set_list_done (CtkPrintBackend *backend)
{
  if (!backend->priv->printer_list_done)
    {
      backend->priv->printer_list_done = TRUE;
      g_signal_emit (backend, signals[PRINTER_LIST_DONE], 0);
    }
}


/**
 * ctk_print_backend_get_printer_list:
 *
 * Returns the current list of printers.
 *
 * Returns: (element-type CtkPrinter) (transfer container):
 *   A list of #CtkPrinter objects. The list should be freed
 *   with g_list_free().
 */
GList *
ctk_print_backend_get_printer_list (CtkPrintBackend *backend)
{
  CtkPrintBackendPrivate *priv;
  GList *result;
  
  g_return_val_if_fail (CTK_IS_PRINT_BACKEND (backend), NULL);

  priv = backend->priv;

  result = NULL;
  if (priv->printers != NULL)
    g_hash_table_foreach (priv->printers,
                          (GHFunc) printer_hash_to_sorted_active_list,
                          &result);

  if (!priv->printer_list_requested && priv->printers != NULL)
    {
      if (CTK_PRINT_BACKEND_GET_CLASS (backend)->request_printer_list)
	CTK_PRINT_BACKEND_GET_CLASS (backend)->request_printer_list (backend);
      priv->printer_list_requested = TRUE;
    }

  return result;
}

gboolean
ctk_print_backend_printer_list_is_done (CtkPrintBackend *print_backend)
{
  g_return_val_if_fail (CTK_IS_PRINT_BACKEND (print_backend), TRUE);

  return print_backend->priv->printer_list_done;
}

CtkPrinter *
ctk_print_backend_find_printer (CtkPrintBackend *backend,
                                const gchar     *printer_name)
{
  CtkPrintBackendPrivate *priv;
  CtkPrinter *printer;
  
  g_return_val_if_fail (CTK_IS_PRINT_BACKEND (backend), NULL);

  priv = backend->priv;

  if (priv->printers)
    printer = g_hash_table_lookup (priv->printers, printer_name);
  else
    printer = NULL;

  return printer;  
}

void
ctk_print_backend_print_stream (CtkPrintBackend        *backend,
                                CtkPrintJob            *job,
                                GIOChannel             *data_io,
                                CtkPrintJobCompleteFunc callback,
                                gpointer                user_data,
				GDestroyNotify          dnotify)
{
  g_return_if_fail (CTK_IS_PRINT_BACKEND (backend));

  CTK_PRINT_BACKEND_GET_CLASS (backend)->print_stream (backend,
						       job,
						       data_io,
						       callback,
						       user_data,
						       dnotify);
}

void 
ctk_print_backend_set_password (CtkPrintBackend  *backend,
                                gchar           **auth_info_required,
                                gchar           **auth_info,
                                gboolean          store_auth_info)
{
  g_return_if_fail (CTK_IS_PRINT_BACKEND (backend));

  if (CTK_PRINT_BACKEND_GET_CLASS (backend)->set_password)
    CTK_PRINT_BACKEND_GET_CLASS (backend)->set_password (backend,
                                                         auth_info_required,
                                                         auth_info,
                                                         store_auth_info);
}

static void
store_auth_info_toggled (CtkCheckButton *chkbtn,
                         gpointer        user_data)
{
  gboolean *data = (gboolean *) user_data;
  *data = ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (chkbtn));
}

static void
store_entry (CtkEntry  *entry,
             gpointer   user_data)
{
  gchar **data = (gchar **) user_data;

  if (*data != NULL)
    {
      memset (*data, 0, strlen (*data));
      g_free (*data);
    }

  *data = g_strdup (ctk_entry_get_text (entry));
}

static void
password_dialog_response (CtkWidget       *dialog,
                          gint             response_id,
                          CtkPrintBackend *backend)
{
  CtkPrintBackendPrivate *priv = backend->priv;
  gint i, auth_info_len;

  if (response_id == CTK_RESPONSE_OK)
    ctk_print_backend_set_password (backend, priv->auth_info_required, priv->auth_info, priv->store_auth_info);
  else
    ctk_print_backend_set_password (backend, priv->auth_info_required, NULL, FALSE);

  /* We want to clear the data before freeing it */
  auth_info_len = g_strv_length (priv->auth_info_required);
  for (i = 0; i < auth_info_len; i++)
    {
      if (priv->auth_info[i] != NULL)
        {
          memset (priv->auth_info[i], 0, strlen (priv->auth_info[i]));
          g_free (priv->auth_info[i]);
          priv->auth_info[i] = NULL;
        }
    }

  g_clear_pointer (&priv->auth_info, g_free);
  g_clear_pointer (&priv->auth_info_required, g_strfreev);

  ctk_widget_destroy (dialog);

  g_object_unref (backend);
}

static void
request_password (CtkPrintBackend  *backend,
                  gpointer          auth_info_required,
                  gpointer          auth_info_default,
                  gpointer          auth_info_display,
                  gpointer          auth_info_visible,
                  const gchar      *prompt,
                  gboolean          can_store_auth_info)
{
  CtkPrintBackendPrivate *priv = backend->priv;
  CtkWidget *dialog, *box, *main_box, *label, *icon, *vbox, *entry;
  CtkWidget *focus = NULL;
  CtkWidget *content_area;
  gchar     *markup;
  gint       length;
  gint       i;
  gchar    **ai_required = (gchar **) auth_info_required;
  gchar    **ai_default = (gchar **) auth_info_default;
  gchar    **ai_display = (gchar **) auth_info_display;
  gboolean  *ai_visible = (gboolean *) auth_info_visible;

  priv->auth_info_required = g_strdupv (ai_required);
  length = g_strv_length (ai_required);
  priv->auth_info = g_new0 (gchar *, length + 1);
  priv->store_auth_info = FALSE;

  dialog = ctk_dialog_new_with_buttons ( _("Authentication"), NULL, CTK_DIALOG_MODAL, 
                                         _("_Cancel"), CTK_RESPONSE_CANCEL,
                                         _("_OK"), CTK_RESPONSE_OK,
                                         NULL);

  ctk_dialog_set_default_response (CTK_DIALOG (dialog), CTK_RESPONSE_OK);

  main_box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);

  /* Left */
  icon = ctk_image_new_from_icon_name ("dialog-password-symbolic", CTK_ICON_SIZE_DIALOG);
  ctk_widget_set_halign (icon, CTK_ALIGN_CENTER);
  ctk_widget_set_valign (icon, CTK_ALIGN_START);
  g_object_set (icon, "margin", 6, NULL);

  /* Right */
  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  ctk_widget_set_size_request (CTK_WIDGET (vbox), 320, -1);

  /* Right - 1. */
  label = ctk_label_new (NULL);
  markup = g_markup_printf_escaped ("<span weight=\"bold\" size=\"large\">%s</span>", prompt);
  ctk_label_set_markup (CTK_LABEL (label), markup);
  ctk_label_set_line_wrap (CTK_LABEL (label), TRUE);
  ctk_widget_set_size_request (CTK_WIDGET (label), 320, -1);
  g_free (markup);


  /* Packing */
  content_area = ctk_dialog_get_content_area (CTK_DIALOG (dialog));
  ctk_box_pack_start (CTK_BOX (content_area), main_box, TRUE, FALSE, 0);

  ctk_box_pack_start (CTK_BOX (main_box), icon, FALSE, FALSE, 6);
  ctk_box_pack_start (CTK_BOX (main_box), vbox, FALSE, FALSE, 6);

  ctk_box_pack_start (CTK_BOX (vbox), label, FALSE, TRUE, 6);
  
  /* Right - 2. */
  for (i = 0; i < length; i++)
    {
      priv->auth_info[i] = g_strdup (ai_default[i]);
      if (ai_display[i] != NULL)
        {
          box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
          ctk_box_set_homogeneous (CTK_BOX (box), TRUE);

          label = ctk_label_new (ai_display[i]);
          ctk_widget_set_halign (label, CTK_ALIGN_START);
          ctk_widget_set_valign (label, CTK_ALIGN_CENTER);

          entry = ctk_entry_new ();
          focus = entry;

          if (ai_default[i] != NULL)
            ctk_entry_set_text (CTK_ENTRY (entry), ai_default[i]);

          ctk_entry_set_visibility (CTK_ENTRY (entry), ai_visible[i]);
          ctk_entry_set_activates_default (CTK_ENTRY (entry), TRUE);

          ctk_box_pack_start (CTK_BOX (vbox), box, FALSE, TRUE, 6);

          ctk_box_pack_start (CTK_BOX (box), label, TRUE, TRUE, 0);
          ctk_box_pack_start (CTK_BOX (box), entry, TRUE, TRUE, 0);

          g_signal_connect (entry, "changed",
                            G_CALLBACK (store_entry), &(priv->auth_info[i]));
        }
    }

  if (can_store_auth_info)
    {
      CtkWidget *chkbtn;

      chkbtn = ctk_check_button_new_with_mnemonic (_("_Remember password"));
      ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (chkbtn), FALSE);
      ctk_box_pack_start (CTK_BOX (vbox), chkbtn, FALSE, FALSE, 6);
      g_signal_connect (chkbtn, "toggled",
                        G_CALLBACK (store_auth_info_toggled),
                        &(priv->store_auth_info));
    }

  if (focus != NULL)
    {
      ctk_widget_grab_focus (focus);
      focus = NULL;
    }

  g_object_ref (backend);
  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK (password_dialog_response), backend);

  ctk_widget_show_all (dialog);
}

void
ctk_print_backend_destroy (CtkPrintBackend *print_backend)
{
  /* The lifecycle of print backends and printers are tied, such that
   * the backend owns the printers, but the printers also ref the backend.
   * This is so that if the app has a reference to a printer its backend
   * will be around. However, this results in a cycle, which we break
   * with this call, which causes the print backend to release its printers.
   */
  g_object_run_dispose (G_OBJECT (print_backend));
}
