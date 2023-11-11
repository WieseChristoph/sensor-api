#include "i2cdev.h"
#include "esp_adc/adc_oneshot.h"

typedef struct am2320_data
{
    float temperature;
    float humidity;
} am2320_data_t;

/**
 * Get temperature and humidity data from an AM2320 sensor
 * 
 * @param dev Pointer to the I2C device
 * @param out_data Pointer for returning the AM2320 data
 */
void get_am2320_data(i2c_dev_t* dev, am2320_data_t* out_data);

/**
 * Calculates the heart rate in beats per minute (BPM) using an ADC to measure the heart rate pulse
 * 
 * @param unit_handle Pointer to the ADC oneshot unit handle
 * @param channel ADC channel to read from
 * @param heartbeat_threshold The threshold value for detecting a heart rate pulse
 * @param required_beats The number of heart rate pulses required to calculate the BPM
 * @param timeout_ms The timeout in milliseconds for detecting a single heart rate pulse
 * @param led_gpio The GPIO pin to use for the heartbeat LED (use GPIO_NUM_NC for no LED)
 * 
 * @return The heart rate in BPM
 */
u_int8_t get_heart_rate(adc_oneshot_unit_handle_t *unit_handle, adc_channel_t channel, u_int16_t heartbeat_threshold, u_int8_t required_beats, u_int32_t timeout_ms, gpio_num_t led_gpio);