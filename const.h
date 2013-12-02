#ifndef CONST_H
#define CONST_H

#include<stdio.h>
#include<stdlib.h>

#define P2P_SVR_PORT 8980
#define MAX_PEER_NUMBER 10



typedef struct {
  int count;
  struct in_addr* peerlist[MAX_PEER_NUMBER];
} peerset_t;

int peersetInit(p_peerset)
     peerset_t *p_peerset;
{
  int i;
  p_peerset->count = 0;
  for (i=0; i<MAX_PEER_NUMBER; i++)
    p_peerset->peerlist[i] =  (struct in_addr *) 0;
  printf("[central-server] peerset constructed, peer_num = %d/%d\n",
	 p_peerset->count, MAX_PEER_NUMBER);
  return 0;
}


int peersetAdd(p_peerset, p_addr)
     peerset_t* p_peerset;
     struct in_addr* p_addr; /*in_addr to be added*/
{
  int i;
  if (p_peerset->count == MAX_PEER_NUMBER) {
    printf("[central-server] fail to add %s\n", inet_ntoa(*p_addr));
    return -1;
  }    

  for (i=0; i<MAX_PEER_NUMBER; i++)
  {
    if (!p_peerset->peerlist[i]) {
      p_peerset->peerlist[i] = p_addr;
      p_peerset->count ++;
      printf("[central-server] added %s, peer_num = %d/%d\n", 
	     inet_ntoa(*p_addr), p_peerset->count, MAX_PEER_NUMBER);
      return 0;
    }
  }

}

struct in_addr* peersetRand(p_peerset)
     peerset_t* p_peerset;
{
  int i, idx;
  time_t t;
  
  if (p_peerset->count <= 0){
    printf("peersetRand failed: count <=0\n");
    return (struct in_addr*)0;
  }

  /* Intializes random number generator */
  srand((unsigned) time(&t));
  idx = rand()%p_peerset->count;

  for (i=0; i<MAX_PEER_NUMBER; i++){
    if (p_peerset->peerlist[i]){
      if (!idx){
	printf("[central-server] random addr %s \
picked. i=%d rand=%d\n",
	       inet_ntoa(*p_peerset->peerlist[i]), i, idx);
	return p_peerset->peerlist[i];
      }else{
	idx-- ;
      }
    }
  }
  printf("peersetRand: some error occured, no addr returned\n");
  return (struct in_addr*)0;
}

#endif
