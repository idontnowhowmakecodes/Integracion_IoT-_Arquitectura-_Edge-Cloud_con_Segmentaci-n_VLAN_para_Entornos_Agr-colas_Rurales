FROM ubuntu:latest

# Instalar Mosquitto y herramientas de red útiles para el laboratorio
RUN apt-get update && apt-get install -y \
    mosquitto \
    mosquitto-clients \
    iproute2 \
    iputils-ping \
    net-tools \
    tcpdump \
    curl \
    nano \
    iperf3 \
    python3 \
    python3-paho-mqtt \
    && rm -rf /var/lib/apt/lists/*

# Configuración básica para permitir conexiones anónimas locales en Mosquitto
RUN echo "listener 1883" >> /etc/mosquitto/mosquitto.conf && \
    echo "allow_anonymous true" >> /etc/mosquitto/mosquitto.conf

# Exponer el puerto de MQTT
EXPOSE 1883

# Comando por defecto para mantener el contenedor vivo y permitir la consola interactiva en GNS3
CMD ["tail", "-f", "/dev/null"]

