#include "adc.h"

#include "read_adc/read_adc.h"

uint32_t ADC_VAL;

// using normal read
uint32_t read_data_adc() {
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 100);
    uint32_t adc_value = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);

    return adc_value;
}

void adc_init() { HAL_ADC_Start_DMA(&hadc1, (uint32_t *)ADC_VAL, 1); }

// using dma
uint32_t read_data_adc_nvic() { return ADC_VAL; }

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
    if (hadc->Instance == ADC1) {
        // todo poner semaforo en true
    }
}