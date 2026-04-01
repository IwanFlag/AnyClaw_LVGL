#ifndef HEALTH_H
#define HEALTH_H

#include "tray.h"
#include "app.h"  /* for ClawStatus */

/* Health status callback type */
typedef void (*HealthStatusCallback)(ClawStatus status);

/* Start health monitoring thread (10s interval) */
void health_start();

/* Stop health monitoring thread */
void health_stop();

/* Set callback for status changes */
void health_set_callback(HealthStatusCallback cb);

#endif /* HEALTH_H */
