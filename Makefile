
CC=gcc

CFLAGS=-O3 -Wall -g -pg

LDFLAGS=-L./

all : libcritcache.a critcache-server critcache-client

critbit.o : CFLAGS+=-std=c99

critcache.o : critcache.h

libcritcache.a : critcache.o
	ar r $@ $^

critcache-server : LIBS=-lcritcache

critcache-server : critbit.o critcache-server.o libcritcache.a
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LIBS)

critcache-client : LIBS=-lcritcache

critcache-client : critbit.o critcache-client.o libcritcache.a
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LIBS)

gen_stressclient : LIBS=-lcritcache

gen_stressclient : critbit.o gen_stressclient.o libcritcache.a
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LIBS)

.PHONY:
clean:
	-rm *.o libcritcache.a critcache-server critcache-client
