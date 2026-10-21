getconf NAME_MAX .

df -T .

MTU (Maximum Transmission Unit)

for fish:
set old_mtu (cat /sys/class/net/lo/mtu)
sudo tc qdisc add dev lo root tbf rate 20mbit burst 64kb latency 100ms
dd if=/dev/zero of=mtu-test.bin bs=1M count=32 status=progress
sudo ip link set dev lo mtu 9000
sudo tc qdisc del dev lo root
sudo ip link set dev lo mtu $old_mtu

| MTU (байт) | Время отправки (с) | Полное время (с) | Скорость (MB/s) |
|-----------:|-------------------:|-----------------:|----------------:|
| 9000       | 12.789             | 13.52            | 2.624           |
| 1500       | 13.580             | 14.34            | 2.471           |
| 1200       | 14.086             | 14.58            | 2.382           |
| 900        | 14.328             | 14.98            | 2.342           |
| 600        | 15.136             | 15.86            | 2.217           |
| 300        | 17.980             | 18.91            | 1.866           |
| 200        | 20.943             | 22.72            | 1.602           |
| 100        | 47.200             | 52.59            | 0.711           |
