/**
 * @file    tasks.c
 * @author  Jose
 * @brief   FreeRTOS tasks creation
 * @date    15/9/26
 * @project firmware-root
 */

#ifndef TASKS_H
#define TASKS_H

#include <stdint.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "event_groups.h"
#include "queue.h"
#include "semphr.h"
#include "tsl2591.h"
#include "sh1106.h"
#include "sht40.h"

#define FLAG_TSL_DONE   (1 << 0)
#define FLAG_SHT_DONE   (1 << 1)
#define FLAG_UI_DONE    (1 << 2)
#define ALL_SYNC_FLAGS  (FLAG_TSL_DONE | FLAG_SHT_DONE | FLAG_UI_DONE)

typedef enum {
 SENSOR_TYPE_TSL2591 = 0,
 SENSOR_TYPE_SHT40
} SensorType_t;

typedef struct {
 SensorType_t type;
 uint32_t timestamp_ms;
 union {
  struct {
   float lux;
   uint16_t ch0_raw;
   uint16_t ch1_raw;
  } tsl2591;
  struct {
   float temperature;
   float humidity;
  } sht40;
 } data;
} SensorDataMsg_t;

extern QueueHandle_t xTslMailbox;
extern QueueHandle_t xShtMailbox;
extern SemaphoreHandle_t xI2C2Mutex;
extern EventGroupHandle_t xSyncEventGroup;
extern TaskHandle_t ui_task_handle;
extern TaskHandle_t tsl2591_task_handle;
extern TaskHandle_t sht40_task_handle;
extern TaskHandle_t spi_test_handle;

void TSL2591_Measure_Task(void *pvParameters);
void SHT40_Measure_Task(void *pvParameters);
void UI_Task(void *pvParameters);
void SPI_Test_Task(void *pvParameters);

bool Tasks_Init(TSL2591_Handle_t *tsl_h, SHT40_Handle_t *sht_h, SH1106_Handle_t *disp_h);

#endif //TASKS_H