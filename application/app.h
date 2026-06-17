/**
 * @file application.h
 * @brief Application main header.
 * @author Philipp Schilk, 2024
 */
#ifndef APP_MAIN_H_
#define APP_MAIN_H_

/**
 * @brief Application Entry Point.
 *
 * Initializes all RTOS resources & starts the scheduler. If this is done successfully,
 * it never returns and hands control to FreeRTOS. If initialization fails, it returns
 * an error code.
 */
int app_main(void);

#endif /* APP_MAIN_H_ */
