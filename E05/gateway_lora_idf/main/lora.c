#include "lora.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Registros del SX127x
#define REG_FIFO                 0x00
#define REG_OP_MODE              0x01
#define REG_FRF_MSB              0x06
#define REG_FRF_MID              0x07
#define REG_FRF_LSB              0x08
#define REG_PA_CONFIG            0x09
#define REG_FIFO_ADDR_PTR        0x0d
#define REG_FIFO_TX_BASE_ADDR    0x0e
#define REG_FIFO_RX_BASE_ADDR    0x0f
#define REG_FIFO_RX_CURRENT_ADDR 0x10
#define REG_IRQ_FLAGS            0x12
#define REG_RX_NB_BYTES          0x13
#define REG_PKT_RSSI_VALUE       0x1a
#define REG_MODEM_CONFIG_1       0x1d
#define REG_MODEM_CONFIG_2       0x1e
#define REG_MODEM_CONFIG_3       0x26
#define REG_VERSION              0x42

// Modos de operación
#define MODE_LONG_RANGE_MODE     0x80
#define MODE_SLEEP               0x00
#define MODE_STDBY               0x01
#define MODE_TX                  0x03
#define MODE_RX_CONTINUOUS       0x05

static spi_device_handle_t spi_handle;
static int cs_pin;
static int rst_pin;
static int packet_rssi;

static void lora_write_reg(uint8_t reg, uint8_t val) {
    uint8_t tx[2] = { reg | 0x80, val };
    spi_transaction_t t = {
        .length = 16,
        .tx_buffer = tx,
    };
    gpio_set_level(cs_pin, 0);
    spi_device_transmit(spi_handle, &t);
    gpio_set_level(cs_pin, 1);
}

static uint8_t lora_read_reg(uint8_t reg) {
    uint8_t tx[2] = { reg & 0x7F, 0 };
    uint8_t rx[2] = { 0, 0 };
    spi_transaction_t t = {
        .length = 16,
        .tx_buffer = tx,
        .rx_buffer = rx,
    };
    gpio_set_level(cs_pin, 0);
    spi_device_transmit(spi_handle, &t);
    gpio_set_level(cs_pin, 1);
    return rx[1];
}

void lora_init(spi_host_device_t host_id, int miso, int mosi, int sck, int cs, int rst) {
    cs_pin = cs;
    rst_pin = rst;

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << cs_pin) | (1ULL << rst_pin),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&io_conf);
    gpio_set_level(cs_pin, 1);

    spi_bus_config_t bus_cfg = {
        .miso_io_num = miso,
        .mosi_io_num = mosi,
        .sclk_io_num = sck,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(host_id, &bus_cfg, SPI_DMA_CH_AUTO));

    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = 9000000,
        .mode = 0,
        .spics_io_num = -1, // Lo manejamos manualmente
        .queue_size = 1,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(host_id, &dev_cfg, &spi_handle));

    // Hard reset
    gpio_set_level(rst_pin, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(rst_pin, 1);
    vTaskDelay(pdMS_TO_TICKS(10));

    // Verificar si el módulo está conectado
    uint8_t ver = lora_read_reg(REG_VERSION);
    if (ver != 0x12) {
        ESP_LOGE("LORA", "Módulo SX127x no reconocido. Versión leída: 0x%02X", ver);
    } else {
        ESP_LOGI("LORA", "Módulo SX127x inicializado correctamente.");
    }

    // Modo Sleep para configurar
    lora_write_reg(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_SLEEP);
    vTaskDelay(pdMS_TO_TICKS(10));

    // Configurar Frecuencia a 433 MHz (Estandar del XL1278-SM1)
    uint64_t frf = ((uint64_t)433000000 << 19) / 32000000;
    lora_write_reg(REG_FRF_MSB, (uint8_t)(frf >> 16));
    lora_write_reg(REG_FRF_MID, (uint8_t)(frf >> 8));
    lora_write_reg(REG_FRF_LSB, (uint8_t)(frf >> 0));

    // Direcciones FIFO Base
    lora_write_reg(REG_FIFO_TX_BASE_ADDR, 0);
    lora_write_reg(REG_FIFO_RX_BASE_ADDR, 0);

    // Potencia de Transmisión (PA Boost)
    lora_write_reg(REG_PA_CONFIG, 0x8F); 

    // Configurar Modem: BW 125kHz, Error Coding 4/5, Spreading Factor 7
    lora_write_reg(REG_MODEM_CONFIG_1, 0x72); 
    lora_write_reg(REG_MODEM_CONFIG_2, 0x74);
    lora_write_reg(REG_MODEM_CONFIG_3, 0x04); // Auto AGC activado

    // Pasar a modo de espera
    lora_write_reg(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_STDBY);
}

void lora_send_packet(uint8_t *buf, int size) {
    // Pasar a standby y preparar buffer
    lora_write_reg(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_STDBY);
    lora_write_reg(REG_FIFO_ADDR_PTR, 0);
    
    for (int i = 0; i < size; i++) {
        lora_write_reg(REG_FIFO, buf[i]);
    }
    
    // Disparar transmisión
    lora_write_reg(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_TX);
    
    // Esperar a que termine de transmitir
    while ((lora_read_reg(REG_IRQ_FLAGS) & 0x08) == 0) {
        vTaskDelay(pdMS_TO_TICKS(2));
    }
    
    // Limpiar banderas de interrupción
    lora_write_reg(REG_IRQ_FLAGS, 0x08); 
}

int lora_receive_packet(uint8_t *buf, int size) {
    int irq = lora_read_reg(REG_IRQ_FLAGS);
    lora_write_reg(REG_IRQ_FLAGS, irq); // Limpiar banderas

    // Asegurarnos de que el módulo esté constantemente escuchando
    if ((lora_read_reg(REG_OP_MODE) & 0x07) != MODE_RX_CONTINUOUS) {
        lora_write_reg(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_RX_CONTINUOUS);
    }

    if ((irq & 0x40) == 0) {
        // No ha llegado ningún paquete
        return 0;
    }

    int len = lora_read_reg(REG_RX_NB_BYTES);
    lora_write_reg(REG_FIFO_ADDR_PTR, lora_read_reg(REG_FIFO_RX_CURRENT_ADDR));

    packet_rssi = lora_read_reg(REG_PKT_RSSI_VALUE) - 164; // Compensación para 433MHz

    for (int i = 0; i < len && i < size; i++) {
        buf[i] = lora_read_reg(REG_FIFO);
    }
    
    // Al finalizar lectura, asegurarse de estar escuchando otra vez
    lora_write_reg(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_RX_CONTINUOUS);
    
    return len;
}

int lora_packet_rssi(void) {
    return packet_rssi;
}
