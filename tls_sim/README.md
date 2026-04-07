# Simulation handshake TLS

## Prérequis
```bash
# Debian/Ubuntu
sudo apt install libssl-dev

# macOS
brew install openssl
```

## Compilation
```bash
make
```

## Usage
```bash
./tls_sim [host] [port]

# Exemples
./tls_sim example.com 443
./tls_sim google.com 443
./tls_sim localhost 8443
```

## Ce que ça fait
1. **Résolution DNS** via `getaddrinfo()` (IPv4 + IPv6)
2. **Connexion TCP** standard
3. **ClientHello** : `SSL_connect()` déclenche le handshake complet
   - Envoie les versions TLS supportées, cipher suites, random client
4. **ServerHello + Certificate** : reçus et vérifiés automatiquement
5. **Vérification du certificat** via les CA système
6. **Session chiffrée** : affiche version TLS, cipher suite, infos certificat
7. **Requête HTTP** envoyée chiffrée, réponse affichée

## Points clés du code
- `TLS_client_method()` : négocie TLS 1.2 ou 1.3 automatiquement
- `SSL_set_tlsext_host_name()` : active SNI (obligatoire en prod)
- `SSL_CTX_set_verify(VERIFY_PEER)` : vérifie le certificat serveur
- `SSL_shutdown()` : fermeture propre (envoi d'un `close_notify`)

## Observer avec Wireshark
```
ssl.handshake
```
Vous verrez les messages TLS mais le contenu sera chiffré.
Le certificat est visible dans le paquet Certificate (avant chiffrement).
