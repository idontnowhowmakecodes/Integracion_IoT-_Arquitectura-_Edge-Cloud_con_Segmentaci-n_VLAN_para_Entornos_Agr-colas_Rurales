#!/bin/sh
# Script de configuración de red para el Nodo ESP32 (Alpine)
# Ejecutar en la consola del contenedor si se reinicia y pierde la red.

echo "Configurando IP estática para Nodo ESP32..."
ip addr add 10.0.10.5/23 dev eth0
ip link set eth0 up

echo "Configurando puerta de enlace predeterminada..."
ip route add default via 10.0.10.1

echo "Configuración completada. IP actual:"
ip addr show eth0
echo "Tabla de rutas actual:"
ip route show
