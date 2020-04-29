
CC      = gcc
CFLAGS  = -Wall -pthread -std=gnu99

OBJECT  = main.o mysched.o

EXEC    = sched.out

default : $(EXEC)

$(EXEC) : $(OBJECT) 
	$(CC) $(CFLAGS) -o $(EXEC) $(OBJECT)

main.o : main.c mysched.h list.h
	$(CC) $(CFLAGS) -c main.c
mysched.o : mysched.c mysched.h list.h

	$(CC) $(CFLAGS) -c mysched.c

clean :
	$(RM) $(EXEC) *.o *.out
