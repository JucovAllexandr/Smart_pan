#include "application.h"
#include <string.h>
#include <stdio.h>

#include "ds18b20.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "nvs_def.h"
#include "utils.h"
#include "cJSON.h"
#include "pid.h"

#define TAG "APPLICATION"


void connect_wifi(char* ssid, char* pass);
esp_err_t wifi_event_handler(void *ctx, system_event_t *event);
esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event);
void mqtt_init();
void temperature_task(void *arg);
void publish_task(void *arg);
void i2c_task(void *arg);
void control_task(void *arg);

bool is_mqtt_connected = false;

void application_init()
{
    char ssid[35] = {0};
    size_t ssid_length = 0;
    char pass[35] = {0};
    size_t pass_length = 0;

    char kp[10] = {0};
    size_t kp_length = 0;
    char ki[10] = {0};
    size_t ki_length = 0;
    char kd[10] = {0};
    size_t kd_length = 0;

    nvs_handle hnd;
    esp_err_t err;
    err = nvs_open(NVS_DEF_NAME, NVS_READWRITE, &hnd);
    if(err != ESP_OK)
    {
        ESP_ERROR_CHECK(err);
    }

    nvs_get_str(hnd, NVS_DEF_WIFI_SSID, NULL, &ssid_length);
    err = nvs_get_str(hnd, NVS_DEF_WIFI_SSID, ssid, &ssid_length);

    if (err != ESP_OK && err == ESP_ERR_NVS_NOT_FOUND)
    {
        nvs_set_str(hnd, NVS_DEF_WIFI_SSID, "");
    }
    else
    {
        ESP_ERROR_CHECK(err);
    }

    nvs_get_str(hnd, NVS_DEF_WIFI_PASS, NULL, &pass_length);
    err = nvs_get_str(hnd, NVS_DEF_WIFI_PASS, pass, &pass_length);

    if (err != ESP_OK && err == ESP_ERR_NVS_NOT_FOUND)
    {
        nvs_set_str(hnd, NVS_DEF_WIFI_PASS, "");
    }
    else
    {
        ESP_ERROR_CHECK(err);
    }

    nvs_get_str(hnd, NVS_DEF_PID_KP, NULL, &kp_length);
    err = nvs_get_str(hnd, NVS_DEF_PID_KP, kp, &kp_length);

    if(err == ESP_OK)
    {
        printf("KP %s\n", kp);
        smart_pan_config.kp = strtof(kp, NULL);
    }
    else if (err != ESP_OK && err == ESP_ERR_NVS_NOT_FOUND)
    {
        nvs_set_str(hnd, NVS_DEF_PID_KP, "1");
        smart_pan_config.kp = PID_KP;
    }
    else
    {
        ESP_ERROR_CHECK(err);
    }

    nvs_get_str(hnd, NVS_DEF_PID_KI, NULL, &ki_length);
    err = nvs_get_str(hnd, NVS_DEF_PID_KI, ki, &ki_length);

    if(err == ESP_OK)
    {
        printf("KI %s\n", ki);
        smart_pan_config.ki = strtof(ki, NULL);
    }
    else if (err != ESP_OK && err == ESP_ERR_NVS_NOT_FOUND)
    {
        nvs_set_str(hnd, NVS_DEF_PID_KI, "1");
        smart_pan_config.ki = PID_KI;
    }
    else
    {
        ESP_ERROR_CHECK(err);
    }

    nvs_get_str(hnd, NVS_DEF_PID_KD, NULL, &kd_length);
    err = nvs_get_str(hnd, NVS_DEF_PID_KD, kd, &kd_length);

    if(err == ESP_OK)
    {
        smart_pan_config.kd = strtof(kd, NULL);
    }
    else if (err != ESP_OK && err == ESP_ERR_NVS_NOT_FOUND)
    {
        nvs_set_str(hnd, NVS_DEF_PID_KD, "0");
        smart_pan_config.kd = PID_KD;
    }
    else
    {
        ESP_ERROR_CHECK(err);
    }

    nvs_close(hnd);

    printf("Wi-Fi SSID:'%s', PASS:'%s'\n", ssid, pass);

    xSemaphore = xSemaphoreCreateMutex();

    if( xSemaphore == NULL )
    {
       ESP_LOGE(TAG, "Error create mutex");
       esp_restart();
    }

    if(strlen(ssid) > 0)
    {
        connect_wifi(ssid, pass);
        ds18b20_init(GPIO_NUM_12);
        mqtt_init();
        pid_init(smart_pan_config.kp, smart_pan_config.ki, smart_pan_config.kd, PID_MIN, PID_MAX);
        xTaskCreate(temperature_task, "temperature_task", 1024, NULL, 1, NULL);
        xTaskCreate(i2c_task, "i2c_task", 2048, NULL, 1, NULL);
        xTaskCreate(publish_task, "publish_task", 2048*2, NULL, 1, NULL);
        xTaskCreate(control_task, "control_task", 2048, NULL, 1, NULL);
    }
 
}

void connect_wifi(char* ssid, char* pass)
{
    ESP_ERROR_CHECK(esp_event_loop_init(wifi_event_handler, NULL));

    tcpip_adapter_init();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "",
            .password = ""
        },
    }; 

    strcpy((char *)wifi_config.sta.ssid, ssid);
    strcpy((char *)wifi_config.sta.password, pass);

    if (strlen((char *)wifi_config.sta.password)) {
        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    }

    printf("Wi-Fi SSID:'%s', PASS:'%s'\n", (char *)wifi_config.sta.ssid, (char *)wifi_config.sta.password);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id)
    {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "got ip:%s",
                 ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));

        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
    {
        ESP_LOGI(TAG, "retry to connect to the AP");
        ESP_LOGI(TAG, "connect to the AP fail\n");
        esp_wifi_connect();
        vTaskDelay(100/portTICK_RATE_MS);
        break;
    }
    default:
        break;
    }
    return ESP_OK;
}

void mqtt_init()
{
    mqtt_cfg.uri = MQTT_URL;
    mqtt_cfg.event_handle = mqtt_event_handler;

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);

    if (mqtt_client)
    {
        esp_mqtt_client_start(mqtt_client);
    }
    else
    {
        printf("Error esp_mqtt_client_init return NULL\n");
    }
}

esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        is_mqtt_connected = true;
        esp_mqtt_client_subscribe(mqtt_client, SUB_TOPIC, 1);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        is_mqtt_connected = false;
        break;
    case MQTT_EVENT_SUBSCRIBED:
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        break;
    case MQTT_EVENT_PUBLISHED:
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        
        if(xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE)
        {
            cJSON *obj = cJSON_Parse(event->data);
            cJSON *set_temp = cJSON_GetObjectItem(obj, "set_temperature");
            cJSON *wrk_mode = cJSON_GetObjectItem(obj, "work_mode");
            cJSON *kp = cJSON_GetObjectItem(obj, "kp");
            cJSON *ki = cJSON_GetObjectItem(obj, "ki");
            cJSON *kd = cJSON_GetObjectItem(obj, "kd");

            if(set_temp != NULL)
            {
                smart_pan_config.set_temperature = set_temp->valuedouble;
            }

            if(wrk_mode != NULL)
            {
                smart_pan_config.work_mode = wrk_mode->valueint;
            }

            if(kp != NULL && ki != NULL && kd != NULL)
            {
                smart_pan_config.kp = kp->valuedouble;
                smart_pan_config.ki = ki->valuedouble;
                smart_pan_config.kd = kd->valuedouble;

                char kp_str[10] = {0};
                char ki_str[10] = {0};
                char kd_str[10] = {0};

                nvs_handle hnd;
                esp_err_t err;

                sprintf(kp_str, "%f", smart_pan_config.kp);
                sprintf(ki_str, "%f", smart_pan_config.ki);
                sprintf(kd_str, "%f", smart_pan_config.kd);

                err = nvs_open(NVS_DEF_NAME, NVS_READWRITE, &hnd);

                if(err != ESP_OK)
                {
                    ESP_ERROR_CHECK(err);
                }

                nvs_set_str(hnd, NVS_DEF_PID_KP, kp_str);
                nvs_set_str(hnd, NVS_DEF_PID_KI, ki_str);
                nvs_set_str(hnd, NVS_DEF_PID_KD, kd_str);

                nvs_close(hnd);

                pid_set_coefficients(smart_pan_config.kp, smart_pan_config.ki, smart_pan_config.kd);
            }

            cJSON_Delete(obj);
            xSemaphoreGive(xSemaphore);    
        }
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
    return ESP_OK;
}

void temperature_task(void *arg)
{
    float temperature = 0;
    
    while (1)
    {
        temperature = ds18b20_get_temp();

        if(temperature < 150){
            if(xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE)
            {
                smart_pan_config.current_temperature = temperature;
                xSemaphoreGive( xSemaphore );
            }
        }
        vTaskDelay(750 / portTICK_RATE_MS);
    }
}

void publish_task(void *arg)
{
    char *string = NULL;
    cJSON *current_temperature = NULL;
    cJSON *set_temperature = NULL; 
    cJSON *current_pwm = NULL;
    cJSON *work_mode = NULL;
    cJSON *kp = NULL;
    cJSON *ki = NULL;
    cJSON *kd = NULL;

    while (1)
    {
        if(is_mqtt_connected){
            cJSON *sp_config = cJSON_CreateObject();
            if (sp_config == NULL)
            {
                ESP_LOGE(TAG, "Error cJSON_CreateObject");
            }
            
            if(xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE){
                current_temperature = cJSON_AddNumberToObject(sp_config, "current_temperature", smart_pan_config.current_temperature);
                if (current_temperature == NULL)
                {
                    ESP_LOGE(TAG, "Error cJSON_CreateNumber current_temperature");
                }

                set_temperature = cJSON_AddNumberToObject(sp_config, "set_temperature", smart_pan_config.set_temperature);
                if (set_temperature == NULL)
                {
                    ESP_LOGE(TAG, "Error cJSON_CreateNumber set_temperature");
                }

                current_pwm = cJSON_AddNumberToObject(sp_config, "current_pwm", smart_pan_config.current_pwm);
                if (current_pwm == NULL)
                {
                    ESP_LOGE(TAG, "Error cJSON_CreateNumber current_pwm");
                }

                work_mode = cJSON_AddNumberToObject(sp_config, "work_mode", smart_pan_config.work_mode);
                if (work_mode == NULL)
                {
                    ESP_LOGE(TAG, "Error cJSON_CreateNumber work_mode");
                }

                kp = cJSON_AddNumberToObject(sp_config, "kp", smart_pan_config.kp);
                if (kp == NULL)
                {
                    ESP_LOGE(TAG, "Error cJSON_CreateNumber kp");
                }

                ki = cJSON_AddNumberToObject(sp_config, "ki", smart_pan_config.ki);
                if (ki == NULL)
                {
                    ESP_LOGE(TAG, "Error cJSON_CreateNumber ki");
                }

                kd = cJSON_AddNumberToObject(sp_config, "kd", smart_pan_config.kd);
                if (kd == NULL)
                {
                    ESP_LOGE(TAG, "Error cJSON_CreateNumber kd");
                }

                xSemaphoreGive(xSemaphore);
            }
   
            string = cJSON_Print(sp_config);
            if (string == NULL)
            {
                ESP_LOGE(TAG, "Error cJSON_Print");
                
            }
            else
            {
                esp_mqtt_client_publish(mqtt_client, PUB_TOPIC, string, strlen(string), 0, 0);
                free(string);
            }
            

            cJSON_Delete(sp_config);
            
        }
        vTaskDelay(1000/portTICK_RATE_MS);
    }
    
}

esp_err_t i2c_master_init()
{
    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.sda_pullup_en = 1;
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.scl_pullup_en = 1;
    conf.clk_stretch_tick = 300; // 300 ticks, Clock stretch is about 210us, you can make changes according to the actual situation.
    ESP_ERROR_CHECK(i2c_driver_install(i2c_master_port, conf.mode));
    ESP_ERROR_CHECK(i2c_param_config(i2c_master_port, &conf));
    return ESP_OK;
}

static esp_err_t i2c_master_arduino_write(i2c_port_t i2c_num, uint8_t *data, size_t data_len)
{
    int ret;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, ARDUINO_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
    //i2c_master_write_byte(cmd, reg_address, ACK_CHECK_EN);
    i2c_master_write(cmd, data, data_len, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    return ret;
}

void i2c_task(void *arg)
{
    i2c_master_init();
    char buf[64];
    while (1)
    {
        memset(buf, 0, sizeof(buf));
        if(xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE)
        {
            sprintf(buf, "%d\r", smart_pan_config.current_pwm);
            xSemaphoreGive(xSemaphore);
        }

        ESP_ERROR_CHECK(i2c_master_arduino_write(I2C_MASTER_NUM, (uint8_t*)buf, strlen(buf)));
        vTaskDelay(1000/portTICK_RATE_MS);
    }
    
}

void control_task(void *arg)
{
    #define DELAY_MS 1000
    #define MIN_TO_SEC(min) min*60
    long time_ms = 0;
    float current_temperature = 0;
    float set_temperature = 0;
    int pwm = 0;
    work_mode_t work_mode = 0;
    size_t work_mode_iter = 0;
    bool wait_mode = 0;
    typedef struct
    {
        uint8_t temperature;
        uint32_t wait_time_sec;        
    } work_mode_cfg_t;
    
    work_mode_cfg_t milk[] = {
        {30, MIN_TO_SEC(10)},
        {50, MIN_TO_SEC(10)},
        {80, MIN_TO_SEC(10)},
        {90, MIN_TO_SEC(10)},
        {100, MIN_TO_SEC(10)},
    };

    
    while (1)
    {
        if(xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE)
        {
            current_temperature = smart_pan_config.current_temperature;
            set_temperature = smart_pan_config.set_temperature;
            work_mode = smart_pan_config.work_mode;
            xSemaphoreGive(xSemaphore);
        }

        if(work_mode == MILK)
        {
            time_ms++;
            work_mode = TEMPERATURE_MAINTENANCE;
            
            if(work_mode_iter < sizeof milk)
            {
                if(wait_mode)
                {
                    if((time_ms/1000) >= milk[work_mode_iter].wait_time_sec)
                    {
                        work_mode_iter++;
                        wait_mode = 0;
                    }
                }
                else if(current_temperature >= milk[work_mode_iter].temperature)
                {
                    time_ms = 0;
                    wait_mode = 1;
                }

                set_temperature = milk[work_mode_iter].temperature;
            }
            else
            {
                smart_pan_config.set_temperature = 0;
                work_mode = NO_SET;
                smart_pan_config.work_mode = work_mode;
            }
        }

        if(work_mode == TEMPERATURE_MAINTENANCE)
        {
            pwm = pid_compute(current_temperature, set_temperature, DELAY_MS/1000); 
        }
        else if(work_mode == NO_SET)
        {
            time_ms = 0;
            pwm = 0;
        }

        if(xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE)
        {
            smart_pan_config.current_pwm = pwm;
            xSemaphoreGive(xSemaphore);
        }

        vTaskDelay(DELAY_MS/portTICK_RATE_MS);
    }
}