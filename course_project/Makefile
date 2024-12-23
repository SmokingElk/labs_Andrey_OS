LIBS = -lpthread -luuid

all: shm_server server client link

link:
	gcc ./shm_server.o ./server.o -o ./server.out $(LIBS)
	gcc ./shm_server.o ./client.o -o ./client.out $(LIBS)

shm_server: ./shm_server/shm_server.c
	gcc -c ./shm_server/shm_server.c -o ./shm_server.o

client: ./client.c
	gcc -c ./client.c -o ./client.o

server: ./server.c
	gcc -c ./server.c -o ./server.o

clean: 
	rm -f *.o
	rm -f *.out