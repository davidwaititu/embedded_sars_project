
#include "stts22h_driver.h"
#include "main.h"
extern I2C_HandleTypeDef hi2c1;

/**
 * @brief Verify that communication with the stts22h is functional.
 *
 * This is done by reading the sensor's WHOAMI register and verifying that it
 * the expected value is present.
 *
 * @return STTS22H_OK if the communication is functional, STTS22H_ERROR otherwise.
 */
STTS22H_Status stts22h_check_communication(void) {
  uint8_t whoami = 0;

   if (HAL_I2C_Mem_Read(&hi2c1,STTS22H_I2C_ADDR_7B<<1, STTS22H_REG_WHOAMI, I2C_MEMADD_SIZE_8BIT, &whoami, 1, 20) != HAL_OK) {
    return STTS22H_ERROR;
  }

  if(whoami== STTS22H_REG_WHOAMI_VALUE){
    return STTS22H_OK;
  } else {
    return STTS22H_ERROR;
  }
}

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
STTS22H_Status stts22h_configure(STTS22H_Mode mode) {
  // HINT: Anytime we have to modify certain bits in a sensors register we will use
  //       the following procedure:
  //         1) Read the current register value via I2C into a variable.
  //         2) Make the desired modifications to that variable.
  //         3) Write the new register value back to the register via I2C.

  // Read current "CTRL" register:
  uint8_t reg_ctrl = 0;
HAL_I2C_Mem_Read(&hi2c1,STTS22H_I2C_ADDR_7B<<1, STTS22H_REG_CTRL, I2C_MEMADD_SIZE_8BIT, &reg_ctrl, 1, 20);
  // === Step 1: Power-down the sensor ===

  // The datasheet reads:
  //   Before changing the operating mode or ODR frequency, the user has to power down the device by writing 0 to
  //   both the FREERUN and LOW_ODR_START bits.

  // TO Set FREERUN to 0:
  //uint8_t FREERUN = 0; or
  reg_ctrl &= ~(0x01 <<2);

  // TO Set LOW_ODR_START to 0:
  //uint8_t LOW_ODR_START = 0; or
  reg_ctrl &= ~(0x01 <<7);

  // TO Write the new CTRL register value to sensor to apply it:
  // HINT : The STM32 HAL I2C functions expect the I2C address in 8-bit format. (addr_8bit = addr_7bit << 1)
  if (HAL_I2C_Mem_Write(&hi2c1,STTS22H_I2C_ADDR_7B<<1 , STTS22H_REG_CTRL, I2C_MEMADD_SIZE_8BIT, &reg_ctrl, 1, 20) != HAL_OK) {
    return STTS22H_ERROR;
  }

  //LOW_ODR is bit 7 and FREERUN is bit 2 in the register map

  // === Step 2: Configure the Sensor ===

  // Depending on which mode was requested, we must correctly configure the
  // FREERUN, LOW_ODR_START, and AVG bitfields in the ctrl register.

  // LOW_ODR_START: If the low-odr (1Hz) mode was requested, the LOW_ODR_START bit
  //                must be set to 1. Otherwise it should be set to 0.
  if (mode == STTS22H_MODE_LOW_ODR_1HZ) {
    // TO Set LOW_ODR_START bit to 1:
    reg_ctrl |= (0x1 << 7);
  } else {
    // TO Set LOW_ODR_START bit to 0:
    reg_ctrl &= ~(0x1 << 7);
  }

  // FREERUN: If the freerun mode was requested (no matter what ouput rate),
  //          the FREERUN bit should be set to 1. Otherwise it should be set to 0.
  // HINT:    The freerun mode was requested if mode is set to any of the following:
  //            - STTS22H_MODE_FREERUN_25HZ
  //            - STTS22H_MODE_FREERUN_50H
  //            - STTS22H_MODE_FREERUN_100HZ
  //            - STTS22H_MODE_FREERUN_200HZ
  if(mode ==STTS22H_MODE_FREERUN_25HZ
    ||mode == STTS22H_MODE_FREERUN_50HZ
    ||mode == STTS22H_MODE_FREERUN_100HZ||mode == STTS22H_MODE_FREERUN_200HZ){
      //if(mode >= STTS22H_MODE_FREERUN_25HZ && mode <= STTS22H_MODE_FREERUN_200HZ)
    reg_ctrl|= (0x1 << 2);
  }
  else{
    reg_ctrl &= ~(0x1 << 2);
  }

 
  // AVG:  If the freerunn mode was requested, we must set the AVG field to
  //       configure the correct output data rate. In all other modes the
  //       AVG field's value does not matter.
  // HINT: Look at the sensor datasheet to see what value you have to write to the
  //       AVG field to achieve a certain output data rate.
  if (mode == STTS22H_MODE_FREERUN_25HZ){
    reg_ctrl &= ~(0x03<<4); // set AVG to 0
    } 
    else if (mode == STTS22H_MODE_FREERUN_50HZ){
      reg_ctrl |= (0x01<<4);
    }
    else if (mode == STTS22H_MODE_FREERUN_100HZ){
      reg_ctrl |= (0x02<<4);
    }
    else if (mode == STTS22H_MODE_FREERUN_200HZ){
      reg_ctrl |= (0x03<<4);
    }

  // No matter the mode, always set BDU to 1 and IF_ADD_INC to 1:
  // These are settings that control how the communication interface works.
  // Have a look at the datasheet if you are interested.
  reg_ctrl |= (0x1 << 6);
  reg_ctrl |= (0x1 << 3);

  // TO Write to sensor to apply:
  if(HAL_I2C_Mem_Write(&hi2c1, STTS22H_I2C_ADDR_7B<<1 , STTS22H_REG_CTRL, I2C_MEMADD_SIZE_8BIT, &reg_ctrl, 1, 20) != HAL_OK) {
    return STTS22H_ERROR;
  }
  return STTS22H_OK;
}

/**
 * @brief Retrieve the latest temperature measurement.
 *
 * @note this is only valid if at least one measurement has been completed.
 * @param temp_centidegree the latest temperature measurement in centi-degrees (in hundredths of a degree)
 * @return
 */
STTS22H_Status stts22h_read_temp(int16_t *temp_centidegree) {
  // TO Read "TEMP_H_OUT and TEMP_L_OUT" register:
  // HINT:   You can read sequential register with a single I2C read.
  uint8_t temp_out[2] = {0};
 if(HAL_I2C_Mem_Read(&hi2c1,STTS22H_I2C_ADDR_7B<<1, STTS22H_REG_TEMP_L_OUT, I2C_MEMADD_SIZE_8BIT, temp_out, 2, 20)!=HAL_OK){
    return STTS22H_ERROR;
  }
  

  // TO Construct temp_centidegree from TEMP_H_OUT and TEMP_L_OUT registers:
  *temp_centidegree = 0;

 // Construct temp_centidegree properly by casting to int16_t first
  *temp_centidegree = (int16_t)((temp_out[1] << 8) | temp_out[0]);

  return STTS22H_OK;
}

/**
 * @brief Trigger a single temperature measurement.
 *
 * @note this is only possible when the sensor is in one-shot mode.
 * @return STTS22H_OK if a measurement was be triggered, STTS22H_ERROR otherwise.
 */
STTS22H_Status stts22h_trigger_oneshot(void) {
  // TO Read current "CTRL" register:
  uint8_t reg_ctrl = 0;
  if(HAL_I2C_Mem_Read(&hi2c1,STTS22H_I2C_ADDR_7B<<1, STTS22H_REG_CTRL, I2C_MEMADD_SIZE_8BIT, &reg_ctrl, 1, 20) != HAL_OK) {
    return STTS22H_ERROR;
  }

  // TO Check if we are in one-shot mode (LOW_ODR_START == 0 and FREERUN == 0)-
  //         If not, return an error.
  //uint8_t low_odr_start = (reg_ctrl >> 7) & 0x01;
  //uint8_t freerun = (reg_ctrl >> 2) & 0x01;
  
//   if(low_odr_start !=0 || freerun != 0){
//   return STTS22H_ERROR;
//  }
 
  // TO Set "ONESHOT" bit to 1:
  reg_ctrl |= (0x01 << 0);

  // TO Write new CTRL register value to sensor to apply it:
  if(HAL_I2C_Mem_Write(&hi2c1,STTS22H_I2C_ADDR_7B<<1, STTS22H_REG_CTRL, I2C_MEMADD_SIZE_8BIT, &reg_ctrl, 1, 20) != HAL_OK) {
    return STTS22H_ERROR;
  }
  return STTS22H_OK;
}

/**
 * @brief Check if a one-shot measurement has completed or is still in progress.
 *
 * @note this is only valid when the sensor is in one-shot mode.
 * @param is_busy
 * @return
 */
STTS22H_Status stts22h_check_is_busy(bool *is_busy) {
  // TO Read current "STATUS" register:
  uint8_t reg_status = 0;
  if(HAL_I2C_Mem_Read(&hi2c1,STTS22H_I2C_ADDR_7B<<1, STTS22H_REG_STATUS, I2C_MEMADD_SIZE_8BIT, &reg_status, 1, 20) != HAL_OK) {
    return STTS22H_ERROR;
  }

  // To Extract "BUSY" bit into is_busy:
  *is_busy= (reg_status >> 0) & 0x01; 

  return STTS22H_OK;
}
