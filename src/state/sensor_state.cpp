#include "sensor_state.h"

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

namespace sensor_state {

static SensorSnapshot g_state;
static SemaphoreHandle_t g_mutex = nullptr;
static StaticSemaphore_t g_mutex_buf;   // static alloc — no heap after init

void init() {
    g_mutex = xSemaphoreCreateMutexStatic(&g_mutex_buf);
    for (int i = 0; i < CH_COUNT; i++) {
        g_state.ch[i] = { 0.0f, 0, false };
    }
}

void publish(Channel c, float value, bool valid) {
    xSemaphoreTake(g_mutex, portMAX_DELAY);
    g_state.ch[c].value = value;
    g_state.ch[c].ts_ms = millis();
    g_state.ch[c].valid = valid;
    xSemaphoreGive(g_mutex);
}

SensorSnapshot snapshot() {
    SensorSnapshot copy;
    xSemaphoreTake(g_mutex, portMAX_DELAY);
    copy = g_state;
    xSemaphoreGive(g_mutex);
    return copy;
}

} // namespace sensor_state
