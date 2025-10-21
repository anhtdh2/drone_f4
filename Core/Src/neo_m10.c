#include "neo_m10.h"
#include "usart.h"
#include <string.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "semphr.h"

#define NMEA_BUFFER_SIZE 100

uint8_t nmea_buffer[NMEA_BUFFER_SIZE];
uint8_t nmea_buffer_index = 0;
static volatile bool new_data_flag = false;
static gps_data_t gps_data_internal;
static SemaphoreHandle_t gps_mutex;

static void parse_gngga(char* gngga_sentence);

void neo_m10_init(void) {
    gps_mutex = xSemaphoreCreateMutex();
    memset(&gps_data_internal, 0, sizeof(gps_data_t));
    HAL_UART_Receive_IT(&huart2, &nmea_buffer[nmea_buffer_index], 1);
}

void neo_m10_process_byte(uint8_t byte) {
    if (byte == '$') {
        nmea_buffer_index = 0;
        memset(nmea_buffer, 0, NMEA_BUFFER_SIZE);
    }

    if (nmea_buffer_index < NMEA_BUFFER_SIZE - 1) {
        nmea_buffer[nmea_buffer_index++] = byte;
    }

    if (byte == '\n') {
        if (strncmp((char*)nmea_buffer, "$GNGGA", 6) == 0) {
            if (xSemaphoreTake(gps_mutex, (TickType_t)10) == pdTRUE) {
                parse_gngga((char*)nmea_buffer);
                xSemaphoreGive(gps_mutex);
            }
        }
        nmea_buffer_index = 0;
    }
}

bool neo_m10_get_data(gps_data_t* data) {
    if (xSemaphoreTake(gps_mutex, (TickType_t)10) == pdTRUE) {
        memcpy(data, &gps_data_internal, sizeof(gps_data_t));
        xSemaphoreGive(gps_mutex);
        return true;
    }
    return false;
}

static void parse_gngga(char* gngga_sentence) {
    char* token;
    int token_index = 0;

    token = strtok(gngga_sentence, ",");

    while (token != NULL) {
        switch (token_index) {
            case 2: // Latitude
                if (strlen(token) > 0) {
                    double lat = atof(token);
                    int deg = (int)(lat / 100);
                    double min = lat - (deg * 100);
                    gps_data_internal.latitude = (float)(deg + (min / 60.0));
                }
                break;
            case 3: // N/S Indicator
                if (token[0] == 'S') {
                    gps_data_internal.latitude *= -1.0f;
                }
                break;
            case 4: // Longitude
                if (strlen(token) > 0) {
                    double lon = atof(token);
                    int deg = (int)(lon / 100);
                    double min = lon - (deg * 100);
                    gps_data_internal.longitude = (float)(deg + (min / 60.0));
                }
                break;
            case 5: // E/W Indicator
                if (token[0] == 'W') {
                    gps_data_internal.longitude *= -1.0f;
                }
                break;
            case 6: // Fix Quality
                gps_data_internal.fix_quality = atoi(token);
                break;
            case 7: // Satellites Tracked
                gps_data_internal.satellites_tracked = atoi(token);
                break;
            case 9: // Altitude
                gps_data_internal.altitude = atof(token);
                break;
        }
        token = strtok(NULL, ",");
        token_index++;
    }
    
    if (gps_data_internal.fix_quality > 0) {
        gps_data_internal.data_valid = true;
    } else {
        gps_data_internal.data_valid = false;
    }
}