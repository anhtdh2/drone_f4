#ifndef MPU9250_I2C_DRIVER_H
#define MPU9250_I2C_DRIVER_H

#include "main.h"
#include <stdbool.h>

typedef struct {
    float accel[3];
    float gyro[3];
    float temp;
} imu_data_t;

bool mpu9250_init(void);
bool mpu9250_read_data(imu_data_t* data);

#endif