# readme organization

# description :
- ping cest quoi ca fait quoi  ca sert a quoi
ping inspired of inetutils 2.0


- les differentes couches de protocole (modele osi)  

- ou se trouve ping dans ces couches  

- les differents protocole de cette couche  

- le protocol de ping  


- retrouver le code de ping :
  ```
  apt source inetutils
  ```
  apres il faut faire `./configure` et `make`

- conversion :
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

## icmp packet description

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


# simuler un environment de reseau pour ping :
explications des commandes netem
exemples de commandes netem

# liste de tests interessants :
ecrire des tests
pour tester le -W il faut mettre un delay de 10000ms et tester avec -W petit comme ca ca permet de voir que cest bien a la fin du dernier paquet que le -W compte
tester -c => simple suffit de mettre un chiffre

# comment utiliser ping :
clone etc
