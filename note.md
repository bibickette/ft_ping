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


# IPv4 = Internet Protocol version 4.

C'est le protocole qui permet notamment d'identifier et d'acheminer des paquets entre des machines grâce aux adresses IPv4.

Par exemple :

127.0.0.1
192.168.1.10
8.8.8.8

Une adresse IPv4 fait 32 bits, généralement écrits sous forme de 4 nombres décimaux :

192 . 168 . 1 . 10
 \____4 octets____/

4 × 8 bits = 32 bits

IPv4 définit également un header qui accompagne chaque paquet.

# 8.8.8.8 and 0.0.0.0

Ce sont deux adresses qui ont des significations très différentes.
ping 8.8.8.8

8.8.8.8 est une adresse IPv4 publique utilisée par le DNS public de Google.

ping 0.0.0.0

Là, c'est complètement différent.

0.0.0.0 est une adresse spéciale, appelée adresse IPv4 non spécifiée (unspecified address).

Elle ne signifie pas :

« une machine dont l'adresse est 0.0.0.0 ».

Elle signifie plutôt :

« aucune adresse / toutes les adresses locales selon le contexte ».

Par exemple, tu peux voir 0.0.0.0 dans :

0.0.0.0

pour dire « toutes les interfaces » lorsqu'un serveur écoute :

0.0.0.0:8080

Cela signifie :

écoute sur toutes les adresses IPv4 locales.

Et pourquoi ton ft_ping 0.0.0.0 t'avait donné 127.0.0.1 ?

C'est justement le cas intéressant que tu avais rencontré.

Tu avais quelque chose comme :

PING 0.0.0.0 (0.0.0.0)
echo reply from unexpected ip : 127.0.0.1

Le système traite 0.0.0.0 d'une manière particulière pour la destination locale. Il peut finalement faire arriver le trafic sur la loopback 127.0.0.1.

Donc tu peux avoir :

Destination demandée :
        0.0.0.0
             │
             ▼
       traitement local
             │
             ▼
        127.0.0.1

Et c'est pour ça que ton test :

if (received_ip != destination_ip)

peut considérer la réponse comme « inattendue », alors que le comportement du système est cohérent.

À retenir
Adresse	Signification
8.8.8.8	Adresse IPv4 publique réelle
127.0.0.1	Loopback : ta propre machine
0.0.0.0	Adresse non spécifiée / adresse spéciale, pas une machine à contacter normalement


# la