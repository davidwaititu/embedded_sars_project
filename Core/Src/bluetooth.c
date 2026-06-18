#include "bluetooth.h"

/**
  * @brief  Sends a null-terminated string over Bluetooth.
  * @param  str: Pointer to the string to be sent.
  */
void BT_SendString(const char *str)
{
    if (str == NULL) return;
    
    HAL_UART_Transmit(&huart3, (uint8_t*)str, strlen(str), BT_TIMEOUT);
}

/**
  * @brief  Sends raw binary data/bytes over Bluetooth.
  * @param  pData: Pointer to the data buffer.
  * @param  size: Number of bytes to send.
  */
void BT_SendBytes(const uint8_t *pData, uint16_t size)
{
    if (pData == NULL || size == 0) return;
    
    HAL_UART_Transmit(&huart3, (uint8_t*)pData, size, BT_TIMEOUT);
}

/**
  * @brief  Formats and sends a string over Bluetooth (similar to printf).
  * @param  format: String format configuration.
  */
void BT_Printf(const char *format, ...)
{
    va_list args;
    char buffer[128];

    va_start(args, format);
    // Format the string into our local buffer
    int len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (len > 0)
    {
        HAL_UART_Transmit(&huart3, (uint8_t*)buffer, len, BT_TIMEOUT);
    }
}