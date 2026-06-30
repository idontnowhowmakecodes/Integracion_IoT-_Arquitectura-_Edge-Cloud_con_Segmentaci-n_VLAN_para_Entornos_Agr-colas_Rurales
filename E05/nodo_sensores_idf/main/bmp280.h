#ifndef BMP280_H
#define BMP280_H

#include "driver/i2c.h"
#include <stdint.h>
#include <stdbool.h>

// La dirección por defecto del BMP280 suele ser 0x76 (o 0x77 si SDO está a VCC)
#define BMP280_I2C_ADDR 0x76

/**
 * @brief Inicializa el sensor BMP280 y lee sus datos de calibración
 * @param i2c_num El puerto I2C donde está conectado el sensor
 * @return true si se inicializó correctamente, false en caso de error
 */
bool bmp280_init(i2c_port_t i2c_num);

/**
 * @brief Lee la temperatura y la presión compensadas
 * @param temperature Puntero para almacenar la temperatura en grados Celsius
 * @param pressure Puntero para almacenar la presión en hPa
 * @return true si se leyó correctamente, false en caso de error
 */
bool bmp280_read(float *temperature, float *pressure);

#endif // BMP280_H
