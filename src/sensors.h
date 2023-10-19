#include "i2cdev.h"
#include "esp_adc/adc_oneshot.h"

typedef struct am2320_data
{
    float temperature;
    float humidity;
} am2320_data_t;

typedef struct photo_data
{
    int raw;
    int voltage;
    float resistence;
    float lux;
} photo_data_t;

/**
 * Get temperature and humidity data from an AM2320 sensor
 * 
 * @param dev Pointer to the I2C device
 * 
 * @return Pointer to AM2320 data struct
 */
am2320_data_t *get_am2320_data(i2c_dev_t *dev);

/**
 * Get photo data from an ADC channel
 * 
 * @param adc_handle ADC oneshot unit handle
 * @param adc_channel ADC channel to read from
 * @param adc_cali_handle ADC calibration handle
 * @param is_calibrated Whether calibration for conversion to voltage has been done
 * @param series_resistor Resistance of the series resistor in Ohms
 * @param input_voltage Input voltage in volts
 * 
 * @return Pointer to photo data struct
 */
photo_data_t *get_photo_data(adc_oneshot_unit_handle_t *adc_handle, adc_channel_t adc_channel, adc_cali_handle_t *adc_cali_handle, bool is_calibrated, u_int32_t series_resistor, float input_voltage);

/**
 * Calculates the heart rate in beats per minute (BPM) using an ADC to measure the heart rate pulse
 * 
 * @param adc_handle Pointer to the ADC oneshot unit handle
 * @param adc_channel ADC channel to read from
 * @param heartbeat_threshold The threshold value for detecting a heart rate pulse
 * @param required_beats The number of heart rate pulses required to calculate the BPM
 * @param timeout_s The timeout in seconds for the heart rate calculation
 * @param led_gpio The GPIO pin to use for the heartbeat LED (use GPIO_NUM_NC for no LED)
 * 
 * @return The heart rate in BPM
 */
u_int8_t get_heart_rate(adc_oneshot_unit_handle_t *adc_handle, adc_channel_t adc_channel, u_int16_t heartbeat_threshold, u_int8_t required_beats, u_int8_t timeout_s, gpio_num_t led_gpio);