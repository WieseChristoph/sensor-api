#include "esp_http_server.h"

struct webserver_sensor_data {
    SemaphoreHandle_t semaphore;
    float temperature;
    float humidity;
    u_int32_t illuminance;
    u_int8_t heartrate;
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