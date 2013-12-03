#include<stdio.h>
#include"peerlist.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char*argv[]){
  struct peerlist apeerlist, *peerlistp=&apeerlist;

  peerlistinit(peerlistp);
  printf("list length %d\n", peerlistp->cnt);

  printf("insert ipporta\n");
  peerlistinsert(peerlistp, 8980, "127.0.0.1");
  printf("list length %d\n", peerlistp->cnt);


  printf("insert ipporta\n");
  peerlistinsert(peerlistp, 8980, "127.0.0.1");
  printf("list length %d\n", peerlistp->cnt);


  printf("insert ipportb\n");
  peerlistinsert(peerlistp, 5511, "192.168.1.1");
  printf("list length %d\n", peerlistp->cnt);

  peerlistprint(peerlistp);

  printf("delete ipporta\n");
  peerlistdelete(peerlistp, 8980, "127.0.0.1");
  printf("list length %d\n", peerlistp->cnt);

  printf("delete a node not exist\n");
  peerlistdelete(peerlistp, 5511, "127.0.0.1");
  printf("list length %d\n", peerlistp->cnt);

  printf("Search ipporta: %d\n", 
	 peerlistsearch(peerlistp, 8980, "127.0.0.1"));

  printf("Search ipportb: %d\n", 
	 peerlistsearch(peerlistp, 5511, "192.168.1.1"));


  printf("clear the peerlist\n");
  peerlistclear(peerlistp);
  printf("list length %d\n", peerlistp->cnt);


  printf("testpeerlist\n");
  return 0;
}
