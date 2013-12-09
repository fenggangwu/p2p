#include "peerlist.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

int peerlistinit(struct peerlist* peerlistp){
  peerlistp->cnt = 0;
  peerlistp->head = NULL;
  return 0;
}

int peerlistclear(struct peerlist* peerlistp){
  struct ipport* temp;
  peerlistp->cnt = 0;
  while(peerlistp->head){
    temp = peerlistp->head;
    peerlistp->head = temp->next;
    free(temp);
  }
  return 0;
}

/* return the list length  */
int peerlistinsert(struct peerlist* peerlistp, 
		   unsigned short port, 
		   char * ip){
  struct ipport* ipportp = (struct ipport*)malloc(sizeof(struct ipport));
  ipportset(ipportp, port, ip);  

  printf("inserting (%hu, %s)...\n", port, ip);

  if(peerlistsearch(peerlistp, port, ip)){
    printf("insert (%hu, %s) failed.\n", port, ip);
    return -1;
  }
  ipportp->next = peerlistp->head;
  peerlistp->head = ipportp;
  peerlistp->cnt++;

  printf("inserted (%hu, %s).\n", port, ip);
  return peerlistp->cnt;
}


/* return: cnt on success, -1 on failure (not found)*/
int peerlistdelete(struct peerlist* peerlistp, 
		   unsigned short port, char* ip){
  struct ipport *ptr, *prevptr;
  printf("deleting (%hu, %s)...\n", port, ip);
  for(ptr=peerlistp->head, prevptr=NULL; ptr!=NULL; ptr=ptr->next){
    //    printf("ptr=%ld, prevptr=%ld\n", (long)ptr, (long)prevptr);
    //   ipportprint(ptr);
    //    ipportprint(ipportp);
    if(!ipportcompare2(ptr, port, ip)){ /* node found */
      if(ptr == peerlistp->head){
	//	printf("ifclause\n");
	peerlistp->head = ptr->next;
	free(ptr);
      }else{
	//	printf("elseclause\n");
	prevptr->next = ptr->next;
	ipportprint(ptr);
	//	printf("FlagA\n");
	free(ptr);
	//	printf("FlagB\n");
      }
      peerlistp->cnt --;
      printf("deleted (%hu, %s).\n", port, ip);
      return peerlistp->cnt;
    }
    prevptr = ptr;
  }
  printf("delete (%hu, %s) failed.\n", port, ip);
  return -1;
}

/* return 1 if found, 0 if not found*/
int peerlistsearch(struct peerlist* peerlistp, 
		   unsigned short port, char* ip){
  struct ipport* ptr;
  for (ptr=peerlistp->head; ptr!=NULL; ptr=ptr->next){
    printf("searching for (%hu, %s)\n",port, ip);
    //    ipportprint(ptr);
    if (!ipportcompare2(ptr, port, ip)){
      printf("search (%hu, %s) found\n", port, ip);
      return 1;
    }
  }
  printf("search (%hu, %s) not found\n", port, ip);
  return 0;
}


/* return a randomly picked neighbor */
/* if the list is empty, return NULL*/
struct ipport* peerlistrand(struct peerlist* peerlistp){
  int r, i;
  struct ipport* ptr;
  if(peerlistp->cnt == 0)
    return NULL;
  srand(time(NULL));
  r = rand()%peerlistp->cnt;
  for (ptr=peerlistp->head, i=0; i<r; ptr=ptr->next, i++)
    ;
  
  printf("randomly pick %d-th nbr (%hu, %s)\n", 
	 r, ptr->port, inet_ntoa(ptr->addr));
  return ptr;
}


int peerlistprint(struct peerlist* peerlistp){
  struct ipport* ptr;
  printf("--dumping peerlist--\n");
  printf("cnt=%d\n", peerlistp->cnt);
  for (ptr=peerlistp->head; ptr!=NULL; ptr=ptr->next){
    ipportprint(ptr);
  }
  printf("--dumping ends------\n");
  return 0;
}


int ipportcompare (struct ipport* pa, struct ipport* pb){
  if(pa->addr.s_addr == pb->addr.s_addr)
    return (pa->port - pb->port);
  return pa->addr.s_addr - pb->addr.s_addr;
}

int ipportcompare2 (struct ipport* pa, unsigned short port, char* ip){
  //  printf("comparing (%hu, %s)",port, ip);
  //  ipportprint(pa);
  if(strcmp(inet_ntoa(pa->addr), ip)){
    //   printf("ifclause, <%s>, <%s>, %d\n",inet_ntoa(pa->addr), ip, strcmp(inet_ntoa(pa->addr), ip) );
    return strcmp(inet_ntoa(pa->addr), ip);
  }
  //  printf("elseclause\n");
  return pa->port - port;
}

int ipportset(struct ipport* ipportp, unsigned short port, char* ip){
  bzero(ipportp, sizeof(struct ipport));
  ipportp->port = port;
  inet_aton(ip, &(ipportp->addr));
  return 0; 
}


int ipportprint(struct ipport* ipportp){
  printf("(%hu, %s)\n", ipportp->port, inet_ntoa(ipportp->addr));
  return 0;
}




/* char* ipport2str(struct ipport* ipportp){ */
/*   char buf[64]; */
/*   sprintf(buf, "(%s, %hu)", inet_ntoa(ipportp->addr), ipportp->port); */
/*   return buf; */
/* } */
