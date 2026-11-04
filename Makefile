CC = gcc
CFLAGS = -Wall -Wextra -std=gnu11 -Wpedantic -pthread

.PHONY: all check clean

all: sync_car

sync_car: sync_car.c sync_car.h sync_car-runner.c
	$(CC) $(CFLAGS) sync_car.c sync_car-runner.c -o sync_car
	
check: sync_car.c sync_car.h
	$(CC) $(CFLAGS) -fsyntax-only sync_car.c sync_car.h

clean:
	rm -f sync_car *.o