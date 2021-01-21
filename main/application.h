#ifndef APPLICATION_H
#define APPLICATION_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "mqtt_client.h"

#define MQTT_URL "mqtt://test.mosquitto.org:1883"
#define PUB_TOPIC "/SMART_PAN/DEVICE"
#define SUB_TOPIC "/SMART_PAN/DEVICE/SET_CONFIG"

#define PID_KP 1
#define PID_KI 1
#define PID_KD 0

#define PID_MAX 255
#define PID_MIN 0

#define I2C_MASTER_SCL_IO           4                /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO           5               /*!< gpio number for I2C master data  */
#define I2C_MASTER_NUM              I2C_NUM_0        /*!< I2C port number for master dev */
#define I2C_MASTER_TX_BUF_DISABLE   0                /*!< I2C master do not need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0                /*!< I2C master do not need buffer */
#define WRITE_BIT                           I2C_MASTER_WRITE /*!< I2C master write */
#define READ_BIT                            I2C_MASTER_READ  /*!< I2C master read */
#define ACK_CHECK_EN                        0x1              /*!< I2C master will check ack from slave*/
#define ARDUINO_ADDR                 8             /*!< slave address for arduino */

static esp_mqtt_client_config_t mqtt_cfg;
static esp_mqtt_client_handle_t mqtt_client;
static SemaphoreHandle_t xSemaphore = NULL;

typedef enum
{
    NO_SET = 0,
    TEMPERATURE_MAINTENANCE,
    MILK,

} work_mode_t;

typedef struct{
    float current_temperature;
    float set_temperature;
    float kp;
    float ki;
    float kd;

    uint8_t current_pwm;
    work_mode_t work_mode;
} smart_pan_t;

static smart_pan_t smart_pan_config = {
    .current_temperature = 0,
    .set_temperature = 0,
    .current_pwm = 0,
    .work_mode = NO_SET,   
};

void application_init();

#endif //APPLICATION_H