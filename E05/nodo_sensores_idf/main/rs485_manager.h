#ifndef RS485_MANAGER_H
#define RS485_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

#define RS485_TXD_PIN 17
#define RS485_RXD_PIN 16
#define RS485_RTS_PIN 4 // Pin RE/DE

void rs485_init(void);
bool rs485_read_sensor(uint8_t slave_addr, uint16_t reg_addr, float *out_val);

#endif // RS485_MANAGER_H
