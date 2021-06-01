CC = gcc
CFLAGS = -g -Wall

default: aig-basic

aig-basic: main.o depend.o
	$(CC) $(CFLAGS) -o main main.o depend.o

main.o: main.c depend.h
	$(CC) $(CFLAGS) -c main.c

depend.o: depend.c depend.h
	$(CC) $(CFLAGS) -c depend.c

clean:
	$(RM) $(TARGET)
