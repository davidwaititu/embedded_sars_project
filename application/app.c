 #include "main.h"
 #include "stts22h_driver.h"
 #include <stdio.h>
 #include "app.h"
 #include "cmsis_os.h"

extern ADC_HandleTypeDef hadc1;

extern DFSDM_Filter_HandleTypeDef hdfsdm1_filter0;
extern DFSDM_Channel_HandleTypeDef hdfsdm1_channel4;
extern DMA_HandleTypeDef hdma_dfsdm1_flt0;

extern I2C_HandleTypeDef hi2c1;

extern SPI_HandleTypeDef hspi2;

extern TIM_HandleTypeDef htim3;

extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;


  int _write(int file, char *ptr, int len)
  {
    HAL_UART_Transmit(&huart2, (uint8_t *)ptr, len, 5000);
    return len;
  }
  
  #define STACK_SIZE_Task4 1024

  // Temperature reading
    TaskHandle_t task4 = 0;              // Task handle.
    StaticTask_t task4_tcb = {0};        // Task tcb.
    StackType_t task4_stack[STACK_SIZE_Task4]; // Task stack.

 void wait_for_time(void) {
  vTaskDelay(pdMS_TO_TICKS(1000));
 }
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


  
int app_main(void) {
    // Start tracer acquisition:
    // Create task 1:
    task4 = xTaskCreateStatic(task4_entry, // Function that implements the task.
                              "task4",     // Text name for the task.
                              STACK_SIZE_Task4,  // Number of indexes in the stack array.
                              0,           // Parameter passed into the task.
                              2,           // Priority at which the task is created.
                              task4_stack, // Array to use as the task's stack.
                              &task4_tcb); // Variable to hold the task's TCB.


vTaskStartScheduler(); // never returns
    return 0;
  }