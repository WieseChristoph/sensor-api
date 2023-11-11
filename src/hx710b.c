#include "hx710b.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <rom/ets_sys.h>

uint8_t shiftInSlow(uint8_t dataPin, uint8_t clockPin, bit_order_t bitOrder) {
    uint8_t value = 0;
    uint8_t i;

    for(i = 0; i < 8; ++i) {
        gpio_set_level(clockPin, 1);
        ets_delay_us(1);
        if(bitOrder == LSBFIRST)
            value |= gpio_get_level(dataPin) << i;
        else
            value |= gpio_get_level(dataPin) << (7 - i);
        gpio_set_level(clockPin, 0);
        ets_delay_us(1);
    }
    return value;
}

void hx710b_init(hx710b_t *sensor, unsigned int sck, unsigned int dout, hx710b_gain_t gain) {
    sensor->sck = sck;
    sensor->dout = dout;
    sensor->gain = gain;

    gpio_reset_pin(sensor->sck);
    gpio_set_direction(sensor->sck, GPIO_MODE_OUTPUT);
    
    gpio_reset_pin(sensor->dout);
    gpio_set_direction(sensor->dout, GPIO_MODE_INPUT);
    gpio_set_pull_mode(sensor->dout, GPIO_PULLUP_ONLY);
}

int hx710b_is_ready(hx710b_t *sensor) {
    return gpio_get_level(sensor->dout) == 0;
}

void hx710b_wait_ready(hx710b_t *sensor, unsigned int delay_ms) {
    while (!hx710b_is_ready(sensor)) {
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}

long hx710b_read(hx710b_t *sensor) {
    hx710b_wait_ready(sensor, 10);

    unsigned long value = 0;
	uint8_t data[3] = { 0 };
	uint8_t filler = 0x00;

    // Begin of critical section.
	// Critical sections are used as a valid protection method
	// against simultaneous access in vanilla FreeRTOS.
	// Disable the scheduler and call portDISABLE_INTERRUPTS. This prevents
	// context switches and servicing of ISRs during a critical section.
	portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
	portENTER_CRITICAL(&mux);

    // Pulse the clock pin 24 times to read the data.
    for (u_int8_t i = 3; i--;) {
        data[i] = shiftInSlow(sensor->dout, sensor->sck, MSBFIRST);
    }

    // Config gain + channel for next read
    for (size_t i = 0; i <= sensor->gain; i++) {
        gpio_set_level(sensor->sck, 1);
        ets_delay_us(1);
        gpio_set_level(sensor->sck, 0);
        ets_delay_us(1);
    }

    // End of critical section.
	portEXIT_CRITICAL(&mux);

    // Replicate the most significant bit to pad out a 32-bit signed integer
	if (data[2] & 0x80) {
		filler = 0xFF;
	} else {
		filler = 0x00;
	}

	// Construct a 32-bit signed integer
	value = ( (unsigned long)filler << 24
			| (unsigned long)data[2] << 16
			| (unsigned long)data[1] << 8
			| (unsigned long)data[0] );

	return (long)(value);
}

long hx710b_read_average(hx710b_t *sensor, uint8_t times) {
    long sum = 0;
    for (uint8_t i = 0; i < times; i++) {
        sum += hx710b_read(sensor);
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    return sum / times;
}

// TODO: Fix this shit
float hx710b_read_hpa(hx710b_t *sensor) {
    float value = (((hx710b_read_average(sensor, 10) * HX710B_RES) - 40) * 760) / 101.325;
    return value;
}

void hx710b_power_down(hx710b_t *sensor){
    gpio_set_level(sensor->sck, 0);
    gpio_set_level(sensor->sck, 1);
}

void hx710b_power_up(hx710b_t *sensor){
    gpio_set_level(sensor->sck, 0);
}