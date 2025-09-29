#pragma once
#include "mqtt_client.h"
#ifdef __cplusplus
extern "C" {
#endif

extern char client_id[32];
extern esp_mqtt_client_handle_t client;
void mqtt_start(const char *uri);

#ifdef __cplusplus
}
#endif