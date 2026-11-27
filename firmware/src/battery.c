#include "battery.h"
#include "CH59x_common.h"
#include "proto.h"

uint16_t battery_measure_mv(void) {
    ADC_InterBATSampInit();
    signed short calib = ADC_DataCalib_Rough(); /* restores channel and gain itself */
    (void)ADC_ExcutSingleConver(); /* first conversion is discarded, as in WCH HAL/BAT.c */
    uint32_t sum = 0;
    for (int i = 0; i < 8; i++) {
        int v = (int)ADC_ExcutSingleConver() + calib;
        sum += v > 0 ? (uint32_t)v : 0;
    }
    R8_ADC_CFG &= ~RB_ADC_POWER_ON;
    return proto_vbat_mv((uint16_t)(sum / 8));
}
