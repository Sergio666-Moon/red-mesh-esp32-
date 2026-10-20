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
#include "packet.h"

#define TAG "COORDINADOR"
#define ESPNOW_CHANNEL 1

typedef struct {
    uint8_t src[6];
    sensor_packet_t packet;
} rx_item_t;

static QueueHandle_t rx_queue;

static void recv_cb(const esp_now_recv_info_t *info, const uint8_t *data, int len)
{
    if (len != sizeof(sensor_packet_t)) {
        return;
    }
    rx_item_t item;
    memcpy(item.src, info->src_addr, 6);
    memcpy(&item.packet, data, sizeof(sensor_packet_t));
    xQueueSend(rx_queue, &item, 0);
}

static void print_task(void *arg)
{
    rx_item_t item;
    while (1) {
        if (xQueueReceive(rx_queue, &item, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI(TAG, "Nodo %d | Saltos: %d | Enviado por " MACSTR " | Temperatura: %.2f C | Humedad: %.2f %%",
                     item.packet.node_id, item.packet.hops, MAC2STR(item.src),
                     item.packet.temperature / 100.0f, item.packet.humidity / 100.0f);
        }
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
    ESP_LOGI(TAG, "MAC del coordinador: " MACSTR, MAC2STR(mac));

    rx_queue = xQueueCreate(20, sizeof(rx_item_t));
    ESP_ERROR_CHECK(esp_now_register_recv_cb(recv_cb));
    xTaskCreate(print_task, "print_task", 4096, NULL, 5, NULL);
}
