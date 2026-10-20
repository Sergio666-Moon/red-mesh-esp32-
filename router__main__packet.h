#pragma once

#include <stdint.h>

typedef struct __attribute__((packed)) {
    uint8_t node_id;
    uint8_t hops;
    int16_t temperature;
    uint16_t humidity;
} sensor_packet_t;
