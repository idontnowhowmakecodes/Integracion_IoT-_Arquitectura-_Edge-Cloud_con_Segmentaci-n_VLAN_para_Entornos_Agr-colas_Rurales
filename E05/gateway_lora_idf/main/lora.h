#ifndef LORA_H
#define LORA_H

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Inicializa el módulo LoRa SX127x
 * @param host_id Puerto SPI (SPI2_HOST o SPI3_HOST)
 * @param miso Pin MISO
 * @param mosi Pin MOSI
 * @param sck Pin SCK
 * @param cs Pin Chip Select (NSS)
 * @param rst Pin Reset
 */
void lora_init(spi_host_device_t host_id, int miso, int mosi, int sck, int cs, int rst);

/**
 * @brief Envía un paquete de datos por LoRa
 * @param buf Puntero al buffer de datos
 * @param size Tamaño de los datos
 */
void lora_send_packet(uint8_t *buf, int size);

/**
 * @brief Revisa si llegó un paquete y lo lee
 * @param buf Puntero al buffer donde guardar los datos
 * @param size Tamaño máximo del buffer
 * @return La cantidad de bytes recibidos, o 0 si no hay nada
 */
int lora_receive_packet(uint8_t *buf, int size);

/**
 * @brief Obtiene la fuerza de la señal (RSSI) del último paquete
 * @return RSSI en dBm
 */
int lora_packet_rssi(void);

#endif // LORA_H
