CC = gcc
CFLAGS = -Wall

PROGRAMS = 1_server_basic 1_client_basic 3_server_concurrent 3_client_concurrent \
           4_server_secure 4_client_secure 6_server_final 6_client_final

all: $(PROGRAMS)

%: %.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(PROGRAMS)

.PHONY: all clean