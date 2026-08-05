simuler mauvaises conditions reseaux lo => localhost
sudo tc qdisc add dev lo root netem loss 30%
sudo tc qdisc add dev lo root netem delay 500ms
sudo tc qdisc add dev lo root netem delay 100ms 50ms
sudo tc qdisc add dev lo root netem delay 3000ms (tester -W)

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




TEST MANDATORY :
help
sudo ./ft_ping -h localhost
sudo ./ft_ping -help localhost
sudo ./ft_ping --help localhost
sudo ./ft_ping -? localhost
sudo ./ft_ping "-?" localhost
sudo ./ft_ping -dntexist localhost
sudo ./ft_ping -h localhost pouet

verbose
sudo ./ft_ping -v localhost
sudo ./ft_ping -v localhost pouet
sudo ./ft_ping -v 


normal environment
sudo ./ft_ping -v localhost
sudo ./ft_ping 8.8.8.8
sudo ./ft_ping google.com

double ping
sudo ./ft_ping google.com
sudo ./ft_ping localhost


package loss
sudo ./ft_ping -v 10.255.255.1


no internet
sudo ./ft_ping google.com

loss environment
sudo tc qdisc add dev lo root netem loss 30%
sudo ./ft_ping localhost


delay environment
sudo tc qdisc del dev lo root
sudo tc qdisc add dev lo root netem delay 500ms
sudo tc qdisc add dev lo root netem delay 100ms 50ms
sudo tc qdisc add dev lo root netem delay 1500ms
sudo tc qdisc add dev lo root netem delay 10000ms
sudo ./ft_ping localhost

duplicate environment
sudo tc qdisc del dev lo root
sudo tc qdisc add dev lo root netem duplicate 100%
sudo tc qdisc replace dev lo root netem duplicate 50%

packet not in right order
sudo tc qdisc add dev lo root netem delay 100ms 1000ms distribution normal

QUE FAIT -W EXACTEMENT ?
Timeout pour select, si rien ne se passe apres -W secondes, le programme s'arrete
si des paquets continue d'etre envoyés, le programme continue et les recoit
si il y a -c, le programme sarreter apres -c paquet envoyé puis -W s attendue
COMMENT TESTER ?
sudo tc qdisc add dev lo root netem delay 15000ms
offre un delay aller 15s + retour 15s sur le reseau
le -W de base est de 10s
pour tester correctement, il faut envoyer un nombre de paquet exemple 1

./inetutils-2.0/ping/ping -c 1 localhost
-> le programme va sarreter a 10s (W de base), il naura pas le temps de recevoir le paquet puisquil prends 30s
./inetutils-2.0/ping/ping -c 1 -W 1 localhost
-> le programme va sarreter a 1s, car le select attendra 1s et ne recevra rien
./inetutils-2.0/ping/ping -W 1 localhost
-> le programme recoit les ping en delay et cest > à 1s

