#include "logger.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "FreeRTOS.h"
#include "semphr.h"

#define LOG_BUFFER_SIZE 256

static char log_buffer[LOG_BUFFER_SIZE];
static SemaphoreHandle_t log_mutex;

void logger_init(void) {
    log_mutex = xSemaphoreCreateMutex();
}

void logger_log(const char* format, ...) {
    if (xSemaphoreTake(log_mutex, portMAX_DELAY) == pdTRUE) {
        va_list args;
        va_start(args, format);
        int len = vsnprintf(log_buffer, LOG_BUFFER_SIZE, format, args);
        va_end(args);

        if (len > 0) {
            HAL_UART_Transmit(&huart1, (uint8_t*)log_buffer, len, 100);
        }
        xSemaphoreGive(log_mutex);
    }
}