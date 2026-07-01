Canva: [https://canva.link/q87jq8xrtzpqt2w](https://canva.link/0amamo7nqnjhizh)

# Diseño, Simulación e Implementación de una Red de Computadoras Integrando: IoT, Arquitectura Edge-Cloud con Segmentación VLAN para Entornos Agrícolas Rurales 

## Descripción del Proyecto
Este proyecto propone y simula una arquitectura de red convergente para la agricultura de precisión, diseñada para superar las limitaciones de conectividad en entornos rurales. La solución implementa un ecosistema distribuido que integra **IoT (Internet of Things)**, **Edge Computing** y **Microsegmentación (VLAN)** para garantizar el control determinista de cultivos y la reducción crítica de la latencia en la toma de decisiones.

## 1. Problema de Investigación
* **Latencia Cloud:** La dependencia de servidores remotos genera retardos (300-350 ms) que impiden el control en tiempo real de actuadores críticos (ej. sistemas de riego).
* **Saturación LAN:** Las redes rurales operan con topologías planas, causando tormentas de *broadcast* donde el tráfico crítico de sensores compite ineficientemente con flujos pesados (CCTV).
* **Vulnerabilidad Física:** Falta de redundancia frente a cortes de cableado por maquinaria agrícola.

## 2. Objetivos y Preguntas de Investigación
* **Objetivo General:** Implementar una arquitectura convergente (Edge + VLAN + RSTP) para asegurar fiabilidad en el control IoT.
* **Preguntas:** ¿En qué medida la computación *Edge* reduce la latencia comparada con la nube? ¿Cómo impacta la microsegmentación VLAN en la eficiencia del *throughput* en entornos con videovigilancia? ¿Cuál es el tiempo real de recuperación ante fallos físicos usando RSTP?

## 3. Estado del Arte
* **Edge-Cloud:** La literatura indica una reducción de latencia de 315 ms a ~48 ms al procesar localmente.
* **LPWAN:** Solución clave para larga distancia y bajo consumo, aunque con brechas en su integración con redes LAN segmentadas.
* **VLAN/RSTP:** Estándares probados en industria que, en este proyecto, se adaptan para mitigar el tráfico de difusión en redes agrarias rurales.

## 4. Marco Teórico Aplicado
* **Modelo de Capas IoT:** Adaptación del stack TCP/IP donde la Capa de Percepción integra LoRaWAN, y el núcleo de red aplica VLANs (IEEE 802.1Q) para aislamiento lógico.
* **Resiliencia:** Uso de RSTP (IEEE 802.1w) para convergencia en <50ms, superando las limitaciones del STP tradicional (30s).
* **Control Determinista:** Implementación de MQTT bajo un modelo de publicación-suscripción para optimización de ancho de banda.

## 5. Lógica de Funcionamiento
El sistema utiliza una topología física en **anillo** (para resiliencia) con lógica de **estrella** (árbol libre de bucles). El tráfico se segmenta en VLANs:
1.  **VLAN 10:** Telemetría (IoT).
2.  **VLAN 20:** Videovigilancia (Aislado).
3.  **VLAN 30:** Gestión Edge (Procesamiento local).
4.  **VLAN 99:** *Uplink* troncal.

## Stack Tecnológico y Protocolos
| Capa | Protocolos / Tecnologías |
| :--- | :--- |
| **Física/Enlace** | LoRa/LoRaWAN, IEEE 802.1Q (VLAN), RSTP (IEEE 802.1w), LACP |
| **Red** | IPv4, ICMP, ACLs (Listas de Control de Acceso) |
| **Transporte** | TCP (control), UDP (telemetría ligera) |
| **Aplicación** | MQTT (Broker Mosquitto), DHCP, DNS |

## Tabla de Segmentación de Red (VLANs)
| VLAN | ID | Subred | Descripción |
| :--- | :--- | :--- | :--- |
| **IoT** | 10 | 10.0.10.0/23 | Telemetría sensores (MQTT) |
| **CCTV** | 20 | 10.0.20.0/24 | Videovigilancia (Aislado) |
| **Edge_Mgmt**| 30 | 10.0.30.0/24 | Servidor de procesamiento local |
| **Uplink** | 99 | 10.0.99.0/30 | Troncal hacia Router WAN |

---
*Desarrollado por: Paul A. Landeo A., Anderson V. Urbina S., Vladimir Cardena M.*
*E.P. Ingeniería de Telecomunicaciones - UNMSM*
