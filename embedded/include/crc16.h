#ifndef CRC16_H
#define CRC16_H

#include <stdint.h>
#include <stddef.h>

/**
 * @brief Computes CRC16-CCITT (0x1021)
 * 
 * @param data Pointer to data buffer
 * @param length Length of data in bytes
 * @return uint16_t Calculated CRC
 */
uint16_t crc16_ccitt(const uint8_t *data, size_t length);

#endif // CRC16_H
