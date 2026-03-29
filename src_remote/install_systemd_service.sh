#!/usr/bin/env bash
set -euo pipefail

SERVICE_NAME="remote-serial-gateway.service"
SRC_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SERVICE_SRC="${SRC_DIR}/remote_serial_gateway.service"
SERVICE_DST="/etc/systemd/system/${SERVICE_NAME}"

if [[ $EUID -ne 0 ]]; then
  echo "Please run with sudo: sudo ./install_systemd_service.sh"
  exit 1
fi

if [[ ! -f "${SERVICE_SRC}" ]]; then
  echo "Service template not found: ${SERVICE_SRC}"
  exit 1
fi

cp "${SERVICE_SRC}" "${SERVICE_DST}"
systemctl daemon-reload
systemctl enable "${SERVICE_NAME}"
systemctl restart "${SERVICE_NAME}"

echo "Installed and restarted ${SERVICE_NAME}"
echo
echo "Check status: sudo systemctl status ${SERVICE_NAME}"
echo "View logs:    sudo journalctl -u ${SERVICE_NAME} -f"
