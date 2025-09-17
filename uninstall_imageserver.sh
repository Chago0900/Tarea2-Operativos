#!/bin/bash

# Definimos las rutas
BIN_PATH="/usr/local/bin/imageserver"
SERVICE_FILE="/etc/systemd/system/imageserver.service"
LOG_FILE="/var/log/imageserver.log"
DATA_DIR="/var/lib/imageserver"

# Verificar si se está ejecutando como root
if [[ $EUID -ne 0 ]]; then
   echo "Este script debe ejecutarse como root."
   exit 1
fi

# Detener y deshabilitar el servicio
echo "Deteniendo el servicio 'imageserver'..."
systemctl stop imageserver.service
systemctl disable imageserver.service

# Eliminar archivo del servicio
echo "Eliminando el archivo del servicio..."
rm -f "$SERVICE_FILE"

# Eliminar el archivo binario
echo "Eliminando el binario de imageserver..."
rm -f "$BIN_PATH"

# Eliminar el archivo de log
if [[ -f "$LOG_FILE" ]]; then
    echo "Eliminando el archivo de log..."
    rm -f "$LOG_FILE"
fi

# Eliminar directorios de datos
if [[ -d "$DATA_DIR" ]]; then
    echo "Eliminando directorios de almacenamiento en $DATA_DIR..."
    rm -rf "$DATA_DIR"
fi

# Recargar systemd
echo "Recargando systemd..."
systemctl daemon-reexec
systemctl daemon-reload

echo "Desinstalación completada. El daemon y sus datos han sido eliminados."
