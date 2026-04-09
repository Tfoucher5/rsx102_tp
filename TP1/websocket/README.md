# Serveur WebSocket

## Installation
```bash
npm install
```

## Lancement
```bash
node server.js
# ou
npm start
```

## Ce qui démarre
- **http://localhost:3000** — page cliente HTML (interface de chat)
- **ws://localhost:8080** — serveur WebSocket (sous-protocole `rsx102`)

## Fonctionnalités
- Logs détaillés dans le terminal : IP cliente, headers Upgrade, Sec-WebSocket-Key
- Broadcast : tout message reçu est diffusé à tous les clients
- Affichage des trames côté client (type, direction, taille)
- Reconnexion automatique côté client

## Observer avec Wireshark
```
tcp.port == 8080
```
Vous verrez :
1. Le 3-way handshake TCP
2. La requête HTTP Upgrade avec `Sec-WebSocket-Key`
3. La réponse `101 Switching Protocols` avec `Sec-WebSocket-Accept`
4. Les trames WebSocket (opcode 0x01 = texte, 0x02 = binaire, 0x08 = close)

## Observer avec Burp Suite
1. Configurer Burp en proxy sur `127.0.0.1:8080` (ou port différent)
2. Configurer le navigateur pour utiliser ce proxy
3. Les échanges WebSocket apparaissent dans **Proxy > WebSockets history**
4. Vous pouvez modifier les trames à la volée

## Structure d'un message
```json
{
  "type": "echo",
  "from": "client-1",
  "data": "Bonjour !",
  "time": "2024-01-15T10:30:00.000Z"
}
```
