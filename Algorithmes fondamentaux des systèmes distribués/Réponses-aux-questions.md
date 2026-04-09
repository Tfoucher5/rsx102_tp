# TP – Algorithmes fondamentaux des systèmes distribués
**RSX102 – Boris Rose**

---

## Exercice 1 : Horloges logiques de Lamport

**Règles de l'algorithme :**
- Événement local : `H = H + 1`
- Envoi d'un message : `H = H + 1`, le message embarque la valeur H
- Réception d'un message : `H = max(H_local, H_reçu) + 1`

| Étape | Processus | Événement | Calcul | Horloge |
|-------|-----------|-----------|--------|---------|
| 1 | P1 | Événement local | 0 + 1 | **H(P1) = 1** |
| 2 | P1 | Envoi message → P2 | 1 + 1 | **H(P1) = 2** (message envoyé avec H=2) |
| 3 | P2 | Réception depuis P1 | max(0, 2) + 1 | **H(P2) = 3** |
| 4 | P2 | Envoi message → P3 | 3 + 1 | **H(P2) = 4** (message envoyé avec H=4) |
| 5 | P3 | Réception depuis P2 | max(0, 4) + 1 | **H(P3) = 5** |

---

## Exercice 2 : Horloges vectorielles

**Règles de l'algorithme :**
- Événement local sur Pi : `V[i] = V[i] + 1`
- Envoi sur Pi : `V[i] = V[i] + 1`, le vecteur complet est joint au message
- Réception par Pj d'un message portant Vm : `V[j] = max(V[j], Vm) + 1` sur la composante j

Le vecteur est noté **(V[P1], V[P2], V[P3])**.

| Étape | Processus | Événement | Calcul | Vecteur résultant |
|-------|-----------|-----------|--------|-------------------|
| Début | — | — | — | **(0, 0, 0)** |
| 1 | P1 | Événement local | V[P1] : 0→1 | **(1, 0, 0)** |
| 2 | P1 | Envoi message → P2 | V[P1] : 1→2 | **(2, 0, 0)** ← joint au message |
| 3 | P2 | Réception depuis P1 | max((0,0,0),(2,0,0)) → V[P2] : 0→1 | **(2, 1, 0)** |
| 4 | P2 | Événement local | V[P2] : 1→2 | **(2, 2, 0)** |

---

## Exercice 3 : Algorithme d'élection du leader (Bully)

### 1. À quels processus P2 envoie-t-il un message ?

P2 envoie un message **ELECTION** à tous les processus d'identifiant **strictement supérieur** au sien. Ici P2 (id=2) contacte donc **P3 (id=3) et P4 (id=4)**.

> Note : P2 n'envoie pas à P1 car 1 < 2.

### 2. Quel processus devient le nouveau leader ?

**P3** devient le nouveau leader.

Déroulement :
- P2 envoie ELECTION à P3 et P4.
- P4 étant en panne, il ne répond pas.
- P3 répond OK à P2 (il a un identifiant plus grand), puis démarre sa propre élection.
- P3 envoie ELECTION à P4 uniquement (seul id > 3) ; P4 ne répond toujours pas.
- P3 se déclare leader et envoie un message COORDINATOR à P1 et P2.

### 3. Pourquoi cet algorithme s'appelle-t-il "Bully" ?

Le terme "Bully" (en anglais : intimidateur, tyran) reflète le comportement central de l'algorithme : **tout processus qui détecte la panne du leader s'impose en lançant une élection, et le processus avec l'identifiant le plus élevé l'emporte toujours**. Les processus à fort identifiant "écrasent" les candidatures des processus à identifiant inférieur, comme un tyran qui impose sa supériorité.

---

## Exercice 4 : Consensus distribué

### 1. Quelle valeur est choisie ?

La valeur **10** est choisie.

S1 propose 10, S2 propose 10, S3 propose 20. La majorité absolue requiert **au moins 2 votes sur 3**. La valeur 10 obtient 2 votes (S1 et S2) contre 1 pour la valeur 20 (S3) → **10 est élue**.

### 2. Que se passe-t-il si S2 tombe en panne ?

Si S2 tombe en panne **avant** de voter, il ne reste que S1 (valeur 10) et S3 (valeur 20) opérationnels. Aucune valeur n'atteint la majorité absolue (2/3). Le consensus **ne peut pas être atteint** et le système se bloque (ou attend un timeout selon le protocole).

> C'est une illustration du **théorème FLP** : dans un système asynchrone, un seul processus défaillant peut empêcher le consensus.

### 3. Combien de serveurs minimum pour tolérer une panne ?

Pour tolérer **f pannes**, il faut au minimum **2f + 1 serveurs** (afin de toujours disposer d'une majorité malgré les défaillances).

Ici, pour tolérer **1 panne** : 2×1 + 1 = **3 serveurs minimum** (ce qui correspond bien à notre exemple).

---

## Exercice 5 : Exclusion mutuelle distribuée

L'algorithme de Lamport pour l'exclusion mutuelle ordonne les requêtes par **timestamp croissant**. En cas d'égalité, l'identifiant du processus sert de critère de départage.

| Processus | Timestamp |
|-----------|-----------|
| P2 | 2 |
| P1 | 5 |
| P3 | 8 |

**Ordre d'accès à la ressource critique :**

1. **P2** (timestamp = 2, le plus faible)
2. **P1** (timestamp = 5)
3. **P3** (timestamp = 8)

---

## Exercice 6 : Réplication et quorum

Configuration : 5 serveurs, quorum de lecture = 2, quorum d'écriture = 3.

### 1. Peut-on lire si 3 serveurs sont en panne ?

Non. Il reste 5 − 3 = **2 serveurs disponibles**, ce qui atteint exactement le quorum de lecture (2). **La lecture est théoriquement possible**, mais en situation limite : la moindre panne supplémentaire bloquerait la lecture.

> Si l'on interprète la question comme « 3 serveurs en panne simultanément », la lecture reste possible avec exactement le nombre requis.

### 2. Peut-on écrire si 2 serveurs sont en panne ?

Non. Il reste 5 − 2 = **3 serveurs disponibles**, ce qui atteint exactement le quorum d'écriture (3). **L'écriture est possible**, mais là encore en situation limite.

> Si un troisième serveur tombe en panne au même moment, l'écriture devient impossible.

### 3. Pourquoi l'écriture nécessite-t-elle plus de confirmations que la lecture ?

L'écriture doit garantir la **cohérence** des données sur l'ensemble du système. En exigeant un quorum d'écriture élevé, on s'assure que :
- Toute écriture future sera visible par n'importe quel quorum de lecture (les deux quorums se « chevauchent » nécessairement : Qr + Qw > N, ici 2 + 3 = 5 > 5 ✓).
- Aucune lecture ne peut renvoyer une valeur obsolète.

La lecture peut se contenter d'un quorum plus faible car elle n'altère pas l'état du système ; elle profite de la cohérence garantie par les écritures.

---

## Exercice 7 : Détection de panne par heartbeat

| Temps | Message |
|-------|---------|
| 0 s | alive |
| 2 s | alive |
| 4 s | alive |
| 6 s | rien |
| 8 s | alive attendu mais absent |

Le serveur envoie un heartbeat **toutes les 2 secondes**. La dernière confirmation reçue date de **t = 4 s**. À **t = 6 s**, aucun message n'arrive (premier silence). À **t = 8 s**, un second heartbeat est attendu et absent.

**On peut suspecter une panne à partir de t = 6 s** (premier délai manqué), soit **2 secondes après le dernier heartbeat reçu**.

En pratique, les systèmes attendent souvent **plusieurs heartbeats manqués** avant de déclarer une panne, afin de réduire les faux positifs liés à des latences réseau passagères. Avec un seuil de 2 absences consécutives, la suspicion serait levée à **t = 8 s**.