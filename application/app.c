 #include "main.h"
 #include "stts22h_driver.h"
 #include <stdio.h>
 #include "app.h"
 #include "cmsis_os.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

extern ADC_HandleTypeDef hadc1;

extern DFSDM_Filter_HandleTypeDef hdfsdm1_filter0;
extern DFSDM_Channel_HandleTypeDef hdfsdm1_channel4;
extern DMA_HandleTypeDef hdma_dfsdm1_flt0;

extern I2C_HandleTypeDef hi2c1;

extern SPI_HandleTypeDef hspi2;

extern TIM_HandleTypeDef htim3;

extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;

extern TaskHandle_t task3; // Task 3 handle, used to notify task 3 when DMA is done


  int _write(int file, char *ptr, int len)
  {
    HAL_UART_Transmit(&huart2, (uint8_t *)ptr, len, 5000);
    return len;
  }
  #define STACK_SIZE_TASK3 1024
  #define STACK_SIZE_Task4 1024
  #define STACK_SIZE_TASK6 512
  #define STACK_SIZE_TASK7 512
  #define STACK_SIZE_TASK9 1024
  #define MIC_BUFFER_SIZE 128

  // Temperature reading
    TaskHandle_t task3 = 0;              // Task handle.
    StaticTask_t task3_tcb = {0};        // Task tcb.
    StackType_t task3_stack[STACK_SIZE_TASK3]; // Task stack.

    TaskHandle_t task4 = 0;              // Task handle.
    StaticTask_t task4_tcb = {0};        // Task tcb.
    StackType_t task4_stack[STACK_SIZE_Task4]; // Task stack.

    TaskHandle_t task6 = 0;              // Task handle.
    StaticTask_t task6_tcb = {0};        // Task tcb.
    StackType_t task6_stack[STACK_SIZE_TASK6]; // Task stack.

    TaskHandle_t task7 = 0;              // Task handle.
    StaticTask_t task7_tcb = {0};        // Task tcb.
    StackType_t task7_stack[STACK_SIZE_TASK7]; // Task stack.

    TaskHandle_t task9 = 0;              // Task handle.
    StaticTask_t task9_tcb = {0};        // Task tcb.
    StackType_t task9_stack[STACK_SIZE_TASK9]; // Task stack.


int32_t mic_buffer[MIC_BUFFER_SIZE]; // buffer to store microphone data, used in task 3 to read microphone data
QueueHandle_t soundLevelQueue; // queue to store microphone data, used in task 3 to read microphone data and task 6 to display VU meter
volatile uint32_t grace_period_ms = 5000;  // default 5 seconds

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

//TASK 4: TEMPERATURE READINGS
  void task4_entry(void *args)
{
    UNUSED(args);

    printf("Temperature task started\r\n");

    // Check communication
    if (stts22h_check_communication() != STTS22H_OK)
    {
        printf("STTS22H communication failed\r\n");
        vTaskDelete(NULL);
    }

    printf("STTS22H connected\r\n");


    // Set sensor to continuous mode
    if (stts22h_configure(STTS22H_MODE_FREERUN_50HZ) != STTS22H_OK)
    {
        printf("Freerun configuration failed\r\n");
        vTaskDelete(NULL);
    }

    printf("Freerun mode enabled\r\n");


    while(1)
    {
        int16_t temp_centidegree = 0;


        if(stts22h_read_temp(&temp_centidegree) == STTS22H_OK)
        {
            float temp = temp_centidegree / 100.0f;

            printf("Temperature: %.2f °C\r\n", temp);
        }
        else
        {
            printf("Temperature read error\r\n");
        }


        // Task sleeps for 1 second
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

// ==== Task 6 =================================================================
// Using the 12 matrix display real time VU meter of the microphone input using data in task 3
// the VU meter show level green, yellow and red for low, medium and high sound levels respectively. The VU meter should update at least 10 times per second.
// used SPI, Drives the 74HC595 shift registers to display 


// ==== Task 7 =================================================================
//Adjusting the delay(Grace Period) using the potentiometer
  void task7_entry(void *args)
{
    UNUSED(args);
    printf("Task 7: Potentiometer + Grace control started\r\n");

    while (1)
    {
        uint32_t adc_value = 0;

        // Start ADC
        HAL_ADC_Start(&hadc1);

        // Wait for conversion
        if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK)
        {
            adc_value = HAL_ADC_GetValue(&hadc1);

            // Map ADC → grace period (1s to 10s)
            grace_period_ms = 1000 +
                ((adc_value * 9000) / 4095);

            printf("ADC: %lu \r\n Grace_Period: %lu ms\r\n",
                   adc_value,
                   grace_period_ms);
        }
        else
        {
            printf("ADC error\r\n");
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
    
}


// ==== Task 9 =================================================================
// Power managemnt: Put the microcontroller in sleep mode when there is no sound input for more than 5 seconds. 
// Wake up the microcontroller when sound is detected again. 
// Use the microphone input to detect sound levels and determine when to enter and exit sleep mode.




  
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
  

    task4 = xTaskCreateStatic(task4_entry, // Function that implements the task.
                              "task4",     // Text name for the task.
                              STACK_SIZE_Task4,  // Number of indexes in the stack array.
                              0,           // Parameter passed into the task.
                              2,           // Priority at which the task is created.
                              task4_stack, // Array to use as the task's stack.
                              &task4_tcb); // Variable to hold the task's TCB.
    
    task7 = xTaskCreateStatic(task7_entry, // Function that implements the task.
                              "task7",     // Text name for the task.
                              STACK_SIZE_TASK7,  // Number of indexes in the stack array.
                              0,           // Parameter passed into the task.
                              2,           // Priority at which the task is created.
                              task7_stack, // Array to use as the task's stack.
                              &task7_tcb); // Variable to hold the task's TCB.



vTaskStartScheduler(); // never returns
    return 0;
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






















