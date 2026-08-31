// bno055_wrapper.h
#ifndef BNO055_WRAPPER_H
#define BNO055_WRAPPER_H

#include <stdint.h>

#include "BNO055/bno055.h"

s32 bno055_init_a(void);

void struct_init(void);

s32 bno055_read(struct bno055_accel_t *accel_out,
                struct bno055_gyro_t *gyro_out);

s32 bno055_convert_double(struct bno055_accel_double_t *accel_out,
                          struct bno055_gyro_double_t *gyro_out);

BNO055_RETURN_FUNCTION_TYPE bno055_read_euler(struct bno055_euler_t *euler);

s32 bno055_stop(void);

// Codigo HAL_I2C_ErrorCode (I2C_FLAG_AF/BERR/ARLO/OVR...) del ultimo fallo
// I2C, o 0xFFFFFFFF si el fallo fue un timeout de nuestro semaforo (ningun
// callback HAL llego a ejecutarse).
uint32_t bno055_last_i2c_error(void);

#endif
