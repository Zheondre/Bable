CC = gcc
INCLUDES = -I/usr/include/modbus -I./modbus
LIBS = -lmodbus -lpthread
CFLAGS = -g -Wall -pthread -std=c11 -o
SOURCES = canItf.c canserver.c canItf.c lib.c ./modbus/genset_modbus.c
OBJECTS = $(SOURCES:.c=.o)
MAIN = canserver

.PHONY: depend clean

all: gccversion $(MAIN)

gccversion:
		@$(CC) --version
		
$(MAIN): $(OBJECTS)
		$(CC) $(CFLAGS) $(INCLUDES) -o $(MAIN) $(OBJECTS) $(LIBS)

.c.o:
		$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@ $(LIBS)

clean:
		$(RM) *.o *~ $(MAIN)
		$(RM) ./modbus/*.o

depend:
		makedepend $(INCLUDES) $(SOURCES)