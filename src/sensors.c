#include "sensors.h"
#include "driver/gpio.h"
#include "am2320.h"
#include "esp_timer.h"
#include "esp_log.h"

static const char* TAG = "sensors";

void get_am2320_data(i2c_dev_t* dev, am2320_data_t* out_data) {
    esp_err_t res = am2320_get_rht(dev, &out_data->temperature, &out_data->humidity);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Error reading data: %d (%s)", res, esp_err_to_name(res));
    }
}

void get_photo_data(adc_oneshot_unit_handle_t* unit_handle, adc_channel_t channel, adc_cali_handle_t* cali_handle, bool is_calibrated, u_int32_t series_resistor, float input_voltage, photo_data_t *out_data) {
    ESP_ERROR_CHECK(adc_oneshot_read(*unit_handle, channel, &out_data->raw));

    if (is_calibrated) {
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(*cali_handle, out_data->raw, &out_data->voltage));
        float voltage_v = out_data->voltage / 1000.0;
        out_data->resistence = (series_resistor * (input_voltage - voltage_v))/voltage_v;
        out_data->lux = 500 / (out_data->resistence / 1000);
    }
}

u_int8_t get_heart_rate(adc_oneshot_unit_handle_t *unit_handle, adc_channel_t channel, u_int16_t heartbeat_threshold, u_int8_t required_beats, u_int32_t timeout_ms, gpio_num_t led_gpio) {
    int heartrate_raw = 0;
    int64_t timeout_time = esp_timer_get_time();
    int64_t start_time = 0;
    bool pulse_started = false;
    int beats_detected = 0;

    while (beats_detected < required_beats) {
        ESP_ERROR_CHECK(adc_oneshot_read(*unit_handle, channel, &heartrate_raw));
        
        if (!pulse_started && heartrate_raw >= heartbeat_threshold) {
            // Heart beat pulse start
            if (led_gpio != GPIO_NUM_NC) {
                gpio_set_level(led_gpio, 1);
            }
            if (start_time == 0) {
                start_time = esp_timer_get_time();
            }
            pulse_started = true;
            timeout_time = esp_timer_get_time();
        } else if (pulse_started && heartrate_raw < heartbeat_threshold) {
            // Heart beat pulse end
            if (led_gpio != GPIO_NUM_NC) {
                gpio_set_level(led_gpio, 0);
            }
            pulse_started = false;
            beats_detected++;
        }

        // Check for timeout
        if (esp_timer_get_time() - timeout_time > timeout_ms * 1000) {
            if (led_gpio != GPIO_NUM_NC) {
                gpio_set_level(led_gpio, 0);
            }
            ESP_LOGI(TAG, "Heartbeat timeout");
            return 0;
        }

        // Block the minimum amount of time between samples
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    int64_t duration = esp_timer_get_time() - start_time;
    u_int8_t bpm = (required_beats * 60000000) / duration;

    return bpm;
}