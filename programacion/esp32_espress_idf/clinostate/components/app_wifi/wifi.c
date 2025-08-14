#include "wifi.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_check.h"     // ESP_RETURN_ON_* (IDF v5.x)

static const char *TAG = "app_wifi";

/* Event bits */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static EventGroupHandle_t s_wifi_event_group = NULL;
static esp_event_handler_instance_t s_any_id_inst, s_got_ip_inst;
static bool s_initialized = false;
static bool s_started = false;
static int s_retry_num = 0;
static const int s_max_retry = 5;

/* Handler de eventos:
   - NO llama esp_wifi_connect() en WIFI_EVENT_STA_START (evitamos doble connect).
   - Reintenta en DISCONNECTED hasta s_max_retry.
   - Marca CONNECTED cuando obtiene IP. */
static void wifi_event_handler(void* arg, esp_event_base_t base, int32_t id, void* data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < s_max_retry) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGW(TAG, "Wi-Fi desconectado, reintentando... (%d/%d)", s_retry_num, s_max_retry);
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        const ip_event_got_ip_t* ev = (const ip_event_got_ip_t*) data;
        ESP_LOGI(TAG, "IP: " IPSTR, IP2STR(&ev->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

esp_err_t wifi_init(void)
{
    if (s_initialized) return ESP_OK;

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_RETURN_ON_ERROR(err, TAG, "nvs_flash_init failed");

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    (void) esp_netif_create_default_wifi_sta();

    wifi_init_config_t wicfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wicfg));

    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL, &s_any_id_inst));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL, &s_got_ip_inst));

    s_initialized = true;
    return ESP_OK;
}

esp_err_t wifi_connect(const char *ssid, const char *pass, uint32_t timeout_ms)
{
    ESP_RETURN_ON_FALSE(s_initialized, ESP_ERR_INVALID_STATE, TAG, "wifi_init() no llamado");
    ESP_RETURN_ON_FALSE(ssid && ssid[0] != '\0', ESP_ERR_INVALID_ARG, TAG, "SSID vacío");

    wifi_config_t wc = {0};
    snprintf((char*)wc.sta.ssid,     sizeof(wc.sta.ssid),     "%s", ssid);
    snprintf((char*)wc.sta.password, sizeof(wc.sta.password), "%s", pass ? pass : "");
    wc.sta.threshold.authmode = (pass && pass[0]) ? WIFI_AUTH_WPA2_PSK : WIFI_AUTH_OPEN;
    wc.sta.pmf_cfg.capable = true;
    wc.sta.pmf_cfg.required = false;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wc));

    /* Arranca el driver si no estaba en marcha */
    if (!s_started) {
        ESP_ERROR_CHECK(esp_wifi_start());
        s_started = true;
    }

    /* Limpia flags y arranca un solo intento de conexión.
       Importante: NO conectamos en WIFI_EVENT_STA_START en el handler. */
    xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);
    s_retry_num = 0;

    // Ignora error benigno si ya está "connecting"
    esp_err_t err = esp_wifi_connect();
    if (err == ESP_ERR_WIFI_CONN) {
        ESP_LOGW(TAG, "connect() llamado mientras ya conecta; continúo");
    } else {
        ESP_ERROR_CHECK(err);
    }

    /* Espera resultado */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE, pdFALSE,
                                           pdMS_TO_TICKS(timeout_ms));

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Conectado a '%s'", ssid);
        return ESP_OK;
    }
    if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "No se pudo conectar a '%s'", ssid);
        return ESP_ERR_TIMEOUT;
    }
    ESP_LOGE(TAG, "Timeout esperando conexión");
    return ESP_ERR_TIMEOUT;
}

void wifi_disconnect(void)
{
    if (!s_initialized) return;
    (void) esp_wifi_disconnect();
    // Si quisieras parar completamente:
    // (void) esp_wifi_stop(); s_started = false;
}

bool wifi_is_connected(void)
{
    if (!s_wifi_event_group) return false;
    EventBits_t b = xEventGroupGetBits(s_wifi_event_group);
    return (b & WIFI_CONNECTED_BIT) != 0;
}
