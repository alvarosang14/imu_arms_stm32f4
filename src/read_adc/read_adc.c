#include "read_adc/read_adc.h"
#include "adc.h"
#include <stdint.h>

uint16_t ADC_VAL[4];

// using normal read
uint32_t read_data_adc() {
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 100);
    uint32_t adc_value = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);

    return adc_value;
}

void adc_init() { HAL_ADC_Start_DMA(&hadc1, (uint32_t *)ADC_VAL, 4); }

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {}