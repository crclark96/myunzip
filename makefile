CC = gcc
CFLAGS = -Wall -Wshadow -march=native -g
SRC = $(wildcard *.c)

myunzip: $(SRC)
	gcc -o $@ $^ $(CFLAGS)

clean:
	$(RM) -r myunzip *.dSYM *.o
