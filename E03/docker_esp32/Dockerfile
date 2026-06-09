FROM alpine:latest

# Instalar utilidades de red básicas, python y pip
RUN apk update && apk add --no-cache \
    iproute2 \
    iputils \
    net-tools \
    tcpdump \
    busybox-extras \
    mosquitto-clients \
    python3 \
    py3-pip \
    iperf3 \
    && pip3 install --break-system-packages paho-mqtt || pip3 install paho-mqtt

# Copiar el script de simulación al contenedor
COPY simulador_esp32.py /simulador_esp32.py

# Comando por defecto para mantener el contenedor vivo en GNS3
CMD ["tail", "-f", "/dev/null"]
