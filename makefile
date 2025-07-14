CFLAGS = -Wall -Wextra -g

all: client server

client: client.o net_socket.o file_tools.o
	gcc $(CFLAGS) -o $@ $^

server: server.o net_socket.o file_tools.o
	gcc $(CFLAGS) -o $@ $^

%.o: %.c
	gcc $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o client server
