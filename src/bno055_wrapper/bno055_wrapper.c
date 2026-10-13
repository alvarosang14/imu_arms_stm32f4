#include "bno055_wrapper/bno055_wrapper.h"
#include "BNO055/bno055.h"
#include "cmsis_os.h"
#include "i2c.h"

/*----------------------------------------------------------------------------*
 *  struct bno055_t parameters can be accessed by using BNO055
 *  BNO055_t having the following parameters
 *  Bus write function pointer: BNO055_WR_FUNC_PTR
 *  Bus read function pointer: BNO055_RD_FUNC_PTR
 *  Burst read function pointer: BNO055_BRD_FUNC_PTR
 *  Delay function pointer: delay_msec
 *  I2C address: dev_addr
 *  Chip id of the sensor: chip_id
 *---------------------------------------------------------------------------*/
static struct bno055_t bno055;
static u8 power_mode = BNO055_INIT_VALUE;

/*----------------------------------------------------------------------------*
 *  Sincronizacion I2C1 en DMA.
 *  El driver de Bosch espera que bus_read/bus_write vuelvan con los datos ya
 *  transferidos, asi que lanzamos el DMA y dormimos la task hasta que el
 *  callback suelte el semaforo.
 *---------------------------------------------------------------------------*/
static osSemaphoreId_t i2c_done = NULL;
static volatile HAL_StatusTypeDef i2c_result = HAL_OK;
static volatile uint32_t last_i2c_error = 0;

static struct bno055_accel_t accel_init;
static struct bno055_gyro_t gyro_init;
static struct bno055_euler_t euler_init;

static inline float wrap_angle(float diff) {
    while (diff > 180.0f)
        diff -= 360.0f;
    while (diff < -180.0f)
        diff += 360.0f;
    return diff;
}

uint32_t bno055_last_i2c_error(void) { return last_i2c_error; }

static s8 BNO055_I2C_bus_write(u8 dev_addr, u8 reg_addr, u8 *reg_data, u8 len) {
    while (osSemaphoreAcquire(i2c_done, 0) == osOK) {
    }
    i2c_result = HAL_BUSY;

    if (HAL_I2C_Mem_Write_DMA(&hi2c1, (uint16_t)(dev_addr << 1), reg_addr,
                              I2C_MEMADD_SIZE_8BIT, reg_data, len) != HAL_OK) {
        return BNO055_ERROR;
    }

    if (osSemaphoreAcquire(i2c_done, osWaitForever) != osOK) {
        last_i2c_error = 0xFFFFFFFFU;
        HAL_I2C_Master_Abort_IT(&hi2c1, (uint16_t)(dev_addr << 1));
        osSemaphoreAcquire(i2c_done, 5);
        return BNO055_ERROR;
    }

    return (i2c_result == HAL_OK) ? BNO055_SUCCESS : BNO055_ERROR;
}

static s8 BNO055_I2C_bus_read(u8 dev_addr, u8 reg_addr, u8 *reg_data, u8 len) {
    while (osSemaphoreAcquire(i2c_done, 0) == osOK) {
    }
    i2c_result = HAL_BUSY;

    if (HAL_I2C_Mem_Read_DMA(&hi2c1, (uint16_t)(dev_addr << 1), reg_addr,
                             I2C_MEMADD_SIZE_8BIT, reg_data, len) != HAL_OK) {
        return BNO055_ERROR;
    }

    if (osSemaphoreAcquire(i2c_done, osWaitForever) != osOK) {
        last_i2c_error = 0xFFFFFFFFU;
        HAL_I2C_Master_Abort_IT(&hi2c1, (uint16_t)(dev_addr << 1));
        osSemaphoreAcquire(i2c_done, 5);
        return BNO055_ERROR;
    }

    return (i2c_result == HAL_OK) ? BNO055_SUCCESS : BNO055_ERROR;
}

/* osDelay en lugar de HAL_Delay: HAL_Delay es busy-wait y no cede la CPU.
 * El init del BNO055 mete esperas de 30-650 ms segun el modo. */
static void BNO055_delay_msec(u32 msec) { osDelay(msec); }

/*----------------------------------------------------------------------------*
 *  Callbacks HAL. Se ejecutan en contexto de ISR (I2C1_EV / I2C1_ER / DMA1).
 *  osSemaphoreRelease detecta el IPSR y llama sola a la variante FromISR,
 *  por eso esas IRQ deben estar en prioridad NVIC >= 5.
 *---------------------------------------------------------------------------*/
void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c) {
    if (hi2c->Instance == I2C1) {
        i2c_result = HAL_OK;
        osSemaphoreRelease(i2c_done);
    }
}

void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c) {
    if (hi2c->Instance == I2C1) {
        i2c_result = HAL_OK;
        osSemaphoreRelease(i2c_done);
    }
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c) {
    if (hi2c->Instance == I2C1) {
        i2c_result = HAL_ERROR;
        last_i2c_error = hi2c->ErrorCode;
        osSemaphoreRelease(i2c_done);
    }
}

void HAL_I2C_AbortCpltCallback(I2C_HandleTypeDef *hi2c) {
    if (hi2c->Instance == I2C1) {
        i2c_result = HAL_ERROR;
        last_i2c_error = hi2c->ErrorCode;
        osSemaphoreRelease(i2c_done);
    }
}

// Lectura cruda, sin restar nada — la usa struct_init() y bno055_read()
static s32 bno055_read_raw(struct bno055_accel_t *accel_out,
                           struct bno055_gyro_t *gyro_out) {
    s32 comres = BNO055_SUCCESS;
    comres += bno055_read_accel_xyz(accel_out);
    comres += bno055_read_gyro_xyz(gyro_out);
    return comres;
}

void struct_init(void) {
    if (bno055_read_raw(&accel_init, &gyro_init) != BNO055_SUCCESS) {
        accel_init.x = accel_init.y = accel_init.z = 0;
        gyro_init.x = gyro_init.y = gyro_init.z = 0;
    }

    if (bno055_read_euler_hrp(&euler_init) != BNO055_SUCCESS) {
        euler_init.h = euler_init.p = euler_init.r = 0;
    }
}

s32 bno055_init_a(void) {
    s32 comres = BNO055_ERROR;

    // Antes de cualquier acceso al bus: bno055_init() ya hace lecturas
    if (i2c_done == NULL) {
        i2c_done = osSemaphoreNew(1, 0, NULL);
        if (i2c_done == NULL) {
            return BNO055_ERROR;
        }
    }

    bno055.bus_write = BNO055_I2C_bus_write;
    bno055.bus_read = BNO055_I2C_bus_read;
    bno055.delay_msec = BNO055_delay_msec;
    bno055.dev_addr = BNO055_I2C_ADDR1;

    comres = bno055_init(&bno055);

    power_mode = BNO055_POWER_MODE_NORMAL;
    comres += bno055_set_power_mode(power_mode);

    // Inertial measurement unit.
    // Reads accel,gyro and fusion data
    comres += bno055_set_operation_mode(BNO055_OPERATION_MODE_IMUPLUS);
    comres += bno055_set_euler_unit(BNO055_EULER_UNIT_DEG);

    return comres;
}

s32 bno055_read(struct bno055_accel_t *accel_out,
                struct bno055_gyro_t *gyro_out) {
    s32 comres = BNO055_SUCCESS;

    /*  Raw accel X, Y and Z data can read from the register
     * page - page 0
     * register - 0x08 to 0x0D*/
    comres += bno055_read_accel_xyz(accel_out);

    /*  Raw gyro X, Y and Z data can read from the register
     * page - page 0
     * register - 0x14 to 0x19*/
    comres += bno055_read_gyro_xyz(gyro_out);

    gyro_out->x = gyro_out->x - gyro_init.x;
    gyro_out->y = gyro_out->y - gyro_init.y;
    gyro_out->z = gyro_out->z - gyro_init.z;

    accel_out->x = accel_out->x - accel_init.x;
    accel_out->y = accel_out->y - accel_init.y;
    accel_out->z = accel_out->z - accel_init.z;

    return comres;
}

s32 bno055_convert_double(struct bno055_accel_double_t *accel_out,
                          struct bno055_gyro_double_t *gyro_out) {
    s32 comres = BNO055_SUCCESS;

    /*  API used to read accel data output as double  - m/s2 and mg
     * float functions also available in the BNO055 API */
    comres += bno055_convert_double_accel_xyz_msq(accel_out);

    /*  API used to read gyro data output as double  - dps and rps
     * float functions also available in the BNO055 API */
    comres += bno055_convert_double_gyro_xyz_dps(gyro_out);

    return comres;
}

BNO055_RETURN_FUNCTION_TYPE bno055_read_euler(struct bno055_euler_t *euler) {
    BNO055_RETURN_FUNCTION_TYPE comres = BNO055_SUCCESS;
    comres += bno055_read_euler_hrp(euler);

    euler->h = wrap_angle(euler->h - euler_init.h);
    euler->r = wrap_angle(euler->r - euler_init.r);
    euler->p = euler->p - euler_init.p;

    return comres;
}

s32 bno055_stop(void) {
    s32 comres = BNO055_SUCCESS;

    power_mode = BNO055_POWER_MODE_SUSPEND;
    comres += bno055_set_power_mode(power_mode);

    return comres;
}