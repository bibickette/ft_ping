simuler mauvaises conditions reseaux lo => localhost
sudo tc qdisc add dev lo root netem loss 30%
sudo tc qdisc add dev lo root netem delay 500ms
sudo tc qdisc add dev lo root netem delay 100ms 50ms
sudo tc qdisc add dev lo root netem delay 3000ms (tester late reply)

paquet dupplicata
sudo tc qdisc add dev lo root netem duplicate 100%
sudo tc qdisc replace dev lo root netem duplicate 50%

observer les paquets
sudo tcpdump -i lo -n -tt icmp

remettre
sudo tc qdisc del dev lo root

tc qdisc add dev lo root netem delay 500ms
│  │    │   │   │    │
│  │    │   │   │    └── ajoute 500 ms de délai
│  │    │   │   └────── netem = Network Emulator
│  │    │   └────────── interface lo
│  │    └────────────── ajoute
│  └─────────────────── qdisc = queueing discipline
└────────────────────── traffic control

dl le ping inetutils

wget https://ftp.gnu.org/gnu/inetutils/inetutils-2.0.tar.gz
tar -xzf inetutils-2.0.tar.gz
cd inetutils-2.0
./configure
make
./ping/ping google.com
