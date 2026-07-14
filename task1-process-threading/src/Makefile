# Makefile for Task 1: Process Management and Threading
# ---------------------------------------------------------
# Usage:
#   make            -> builds all 6 programs
#   make run1       -> builds and runs 1_thread_creation
#   make run2       -> builds and runs 2_race_condition
#   ... and so on up to run6
#   make clean      -> removes all compiled binaries

CC = gcc
CFLAGS = -Wall -pthread   # -Wall shows helpful compiler warnings

# List of all program names (without .c extension)
PROGRAMS = 1_thread_creation 2_race_condition 3_synchronization \
           4_deadlock 5_deadlock_prevention 6_scheduler

# Default target: build everything
all: $(PROGRAMS)

# Generic rule: how to build any <name> from <name>.c
%: %.c
	$(CC) $(CFLAGS) $< -o $@

# Convenience targets to build + run each program individually
run1: 1_thread_creation
	./1_thread_creation

run2: 2_race_condition
	./2_race_condition

run3: 3_synchronization
	./3_synchronization

run4: 4_deadlock
	./4_deadlock

run5: 5_deadlock_prevention
	./5_deadlock_prevention

run6: 6_scheduler
	./6_scheduler

# Remove all compiled binaries
clean:
	rm -f $(PROGRAMS)

.PHONY: all clean run1 run2 run3 run4 run5 run6