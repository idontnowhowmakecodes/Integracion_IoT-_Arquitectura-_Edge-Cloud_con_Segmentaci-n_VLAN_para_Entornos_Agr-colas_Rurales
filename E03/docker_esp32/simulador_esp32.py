import paho.mqtt.client as mqtt
import time
import random

# Configuración de Red del Broker (Raspberry Pi en VLAN 30)
MQTT_BROKER = "10.0.30.5"
MQTT_PORT = 1883

import socket

# Obtener el nombre del nodo en GNS3 de forma automática
hostname = socket.gethostname()

# Tópicos agrícolas organizados jerárquicamente por nodo
TOPIC_TEMP = f"estaciones/{hostname}/temperatura"
TOPIC_HUM = f"estaciones/{hostname}/humedad"
TOPIC_SUELO = f"estaciones/{hostname}/humedad_Suelo"

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print(f"[+] Conectado exitosamente al Broker MQTT en el Edge Node (10.0.30.5) como {hostname}")
    else:
        print(f"[-] Falla en conexión. Código de error: {rc}")

# Inicializar cliente MQTT (Usando API VERSION1 para compatibilidad e ID único)
client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION1, f"ESP32_{hostname}")
client.on_connect = on_connect

# Conectar al broker Edge
print(f"Intentando conectar a {MQTT_BROKER}...")
try:
    client.connect(MQTT_BROKER, MQTT_PORT, 60)
except Exception as e:
    print(f"[-] Error al intentar conectar a {MQTT_BROKER}: {e}")
    exit(1)

client.loop_start()

try:
    while True:
        # Generar datos simulados de telemetría agrícola
        temperatura = round(random.uniform(15.0, 35.0), 2)
        humedad_rel = round(random.uniform(40.0, 80.0), 2)
        humedad_suelo = round(random.uniform(20.0, 60.0), 2) # %
        
        # Publicar datos
        client.publish(TOPIC_TEMP, str(temperatura), qos=0)
        client.publish(TOPIC_HUM, str(humedad_rel), qos=0)
        client.publish(TOPIC_SUELO, str(humedad_suelo), qos=0)
        
        print(f"Publicado -> Temp: {temperatura}°C | Hum: {humedad_rel}% | Suelo: {humedad_suelo}%")
        
        # Simular delay de muestreo de 5 segundos
        time.sleep(5)
except KeyboardInterrupt:
    print("\nSimulación detenida por el usuario.")
    client.loop_stop()
    client.disconnect()
