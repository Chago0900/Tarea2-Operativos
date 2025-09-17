#!/bin/bash

SRC_FILES="main.c clasificador.c histogram.c stb_wrapper.c"
BIN_PATH="/usr/local/bin/imageserver"
SERVICE_FILE="/etc/systemd/system/imageserver.service"
DATA_DIR="/var/lib/imageserver"

if [[ $EUID -ne 0 ]]; then
   echo "Este script debe ejecutarse como root."
   exit 1
fi

echo "Compilando servidor de imágenes..."
gcc $SRC_FILES -o "$BIN_PATH" -lpthread -lm || { echo "❌ Falló la compilación."; exit 1; }
chmod +x "$BIN_PATH"

echo "Creando directorios de almacenamiento en $DATA_DIR..."
mkdir -p $DATA_DIR/{rojas,verdes,azules,filtrado}
chown -R root:root $DATA_DIR
chmod -R 755 $DATA_DIR

echo "Creando archivo de servicio systemd..."
cat > "$SERVICE_FILE" <<EOF
[Unit]
Description=Image Server Daemon
After=network.target

[Service]
Type=simple
ExecStart=$BIN_PATH
WorkingDirectory=$DATA_DIR
Restart=on-failure
User=root
Group=root

[Install]
WantedBy=multi-user.target
EOF

echo "Recargando systemd y habilitando servicio..."
systemctl daemon-reexec
systemctl daemon-reload
systemctl enable imageserver.service
systemctl restart imageserver.service

echo "Instalación completa. Estado del servicio:"
systemctl status imageserver.service --no-pager
