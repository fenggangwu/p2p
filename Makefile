all: server client central-server peer
central-server : central-server.c #const.h
	gcc -lnsl central-server.c -o central-server
peer : peer.c #const.h
	gcc -lnsl peer.c -o peer
server : server.c
	gcc -lnsl server.c -o server
client : client.c
	gcc -lnsl client.c -o client
clean:
	rm server client \
	#central-server peer