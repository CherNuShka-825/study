#!/bin/bash
set -e

echo "=== Базовая настройка Debian 12 ==="
export DEBIAN_FRONTEND=noninteractive

apt-get update && apt-get upgrade -y
apt-get install -y \
  curl wget vim git htop net-tools dnsutils \
  telnet tcpdump socat jq tree \
  bash-completion ca-certificates gnupg \
  lsb-release software-properties-common chrony \
  ufw xfsprogs

timedatectl set-timezone Europe/Moscow
systemctl enable chrony --now

# === Форматирование и монтирование дисков MinIO ===
for disk in sdb sdc; do
    dev="/dev/$disk"
    # Проверяем, является ли устройство блочным и не форматировано ли оно уже
    if [ -b "$dev" ] && ! blkid "$dev" > /dev/null 2>&1; then
        mkfs.xfs "$dev"
    fi
done

mkdir -p /data/disk1 /data/disk2

UUID1=$(blkid -s UUID -o value /dev/sdb 2>/dev/null || true)
UUID2=$(blkid -s UUID -o value /dev/sdc 2>/dev/null || true)

if [ -n "$UUID1" ] && ! grep -q "$UUID1" /etc/fstab; then
    echo "UUID=$UUID1 /data/disk1 xfs defaults 0 0" >> /etc/fstab
fi

if [ -n "$UUID2" ] && ! grep -q "$UUID2" /etc/fstab; then
    echo "UUID=$UUID2 /data/disk2 xfs defaults 0 0" >> /etc/fstab
fi

mount -a || true

# === High-load оптимизации ===
cat >> /etc/sysctl.conf << 'EOF'
net.core.somaxconn = 65535
net.core.netdev_max_backlog = 65535
net.ipv4.tcp_max_syn_backlog = 65535
net.ipv4.tcp_fin_timeout = 30
net.ipv4.tcp_tw_reuse = 1
net.ipv4.tcp_keepalive_time = 1200
net.ipv4.ip_local_port_range = 1024 65535
net.ipv4.tcp_rmem = 4096 87380 33554432
net.ipv4.tcp_wmem = 4096 65536 33554432
net.core.rmem_max = 33554432
net.core.wmem_max = 33554432
EOF
sysctl -p

# === Firewall: базовые правила ===
ufw --force reset
ufw default deny incoming
ufw default allow outgoing
ufw allow ssh
# Открываем порты для MinIO (API и Console), если планируете их использовать снаружи
ufw allow 9000/tcp
ufw allow 9001/tcp
ufw --force enable

# === SSH (ИСПРАВЛЕНО: строки объединены) ===
sed -i 's|^#PermitRootLogin.*|PermitRootLogin yes|' /etc/ssh/sshd_config
sed -i 's|^#PasswordAuthentication.*|PasswordAuthentication yes|' /etc/ssh/sshd_config
systemctl restart sshd

echo "root:debian12" | chpasswd
if ! id "appuser" &>/dev/null; then
    useradd -m -s /bin/bash appuser
fi
echo "appuser:appuser" | chpasswd
usermod -aG sudo appuser

echo "=== Базовая настройка завершена ==="
