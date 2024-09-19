all: server client

server: server.o
	@gcc -o server server.c

client: client.o
	@gcc -o client client.c

clean:
	rm -f *.o 
