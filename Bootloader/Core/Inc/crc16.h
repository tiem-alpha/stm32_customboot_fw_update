#ifndef __CRC_16_H
#define __CRC_16_H

#ifdef __cplusplus
extern "C" {
#endif
#include<stdint.h>
uint16_t crc_16(const unsigned char *input_str, uint32_t num_bytes); 

#ifdef __cplusplus
}
#endif

#endif /* __CRC_16_H */