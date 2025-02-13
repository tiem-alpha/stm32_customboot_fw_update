#include "crc16.h"
#include <stdint.h>
#include <stdbool.h>
#define SEEK 0xFFFF
#define POLY 0x1021

uint16_t crc_16(const unsigned char *input_str, uint32_t num_bytes) {
    uint16_t crc = SEEK; // Initial value
    uint16_t polynomial = POLY; // CRC-16-IBM-3740 polynomial

    for (uint32_t i = 0; i < num_bytes; i++) {
        crc ^= (input_str[i] << 8); // XOR byte into MSB of crc

        for (uint8_t bit = 0; bit < 8; bit++) {
            if (crc & 0x8000) { // If MSB is set
                crc = (crc << 1) ^ polynomial;
            } else {
                crc = crc << 1;
            }
        }
    }

    return crc;
}