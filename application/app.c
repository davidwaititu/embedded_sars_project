 #include "bluetooth.h"
#include "main.h"
#include "projdefs.h"
 #include "stts22h_driver.h"
 #include <stdio.h>
 #include "app.h"
 #include "cmsis_os.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "math.h"
#include <stdbool.h>
#include "semphr.h"


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




  #define STACK_SIZE_TASK2 512
  #define STACK_SIZE_TASK3 1024
  #define STACK_SIZE_Task4 1024
  #define STACK_SIZE_TASK6 512
  #define STACK_SIZE_TASK7 512
    #define STACK_SIZE_TASK8 512
  #define STACK_SIZE_TASK9 1024
  #define STACK_SIZE_TASK10 512
  #define MIC_BUFFER_SIZE 128


  #define PUBLIC_ASSEMBLY_THRESHOLD 60
    #define COMMERCIAL_THRESHOLD 65
    #define RESIDENTIAL_THRESHOLD 48
    #define EDUCATIONAL_THRESHOLD 50

    #define HYSTERESIS_MS 2000  // must stay below threshold for 2s before reset



TaskHandle_t task2 = 0;              // Task handle.
StaticTask_t task2_tcb = {0};        // Task tcb.
StackType_t task2_stack[STACK_SIZE_TASK2]; // Task stack.

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

        TaskHandle_t task8 = 0;              // Task handle.
    StaticTask_t task8_tcb = {0};        // Task tcb.
    StackType_t task8_stack[STACK_SIZE_TASK8]; // Task stack.

    TaskHandle_t task9 = 0;              // Task handle.
    StaticTask_t task9_tcb = {0};        // Task tcb.
    StackType_t task9_stack[STACK_SIZE_TASK9]; // Task stack.

      TaskHandle_t task10 = 0;              // Task handle.
    StaticTask_t task10_tcb = {0};        // Task tcb.
    StackType_t task10_stack[STACK_SIZE_TASK10]; // Task stack.


int32_t mic_buffer[MIC_BUFFER_SIZE]; // buffer to store microphone data, used in task 3 to read microphone data
QueueHandle_t soundLevelQueueVU,soundLevelQueueBT, gracePeriodQueue, soundLevelQueuePWM ; // queue to store microphone data, 
SemaphoreHandle_t stdoutMutex;

 uint32_t grace_period_ms = 5000;  // default 5 seconds
volatile int THRESHOLD = 60;


// ==== Task 2 =================================================================
void Task2_entry(void* args)// Servo_Motor control dependent on the sound threshold
{
  UNUSED(args);
    printf("Mode selection \r\n");// should be displayed on the LCD display
    printf("**************\r\n");
    printf("1 --> Press UP button:      Public Assembly mode \r\n");
    printf("2 --> Press RIGHT button:   Commercial Areas mode \r\n");  
    printf("3 --> Press DOWN button:    Residential Areas mode \r\n");
    printf("4 --> Press LEFT button:    Educational and Health Institutions mode \r\n");

    
while (1)
  {
    
    if(HAL_GPIO_ReadPin(UP_BTN_GPIO_Port, UP_BTN_Pin) == GPIO_PIN_RESET)
    {
     THRESHOLD=PUBLIC_ASSEMBLY_THRESHOLD; // in dB
     printf("Public Assembly: Threshold of %d dB\r\n", THRESHOLD);
    }
    else if(HAL_GPIO_ReadPin(RIGHT_BTN_GPIO_Port, RIGHT_BTN_Pin) == GPIO_PIN_RESET)
    {
      THRESHOLD=COMMERCIAL_THRESHOLD; // in dB
      printf("Commercial Areas: Threshold of %d dB\r\n", THRESHOLD);
    }
    else if(HAL_GPIO_ReadPin(DOWN_BTN_GPIO_Port, DOWN_BTN_Pin) == GPIO_PIN_RESET)
    {
      THRESHOLD=RESIDENTIAL_THRESHOLD; // in dB
      printf("Residential Areas: Threshold of %d dB\r\n", THRESHOLD);
    }
    else if(HAL_GPIO_ReadPin(LEFT_BTN_GPIO_Port, LEFT_BTN_Pin) == GPIO_PIN_RESET)
    {
      THRESHOLD=EDUCATIONAL_THRESHOLD; // in dB
      printf("Educational and HealthInstitutions: Threshold of %d dB\r\n", THRESHOLD);
    }
    vTaskDelay(pdMS_TO_TICKS(1000)); // Delay to debounce button presses and avoid rapid threshold changes
  }
}

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
    static uint32_t notify_count = 0;
    notify_count++;
    printf("Task3 notify #%lu\r\n", notify_count);
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
        // Calculate RMS amplitude
        float sum_sq = 0.0f;

        for(int i = 0; i < MIC_BUFFER_SIZE; i++)
        {
            float sample = (float)(mic_buffer[i] - dc_offset);
            sum_sq += sample * sample;
        }

        current_level = (int32_t)sqrtf(sum_sq / MIC_BUFFER_SIZE);

        


        // simple moving average filter for sound level
        static int32_t smoothed_level = 0;
        smoothed_level = (smoothed_level * 3 + current_level) / 4;
        
        // printf("Sound Level: %ld\r\n", smoothed_level);

        // convert to dB, assuming 0-4095 range for microphone data
        float normalized_level = (float)smoothed_level / 32767.0f;

        if(normalized_level < 0.000001f)
        {
            normalized_level = 0.000001f;
        }

        float dbfs = 20.0f * log10f(normalized_level);

        // MP34DT05-A sensitivity:
        float calibration_offset = 65.0f; 
        
        float dbspl = dbfs + calibration_offset;

        printf("Sound Level: %.2f dBSPL\r\n", dbspl);


                // send to queue for Task 6 and Task 9
        xQueueSend(soundLevelQueueVU, &dbspl, 0);
        xQueueSend(soundLevelQueueBT, &dbspl, 0);
        xQueueSend(soundLevelQueuePWM, &dbspl, 0);


        // delay
        vTaskDelay(pdMS_TO_TICKS(500));
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

/// ==== Task 6 =================================================================
// Using the 12 matrix display real time VU meter of the microphone input using data in task 3
// the VU meter show level green, yellow and red for low, medium and high sound levels respectively. The VU meter should update at least 10 times per second.
// used SPI, Drives the 74HC595 shift registers to display 

void task6_entry(void *args){
    float dbspl = 0.0f; // Changed from int32_t to float
    
    // Thresholds for the 12 LEDs based on dBSPL 
   const float thresholds[12] = {
        55.0f, 60.0f, 65.0f, 70.0f,    // Green zone (Normal background to quiet talking)
        75.0f, 80.0f, 85.0f, 90.0f,    // Yellow zone (Clear talking to loud conversation)
        95.0f, 100.0f, 105.0f, 110.0f  // Red zone (Shouting or tapping the mic)
    };

    const volatile uint8_t green_bits[12] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    const volatile uint8_t red_bits[12]   = {12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23};

    while(1) {
        // Receive the calculated float directly from Task 3
        if (xQueueReceive(soundLevelQueueVU, &dbspl, pdMS_TO_TICKS(100)) == pdPASS) {
            
            uint32_t matrix_data = 0; // 24-bit payload
            
            // 1. Determine which LEDs should be on based on dBSPL thresholds
            for (int i = 0; i < 12; i++) {
                if (dbspl >= thresholds[i]) {
                    
                    // LEDs 0-3: Green
                    if (i < 4) {
                        matrix_data |= (1 << green_bits[i]);
                    }
                    // LEDs 4-7: Yellow (Green + Red)
                    else if (i < 8) {
                        matrix_data |= (1 << green_bits[i]);
                        matrix_data |= (1 << red_bits[i]);
                    }
                    // LEDs 8-11: Red
                    else {
                        matrix_data |= (1 << red_bits[i]);
                    }
                }
            }
            
            // 2. Break the 24-bit payload into 3 bytes for SPI transmission
            uint8_t spi_payload[3];
            spi_payload[0] = (matrix_data >> 16) & 0xFF; // SR3 Data
            spi_payload[1] = (matrix_data >> 8) & 0xFF;  // SR2 Data
            spi_payload[2] = matrix_data & 0xFF;         // SR1 Data
            
            // 3. Transmit via SPI
            HAL_SPI_Transmit(&hspi2, spi_payload, 3, HAL_MAX_DELAY);
            
            // 4. Latch the shift registers to display the LEDs (Toggle PC7)
            HAL_GPIO_WritePin(MATRIX_RCK_GPIO_Port, MATRIX_RCK_Pin, GPIO_PIN_SET);
            for(volatile int d=0; d<100; d++); 
            HAL_GPIO_WritePin(MATRIX_RCK_GPIO_Port, MATRIX_RCK_Pin, GPIO_PIN_RESET);
        }
    }
}


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

            // Map ADC value to grace period (1s to 10s)
            grace_period_ms = 1000 +
                ((adc_value * 9000) / 4095);

            // printf("ADC: %lu \r\n Grace_Period: %lu ms\r\n",
            //        adc_value,
            //        grace_period_ms);

            // Send grace period to Task 8 via queue
            xQueueSend(gracePeriodQueue, &grace_period_ms, 0);
        
        }
        else
        {
            printf("ADC error\r\n");
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
    
}



 void task10_entry(void *args)
{
    

    while (1)
    {
        // send sound level in dB over Bluetooth
         float dbspl;
        if (xQueueReceive(soundLevelQueueBT, &dbspl, pdMS_TO_TICKS(100)) == pdPASS) {
        
            char bt_message[50];
            
            snprintf(bt_message, sizeof(bt_message), "Sound Level: %.2f dBSPL\r\n", dbspl);
            BT_SendString(bt_message);
        }
         vTaskDelay(pdMS_TO_TICKS(2000));
    }

       
 } 


// TASK 8: PWM
void Task8_entry(void* args) 
{
  UNUSED(args);
  
  if(HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }

  // Sweep test - runs ONCE at startup
  printf("Servo sweep test starting...\r\n");
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 500);
  vTaskDelay(pdMS_TO_TICKS(1000));
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 1000);
  vTaskDelay(pdMS_TO_TICKS(1000));
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 1500);
  vTaskDelay(pdMS_TO_TICKS(1000));
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 2000);
  vTaskDelay(pdMS_TO_TICKS(1000));
  printf("Servo sweep test done.\r\n");

  float dbspl = 0.0f;
  uint32_t grace_period = 5000;

  bool is_threshold_exceeded = false;
  TickType_t exceed_start_time = 0;
  TickType_t last_above_threshold_time = 0;

  while (1)
  {
    
    xQueueReceive(soundLevelQueuePWM, &dbspl, 0);
    xQueueReceive(gracePeriodQueue, &grace_period, 0);

    TickType_t now = xTaskGetTickCount();

    if (dbspl >= THRESHOLD)
    {
      last_above_threshold_time = now;

      if (!is_threshold_exceeded)
      {
        is_threshold_exceeded = true;
        exceed_start_time = now;
        printf("Threshold exceeded! Grace period started.\r\n");
      }

      TickType_t elapsed = now - exceed_start_time;
      printf("Task 8: dBSPL: %.2f, elapsed: %lu ms, grace: %lu ms\r\n",
       dbspl, (elapsed * 1000) / configTICK_RATE_HZ, grace_period);

      if (elapsed >= pdMS_TO_TICKS(grace_period))
      {
        printf("Grace period elapsed. Opening servo.\r\n");
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 2000); // Open
      }
      else
      {
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 1000); // Hold closed
      }
    }
    else
    {
      if (is_threshold_exceeded &&
          (now - last_above_threshold_time) >= pdMS_TO_TICKS(HYSTERESIS_MS))
      {
        is_threshold_exceeded = false;
        printf("Sound settled. Resetting. Closing servo.\r\n");
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 1000); // Close
      }
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}


// ==== Task 9 =================================================================
// Power managemnt: Put the microcontroller in sleep mode when there is no sound input for more than 5 seconds. 
// Wake up the microcontroller when sound is detected again. 
// Use the microphone input to detect sound levels and determine when to enter and exit sleep mode.




  
int app_main(void) {


soundLevelQueueVU = xQueueCreate(1, sizeof(float)); // create queue to store microphone data for VU meter
soundLevelQueueBT = xQueueCreate(1, sizeof(float)); // create queue to store normalized sound levels for Bluetooth transmission
gracePeriodQueue = xQueueCreate(1, sizeof(uint32_t)); // create queue to store grace period for Task 8
stdoutMutex = xSemaphoreCreateMutex();
soundLevelQueuePWM = xQueueCreate(1, sizeof(float));


task2 = xTaskCreateStatic(Task2_entry, // Function that implements the task.
                              "task2",     // Text name for the task.
                              STACK_SIZE_TASK2,  // Number of indexes in the stack array.
                              0,           // Parameter passed into the task.
                              5,           // Priority at which the task is created.
                              task2_stack, // Array to use as the task's stack.
                              &task2_tcb); // Variable to hold the task's TCB.
  // Create task 3: Task to read microphone data and process it
  task3 = xTaskCreateStatic(
    task3_entry, 
    "Task 3", 
    STACK_SIZE_TASK3, 
    NULL, 
    4, 
    task3_stack, 
    &task3_tcb);
  
// Create task 4: Task to read temperature from STTS22H sensor
    task4 = xTaskCreateStatic(task4_entry, // Function that implements the task.
                              "task4",     // Text name for the task.
                              STACK_SIZE_Task4,  // Number of indexes in the stack array.
                              0,           // Parameter passed into the task.
                              1,           // Priority at which the task is created.
                              task4_stack, // Array to use as the task's stack.
                              &task4_tcb); // Variable to hold the task's TCB.
    // Create task 7: Task to read potentiometer and adjust grace period
    task7 = xTaskCreateStatic(task7_entry, // Function that implements the task.
                              "task7",     // Text name for the task.
                              STACK_SIZE_TASK7,  // Number of indexes in the stack array.
                              0,           // Parameter passed into the task.
                              2,           // Priority at which the task is created.
                              task7_stack, // Array to use as the task's stack.
                              &task7_tcb); // Variable to hold the task's TCB.

// Create task 10: Task to send data over Bluetooth
    task10 = xTaskCreateStatic(task10_entry, // Function that implements the task.
                              "task10",     // Text name for the task.
                              STACK_SIZE_TASK10,  // Number of indexes in the stack array.
                              0,           // Parameter passed into the task.
                              2,           // Priority at which the task is created.
                              task10_stack, // Array to use as the task's stack.
                              &task10_tcb); // Variable to hold the task's TCB.

// create task 6: Task to display VU meter on matrix display
    xTaskCreateStatic(task6_entry, // Function that implements the task.
                              "task6",     // Text name for the task.
                              STACK_SIZE_TASK6,  // Number of indexes in the stack array.
                              0,           // Parameter passed into the task.
                              4,           // Priority at which the task is created.
                              task6_stack, // Array to use as the task's stack.
                              &task6_tcb); // Variable to hold the task's TCB.

task8 = xTaskCreateStatic(Task8_entry, // Function that implements the task.
                              "task8",     // Text name for the task.
                              STACK_SIZE_TASK8,  // Number of indexes in the stack array.
                              0,           // Parameter passed into the task.
                              2,           // Priority at which the task is created.
                              task8_stack, // Array to use as the task's stack.
                              &task8_tcb); // Variable to hold the task's TCB.

vTaskStartScheduler(); // never returns
    return 0;
  }


void HAL_DFSDM_FilterRegConvCpltCallback(DFSDM_Filter_HandleTypeDef *hdfsdm_filter)
{
// HAL_GPIO_TogglePin(LED_1_GPIO_Port, LED_1_Pin); // toggle LED to indicate DMA is done

    
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(task3, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_DFSDM_FilterRegConvCpltCallback could be implemented in the user file.
   */
}

int _write(int file, char *ptr, int len)
{
    if (stdoutMutex != NULL) {
        xSemaphoreTake(stdoutMutex, portMAX_DELAY);
    }

    HAL_UART_Transmit(&huart2, (uint8_t *)ptr, len, 5000);

    if (stdoutMutex != NULL) {
        xSemaphoreGive(stdoutMutex);
    }
    return len;
}






















 #include "bluetooth.h"
#include "main.h"
#include "projdefs.h"
 #include "stts22h_driver.h"
 #include <stdio.h>
 #include "app.h"
 #include "cmsis_os.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "liquidcrystali2c.h"

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
  #define STACK_SIZE_TASK5 512
  #define STACK_SIZE_TASK7 512
  #define STACK_SIZE_TASK9 1024
  #define STACK_SIZE_TASK10 512
  #define MIC_BUFFER_SIZE 128

  // Temperature reading
    TaskHandle_t task3 = 0;              // Task handle.
    StaticTask_t task3_tcb = {0};        // Task tcb.
    StackType_t task3_stack[STACK_SIZE_TASK3]; // Task stack.

    TaskHandle_t task4 = 0;              // Task handle.
    StaticTask_t task4_tcb = {0};        // Task tcb.
    StackType_t task4_stack[STACK_SIZE_Task4]; // Task stack.

    TaskHandle_t task5 = 0;              // Task handle.
    StaticTask_t task5_tcb = {0};        // Task tcb.
    StackType_t task5_stack[STACK_SIZE_TASK5]; // Task stack.

    TaskHandle_t task6 = 0;              // Task handle.
    StaticTask_t task6_tcb = {0};        // Task tcb.
    StackType_t task6_stack[STACK_SIZE_TASK6]; // Task stack.

    TaskHandle_t task7 = 0;              // Task handle.
    StaticTask_t task7_tcb = {0};        // Task tcb.
    StackType_t task7_stack[STACK_SIZE_TASK7]; // Task stack.

    TaskHandle_t task9 = 0;              // Task handle.
    StaticTask_t task9_tcb = {0};        // Task tcb.
    StackType_t task9_stack[STACK_SIZE_TASK9]; // Task stack.

      TaskHandle_t task10 = 0;              // Task handle.
    StaticTask_t task10_tcb = {0};        // Task tcb.
    StackType_t task10_stack[STACK_SIZE_TASK10]; // Task stack.


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
// ==== Task 5 =================================================================
void task5_entry(void *args){
    printf("LCD Display");
    // You can update real-time info here later (like temperature or sound data)
        HD44780_Init(2); // Initialize LCD with 2 rows
        HD44780_Clear(); // Clear the display
        HD44780_Backlight(); // Turn on the backlight
        HD44780_SetCursor(0, 0); // Set cursor to first row, first column
        HD44780_PrintStr("WELCOME TO EBAN"); // Print string on LCD
        HD44780_SetCursor(0, 1); // Set cursor to first row, first column
        HD44780_PrintStr("SOUND DETECTION");
        // Block to let other tasks execute seamlessly
        vTaskDelay(pdMS_TO_TICKS(500)); 

   while(1) {
       
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

 void task10_entry(void *args)
{
    

    while (1)
    {
        
        BT_SendString("Hey \n");
        vTaskDelay(pdMS_TO_TICKS(1000));
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
    3, 
    task3_stack, 
    &task3_tcb);
  

    task4 = xTaskCreateStatic(task4_entry, // Function that implements the task.
                              "task4",     // Text name for the task.
                              STACK_SIZE_Task4,  // Number of indexes in the stack array.
                              0,           // Parameter passed into the task.
                              2,           // Priority at which the task is created.
                              task4_stack, // Array to use as the task's stack.
                              &task4_tcb); // Variable to hold the task's TCB.

    task5 = xTaskCreateStatic(task5_entry, // Function that implements the task.
                              "task5",     // Text name for the task.
                              STACK_SIZE_TASK5,  // Number of indexes in the stack array.
                              0,           // Parameter passed into the task.
                              2,           // Priority at which the task is created.
                              task5_stack, // Array to use as the task's stack.
                              &task5_tcb); // Variable to hold the task's TCB.
    
    task7 = xTaskCreateStatic(task7_entry, // Function that implements the task.
                              "task7",     // Text name for the task.
                              STACK_SIZE_TASK7,  // Number of indexes in the stack array.
                              0,           // Parameter passed into the task.
                              2,           // Priority at which the task is created.
                              task7_stack, // Array to use as the task's stack.
                              &task7_tcb); // Variable to hold the task's TCB.
    task10 = xTaskCreateStatic(task10_entry, // Function that implements the task.
                              "task10",     // Text name for the task.
                              STACK_SIZE_TASK10,  // Number of indexes in the stack array.
                              0,           // Parameter passed into the task.
                              2,           // Priority at which the task is created.
                              task10_stack, // Array to use as the task's stack.
                              &task10_tcb); // Variable to hold the task's TCB.



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






















