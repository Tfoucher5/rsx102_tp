# Scanner de ports TCP

## Compilation
```bash
make
```

## Usage
```bash
./port_scanner <ip> <port_debut> <port_fin>

# Exemples
./port_scanner 127.0.0.1 1 1024
./port_scanner 192.168.1.1 20 443
```

## Ce que ça fait
- Crée une socket TCP non-bloquante par port
- Lance `connect()` → retourne immédiatement (EINPROGRESS)
- Attend la réponse du kernel avec `select()` (timeout 300ms)
- Si la socket est prête en écriture → le 3-way handshake TCP a réussi → port ouvert

## Observer avec Wireshark
```
tcp.flags.syn == 1 && ip.dst == 127.0.0.1
```
Vous verrez un SYN par port scanné, et un SYN-ACK uniquement pour les ports ouverts.
