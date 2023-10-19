#include "esp_adc/adc_oneshot.h"

typedef struct adc_oneshot_unit {
    adc_unit_t adc_unit;
    adc_oneshot_unit_handle_t* adc_handle;
    adc_oneshot_chan_cfg_t* adc_chan_config;
} adc_oneshot_unit_t;


/**
 * Initializes an ADC oneshot unit with the specified ADC unit and attenuation
 * 
 * @param adc_unit The ADC unit to use
 * @param atten The attenuation to use
 * 
 * @return Pointer to ADC oneshot unit struct
 */
adc_oneshot_unit_t *adc_oneshot_unit_init(adc_unit_t adc_unit, adc_atten_t atten);

/**
 * @brief Frees the resources used by an ADC oneshot unit.
 *
 * @param adc_oneshot_unit Pointer to the ADC oneshot unit to free.
 */
void adc_oneshot_unit_free(adc_oneshot_unit_t *adc_oneshot_unit);


/**
 * Initializes ADC calibration for a given unit, channel, and attenuation.
 *
 * @param unit The ADC unit to calibrate.
 * @param channel The ADC channel to calibrate.
 * @param atten The attenuation level to use for calibration.
 * @param out_handle Pointer for returning the calibration handle.
 * 
 * @return true if calibration was successful, false otherwise.
 */
bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle);

/**
 * Deinitializes ADC calibration.
 *
 * @param handle The calibration handle to deinitialize.
 */
void adc_calibration_deinit(adc_cali_handle_t handle);