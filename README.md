simuler mauvaises conditions reseaux lo => localhost
sudo tc qdisc add dev lo root netem loss 30%
sudo tc qdisc add dev lo root netem delay 500ms
sudo tc qdisc add dev lo root netem delay 100ms 50ms
sudo tc qdisc add dev lo root netem delay 3000ms (tester -W)

paquet dupplicata
sudo tc qdisc add dev lo root netem duplicate 100%
sudo tc qdisc replace dev enp0s1 root netem duplicate 50%

corrupt
sudo tc qdisc add dev lo root netem corrupt 50%

observer les paquets
sudo tcpdump -i lo -n -tt icmp
sudo tcpdump -i enp0s1 -n -vv icmp

icmp -> filtre les packets icmp 4

remettre
sudo tc qdisc del dev lo root

voir
sudo tc qdisc show
sudo tc qdisc show dev enp0s1


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


my original ping : iputils-ping
ping in my machine : ping (GNU inetutils) 2.0

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
sudo tc qdisc add dev lo root netem delay 500ms
sudo tc qdisc add dev lo root netem delay 100ms 50ms
sudo tc qdisc add dev lo root netem delay 1500ms
sudo tc qdisc add dev lo root netem delay 10000ms

duplicate environment
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
-> le programme va sarreter a 1s apres le dernier paquet envoyé
./inetutils-2.0/ping/ping -W 1 localhost
-> le programme recoit les ping en delay et cest > à 1s











Oui. tc est un outil Linux qui permet de contrôler la façon dont les paquets sont mis en file et transmis par une interface réseau. Avec netem, tu peux volontairement ajouter du délai, de la perte, de la corruption, des duplications, etc. C'est parfait pour tester ton ping.

Prenons :

sudo tc qdisc add dev lo root netem delay 3000ms

Décomposons :

tc

C'est Traffic Control.

Il permet de configurer les queuing disciplines (QDisc), c'est-à-dire la manière dont Linux gère les paquets qui passent par une interface réseau.

En gros :

ton programme
    ↓
socket
    ↓
réseau Linux
    ↓
[ QDisc / tc ]
    ↓
interface réseau
    ↓
réseau

tc te permet donc d'insérer des comportements artificiels à cet endroit.

qdisc

Abréviation de queueing discipline.

tc qdisc

signifie que tu veux manipuler la discipline de file d'attente d'une interface.

Tu peux par exemple avoir :

pfifo
fq_codel
htb
netem

Dans ton cas tu utilises netem.

add
tc qdisc add

Ça veut dire :

Ajoute une nouvelle qdisc.

Donc :

tc qdisc add ...

≠ modifier une qdisc existante.

Pour modifier une qdisc existante, tu peux utiliser change :

tc qdisc change ...
dev lo

dev signifie device, donc l'interface réseau concernée.

dev lo

veut dire :

applique cette configuration à l'interface lo.

lo est l'interface loopback :

127.0.0.1
    ↕
   lo

C'est pour ça que tes tests avec :

ping localhost

sont affectés.

Pour voir tes interfaces :

ip link

Tu verras par exemple :

lo
enp0s1
eth0
...
root

C'est probablement le morceau le plus important à comprendre.

root

veut dire :

cette qdisc devient la qdisc principale (racine) de cette interface.

Tu peux imaginer une arborescence :

                 interface lo
                     │
                    root
                     │
                  netem
                 /  |  \
              delay loss duplicate

Quand tu fais :

tc qdisc add dev lo root netem ...

tu demandes donc :

Mets netem comme qdisc racine de lo.

netem

netem signifie Network Emulator.

C'est lui qui permet de simuler des problèmes réseau.

Par exemple :

netem delay 100ms

→ ajoute du délai.

netem loss 20%

→ perd environ 20 % des paquets.

netem duplicate 50%

→ duplique des paquets.

netem corrupt 50%

→ corrompt des paquets.

Tu peux même combiner :

sudo tc qdisc add dev lo root netem delay 100ms loss 10%
delay 3000ms

Enfin :

delay 3000ms

est le paramètre de netem.

Ça signifie :

introduis environ 3 secondes de délai.

Donc :

sudo tc qdisc add dev lo root netem delay 3000ms

donne :

                  lo
                   │
                   ▼
              ┌─────────┐
              │  netem  │
              │         │
              │ delay   │
              │ 3000 ms │
              └────┬────┘
                   │
                   ▼
              paquet transmis

Et dans tes tests ping

Quand tu fais :

sudo tc qdisc add dev lo root netem delay 3000ms

tu ne modifies pas le paquet ICMP lui-même.

Tu modifies le comportement de l'interface réseau :

            ping
            │
            │ ICMP Echo Request
            ▼
        ┌──────────────┐
        │ Linux / lo   │
        │              │
        │    netem     │ ← attend ~3 s
        │    delay     │
        └──────┬───────┘
            │
            ▼
            paquet

C'est pour ça que tcpdump est très utile : tu peux voir à quel moment le paquet entre/sort réellement.

Petit résumé

Pour :

sudo tc qdisc add dev lo root netem delay 3000ms
Élément	Signification
tc	Traffic Control
qdisc	Queueing Discipline
add	Ajouter
dev	Interface réseau
lo	Interface loopback
root	Qdisc racine de l'interface
netem	Network Emulator
delay	Ajouter un délai
3000ms	3 secondes

Et pour remettre lo comme avant :

sudo tc qdisc del dev lo root



on remarque que ping count cest package sent and package received + duplicata