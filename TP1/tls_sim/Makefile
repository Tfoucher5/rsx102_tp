CC     = gcc
CFLAGS = -Wall -Wextra -O2

# Détection automatique d'OpenSSL (Linux vs macOS Homebrew)
UNAME := $(shell uname)
ifeq ($(UNAME), Darwin)
    # macOS : OpenSSL installé via Homebrew
    OPENSSL_PREFIX := $(shell brew --prefix openssl 2>/dev/null || echo "/usr/local")
    CFLAGS  += -I$(OPENSSL_PREFIX)/include
    LDFLAGS  = -L$(OPENSSL_PREFIX)/lib -lssl -lcrypto
else
    # Linux
    LDFLAGS  = -lssl -lcrypto
endif

all: tls_sim

tls_sim: tls_sim.c
	$(CC) $(CFLAGS) -o tls_sim tls_sim.c $(LDFLAGS)

clean:
	rm -f tls_sim
