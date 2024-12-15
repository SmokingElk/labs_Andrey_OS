CC = gcc
LIBS = -lrabbitmq -lpthread -luuid

all: clean master_node slave_node

master_node: master_node.c
	$(CC) -o master_node.out master_node.c $(LIBS)

slave_node: slave_node.c
	$(CC) -o slave_node.out slave_node.c $(LIBS)

clean:
	rm -f *.txt
	rm -f *.out

run_server:
	sudo systemctl enable rabbitmq-server
	sudo systemctl start rabbitmq-server