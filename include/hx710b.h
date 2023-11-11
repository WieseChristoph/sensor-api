// https://github.com/kurimawxx00/hx710B_pressure_sensor
#ifndef __HX710B_H__
#define __HX710B_H__

#include <stdint.h>

#define HX710B_RES 2.98023e-7

typedef enum bit_order {
    LSBFIRST = 0,
    MSBFIRST = 1
} bit_order_t;

typedef enum hx710b_gain {
    HX710B_GAIN_128 = 1, // Channel A
    HX710B_GAIN_32 = 2, // Channel A
    HX710B_GAIN_64 = 3 // Channel B
} hx710b_gain_t;

typedef struct hx710b {
    unsigned long sck;
    unsigned long dout;
    hx710b_gain_t gain;
} hx710b_t;

void hx710b_init(hx710b_t *sensor, unsigned int sck, unsigned int dout, hx710b_gain_t gain);

int hx710b_is_ready(hx710b_t *sensor);

void hx710b_wait_ready(hx710b_t *sensor, unsigned int delay_ms);

long hx710b_read(hx710b_t *sensor);

long hx710b_read_average(hx710b_t *sensor, uint8_t times);

float hx710b_read_hpa(hx710b_t *sensor);

void hx710b_power_down(hx710b_t *sensor);

void hx710b_power_up(hx710b_t *sensor);

#endif