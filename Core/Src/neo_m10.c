/* FILE: neo_m10.c */

#include "neo_m10.h"
#include "logger.h"
#include "usart.h"
#include <string.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "semphr.h"

// Các biến và hàm khác không đổi...
static gps_data_t gps_data_internal;
static SemaphoreHandle_t gps_mutex;

// Hàm private để phân tích GNGGA, sử dụng strtok_r
static void parse_gngga(char* gngga_sentence);

void neo_m10_init(void) {
    gps_mutex = xSemaphoreCreateMutex();
    memset(&gps_data_internal, 0, sizeof(gps_data_t));
}

bool neo_m10_get_data(gps_data_t* data) {
    if (xSemaphoreTake(gps_mutex, (TickType_t)10) == pdTRUE) {
        memcpy(data, &gps_data_internal, sizeof(gps_data_t));
        xSemaphoreGive(gps_mutex);
        return true;
    }
    return false;
}

/**
  * @brief  Xử lý một buffer chứa dữ liệu NMEA nhận được từ DMA.
  * @param  buffer: Con trỏ tới buffer dữ liệu.
  * @param  len: Độ dài của dữ liệu trong buffer.
  * @retval None
  */
void neo_m10_process_buffer(uint8_t* buffer, uint16_t len) {
    char local_buffer[len + 1];
    memcpy(local_buffer, buffer, len);
    local_buffer[len] = '\0';

    char *saveptr1; // Con trỏ lưu trạng thái cho việc tách câu

    // Tách buffer thành các câu NMEA riêng lẻ
    char* sentence = strtok_r(local_buffer, "\r\n", &saveptr1);

    while (sentence != NULL) {
        // Kiểm tra câu GNGGA
        if (strncmp(sentence, "$GNGGA", 6) == 0) {
            logger_log("GGA: %s\r\n", sentence);
            if (xSemaphoreTake(gps_mutex, (TickType_t)10) == pdTRUE) {
                char gga_copy[strlen(sentence) + 1];
                strcpy(gga_copy, sentence);
                parse_gngga(gga_copy); // Gọi hàm parse đã được sửa
                xSemaphoreGive(gps_mutex);
            }
        }
        // Kiểm tra và log câu GNRMC
        else if (strncmp(sentence, "$GNRMC", 6) == 0) {
            logger_log("RMC: %s\r\n", sentence);
        }

        // Lấy câu tiếp theo
        sentence = strtok_r(NULL, "\r\n", &saveptr1);
    }
}

static void parse_gngga(char* gngga_sentence) {
    char* token;
    int token_index = 0;
    char *saveptr2; // Con trỏ lưu trạng thái cho việc tách trường

    // Bắt đầu tách trường bằng strtok_r
    token = strtok_r(gngga_sentence, ",", &saveptr2);

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
        // Lấy trường tiếp theo bằng strtok_r
        token = strtok_r(NULL, ",", &saveptr2);
        token_index++;
    }
    
    if (gps_data_internal.fix_quality > 0) {
        gps_data_internal.data_valid = true;
    } else {
        gps_data_internal.data_valid = false;
    }
}