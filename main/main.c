/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_spi_flash.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_event.h"

#include "button.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "nvs_def.h"
#include "esp_err.h"
#include "server.h"
#include "application.h"

#define CONFIG_PIN GPIO_NUM_14

static EventGroupHandle_t button_events;
static uint8_t mode;
void button_task(void *arg);

void app_main()
{
    printf("Hello world!\n");
    //gpio_set_direction(GPIO_NUM_13, GPIO_MODE_OUTPUT);
    //gpio_set_level(GPIO_NUM_13,1);
    /* Print chip information */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("This is ESP8266 chip with %d CPU cores, WiFi, ",
            chip_info.cores);

    printf("silicon revision %d, ", chip_info.revision);

    printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    nvs_handle nvs_hnd;
    ESP_ERROR_CHECK(nvs_open(NVS_DEF_NAME, NVS_READWRITE, &nvs_hnd));
    esp_err_t err = nvs_get_u8(nvs_hnd, NVS_DEF_CONFIG_MODE, &mode);

    printf("ESP mode: %d\n", mode);

    if (err != ESP_OK && err == ESP_ERR_NVS_NOT_FOUND)
    {
        nvs_set_u8(nvs_hnd, NVS_DEF_CONFIG_MODE, MODE_NORMAL);
    }
    else
    {
        ESP_ERROR_CHECK(err);
    }

    nvs_commit(nvs_hnd);
    nvs_close(nvs_hnd);

    button_events = button_init(PIN_BIT(CONFIG_PIN));
    xTaskCreate(button_task, "button_task", 1024, NULL, tskIDLE_PRIORITY, NULL);

    if(mode == MODE_CONFIG)
    {
        server_init();
    }
    else
    {
        application_init();
    }
    
    /*
    char buf[1024];
    while (1)
    {
        //sprintf(buf, "%f",0.22);
        ftoa(ds18b20_get_temp(), buf, 4);
        ESP_LOGI("main","Temperature: %s \n",buf);
        fflush(stdout);
        vTaskDelay(100/portTICK_RATE_MS);
    }*/
    
}

void button_task(void *arg)
{
    button_event_t ev;
    while (true)
    {
        if (xQueueReceive(button_events, &ev, portMAX_DELAY))
        {
            if ((ev.pin == CONFIG_PIN) && (ev.event == BUTTON_DOWN))
            {
                nvs_handle nvs_hnd;
                ESP_ERROR_CHECK(nvs_open(NVS_DEF_NAME, NVS_READWRITE, &nvs_hnd));
                esp_err_t err = nvs_get_u8(nvs_hnd, NVS_DEF_CONFIG_MODE, &mode);

                if (err != ESP_OK && err == ESP_ERR_NVS_NOT_FOUND)
                {
                    nvs_set_u8(nvs_hnd, NVS_DEF_CONFIG_MODE, MODE_CONFIG);
                }
                else
                {
                    ESP_ERROR_CHECK(err);
                }

                nvs_set_u8(nvs_hnd, NVS_DEF_CONFIG_MODE, MODE_CONFIG);
                
                nvs_commit(nvs_hnd);
                nvs_close(nvs_hnd);

                esp_restart();
            }
        }
    }
}