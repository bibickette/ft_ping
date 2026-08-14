# readme organization

# description :

`ft_ping` est un projet en C qui consiste à réimplémenter le programme `ping` de inetutils-2.0. `ping` a pour objectif principal de vérifier qu'une machine est joignable sur un réseau et mesurer le temps nécessaire pour obtenir une réponse.

`ping` est un outil permettant principalement de :
- vérifier qu'une destination est joignable 
- vérifier qu'elle répond aux requêtes ICMP 
- mesurer le RTT (Round Trip Time : temps entre l'envoi de l'Echo Request et la réception de l'Echo Reply)
- observer la perte de paquets 
- obtenir des informations comme le TTL dans la réponse

Exemple :
``` sh
ping 8.8.8.8 # IP adress of Google DNS

        ICMP Echo Request
   ┌─────────────────────────►
   │                         │
   │                      8.8.8.8
   │                         │
PC │                         │
   │                      répond
   │                         │
   ◄─────────────────────────┘
        ICMP Echo Reply
```

## norme de communication de tous les systèmes informatiques en réseau (modele osi)  

Le modèle OSI (Open Systems Interconnection) permet de représenter les différentes fonctions nécessaires pour faire communiquer deux machines.

```
┌──────────────────────────────┐
│ 7. Application               │  HTTP, DNS, SSH, ping
├──────────────────────────────┤
│ 6. Présentation              │
├──────────────────────────────┤
│ 5. Session                   │
├──────────────────────────────┤
│ 4. Transport                 │  TCP, UDP
├──────────────────────────────┤
│ 3. Réseau                    │  IP, ICMP, IPv4
├──────────────────────────────┤
│ 2. Liaison de données        │  Ethernet, ARP...
├──────────────────────────────┤
│ 1. Physique                  │  câble, fibre, radio...
└──────────────────────────────┘
```

`ping` est un programme qui utilise principalement le protocole ICMP, lui-même situé à la couche Réseau.


## Les différents protocoles utilisés pour ping
Pour ton README, je ne ferais pas une liste gigantesque. Concentre-toi sur ceux qui permettent de comprendre ping.

IPv4

Internet Protocol version 4

Il permet notamment l'adressage et le routage des paquets.

Un paquet IPv4 contient notamment :
```

┌──────────────────────┐
│ IPv4 Header          │
├──────────────────────┤
│ Payload              │
└──────────────────────┘
```

Dans ton cas, le payload est un paquet ICMP :
```

┌──────────────────────┐
│ IPv4 Header          │
├──────────────────────┤
│ ICMP Header          │
├──────────────────────┤
│ ICMP Payload         │
└──────────────────────┘
```

ICMP

Internet Control Message Protocol

Il est utilisé pour envoyer des messages de contrôle et d'erreur au niveau IP.

C'est le protocole utilisé par ping.

ARP

Address Resolution Protocol

Il permet notamment de trouver l'adresse MAC correspondant à une adresse IPv4 sur un réseau local.

Tu n'as pas besoin de l'implémenter dans ft_ping, mais c'est intéressant de le mentionner parce que le paquet IPv4 doit finalement être transporté sur une couche de liaison.

# ping fonctionnement

## Sockets RAW

Une socket RAW est un type de socket permettant d'interagir directement avec la couche réseau. Elle peut être utilisée pour fabriquer des paquets personnalisés (comme ICMP).

ping utilise une socket RAW :
``` c
socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
```

Schématiquement :
```
Application
     │
     │ socket RAW ICMP
     ▼
┌───────────────┐
│ Linux Kernel  │
└───────┬───────┘
        │
        ▼
IPv4 + ICMP
```

Dans ton cas, à la réception, tu peux accéder au header IPv4 :

```
buffer
  ↓
IPv4 header
  ↓
ICMP header
  ↓
payload
```

C'est notamment ce qui te permet de récupérer :

le TTL ;
l'IP source ;
l'IP destination ;
le protocole ;
le header ICMP ;
l'identifier ;
la séquence ;
le payload.


## icmp packet description

Le ICMP (Internet Control Message Protocol) est un protocole qui opère à la Couche 3 (Couche Réseau) du modèle OSI (Open Systems Interconnection)
    fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

```
AF_INET
   │
   └──► IPv4
```

ping utilise principalement deux messages ICMP :

```
Echo Request
      │
      ▼
   machine
      │
      ▼
Echo Reply
```

Pour IPv4 :

ICMP Echo Request
Type = 8
Code = 0

et :

ICMP Echo Reply
Type = 0
Code = 0
Structure ICMP Echo

```
byte  0               1               2               3
bits  0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     | Type          |   Code (0)    |           Checksum            |   <----
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+       |---- header icmp
     |           Identifier          |        Sequence Number        |   <----
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |                          Data / Payload                       |   <---- payload
```

Type : Indique le type de message.

8 → Echo Request
0 → Echo Reply

Code : Pour les Echo Request/Reply :
Code = 0

Checksum : Permet de détecter une corruption du message ICMP.

Identifier : Permet notamment d'identifier les requêtes appartenant à une instance de ping.

Sequence Number : Permet d'identifier les différents paquets 
C'est notamment ce qui te permet de savoir quelle réponse correspond à quelle requête.


Workflow : 
```
                     ft_ping
                        │
                        │ 1. construit ICMP Echo Request
                        ▼
              ┌───────────────────┐
              │ ICMP Echo Request │
              │ type = 8          │
              │ seq = 0            │
              └─────────┬─────────┘
                        │
                        ▼
              ┌───────────────────┐
              │    IPv4 Header    │    <--- kernel encapsule le icmp echo request dans un ipv4 header
              ├───────────────────┤
              │ ICMP Echo Request │
              └─────────┬─────────┘
                        │
                        │ réseau
                        ▼
                  Destination
                        │
                        │ répond
                        ▼
              ┌───────────────────┐
              │    IPv4 Header    │
              ├───────────────────┤
              │  ICMP Echo Reply  │
              └─────────┬─────────┘
                        │
                        ▼
                     ft_ping
                        │
                        ├── vérifie l'adresse
                        ├── vérifie l'ID
                        ├── vérifie la séquence
                        ├── vérifie le checksum
                        ├── récupère le TTL
                        └── calcule le RTT
```

### packet send

schema dun packet :
```
byte  0               1               2               3
bits  0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     | Type (8=Req)  |   Code (0)    |           Checksum            |   <----
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+       |---- header icmp
     |           Identifier          |        Sequence Number        |   <----
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |                          Data / Payload                       |   <---- payload
```

structure :  
header icmp :
``` h
/* from #include <netinet/ip_icmp.h> */
struct icmphdr
{
  uint8_t type;		/* message type */
  uint8_t code;		/* type sub-code */
  uint16_t checksum;
  union
  {
    struct
    {
      uint16_t	id;
      uint16_t	sequence;
    } echo;			/* echo datagram */
    uint32_t	gateway;	/* gateway address */
    struct
    {
      uint16_t	__glibc_reserved;
      uint16_t	mtu;
    } frag;			/* path mtu discovery */
  } un;
};

```

packet :
``` c
/* from send.c */

struct ping_packet
{
    struct icmphdr icmp_hdr;
    char payload[PAYLOAD_SIZE];
};

```

build the packet:
``` c
/* from send.c */
struct ping_packet packet = {
        .icmp_hdr = {
            .type = ICMP_ECHO,
            .code = 0,
            .checksum = 0,
            .un = {.echo = {.id = htons(ping->pid), .sequence = htons(ping->sequence_number)}}},
        .payload = {0}};

gettimeofday(start_time, NULL);
memmove((char *)&packet + sizeof(struct icmphdr), start_time, sizeof(*start_time)); // Copy the start time into the payload
packet.icmp_hdr.checksum = calculate_checksum((unsigned short *)&packet, sizeof(packet));
```
- build a packet with the pid of the program, sequence of each packet (increment after sending)
- copy the timestamp in the payload of the packet
- calculate the checksum 

envoie des packets de taille 64 (header icmp size : 8 + payload 56)

### description du packet recu
schema du packet recu  :
```
┌──────────────────────────────────────────────────────────┐
│                    IPv4 HEADER                           │
│                      20 bytes                            │
│                                                          │
│  Version │ IHL │ ... │ TTL │ Protocol │ Checksum │ ...  │    <--- add by kernel
│                                                          │
│  Source IP      : 127.0.0.1                              │
│  Destination IP : 127.0.0.1                              │
└──────────────────────────────────────────────────────────┘
                         ↓
┌──────────────────────────────────────────────────────────┐
│                    ICMP HEADER                           │
│                       8 bytes                            │
│                                                          │
│  Type │ Code │       Checksum                            │
│                                                          │
│  Identifier │ Sequence Number                            │
└──────────────────────────────────────────────────────────┘
                         ↓
┌──────────────────────────────────────────────────────────┐
│                    ICMP PAYLOAD                          │
│                       56 bytes                            │
│                                                          │
│  timestamp / données que tu as envoyées                  │
│  ...                                                     │
└──────────────────────────────────────────────────────────┘
```

structure du packet recu :
``` h
struct iphdr
  {
#if __BYTE_ORDER == __LITTLE_ENDIAN
    unsigned int ihl:4;
    unsigned int version:4;
#elif __BYTE_ORDER == __BIG_ENDIAN
    unsigned int version:4;
    unsigned int ihl:4;
#else
# error	"Please fix <bits/endian.h>"
#endif
    uint8_t tos;
    uint16_t tot_len;
    uint16_t id;
    uint16_t frag_off;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t check;
    uint32_t saddr;
    uint32_t daddr;
  };
```

retrouver le header icmp a partir du packet recu :

1. size header ipv4
``` c
    struct iphdr *ip_hdr = (struct iphdr *)buffer;
```
IHL signifie Internet Header Length, mais il est exprimé en blocs de 32 bits (4 octets).
La taille du ip hdr est la suivante : nombre d'ihl * taille d'ihl
ip_hdr->ihl * 4
a savoir que pour un header ipv4 valide minimum ihl fera minimum 5
2. header icmp
```c
struct icmphdr *icmp = (struct icmphdr *)(buffer + ip_hdr->ihl * 4);
```
buffer + size ipv4
3. extraire le timestamp du payload
``` c
 char payload[56] = {0};
    memmove(payload, buffer + ip_hdr->ihl * 4 + sizeof(struct icmphdr), sizeof(payload));
    struct timeval *sent_time = (struct timeval *)payload;
```


### TTL

Je rajouterais absolument une section sur le TTL puisque tu as travaillé dessus.

TTL = Time To Live

Le TTL est un champ du header IPv4 :

IPv4 Header
┌─────────────────────────────┐
│ ...                         │
│ TTL = 64                    │
│ Protocol = ICMP             │
│ ...                         │
└─────────────────────────────┘

Le TTL est décrémenté lorsqu'un paquet traverse un routeur.

Exemple :

PC                    Routeur                 Destination
 │                       │                         │
 │ TTL = 64              │                         │
 ├──────────────────────►│                         │
 │                       │ TTL = 63                │
 │                       ├────────────────────────►│
 │                       │                         │

Cela permet notamment d'éviter qu'un paquet bloqué dans une boucle de routage circule indéfiniment.

Dans ton programme, avec un paquet reçu :

struct iphdr *ip_hdr = (struct iphdr *)buffer;

printf("TTL = %u\n", ip_hdr->ttl);

ttl est un champ de 8 bits, donc tu n'as pas besoin de ntohs().


### RTT et statistiques

mesure de temps entre lenvoie et la reception de deux paquets
min : le minimum de temps 
avg : le temps moyen 
max : le maximum de temps
stddev : description + calcul


Exemple :
```
here insert rtt exemple
```


## conversion :
Host To Network Short
sert quand TOI tu prends une valeur de ta machine et que tu veux la mettre dans un paquet réseau.

nous on a un lil endian donc on doit le convertir en big pour envoyer
on recoit un big et on doit convertir en petit pour imprimer
little endian = poids faible a droite
big endian = poids fort a droite
reseau = big endian
host = lil endian
Network To Host Short
sert quand tu lis une valeur provenant du paquet réseau et que tu veux l'utiliser comme entier sur ta machine.

Sur une machine little-endian, en mémoire tu as :

adresse →
       +----+----+
       | 34 | 12 |
       +----+----+

Alors que le réseau utilise le network byte order, qui est big-endian :

       +----+----+
       | 12 | 34 |
       +----+----+



## retrouver le code de ping :
```
apt source inetutils
```
apres il faut faire `./configure` et `make`


# environment :
description de la VM :  
host : debian bullseye, built with qemu

ping de inetutils-2.0 (ping -V)
```
➜  ft_ping git:(main) ✗ ping -V
ping (GNU inetutils) 2.0
Copyright (C) 2021 Free Software Foundation, Inc.
License GPLv3+: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>.
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.

Written by Sergey Poznyakoff.
```

# tree : 
```
.
├── main.c
├── Makefile
├── README.md
├── include
│   └── ft_ping.h
└── src
    ├── loop.c
    ├── parse_opts.c
    ├── receive.c
    ├── resolve_dest.c
    └── send.c

```
# ping fonctionnement :
## operations permitted and descriptions
-? -h --help : display help
-v, --verbose : enable verbose output (view error from packet and pid)
-w, --timeout=N : stop after N seconds
-W, --linger=N : (redefinition of the description for better understanding in the ping implementation)  time in second where ping wait for icmp packet reply to respond after the sending all of the packets, original definition : "number of seconds to wait for response"
dans le ping de inetutils 2.0 :
```
	  if (!ping->ping_count || ping->ping_num_xmit < ping->ping_count)
	  else if (finishing)
	  else
	    {
	      finishing = 1;
	      intvl.tv_sec = linger;
	    }
```

-i, --interval=NUMBER :      wait NUMBER seconds between sending each packet 
-c, --count=NUMBER :        stop after sending NUMBER packets (and accessories wait for linger seconds)
-t, --ttl=N :               specify N as time-to-live (set with setsockopt)

ping need to be launched with root permission (sudo) because it needs to be able to create socket raw (capacity CAP_NET_RAW reserved to root)

ctrl C handled


# simuler un environment de reseau pour ping :
explications des commandes netem
exemples de commandes netem

# liste de tests interessants :
ecrire des tests
pour tester le -W il faut mettre un delay de 10000ms et tester avec -W petit comme ca ca permet de voir que cest bien a la fin du dernier paquet que le -W compte
tester -c => simple suffit de mettre un chiffre

# comment utiliser ping :
clone etc
