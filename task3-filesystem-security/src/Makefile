CC = gcc
CFLAGS = -Wall

PROGRAMS = 1_authentication 2_file_operations 3_permissions 4_encryption 5_audit_logging 6_integrated_system

all: $(PROGRAMS)

%: %.c
	$(CC) $(CFLAGS) $< -o $@

run1: 1_authentication
	./1_authentication

run2: 2_file_operations
	./2_file_operations

run3: 3_permissions
	./3_permissions

run4: 4_encryption
	./4_encryption

run5: 5_audit_logging
	./5_audit_logging

run6: 6_integrated_system
	./6_integrated_system

clean:
	rm -f $(PROGRAMS)

.PHONY: all clean run1 run2 run3 run4 run5 run6