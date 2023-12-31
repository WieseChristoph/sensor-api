#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "am2320.h"
#include "tsl2561.h"
#include "hx710b.h"
#include "heartrate.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
#include "adc.h"
#include "wifi.h"
#include "webserver.h"
#include "config.h"
#include "secrets.h"

const static char *TAG = "sensor_api";

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
                case RESET_BUTTON_GPIO:
                    ESP_LOGI(TAG, "Resetting...");
                    ESP_ERROR_CHECK(nvs_flash_erase());
                    esp_restart();
                    break;
            }
        }
    }
}

void app_main() {
    //-------------LED GPIO Init---------------//
    gpio_reset_pin(WIFI_DISCONNECT_LED_GPIO);
    gpio_set_direction(WIFI_DISCONNECT_LED_GPIO, GPIO_MODE_INPUT_OUTPUT);
    gpio_set_level(WIFI_DISCONNECT_LED_GPIO, 1);
    gpio_reset_pin(HEARTBEAT_LED_GPIO);
    gpio_set_direction(HEARTBEAT_LED_GPIO, GPIO_MODE_INPUT_OUTPUT);
    gpio_set_level(HEARTBEAT_LED_GPIO, 0);

    // -------------NVS Init---------------//
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    //-------------WiFi Init---------------//
    gpio_num_t disconnect_led_pin = WIFI_DISCONNECT_LED_GPIO;
    wifi_init_sta(WIFI_SSID, WIFI_PASS, &disconnect_led_pin);
    
    //-------------Reset GPIO Init---------------//
    gpio_set_direction(RESET_BUTTON_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(RESET_BUTTON_GPIO, GPIO_PULLDOWN_ONLY);
    gpio_set_intr_type(RESET_BUTTON_GPIO, GPIO_INTR_POSEDGE);

    // Create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));

    // Start gpio task
    xTaskCreate(gpio_task, "gpio_task", 2048, NULL, 10, NULL);

    // Install gpio isr service
    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    ESP_ERROR_CHECK(gpio_isr_handler_add(RESET_BUTTON_GPIO, gpio_isr_handler, (void*) RESET_BUTTON_GPIO));

    //-------------I2C Init---------------//
    ESP_ERROR_CHECK(i2cdev_init());

    //-------------AM2320 Init---------------//
    i2c_dev_t am2320_i2c_dev = {0};
    ESP_ERROR_CHECK(am2320_init_desc(&am2320_i2c_dev, I2C_NUM_0, I2C_SDA_PIN, I2C_SCL_PIN));

    //-------------TSL2561 Init---------------//
    tsl2561_t tsl2561_dev = {0};
    ESP_ERROR_CHECK(tsl2561_init_desc(&tsl2561_dev, TSL2561_I2C_ADDR_FLOAT, I2C_NUM_0, I2C_SDA_PIN, I2C_SCL_PIN));
    ESP_ERROR_CHECK(tsl2561_init(&tsl2561_dev));

    //-------------HX710B Init---------------//
    // hx710b_t hx710b_dev = {0};
    // hx710b_init(&hx710b_dev, PRESSURE_SENSOR_SCK_PIN, PRESSURE_SENSOR_OUT_PIN, HX710B_GAIN_128);

    // while (1)
    // {
    //     float out = hx710b_read_hpa(&hx710b_dev);
    //     ESP_LOGI(TAG, "Pressure: %f", out);
    //     vTaskDelay(pdMS_TO_TICKS(1000));
    // }

    //-------------ADC Init---------------//
    adc_oneshot_unit_t adc_oneshot = {0};
    adc_oneshot_unit_init(ADC_UNIT_1, ADC_ATTEN, &adc_oneshot);

    //-------------ADC Config---------------//
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_oneshot.adc_handle, HEARTRATE_SENSOR_ADC_CHANNEL, &adc_oneshot.adc_chan_config));

    //-------------Webserver Init---------------//
    webserver_sensor_data_t webserver_sensor_data = {0};
    webserver_sensor_data.semaphore = xSemaphoreCreateMutex();
    if(webserver_sensor_data.semaphore == NULL ) {
        ESP_LOGE(TAG, "Semaphore creation failed!");
        return;
    }
    httpd_handle_t server = start_webserver(80, &webserver_sensor_data);

    // Update sensor data every 5 seconds
    for (;;) {
        // Illuminance
        u_int32_t illuminance = 0;
        tsl2561_read_lux(&tsl2561_dev, &illuminance);

        // Temperature and Humidity
        float temperature = 0;
        float humidity = 0;
        esp_err_t res = am2320_get_rht(&am2320_i2c_dev, &temperature, &humidity);
        if (res != ESP_OK) {
            ESP_LOGE(TAG, "Error reading AM2320 data: %d (%s)", res, esp_err_to_name(res));
        }

        // Heart Rate
        u_int8_t heartrate = get_heart_rate(
            &adc_oneshot.adc_handle, 
            HEARTRATE_SENSOR_ADC_CHANNEL, 
            HEARTBEAT_THRESHOLD, 
            REQUIRED_HEARTBEATS, 
            HEARTBEAT_TIMEOUT_MS, 
            HEARTBEAT_LED_GPIO
        );

        // Update webserver data
        if (xSemaphoreTake(webserver_sensor_data.semaphore, portMAX_DELAY) == pdTRUE) {
            webserver_sensor_data.temperature = temperature;
            webserver_sensor_data.humidity = humidity;
            webserver_sensor_data.illuminance = illuminance;
            webserver_sensor_data.heartrate = heartrate;
            if (xSemaphoreGive(webserver_sensor_data.semaphore) != pdTRUE) {
                ESP_LOGE(TAG, "Could not give semaphore!");
            }
        } else {
            ESP_LOGE(TAG, "Could not take semaphore!");
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }

    // Tear Down Webserver
    stop_webserver(server);

    // Tear Down ADC
    ESP_ERROR_CHECK(adc_oneshot_del_unit(adc_oneshot.adc_handle));
}