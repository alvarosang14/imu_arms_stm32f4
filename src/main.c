#include "main.h"

#include "adc.h"
#include "bno055_wrapper/bno055_wrapper.h"
#include "free_rtos/free_rtos.h"
#include "gpio.h"
#include "i2c.h"
#include "usb_device.h"
#include "utils/utils.h"

#include <stdint.h>

/**
 * @brief System Clock Configuration
 * @retval None
 */
// Extern not is neccesary
extern void SystemClock_Config(void);
extern void MX_FREERTOS_Init(void);

static void init_stm32() {
    /* Reset of all peripherals, Initializes the Flash interface and the
     * Systick. */
    HAL_Init();

    /* Configure the system clock */
    SystemClock_Config();

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_ADC1_Init();
    MX_I2C1_Init();
    MX_USB_DEVICE_Init();

    bno055_init_a();
    freertos_init();
}

static void stop_stm32() { bno055_stop(); }

int main(void) {
    init_stm32();

    /* Infinite loop */
    int loop = 1;
    while (loop) {
        sleep(500);
    }
    stop_stm32();
}
