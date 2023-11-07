#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

#define WIFI_DISCONNECT_LED_GPIO GPIO_NUM_14
#define HEARTBEAT_LED_GPIO GPIO_NUM_13
#define RESET_BUTTON_GPIO GPIO_NUM_25
#define HEARTRATE_SENSOR_GPIO_ADC ADC_CHANNEL_6
#define I2C_SCL_PIN 22
#define I2C_SDA_PIN 21

#define ADC_ATTEN ADC_ATTEN_DB_11
#define HEARTBEAT_THRESHOLD 3000
#define REQUIRED_HEARTBEATS 7
#define HEARTBEAT_TIMEOUT_MS 2000