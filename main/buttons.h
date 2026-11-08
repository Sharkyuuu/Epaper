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
// @brief      按键模块初始化
// @param      handler     按键事件回调函数
// @param      context     传递给回调函数的用户参数，不需要时填 NULL
// @return     ESP_OK：初始化成功；其他值：初始化失败
// Sample usage:           buttons_init(button_event_handler, NULL);
//-------------------------------------------------------------------------------------------------------------------
esp_err_t buttons_init(button_event_handler_t handler, void *context);

//-------------------------------------------------------------------------------------------------------------------
// @brief      扫描全部按键并生成短按、长按、连发和松开事件
// @param      void
// @return     void
// Sample usage:           buttons_poll(); // 每隔 BUTTONS_POLL_INTERVAL_MS 调用一次
//-------------------------------------------------------------------------------------------------------------------
void buttons_poll(void);

//-------------------------------------------------------------------------------------------------------------------
// @brief      获取按键事件名称
// @param      type        按键事件类型
// @return     对应的静态字符串，例如 "SHORT" 或 "LONG"
// Sample usage:           const char *name = button_event_type_name(event.type);
//-------------------------------------------------------------------------------------------------------------------
const char *button_event_type_name(button_event_type_t type);
