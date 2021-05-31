/* ctkcupsutils.h 
 * Copyright (C) 2006 John (J5) Palmieri <johnp@redhat.com>
 *
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
 
#ifndef __CTK_CUPS_UTILS_H__
#define __CTK_CUPS_UTILS_H__

#include <glib.h>
#include <cups/cups.h>
#include <cups/language.h>
#include <cups/http.h>
#include <cups/ipp.h>

G_BEGIN_DECLS

typedef struct _GtkCupsRequest        GtkCupsRequest;
typedef struct _GtkCupsResult         GtkCupsResult;
typedef struct _GtkCupsConnectionTest GtkCupsConnectionTest;

typedef enum
{
  CTK_CUPS_ERROR_HTTP,
  CTK_CUPS_ERROR_IPP,
  CTK_CUPS_ERROR_IO,
  CTK_CUPS_ERROR_AUTH,
  CTK_CUPS_ERROR_GENERAL
} GtkCupsErrorType;

typedef enum
{
  CTK_CUPS_POST,
  CTK_CUPS_GET
} GtkCupsRequestType;


/** 
 * Direction we should be polling the http socket on.
 * We are either reading or writting at each state.
 * This makes it easy for mainloops to connect to poll.
 */
typedef enum
{
  CTK_CUPS_HTTP_IDLE,
  CTK_CUPS_HTTP_READ,
  CTK_CUPS_HTTP_WRITE
} GtkCupsPollState;

typedef enum
{
  CTK_CUPS_CONNECTION_AVAILABLE,
  CTK_CUPS_CONNECTION_NOT_AVAILABLE,
  CTK_CUPS_CONNECTION_IN_PROGRESS  
} GtkCupsConnectionState;

typedef enum
{
  CTK_CUPS_PASSWORD_NONE,
  CTK_CUPS_PASSWORD_REQUESTED,
  CTK_CUPS_PASSWORD_HAS,
  CTK_CUPS_PASSWORD_APPLIED,
  CTK_CUPS_PASSWORD_NOT_VALID
} GtkCupsPasswordState;

struct _GtkCupsRequest 
{
  GtkCupsRequestType type;

  http_t *http;
  http_status_t last_status;
  ipp_t *ipp_request;

  gchar *server;
  gchar *resource;
  GIOChannel *data_io;
  gint attempts;

  GtkCupsResult *result;

  gint state;
  GtkCupsPollState poll_state;
  guint64 bytes_received;

  gchar *password;
  gchar *username;

  gint own_http : 1;
  gint need_password : 1;
  gint need_auth_info : 1;
  gchar **auth_info_required;
  gchar **auth_info;
  GtkCupsPasswordState password_state;
};

struct _GtkCupsConnectionTest
{
  GtkCupsConnectionState at_init;
  http_addrlist_t       *addrlist;
  http_addrlist_t       *current_addr;
  http_addrlist_t       *last_wrong_addr;
  gint                   socket;
};

#define CTK_CUPS_REQUEST_START 0
#define CTK_CUPS_REQUEST_DONE 500

/* POST states */
enum 
{
  CTK_CUPS_POST_CONNECT = CTK_CUPS_REQUEST_START,
  CTK_CUPS_POST_SEND,
  CTK_CUPS_POST_WRITE_REQUEST,
  CTK_CUPS_POST_WRITE_DATA,
  CTK_CUPS_POST_CHECK,
  CTK_CUPS_POST_AUTH,
  CTK_CUPS_POST_READ_RESPONSE,
  CTK_CUPS_POST_DONE = CTK_CUPS_REQUEST_DONE
};

/* GET states */
enum
{
  CTK_CUPS_GET_CONNECT = CTK_CUPS_REQUEST_START,
  CTK_CUPS_GET_SEND,
  CTK_CUPS_GET_CHECK,
  CTK_CUPS_GET_AUTH,
  CTK_CUPS_GET_READ_DATA,
  CTK_CUPS_GET_DONE = CTK_CUPS_REQUEST_DONE
};

GtkCupsRequest        * ctk_cups_request_new_with_username (http_t             *connection,
							    GtkCupsRequestType  req_type,
							    gint                operation_id,
							    GIOChannel         *data_io,
							    const char         *server,
							    const char         *resource,
							    const char         *username);
GtkCupsRequest        * ctk_cups_request_new               (http_t             *connection,
							    GtkCupsRequestType  req_type,
							    gint                operation_id,
							    GIOChannel         *data_io,
							    const char         *server,
							    const char         *resource);
void                    ctk_cups_request_ipp_add_string    (GtkCupsRequest     *request,
							    ipp_tag_t           group,
							    ipp_tag_t           tag,
							    const char         *name,
							    const char         *charset,
							    const char         *value);
void                    ctk_cups_request_ipp_add_strings   (GtkCupsRequest     *request,
							    ipp_tag_t           group,
							    ipp_tag_t           tag,
							    const char         *name,
							    int                 num_values,
							    const char         *charset,
							    const char * const *values);
const char            * ctk_cups_request_ipp_get_string    (GtkCupsRequest     *request,
							    ipp_tag_t           tag,
							    const char         *name);
gboolean                ctk_cups_request_read_write        (GtkCupsRequest     *request,
                                                            gboolean            connect_only);
GtkCupsPollState        ctk_cups_request_get_poll_state    (GtkCupsRequest     *request);
void                    ctk_cups_request_free              (GtkCupsRequest     *request);
GtkCupsResult         * ctk_cups_request_get_result        (GtkCupsRequest     *request);
gboolean                ctk_cups_request_is_done           (GtkCupsRequest     *request);
void                    ctk_cups_request_encode_option     (GtkCupsRequest     *request,
						            const gchar        *option,
							    const gchar        *value);
void                    ctk_cups_request_set_ipp_version   (GtkCupsRequest     *request,
							    gint                major,
							    gint                minor);
gboolean                ctk_cups_result_is_error           (GtkCupsResult      *result);
ipp_t                 * ctk_cups_result_get_response       (GtkCupsResult      *result);
GtkCupsErrorType        ctk_cups_result_get_error_type     (GtkCupsResult      *result);
int                     ctk_cups_result_get_error_status   (GtkCupsResult      *result);
int                     ctk_cups_result_get_error_code     (GtkCupsResult      *result);
const char            * ctk_cups_result_get_error_string   (GtkCupsResult      *result);
GtkCupsConnectionTest * ctk_cups_connection_test_new       (const char         *server,
                                                            const int           port);
GtkCupsConnectionState  ctk_cups_connection_test_get_state (GtkCupsConnectionTest *test);
void                    ctk_cups_connection_test_free      (GtkCupsConnectionTest *test);

G_END_DECLS
#endif 
