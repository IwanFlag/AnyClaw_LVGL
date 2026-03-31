#ifndef APP_H
#define APP_H

#include "lvgl.h"

/* OpenClaw status */
enum class ClawStatus {
    NotInstalled,
    Detected,
    Running,
    Error
};

struct OpenClawInfo {
    char executable[512] = {0};
    char install_dir[512] = {0};
    char config_dir[512] = {0};
    char config_file[512] = {0};
    char version[128] = {0};
    int gateway_port = 18789;
};

/* UI functions */
void app_ui_init();
void app_refresh_status();

/* Manager functions */
OpenClawInfo app_detect_openclaw();
ClawStatus app_check_status(const OpenClawInfo& info);
bool app_start_gateway();
bool app_stop_gateway();

/* HTTP helper */
int http_get(const char* url, char* response, int resp_size, int timeout_sec = 3);

#endif /* APP_H */
