#include "bmp280.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "BMP280_DRV";
static i2c_port_t port;

// Datos de calibración
static uint16_t dig_T1;
static int16_t dig_T2, dig_T3;
static uint16_t dig_P1;
static int16_t dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
static int32_t t_fine;

static uint8_t dev_addr = BMP280_I2C_ADDR;

static esp_err_t read_registers(uint8_t reg, uint8_t *data, size_t len) {
    return i2c_master_write_read_device(port, dev_addr, &reg, 1, data, len, pdMS_TO_TICKS(1000));
}

static esp_err_t write_register(uint8_t reg, uint8_t val) {
    uint8_t data[2] = {reg, val};
    return i2c_master_write_to_device(port, dev_addr, data, 2, pdMS_TO_TICKS(1000));
}

bool bmp280_init(i2c_port_t i2c_num) {
    port = i2c_num;
    uint8_t id = 0;
    
    // Verificar el ID del chip (0x58/0x56 para BMP280, 0x60 para BME280)
    dev_addr = 0x76;
    if (read_registers(0xD0, &id, 1) != ESP_OK || (id != 0x58 && id != 0x56 && id != 0x60)) {
        // Fallback a la dirección 0x77 (común en módulos de Adafruit y otros)
        dev_addr = 0x77;
        if (read_registers(0xD0, &id, 1) != ESP_OK || (id != 0x58 && id != 0x56 && id != 0x60)) {
            ESP_LOGE(TAG, "Sensor no encontrado ni en 0x76 ni en 0x77. ID leído: 0x%02X", id);
            return false;
        }
    }
    ESP_LOGI(TAG, "BMP280 encontrado en la dirección I2C: 0x%02X (ID: 0x%02X)", dev_addr, id);

    uint8_t calib[24];
    if (read_registers(0x88, calib, 24) != ESP_OK) {
        ESP_LOGE(TAG, "Error leyendo los datos de calibración");
        return false;
    }

    // Parsear datos de calibración con casteo explícito para evitar problemas de signo
    dig_T1 = (uint16_t)(calib[0] | (calib[1] << 8));
    dig_T2 = (int16_t)(calib[2] | (calib[3] << 8));
    dig_T3 = (int16_t)(calib[4] | (calib[5] << 8));
    dig_P1 = (uint16_t)(calib[6] | (calib[7] << 8));
    dig_P2 = (int16_t)(calib[8] | (calib[9] << 8));
    dig_P3 = (int16_t)(calib[10] | (calib[11] << 8));
    dig_P4 = (int16_t)(calib[12] | (calib[13] << 8));
    dig_P5 = (int16_t)(calib[14] | (calib[15] << 8));
    dig_P6 = (int16_t)(calib[16] | (calib[17] << 8));
    dig_P7 = (int16_t)(calib[18] | (calib[19] << 8));
    dig_P8 = (int16_t)(calib[20] | (calib[21] << 8));
    dig_P9 = (int16_t)(calib[22] | (calib[23] << 8));

    // Es crucial escribir primero el registro CONFIG (0xF5) mientras está en modo Sleep
    // Configuración: t_sb = 500ms, filter = x16 -> config = 0x90
    write_register(0xF5, 0x90);
    
    // Luego escribir el registro CTRL_MEAS (0xF4) para pasarlo a modo Normal
    // Configuración: modo normal, osrs_t x1, osrs_p x1 -> ctrl_meas = 0x27
    write_register(0xF4, 0x27);
    
    ESP_LOGI(TAG, "BMP280 inicializado correctamente.");
    return true;
}

bool bmp280_read(float *temperature, float *pressure) {
    uint8_t data[6];
    if (read_registers(0xF7, data, 6) != ESP_OK) return false;

    int32_t adc_P = (data[0] << 12) | (data[1] << 4) | (data[2] >> 4);
    int32_t adc_T = (data[3] << 12) | (data[4] << 4) | (data[5] >> 4);

    // Compensación de Temperatura
    int32_t var1, var2;
    var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)dig_T1)) * ((adc_T >> 4) - ((int32_t)dig_T1))) >> 12) * ((int32_t)dig_T3)) >> 14;
    t_fine = var1 + var2;
    *temperature = (t_fine * 5 + 128) >> 8;
    *temperature /= 100.0;

    // Compensación de Presión
    int64_t p_var1, p_var2, p;
    p_var1 = ((int64_t)t_fine) - 128000;
    p_var2 = p_var1 * p_var1 * (int64_t)dig_P6;
    p_var2 = p_var2 + ((p_var1 * (int64_t)dig_P5) << 17);
    p_var2 = p_var2 + (((int64_t)dig_P4) << 35);
    p_var1 = ((p_var1 * p_var1 * (int64_t)dig_P3) >> 8) + ((p_var1 * (int64_t)dig_P2) << 12);
    p_var1 = (((((int64_t)1) << 47) + p_var1)) * ((int64_t)dig_P1) >> 33;

    if (p_var1 == 0) return false; // Evitar división por cero
    
    p = 1048576 - adc_P;
    p = (((p << 31) - p_var2) * 3125) / p_var1;
    p_var1 = (((int64_t)dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    p_var2 = (((int64_t)dig_P8) * p) >> 19;
    p = ((p + p_var1 + p_var2) >> 8) + (((int64_t)dig_P7) << 4);
    
    *pressure = (float)p / 256.0 / 100.0; // en hPa

    return true;
}
