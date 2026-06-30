#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_eth.h"
#include "esp_eth_enc28j60.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lora.h"
#include "mqtt_client.h"
#include "nvs_flash.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "GATEWAY_MAIN";

// ======================================================================
// CONFIGURACIÓN DE PINES
// ======================================================================

// Pines SPI para el módulo LoRa SX1278 en ESP32-S3 (SPI2_HOST)
#define LORA_MISO_PIN 13
#define LORA_MOSI_PIN 11
#define LORA_SCK_PIN 12
#define LORA_CS_PIN 10
#define LORA_RST_PIN 9

// Pines SPI para el módulo Ethernet ENC28J60 en ESP32-S3 (SPI3_HOST)
#define ETH_MISO_PIN 37 // SO
#define ETH_MOSI_PIN 35 // SI
#define ETH_SCK_PIN 36  // SCK
#define ETH_CS_PIN 34   // CS
#define ETH_INT_PIN 33  // INT
#define ETH_RST_PIN 38  // RESET

// Broker MQTT (IP de la Raspberry Pi u Orange Pi)
#define MQTT_BROKER_URI "mqtt://10.0.30.1"

// ======================================================================
// SELECCIÓN DE MODO LORA
// ======================================================================
// Cambia a 1 para usar una implementación LoRaWAN (Requiere librería externa o
// stack LoRaWAN MAC) Actualmente configurado a 0 para usar LoRa P2P con el
// SX1278
#define USE_LORAWAN 0

static esp_mqtt_client_handle_t mqtt_client = NULL;
static bool mqtt_connected = false;

// ======================================================================
// MANEJADORES DE EVENTOS
// ======================================================================

static void eth_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data) {
  uint8_t mac_addr[6] = {0};
  esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;

  switch (event_id) {
  case ETHERNET_EVENT_CONNECTED:
    esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
    ESP_LOGI(TAG, "Ethernet Enlace Arriba. MAC: %02x:%02x:%02x:%02x:%02x:%02x",
             mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4],
             mac_addr[5]);
    break;
  case ETHERNET_EVENT_DISCONNECTED:
    ESP_LOGI(TAG, "Ethernet Enlace Abajo");
    break;
  case ETHERNET_EVENT_START:
    ESP_LOGI(TAG, "Ethernet Iniciado");
    break;
  case ETHERNET_EVENT_STOP:
    ESP_LOGI(TAG, "Ethernet Detenido");
    break;
  default:
    break;
  }
}

static void got_ip_event_handler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data) {
  ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
  const esp_netif_ip_info_t *ip_info = &event->ip_info;
  ESP_LOGI(TAG, "Ethernet Got IP Address");
  ESP_LOGI(TAG, "~~~~~~~~~~~");
  ESP_LOGI(TAG, "IP          : " IPSTR, IP2STR(&ip_info->ip));
  ESP_LOGI(TAG, "Netmask     : " IPSTR, IP2STR(&ip_info->netmask));
  ESP_LOGI(TAG, "Gateway     : " IPSTR, IP2STR(&ip_info->gw));
  ESP_LOGI(TAG, "~~~~~~~~~~~");
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data) {
  esp_mqtt_event_handle_t event = event_data;
  switch ((esp_mqtt_event_id_t)event_id) {
  case MQTT_EVENT_CONNECTED:
    ESP_LOGI(TAG, "MQTT Conectado al broker");
    mqtt_connected = true;
    break;
  case MQTT_EVENT_DISCONNECTED:
    ESP_LOGI(TAG, "MQTT Desconectado");
    mqtt_connected = false;
    break;
  default:
    break;
  }
}

// ======================================================================
// INICIALIZACIONES
// ======================================================================

void init_ethernet_enc28j60(void) {
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_config_t netif_cfg = ESP_NETIF_DEFAULT_ETH();
  esp_netif_t *eth_netif = esp_netif_new(&netif_cfg);

  spi_bus_config_t buscfg = {
      .miso_io_num = ETH_MISO_PIN,
      .mosi_io_num = ETH_MOSI_PIN,
      .sclk_io_num = ETH_SCK_PIN,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
  };
  ESP_ERROR_CHECK(spi_bus_initialize(SPI3_HOST, &buscfg, SPI_DMA_CH_AUTO));

  spi_device_interface_config_t spi_devcfg = {
      .mode = 0,
      .clock_speed_hz = 8000000,
      .spics_io_num = ETH_CS_PIN,
      .queue_size = 20,
  };

  eth_enc28j60_config_t enc28j60_config =
      ETH_ENC28J60_DEFAULT_CONFIG(SPI3_HOST, &spi_devcfg);
  enc28j60_config.int_gpio_num = ETH_INT_PIN;

  eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
  esp_eth_mac_t *mac = esp_eth_mac_new_enc28j60(&enc28j60_config, &mac_config);

  eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
  phy_config.phy_addr = -1; // Default
  phy_config.reset_gpio_num = ETH_RST_PIN;
  esp_eth_phy_t *phy = esp_eth_phy_new_enc28j60(&phy_config);

  esp_eth_config_t eth_config = ETH_DEFAULT_CONFIG(mac, phy);
  esp_eth_handle_t eth_handle = NULL;
  ESP_ERROR_CHECK(esp_eth_driver_install(&eth_config, &eth_handle));

  // El ESP32-S3 a veces requiere simular la MAC para ENC28J60
  uint8_t mac_addr[6] = {0x02, 0x00, 0x00, 0x12, 0x34, 0x56};
  esp_eth_ioctl(eth_handle, ETH_CMD_S_MAC_ADDR, mac_addr);

  ESP_ERROR_CHECK(
      esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handle)));
  ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID,
                                             &eth_event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP,
                                             &got_ip_event_handler, NULL));

  ESP_ERROR_CHECK(esp_eth_start(eth_handle));
}

void init_mqtt(void) {
  esp_mqtt_client_config_t mqtt_cfg = {
      .broker.address.uri = MQTT_BROKER_URI,
  };
  mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
  esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID,
                                 mqtt_event_handler, NULL);
  esp_mqtt_client_start(mqtt_client);
}

// ======================================================================
// TAREAS PRINCIPALES
// ======================================================================

void lora_rx_task(void *pvParameters) {
  uint8_t buf[256];
  while (1) {
#if USE_LORAWAN
    // Placeholder para LoRaWAN Rx (usaría eventos callbacks normalmente en
    // stacks como LMIC o esp-lorawan) int len = lorawan_receive(...);
    vTaskDelay(pdMS_TO_TICKS(1000));
#else
    // LoRa P2P Raw Recive
    int len = lora_receive_packet(buf, sizeof(buf) - 1);
    if (len > 0) {
      buf[len] = '\0';
      int rssi = lora_packet_rssi();

      ESP_LOGI(TAG, "==== PAQUETE LORA RECIBIDO ====");
      ESP_LOGI(TAG, "RSSI: %d dBm", rssi);
      ESP_LOGI(TAG, "Payload: %s", (char *)buf);

      if (mqtt_connected && mqtt_client != NULL) {
        // Publicar al Raspberry Pi
        // Formato asumido del Payload: {"id": "NODO_1", "hum": 45, ...}
        esp_mqtt_client_publish(mqtt_client, "agricola/gateway/rx", (char *)buf,
                                len, 0, 0);
        ESP_LOGI(TAG, "-> Reenviado por MQTT a agricola/gateway/rx");
      } else {
        ESP_LOGW(TAG, "-> MQTT No conectado, paquete descartado");
      }
      ESP_LOGI(TAG, "===============================");
    }
#endif
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void app_main(void) {
  ESP_ERROR_CHECK(nvs_flash_init());
  ESP_LOGI(TAG, "Iniciando Gateway...");

  // 1. Inicializar Ethernet y conectarse al Broker MQTT
  ESP_LOGI(TAG, "Inicializando Ethernet ENC28J60...");
  init_ethernet_enc28j60();

  ESP_LOGI(TAG, "Inicializando MQTT...");
  init_mqtt();

  // 2. Inicializar LoRa
#if USE_LORAWAN
  ESP_LOGI(TAG,
           "Modo LoRaWAN habilitado. Asegurese de vincular el stack correcto.");
  // lorawan_init(...);
#else
  ESP_LOGI(TAG, "Modo LoRa P2P habilitado.");
  lora_init(SPI2_HOST, LORA_MISO_PIN, LORA_MOSI_PIN, LORA_SCK_PIN, LORA_CS_PIN,
            LORA_RST_PIN);
  xTaskCreate(lora_rx_task, "lora_rx_task", 4096, NULL, 5, NULL);
#endif
}
