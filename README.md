Canva: https://www.canva.com/design/DAHMHQeCT2g/jDk2np1wXHK3mXcjj1Wg9A/edit


# Smart-Agri-Edge: Arquitectura de Red IoT con Computación en el Borde

## Descripción del Proyecto
Este proyecto propone y simula una arquitectura de red convergente para la agricultura de precisión, diseñada para superar las limitaciones de conectividad en entornos rurales. La solución implementa un ecosistema distribuido que integra **IoT (Internet of Things)**, **Edge Computing** y **Microsegmentación (VLAN)** para garantizar el control determinista de cultivos y la reducción crítica de la latencia en la toma de decisiones.

## Lógica de Funcionamiento
El sistema opera mediante una topología física en anillo que garantiza alta disponibilidad ante fallos físicos, gestionada lógicamente mediante protocolos de capa 2 para eliminar bucles. La lógica se divide en:
1.  **Capa de Percepción:** Nodos sensores (simulados vía ESP32) capturan variables ambientales.
2.  **Capa de Conmutación:** Segmentación de tráfico mediante VLANs para aislar telemetría de misión crítica de tráfico pesado (CCTV).
3.  **Capa de Procesamiento (Edge):** El servidor Edge (Raspberry Pi 5) procesa datos localmente mediante un broker MQTT, evitando la dependencia de latencias del Cloud para actuadores.
4.  **Capa de Backhaul:** Enrutamiento gestionado vía *Router-on-a-stick* hacia la red WAN/Cloud.

## Stack Tecnológico y Protocolos

| Capa | Protocolos / Tecnologías |
| :--- | :--- |
| **Física/Enlace** | LoRa/LoRaWAN, IEEE 802.1Q (VLAN), RSTP (IEEE 802.1w), LACP |
| **Red** | IPv4, ICMP, ACLs (Listas de Control de Acceso) |
| **Transporte** | TCP (para control/MQTT), UDP (para telemetría ligera) |
| **Aplicación** | MQTT (Broker Mosquitto), DHCP, DNS |

## Especificaciones de Hardware (Emuladas en GNS3)
* **Gateway IoT:** Contenedores Docker (Alpine Linux) con scripts de inyección en Python.
* **Switching:** MikroTik CHR (Cloud Hosted Router) configurado como switch de capa 2 (Root Bridge con RSTP).
* **Edge Node:** Servidor Docker (Ubuntu) con Broker MQTT.
* **Router WAN:** Cisco c7200 (Enrutamiento inter-VLAN).

## Tabla de Segmentación de Red (VLANs)

| VLAN | ID | Subred | Descripción |
| :--- | :--- | :--- | :--- |
| **IoT** | 10 | 10.0.10.0/23 | Telemetría sensores (MQTT) |
| **CCTV** | 20 | 10.0.20.0/24 | Videovigilancia (Aislado) |
| **Edge_Mgmt**| 30 | 10.0.30.0/24 | Servidor de procesamiento local |
| **Uplink** | 99 | 10.0.99.0/30 | Troncal hacia Router WAN |

## Métricas de Desempeño Objetivo
* **Latencia Promedio:** < 150 ms (Local).
* **Jitter:** < 15 ms.
* **Disponibilidad:** 99.9% mediante redundancia RSTP.
* **Convergencia ante fallos:** ~2 segundos.

---
*Desarrollado por: Paul A. Landeo A., Anderson V. Urbina S., Vladimir Cardena M.*
*E.P. Ingeniería de Telecomunicaciones - UNMSM*
