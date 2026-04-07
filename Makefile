CC      = gcc
CFLAGS  = -Wall -Wextra -O2

.PHONY: all clean scanner tcp_sim tls_sim

all: scanner tcp_sim tls_sim
	@echo ""
	@echo "✓ Compilation terminée."
	@echo "  ./scanner/port_scanner <ip> <port_debut> <port_fin>"
	@echo "  sudo ./tcp_sim/tcp_sim"
	@echo "  ./tls_sim/tls_sim"

scanner:
	$(MAKE) -C scanner

tcp_sim:
	$(MAKE) -C tcp_sim

tls_sim:
	$(MAKE) -C tls_sim

clean:
	$(MAKE) -C scanner clean
	$(MAKE) -C tcp_sim clean
	$(MAKE) -C tls_sim clean
