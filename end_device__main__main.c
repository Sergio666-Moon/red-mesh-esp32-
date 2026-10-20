#include <stdint.h>
#include <string.h>
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_now.h"
#include "esp_sleep.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nodes.h"
#include "packet.h"
#include "sensor.h"

#define TAG "DISPOSITIVO_FINAL"
#define NODE_ID 3
#define SLEEP_PERIOD_US (30ULL * 1000000ULL)

static const uint8_t router_mac[6] = ROUTER_MAC;

static void wifi_espnow_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE));
    ESP_ERROR_CHECK(esp_now_init());
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_espnow_init();

    esp_now_peer_info_t peer = {0};
    memcpy(peer.peer_addr, router_mac, 6);
    peer.channel = ESPNOW_CHANNEL;
    peer.ifidx = WIFI_IF_STA;
    peer.encrypt = false;
    ESP_ERROR_CHECK(esp_now_add_peer(&peer));

    float temperature = 0;
    float humidity = 0;
    if (sensor_init() == ESP_OK && sensor_read(&temperature, &humidity) == ESP_OK) {
        sensor_packet_t packet = {
            .node_id = NODE_ID,
            .hops = 0,
            .temperature = (int16_t)(temperature * 100),
            .humidity = (uint16_t)(humidity * 100),
        };
        esp_err_t err = esp_now_send(router_mac, (uint8_t *)&packet, sizeof(packet));
        ESP_LOGI(TAG, "Temperatura: %.2f C | Humedad: %.2f %% | Envio: %s", temperature, humidity, esp_err_to_name(err));
        vTaskDelay(pdMS_TO_TICKS(300));
    } else {
        ESP_LOGW(TAG, "Error con el sensor");
    }

    ESP_LOGI(TAG, "Entrando en sueno profundo");
    ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(SLEEP_PERIOD_US));
    esp_deep_sleep_start();
}
