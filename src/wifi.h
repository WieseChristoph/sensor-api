#include "esp_event.h"

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

/**
 * Initializes the WiFi station mode with the provided SSID and password.
 * 
 * @param ssid The SSID of the WiFi network to connect to.
 * @param pass The password for the WiFi network.
 */
void wifi_init_sta(char *ssid, char *pass);