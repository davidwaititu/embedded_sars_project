

/**
 * @file app_main.c
 * @brief Application entry point.
 * @author Philipp Schilk, 2024
 */

#include "application.h"

#include "main.h"

#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>

// ==== Static Variables =======================================================

#define STACK_SIZE_TASK3 128
#define STACK_SIZE_TASK6 512
#define STACK_SIZE_TASK9 1024


TaskHandle_t task3 = 0;              // Task handle.
StaticTask_t task3_tcb = {0};        // Task tcb.
StackType_t task3_stack[STACK_SIZE_TASK3]; // Task stack.


TaskHandle_t task6 = 0;              // Task handle.
StaticTask_t task6_tcb = {0};        // Task tcb.
StackType_t task6_stack[STACK_SIZE_TASK6]; // Task stack.


TaskHandle_t task9 = 0;              // Task handle.
StaticTask_t task9_tcb = {0};        // Task tcb.
StackType_t task9_stack[STACK_SIZE_TASK9]; // Task stack.



// ==== Task 3 ======================================================================
// Read from the MP34DT05-A microphone using PDM, pocess data and print the result in serial








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
  

  // Create task 3:
  

  // create task 6:


  

  // create task 9:
  


  // start scheduler:
  vTaskStartScheduler();

  return 0; // We should never get here.
 

}


