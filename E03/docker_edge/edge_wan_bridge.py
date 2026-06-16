import paho.mqtt.client as mqtt
import time
import json
import threading

LOCAL_BROKER = "127.0.0.1"
LOCAL_TOPIC = "estaciones/#"

# --- Configuración Remota (WAN / Cloud) ---
# Usamos un broker público gratuito en Internet para simular la nube de AWS/Azure
CLOUD_BROKER = "broker.emqx.io" 
CLOUD_TOPIC = "tesis/agricultura/edge/resumen"

datos_acumulados = []

# Función que se ejecuta cuando el Edge recibe datos del ESP32
def on_local_message(client, userdata, msg):
    try:
        valor = float(msg.payload.decode())
        datos_acumulados.append(valor)
        print(f"[LAN] Recibido del ESP32: {valor}")
    except ValueError:
        pass

# Función que calcula promedios y envía a Internet cada 10 segundos
def procesar_y_enviar_nube():
    cloud_client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION1, "Edge_WAN_Bridge")
    try:
        # Conectar a Internet
        cloud_client.connect(CLOUD_BROKER, 1883, 60)
        while True:
            time.sleep(10) # Frecuencia de envío WAN (cada 10 seg)
            
            if datos_acumulados:
                # Procesamiento Edge: Calcular promedio
                promedio = sum(datos_acumulados) / len(datos_acumulados)
                cantidad_mensajes = len(datos_acumulados)
                datos_acumulados.clear() # Limpiar arreglo
                
                # Crear paquete JSON empaquetado
                payload = json.dumps({
                    "nodo": "Edge_Agricola_1",
                    "temp_promedio": round(promedio, 2), 
                    "muestras_procesadas": cantidad_mensajes
                })
                
                # Enviar a Internet
                cloud_client.publish(CLOUD_TOPIC, payload)
                print(f"\n[WAN] ---> Resumen enviado a la Nube: {payload}\n")
    except Exception as e:
        print(f"Error conectando a la WAN: {e}")

# --- Configuración del suscriptor local ---
local_client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION1, "Edge_Local_Subscriber")
local_client.on_message = on_local_message
local_client.connect(LOCAL_BROKER, 1883, 60)
local_client.subscribe(LOCAL_TOPIC)

# Iniciar el proceso de envío a la nube en paralelo
hilo_cloud = threading.Thread(target=procesar_y_enviar_nube, daemon=True)
hilo_cloud.start()

print("[EDGE] Iniciando puente Local -> WAN...")
print("Recopilando datos de la LAN y enviando a broker.emqx.io cada 10s...")
local_client.loop_forever()

