#include "read_adc/read_adc.h"
#include "adc.h"

static volatile uint16_t adc_dma[ADC_OVERSAMPLE][ADC_CHANNELS];

int adc_init(void) {
    if (hadc1.Init.NbrOfConversion != ADC_CHANNELS) {
        return -1;
    }

    if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_dma,
        ADC_OVERSAMPLE * ADC_CHANNELS) != HAL_OK) {
        return -1;
    }

    return 0;
}

void adc_read(uint16_t out[ADC_CHANNELS]) {
    for (uint32_t ch = 0; ch < ADC_CHANNELS; ch++) {
        uint32_t sum = 0;

        // Calculamos la media
        for (uint32_t i = 0; i < ADC_OVERSAMPLE; i++) {
            sum += adc_dma[i][ch];
        }

        // Si le sumaramos 0, al ser un entero redondeamos
        out[ch] = (uint16_t)(sum / ADC_OVERSAMPLE);
    }
}
