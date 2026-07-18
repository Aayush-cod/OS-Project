# Makefile for Task 2: Memory Management Simulation
# -----------------------------------------------------
# Usage:
#   make            -> builds all 4 programs
#   make run1..run4 -> builds and runs a specific program
#   make clean      -> removes all compiled binaries

CC = gcc
CFLAGS = -Wall

PROGRAMS = 1_paging_system 2_fifo_replacement 3_lru_replacement 4_comparison

all: $(PROGRAMS)

%: %.c
	$(CC) $(CFLAGS) $< -o $@

run1: 1_paging_system
	./1_paging_system

run2: 2_fifo_replacement
	./2_fifo_replacement

run3: 3_lru_replacement
	./3_lru_replacement

run4: 4_comparison
	./4_comparison

clean:
	rm -f $(PROGRAMS)

.PHONY: all clean run1 run2 run3 run4