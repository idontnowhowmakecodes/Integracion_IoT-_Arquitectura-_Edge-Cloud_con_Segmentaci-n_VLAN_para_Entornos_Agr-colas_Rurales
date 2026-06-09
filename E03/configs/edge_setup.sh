#!/bin/sh
# Script de configuración de red para el Nodo Edge (Ubuntu)
# Ejecutar en la consola del contenedor si se reinicia y pierde la red.

echo "Configurando IP estática para Nodo Edge..."
ip addr add 10.0.30.5/24 dev eth0
ip link set eth0 up

echo "Configurando puerta de enlace predeterminada..."
ip route add default via 10.0.30.1

echo "Configuración completada. IP actual:"
ip addr show eth0
echo "Tabla de rutas actual:"
ip route show
