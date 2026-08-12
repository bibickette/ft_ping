# pourquoi ping a besoin detre lancé avec root

Parce que ping a historiquement besoin de pouvoir créer un socket ICMP raw :
`socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);`
Or, sous Linux, créer un SOCK_RAW nécessite généralement la capacité :
`CAP_NET_RAW`
qui est normalement réservée à root.

## Pourquoi un raw socket est privilégié ?

Avec un raw socket, ton programme manipule directement les paquets ICMP :
``` sh
    Application
        ↓
    SOCK_RAW
        ↓
    ICMP
        ↓
    IP
        ↓
    réseau
```
## Mais ping moderne n'a pas forcément besoin de root

C'est important dans ton cas : un ping peut fonctionner sans root aujourd'hui.
Linux possède les ICMP datagram sockets :
`socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);`
Ils permettent d'envoyer des Echo Request sans donner à l'application tous les privilèges d'un raw socket.
C'est d'ailleurs probablement lié à ce que tu avais observé au début avec ton passage de :
`SOCK_RAW`
à :
`SOCK_DGRAM`
Avec SOCK_DGRAM, le kernel prend davantage en charge la construction du paquet IP/ICMP.



# Conversion : Règle très simple à retenir
Tu peux retenir cette flèche :
``` sh
                 ENVOI
TOI ──────────────────────────→ RÉSEAU
           htons / htonl


                 RECEPTION
TOI ←────────────────────────── RÉSEAU
           ntohs / ntohl
```
Donc :
Situation	Conversion
Je mets un uint16_t dans un paquet	htons()
Je lis un uint16_t depuis un paquet	ntohs()
Je mets un uint32_t dans un paquet	htonl()
Je lis un uint32_t depuis un paquet	ntohl()
uint8_t	rien





# Time : Pourquoi les while ?

Parce que les microsecondes peuvent devenir négatives.
Exemple :
```prochain envoi = 11 s + 200000 µs
now           = 10 s + 700000 µs```

On fait naïvement :
```sec  = 11 - 10 = 1
usec = 200000 - 700000 = -500000```

On obtient :
`1 s - 500000 µs`
Ce n'est pas la représentation correcte d'un timeval.
Donc :
```while (resp_time->tv_usec < 0)
{
    resp_time->tv_usec += 1000000;
    resp_time->tv_sec--;
}
```

transforme :
`1 s - 500000 µs`
en :
`0 s + 500000 µs`

Parce que :
`1 seconde = 1 000 000 µs`
Donc le temps restant est bien :
`500 ms`


# la