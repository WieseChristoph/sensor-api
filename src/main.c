#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "am2320.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
#include "sensors.h"
#include "adc.h"
#include "wifi.h"
#include "secrets.h"

const static char *TAG = "sensor_api";

#define RED_GPIO GPIO_NUM_14
#define GREEN_GPIO GPIO_NUM_13
#define RESET_GPIO GPIO_NUM_25
#define PHOTO_GPIO_ADC ADC_CHANNEL_7
#define HEARTRATE_GPIO_ADC ADC_CHANNEL_6
#define I2C_MASTER_SCL 22
#define I2C_MASTER_SDA 21

#define INPUT_VOLTAGE 3.3
#define PHOTO_RESISTOR 10000

#define ADC_ATTEN ADC_ATTEN_DB_11

static QueueHandle_t gpio_evt_queue = NULL;

void IRAM_ATTR gpio_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

void gpio_task(void* arg) {
    uint32_t io_num;
    for (;;) {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            switch (io_num) {
                case RESET_GPIO:
                    ESP_LOGI(TAG, "Resetting...");
                    ESP_ERROR_CHECK(nvs_flash_erase());
                    esp_restart();
                    break;
            }
        }
    }
}

void app_main() {
    // -------------NVS Init---------------//
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_init_sta(WIFI_SSID, WIFI_PASS);

    //-------------GPIO Init---------------//
    gpio_reset_pin(RED_GPIO);
    gpio_set_direction(RED_GPIO, GPIO_MODE_INPUT_OUTPUT);
    gpio_reset_pin(GREEN_GPIO);
    gpio_set_direction(GREEN_GPIO, GPIO_MODE_INPUT_OUTPUT);
    
    gpio_set_direction(RESET_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(RESET_GPIO, GPIO_PULLDOWN_ONLY);
    gpio_set_intr_type(RESET_GPIO, GPIO_INTR_POSEDGE);

    // Create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));

    // Start gpio task
    xTaskCreate(gpio_task, "gpio_task", 2048, NULL, 10, NULL);

    // Install gpio isr service
    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    ESP_ERROR_CHECK(gpio_isr_handler_add(RESET_GPIO, gpio_isr_handler, (void*) RESET_GPIO));

    //-------------AM2320 Init---------------//
    i2c_dev_t am2320_i2c_dev = {0};
    ESP_ERROR_CHECK(i2cdev_init());
    ESP_ERROR_CHECK(am2320_init_desc(&am2320_i2c_dev, 0, I2C_MASTER_SDA, I2C_MASTER_SCL));

    //-------------ADC Init---------------//
    adc_oneshot_unit_t adc_oneshot = {0};
    adc_oneshot_unit_init(ADC_UNIT_1, ADC_ATTEN, &adc_oneshot);

    //-------------ADC Config---------------//
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_oneshot.adc_handle, PHOTO_GPIO_ADC, &adc_oneshot.adc_chan_config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_oneshot.adc_handle, HEARTRATE_GPIO_ADC, &adc_oneshot.adc_chan_config));

    //-------------ADC Calibration Init---------------//
    adc_cali_handle_t adc1_cali_photo_handle = NULL;
    bool is_calibrated_photo = adc_calibration_init(ADC_UNIT_1, PHOTO_GPIO_ADC, ADC_ATTEN, &adc1_cali_photo_handle);

    gpio_set_level(GREEN_GPIO, 1);
    while (1) {
        ESP_LOGI(TAG, "-----------------------------");

        //-------------LED Toggle---------------//
        gpio_set_level(RED_GPIO, gpio_get_level(RED_GPIO) ^ 1);
        gpio_set_level(GREEN_GPIO, gpio_get_level(RED_GPIO) ^ 1);

        // //-------------Photo-Sensor Read---------------//
        photo_data_t photo_data = {0};
        get_photo_data(&adc_oneshot.adc_handle, PHOTO_GPIO_ADC, &adc1_cali_photo_handle, is_calibrated_photo, PHOTO_RESISTOR, INPUT_VOLTAGE, &photo_data);
        ESP_LOGI(TAG, "ADC%d Channel[%d] [Photo] Raw: %d, Voltage: %d mV, Resistence: %f Ohm, Intensity: %f Lux", ADC_UNIT_1 + 1, PHOTO_GPIO_ADC, photo_data.raw, photo_data.voltage, photo_data.resistence, photo_data.lux);

        //-------------AM2320 Read---------------//
        am2320_data_t temp_hum_data = {0};
        get_am2320_data(&am2320_i2c_dev, &temp_hum_data);
        ESP_LOGI(TAG, "Temperature: %.1f°C, Humidity: %.1f%%", temp_hum_data.temperature, temp_hum_data.humidity);

        //-------------Heart-Rate Read---------------//
        u_int8_t bpm = get_heart_rate(&adc_oneshot.adc_handle, HEARTRATE_GPIO_ADC, 3000, 5, 5, GREEN_GPIO);
        ESP_LOGI(TAG, "Heartrate: %d Bpm", bpm);

        //-------------LED Toggle---------------//
        gpio_set_level(RED_GPIO, gpio_get_level(RED_GPIO) ^ 1);
        gpio_set_level(GREEN_GPIO, gpio_get_level(RED_GPIO) ^ 1);

        //-------------1s Delay---------------//
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    // Tear Down ADC
    ESP_ERROR_CHECK(adc_oneshot_del_unit(adc_oneshot.adc_handle));
    if (is_calibrated_photo) {
        adc_calibration_deinit(adc1_cali_photo_handle);
    }
}