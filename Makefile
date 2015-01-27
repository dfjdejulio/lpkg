CC= gcc
CFLAGS= -O2
OBJS= main.o crc16.o fram.o

all: lpkg
lpkg: $(OBJS)
	$(CC) -o lpkg $(OBJS)
	
clean: 
	rm -f *.o
