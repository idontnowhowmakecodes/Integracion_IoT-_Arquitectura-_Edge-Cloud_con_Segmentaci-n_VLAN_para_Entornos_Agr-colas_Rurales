# Guía Paso a Paso: Topología IoT en GNS3

## 1. Preparación de Nodos en GNS3

### A. Cargar Nodos Docker
1. En GNS3, ve a **Edit > Preferences > Docker containers**.
2. Haz clic en **New**.
3. Selecciona **New image** y escribe `ubuntu:latest` para la Raspberry Pi. Termina el asistente (puedes llamarlo `Edge_Node`).
4. Repite el proceso con la imagen `alpine:latest` para el ESP32. Llámalo `ESP32_Node`.
5. *(Opcional)* Si prefieres usar los Dockerfiles locales generados en este proyecto, deberás compilarlos primero en tu máquina usando `docker build -t mi_edge docker_edge/` y luego seleccionar `mi_edge` en GNS3.

### B. Cargar Router Cisco c7200
1. Ve a **Edit > Preferences > IOS routers**.
2. Clic en **New** y busca la imagen del IOS Cisco c7200 que tienes en tu PC.
3. Sigue el asistente, asegúrate de añadirle tarjetas de red (ej. `PA-GE` o `PA-FE-TX` en el slot 0) para tener puertos FastEthernet o GigabitEthernet.

### C. Nodos de Switching
Si estás usando `Cisco IOSvL2`, cárgalo desde la sección de QEMU VMs. Si usas los switches genéricos, arrastra 3 nodos `Ethernet switch` al área de trabajo.

## 2. Topología y Cableado

1. Arrastra 3 Switches al centro de tu topología.
2. Arrastra 1 Router c7200 a la parte superior.
3. Arrastra 1 contenedor `Edge_Node` (Ubuntu) y al menos 1 contenedor `ESP32_Node` (Alpine).
4. Usa la herramienta **Add a link** (el cable de red a la izquierda):
   - Conecta el switch 1 con el switch 2, el switch 2 con el 3, y el 3 con el 1 (esto forma el anillo físico).
   - Conecta el Switch 1 (el que actuará de Distribución) al puerto `FastEthernet0/0` del Router c7200.
   - Conecta el `Edge_Node` al Switch 1.
   - Conecta el `ESP32_Node` al Switch 2 o 3.

## 3. Configuración de Dispositivos de Red

### Router c7200
1. Enciende el router c7200 (clic derecho > **Start**).
2. Abre su consola (clic derecho > **Console**).
3. Abre el archivo `configs/router_c7200.txt` en tu computadora, copia su contenido, y pégalo directamente en la consola del router. Presiona Enter.

### Switches (MikroTik)
1. Enciende los switches y abre sus consolas.
2. Basándote en el archivo `configs/mikrotik_switches.txt`, configura cada switch copiando y pegando el bloque de comandos correspondiente para **SW-Core**, **SW-Acc-A** y **SW-Acc-B**.
3. **Importante:** El Switch Core (`SW-Core`) tiene configurada una prioridad STP de `4096` para convertirse automáticamente en el Root Bridge (Puente Raíz). Los puertos que van hacia los contenedores tienen configurado `edge=yes` (el equivalente a PortFast en Cisco) para acelerar la transición de estados.

## 4. Configuración de Contenedores y Pruebas

### Configuración IP Estática (ESP32 / Alpine)
Existen dos formas de configurarlo:

**Opción A: Temporal (vía consola):**
1. Inicia el contenedor del ESP32 en GNS3 y abre su consola.
2. Escribe el comando para asignar la IP (o ejecuta el script `configs/esp32_setup.sh`):
   ```bash
   ip addr add 10.0.10.5/23 dev eth0
   ip link set eth0 up
   ip route add default via 10.0.10.1
   ```

**Opción B: Persistente en GNS3 (Recomendado):**
1. Haz clic derecho sobre el nodo **ESP32_Node** en GNS3 (estando apagado) y selecciona **Edit config**.
2. Configura las interfaces para que quede de la siguiente forma:
   ```text
   # Static config for eth0
   auto eth0
   iface eth0 inet static
       address 10.0.10.5
       netmask 255.255.254.0
       gateway 10.0.10.1
   ```
3. Guarda los cambios y enciende el nodo.

### Configuración IP Estática (Edge / Ubuntu)
**Opción A: Temporal (vía consola):**
1. Inicia el contenedor Edge y abre su consola.
2. Asigna la IP (o ejecuta el script `configs/edge_setup.sh`):
   ```bash
   ip addr add 10.0.30.5/24 dev eth0
   ip link set eth0 up
   ip route add default via 10.0.30.1
   ```

**Opción B: Persistente en GNS3 (Recomendado):**
1. Haz clic derecho sobre el nodo **Edge_Node** en GNS3 (estando apagado) y selecciona **Edit config**.
2. Edita el archivo `/etc/network/interfaces` de la siguiente forma:
   ```text
   # Static config for eth0
   auto eth0
   iface eth0 inet static
       address 10.0.30.5
       netmask 255.255.255.0
       gateway 10.0.30.1
   ```
3. Guarda los cambios y enciende el nodo.

### Prueba de Fuego (Ping y MQTT)
1. Desde la consola del ESP32, lanza un ping a la Raspberry Pi:
   ```bash
   ping 10.0.30.5
   ```
2. Si el ping responde, ¡el enrutamiento Inter-VLAN a través del anillo está funcionando!
3. Abre una consola en el **Edge Node** y suscríbete a todos los tópicos MQTT para ver llegar los mensajes:
   ```bash
   mosquitto_sub -h localhost -t "#" -v
   ```
4. Abre la consola del **ESP32_Node** y corre el simulador de telemetría en Python (ya incorporado en la imagen Docker):
   ```bash
   python3 /simulador_esp32.py
   ```
   *Deberías ver cómo el script comienza a generar lecturas de temperatura, humedad y humedad de suelo, y las publica cada 5 segundos hacia el Edge Node (10.0.30.5). En la consola del Edge Node verás aparecer las publicaciones.*
