CC = gcc
CFLAGS = -Wall -Wextra -std=gnu11 -Wpedantic -pthread

.PHONY: all check test clean

all: sync_car

sync_car: sync_car.c sync_car.h sync_car-runner.c
	$(CC) $(CFLAGS) sync_car.c sync_car-runner.c -o sync_car
	
check: sync_car.c sync_car.h
	$(CC) $(CFLAGS) -fsyntax-only sync_car.c

sync_car-tests: sync_car.c sync_car.h testes/sync_car-tests.c
	$(CC) $(CFLAGS) sync_car.c testes/sync_car-tests.c -Wl,--wrap=pthread_cond_wait -o sync_car-tests

test: sync_car sync_car-tests
	sh testes/executar.sh

clean:
	rm -f sync_car sync_car-tests *.o
