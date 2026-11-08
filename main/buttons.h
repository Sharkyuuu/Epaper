#pragma once

#include <stdint.h>

#include "driver/gpio.h"
#include "esp_err.h"

#define BUTTONS_POLL_INTERVAL_MS 10

typedef enum {
  BUTTON_EVENT_SHORT,
  BUTTON_EVENT_LONG,
  BUTTON_EVENT_REPEAT,
  BUTTON_EVENT_RELEASE,
} button_event_type_t;

typedef struct {
  const char *button_name;
  gpio_num_t gpio;
  button_event_type_t type;
  uint32_t duration_ms;
  uint32_t count;
} button_event_t;

typedef void (*button_event_handler_t)(const button_event_t *event,
                                       void *context);

//-------------------------------------------------------------------------------------------------------------------
// @brief      Initialize all project buttons with internal pull-ups.
// @param      handler     Callback invoked when a button event is detected.
// @param      context     User context passed to the callback.
// @return     ESP_OK on success, otherwise an ESP-IDF error code.
//-------------------------------------------------------------------------------------------------------------------
esp_err_t buttons_init(button_event_handler_t handler, void *context);

//-------------------------------------------------------------------------------------------------------------------
// @brief      Poll all buttons and generate debounced events.
// @note       Call this function every BUTTONS_POLL_INTERVAL_MS.
//-------------------------------------------------------------------------------------------------------------------
void buttons_poll(void);

//-------------------------------------------------------------------------------------------------------------------
// @brief      Convert a button event type to a readable name.
// @param      type        Event type to convert.
// @return     Static event name string.
//-------------------------------------------------------------------------------------------------------------------
const char *button_event_type_name(button_event_type_t type);
