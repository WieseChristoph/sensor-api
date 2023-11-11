#include "webserver.h"
#include "esp_event.h"
#include "esp_log.h"
#include "heartrate.h"
#include "am2320.h"

const static char *TAG = "webserver";

static esp_err_t get_illuminance_handler(httpd_req_t *req) {
    webserver_sensor_data_t *webserver_sensor_data = (webserver_sensor_data_t *) req->user_ctx;

    u_int32_t illuminance = 0;
    tsl2561_read_lux(webserver_sensor_data->tsl2561_dev, &illuminance);

    char *resp;
    asprintf(&resp, "%ld Lux", illuminance);

    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t get_temp_handler(httpd_req_t *req) {
    webserver_sensor_data_t *webserver_sensor_data = (webserver_sensor_data_t *) req->user_ctx;

    float temperature = 0;
    esp_err_t res = am2320_get_rht(webserver_sensor_data->am2320_i2c_dev, &temperature, NULL);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Error reading AM2320 data: %d (%s)", res, esp_err_to_name(res));
    }

    char *resp;
    asprintf(&resp, "%.1f°C", temperature);

    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t get_hum_handler(httpd_req_t *req) {
    webserver_sensor_data_t *webserver_sensor_data = (webserver_sensor_data_t *) req->user_ctx;

    float humidity = 0;
    esp_err_t res = am2320_get_rht(webserver_sensor_data->am2320_i2c_dev, NULL, &humidity);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Error reading AM2320 data: %d (%s)", res, esp_err_to_name(res));
    }

    char *resp;
    asprintf(&resp, "%.1f%%", humidity);

    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t get_heartrate_handler(httpd_req_t *req) {
    webserver_sensor_data_t *webserver_sensor_data = (webserver_sensor_data_t *) req->user_ctx;

    u_int8_t heartrate = get_heart_rate(
        &webserver_sensor_data->adc_oneshot_unit->adc_handle, 
        webserver_sensor_data->heartrate_adc_channel, 
        webserver_sensor_data->heartbeat_threshold, 
        webserver_sensor_data->required_heartbeats, 
        webserver_sensor_data->heartbeat_timeout_ms, 
        webserver_sensor_data->heatbeat_led_gpio
    );

    char *resp;
    asprintf(&resp, "%d BPM", heartrate);

    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t get_metrics_handler(httpd_req_t *req) {
    webserver_sensor_data_t *webserver_sensor_data = (webserver_sensor_data_t *) req->user_ctx;

    u_int32_t illuminance = 0;
    tsl2561_read_lux(webserver_sensor_data->tsl2561_dev, &illuminance);

    float temperature = 0;
    float humidity = 0;
    esp_err_t res = am2320_get_rht(webserver_sensor_data->am2320_i2c_dev, &temperature, &humidity);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Error reading AM2320 data: %d (%s)", res, esp_err_to_name(res));
    }

    u_int8_t heartrate = get_heart_rate(
        &webserver_sensor_data->adc_oneshot_unit->adc_handle, 
        webserver_sensor_data->heartrate_adc_channel, 
        webserver_sensor_data->heartbeat_threshold, 
        webserver_sensor_data->required_heartbeats, 
        webserver_sensor_data->heartbeat_timeout_ms, 
        webserver_sensor_data->heatbeat_led_gpio
    );

    char *resp;
    asprintf(
        &resp,
        "# HELP illuminance_lux Light intensity in lux\n"
        "# TYPE illuminance_lux gauge\n"
        "illuminance_lux %ld\n\n"
        "# HELP temperature_celsius Temperature in celsius\n"
        "# TYPE temperature_celsius gauge\n"
        "temperature_celsius %.1f\n\n"
        "# HELP humidity_relative Relative humidity in percent\n"
        "# TYPE humidity_relative gauge\n"
        "humidity_relative %.1f\n\n"
        "# HELP heartrate_bpm Heart rate in beats per minute\n"
        "# TYPE heartrate_bpm gauge\n"
        "heartrate_bpm %d"
        , illuminance, temperature, humidity, heartrate
    );

    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

httpd_handle_t start_webserver(u_int16_t port, webserver_sensor_data_t *webserver_sensor_data) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = port;

    httpd_handle_t server = NULL;

    httpd_uri_t uris[] = {
        {
            .uri = "/illuminance",
            .method = HTTP_GET,
            .handler = get_illuminance_handler,
            .user_ctx = webserver_sensor_data
        },
        {
            .uri = "/temp",
            .method = HTTP_GET,
            .handler = get_temp_handler,
            .user_ctx = webserver_sensor_data
        },
        {
            .uri = "/hum",
            .method = HTTP_GET,
             .handler = get_hum_handler,
             .user_ctx = webserver_sensor_data
        },
        {
            .uri = "/heartrate",
            .method = HTTP_GET,
            .handler = get_heartrate_handler,
            .user_ctx = webserver_sensor_data
        },
        {
            .uri = "/metrics",
            .method = HTTP_GET,
            .handler = get_metrics_handler,
            .user_ctx = webserver_sensor_data
        }
    };

    ESP_LOGI(TAG, "Starting webserver on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI(TAG, "Registering URI handlers");
        for (int i = 0; i < sizeof(uris)/sizeof(httpd_uri_t); i++) {
            httpd_register_uri_handler(server, &uris[i]);
        }
    }

    return server;
}

void stop_webserver(httpd_handle_t server) {
    if (server) {
        ESP_LOGI(TAG, "Stopping webserver");
        httpd_stop(server);
    }
}