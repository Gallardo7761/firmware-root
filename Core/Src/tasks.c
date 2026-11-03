/**
 * @file    tasks.c
 * @author  Jose
 * @brief   FreeRTOS tasks creation
 * @date    15/9/26
 * @project firmware-root
 */

#include "tasks.h"
#include "task.h"
#include "tsl2591.h"
#include "sh1106.h"
#include "ui.h"
#include "util.h"
#include "main.h"

extern SPI_HandleTypeDef hspi1;

QueueHandle_t xTslMailbox = NULL;
SemaphoreHandle_t xI2C2Mutex;
EventGroupHandle_t xSyncEventGroup = NULL;
TaskHandle_t ui_task_handle;
TaskHandle_t tsl2591_task_handle;
TaskHandle_t spi_test_handle;

bool Tasks_Init(TSL2591_Handle_t *tsl_h, SH1106_Handle_t *disp_h)
{
    if (tsl_h == NULL || disp_h == NULL) return false;

    xTslMailbox = xQueueCreate(1, sizeof(SensorDataMsg_t));
    if (xTslMailbox == NULL) return false;

    xSyncEventGroup = xEventGroupCreate();
    if (xSyncEventGroup == NULL) return false;

    xI2C2Mutex = xSemaphoreCreateMutex();
    if (xI2C2Mutex == NULL) return false;

    BaseType_t ret;

    ret = xTaskCreate(
        TSL2591_Measure_Task,
        "TSL2591 Task",
        256,
        tsl_h,
        2,
        &tsl2591_task_handle
    );
    if (ret != pdPASS) return false;

    ret = xTaskCreate(
        UI_Task,
        "UI Task",
        512,
        disp_h,
        1,
        &ui_task_handle
    );
    if (ret != pdPASS) return false;

    ret = xTaskCreate(
        SPI_Test_Task,
        "SPI Test",
        256,
        NULL,
        1,
        &spi_test_handle
    );
    if (ret != pdPASS) return false;

    return true;
}

void TSL2591_Measure_Task(void *pvParameters)
{
    TSL2591_Handle_t *tsl_h = (TSL2591_Handle_t *)pvParameters;
    if (tsl_h == NULL) vTaskDelete(NULL);

    uint16_t ch0 = 0, ch1 = 0;
    SensorDataMsg_t msg;
    msg.type = SENSOR_TYPE_TSL2591;

    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        uint16_t delay;
        switch (tsl_h->integration_time) {
            case TSL2591_INTEGRATIONTIME_100MS:
                delay = 100;
                break;
            case TSL2591_INTEGRATIONTIME_200MS:
                delay = 200;
                break;
            case TSL2591_INTEGRATIONTIME_300MS:
                delay = 300;
                break;
            case TSL2591_INTEGRATIONTIME_400MS:
                delay = 400;
                break;
            case TSL2591_INTEGRATIONTIME_500MS:
                delay = 500;
                break;
            case TSL2591_INTEGRATIONTIME_600MS:
                delay = 600;
                break;
            default:
                delay = 200;
                break;
        }
        vTaskDelay(delay);

        xSemaphoreTake(xI2C2Mutex, portMAX_DELAY);
        HAL_StatusTypeDef st = TSL2591_Read_Raw_Data(tsl_h, &ch0, &ch1);
        xSemaphoreGive(xI2C2Mutex);

        if ( st == HAL_OK) {
            msg.timestamp_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
            msg.data.tsl2591.ch0_raw = ch0;
            msg.data.tsl2591.ch1_raw = ch1;
            msg.data.tsl2591.lux = TSL2591_Calculate_Lux(tsl_h, ch0, ch1);

            xQueueOverwrite(xTslMailbox, &msg);
        }

        xEventGroupSetBits(xSyncEventGroup, FLAG_TSL_DONE);
    }
}

void UI_Task(void *pvParameters)
{
    SH1106_Handle_t *disp_h = (SH1106_Handle_t *)pvParameters;
    if (disp_h == NULL) vTaskDelete(NULL);

    /* Cached latest data */
    float last_lux = 0.0f;
    uint8_t battery_pct = 85;   // TODO real battery level
    bool lora_connected = true; // TODO real connection status

    SensorDataMsg_t msg;

    UI_Container_t root_screen = {0, 0, 128, 64, 2};
    UI_Container_t status_bar_view, main_view;

    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (xQueuePeek(xTslMailbox, &msg, 0) == pdPASS) last_lux = msg.data.tsl2591.lux;

        SH1106_Clear(disp_h);

        UI_Container_Split_V(&root_screen, &status_bar_view, &main_view, 18);
        UI_Draw_Status_Bar(disp_h, &status_bar_view, "Croppy v1.0", battery_pct, lora_connected);

        FloatSplit_t lux_split = Util_Split_Float(last_lux, 0);
        UI_Container_Draw_Icon(disp_h, &main_view, 0, 34, ICON_15x15_LUX, SH1106_PIXEL_ON);
        UI_Container_Draw_Text(disp_h, &main_view, 17, 34, SH1106_PIXEL_ON, &Font_10x16,
                      "Lux:  %u", lux_split.integer_part);

        (void)SH1106_Update_Screen(disp_h);

        xEventGroupSetBits(xSyncEventGroup, FLAG_UI_DONE);
    }
}

#define RFM95_REG_VERSION   0x42
#define RFM95_VERSION_ID    0x12

volatile uint8_t spi_test_version = 0;
volatile uint32_t spi_test_ok = 0;
volatile uint32_t spi_test_fail = 0;

void SPI_Test_Task(void *pvParameters)
{
    (void)pvParameters;

    /* SX1276: bit 7 del address a 0 = lectura */
    uint8_t tx[2] = {RFM95_REG_VERSION & 0x7F, 0x00};
    uint8_t rx[2];

    for (;;) {
        rx[1] = 0;
        HAL_GPIO_WritePin(SPI_CS_GPIO_Port, SPI_CS_Pin, GPIO_PIN_RESET);
        HAL_StatusTypeDef st = HAL_SPI_TransmitReceive(&hspi1, tx, rx, sizeof(tx), 100);
        HAL_GPIO_WritePin(SPI_CS_GPIO_Port, SPI_CS_Pin, GPIO_PIN_SET);

        spi_test_version = rx[1];
        if (st == HAL_OK && rx[1] == RFM95_VERSION_ID) spi_test_ok++;
        else spi_test_fail++;

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}