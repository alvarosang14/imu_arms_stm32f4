#ifndef READ_ADC_H
#define READ_ADC_H

#include <stdint.h>

void adc_init();
uint32_t read_data_adc();
uint32_t *read_data_adc_nvic();

extern uint16_t ADC_VAL[4];

#endif // READ_ADC_H