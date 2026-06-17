
#ifndef STTS22H_DRIVER_H_
#define STTS22H_DRIVER_H_

#include <stdbool.h>
#include <stdint.h>

#define STTS22H_I2C_ADDR_7B 0x3F /** @brief I2C Address (7-bit format). */

// ===== Registers =============================================================

#define STTS22H_REG_WHOAMI       0x01 /** @brief WHO-AM-I Register Address. */
#define STTS22H_REG_TEMP_H_LIMIT 0X02 /** @brief TEMP_H_LIMIT Register Address. */
#define STTS22H_REG_TEMP_L_LIMIT 0X03 /** @brief TEMP_L_LIMIT Register Address. */
#define STTS22H_REG_CTRL         0X04 /** @brief CTRL Register Address. */
#define STTS22H_REG_STATUS       0X05 /** @brief STATUS Register Address. */
#define STTS22H_REG_TEMP_L_OUT   0X06 /** @brief TEMP_L_OUT Register Address. */
#define STTS22H_REG_TEMP_H_OUT   0X07 /** @brief TEMP_H_OUT Register Address. */



#define STTS22H_REG_WHOAMI_VALUE 0xA0 /** @brief WHO-AM-I Register Value. */


typedef enum {
  STTS22H_OK = 0,
  STTS22H_ERROR = 1
} STTS22H_Status;

typedef enum {
  STTS22H_MODE_ONE_SHOT = 0,
  STTS22H_MODE_LOW_ODR_1HZ,
  STTS22H_MODE_FREERUN_25HZ,
  STTS22H_MODE_FREERUN_50HZ,
  STTS22H_MODE_FREERUN_100HZ,
  STTS22H_MODE_FREERUN_200HZ,
} STTS22H_Mode;


STTS22H_Status stts22h_check_communication(void);

/**
 * @brief Set the sensor to a specific operating mode.
 *
 * The sensor can run in one of the following modes:
 *    - FREERUN: The temperature is measured continuously at 25, 50, 100 or 200Hz.
 *    - LOW_ODR: The temperature is measured continuously at 1Hz.
 *    - ONESHOT: The sensor is in sleep mode, and an individual measurement can be triggered.
 *
 * @param mode the mode to configure the sensor to.
 * @return STTS22H_OK if it was configured successfully, STTS22H_ERROR otherwise.
 */
STTS22H_Status stts22h_configure(STTS22H_Mode mode);
STTS22H_Status stts22h_init_oneshot(void);

/**
 * @brief Retrieve the latest temperature measurement.
 *
 * @note this is only valid if at least one measurement has been completed.
 * @param temp_centidegree the latest temperature measurement in centi-degrees (in hundredths of a degree)
 * @return
 */
STTS22H_Status stts22h_read_temp(int16_t *temp_centidegree);

/**
 * @brief Trigger a single temperature measurement.
 *
 * @note this is only possible when the sensor is in one-shot mode.
 * @return STTS22H_OK if a measurement was be triggered, STTS22H_ERROR otherwise.
 */
STTS22H_Status stts22h_trigger_oneshot(void);

/**
 * @brief Check if a one-shot measurement has completed or is still in progress.
 *
 * @note this is only valid when the sensor is in one-shot mode.
 * @param is_busy
 * @return
 */
STTS22H_Status stts22h_check_is_busy(bool *is_busy);

#endif /* STTS22H_DRIVER_H_ */
