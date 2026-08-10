#include "FreeRTOS.h"
#include "adc.h"
#include "bno055_wrapper/bno055_wrapper.h"
#include "cmsis_os.h"
#include "cmsis_os2.h"
#include "read_adc/read_adc.h"
#include "send_info/send_info.h"
#include "task.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define N_DMA_CH 1U
#define I_DMA_CH 0U

osSemaphoreId_t adc_sem;

/* Definitions for readADC */
osThreadId_t readADCHandle;
const osThreadAttr_t readADC_attributes = {
    .name = "readADC",
    .stack_size = 128 * 4,
    .priority = (osPriority_t)osPriorityBelowNormal,
};
/* Definitions for readI2C1 */
osThreadId_t readI2C1Handle;
const osThreadAttr_t readI2C1_attributes = {
    .name = "readI2C1",
    .stack_size = 128 * 4,
    .priority = (osPriority_t)osPriorityNormal,
};
/* Definitions for readI2C2 */
osThreadId_t readI2C2Handle;
const osThreadAttr_t readI2C2_attributes = {
    .name = "readI2C2",
    .stack_size = 128 * 4,
    .priority = (osPriority_t)osPriorityNormal,
};
/* Definitions for pushSerial */
osThreadId_t pushSerialHandle;
const osThreadAttr_t pushSerial_attributes = {
    .name = "pushSerial",
    .stack_size = 128 * 4,
    .priority = (osPriority_t)osPriorityLow,
};

// ============ Struct ============
struct message {
    uint16_t adc_value[4];
    struct bno055_accel_t accel_out;
    struct bno055_gyro_t gyro_out;
    struct bno055_euler_t euler;
};

struct message msg_raw;

static void StartReadADC(void *argument) {
    // PA1
    adc_init();
    for (;;) {
        memcpy(msg_raw.adc_value, ADC_VAL, sizeof(ADC_VAL));
        osDelay(100);
    }
}

static void StartReadI2C1(void *argument) {
    // SDA=azul=PB7 SCL=amarillo=PB6

    s32 err;
    s8 err2;
    for (;;) {
        err = bno055_read(&msg_raw.accel_out, &msg_raw.gyro_out);
        err2 = bno055_read_euler(&msg_raw.euler);
        if (err != BNO055_SUCCESS || err2 != BNO055_SUCCESS) {
            send_info("Error in I2C1");
            osThreadExit();
        }
        osDelay(50);
    }
}

static void StartReadI2C2(void *argument) {
    for (;;) {
        // HAL_I2C_Master_Receive(&hi2c2, dev_addr, buf, len, HAL_MAX_DELAY);
        // todo
        osDelay(50);
    }
}

static void StartPushSerial(void *argument) {
    char msg[128];
    for (;;) {
        snprintf(msg, sizeof(msg),
                 "{\"adc\":%lu,%lu,%lu,%lu"
                 "\"euler\":{\"h\":%d, \"p\":%d, \"r\":%d},"
                 "\"accel\":{\"x\":%d,\"y\":%d,\"z\":%d},"
                 "\"gyro\":{\"x\":%d,\"y\":%d,\"z\":%d}}\r\n",
                 msg_raw.adc_value[0], msg_raw.adc_value[1],
                 msg_raw.adc_value[2], msg_raw.adc_value[3], msg_raw.euler.h,
                 msg_raw.euler.p, msg_raw.euler.r, msg_raw.accel_out.x,
                 msg_raw.accel_out.y, msg_raw.accel_out.z, msg_raw.gyro_out.x,
                 msg_raw.gyro_out.y, msg_raw.gyro_out.z);

        send_info(msg);

        osDelay(1000);
    }
}

static void thread_create(void) {
    /* Create the thread(s) */
    /* creation of readADC */
    readADCHandle = osThreadNew(StartReadADC, NULL, &readADC_attributes);

    /* creation of readI2C1 */
    readI2C1Handle = osThreadNew(StartReadI2C1, NULL, &readI2C1_attributes);

    /* creation of readI2C2 */
    readI2C2Handle = osThreadNew(StartReadI2C2, NULL, &readI2C2_attributes);

    /* creation of pushSerial */
    pushSerialHandle =
        osThreadNew(StartPushSerial, NULL, &pushSerial_attributes);
}

/**
 * @brief  FreeRTOS initialization
 * @param  None
 * @retval None
 */
void freertos_init(void) {
    /* add mutexes, ... */

    /* start timers, add new ones, ... */
    /* add queues, ... */

    /* Init scheduler */
    osKernelInitialize(); /* Call init function for freertos objects (in
                             cmsis_os2.c) */

    /* add semaphores, ... */
    // adc_sem = osSemaphoreNew(N_DMA_CH, I_DMA_CH, NULL);

    /* Init thread */
    thread_create();

    /* Start scheduler */
    osKernelStart();
}