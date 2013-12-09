#ifndef PEERLIST_H
#define PEERLIST_H

#include <netinet/in.h>

struct ipport{
  unsigned short port;
  struct in_addr addr;
  struct ipport* next;
};

struct peerlist{
  int cnt;
  struct ipport* head;
};

int peerlistinit(struct peerlist* peerlistp);
int peerlistclear(struct peerlist* peerlistp);
int peerlistinsert(struct peerlist* peerlistp, 
		   unsigned short port, char* ip);
int peerlistdelete(struct peerlist* peerlistp, 
		   unsigned short port, char* ip);
int peerlistsearch(struct peerlist* peerlistp, 
		   unsigned short port, char* ip);
struct ipport* peerlistrand(struct peerlist* peerlistp);
int peerlistprint(struct peerlist* peerlistp);

int ipportcompare (struct ipport* pa, struct ipport* pb);
int ipportcompare2 (struct ipport* pa, unsigned short port, char* ip);
int ipportset(struct ipport* ipportp, unsigned short port, char* ip);

int ipportprint(struct ipport* ipportp);

#endif
