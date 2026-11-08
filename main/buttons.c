#include "buttons.h"

#include <stdbool.h>
#include <stddef.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BUTTON_DEBOUNCE_MS 30
#define BUTTON_LONG_PRESS_MS 800
#define BUTTON_REPEAT_INTERVAL_MS 250

typedef struct {
  const char *name;
  gpio_num_t gpio;
  int raw_level;
  int stable_level;
  TickType_t raw_changed_at;
  TickType_t pressed_at;
  TickType_t repeated_at;
  bool long_reported;
  uint32_t short_count;
  uint32_t long_count;
  uint32_t repeat_count;
} button_state_t;

static button_state_t s_buttons[] = {
    {.name = "UP", .gpio = GPIO_NUM_13},
    {.name = "DOWN", .gpio = GPIO_NUM_14},
    {.name = "OK", .gpio = GPIO_NUM_15},
    {.name = "RECORD", .gpio = GPIO_NUM_16},
};

static button_event_handler_t s_event_handler;
static void *s_event_context;

static void emit_event(button_state_t *button, button_event_type_t type,
                       TickType_t duration, uint32_t count) {
  if (s_event_handler == NULL) {
    return;
  }

  const button_event_t event = {
      .button_name = button->name,
      .gpio = button->gpio,
      .type = type,
      .duration_ms = duration * portTICK_PERIOD_MS,
      .count = count,
  };
  s_event_handler(&event, s_event_context);
}

static void update_button(button_state_t *button, TickType_t now) {
  const TickType_t debounce_ticks = pdMS_TO_TICKS(BUTTON_DEBOUNCE_MS);
  const TickType_t long_press_ticks = pdMS_TO_TICKS(BUTTON_LONG_PRESS_MS);
  const TickType_t repeat_ticks = pdMS_TO_TICKS(BUTTON_REPEAT_INTERVAL_MS);
  const int level = gpio_get_level(button->gpio);

  // A level must remain unchanged for the debounce period before it becomes
  // stable.
  if (level != button->raw_level) {
    button->raw_level = level;
    button->raw_changed_at = now;
  }

  if (button->raw_level != button->stable_level &&
      now - button->raw_changed_at >= debounce_ticks) {
    button->stable_level = button->raw_level;

    if (button->stable_level == 0) {
      button->pressed_at = now;
      button->repeated_at = now;
      button->long_reported = false;
    } else {
      const TickType_t duration = now - button->pressed_at;
      if (!button->long_reported) {
        button->short_count++;
        emit_event(button, BUTTON_EVENT_SHORT, duration, button->short_count);
      } else {
        emit_event(button, BUTTON_EVENT_RELEASE, duration, 0);
      }
    }
  }

  if (button->stable_level != 0) {
    return;
  }

  const TickType_t held_ticks = now - button->pressed_at;
  if (!button->long_reported && held_ticks >= long_press_ticks) {
    button->long_reported = true;
    button->repeated_at = now;
    button->long_count++;
    emit_event(button, BUTTON_EVENT_LONG, held_ticks, button->long_count);
  } else if (button->long_reported &&
             now - button->repeated_at >= repeat_ticks) {
    button->repeated_at = now;
    button->repeat_count++;
    emit_event(button, BUTTON_EVENT_REPEAT, held_ticks, button->repeat_count);
  }
}

esp_err_t buttons_init(button_event_handler_t handler, void *context) {
  uint64_t pin_mask = 0;
  for (size_t i = 0; i < sizeof(s_buttons) / sizeof(s_buttons[0]); i++) {
    pin_mask |= 1ULL << s_buttons[i].gpio;
  }

  const gpio_config_t config = {
      .pin_bit_mask = pin_mask,
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_ENABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE,
  };
  const esp_err_t result = gpio_config(&config);
  if (result != ESP_OK) {
    return result;
  }

  s_event_handler = handler;
  s_event_context = context;

  const TickType_t now = xTaskGetTickCount();
  for (size_t i = 0; i < sizeof(s_buttons) / sizeof(s_buttons[0]); i++) {
    const int level = gpio_get_level(s_buttons[i].gpio);
    s_buttons[i].raw_level = level;
    s_buttons[i].stable_level = level;
    s_buttons[i].raw_changed_at = now;
    s_buttons[i].pressed_at = now;
    s_buttons[i].repeated_at = now;
    s_buttons[i].long_reported = false;
  }
  return ESP_OK;
}

void buttons_poll(void) {
  const TickType_t now = xTaskGetTickCount();
  for (size_t i = 0; i < sizeof(s_buttons) / sizeof(s_buttons[0]); i++) {
    update_button(&s_buttons[i], now);
  }
}

const char *button_event_type_name(button_event_type_t type) {
  switch (type) {
  case BUTTON_EVENT_SHORT:
    return "SHORT";
  case BUTTON_EVENT_LONG:
    return "LONG";
  case BUTTON_EVENT_REPEAT:
    return "REPEAT";
  case BUTTON_EVENT_RELEASE:
    return "RELEASE";
  default:
    return "UNKNOWN";
  }
}
