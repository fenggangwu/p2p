all: server client central-server peer testpeerlist messenger
central-server : central-server.c const.h
	gcc  -Wall -Werror -lnsl central-server.c -o central-server
peer : peer.o peerlist.o
	gcc  -Wall -Werror peer.o peerlist.o -o peer
peer.o : peer.c const.h
	gcc -c -Wall -Werror peer.c
server : server.c
	gcc  -Wall -Werror -lnsl server.c -o server
client : client.c const.h
	gcc  -Wall -Werror -lnsl client.c -o client
messenger : messenger.c const.h
	gcc  -Wall -Werror -lnsl messenger.c -o messenger
testpeerlist : testpeerlist.o peerlist.o
	gcc  -Wall -Werror testpeerlist.o peerlist.o -o testpeerlist
peerlist.o : peerlist.c peerlist.h
	gcc -c  -Wall -Werror  peerlist.c
testpeerlist.o : testpeerlist.c peerlist.h
	gcc -c  -Wall -Werror  testpeerlist.c peerlist.h
clean:
	rm *.o \
	server client messenger \
	central-server peer \
	testpeerlist