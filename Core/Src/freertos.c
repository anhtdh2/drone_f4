/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : freertos.c
 * Description        : Code for freertos applications
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "main.h"
#include "task.h"


/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "logger.h"
#include "neo_m10.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
void logger_task(void *argument);
void gps_processing_task(void *argument);

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
osThreadId gpsProcessingTaskHandle;
osSemaphoreId gpsDataSemHandle;
/* USER CODE END Variables */
osThreadId defaultTaskHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   uint32_t *pulIdleTaskStackSize);

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   uint32_t *pulIdleTaskStackSize) {
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
 * @brief  FreeRTOS initialization
 * @param  None
 * @retval None
 */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  osSemaphoreDef(gpsDataSem);
  gpsDataSemHandle = osSemaphoreCreate(osSemaphore(gpsDataSem), 1);
  osSemaphoreWait(gpsDataSemHandle, 0);
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  osThreadDef(logger, logger_task, osPriorityLow, 0, 256);
  osThreadCreate(osThread(logger), NULL);

  // Định nghĩa và tạo task xử lý GPS mới
  osThreadDef(gps_processing, gps_processing_task, osPriorityNormal, 0, 512);
  gpsProcessingTaskHandle = osThreadCreate(osThread(gps_processing), NULL);
  /* USER CODE END RTOS_THREADS */
}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
 * @brief  Function implementing the defaultTask thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const *argument) {
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for (;;) {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void logger_task(void *argument) {
    gps_data_t current_gps_data;

    for(;;) {
        if (neo_m10_get_data(&current_gps_data)) {
            if (current_gps_data.data_valid) {
                 logger_log("GPS Fix: YES, Sats: %d, Lat: %f, Lon: %f, Alt: %.2f\r\n",
                           current_gps_data.satellites_tracked,
                           current_gps_data.latitude,
                           current_gps_data.longitude,
                           current_gps_data.altitude);
            } else {
                logger_log("GPS Fix: NO, Sats: %d\r\n", current_gps_data.satellites_tracked);
            }
        }
        osDelay(1000); // Tần suất log ra thông tin đã xử lý
    }
}

// Buffer nhận dữ liệu DMA và kích thước dữ liệu nhận được
// Chúng sẽ được cập nhật trong callback và được xử lý trong task này
extern uint8_t dma_rx_buffer[];
extern volatile uint16_t dma_rx_size;

/**
 * @brief Task chuyên xử lý buffer NMEA nhận được từ DMA.
 * @param argument: Không dùng
 * @retval None
 */
void gps_processing_task(void *argument) {
    for(;;) {
        // Đợi tín hiệu từ callback DMA báo rằng đã có dữ liệu mới
        if (osSemaphoreWait(gpsDataSemHandle, osWaitForever) == osOK) {
            // Có dữ liệu mới, gọi hàm xử lý buffer
            if (dma_rx_size > 0) {
                neo_m10_process_buffer(dma_rx_buffer, dma_rx_size);
            }
        }
    }
}
/* USER CODE END Application */
