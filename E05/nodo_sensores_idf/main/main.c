#include <stdio.h>
#include <string.h>
#include <math.h>
#include "bmp280.h"
#include "driver/i2c.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lora.h"
#include "rs485_manager.h"

// Pines de I2C para BMP280
#define I2C_MASTER_SCL_IO 22
#define I2C_MASTER_SDA_IO 21
#define I2C_MASTER_NUM 0
#define I2C_MASTER_FREQ_HZ 100000

// Pines SPI para el módulo LoRa XL1278 (VSPI estándar del ESP32)
#define LORA_MISO_PIN 19
#define LORA_MOSI_PIN 23
#define LORA_SCK_PIN  18
#define LORA_CS_PIN   5
#define LORA_RST_PIN  14

// Canal ADC para el sensor de humedad de suelo (GPIO 34 = ADC1_CHANNEL_6)
#define SOIL_MOISTURE_ADC_UNIT ADC_UNIT_1
#define SOIL_MOISTURE_ADC_CHANNEL ADC_CHANNEL_6

static const char *TAG = "NODO_SENSORES";

// Configuración Deep Sleep
#define WAKEUP_INTERVAL_SEC 60      // Despertar cada 60 segundos
#define MAX_WAKEUPS_BEFORE_TX 5     // Enviar forzosamente cada 5 despertares (5 mins)

// Umbrales para envío inmediato por cambio abrupto
#define THRESHOLD_TEMP 2.0   // 2 grados de diferencia
#define THRESHOLD_HUM  10    // 10% de diferencia

// Selección de Modo LoRa (0 = P2P, 1 = LoRaWAN)
#define USE_LORAWAN 0

// Variables retenidas en RTC (sobreviven al Deep Sleep)
RTC_DATA_ATTR int wake_count = 0;
RTC_DATA_ATTR float last_tx_temp = 0.0;
RTC_DATA_ATTR int last_tx_hum = 0;

// Valores de calibración del sensor de suelo
const int AIR_VALUE = 3500;
const int WATER_VALUE = 1500;

// Inicialización del bus I2C
static esp_err_t i2c_master_init(void) {
  i2c_config_t conf = {
      .mode = I2C_MODE_MASTER,
      .sda_io_num = I2C_MASTER_SDA_IO,
      .scl_io_num = I2C_MASTER_SCL_IO,
      .sda_pullup_en = GPIO_PULLUP_ENABLE,
      .scl_pullup_en = GPIO_PULLUP_ENABLE,
      .master.clk_speed = I2C_MASTER_FREQ_HZ,
  };
  i2c_param_config(I2C_MASTER_NUM, &conf);
  return i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}

void app_main(void) {
    ESP_LOGI(TAG, "Despertando de Deep Sleep... Wake count: %d", wake_count);
    wake_count++;

    // 1. Inicializar I2C y BMP280
    ESP_ERROR_CHECK(i2c_master_init());
    float temperature = 0, pressure = 0;
    if (bmp280_init(I2C_MASTER_NUM)) {
        bmp280_read(&temperature, &pressure);
        ESP_LOGI(TAG, "Temp: %.2f °C | Pres: %.2f hPa", temperature, pressure);
    } else {
        ESP_LOGE(TAG, "Error BMP280");
    }

    // 2. Inicializar ADC y leer Humedad de Suelo
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config1 = {.unit_id = SOIL_MOISTURE_ADC_UNIT};
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));
    adc_oneshot_chan_cfg_t config = {.bitwidth = ADC_BITWIDTH_DEFAULT, .atten = ADC_ATTEN_DB_12};
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, SOIL_MOISTURE_ADC_CHANNEL, &config));
    
    int adc_raw;
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, SOIL_MOISTURE_ADC_CHANNEL, &adc_raw));
    
    int soil_moisture_percent = 0;
    if (adc_raw > AIR_VALUE) soil_moisture_percent = 0;
    else if (adc_raw < WATER_VALUE) soil_moisture_percent = 100;
    else soil_moisture_percent = (AIR_VALUE - adc_raw) * 100 / (AIR_VALUE - WATER_VALUE);
    
    ESP_LOGI(TAG, "Humedad Suelo: %d %%", soil_moisture_percent);

    // 3. Inicializar RS485 y leer sensor Modbus (Ejemplo de esclavo ID 1)
    rs485_init();
    float rs485_val = -1; // -1 indica desconectado o error
    bool rs485_ok = rs485_read_sensor(1, 0x0000, &rs485_val);

    // 4. Lógica de Envío (Cada 5 mins o cambio abrupto)
    bool force_tx = false;
    if (wake_count >= MAX_WAKEUPS_BEFORE_TX) {
        force_tx = true;
        ESP_LOGI(TAG, "Transmisión forzada por tiempo.");
    } else if (fabs(temperature - last_tx_temp) >= THRESHOLD_TEMP) {
        force_tx = true;
        ESP_LOGI(TAG, "Transmisión forzada por cambio en temperatura.");
    } else if (abs(soil_moisture_percent - last_tx_hum) >= THRESHOLD_HUM) {
        force_tx = true;
        ESP_LOGI(TAG, "Transmisión forzada por cambio en humedad.");
    }

    if (force_tx) {
        // Formatear payload (JSON)
        char lora_payload[128];
        snprintf(lora_payload, sizeof(lora_payload), 
                 "{\"id\":\"NODO_1\",\"T\":%.2f,\"P\":%.2f,\"H\":%d,\"RS485\":%.1f}", 
                 temperature, pressure, soil_moisture_percent, rs485_val);
                 
        ESP_LOGI(TAG, "Enviando por LoRa: %s", lora_payload);

        // Inicializar LoRa y enviar
#if USE_LORAWAN
        ESP_LOGI(TAG, "Modo LoRaWAN habilitado. Asegurese de vincular el stack correcto.");
        // lorawan_send(...);
#else
        lora_init(SPI2_HOST, LORA_MISO_PIN, LORA_MOSI_PIN, LORA_SCK_PIN, LORA_CS_PIN, LORA_RST_PIN);
        lora_send_packet((uint8_t *)lora_payload, strlen(lora_payload));
#endif

        // Actualizar variables retenidas
        wake_count = 0;
        last_tx_temp = temperature;
        last_tx_hum = soil_moisture_percent;
    } else {
        ESP_LOGI(TAG, "Sin cambios abruptos. Omitiendo TX LoRa para ahorrar energía.");
    }

    // 5. Entrar a Deep Sleep
    ESP_LOGI(TAG, "Entrando a Deep Sleep por %d segundos...", WAKEUP_INTERVAL_SEC);
    esp_sleep_enable_timer_wakeup(WAKEUP_INTERVAL_SEC * 1000000ULL);
    esp_deep_sleep_start();
}
