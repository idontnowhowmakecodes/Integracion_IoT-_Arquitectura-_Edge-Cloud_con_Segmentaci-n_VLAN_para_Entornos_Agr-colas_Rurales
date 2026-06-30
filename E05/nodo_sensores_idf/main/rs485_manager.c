#include "rs485_manager.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "RS485_MGR";
#define UART_NUM UART_NUM_2
#define BUF_SIZE (127)

void rs485_init(void) {
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
    };

    ESP_LOGI(TAG, "Inicializando UART2 para RS485");
    ESP_ERROR_CHECK(uart_param_config(UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM, RS485_TXD_PIN, RS485_RXD_PIN, RS485_RTS_PIN, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0));

    // Configurar modo RS485 Half Duplex (el driver UART maneja el RTS automáticamente si se configura)
    ESP_ERROR_CHECK(uart_set_mode(UART_NUM, UART_MODE_RS485_HALF_DUPLEX));
}

// Ejemplo de lectura de un registro Modbus (Función 03)
// Retorna true si lee correctamente, false si el sensor está desconectado (timeout)
bool rs485_read_sensor(uint8_t slave_addr, uint16_t reg_addr, float *out_val) {
    uint8_t req[8] = {slave_addr, 0x03, (uint8_t)(reg_addr >> 8), (uint8_t)(reg_addr & 0xFF), 0x00, 0x01, 0x00, 0x00};
    
    // Cálculo de CRC simple (Placeholder - se debe implementar un CRC16 Modbus real aquí)
    // Para simplificar la prueba y cumplir el requerimiento de detectar fallas:
    req[6] = 0xFF; // Placeholder CRC LSB
    req[7] = 0xFF; // Placeholder CRC MSB

    uart_write_bytes(UART_NUM, (const char *)req, 8);

    uint8_t rx_data[16];
    int len = uart_read_bytes(UART_NUM, rx_data, sizeof(rx_data), pdMS_TO_TICKS(500));

    if (len > 0) {
        // Parsear respuesta Modbus
        // Asumiendo respuesta: SlaveAddr, Func(03), ByteCount, DataHi, DataLo, CrcHi, CrcLo
        if (len >= 7 && rx_data[0] == slave_addr && rx_data[1] == 0x03) {
            uint16_t val = (rx_data[3] << 8) | rx_data[4];
            *out_val = (float)val; // Depende de la escala del sensor
            return true;
        }
    }
    
    ESP_LOGW(TAG, "Sensor RS485 ID:%d desconectado o sin respuesta", slave_addr);
    return false;
}
