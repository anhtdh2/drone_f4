#include "mpu9250_i2c_driver.h"
#include "i2c.h"
#include "cmsis_os.h"
#include "logger.h"
#include <math.h>

#define MPU9250_I2C_ADDR        (0x68 << 1)
#define WHO_AM_I_MPU9250        0x75
#define PWR_MGMT_1              0x6B
#define GYRO_CONFIG             0x1B
#define ACCEL_CONFIG            0x1C
#define ACCEL_XOUT_H            0x3B

static float _accel_scale;
static float _gyro_scale;

static bool mpu_write_register(uint8_t reg, uint8_t value) {
    return HAL_I2C_Mem_Write(&hi2c1, MPU9250_I2C_ADDR, reg, 1, &value, 1, HAL_MAX_DELAY) == HAL_OK;
}

static bool mpu_read_registers(uint8_t reg, uint8_t* buffer, uint16_t len) {
    return HAL_I2C_Mem_Read(&hi2c1, MPU9250_I2C_ADDR, reg, 1, buffer, len, HAL_MAX_DELAY) == HAL_OK;
}

bool mpu9250_init(void) {
    uint8_t who_am_i = 0;

    if (HAL_I2C_IsDeviceReady(&hi2c1, MPU9250_I2C_ADDR, 2, 100) != HAL_OK) {
        logger_log("MPU6500 not found on I2C bus.\r\n");
        return false;
    }
    
    if (!mpu_read_registers(WHO_AM_I_MPU9250, &who_am_i, 1) || (who_am_i != 0x70)) {
        logger_log("MPU6500 Init Failed. WHO_AM_I = 0x%02X\r\n", who_am_i);
        return false;
    }
    
    mpu_write_register(PWR_MGMT_1, 0x80); osDelay(100);
    mpu_write_register(PWR_MGMT_1, 0x01); osDelay(10);
    mpu_write_register(GYRO_CONFIG, 0x00); 
    _gyro_scale = 250.0f / 32768.0f;
    mpu_write_register(ACCEL_CONFIG, 0x00);
    _accel_scale = 2.0f / 32768.0f;
    
    logger_log("MPU6500 (6-DoF) Initialized Successfully via I2C. WHO_AM_I = 0x%02X\r\n", who_am_i);
    return true;
}

bool mpu9250_read_data(imu_data_t* data) {
    uint8_t buffer[14];
    
    if (!mpu_read_registers(ACCEL_XOUT_H, buffer, 14)) return false;

    int16_t ax_raw = (int16_t)(buffer[0] << 8 | buffer[1]);
    int16_t ay_raw = (int16_t)(buffer[2] << 8 | buffer[3]);
    int16_t az_raw = (int16_t)(buffer[4] << 8 | buffer[5]);
    int16_t temp_raw = (int16_t)(buffer[6] << 8 | buffer[7]);
    int16_t gx_raw = (int16_t)(buffer[8] << 8 | buffer[9]);
    int16_t gy_raw = (int16_t)(buffer[10] << 8 | buffer[11]);
    int16_t gz_raw = (int16_t)(buffer[12] << 8 | buffer[13]);

    data->accel[0] = (float)ax_raw * _accel_scale;
    data->accel[1] = (float)ay_raw * _accel_scale;
    data->accel[2] = (float)az_raw * _accel_scale;
    data->gyro[0] = (float)gx_raw * _gyro_scale;
    data->gyro[1] = (float)gy_raw * _gyro_scale;
    data->gyro[2] = (float)gz_raw * _gyro_scale;
    data->temp = ((float)temp_raw) / 333.87f + 21.0f;

    return true;
}