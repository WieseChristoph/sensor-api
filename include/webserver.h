#include "i2cdev.h"
#include "driver/gpio.h"
#include "adc.h"
#include "esp_http_server.h"

struct webserver_sensor_data {
    i2c_dev_t *am2320_i2c_dev;
    adc_oneshot_unit_t *adc_oneshot_unit;
    adc_channel_t photo_adc_channel;
    adc_cali_handle_t *adc_cali_photo_handle;
    bool photo_is_calibrated;
    u_int32_t series_resistor;
    float input_voltage;
    adc_channel_t heartrate_adc_channel;
    u_int16_t heartbeat_threshold;
    u_int8_t required_heartbeats;
    u_int8_t heartbeat_timeout_s;
    gpio_num_t heatbeat_led_gpio;
} typedef webserver_sensor_data_t;

/**
 * Starts the webserver on the specified port
 * 
 * @param port The port number to start the webserver on
 * @param webserver_sensor_data A pointer to the webserver sensor data struct
 * 
 * @return The handle to the started webserver
 */
httpd_handle_t start_webserver(u_int16_t port, webserver_sensor_data_t *webserver_sensor_data);

/**
 * Stops the HTTP server
 *
 * @param server The handle of the HTTP server to stop
 */
void stop_webserver(httpd_handle_t server);