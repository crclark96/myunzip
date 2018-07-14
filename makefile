CC = gcc
CFLAGS = -Wall -Wshadow -g -march=native
SRC = $(wildcard *.c)

my-unzip: $(SRC)
	gcc -o $@ $^ $(CFLAGS) 
