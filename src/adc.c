#include "adc.h"
#include "esp_log.h"

static const char *TAG = "adc_calibration";

// TODO: output data as pointer to struct in params
adc_oneshot_unit_t* adc_oneshot_unit_init(adc_unit_t adc_unit, adc_atten_t atten) {
    // allocate memory for adc_oneshot_unit
    adc_oneshot_unit_t* adc_oneshot_unit = calloc(1, sizeof(adc_oneshot_unit_t));
    if (adc_oneshot_unit == NULL) {
        ESP_LOGE(TAG, "Error allocating memory for adc_oneshot_unit");
        return NULL;
    }

    // initialize adc_oneshot_unit
    adc_oneshot_unit->adc_unit = adc_unit;
    adc_oneshot_unit->adc_handle = calloc(1, sizeof(adc_oneshot_unit_handle_t));
    if (adc_oneshot_unit->adc_handle == NULL) {
        ESP_LOGE(TAG, "Error allocating memory for adc_oneshot_unit_handle_t");
        return NULL;
    }
    adc_oneshot_unit->adc_chan_config = calloc(1, sizeof(adc_oneshot_chan_cfg_t));
    if (adc_oneshot_unit->adc_chan_config == NULL) {
        ESP_LOGE(TAG, "Error allocating memory for adc_oneshot_chan_cfg_t");
        return NULL;
    }
    adc_oneshot_unit->adc_chan_config->bitwidth = ADC_BITWIDTH_DEFAULT;
    adc_oneshot_unit->adc_chan_config->atten = atten;

    // initialize a new adc_oneshot_unit
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = adc_unit,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, adc_oneshot_unit->adc_handle));

    return adc_oneshot_unit;
}

void adc_oneshot_unit_free(adc_oneshot_unit_t* adc_oneshot_unit) {
    free(adc_oneshot_unit->adc_handle);
    free(adc_oneshot_unit->adc_chan_config);
    free(adc_oneshot_unit);
}

bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle) {
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Line Fitting");
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }

    *out_handle = handle;
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Calibration Success");
    } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
        ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
    } else {
        ESP_LOGE(TAG, "Invalid arg or no memory");
    }

    return calibrated;
}

void adc_calibration_deinit(adc_cali_handle_t handle) {
    ESP_LOGI(TAG, "deregister %s calibration scheme", "Line Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_line_fitting(handle));
}