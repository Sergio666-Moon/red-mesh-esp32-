#include <stdint.h>
#include <string.h>
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "nodes.h"
#include "packet.h"
#include "sensor.h"

#define TAG "ROUTER"
#define NODE_ID 2
#define REPORT_PERIOD_MS 10000

static const uint8_t coordinator_mac[6] = COORDINATOR_MAC;
static QueueHandle_t relay_queue;

static void recv_cb(const esp_now_recv_info_t *info, const uint8_t *data, int len)
{
    if (len != sizeof(sensor_packet_t)) {
        return;
    }
    sensor_packet_t packet;
    memcpy(&packet, data, sizeof(packet));
    xQueueSend(relay_queue, &packet, 0);
}

static void relay_task(void *arg)
{
    sensor_packet_t packet;
    while (1) {
        if (xQueueReceive(relay_queue, &packet, portMAX_DELAY) == pdTRUE) {
            packet.hops++;
            esp_err_t err = esp_now_send(coordinator_mac, (uint8_t *)&packet, sizeof(packet));
            ESP_LOGI(TAG, "Retransmitiendo paquete del nodo %d: %s", packet.node_id, esp_err_to_name(err));
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }
}

static void sensor_task(void *arg)
{
    if (sensor_init() != ESP_OK) {
        ESP_LOGE(TAG, "Error al iniciar el sensor");
        vTaskDelete(NULL);
        return;
    }
    while (1) {
        float temperature = 0;
        float humidity = 0;
        if (sensor_read(&temperature, &humidity) == ESP_OK) {
            sensor_packet_t packet = {
                .node_id = NODE_ID,
                .hops = 0,
                .temperature = (int16_t)(temperature * 100),
                .humidity = (uint16_t)(humidity * 100),
            };
            esp_err_t err = esp_now_send(coordinator_mac, (uint8_t *)&packet, sizeof(packet));
            ESP_LOGI(TAG, "Temperatura: %.2f C | Humedad: %.2f %% | Envio: %s", temperature, humidity, esp_err_to_name(err));
        } else {
            ESP_LOGW(TAG, "Error al leer el sensor");
        }
        vTaskDelay(pdMS_TO_TICKS(REPORT_PERIOD_MS));
    }
}

static void wifi_espnow_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    ESP_ERROR_CHECK(esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE));
    ESP_ERROR_CHECK(esp_now_init());
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_espnow_init();

    uint8_t mac[6];
    ESP_ERROR_CHECK(esp_read_mac(mac, ESP_MAC_WIFI_STA));
    ESP_LOGI(TAG, "MAC del router: " MACSTR, MAC2STR(mac));

    esp_now_peer_info_t peer = {0};
    memcpy(peer.peer_addr, coordinator_mac, 6);
    peer.channel = ESPNOW_CHANNEL;
    peer.ifidx = WIFI_IF_STA;
    peer.encrypt = false;
    ESP_ERROR_CHECK(esp_now_add_peer(&peer));

    relay_queue = xQueueCreate(20, sizeof(sensor_packet_t));
    ESP_ERROR_CHECK(esp_now_register_recv_cb(recv_cb));
    xTaskCreate(relay_task, "relay_task", 4096, NULL, 5, NULL);
    xTaskCreate(sensor_task, "sensor_task", 4096, NULL, 4, NULL);
}
