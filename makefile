CC=gcc
CFLAGS= -pthread
LFLAGS=-lpthread

FILES=integral.c net.c
COMMON_HEADERS=$(wildcard *.h)
OBJ_FILES=$(patsubst %.c,%.o,$(FILES))

SERVER=server
UNIT=unit

all: exec

exec: $(SERVER) $(UNIT)

$(SERVER): server.c $(OBJ_FILES)
	$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS)

$(UNIT): unit.c $(OBJ_FILES)
	$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

.PHONY: clean

clean:
	rm -rf *.o server unit