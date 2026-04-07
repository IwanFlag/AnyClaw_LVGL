#ifndef HEALTH_H
#define HEALTH_H

#include "tray.h"
#include "app.h"  /* for ClawStatus */
#include "session_manager.h"  /* for SessionManager */

/* Health status callback type */
typedef void (*HealthStatusCallback)(ClawStatus status);

/* Start health monitoring thread (10s interval) */
void health_start();

/* Stop health monitoring thread */
void health_stop();

/* Set callback for status changes */
void health_set_callback(HealthStatusCallback cb);

/* Get count of active sessions (from last health check) */
int health_get_session_count();

/* Get formatted session info string (e.g. "webchat:30s, telegram:120s") */
const char* health_get_session_info();

#endif /* HEALTH_H */
