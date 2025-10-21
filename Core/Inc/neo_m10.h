#ifndef DEVICE_DRIVERS_INC_NEO_M10_H_
#define DEVICE_DRIVERS_INC_NEO_M10_H_

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    float latitude;
    float longitude;
    float altitude;
    uint8_t fix_quality;
    uint8_t satellites_tracked;
    bool data_valid;
} gps_data_t;

void neo_m10_init(void);
void neo_m10_process_byte(uint8_t byte);
bool neo_m10_get_data(gps_data_t* data);

#endif /* DEVICE_DRIVERS_INC_NEO_M10_H_ */