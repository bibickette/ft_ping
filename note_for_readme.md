

# description :
ping ca fait quoi
les differentes couches de protocole (modele osi)
ou se trouve ping dans ces couches
les differents protocole de cette couche
le protocol de ping


# environment :
description de la VM
host
ping de inetutils-2.0 (ping -V)
➜  ft_ping git:(main) ✗ ping -V
ping (GNU inetutils) 2.0
Copyright (C) 2021 Free Software Foundation, Inc.
License GPLv3+: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>.
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.

Written by Sergey Poznyakoff.

# tree : 

# ping fonctionnement :
flag permis et descriptions

si droit root utilise une socket raw
sinon utilise une socket dgram

description du packet denvoie, le header icmp
envoie des packets de PAYLOAD SIZE

description du packet recu

comment il attend avec select

# simuler un environment de reseau pour ping :
explications des commandes netem
exemples de commandes netem

# liste de tests interessants :
ecrire des tests et leurs results
pour tester le -W il faut mettre un delay de 10000ms et tester avec -W petit comme ca ca permet de voir que cest bien a la fin du dernier paquet que le -W compte
tester -c => simple suffit de mettre un chiffre

# comment utiliser ping :
clone etc
