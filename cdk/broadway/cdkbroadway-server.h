#ifndef __CDK_BROADWAY_SERVER__
#define __CDK_BROADWAY_SERVER__

#include <cdk/cdktypes.h>
#include "broadway-protocol.h"

typedef struct _CdkBroadwayServer CdkBroadwayServer;
typedef struct _CdkBroadwayServerClass CdkBroadwayServerClass;

#define CDK_TYPE_BROADWAY_SERVER              (cdk_broadway_server_get_type())
#define CDK_BROADWAY_SERVER(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), CDK_TYPE_BROADWAY_SERVER, CdkBroadwayServer))
#define CDK_BROADWAY_SERVER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), CDK_TYPE_BROADWAY_SERVER, CdkBroadwayServerClass))
#define CDK_IS_BROADWAY_SERVER(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), CDK_TYPE_BROADWAY_SERVER))
#define CDK_IS_BROADWAY_SERVER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), CDK_TYPE_BROADWAY_SERVER))
#define CDK_BROADWAY_SERVER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), CDK_TYPE_BROADWAY_SERVER, CdkBroadwayServerClass))

CdkBroadwayServer *_cdk_broadway_server_new                      (const char         *display,
								  GError            **error);
void               _cdk_broadway_server_flush                    (CdkBroadwayServer  *server);
void               _cdk_broadway_server_sync                     (CdkBroadwayServer  *server);
gulong             _cdk_broadway_server_get_next_serial          (CdkBroadwayServer  *server);
guint32            _cdk_broadway_server_get_last_seen_time       (CdkBroadwayServer  *server);
gboolean           _cdk_broadway_server_lookahead_event          (CdkBroadwayServer  *server,
								  const char         *types);
void               _cdk_broadway_server_query_mouse              (CdkBroadwayServer  *server,
								  guint32            *toplevel,
								  gint32             *root_x,
								  gint32             *root_y,
								  guint32            *mask);
CdkGrabStatus      _cdk_broadway_server_grab_pointer             (CdkBroadwayServer  *server,
								  gint                id,
								  gboolean            owner_events,
								  guint32             event_mask,
								  guint32             time_);
guint32            _cdk_broadway_server_ungrab_pointer           (CdkBroadwayServer  *server,
								  guint32             time_);
gint32             _cdk_broadway_server_get_mouse_toplevel       (CdkBroadwayServer  *server);
guint32            _cdk_broadway_server_new_window               (CdkBroadwayServer  *server,
								  int                 x,
								  int                 y,
								  int                 width,
								  int                 height,
								  gboolean            is_temp);
void               _cdk_broadway_server_destroy_window           (CdkBroadwayServer  *server,
								  gint                id);
gboolean           _cdk_broadway_server_window_show              (CdkBroadwayServer  *server,
								  gint                id);
gboolean           _cdk_broadway_server_window_hide              (CdkBroadwayServer  *server,
								  gint                id);
void               _cdk_broadway_server_window_focus             (CdkBroadwayServer  *server,
								  gint                id);
void               _cdk_broadway_server_window_set_transient_for (CdkBroadwayServer  *server,
								  gint                id,
								  gint                parent);
void               _cdk_broadway_server_set_show_keyboard        (CdkBroadwayServer  *server,
								  gboolean            show_keyboard);
gboolean           _cdk_broadway_server_window_translate         (CdkBroadwayServer  *server,
								  gint                id,
								  cairo_region_t     *area,
								  gint                dx,
								  gint                dy);
cairo_surface_t   *_cdk_broadway_server_create_surface           (int                 width,
								  int                 height);
void               _cdk_broadway_server_window_update            (CdkBroadwayServer  *server,
								  gint                id,
								  cairo_surface_t    *surface);
gboolean           _cdk_broadway_server_window_move_resize       (CdkBroadwayServer  *server,
								  gint                id,
								  gboolean            with_move,
								  int                 x,
								  int                 y,
								  int                 width,
								  int                 height);

#endif /* __CDK_BROADWAY_SERVER__ */
