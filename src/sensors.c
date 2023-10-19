#include "sensors.h"
#include "driver/gpio.h"
#include "am2320.h"
#include "esp_timer.h"
#include "esp_log.h"

static const char* TAG = "sensors";

// TODO: output data as pointer to struct in params
am2320_data_t* get_am2320_data(i2c_dev_t* dev) {
    am2320_data_t* data = calloc(1, sizeof(am2320_data_t));
    if (data == NULL) {
        ESP_LOGE(TAG, "Error allocating memory for am2320 data");
        return NULL;
    }

    data->temperature = 0;
    data->humidity = 0;

    esp_err_t res = am2320_get_rht(dev, &data->temperature, &data->humidity);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Error reading data: %d (%s)", res, esp_err_to_name(res));
    }

    return data;
}

// TODO: output data as pointer to struct in params
photo_data_t* get_photo_data(adc_oneshot_unit_handle_t* adc_handle, adc_channel_t channel, adc_cali_handle_t* adc_cali_handle, bool do_calibration_photo, u_int32_t photo_resistor, float input_voltage) {
    photo_data_t* data = calloc(1, sizeof(photo_data_t));
    if (data == NULL) {
        ESP_LOGE(TAG, "Error allocating memory for photo data");
        return NULL;
    }

    data->raw = 0;
    data->voltage = 0;
    data->resistence = 0;
    data->lux = 0;

    ESP_ERROR_CHECK(adc_oneshot_read(*adc_handle, channel, &data->raw));

    if (do_calibration_photo) {
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(*adc_cali_handle, data->raw, &data->voltage));
        float voltage_v = data->voltage / 1000.0;
        data->resistence = (photo_resistor * (input_voltage - voltage_v))/voltage_v;
        data->lux = 500 / (data->resistence / 1000);
    }

    return data;
}

u_int8_t get_heart_rate(adc_oneshot_unit_handle_t *adc_handle, adc_channel_t adc_channel, u_int16_t heartbeat_threshold, u_int8_t required_beats, u_int8_t timeout_s, gpio_num_t led_gpio) {
    int heartrate_raw = 0;
    int64_t start_time = 0;
    bool pulse_started = false;
    int beats_detected = 0;

    while (beats_detected < required_beats) {
        ESP_ERROR_CHECK(adc_oneshot_read(*adc_handle, adc_channel, &heartrate_raw));
        
        if (!pulse_started && heartrate_raw >= heartbeat_threshold) {
            if (led_gpio != GPIO_NUM_NC) {
                gpio_set_level(led_gpio, 1);
            }
            if (start_time == 0) {
                start_time = esp_timer_get_time();
            }
            pulse_started = true;
        } else if (pulse_started && heartrate_raw < heartbeat_threshold) {
            if (led_gpio != GPIO_NUM_NC) {
                gpio_set_level(led_gpio, 0);
            }
            pulse_started = false;
            beats_detected++;
        }

        if (start_time != 0 && esp_timer_get_time() - start_time > timeout_s * 1000000) {
            if (led_gpio != GPIO_NUM_NC) {
                gpio_set_level(led_gpio, 0);
            }
            return 0;
        }

        // block the minimum amount of time between samples
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    int64_t duration = esp_timer_get_time() - start_time;
    u_int8_t bpm = (required_beats * 60000000) / duration;

    return bpm;
}