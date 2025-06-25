CC = gcc
CFLAGS = -Wall
LIBRARY = libksocket.a

all: $(LIBRARY) initksocket user1 user2
#run2 should be executed before run1
run1: user1
	./user1 127.0.0.1 8081 127.0.0.1 5076

run2: user2
	./user2 127.0.0.1 5076 127.0.0.1 8081

#run4 should be executed before run3.Use run3 and run4 to test simultaneous connections
run3: user1
	./user1 127.0.0.1 8082 127.0.0.1 5077

run4: user2
	./user2 127.0.0.1 5077 127.0.0.1 8082



$(LIBRARY): ksocket.o
	ar rcs $(LIBRARY) ksocket.o

user1: user1.o $(LIBRARY)
	$(CC) $(CFLAGS) -o user1 user1.o -L. -lksocket 

user1.o: user1.c ksocket.h
	$(CC) $(CFLAGS) -c user1.c

user2: user2.o $(LIBRARY)
	$(CC) $(CFLAGS) -o user2 user2.o -L. -lksocket 

user2.o: user2.c ksocket.h
	$(CC) $(CFLAGS) -c user2.c

initksocket: initksocket.o $(LIBRARY)
	$(CC) $(CFLAGS) -o initksocket initksocket.o -L. -lksocket -pthread

initksocket.o: initksocket.c ksocket.h
	$(CC) $(CFLAGS) -c initksocket.c

ksocket.o: ksocket.c ksocket.h
	$(CC) $(CFLAGS) -c ksocket.c

clean:
	rm -f *.o user1 user2 $(LIBRARY) initksocket
