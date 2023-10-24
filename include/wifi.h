#include "driver/gpio.h"

/**
 * Initializes the WiFi station mode with the provided SSID and password
 *
 * @param ssid The SSID of the WiFi network to connect to
 * @param pass The password for the WiFi network
 * @param disconnect_led_pin Pointer to a GPIO pin number of an LED that will be turned on when the WIFI disconnects (use GPIO_NUM_NC for no LED)
 */
void wifi_init_sta(char *ssid, char *pass, gpio_num_t *disconnect_led_pin);