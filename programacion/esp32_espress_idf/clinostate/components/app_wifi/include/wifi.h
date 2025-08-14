#ifndef APP_WIFI_H
#define APP_WIFI_H

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inicializa NVS, esp_netif y el event loop (idempotente).
 * @return ESP_OK en éxito.
 */
esp_err_t wifi_init(void);
#ifndef APP_WIFI_H
#define APP_WIFI_H

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inicializa NVS, esp_netif, loop de eventos y driver Wi-Fi (idempotente).
 */
esp_err_t wifi_init(void);

/**
 * @brief Conecta a un AP en modo estación. Bloquea hasta éxito/timeout.
 *
 * @param ssid       SSID (null-terminated)
 * @param pass       Password ("" si red abierta)
 * @param timeout_ms Tiempo máximo de espera (e.g., 15000)
 * @return ESP_OK si conectado y con IP, ESP_ERR_TIMEOUT si falla por timeout,
 *         u otro código de error si falló antes.
 */
esp_err_t wifi_connect(const char *ssid, const char *pass, uint32_t timeout_ms);

/** @brief Desconecta si estaba conectado (no hace esp_wifi_stop). */
void wifi_disconnect(void);

/** @brief true si el STA está con IP (bit de conectado levantado). */
bool wifi_is_connected(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_WIFI_H */

/**
 * @brief Conecta a un AP en modo estación (bloqueante hasta éxito/timeout).
 *
 * @param ssid       SSID (null-terminated)
 * @param pass       Password (puede ser "" para open)
 * @param timeout_ms Tiempo máx. de espera (por ej. 15000)
 * @return ESP_OK si se conectó y obtuvo IP; ESP_ERR_TIMEOUT si expiró;
 *         o un error de ESP-IDF si falló algo antes.
 */
esp_err_t wifi_connect(const char *ssid, const char *pass, uint32_t timeout_ms);

/**
 * @brief Desconecta (si está conectado) y para el driver Wi-Fi (opcional).
 */
void wifi_disconnect(void);

/**
 * @brief Devuelve true si el STA está asociado y con IP (bit de conectado).
 */
bool wifi_is_connected(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_WIFI_H */
