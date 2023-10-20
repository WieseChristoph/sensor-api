#include "esp_adc/adc_oneshot.h"

typedef struct adc_oneshot_unit {
    adc_unit_t adc_unit;
    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_chan_cfg_t adc_chan_config;
} adc_oneshot_unit_t;

/**
 * Initializes an ADC oneshot unit with the specified ADC unit and attenuation
 * 
 * @param unit The ADC unit to use
 * @param atten The attenuation to use
 * @param out_unit Pointer for returning the adc_oneshot_unit
 */
void adc_oneshot_unit_init(adc_unit_t unit, adc_atten_t atten, adc_oneshot_unit_t *out_unit);

/**
 * Initializes ADC calibration for a given unit, channel, and attenuation
 *
 * @param unit The ADC unit to calibrate
 * @param channel The ADC channel to calibrate
 * @param atten The attenuation level to use for calibration
 * @param out_handle Pointer for returning the calibration handle
 * 
 * @return true if calibration was successful, false otherwise
 */
bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle);

/**
 * Deinitializes ADC calibration
 *
 * @param cali_handle The calibration handle to deinitialize
 */
void adc_calibration_deinit(adc_cali_handle_t cali_handle);