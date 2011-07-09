CC=gcc
CFLAGS=-g -Wall 
INCLUDES=-Iinclude
CLIENT_LIB=-lpthread -lrt
COMMON_OBJ=$(patsubst src/common/%.c,obj/common/%.o,$(wildcard src/common/*.c))
CLIENT_OBJ=$(patsubst src/client/%.c,obj/client/%.o,$(wildcard src/client/*.c))
BOOT_OBJ=$(patsubst src/bootstrap/%.c,obj/bootstrap/%.o,$(wildcard src/bootstrap/*.c))

all: boot client

boot: ${BOOT_OBJ} ${COMMON_OBJ}
	${CC} ${CFLAGS} -o bin/bootstrap ${BOOT_OBJ} ${COMMON_OBJ}

client: ${CLIENT_OBJ} ${COMMON_OBJ}
	${CC} ${CFLAGS} -o bin/client ${CLIENT_OBJ} ${COMMON_OBJ} ${CLIENT_LIB}
		
clean:
	rm -f obj/bootstrap/*
	rm -f obj/client/*
	rm -f obj/common/* 
	rm -f bin/client bin/bootstrap

obj/%.o: src/%.c
	${CC} ${CFLAGS} -o $@ -c $< ${INCLUDES}

