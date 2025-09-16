#!/bin/bash

# Definimos las rutas
BIN_PATH="/usr/local/bin/cpu_temp_daemon"
SERVICE_FILE="/etc/systemd/system/cpu_temp_logger.service"
LOG_FILE="/var/log/cpu_temp.log"
UPLOAD_SERVICE_FILE="/etc/systemd/system/cpu-temp-upload.service"
UPLOAD_TARGET="/etc/systemd/system/suspend.target.wants/cpu-temp-upload.service"

# Verificar si se está ejecutando como root
if [[ $EUID -ne 0 ]]; then
   echo "Este script debe ejecutarse como root."
   exit 1
fi

# Detener y deshabilitar el servicio
echo "Deteniendo el servicio 'cpu_temp_logger'..."
systemctl stop cpu_temp_logger.service
systemctl disable cpu_temp_logger.service

# Eliminar archivos del servicio
echo "Eliminando el archivo del servicio..."
rm -f "$SERVICE_FILE"

# Eliminar el archivo binario
echo "Eliminando el archivo binario de cpu_temp_daemon..."
rm -f "$BIN_PATH"

# Eliminar el archivo de log
echo "Eliminando el archivo de log..."
rm -f "$LOG_FILE"

# Eliminar servicio de subida (si existe)
if [[ -f "$UPLOAD_SERVICE_FILE" ]]; then
    echo "Eliminando el servicio de subida..."
    systemctl stop cpu-temp-upload.service
    systemctl disable cpu-temp-upload.service
    rm -f "$UPLOAD_SERVICE_FILE"
    rm -f "$UPLOAD_TARGET"
fi

# Recargar systemd
echo "Recargando systemd..."
systemctl daemon-reexec
systemctl daemon-reload

echo "Desinstalación completada. El daemon y los servicios han sido eliminados."
