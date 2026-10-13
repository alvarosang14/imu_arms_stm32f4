#include "main.h"

#include "adc.h"
#include "app_config.h"
#include "dma.h"
#include "free_rtos/free_rtos.h"
#include "gpio.h"
#include "i2c.h"
#include "usb_device.h"
#include "utils/utils.h"

extern void SystemClock_Config(void);

static void init_stm32() {
    /* Reset of all peripherals, Initializes the Flash interface and the
     * Systick. */
    HAL_Init();

    /* Configure the system clock */
    SystemClock_Config();
    utils_init();

    /* Initialize all configured peripherals */
    MX_DMA_Init();
    MX_GPIO_Init();
    MX_ADC1_Init();
    MX_I2C1_Init();
#if APP_USE_I2C2
    MX_I2C2_Init();
#endif
    MX_USB_DEVICE_Init();
}

int main(void) {
    init_stm32();

    freertos_init();

    for (;;) {
    }
}
