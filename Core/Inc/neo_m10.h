/* FILE: neo_m10.h */

#ifndef INC_NEO_M10_H_
#define INC_NEO_M10_H_

#include "main.h"
#include <stdbool.h>

// Cấu trúc dữ liệu GPS không đổi
typedef struct {
    float latitude;
    float longitude;
    float altitude;
    int satellites_tracked;
    int fix_quality;
    bool data_valid;
} gps_data_t;

void neo_m10_init(void);
bool neo_m10_get_data(gps_data_t* data);

// Hàm mới để xử lý buffer từ DMA
void neo_m10_process_buffer(uint8_t* buffer, uint16_t len);

#endif /* INC_NEO_M10_H_ */