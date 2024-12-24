LIBS = -lpthread -luuid

all: linkedList map shm_server server client link

link:
	gcc ./linkedList.o ./map.o ./shm_server.o ./server.o -o ./server.out $(LIBS)
	gcc ./linkedList.o ./map.o ./shm_server.o ./client.o -o ./client.out $(LIBS)

shm_server: ./shm_server/shm_server.c
	gcc -c ./shm_server/shm_server.c -o ./shm_server.o

linkedList: ./linkedList/linkedList.c
	gcc -c ./linkedList/linkedList.c -o ./linkedList.o

map: ./map/map.c
	gcc -c ./map/map.c -o ./map.o

client: ./client.c
	gcc -c ./client.c -o ./client.o

server: ./server.c
	gcc -c ./server.c -o ./server.o

clean: 
	rm -f *.o
	rm -f *.out