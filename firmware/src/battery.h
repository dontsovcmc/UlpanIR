/* Battery voltage through the internal ADC channel (VDD33 = battery after Q1). */
#ifndef BATTERY_H
#define BATTERY_H
#include <stdint.h>
uint16_t battery_measure_mv(void);
#endif
