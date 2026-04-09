# Réponses — TP Sliding Window et calculs de performance n°2
**RSX102 — Technologies pour les applications en réseau**

---

## Partie 1 — Observation du sliding window dans Wireshark

**Données :** taille segment = 1460 octets | RTT = 40 ms | fenêtre = 8760 octets

**Q1. Combien de segments TCP peuvent être envoyés sans attendre d'acquittement ?**

```
Nombre de segments = Taille fenêtre / Taille segment
                   = 8760 / 1460
                   = 6 segments
```

**Q2. Combien d'octets peuvent être envoyés sans attendre d'ACK ?**

La taille de la fenêtre TCP correspond exactement au nombre d'octets pouvant être en transit sans acquittement :

```
8760 octets
```

**Q3. Combien d'acquittements sont nécessaires pour transmettre 43800 octets ?**

```
Nombre d'ACKs = Total à transmettre / Taille fenêtre
              = 43800 / 8760
              = 5 acquittements
```

Chaque fenêtre de 8760 octets est acquittée une fois (au minimum) avant que la suivante soit envoyée.

**Q4. Quelle est la relation entre la taille de la fenêtre et le nombre de segments envoyés ?**

Le nombre de segments envoyés sans ACK est directement proportionnel à la taille de la fenêtre et inversement proportionnel à la taille d'un segment :

```
Nombre de segments = Taille fenêtre / Taille segment
```

Plus la fenêtre est grande, plus on peut envoyer de segments simultanément sans attendre d'acquittement, ce qui améliore le débit.

---

## Partie 2 — Numéros de séquence

**Séquences observées :** 1000 | 2460 | 3920 | 5380 | 6840

**Q1. Quelle est la taille d'un segment TCP dans cet exemple ?**

```
Taille = Seq(2) − Seq(1) = 2460 − 1000 = 1460 octets
```

(Identique entre chaque segment : 3920−2460 = 1460, 5380−3920 = 1460, 6840−5380 = 1460 ✓)

**Q2. Combien d'octets sont transmis entre le premier et le dernier segment ?**

Le dernier numéro de séquence (6840) indique le début du 5e segment. Les 4 premiers segments ont déjà été envoyés :

```
Octets transmis = Seq(dernier) − Seq(premier) = 6840 − 1000 = 5840 octets
```

(soit 4 segments × 1460 octets = 5840 octets)

**Q3. Quel sera le prochain numéro de séquence attendu ?**

```
Prochain Seq = 6840 + 1460 = 8300
```

C'est aussi le numéro d'ACK que le récepteur enverra après la réception du 5e segment.

**Q4. Que représente le numéro de séquence dans TCP ?**

Le numéro de séquence représente le **numéro d'ordre du premier octet de données** contenu dans le segment. Il est calculé par rapport à l'ISN (Initial Sequence Number) échangé lors du 3-way handshake. Il permet au récepteur de :
- Remettre les données dans le bon ordre
- Détecter les pertes ou duplications
- Construire l'ACK correspondant (ACK = Seq + taille du segment reçu)

---

## Partie 3 — Calcul du débit théorique

**Données :** fenêtre = 64 KB | RTT = 50 ms | Formule : Débit = TailleFenetre / RTT

**Q1. Convertir la taille de fenêtre en octets**

```
64 KB = 64 × 1024 = 65 536 octets
```

**Q2. Convertir le RTT en secondes**

```
50 ms = 50 / 1000 = 0,050 s
```

**Q3. Calculer le débit maximal théorique en octets par seconde**

```
Débit = 65 536 / 0,050 = 1 310 720 octets/s
```

**Q4. Convertir le résultat en Ko/s**

```
1 310 720 / 1024 = 1 280 Ko/s
```

**Q5. Convertir le résultat en Mb/s**

```
1 310 720 × 8 = 10 485 760 bits/s = 10,49 Mb/s
```

---

## Partie 4 — Influence du RTT

**Données :** fenêtre fixe = 65 535 octets | Formule : Débit = 65535 / RTT

**Q1. Débit maximal si RTT = 20 ms**

```
Débit = 65 535 / 0,020 = 3 276 750 octets/s
      = 3 200 Ko/s ≈ 26,2 Mb/s
```

**Q2. Débit maximal si RTT = 100 ms**

```
Débit = 65 535 / 0,100 = 655 350 octets/s
      = 640 Ko/s ≈ 5,24 Mb/s
```

**Q3. Débit maximal si RTT = 200 ms**

```
Débit = 65 535 / 0,200 = 327 675 octets/s
      = 320 Ko/s ≈ 2,62 Mb/s
```

**Q4. Que peut-on conclure concernant l'influence du RTT sur les performances TCP ?**

| RTT | Débit théorique |
|-----|----------------|
| 20 ms | ≈ 26,2 Mb/s |
| 100 ms | ≈ 5,24 Mb/s |
| 200 ms | ≈ 2,62 Mb/s |

Le RTT est **inversement proportionnel au débit** : doubler le RTT divise le débit par deux. Pour une fenêtre fixe, plus la latence est élevée, plus TCP passe de temps à attendre les ACKs au lieu de transmettre des données. C'est pourquoi l'augmentation de la taille de fenêtre (Window Scaling, RFC 7323) est cruciale sur les liens à fort RTT.

---

## Partie 5 — Fenêtre glissante

**Données :** taille segment = 1000 octets | fenêtre = 5000 octets

**Q1. Combien de segments peuvent être envoyés avant réception d'un ACK ?**

```
Nombre = Fenêtre / Segment = 5000 / 1000 = 5 segments
```

**Q2. Quels sont les numéros de séquence envoyés si le premier numéro est 0 ?**

```
Segment 1 : Seq = 0
Segment 2 : Seq = 1000
Segment 3 : Seq = 2000
Segment 4 : Seq = 3000
Segment 5 : Seq = 4000
```

**Q3. Quel numéro d'acquittement sera envoyé après réception complète des données ?**

```
ACK = dernier Seq + taille segment = 4000 + 1000 = 5000
```

L'ACK = 5000 signifie « j'ai bien reçu tout jusqu'à l'octet 4999, j'attends l'octet 5000 ».

**Q4. Que devient la fenêtre après réception de l'ACK ?**

La fenêtre **glisse** vers l'avant : le transmetteur peut maintenant envoyer les 5 segments suivants (Seq 5000 à 9999). C'est le principe du sliding window : la fenêtre avance au fur et à mesure des ACKs reçus, permettant un flux continu de données.

---

## Partie 6 — Analyse d'une capture Wireshark

**Extrait :** Seq=0 | Seq=1460 | Seq=2920 | Seq=4380 | Ack=5840 | Window Size=5840

**Q1. Combien de segments ont été envoyés ?**

```
4 segments (aux séquences 0, 1460, 2920, 4380)
```

**Q2. Combien d'octets ont été transmis ?**

```
4 × 1460 = 5840 octets
```

**Q3. Pourquoi l'ACK indique 5840 ?**

L'ACK indique le **prochain octet attendu** par le récepteur. Le dernier segment reçu commence à Seq=4380 et fait 1460 octets :

```
ACK = 4380 + 1460 = 5840
```

Cela confirme que les 4 segments (0 à 5839) ont bien été reçus.

**Q4. Quelle est la taille de la fenêtre annoncée par le récepteur ?**

```
Window Size = 5840 octets
```

C'est le récepteur qui annonce cette valeur dans son ACK, indiquant combien d'octets supplémentaires il peut accepter dans son buffer.

**Q5. Que signifie une diminution de la taille de la fenêtre ?**

Une diminution de la fenêtre signifie que le **buffer de réception se remplit** : le récepteur consomme les données moins vite qu'elles n'arrivent (application lente, CPU chargé, etc.). Le transmetteur doit **réduire son débit d'envoi** en conséquence. Si la fenêtre tombe à 0, le transmetteur doit s'arrêter complètement d'envoyer jusqu'à recevoir une mise à jour de fenêtre (Window Update).

---

## Partie 7 — Débit réel

**Données :** fenêtre = 12 000 octets | RTT = 60 ms

**Q1. Calculer le débit maximal théorique**

```
Débit théorique = 12 000 / 0,060 = 200 000 octets/s = 200 Ko/s ≈ 1,6 Mb/s
```

**Q2. Si seulement 8000 octets sont transmis par RTT, quel est le débit réel ?**

```
Débit réel = 8 000 / 0,060 ≈ 133 333 octets/s ≈ 133 Ko/s ≈ 1,07 Mb/s
```

**Q3. Comparer débit réel et débit théorique**

```
Efficacité = Débit réel / Débit théorique = 133 333 / 200 000 ≈ 66,7 %
```

Le débit réel est environ **33 % inférieur** au débit théorique.

**Q4. Proposer une explication possible à la différence observée**

Plusieurs facteurs peuvent expliquer cet écart :
- **Pertes de paquets** : des retransmissions consomment une partie de la bande passante disponible
- **Congestion réseau** : TCP réduit sa fenêtre via les algorithmes de contrôle de congestion (slow start, AIMD)
- **Limitation applicative** : l'application n'alimente pas le buffer d'envoi assez vite
- **Overhead protocolaire** : en-têtes TCP/IP, ACKs, options TCP consomment de la bande passante
- **Buffer du récepteur** : la fenêtre annoncée peut être inférieure aux 12 000 octets attendus

---

## Partie 8 — Synthèse

**Q1. Quel est le rôle principal du sliding window ?**

Le sliding window permet d'**envoyer plusieurs segments en continu** sans attendre un ACK pour chacun. Cela maximise l'utilisation de la bande passante en évitant les temps d'attente inutiles entre chaque segment.

**Q2. Pourquoi TCP n'envoie-t-il pas les données une par une ?**

Sans sliding window (mode stop-and-wait), le transmetteur enverrait un segment, attendrait l'ACK (pendant tout le RTT), puis enverrait le suivant. Pendant le temps d'attente, le lien réseau serait **complètement inutilisé**. Avec RTT=40ms et segments de 1460 octets, le débit serait limité à 1460/0,040 = 36,5 Ko/s, quelle que soit la capacité du lien.

**Q3. Quel est l'impact d'une petite fenêtre TCP ?**

Une petite fenêtre :
- Réduit le nombre de segments en transit simultanément
- Sous-utilise la bande passante disponible
- Limite le débit maximal atteignable (Débit ≤ Fenêtre / RTT)
- Peut être causée par un buffer de réception insuffisant côté récepteur

**Q4. Quel est l'impact d'un RTT élevé ?**

Un RTT élevé :
- Augmente le temps d'attente des ACKs
- Réduit directement le débit (relation inversement proportionnelle)
- Rend les liens satellites ou intercontinentaux très pénalisants malgré une bande passante physique importante
- Nécessite des fenêtres très grandes pour maintenir un bon débit (Window Scaling)

**Q5. Pourquoi le mécanisme de sliding window améliore-t-il les performances ?**

Le sliding window améliore les performances en permettant de **"remplir le tuyau"** réseau : au lieu d'alterner envoi/attente, le transmetteur envoie en continu jusqu'à remplir la fenêtre. Tant que la fenêtre est suffisamment grande (≥ Débit × RTT, appelé Bandwidth-Delay Product), le lien est utilisé à pleine capacité. C'est le principe fondamental du **pipeline TCP**.
