/**
 * server.js — RSX102 TP
 *
 * Serveur WebSocket pédagogique avec logs détaillés.
 * Affiche chaque étape de la négociation (upgrade HTTP → WebSocket)
 * et diffuse les messages reçus à tous les clients connectés (broadcast).
 *
 * Installation : npm install
 * Lancement    : node server.js
 * Test client  : ouvrir http://localhost:3000 dans un navigateur
 */

const http      = require('http');
const fs        = require('fs');
const path      = require('path');
const WebSocket = require('ws');

const HTTP_PORT = 3000;
const WS_PORT   = 8080;

/* ------------------------------------------------------------------ */
/*  Couleurs terminal                                                   */
/* ------------------------------------------------------------------ */
const C = {
    reset:  '\x1b[0m',
    cyan:   '\x1b[36m',
    green:  '\x1b[32m',
    yellow: '\x1b[33m',
    red:    '\x1b[31m',
    gray:   '\x1b[90m',
    bold:   '\x1b[1m',
};

function log(color, prefix, msg) {
    const ts = new Date().toISOString().slice(11, 23);
    console.log(`${C.gray}${ts}${C.reset} ${color}${prefix}${C.reset} ${msg}`);
}

/* ------------------------------------------------------------------ */
/*  Serveur HTTP — sert la page client HTML                            */
/* ------------------------------------------------------------------ */
const httpServer = http.createServer((req, res) => {
    if (req.url === '/' || req.url === '/index.html') {
        res.writeHead(200, { 'Content-Type': 'text/html; charset=utf-8' });
        res.end(fs.readFileSync(path.join(__dirname, 'client.html')));
    } else {
        res.writeHead(404);
        res.end('Not found');
    }
});

httpServer.listen(HTTP_PORT, () => {
    log(C.green, '[HTTP]', `Serveur démarré → http://localhost:${HTTP_PORT}`);
});

/* ------------------------------------------------------------------ */
/*  Serveur WebSocket                                                   */
/*                                                                      */
/*  La bibliothèque 'ws' gère :                                         */
/*    1. L'Upgrade HTTP → WebSocket (handshake RFC 6455)               */
/*    2. Le framing des messages (opcodes, masquage côté client)        */
/*    3. Les pings/pongs de keepalive                                   */
/* ------------------------------------------------------------------ */
const wss = new WebSocket.Server({
    port: WS_PORT,

    /*
     * handleProtocols : permet de choisir un sous-protocole
     * parmi ceux proposés par le client dans le header
     * Sec-WebSocket-Protocol.
     */
    handleProtocols: (protocols) => {
        if (protocols.has('rsx102')) return 'rsx102';
        return false;
    },

    /*
     * verifyClient : hook appelé pendant le handshake HTTP.
     * Permet de rejeter des connexions (authentification, CORS…).
     */
    verifyClient: (info) => {
        log(C.yellow, '[WS]  ', `Tentative de connexion depuis ${info.req.socket.remoteAddress}`);
        log(C.gray,   '      ', `  Origin  : ${info.req.headers['origin'] || '(non défini)'}`);
        log(C.gray,   '      ', `  Upgrade : ${info.req.headers['upgrade']}`);
        log(C.gray,   '      ', `  Sec-WebSocket-Key : ${info.req.headers['sec-websocket-key']}`);
        return true; /* Accepte toutes les connexions */
    }
});

wss.on('listening', () => {
    log(C.cyan, '[WS]  ', `Serveur WebSocket démarré → ws://localhost:${WS_PORT}`);
    console.log('');
    console.log(`${C.bold}  Ouvrez http://localhost:${HTTP_PORT} dans votre navigateur${C.reset}`);
    console.log(`  Puis observez la trace réseau dans Wireshark (filtre : websocket)`);
    console.log('');
});

/* ── Gestion des connexions entrantes ─────────────────────────────── */
let clientCount = 0;

wss.on('connection', (ws, req) => {
    clientCount++;
    const clientId = `client-${clientCount}`;
    const ip = req.socket.remoteAddress;

    log(C.green, '[WS]  ', `✓ ${clientId} connecté (${ip}) — ${wss.clients.size} client(s) actif(s)`);
    log(C.gray,  '      ', `  Protocole négocié : ${ws.protocol || '(aucun)'}`);

    /* Message de bienvenue */
    ws.send(JSON.stringify({
        type:    'welcome',
        id:      clientId,
        message: `Bienvenue ${clientId} ! ${wss.clients.size} client(s) connecté(s).`,
        time:    new Date().toISOString()
    }));

    /* ── Réception d'un message ─────────────────────────────────── */
    ws.on('message', (data, isBinary) => {
        const raw = isBinary ? `[binaire ${data.length} octets]` : data.toString();
        log(C.cyan, '[MSG] ', `${clientId} → "${raw}"`);

        let response;
        try {
            const msg = JSON.parse(raw);
            response = JSON.stringify({
                type:   'echo',
                from:   clientId,
                data:   msg,
                time:   new Date().toISOString()
            });
        } catch {
            /* Message texte simple */
            response = JSON.stringify({
                type:   'echo',
                from:   clientId,
                data:   raw,
                time:   new Date().toISOString()
            });
        }

        /* Broadcast à tous les clients connectés */
        let delivered = 0;
        wss.clients.forEach(client => {
            if (client.readyState === WebSocket.OPEN) {
                client.send(response);
                delivered++;
            }
        });
        log(C.gray, '      ', `  Diffusé à ${delivered} client(s)`);
    });

    /* ── Ping / Pong (keepalive) ────────────────────────────────── */
    ws.on('ping', () => {
        log(C.gray, '[PING]', `${clientId} ping reçu — pong envoyé`);
    });

    /* ── Fermeture ──────────────────────────────────────────────── */
    ws.on('close', (code, reason) => {
        log(C.yellow, '[WS]  ', `${clientId} déconnecté — code ${code} "${reason || 'aucune raison'}"`);
        log(C.gray,   '      ', `  ${wss.clients.size} client(s) restant(s)`);

        /* Notifie les autres clients */
        const notif = JSON.stringify({
            type:    'leave',
            id:      clientId,
            clients: wss.clients.size,
            time:    new Date().toISOString()
        });
        wss.clients.forEach(client => {
            if (client.readyState === WebSocket.OPEN) {
                client.send(notif);
            }
        });
    });

    /* ── Erreurs ────────────────────────────────────────────────── */
    ws.on('error', (err) => {
        log(C.red, '[ERR] ', `${clientId} : ${err.message}`);
    });
});
