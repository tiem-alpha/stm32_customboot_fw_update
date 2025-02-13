#ifndef __APP_CONFIG_H
#define __APP_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif
#include<stdint.h>

#define MAJOR 0
#define MINOR 1
#define BUILD 1
#define APP_START_ADDRESS 0x8005000

#define LOG_ENABLE 1

typedef struct app_data{
    uint8_t major; 
    uint8_t minor; 
    uint8_t build; 
    uint16_t length;
}__attribute__((packed))app_data;

#ifdef __cplusplus
}
#endif

#endif /* __APP_CONFIG_H */