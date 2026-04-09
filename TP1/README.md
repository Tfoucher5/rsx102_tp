# RSX102 — TP Réseau : TCP / TLS / WebSocket

Projet pédagogique pour observer et simuler les négociations réseau.

## Structure du projet

```
rsx102_tp/
├── scanner/          # Scanner de ports TCP (C)
├── tcp_sim/          # Simulation 3-way handshake TCP (C, raw sockets)
├── tls_sim/          # Simulation handshake TLS (C + OpenSSL)
├── websocket/        # Serveur WebSocket (Node.js)
└── Makefile          # Build global
```

---

## Prérequis

### Linux (Debian/Ubuntu)
```bash
sudo apt update
sudo apt install -y gcc make libssl-dev nodejs npm wireshark tshark
```

### macOS
```bash
brew install gcc openssl node
# Pour Wireshark : https://www.wireshark.org/download.html
```

---

## Installation et lancement

### 1. Cloner / décompresser le projet
```bash
unzip rsx102_tp.zip
cd rsx102_tp
```

### 2. Compiler tous les programmes C
```bash
make all
```

### 3. Lancer chaque module (voir README dans chaque dossier)
```bash
# Scanner de ports
./scanner/port_scanner 127.0.0.1 1 1024

# Simulation TCP (root requis)
sudo ./tcp_sim/tcp_sim

# Simulation TLS
./tls_sim/tls_sim

# Serveur WebSocket
cd websocket && npm install && node server.js
```

---

## Observation réseau

### Wireshark (interface graphique)
Ouvrir Wireshark, choisir l'interface réseau, puis appliquer les filtres :
- `tcp.flags.syn == 1` — voir les SYN
- `ssl.handshake` — voir le handshake TLS
- `websocket` — voir les trames WebSocket

### TShark (ligne de commande)
```bash
sudo tshark -i lo -f "tcp port 8080" -Y "websocket"
sudo tshark -i eth0 -Y "ssl.handshake" -V
```

### Burp Suite
1. Lancer Burp Suite, aller dans Proxy > Options
2. Configurer le proxy sur `127.0.0.1:8080`
3. Dans le navigateur, configurer le proxy HTTP sur ce port
4. Installer le certificat Burp dans le navigateur pour intercepter HTTPS
5. Les échanges WebSocket apparaissent dans Proxy > WebSockets history
