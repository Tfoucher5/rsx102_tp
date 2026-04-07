# Simulation 3-way handshake TCP

## Compilation
```bash
make
```

## Usage
```bash
# Nécessite root (raw socket)
sudo ./tcp_sim [ip_dest] [port_dest]

# Exemples
sudo ./tcp_sim 127.0.0.1 8080    # vers le serveur WebSocket local
sudo ./tcp_sim 93.184.216.34 80  # vers example.com
```

## Ce que ça fait
1. Crée une **raw socket** (`SOCK_RAW`) — permet de forger manuellement les en-têtes
2. Active `IP_HDRINCL` — le programme gère lui-même l'en-tête IP
3. **Étape 1** : forge un paquet SYN (seq=1000), calcule le checksum TCP, envoie
4. **Étape 2** : attend le SYN-ACK du serveur avec `select()` (timeout 3s)
5. **Étape 3** : forge le ACK final (ack=server_seq+1), envoie

## Observer avec Wireshark
```
tcp.flags.syn == 1 || tcp.flags.ack == 1
```
Vous verrez les 3 paquets du handshake avec les numéros de séquence.

## Note pédagogique
Sans serveur en écoute sur le port cible, le programme passe en mode
démonstration (simule le SYN-ACK) pour montrer la structure du code.
Pour un vrai échange, lancez d'abord le serveur WebSocket :
```bash
cd ../websocket && npm install && node server.js
```
