

/**
 * @file app_main.c
 * @brief Application entry point.
 * @author Philipp Schilk, 2024
 */

#include "app.h"

#include "main.h"

#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include "queue.h"

// ==== Static Variables =======================================================

#define STACK_SIZE_TASK3 128
#define STACK_SIZE_TASK6 512
#define STACK_SIZE_TASK9 1024
#define MIC_BUFFER_SIZE 128

// ==== External Variables =======================================================
extern DFSDM_Filter_HandleTypeDef hdfsdm1_filter0; // microphone filter handle, used in task 3 to read microphone data
extern DFSDM_Channel_HandleTypeDef hdfsdm1_channel4; // microphone channel handle, used in task 3 to read microphone data
extern DMA_HandleTypeDef hdma_dfsdm1_flt0; // microphone DMA handle, used in task 3 to read microphone data
extern UART_HandleTypeDef huart2; // UART handle, used in task 3 to print microphone data

extern TaskHandle_t task3; // Task 3 handle, used to notify task 3 when DMA is done





TaskHandle_t task3 = 0;              // Task handle.
StaticTask_t task3_tcb = {0};        // Task tcb.
StackType_t task3_stack[STACK_SIZE_TASK3]; // Task stack.


TaskHandle_t task6 = 0;              // Task handle.
StaticTask_t task6_tcb = {0};        // Task tcb.
StackType_t task6_stack[STACK_SIZE_TASK6]; // Task stack.


TaskHandle_t task9 = 0;              // Task handle.
StaticTask_t task9_tcb = {0};        // Task tcb.
StackType_t task9_stack[STACK_SIZE_TASK9]; // Task stack.



int32_t mic_buffer[MIC_BUFFER_SIZE]; // buffer to store microphone data, used in task 3 to read microphone data
QueueHandle_t soundLevelQueue; // queue to store microphone data, used in task 3 to read microphone data and task 6 to display VU meter



// ==== Task 3 ======================================================================
// Read from the MP34DT05-A microphone using PDM, pocess data and print the result in serial

void task3_entry(void *pvParameters){
    // start DMA
    HAL_StatusTypeDef status = HAL_DFSDM_FilterRegularStart_DMA(&hdfsdm1_filter0, mic_buffer, MIC_BUFFER_SIZE);
  if (status != HAL_OK) {
      printf("DMA start failed: %d\r\n", status);
  }

    int32_t current_level = 0;
    while(1){
        // wait for DMA to finish
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // process data
        // Calculate DC offset
        int32_t dc_offset = 0;
        for (int i = 0; i < MIC_BUFFER_SIZE; i++) {
            dc_offset += mic_buffer[i];
        }
        dc_offset /= MIC_BUFFER_SIZE;

        // 2. Second pass: Calculate the true sound amplitude
        int32_t sum = 0;
        for(int i = 0; i < MIC_BUFFER_SIZE; i++){
            // Subtract dynamic offset, then take absolute value
            int32_t val = mic_buffer[i] - dc_offset; 
            if (val < 0) val = -val;
            sum += val;
        }
        current_level = sum / MIC_BUFFER_SIZE; // average sound level

        
        // send to queue for Task 6 and Task 9
        xQueueSend(soundLevelQueue, &current_level, 0);

        printf("Raw Data: %ld\r\n", mic_buffer[0]);
        printf("Sound level: %ld\r\n", current_level);

        // delay
        vTaskDelay(pdMS_TO_TICKS(100));
    }

}






// ==== Task 6 =================================================================
// Using the 12 matrix display real time VU meter of the microphone input using data in task 3
// the VU meter show level green, yellow and red for low, medium and high sound levels respectively. The VU meter should update at least 10 times per second.
// used SPI, Drives the 74HC595 shift registers to display 





// ==== Task 9 =================================================================
// Power managemnt: Put the microcontroller in sleep mode when there is no sound input for more than 5 seconds. 
// Wake up the microcontroller when sound is detected again. 
// Use the microphone input to detect sound levels and determine when to enter and exit sleep mode.





/**
 * @brief Application Entry Point.
 *
 * Initializes all RTOS resources & starts the scheduler. If this is done successfully,
 * it never returns and hands control to FreeRTOS. If initialization fails, it returns
 * an error code.
 */
int app_main(void) {
    soundLevelQueue = xQueueCreate(1, sizeof(int32_t)); // create queue to store microphone data
  

  // Create task 3:
  task3 = xTaskCreateStatic(
    task3_entry, 
    "Task 3", 
    STACK_SIZE_TASK3, 
    NULL, 
    1, 
    task3_stack, 
    &task3_tcb);
  

  // create task 6:


  

  // create task 9:
  


  // start scheduler:
  vTaskStartScheduler();

  return 0; // We should never get here.
 

}

// Callbacks

int _write(int file, char *ptr, int len)
{

  HAL_UART_Transmit(&huart2, (uint8_t*)ptr, len, HAL_MAX_DELAY);
  return len;
}

void HAL_DFSDM_FilterRegConvCpltCallback(DFSDM_Filter_HandleTypeDef *hdfsdm_filter)
{
HAL_GPIO_TogglePin(LED_1_GPIO_Port, LED_1_Pin); // toggle LED to indicate DMA is done
    
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(task3, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_DFSDM_FilterRegConvCpltCallback could be implemented in the user file.
   */
}

